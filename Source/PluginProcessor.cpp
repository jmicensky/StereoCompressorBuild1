#include "PluginProcessor.h"
#include "PluginEditor.h" // Pulls in the editor header file

static float ratioFromChoiceIndex (int idx)
{
    constexpr float ratios[] = { 1.5f, 3.0f, 4.0f, 6.0f, 10.0f, 20.0f };
    idx = juce::jlimit (0, int (std::size (ratios)) - 1, idx);
    return ratios[idx]; 
} //Helper function to convert choice index to ratio value.

StereoCompressorBuild1AudioProcessor::StereoCompressorBuild1AudioProcessor()
     : AudioProcessor (BusesProperties() //Tells the host that it's stereo input and stereo output.
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
       apvts(*this, nullptr, "Parameters", createParameterLayout()) // Initialize the APVTS with the processor, no undo manager, a unique ID, and the parameter layout
{}
juce::AudioProcessorValueTreeState::ParameterLayout 
StereoCompressorBuild1AudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back (std::make_unique<juce::AudioParameterFloat>( //Gain, range -24 dB to +24 dB, default 0 dB
        "gain", "Gain (dB)",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f 
    ));

    params.push_back (std::make_unique<juce::AudioParameterBool>( //Bypass
        "bypass", "Bypass", false
    ));
    params.push_back (std::make_unique<juce::AudioParameterBool>(
        "scHPfOn", "Sidechain High-Pass Filter On", false 
    ));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "scHpfFreq", "Sidechain High-Pass Filter Frequency (Hz)",
        juce::NormalisableRange<float>(20.0f, 300.0f, 1.0f, 0.5f), 80.0f
    ));


    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "threshold", "Threshold (dB)",
        juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f), -18.0f
    ));
    params.push_back (std::make_unique<juce::AudioParameterChoice>(
    "detectorMode", "Detector",
    juce::StringArray { "Peak", "RMS" },
    0
    )); //detector mode parameter

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
    "unlink", "Unlink (%)",
    juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f
    )); //stereo unlink parameter. Default = 0% (fully linked)

    params.push_back (std::make_unique<juce::AudioParameterChoice>(
        "ratio", "Ratio",
        juce::StringArray {"1.5:1", "3:1", "4:1","6:1", "10:1", "20:1"},
        2 //default = 4;1
    ));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "attack", "Attack (ms)",
        juce::NormalisableRange<float>(0.1f, 100.0f, 0.1f, 0.5f), 10.0f
    ));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "release", "Release (ms)",
        juce::NormalisableRange<float>(10.0f, 1000.0f, 1.0f, 0.5f), 100.0f
    ));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "knee", "Knee (dB)",
        juce::NormalisableRange<float>(0.0f, 12.0f, 0.1f), 6.0f
    ));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "makeup", "Makeup Gain (dB)",
        juce::NormalisableRange<float>(-12.0f, 24.0f, 0.1f), 0.0f
    ));
return { params.begin(), params.end() };
}
//Prepare to Play
void StereoCompressorBuild1AudioProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = sampleRate;

    envL.prepare (sampleRate); //prepares left envelope follower (peak)
    envR.prepare (sampleRate); //prepares right envelope follower (peak)

    rmsL.prepare (sampleRate);
    rmsR.prepare (sampleRate); //prepares RMS followers
    
    scHpfL.prepare (sampleRate);
    scHpfR.prepare (sampleRate);

    // Use this method as the place to do any pre-playback
    // initialisation that you need..
} //No DSP state yet. Will use this later for envelope filters, followers etc.

bool StereoCompressorBuild1AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
} //Only supports stereo in and out.


void StereoCompressorBuild1AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& )
{
    if (buffer.getNumChannels() < 2) return;

    juce::ScopedNoDenormals noDenormals;
    // Clear any output channels that don't have input data
auto totalNumInputChannels  = getTotalNumInputChannels();
auto totalNumOutputChannels = getTotalNumOutputChannels();

float maxGRDbL = 0.0f;
float maxGRDbR = 0.0f;

for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    buffer.clear (i, 0, buffer.getNumSamples());

//PARAMETER READS
    const bool bypass = apvts.getRawParameterValue("bypass")->load() > 0.5f; //Get bypass parameter value
    if (bypass) return;  // Pulls the DAW/UI Value and exits early if bypassed

    const float gainDb = apvts.getRawParameterValue("gain")->load(); //Get gain parameter value

    const int detectorMode = (int) apvts.getRawParameterValue ("detectorMode")->load(); // 0=Peak, 1=RMS

    const bool scHpfOn = apvts.getRawParameterValue("scHPfOn")->load() > 0.5f; //Get sidechain HPF on/off parameter value
    const float scHpfFreq = apvts.getRawParameterValue("scHpfFreq")->load(); //Get sidechain HPF frequency parameter value
    if (scHpfOn)
    {
        scHpfL.setCutoff (scHpfFreq);
        scHpfR.setCutoff (scHpfFreq);
    }
    const float unlinkPct = apvts.getRawParameterValue("unlink")->load(); // 0..100
    const float unlink = juce::jlimit (0.0f, 1.0f, unlinkPct / 100.0f);   // 0..1
    const float link = 1.0f - unlink; // 1=linked, 0=unlinked

    const float thresholdDb = apvts.getRawParameterValue("threshold")->load(); //Get threshold parameter value
    const float kneeDb = apvts.getRawParameterValue("knee")->load(); //Get knee parameter value
    const float makeupDb = apvts.getRawParameterValue("makeup")->load(); //Get makeup gain parameter value

    const int ratioIndex = (int) apvts.getRawParameterValue("ratio")->load();
    const float ratio = ratioFromChoiceIndex (ratioIndex);

    const float attackMs = apvts.getRawParameterValue("attack") ->load();
    const float releaseMs = apvts.getRawParameterValue("release") ->load();

    envL.updateTimeConstants (attackMs, releaseMs); //update peak envelope follower time constants
    envR.updateTimeConstants (attackMs, releaseMs);
    rmsL.updateTimeConstants (attackMs, releaseMs); //update RMS follower time constants
    rmsR.updateTimeConstants (attackMs, releaseMs);


    const float inputGain  = juce::Decibels::decibelsToGain (gainDb);
    const float makeupGain = juce::Decibels::decibelsToGain (makeupDb);

    constexpr float eps = 1.0e-9f;

auto computeGainReductionDb = [&] (float inLevelDb)
{
    const float x = inLevelDb;
    const float T = thresholdDb;
    const float R = ratio;

    if (kneeDb <= 0.0f)
    {
        if (x <= T) return 0.0f;
        const float y = T + (x - T) / R;
        return (y - x);
    }

    const float halfK = 0.5f * kneeDb;

    if (x < (T - halfK)) return 0.0f;

    if (x > (T + halfK))
    {
        const float y = T + (x - T) / R;
        return (y - x);
    }

    const float d = x - (T - halfK);
    const float y = x + (1.0f / R - 1.0f) * (d * d) / (2.0f * kneeDb);
    return (y - x);
};


// Process every sample    

auto* xL = buffer.getWritePointer (0);
auto* xR = buffer.getWritePointer (1); //We need separate pointers for left and right channels

    for (int n = 0; n < buffer.getNumSamples(); ++n)
    {
    float sL = xL[n] * inputGain;
    float sR = xR[n] * inputGain; //sL and sR represent the audio signals we'll compress and we want them aligned in time
    

    float detInL = sL;
    float detInR = sR;

    if (scHpfOn)
       { detInL = scHpfL.processSample (detInL);
         detInR = scHpfR.processSample (detInR);
       }

    float detL = 0.0f;
    float detR = 0.0f;

    if (detectorMode == 0) // Peak
        {
        detL = envL.processSample (detInL);
        detR = envR.processSample (detInR);
        }
     else // RMS
        {
        detL = rmsL.processSample (detInL);
        detR = rmsR.processSample (detInR);
        }
    const float linkedDet = juce::jmax (detL, detR); // “max link” behavior
    const float detUsedL = detL * unlink + linkedDet * link;
    const float detUsedR = detR * unlink + linkedDet * link;

    const float detDbL = juce::Decibels::gainToDecibels (detUsedL + eps);
    const float detDbR = juce::Decibels::gainToDecibels (detUsedR + eps);

    const float grDbL  = computeGainReductionDb (detDbL);
    const float grDbR  = computeGainReductionDb (detDbR);

    const float grLinL = juce::Decibels::decibelsToGain (grDbL);
    const float grLinR = juce::Decibels::decibelsToGain (grDbR);

    maxGRDbL = juce::jmax (maxGRDbL, -grDbL);
    maxGRDbR = juce::jmax (maxGRDbR, -grDbR);

    xL[n] = sL * grLinL * makeupGain;
    xR[n] = sR * grLinR * makeupGain;

    }

// --- Meter ballistics (fast attack, slow release) ---
constexpr float meterRelease = 0.90f; // closer to 1 = slower decay

grMeterL = juce::jmax(maxGRDbL, grMeterL * meterRelease);
grMeterR = juce::jmax(maxGRDbR, grMeterR * meterRelease);

}
//THIS IS THE END OF THE PROCESS BLOCK


juce::AudioProcessorEditor* StereoCompressorBuild1AudioProcessor::createEditor()
{
    return new StereoCompressorBuild1AudioProcessorEditor (*this); //Creates a generic editor that automatically generates UI based on parameters   
} //JUCE builds a UI automatically from parameter layout.

//Serializing parameter tree to XML and hands it to the DAW for saving
void StereoCompressorBuild1AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
 {   auto state = apvts.copyState(); 
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData); //Saves the APVTS state to a binary block for the host to store
}
    void StereoCompressorBuild1AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{ 
        if (auto xml = getXmlFromBinary(data, sizeInBytes))
            apvts.replaceState (juce::ValueTree::fromXml(*xml)); //Restores the APVTS state from the host
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new StereoCompressorBuild1AudioProcessor();
}
