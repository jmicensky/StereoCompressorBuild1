//Pragma prevents the files from being included twice.
//include line gives you JUCE classes like AudioProcessor, AudioBuffer, APVTS, etc.
#pragma once 
#include <JuceHeader.h>

class StereoCompressorBuild1AudioProcessor  : public juce::AudioProcessor
{
public:
    StereoCompressorBuild1AudioProcessor();
    ~StereoCompressorBuild1AudioProcessor() override = default;

//Plugin is a C++ class that inherits from JUCE's AudioProcessor class. This declares a contructor (runs when plugin loads) and destructor.

    void prepareToPlay (double sampleRate, int samplesPerBlock) override; //called once before audio starts; set sample rate; allocate buffers
    void releaseResources() override{} //called when audio stops. leaving it empty for now
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override; //called repeatedly, this is where DSP happens.

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override; //specifies which channel configurations are supported

    juce::AudioProcessorEditor* createEditor() override; //creates the plugin's GUI
    bool hasEditor() const override {return true;}; //indicates that the plugin has a GUI

// Add the required overrides becuase JUCE supports synths, MIDI effects, multi-program plugins, etc. This plugin is audio-only with one "program".

    const juce::String getName() const override { return JucePlugin_Name; } //returns the plugin's name
    bool acceptsMidi() const override{return false;} //indicates whether the plugin accepts MIDI input
    bool producesMidi() const override{return false;} //indicates whether the plugin produces MIDI output
    bool isMidiEffect() const override{return false;} //indicates whether the plugin is a MIDI effect
    double getTailLengthSeconds() const override{return 0.0;} //returns the tail length in seconds 

    int getNumPrograms() override{return 1;} //returns the number of preset programs
    int getCurrentProgram() override{return 0;} //returns the index of the current program
    void setCurrentProgram (int) override{} //sets the current program
    const juce::String getProgramName (int) override{return {}; } //returns the name of the specified program
    void changeProgramName (int, const juce::String&) override{}

// Save & Restore State for session recall

    void getStateInformation (juce::MemoryBlock& destData) override; //saves the plugin's state
    void setStateInformation (const void* data, int sizeInBytes) override; //restores the plugin's state

// Parameters

juce::AudioProcessorValueTreeState apvts; //manages the plugin's parameters
static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout(); //creates the parameter layout
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StereoCompressorBuild1AudioProcessor)
}; //Prevents accidental copying of the class and adds memory leak detection features.


