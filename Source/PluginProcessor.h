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

    float getLastGainReductionL() const noexcept { return lastGRL.load(); } //public getter for left gain reduction meter
    float getLastGainReductionR() const noexcept { return lastGRR.load(); } //public getter for right gain reduction meter

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

struct OnePoleHPF
{
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        reset();
        setCutoff (80.0f); //default cutoff frequency
    }
    void reset()
    {
        x1 = 0.0f;
        y1 = 0.0f;   
    }
    void setCutoff (float hz)
    {
        hz = juce::jlimit (1.0f, 20000.0f, hz);
        const float w = 2.0f * juce::MathConstants<float>::pi * hz / (float) sr;
        const float x = std::exp (-w);

        a1 = x;
        b0 = (1.0f + x) * 0.5f;
        b1 = -b0;   
    }
    float processSample (float x0)
    {
        const float y0 = b0 * x0 + b1 * x1 - a1 * y1;
        x1 = x0;
        y1 = y0;
        return y0;
    }
    double sr = 44100.0;
    float a1 = 0.0f, b0 = 0.0f, b1 = 0.0f;
    float x1 = 0.0f, y1 = 0.0f;
};



    // --- Gain Reduction meter smoothing (GUI only) ---
float grMeterL = 0.0f;
float grMeterR = 0.0f;


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
    struct RMSFollower
{
        void prepare (double sampleRate)
    {
        sr = sampleRate;
        envPower = 0.0f;
        updateTimeConstants (10.0f, 100.0f);
    }

        void updateTimeConstants (float attackMs, float releaseMs)
    {
        attackCoeff  = std::exp (-1.0f / (0.001f * attackMs  * (float) sr));
        releaseCoeff = std::exp (-1.0f / (0.001f * releaseMs * (float) sr));
    }

        float processSample (float x)
    {
        const float p = x * x; // power
        const float coeff = (p > envPower) ? attackCoeff : releaseCoeff;
        envPower = p + coeff * (envPower - p);
        return std::sqrt (envPower);
    }

        double sr = 44100.0;
        float envPower = 0.0f;
        float attackCoeff = 0.0f;
        float releaseCoeff = 0.0f;
};


    EnvelopeFollower envL, envR;
    RMSFollower rmsL, rmsR;

// Sidechain (detector) HPF
OnePoleHPF scHpfL, scHpfR;

// Gain Reduction meter values (store POSITIVE dB, e.g. 0..24)
std::atomic<float> lastGRL { 0.0f };
std::atomic<float> lastGRR { 0.0f };


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StereoCompressorBuild1AudioProcessor)
}; //Prevents accidental copying of the class and adds memory leak detection features.


