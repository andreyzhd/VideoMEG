%   Copyright (C) 2015 BioMag Laboratory, Helsinki University Central Hospital
%
%   This program is free software: you can redistribute it and/or modify
%   it under the terms of the GNU General Public License as published by
%   the Free Software Foundation, version 3.
%
%   This program is distributed in the hope that it will be useful,
%   but WITHOUT ANY WARRANTY; without even the implied warranty of
%   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
%   GNU General Public License for more details.
%
%   You should have received a copy of the GNU General Public License
%   along with this program.  If not, see <http://www.gnu.org/licenses/>.

function [ts, ids, offset, data, srate, site_id, is_sender] = load_audio0123(filename, fix_tstamps)
    % If FIX_TSTAMPS is true, try to fix the timestamps so that all the
    % buffers have approximately equal length
    MAGIC_STR = 'ELEKTA_AUDIO_FILE';

    h = fopen(filename, 'rb');
    header_str = fread(h, length(MAGIC_STR), 'uchar');
    
    % make sure the file is a valid audio file
    assert(strcmp(char(header_str'), MAGIC_STR));
    
    ver = fread(h, 1, 'uint32');
    assert(ver==0 | ver==1 | ver==2 | ver==3);
    
    if ver==3
        site_id = fread(h, 1, 'uint8');
        sender_flag = fread(h, 1, 'uint8');
        is_sender = (sender_flag==1);
    else
        site_id = -1;
        is_sender = false;
    end 
    
    srate = fread(h, 1, 'uint32');
    nchans = fread(h, 1, 'uint32');
    
    % get the size of the data part of the file in bytes
    dt_start = ftell(h);
    fseek(h, 0 ,'eof');
    dt_end = ftell(h);
    fseek(h, dt_start, 'bof');
    
    % read the buffer size from the first data chunk
    [t, id, buflen] = read_attrib(h, ver);
    frames_per_buf = buflen/(2*nchans);
    
    if(ver==0 | ver==1)
        attrib_sz = 8 + 4;
    end
    
    if(ver==2 | ver==3)
        attrib_sz = 8 + 8 + 4;
    end
    
    assert(mod((dt_end - dt_start), (attrib_sz + buflen)) == 0)
    numof_chunks = (dt_end - dt_start) / (attrib_sz + buflen);
    
    % allocate the memory
    ts = zeros(numof_chunks, 1);
    ids = zeros(numof_chunks, 1);
    data = zeros(frames_per_buf*nchans, numof_chunks);
    
    % compute offset
    offset = [1 : frames_per_buf : frames_per_buf*numof_chunks]';
    
    % read the data
    fprintf('load_audio_0123: reading audio data from %s...\n',filename);
    fseek(h, dt_start, 'bof');
    
    for i = 1 : numof_chunks
        PRINT_INTERVAL=1e6;  % print status messages at this interval
        [ts(i), ids(i), blen] = read_attrib(h, ver);
        %	if i == 1
	%  fprintf('load_audio_0123: first packet id: %.0f\n',ids(1));
	% end
        assert(blen == buflen);
        data(:,i) = fread(h, buflen/2, 'int16');
        
        if mod(i, PRINT_INTERVAL) == 0
            fprintf('load_audio_0123: loaded %g buffers of %i bytes\n', i, buflen);
        end
    end
    fclose(h);
    %    fprintf('load_audio_0123:...done\n');
    data = reshape(data, nchans, frames_per_buf*numof_chunks)';
    
    % fix the timestamps
    if fix_tstamps
        p = polyfit([1:length(ts)]', ts, 1);
        nts = round(p(1) * [1:length(ts)]' + p(2));
        
        % The timestamps can be only shifted into the past, not into the
        % future. This is because we know that each buffer was acquired
        % before (and never after) the corresponding timestamp.
        nts = nts - max(nts - ts);
        
        % DEBUG
        figure;
        plot(nts - ts);
        xlabel('buffer number');
        ylabel('buffer time adjustment, msec');
        
        fprintf('\n\nOriginal buffer lengths:\tmean:\t%0.2f\t\tstd:\t%0.2f\t\tmin:\t%0.2f\t\tmax:\t%0.2f\n', mean(diff(ts)), std(diff(ts)), min(diff(ts)), max(diff(ts)));
        fprintf('Fixed buffer lengths:\t\tmean:\t%0.2f\t\tstd:\t%0.2f\t\tmin:\t%0.2f\t\tmax:\t%0.2f\n\n', mean(diff(nts)), std(diff(nts)), min(diff(nts)), max(diff(nts)));
        % ~DEBUG
        
        ts = nts;
    end
end


function [t, id, blen] = read_attrib(h, ver)
    assert(ver==0 | ver==1 | ver==2 | ver==3);
    t = fread(h, 1, 'uint64');
        
    if(ver==2 | ver==3)
        id = fread(h, 1, 'uint64');
    else
        id = 0;
    end

    blen = fread(h, 1, 'uint32');
end
