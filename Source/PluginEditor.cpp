#include "PluginEditor.h"
#include "PluginProcessor.h"

StereoCompressorBuild1AudioProcessorEditor::StereoCompressorBuild1AudioProcessorEditor (StereoCompressorBuild1AudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    detectorModeBox.addItem ("Peak", 1);
    detectorModeBox.addItem ("RMS",  2);
    addAndMakeVisible (detectorModeBox);

    detectorModeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.apvts, "detectorMode", detectorModeBox);

    genericEditor = std::make_unique<juce::GenericAudioProcessorEditor> (processor);
    addAndMakeVisible (genericEditor.get());   

    startTimerHz (30);
    setSize (900, 700);
}
void StereoCompressorBuild1AudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);

    const float grL = juce::jmax (0.0f, processor.getLastGainReductionL());
    const float grR = juce::jmax (0.0f, processor.getLastGainReductionR());

    // Recreate the same layout as resized()
    auto r = getLocalBounds().reduced (10);

    auto topRow = r.removeFromTop (30);
    // (detectorModeBox lives in topRow in resized(); we don’t draw it here)

    r.removeFromTop (10); // spacing

    auto meterColumn = r.removeFromLeft (180);

    // Title area inside meter column
    auto titleArea = meterColumn.removeFromTop (30);
    g.setColour (juce::Colours::white);
    g.drawText ("Gain Reduction (dB)", titleArea, juce::Justification::centredLeft);

    meterColumn.removeFromTop (10); // spacing under title

    // Split remaining meterColumn into 2 meter rectangles
    auto metersArea = meterColumn;

    const int meterWidth  = 50;
    const int meterGap    = 20;

    auto leftMeter  = metersArea.removeFromLeft (meterWidth);
    metersArea.removeFromLeft (meterGap);
    auto rightMeter = metersArea.removeFromLeft (meterWidth);

    // Make meters taller by using all remaining height
    leftMeter  = leftMeter.withHeight (meterColumn.getHeight()).withY (meterColumn.getY());
    rightMeter = rightMeter.withHeight (meterColumn.getHeight()).withY (meterColumn.getY());

auto drawMeter = [&] (juce::Rectangle<int> rMeter, float grDb)
{
    const float maxDb = 24.0f;
    const float norm = juce::jlimit (0.0f, 1.0f, grDb / maxDb);

    // outline
    g.setColour (juce::Colours::grey);
    g.drawRect (rMeter);

    // tick labels (optional)
    g.setColour (juce::Colours::white.withAlpha (0.7f));
    g.setFont (12.0f);
    g.drawText ("0", rMeter.withHeight (16),
                juce::Justification::centredTop);
    g.drawText ("-" + juce::String ((int) maxDb),
                rMeter.withY (rMeter.getBottom() - 16).withHeight (16),
                juce::Justification::centredBottom);

    // fill grows DOWN from the top as GR increases
    auto fill = rMeter;
    fill.setY (rMeter.getY());
    fill.setHeight ((int) (norm * rMeter.getHeight()));

    g.setColour (juce::Colours::limegreen);
    g.fillRect (fill);

    // numeric readout
    g.setColour (juce::Colours::white);
    g.drawText (juce::String (grDb, 1) + " dB",
                rMeter.reduced (2).removeFromTop (18),
                juce::Justification::centred);
};


    drawMeter (leftMeter,  grL);
    drawMeter (rightMeter, grR);
}

void StereoCompressorBuild1AudioProcessorEditor::timerCallback()
{
    repaint();
}
void StereoCompressorBuild1AudioProcessorEditor::resized()
{
    auto r = getLocalBounds().reduced (10);

    // top row controls
    detectorModeBox.setBounds (r.removeFromTop (30).removeFromLeft (140));

    r.removeFromTop (10); // little spacing

    // left meter column
    auto meterColumn = r.removeFromLeft (180);

    // Generic editor takes the rest
    if (genericEditor)
        genericEditor->setBounds (r);
}

StereoCompressorBuild1AudioProcessorEditor::~StereoCompressorBuild1AudioProcessorEditor() = default;


