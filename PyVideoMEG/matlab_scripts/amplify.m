function amplify(vidFile, sampleCount, framePerSample, cycles, pyramid, videoMerge)

PhaseBasedAmpDir = '/home/janne/PhaseBasedAmp';

% From PhaseBasedAmp
% Paths for the linear method
addpath(fullfile(PhaseBasedAmpDir, 'Linear'));
addpath(fullfile(PhaseBasedAmpDir, 'Util'));
addpath(fullfile(PhaseBasedAmpDir, 'matlabPyrTools'));
addpath(fullfile(PhaseBasedAmpDir, 'matlabPyrTools', 'MEX'));

% Paths for the phase-based method
addpath(fullfile(PhaseBasedAmpDir, 'PhaseBased'));
addpath(fullfile(PhaseBasedAmpDir, 'pyrToolsExt'));
addpath(fullfile(PhaseBasedAmpDir, 'Filters'));



vr = VideoReader(vidFile);
fr = vr.FrameRate;
original = vr.read();

[h, w, nChannel, nFrame] = size(original);

frPerSample = framePerSample;
nSample = sampleCount;
overflow = 0;

% TODO calculate overflow frames and add them to end
while (mod(frPerSample, nSample) ~= 0)
    frPerSample = frPerSample - 1;
    overflow = overflow + 1;
end

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

    disp(sprintf('Amplifying sample %02d/%02d.',s,nSample));

    if cycles ~= 1
        stich(:,:,:,:,2:cycles) = stich(:,:,:,:,1);
    end

    % attenuateOtherFreq default is FALSE
    % sigma default is 0
    amp = phaseAmplifyMod(stich(:,:,:,:,s), 10, 0.3, 1.3, 2*fr+5, '', 'sigma', 0, 'attenuateOtherFreq', false, 'temporalFilter', @FIRWindowBP, 'pyrType', pyramid);

    % Resize to allow amplified and original to be side-by-side.
    % Keep aspect ratio.
    % Since Elekta Graph expects 640,480 video use those values
    if (videoMerge)
        for f = 1:frPerSample
            % Uses bicubic interpolation
            resOrig = imresize(samples(:,:,:,f), [240, 320]);
            resAmp = imresize(amp(:,:,:,f), [240, 320]);
            out(121:360,1:320,:,(s-1)*frPerSample+f) = resOrig;
            out(121:360,321:640,:,(s-1)*frPerSample+f) = resAmp;
        end
    else
        out = amp;
    end
end
if (videoMerge)
    resOrig = imresize(original(:,:,:,nFrame - overflow:nFrame), [240, 320]);
    out(121:360,1:320,:,nFrame - overflow:nFrame) = resOrig;
else
    out(:,:,:,nFrame - overflow:nFrame) = original(:,:,:,nFrame - overflow:nFrame);
end
out = im2uint8(out);
save('/tmp/vid.mat' , 'out');