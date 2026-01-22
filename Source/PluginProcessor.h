//Pragma prevents the files from being included twice.
//include line gives you JUCE classes like AudioProcessor, AudioBuffer, APVTS, etc.
#pragma once 
#include <JuceHeader.h>
#include <cmath>


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
// envelope follower 

private:
    // ---- Envelope follower (Day 3) ----
    double currentSampleRate = 44100.0;

    std::atomic<float> lastEnvL { 0.0f };
    std::atomic<float> lastEnvR { 0.0f };

    struct EnvelopeFollower
    {
        void prepare (double sampleRate)


        {
            sr = sampleRate;
            env = 0.0f;
            updateTimeConstants (10.0f, 100.0f);
        }

        void updateTimeConstants (float attackMs, float releaseMs)
        {
            attackCoeff  = std::exp (-1.0f / (0.001f * attackMs  * (float) sr));
            releaseCoeff = std::exp (-1.0f / (0.001f * releaseMs * (float) sr));
        }

        float processSample (float x)
        {
            x = std::fabs (x); //floating absolute value
            const float coeff = (x > env) ? attackCoeff : releaseCoeff;
            env = x + coeff * (env - x);
            return env;
        }

        float getEnvelope() const { return env; }

        double sr = 44100.0;
        float env = 0.0f;
        float attackCoeff = 0.0f;
        float releaseCoeff = 0.0f;
    };

    EnvelopeFollower envL, envR;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StereoCompressorBuild1AudioProcessor)
}; //Prevents accidental copying of the class and adds memory leak detection features.


