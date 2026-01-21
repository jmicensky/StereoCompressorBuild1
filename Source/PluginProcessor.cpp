#include "PluginProcessor.h"
#include "PluginEditor.h" // Pulls in the editor header file

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

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "threshold", "Threshold (dB)",
        juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f), -18.0f
    ));

    params.push_back (std::make_unique<juce::AudioParameterChoice>(
        "ratio", "Ratio",
        juce::StringArray {"1.5:1", "3:1", "4:1", "10:1", "20:1:"},
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
void StereoCompressorBuild1AudioProcessor::prepareToPlay (double, int)
{
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
    juce::ScopedNoDenormals noDenormals;
    const bool bypass = apvts.getRawParameterValue("bypass")->load() > 0.5f; //Get bypass parameter value
    if (bypass) return;
    // Pulls the DAW/UI Value and exits early if bypassed
    const float gainDb = apvts.getRawParameterValue("gain")->load(); //Get gain parameter value
    const float g = juce::Decibels::decibelsToGain(gainDb); //Convert dB to linear gain

// Process every sample    
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* x = buffer.getWritePointer (ch); //gives you raw float for each channel

        for (int n = 0; n < buffer.getNumSamples(); ++n)
        {
            x[n] *= g; //Apply gain to each sample
        }
    }
}

juce::AudioProcessorEditor* StereoCompressorBuild1AudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this); //Creates a generic editor that automatically generates UI based on parameters   
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
}juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new StereoCompressorBuild1AudioProcessor();
}
