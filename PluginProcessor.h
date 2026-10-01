#pragma once
#include <JuceHeader.h>

class SlikBezzleVocalMasterAudioProcessor : public juce::AudioProcessor
{
public:
    SlikBezzleVocalMasterAudioProcessor();
    ~SlikBezzleVocalMasterAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "SLIK BEZZLE VOCAL MASTER"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    int getDetectedKey() const noexcept { return pitchCorrector.getDetectedKey(); }
    int getDetectedScale() const noexcept { return pitchCorrector.getDetectedScale(); }
    float getDetectedFrequency() const noexcept { return pitchCorrector.getDetectedFrequency(); }

private:
    using Filter = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>;

    juce::dsp::Compressor<float> compressor;
    juce::dsp::Limiter<float> limiter;
    Filter airFilter;
    Filter deEssFilter;
    double currentSampleRate = 44100.0;

    float getParameter(const char* id) const;
    void updateFilters(float airAmount, float deEssAmount);

    class VocalPitchCorrector
    {
    public:
        void prepare(double sampleRate, int maxBlockSize, int channels);
        void reset();
        void process(juce::AudioBuffer<float>& buffer, bool enabled, float amount, float retuneMs,
                     float humanize, int key, int scale, int mode, bool autoKey);
        int getDetectedKey() const noexcept { return detectedKey; }
        int getDetectedScale() const noexcept { return detectedScale; }
        float getDetectedFrequency() const noexcept { return detectedFrequency; }

    private:
        void analyze(const juce::AudioBuffer<float>& buffer, int startSample, int numSamples);
        float detectPitch(const float* samples, int numSamples) const;
        float targetFrequency(float frequency, int key, int scale, float amount, int mode) const;
        void updateAutoKey(float frequency);
        float readLinear(int channel, double position) const;

        double sampleRate = 44100.0;
        int numChannels = 2;
        int writePos = 0;
        int grainSize = 1024;
        std::vector<float> ring;
        std::vector<double> readPos1, readPos2;
        std::vector<float> age1, age2;
        float currentRatio = 1.0f;
        float targetRatio = 1.0f;
        float detectedFrequency = 0.0f;
        int detectedKey = 0;
        int detectedScale = 1;
        std::array<float, 12> pitchClassEnergy{};
        int analysisCounter = 0;
    };

    VocalPitchCorrector pitchCorrector;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SlikBezzleVocalMasterAudioProcessor)
};
