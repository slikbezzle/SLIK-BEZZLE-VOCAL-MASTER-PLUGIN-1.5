#include "PluginProcessor.h"
#include "PluginEditor.h"



void SlikBezzleVocalMasterAudioProcessor::VocalPitchCorrector::prepare(double sr, int maxBlockSize, int channels)
{
    sampleRate = sr;
    numChannels = juce::jmax(1, channels);
    grainSize = 1024;
    const int ringSize = 8192;
    ring.assign((size_t) ringSize * (size_t) numChannels, 0.0f);
    readPos1.assign((size_t) numChannels, 0.0);
    readPos2.assign((size_t) numChannels, 0.0);
    age1.assign((size_t) numChannels, 0.0f);
    age2.assign((size_t) numChannels, 0.5f);
    writePos = grainSize + 16;
    currentRatio = targetRatio = 1.0f;
    detectedFrequency = 0.0f;
    detectedKey = 0;
    detectedScale = 1;
    pitchClassEnergy.fill(0.0f);
    analysisCounter = 0;
    reset();
}

void SlikBezzleVocalMasterAudioProcessor::VocalPitchCorrector::reset()
{
    std::fill(ring.begin(), ring.end(), 0.0f);
    for (int ch = 0; ch < numChannels; ++ch)
    {
        readPos1[(size_t) ch] = 0.0;
        readPos2[(size_t) ch] = 0.0;
        age1[(size_t) ch] = 0.0f;
        age2[(size_t) ch] = 0.5f;
    }
    writePos = grainSize + 16;
    currentRatio = targetRatio = 1.0f;
}

float SlikBezzleVocalMasterAudioProcessor::VocalPitchCorrector::detectPitch(const float* samples, int numSamples) const
{
    constexpr int minLag = 88;   // ~500 Hz at 44.1 kHz
    constexpr int maxLag = 630;  // ~70 Hz at 44.1 kHz
    if (numSamples < 768)
        return 0.0f;

    float rms = 0.0f;
    for (int i = 0; i < numSamples; ++i)
        rms += samples[i] * samples[i];
    rms = std::sqrt(rms / (float) numSamples);
    if (rms < 0.006f)
        return 0.0f;

    float best = 0.0f;
    int bestLag = 0;
    for (int lag = minLag; lag <= juce::jmin(maxLag, numSamples - 2); lag += 2)
    {
        float corr = 0.0f, e1 = 0.0f, e2 = 0.0f;
        for (int i = 0; i < numSamples - lag; ++i)
        {
            const float a = samples[i];
            const float b = samples[i + lag];
            corr += a * b;
            e1 += a * a;
            e2 += b * b;
        }
        const float norm = corr / std::sqrt(e1 * e2 + 1.0e-12f);
        if (norm > best)
        {
            best = norm;
            bestLag = lag;
        }
    }

    if (best < 0.72f || bestLag <= 0)
        return 0.0f;

    return (float) (sampleRate / (double) bestLag);
}

void SlikBezzleVocalMasterAudioProcessor::VocalPitchCorrector::updateAutoKey(float frequency)
{
    if (frequency <= 0.0f)
        return;
    const float midi = 69.0f + 12.0f * std::log2(frequency / 440.0f);
    const int pc = ((int) std::lround(midi) % 12 + 12) % 12;
    pitchClassEnergy[(size_t) pc] = juce::jmin(50.0f, pitchClassEnergy[(size_t) pc] + 1.0f);
    for (auto& v : pitchClassEnergy)
        v *= 0.995f;

    static constexpr int major[] = { 0, 2, 4, 5, 7, 9, 11 };
    static constexpr int minor[] = { 0, 2, 3, 5, 7, 8, 10 };
    float bestScore = -1.0e9f;
    int bestRoot = 0, bestScale = 0;
    for (int scale = 0; scale < 2; ++scale)
    {
        const int* pattern = scale == 0 ? major : minor;
        for (int root = 0; root < 12; ++root)
        {
            float score = 0.0f;
            for (int i = 0; i < 12; ++i)
            {
                const bool inScale = std::find(pattern, pattern + 7, (i - root + 12) % 12) != pattern + 7;
                score += pitchClassEnergy[(size_t) i] * (inScale ? 1.0f : -0.35f);
            }
            if (score > bestScore)
            {
                bestScore = score;
                bestRoot = root;
                bestScale = scale;
            }
        }
    }
    detectedKey = bestRoot;
    detectedScale = bestScale;
}

float SlikBezzleVocalMasterAudioProcessor::VocalPitchCorrector::targetFrequency(float frequency, int key, int scale, float amount, int mode) const
{
    if (frequency <= 0.0f)
        return frequency;

    static constexpr int major[] = { 0, 2, 4, 5, 7, 9, 11 };
    static constexpr int minor[] = { 0, 2, 3, 5, 7, 8, 10 };
    const int* pattern = scale == 0 ? major : minor;
    const float midi = 69.0f + 12.0f * std::log2(frequency / 440.0f);
    const int base = (int) std::floor(midi);
    float bestMidi = (float) base;
    float bestDistance = 1000.0f;
    for (int octave = -1; octave <= 1; ++octave)
        for (int degree = 0; degree < 7; ++degree)
        {
            const float candidate = (float) (key + 12 * (octave + 5) + pattern[degree]);
            const float distance = std::abs(candidate - midi);
            if (distance < bestDistance)
            {
                bestDistance = distance;
                bestMidi = candidate;
            }
        }

    float strength = juce::jlimit(0.0f, 1.0f, amount);
    if (mode == 0) strength *= 0.55f;
    if (mode == 2) strength = juce::jlimit(0.0f, 1.0f, strength * 1.35f);
    const float correctedMidi = midi + (bestMidi - midi) * strength;
    return 440.0f * std::pow(2.0f, (correctedMidi - 69.0f) / 12.0f);
}

float SlikBezzleVocalMasterAudioProcessor::VocalPitchCorrector::readLinear(int channel, double position) const
{
    const int ringSize = (int) ring.size() / numChannels;
    while (position < 0.0) position += ringSize;
    while (position >= ringSize) position -= ringSize;
    const int i0 = (int) position;
    const int i1 = (i0 + 1) % ringSize;
    const float frac = (float) (position - i0);
    const float a = ring[(size_t) i0 * (size_t) numChannels + (size_t) channel];
    const float b = ring[(size_t) i1 * (size_t) numChannels + (size_t) channel];
    return a + (b - a) * frac;
}

void SlikBezzleVocalMasterAudioProcessor::VocalPitchCorrector::analyze(const juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    const int n = juce::jmin(numSamples, 1024);
    if (n < 768 || buffer.getNumChannels() == 0)
        return;
    std::vector<float> mono((size_t) n, 0.0f);
    for (int i = 0; i < n; ++i)
    {
        float v = 0.0f;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            v += buffer.getSample(ch, startSample + i);
        mono[(size_t) i] = v / (float) buffer.getNumChannels();
    }
    detectedFrequency = detectPitch(mono.data(), n);
    if (detectedFrequency > 0.0f)
        updateAutoKey(detectedFrequency);
}

void SlikBezzleVocalMasterAudioProcessor::VocalPitchCorrector::process(juce::AudioBuffer<float>& buffer, bool enabled, float amount, float retuneMs,
                                                                        float humanize, int key, int scale, int mode, bool autoKey)
{
    if (!enabled || amount <= 0.001f || buffer.getNumSamples() == 0)
        return;

    const int channels = juce::jmin(numChannels, buffer.getNumChannels());
    const int ringSize = (int) ring.size() / numChannels;
    const int analysisHop = 256;
    for (int n = 0; n < buffer.getNumSamples(); ++n)
    {
        if (analysisCounter++ % analysisHop == 0)
            analyze(buffer, n, buffer.getNumSamples() - n);

        if (detectedFrequency > 0.0f)
        {
            const int useKey = autoKey ? detectedKey : key;
            const int useScale = autoKey ? detectedScale : scale;
            const float desired = targetFrequency(detectedFrequency, useKey, useScale, amount, mode);
            const float ratio = juce::jlimit(0.5f, 2.0f, desired / detectedFrequency);
            const float human = juce::jlimit(0.0f, 1.0f, humanize);
            targetRatio = 1.0f + (ratio - 1.0f) * (1.0f - human * 0.75f);
            const float timeConstant = juce::jlimit(0.005f, 0.2f, retuneMs * 0.001f);
            const float smoothing = 1.0f - std::exp(-1.0f / (float) (sampleRate * timeConstant));
            currentRatio += (targetRatio - currentRatio) * smoothing;
        }
        else
        {
            currentRatio += (1.0f - currentRatio) * 0.0025f;
        }

        const int wp = writePos % ringSize;
        for (int ch = 0; ch < channels; ++ch)
            ring[(size_t) wp * (size_t) numChannels + (size_t) ch] = buffer.getSample(ch, n);

        const float ratio = juce::jlimit(0.5f, 2.0f, currentRatio);
        for (int ch = 0; ch < channels; ++ch)
        {
            const double readBase = (double) writePos - grainSize;
            if (age1[(size_t) ch] >= 1.0f)
            {
                age1[(size_t) ch] = 0.0f;
                readPos1[(size_t) ch] = readBase;
            }
            if (age2[(size_t) ch] >= 1.0f)
            {
                age2[(size_t) ch] = 0.0f;
                readPos2[(size_t) ch] = readBase + grainSize * 0.5;
            }

            const float a = readLinear(ch, readPos1[(size_t) ch]);
            const float b = readLinear(ch, readPos2[(size_t) ch]);
            const float wa = 0.5f - 0.5f * std::cos(2.0f * juce::MathConstants<float>::pi * age1[(size_t) ch]);
            const float wb = 0.5f - 0.5f * std::cos(2.0f * juce::MathConstants<float>::pi * age2[(size_t) ch]);
            const float sumW = juce::jmax(0.001f, wa + wb);
            buffer.setSample(ch, n, (a * wa + b * wb) / sumW);
            readPos1[(size_t) ch] += ratio;
            readPos2[(size_t) ch] += ratio;
            age1[(size_t) ch] += 1.0f / (float) grainSize;
            age2[(size_t) ch] += 1.0f / (float) grainSize;
        }
        ++writePos;
    }
}

SlikBezzleVocalMasterAudioProcessor::SlikBezzleVocalMasterAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout
SlikBezzleVocalMasterAudioProcessor::createParameterLayout()
{
    using F = juce::AudioParameterFloat;
    using B = juce::AudioParameterBool;
    using C = juce::AudioParameterChoice;
    juce::AudioProcessorValueTreeState::ParameterLayout p;

    p.add(std::make_unique<F>("master", "Vocal Master", juce::NormalisableRange<float>(0.0f, 1.0f), 0.70f));
    p.add(std::make_unique<B>("autotuneOn", "Auto Tune", true));
    p.add(std::make_unique<F>("tuneAmount", "Tune Amount", juce::NormalisableRange<float>(0.0f, 1.0f), 0.65f));
    p.add(std::make_unique<C>("tuneMode", "Tune Mode", juce::StringArray { "Natural", "Modern", "Hard Tune" }, 0));
    p.add(std::make_unique<F>("retune", "Retune Speed", juce::NormalisableRange<float>(1.0f, 100.0f, 1.0f), 18.0f));
    p.add(std::make_unique<F>("humanize", "Humanize", juce::NormalisableRange<float>(0.0f, 1.0f), 0.35f));
    p.add(std::make_unique<B>("autoKey", "Auto Key", true));
    p.add(std::make_unique<C>("key", "Key", juce::StringArray { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 0));
    p.add(std::make_unique<C>("scale", "Scale", juce::StringArray { "Major", "Minor" }, 1));
    p.add(std::make_unique<F>("compress", "Compression", juce::NormalisableRange<float>(0.0f, 1.0f), 0.55f));
    p.add(std::make_unique<F>("deess", "De-Esser", juce::NormalisableRange<float>(0.0f, 1.0f), 0.30f));
    p.add(std::make_unique<F>("saturation", "Saturation", juce::NormalisableRange<float>(0.0f, 1.0f), 0.20f));
    p.add(std::make_unique<F>("air", "Air", juce::NormalisableRange<float>(0.0f, 1.0f), 0.35f));
    p.add(std::make_unique<F>("limiter", "Limiter", juce::NormalisableRange<float>(0.0f, 1.0f), 0.60f));
    p.add(std::make_unique<F>("mix", "Mix", juce::NormalisableRange<float>(0.0f, 1.0f), 1.0f));
    p.add(std::make_unique<F>("input", "Input", juce::NormalisableRange<float>(-24.0f, 12.0f, 0.01f), 0.0f));
    p.add(std::make_unique<F>("output", "Output", juce::NormalisableRange<float>(-24.0f, 12.0f, 0.01f), 0.0f));
    return p;
}

float SlikBezzleVocalMasterAudioProcessor::getParameter(const char* id) const
{
    if (auto* v = apvts.getRawParameterValue(id))
        return v->load();
    return 0.0f;
}

void SlikBezzleVocalMasterAudioProcessor::prepareToPlay(double sr, int blockSize)
{
    currentSampleRate = sr;
    juce::dsp::ProcessSpec spec { sr, (juce::uint32) blockSize, (juce::uint32) getTotalNumOutputChannels() };

    compressor.prepare(spec);
    limiter.prepare(spec);
    airFilter.prepare(spec);
    deEssFilter.prepare(spec);
    pitchCorrector.prepare(sr, blockSize, getTotalNumOutputChannels());

    compressor.setAttack(8.0f);
    compressor.setRelease(80.0f);
    limiter.setRelease(60.0f);

    updateFilters(getParameter("air"), getParameter("deess"));
}

void SlikBezzleVocalMasterAudioProcessor::releaseResources()
{
}

bool SlikBezzleVocalMasterAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto input = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();
    if (input != juce::AudioChannelSet::mono() && input != juce::AudioChannelSet::stereo())
        return false;
    return input == output;
}

void SlikBezzleVocalMasterAudioProcessor::updateFilters(float airAmount, float deEssAmount)
{
    const auto airDb = juce::jmap(airAmount, 0.0f, 1.0f, 0.0f, 7.0f);
    const auto deEssDb = juce::jmap(deEssAmount, 0.0f, 1.0f, 0.0f, -9.0f);

    *airFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
        currentSampleRate, 8500.0f, 0.707f, juce::Decibels::decibelsToGain(airDb));

    *deEssFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
        currentSampleRate, 6500.0f, 0.85f, juce::Decibels::decibelsToGain(deEssDb));
}

void SlikBezzleVocalMasterAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const auto numSamples = buffer.getNumSamples();
    const auto inputDb = getParameter("input");
    const auto outputDb = getParameter("output");
    const auto comp = getParameter("compress");
    const auto deEss = getParameter("deess");
    const auto saturation = getParameter("saturation");
    const auto air = getParameter("air");
    const auto limiterAmount = getParameter("limiter");
    const auto master = getParameter("master");
    const auto mix = juce::jlimit(0.0f, 1.0f, getParameter("mix"));

    juce::AudioBuffer<float> dry;
    dry.makeCopyOf(buffer);

    buffer.applyGain(juce::Decibels::decibelsToGain(inputDb));

    const bool autoTuneOn = getParameter("autotuneOn") > 0.5f;
    const bool autoKeyOn = getParameter("autoKey") > 0.5f;
    const int key = (int) getParameter("key");
    const int scale = (int) getParameter("scale");
    const float tuneAmount = getParameter("tuneAmount");
    const float retuneMs = getParameter("retune");
    const float humanize = getParameter("humanize");
    const int tuneMode = (int) getParameter("tuneMode");
    pitchCorrector.process(buffer, autoTuneOn, tuneAmount, retuneMs, humanize, key, scale, tuneMode, autoKeyOn);

    updateFilters(air, deEss);

    // De-esser / air are deliberately kept simple and CPU-light for this first production build.
    juce::dsp::AudioBlock<float> block(buffer);
    deEssFilter.process(juce::dsp::ProcessContextReplacing<float>(block));
    airFilter.process(juce::dsp::ProcessContextReplacing<float>(block));

    compressor.setThreshold(juce::jmap(comp, 0.0f, 1.0f, -6.0f, -30.0f));
    compressor.setRatio(juce::jmap(comp, 0.0f, 1.0f, 1.0f, 8.0f));
    compressor.setAttack(juce::jmap(comp, 0.0f, 1.0f, 25.0f, 5.0f));
    compressor.setRelease(juce::jmap(comp, 0.0f, 1.0f, 150.0f, 60.0f));
    compressor.process(juce::dsp::ProcessContextReplacing<float>(block));

    // Gentle soft clipping for harmonic density.
    const auto satDrive = juce::jmap(saturation, 0.0f, 1.0f, 1.0f, 2.8f);
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i)
        {
            const auto x = data[i] * satDrive;
            const auto shaped = std::tanh(x);
            data[i] = juce::jmap(saturation, x, shaped);
        }
    }

    limiter.setThreshold(juce::jmap(limiterAmount, 0.0f, 1.0f, -0.1f, -10.0f));
    limiter.process(juce::dsp::ProcessContextReplacing<float>(block));

    const auto masterGain = juce::jmap(master, 0.0f, 1.0f, 0.70f, 1.12f);
    buffer.applyGain(masterGain * juce::Decibels::decibelsToGain(outputDb));

    if (mix < 1.0f)
    {
        buffer.applyGain(mix);
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.addFrom(ch, 0, dry, ch, 0, numSamples, 1.0f - mix);
    }

}

juce::AudioProcessorEditor* SlikBezzleVocalMasterAudioProcessor::createEditor()
{
    return new SlikBezzleVocalMasterAudioProcessorEditor(*this);
}

void SlikBezzleVocalMasterAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void SlikBezzleVocalMasterAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SlikBezzleVocalMasterAudioProcessor();
}
