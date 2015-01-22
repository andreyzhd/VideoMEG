/*
 * main.c
 *
 *  Created on: Jun 29, 2010
 *      Author: Andrey Zhdanov
 *      
 *  Usage: SyncDaemon PORTBASE OUTBITS SITE_ID
 *         SyncDaemon PORTBASE OUTBITS
 *      PORTBASE - specifies the address of the parallel port to use, decimal. Usually
 *          12304 (0x3010) or 888 (0x378). Use 'cat /proc/ioports | grep parport' to chech this.
 *      OUTBITS - a number between 1 and 255 that specifies which parallel
 *          port bits wil be used
 *      SITE_ID - site ID (optional)
 *
 *  Copyright (C) 2014 BioMag Laboratory, Helsinki University Central Hospital
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/io.h>
#include <sched.h>


#define SYNC_INTERVAL       10          // seconds
#define SYNC_TRAIN_INTERVAL 15000000    // nanoseconds
#define N_BITS_TS           42          // should be large enough to hold any timestamp (not counting the parity bit)
#define N_BITS_ID           5           // should be large enough to enumerate all the MEG2MEG sites
#define PRIORITY            20

inline int parity(unsigned long long _inp)
{
    unsigned long long  mask;
    int                 res;
    int                 i;
    
    mask = 1;
    res = 0;

    for(i=0; i<sizeof(unsigned long long)*8; i++)
    {
        res ^= (_inp & mask) ? 1 : 0;
        mask *= 2;
    }
    
    return(res);
}



int main(int _argc, char* _argv[])
{
    struct timespec     traintime;
    struct timespec     synctime;       // start of the next sync
    unsigned long long  msec;           // should be at least (N_BITS + 1) bits long
    struct sched_param  sch_param;
    int                 i;
    unsigned int        portbase;
    unsigned char       outbits;
    unsigned int        site_id;
    
    
    // Read the arguments
    if (_argc > 4 || _argc < 3)
    {
        printf("wrong number of parameters (%i instead of 2 or 3), aborting\n", _argc-1);
        return(EXIT_FAILURE);
    }
    
    if (sscanf(_argv[1], "%u", &portbase) != 1)
    {
        printf("cannot read the first argument (portbase), aborting\n");
        return(EXIT_FAILURE);
    }
    
    if (sscanf(_argv[2], "%hhu", &outbits) != 1)
    {
        printf("cannot read the second argument (outbits), aborting\n");
        return(EXIT_FAILURE);
    }
   
    if (_argc == 4)
    {
        if (sscanf(_argv[3], "%u", &site_id) != 1)
        {
            printf("cannot read the third argument (site ID), aborting\n");
            return(EXIT_FAILURE);
        }
    }
    // Done reading the arguments
    
    
    if(sizeof(unsigned long long)*8 < N_BITS_TS+N_BITS_ID+2)
    {
        printf("\"unsigned long long\" is too short on your platform, aborting\n");
        return(EXIT_FAILURE);
    }

    // Initialize parallel port
    if (ioperm(portbase, 1, 1))
    {
        fprintf(stderr, "Cannot get the port. Maybe you should run this program as root\n");
        return(1);
    }
    outb(0, portbase);

    // Set priority
    sch_param.sched_priority = PRIORITY;
    if (sched_setscheduler(0, SCHED_FIFO, &sch_param))
    {
        fprintf(stderr, "Cannot set new priority\n");
        return(1);
    }

    // Check the new priority
    sch_param.sched_priority = 0;
    sched_getparam(0, &sch_param);
    fprintf(stdout, "Running with new static priority = %i\n", sch_param.sched_priority);
    fprintf(stdout, "LPT address: 0x%x, output bit mask: 0x%x\n", portbase, outbits);
    if (_argc == 4)
    {
        fprintf(stdout, "Site id: %i\n", site_id);
    }
    else
    {
        fprintf(stdout, "Site id is not specified\n");
    }

    //---------------------------------------------------------------------
    // Start synchronizing

    // Round the sync time to the next round minute
    clock_gettime(CLOCK_REALTIME, &synctime);
    synctime.tv_nsec = 0;
    synctime.tv_sec += 1;
    synctime.tv_sec -= synctime.tv_sec % 60;
    synctime.tv_sec += 60;

    // Run the sync loop
    while (1)
    {
        // Sleep between syncs. Use absolute time to make sure that daemons
        // running on machines with synchronized clocks will emit the first
        // pulse of each sync train simultaneously.
        synctime.tv_sec += SYNC_INTERVAL;
        clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &synctime, NULL);

        // start sync

        // the rising edge of the first pulse marks the timepoint
        outb(outbits, portbase);
        clock_gettime(CLOCK_REALTIME, &traintime);

        msec = traintime.tv_nsec / 1000000;
        msec += traintime.tv_sec * 1000;

        // add timestamp parity bit to msec, result should have even parity
        msec += (((unsigned long long)parity(msec)) << N_BITS_TS);

        if (_argc == 4)
        {   // add site ID
            msec += (((unsigned long long)site_id) << (N_BITS_TS+1));
        
            // add site ID parity bit
            msec += (((unsigned long long)parity(site_id)) << (N_BITS_TS+N_BITS_ID+1));
        }

        // complete the first pulse
        traintime.tv_sec = 0;
        traintime.tv_nsec = SYNC_TRAIN_INTERVAL;    // remember that traintime.tv_nsec
                                                    // should not exceed 1 second
        clock_nanosleep(CLOCK_REALTIME, 0, &traintime, NULL);
        outb(0, portbase);

        // emit the bits, from lsb to msb.
        for (i=0; i<(_argc==4 ? N_BITS_TS+N_BITS_ID+2 : N_BITS_TS+1); i++)
        {
            // "0" - short interval between two pulses, "1" - long interval
            traintime.tv_nsec = (1 + 2*(msec % 2)) * SYNC_TRAIN_INTERVAL;   // remember that traintime.tv_nsec
                                                                            // should not exceed 1 second
            msec /= 2;
            clock_nanosleep(CLOCK_REALTIME, 0, &traintime, NULL);
            outb(outbits, portbase);

            traintime.tv_nsec = SYNC_TRAIN_INTERVAL;
            clock_nanosleep(CLOCK_REALTIME, 0, &traintime, NULL);
            outb(0, portbase);
        }
    }
}
