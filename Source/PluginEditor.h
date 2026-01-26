#pragma once

#include <JuceHeader.h>


class StereoCompressorBuild1AudioProcessor;

class StereoCompressorBuild1AudioProcessorEditor
    : public juce::AudioProcessorEditor,
      public juce::Timer
{
public:
    StereoCompressorBuild1AudioProcessorEditor (StereoCompressorBuild1AudioProcessor&);
    ~StereoCompressorBuild1AudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    StereoCompressorBuild1AudioProcessor& processor;

    juce::ComboBox detectorModeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> detectorModeAttach;

    std::unique_ptr<juce::AudioProcessorEditor> genericEditor;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StereoCompressorBuild1AudioProcessorEditor)
};
