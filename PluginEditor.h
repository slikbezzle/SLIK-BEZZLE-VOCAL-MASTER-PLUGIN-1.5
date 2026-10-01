#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class SlikBezzleVocalMasterAudioProcessorEditor : public juce::AudioProcessorEditor,
                                                  private juce::Timer
{
public:
    explicit SlikBezzleVocalMasterAudioProcessorEditor(SlikBezzleVocalMasterAudioProcessor&);
    ~SlikBezzleVocalMasterAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    SlikBezzleVocalMasterAudioProcessor& processor;

    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> autoTuneAttachment, autoKeyAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> keyAttachment, scaleAttachment;

    juce::Slider master, tuneAmount, retune, humanize, compression, deEss, saturation, air, limiter, mix, input, output;
    juce::ToggleButton autoTune, autoKey;
    juce::ComboBox key, scale;
    juce::TextButton naturalMode, modernMode, hardTuneMode;
    juce::Label title, subtitle, detectedNote, detectedPitch, statusLabel;

    float inputMeter = 0.0f, outputMeter = 0.0f;

    void setupSlider(juce::Slider&, bool percent = true);
    void setupToggle(juce::ToggleButton&);
    void timerCallback() override;
    void drawPanel(juce::Graphics&, juce::Rectangle<int>, const juce::String&, juce::Colour accent);
    void drawMeter(juce::Graphics&, juce::Rectangle<float>, float, juce::Colour);
    void drawPitchDisplay(juce::Graphics&, juce::Rectangle<float>);
    juce::String formatValue(const juce::String& parameterId, double value) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SlikBezzleVocalMasterAudioProcessorEditor)
};
