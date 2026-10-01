#include "PluginEditor.h"

namespace
{
    const juce::Colour bg(0xff050708);
    const juce::Colour panel(0xff0c1114);
    const juce::Colour panel2(0xff10171b);
    const juce::Colour line(0xff26343a);
    const juce::Colour white(0xfff4f7f5);
    const juce::Colour green(0xff62ff6a);
    const juce::Colour turquoise(0xff22e6d1);
    const juce::Colour muted(0xff87949a);
}

SlikBezzleVocalMasterAudioProcessorEditor::SlikBezzleVocalMasterAudioProcessorEditor(SlikBezzleVocalMasterAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setResizable(true, true);
    setResizeLimits(980, 650, 1800, 1100);
    setSize(1180, 760);

    auto addSlider = [this](juce::Slider& s, const char* id, bool percent = true)
    {
        setupSlider(s, percent);
        addAndMakeVisible(s);
        sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, id, s));
    };

    addSlider(master, "master");
    addSlider(tuneAmount, "tuneAmount");
    addSlider(retune, "retune", false);
    addSlider(humanize, "humanize");
    addSlider(compression, "compress");
    addSlider(deEss, "deess");
    addSlider(saturation, "saturation");
    addSlider(air, "air");
    addSlider(limiter, "limiter");
    addSlider(mix, "mix");
    addSlider(input, "input", false);
    addSlider(output, "output", false);

    setupToggle(autoTune);
    setupToggle(autoKey);
    autoTune.setButtonText("AUTO TUNE");
    autoKey.setButtonText("AUTO KEY");
    addAndMakeVisible(autoTune);
    addAndMakeVisible(autoKey);
    autoTuneAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.apvts, "autotuneOn", autoTune);
    autoKeyAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.apvts, "autoKey", autoKey);

    key.addItemList({ "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 1);
    scale.addItemList({ "Major", "Minor" }, 1);
    key.setColour(juce::ComboBox::backgroundColourId, panel2);
    key.setColour(juce::ComboBox::outlineColourId, line);
    key.setColour(juce::ComboBox::textColourId, white);
    scale.setColour(juce::ComboBox::backgroundColourId, panel2);
    scale.setColour(juce::ComboBox::outlineColourId, line);
    scale.setColour(juce::ComboBox::textColourId, white);
    addAndMakeVisible(key);
    addAndMakeVisible(scale);
    keyAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.apvts, "key", key);
    scaleAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.apvts, "scale", scale);

    for (auto* b : { &naturalMode, &modernMode, &hardTuneMode })
    {
        b->setButtonText(b == &naturalMode ? "NATURAL" : (b == &modernMode ? "MODERN" : "HARD TUNE"));
        b->setColour(juce::TextButton::buttonColourId, panel2);
        b->setColour(juce::TextButton::textColourOffId, white);
        b->setColour(juce::TextButton::textColourOnId, green);
        b->setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff16331a));
        addAndMakeVisible(*b);
    }
    naturalMode.setToggleState(true, juce::dontSendNotification);
    naturalMode.setClickingTogglesState(true);
    modernMode.setClickingTogglesState(true);
    hardTuneMode.setClickingTogglesState(true);

    auto selectTuneMode = [this](int mode)
    {
        if (auto* p = processor.apvts.getParameter("tuneMode"))
            p->setValueNotifyingHost((float) mode / 2.0f);
        naturalMode.setToggleState(mode == 0, juce::dontSendNotification);
        modernMode.setToggleState(mode == 1, juce::dontSendNotification);
        hardTuneMode.setToggleState(mode == 2, juce::dontSendNotification);
    };
    naturalMode.onClick = [selectTuneMode] { selectTuneMode(0); };
    modernMode.onClick = [selectTuneMode] { selectTuneMode(1); };
    hardTuneMode.onClick = [selectTuneMode] { selectTuneMode(2); };

    title.setText("SLIK BEZZLE", juce::dontSendNotification);
    title.setColour(juce::Label::textColourId, white);
    title.setFont(juce::FontOptions(34.0f, juce::Font::bold));
    addAndMakeVisible(title);

    subtitle.setText("VOCAL MASTER  •  VST3", juce::dontSendNotification);
    subtitle.setColour(juce::Label::textColourId, turquoise);
    subtitle.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    addAndMakeVisible(subtitle);

    detectedNote.setColour(juce::Label::textColourId, green);
    detectedNote.setFont(juce::FontOptions(22.0f, juce::Font::bold));
    detectedNote.setJustificationType(juce::Justification::centred);
    detectedNote.setText("--", juce::dontSendNotification);
    addAndMakeVisible(detectedNote);

    detectedPitch.setColour(juce::Label::textColourId, turquoise);
    detectedPitch.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    detectedPitch.setJustificationType(juce::Justification::centred);
    detectedPitch.setText("NO PITCH DETECTED", juce::dontSendNotification);
    addAndMakeVisible(detectedPitch);

    statusLabel.setColour(juce::Label::textColourId, muted);
    statusLabel.setFont(juce::FontOptions(9.0f, juce::Font::bold));
    statusLabel.setJustificationType(juce::Justification::centred);
    statusLabel.setText("VST3 • REAL-TIME VOCAL PROCESSING", juce::dontSendNotification);
    addAndMakeVisible(statusLabel);

    startTimerHz(30);
}

void SlikBezzleVocalMasterAudioProcessorEditor::setupSlider(juce::Slider& s, bool percent)
{
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 76, 22);
    s.setColour(juce::Slider::rotarySliderFillColourId, green);
    s.setColour(juce::Slider::rotarySliderOutlineColourId, line);
    s.setColour(juce::Slider::thumbColourId, turquoise);
    s.setColour(juce::Slider::textBoxTextColourId, white);
    s.setColour(juce::Slider::textBoxBackgroundColourId, panel2);
    s.setColour(juce::Slider::textBoxOutlineColourId, line);
    s.setNumDecimalPlacesToDisplay(percent ? 0 : 1);
}

void SlikBezzleVocalMasterAudioProcessorEditor::setupToggle(juce::ToggleButton& b)
{
    b.setColour(juce::ToggleButton::textColourId, white);
    b.setColour(juce::ToggleButton::tickColourId, green);
}

juce::String SlikBezzleVocalMasterAudioProcessorEditor::formatValue(const juce::String& id, double value) const
{
    if (id == "retune") return juce::String(value, 0) + " ms";
    if (id == "input" || id == "output") return juce::String(value, 1) + " dB";
    if (id == "master" || id == "tuneAmount" || id == "humanize" || id == "compress" || id == "deess" || id == "saturation" || id == "air" || id == "limiter" || id == "mix")
        return juce::String(value * 100.0, 0) + "%";
    return juce::String(value, 1);
}

void SlikBezzleVocalMasterAudioProcessorEditor::drawPanel(juce::Graphics& g, juce::Rectangle<int> r, const juce::String& label, juce::Colour accent)
{
    g.setColour(panel);
    g.fillRoundedRectangle(r.toFloat(), 14.0f);
    g.setColour(line);
    g.drawRoundedRectangle(r.toFloat().reduced(0.5f), 14.0f, 1.0f);
    g.setColour(accent);
    g.fillRoundedRectangle((float) r.getX() + 1.0f, (float) r.getY() + 1.0f, 4.0f, (float) r.getHeight() - 2.0f, 2.0f);
    g.setColour(white);
    g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    g.drawText(label, r.getX() + 16, r.getY() + 12, r.getWidth() - 32, 20, juce::Justification::left);
}

void SlikBezzleVocalMasterAudioProcessorEditor::drawMeter(juce::Graphics& g, juce::Rectangle<float> r, float level, juce::Colour c)
{
    g.setColour(juce::Colour(0xff172126));
    g.fillRoundedRectangle(r, 3.0f);
    auto filled = r.withTop(r.getBottom() - r.getHeight() * juce::jlimit(0.0f, 1.0f, level));
    g.setColour(c);
    g.fillRoundedRectangle(filled, 3.0f);
}

void SlikBezzleVocalMasterAudioProcessorEditor::drawPitchDisplay(juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour(juce::Colour(0xff071013));
    g.fillRoundedRectangle(r, 10.0f);
    g.setColour(line);
    g.drawRoundedRectangle(r.reduced(0.5f), 10.0f, 1.0f);

    // Pitch lane / note grid.
    g.setColour(juce::Colour(0xff18262b));
    for (int i = 1; i < 12; ++i)
    {
        const float x = r.getX() + r.getWidth() * (float) i / 12.0f;
        g.drawVerticalLine((int) x, r.getY() + 10.0f, r.getBottom() - 10.0f);
    }

    const float pitch = processor.getDetectedFrequency();
    const bool active = pitch > 0.0f;
    const float t = juce::Time::getMillisecondCounterHiRes() * 0.0025f;
    juce::Path trace;
    trace.startNewSubPath(r.getX() + 8.0f, r.getCentreY());
    for (int i = 1; i < 170; ++i)
    {
        const float x = r.getX() + 8.0f + (r.getWidth() - 16.0f) * (float) i / 169.0f;
        const float wobble = active ? 11.0f : 4.0f;
        const float y = r.getCentreY() + std::sin(t + i * 0.17f) * wobble;
        trace.lineTo(x, y);
    }
    g.setColour(active ? turquoise : muted);
    g.strokePath(trace, juce::PathStrokeType(2.0f));

    const int key = processor.getDetectedKey();
    const int scale = processor.getDetectedScale();
    static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    detectedNote.setText(active ? juce::String(names[key]) + (scale == 0 ? " MAJOR" : " MINOR") : "--", juce::dontSendNotification);
    detectedPitch.setText(active ? juce::String(pitch, 1) + " Hz  •  PITCH TRACKING" : "NO PITCH DETECTED", juce::dontSendNotification);
}

void SlikBezzleVocalMasterAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(bg);

    auto area = getLocalBounds().reduced(16);
    g.setColour(juce::Colour(0xff080d0f));
    g.fillRoundedRectangle(area.toFloat(), 20.0f);

    g.setColour(green);
    g.fillRoundedRectangle((float) area.getX(), (float) area.getY(), 6.0f, (float) area.getHeight(), 3.0f);

    g.setColour(white);
    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    g.drawText("PRO VOCAL PROCESSING", area.getRight() - 190, area.getY() + 16, 165, 18, juce::Justification::right);
    g.setColour(muted);
    g.setFont(juce::FontOptions(9.0f));
    g.drawText("AUTO TUNE  •  AUTO KEY  •  COMP  •  DE-ESS  •  SAT  •  AIR  •  LIMIT", area.getX() + 20, area.getBottom() - 20, area.getWidth() - 40, 14, juce::Justification::centred);

    // Header panels.
    drawPanel(g, { area.getX() + 20, area.getY() + 72, 330, 185 }, "AUTO KEY", turquoise);
    drawPanel(g, { area.getX() + 20, area.getY() + 270, 330, 190 }, "AUTO TUNE", green);
    drawPanel(g, { area.getX() + 365, area.getY() + 72, 455, 388 }, "VOCAL MASTER", green);
    drawPanel(g, { area.getX() + 835, area.getY() + 72, area.getWidth() - 855, 388 }, "INPUT / OUTPUT", turquoise);

    // Central master ring.
    auto center = juce::Point<float>((float) area.getX() + 592.0f, (float) area.getY() + 250.0f);
    drawPitchDisplay(g, { center.x - 178.0f, center.y - 88.0f, 356.0f, 68.0f });
    const float radius = 100.0f;
    g.setColour(juce::Colour(0xff1a2529));
    g.fillEllipse(center.x - radius, center.y - radius, radius * 2.0f, radius * 2.0f);
    g.setColour(turquoise.withAlpha(0.85f));
    g.drawEllipse(center.x - radius, center.y - radius, radius * 2.0f, radius * 2.0f, 4.0f);
    g.setColour(green.withAlpha(0.9f));
    g.drawEllipse(center.x - radius + 9, center.y - radius + 9, (radius - 9) * 2.0f, (radius - 9) * 2.0f, 3.0f);
    g.setColour(white);
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    g.drawText("AMOUNT", (int) center.x - 55, (int) center.y - 12, 110, 20, juce::Justification::centred);
    g.setColour(green);
    g.setFont(juce::FontOptions(28.0f, juce::Font::bold));
    g.drawText(juce::String((int) (processor.apvts.getRawParameterValue("master")->load() * 100.0f)) + "%", (int) center.x - 60, (int) center.y + 10, 120, 38, juce::Justification::centred);

    // Waveform-style decorative line.
    g.setColour(turquoise.withAlpha(0.8f));
    juce::Path wave;
    const auto y = center.y - 2.0f;
    wave.startNewSubPath(center.x - 165.0f, y);
    for (int i = 0; i < 150; ++i)
    {
        const auto xx = center.x - 165.0f + i * 2.0f;
        const auto yy = y + std::sin(i * 0.38f) * (8.0f + (i % 17 == 0 ? 12.0f : 0.0f));
        wave.lineTo(xx, yy);
    }
    g.strokePath(wave, juce::PathStrokeType(1.5f));

    drawMeter(g, { (float) area.getX() + 390.0f, (float) area.getY() + 145.0f, 9.0f, 165.0f }, inputMeter, green);
    drawMeter(g, { (float) area.getX() + 785.0f, (float) area.getY() + 145.0f, 9.0f, 165.0f }, outputMeter, turquoise);

    // Bottom processor strip.
    const int bottomY = area.getY() + 475;
    const int gap = 10;
    const int count = 6;
    const int w = (area.getWidth() - 40 - gap * (count - 1)) / count;
    const juce::String names[] = { "COMPRESSOR", "DE-ESSER", "SATURATION", "AIR", "LIMITER", "MIX" };
    for (int i = 0; i < count; ++i)
        drawPanel(g, { area.getX() + 20 + i * (w + gap), bottomY, w, 170 }, names[i], i == 4 ? turquoise : green);
}

void SlikBezzleVocalMasterAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(16);

    title.setBounds(area.getX() + 24, area.getY() + 12, 300, 42);
    subtitle.setBounds(area.getX() + 26, area.getY() + 48, 250, 18);
    detectedNote.setBounds(area.getX() + 450, area.getY() + 12, 280, 30);
    detectedPitch.setBounds(area.getX() + 450, area.getY() + 40, 280, 20);
    statusLabel.setBounds(area.getX() + 380, area.getBottom() - 36, 400, 18);

    auto left = juce::Rectangle<int>(area.getX() + 20, area.getY() + 72, 330, 185);
    auto tune = juce::Rectangle<int>(area.getX() + 20, area.getY() + 270, 330, 190);
    auto center = juce::Rectangle<int>(area.getX() + 365, area.getY() + 72, 455, 388);
    auto io = juce::Rectangle<int>(area.getX() + 835, area.getY() + 72, area.getWidth() - 855, 388);

    autoTune.setBounds(tune.getX() + 16, tune.getY() + 38, 120, 26);
    autoKey.setBounds(left.getX() + 190, left.getY() + 38, 120, 26);

    key.setBounds(left.getX() + 20, left.getY() + 118, 125, 30);
    scale.setBounds(left.getX() + 160, left.getY() + 118, 145, 30);

    tuneAmount.setBounds(tune.getX() + 15, tune.getY() + 75, 90, 105);
    retune.setBounds(tune.getX() + 118, tune.getY() + 75, 90, 105);
    humanize.setBounds(tune.getX() + 221, tune.getY() + 75, 90, 105);

    naturalMode.setBounds(center.getX() + 20, center.getY() + 28, 115, 25);
    modernMode.setBounds(center.getX() + 140, center.getY() + 28, 115, 25);
    hardTuneMode.setBounds(center.getX() + 260, center.getY() + 28, 125, 25);

    input.setBounds(io.getX() + 15, io.getY() + 80, 100, 120);
    output.setBounds(io.getRight() - 115, io.getY() + 80, 100, 120);

    const int bottomY = area.getY() + 475;
    const int gap = 10;
    const int count = 6;
    const int w = (area.getWidth() - 40 - gap * (count - 1)) / count;
    const int knobY = bottomY + 38;
    juce::Slider* bottom[] = { &compression, &deEss, &saturation, &air, &limiter, &mix };
    for (int i = 0; i < count; ++i)
        bottom[i]->setBounds(area.getX() + 20 + i * (w + gap) + 8, knobY, w - 16, 125);

    master.setBounds(center.getX() + 145, center.getY() + 95, 165, 110);
}

void SlikBezzleVocalMasterAudioProcessorEditor::timerCallback()
{
    // Lightweight visual metering for the mockup-style interface.
    inputMeter = juce::jlimit(0.0f, 1.0f, 0.35f + 0.18f * std::sin(juce::Time::getMillisecondCounterHiRes() * 0.004));
    outputMeter = juce::jlimit(0.0f, 1.0f, 0.30f + 0.22f * std::sin(juce::Time::getMillisecondCounterHiRes() * 0.003 + 1.0));
    repaint();
}
