function amplify(vidFile, sampleCount, framePerSample, cycles, pyramid, low, high, ampFactor, attenuate, phase_based_dir)

% From PhaseBasedAmp
% Paths for the linear method
addpath(fullfile(phase_based_dir, 'Linear'));
addpath(fullfile(phase_based_dir, 'Util'));
addpath(fullfile(phase_based_dir, 'matlabPyrTools'));
addpath(fullfile(phase_based_dir, 'matlabPyrTools', 'MEX'));

% Paths for the phase-based method
addpath(fullfile(phase_based_dir, 'PhaseBased'));
addpath(fullfile(phase_based_dir, 'pyrToolsExt'));
addpath(fullfile(phase_based_dir, 'Filters'));

vr = VideoReader(vidFile);
fr = vr.FrameRate;

% This should cover all the possible frames
expected = sampleCount * framePerSample + 1;
% Expect RGB24 uint8
original = zeros(vr.Height, vr.Width, 3, expected);
actual = 0;
while hasFrame(vr)
    actual = actual + 1;
    original(:,:,:,actual) = readFrame(vr);
end

% Remove extra frames
original = original(:,:,:,1:actual);

[h, w, nChannel, nFrame] = size(original);

frPerSample = framePerSample;
nSample = sampleCount;

% Check for cases where nFrame is not multiple of nSample and frPerSample.
overflow = mod(nFrame, nSample);
frPerSample = frPerSample - overflow;

samples = zeros(h, w, nChannel, frPerSample, nSample, 'uint8');
reverse = zeros(h, w, nChannel, frPerSample, nSample,'uint8');
stich = zeros(h, w, nChannel, frPerSample*2, nSample*cycles,'uint8');
out = zeros(h, w, nChannel, nFrame, 'uint8');

for s = 1:nSample
    for f = 1:frPerSample
        samples(:,:,:,f,s) = original(:,:,:,((s-1)*frPerSample)+f);
        reverse(:,:,:,f,s) = original(:,:,:,(s*frPerSample)+1-f);
    end
    stich(:,:,:,1:frPerSample,s) = samples(:,:,:,:,s);
    stich(:,:,:,frPerSample+1:2*frPerSample,s) = reverse(:,:,:,:,s);

    fprintf('Amplifying sample %02d/%02d.\n',s,nSample);

    if cycles ~= 1
        stich(:,:,:,:,2:cycles) = stich(:,:,:,:,1);
    end

    % attenuateOtherFreq default is FALSE
    % sigma default is 0
    amp = phaseAmplifyMod(stich(:,:,:,:,s), ampFactor, low, high, fr, '', 'sigma', 0, 'attenuateOtherFreq', attenuate, 'temporalFilter', @FIRWindowBP, 'pyrType', pyramid);

   
    out(:,:,:,(s-1)*framePerSample+1:s*framePerSample) = amp(:,:,:,1:frPerSample);

    
end

out(:,:,:,nFrame - overflow:nFrame) = original(:,:,:,nFrame - overflow:nFrame);

out = im2uint8(out);
save('/tmp/vid.mat' , 'out');