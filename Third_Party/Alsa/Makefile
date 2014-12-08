CC ?= gcc
COPTS ?= -g

LIBS = -lasound

all : play record disppcm openpcm

clean : 
	rm -f play record disppcm openpcm

play : play.c
	$(CC) -o $@ $(COPTS) $< $(LIBS)
	
record : record.c
	$(CC) -o $@ $(COPTS) $< $(LIBS)
	
disppcm : disppcm.c
	$(CC) -o $@ $(COPTS) $< $(LIBS)
	
openpcm : openpcm.c
	$(CC) -o $@ $(COPTS) $< $(LIBS)