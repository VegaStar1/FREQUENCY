#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <limits>

FrequencyAudioProcessor::FrequencyAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    formatManager.registerBasicFormats();
    for (auto& ptr : stationPointers)
        ptr.store(nullptr, std::memory_order_relaxed);
}

void FrequencyAudioProcessor::prepareToPlay(double sampleRate, int)
{
    outputRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    sampleClock = 0;
    dropoutSamplesRemaining = 0;
}

void FrequencyAudioProcessor::releaseResources() {}

bool FrequencyAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

float FrequencyAudioProcessor::nextNoise() noexcept
{
    // Small xorshift PRNG: avoids allocating objects or calling heavyweight RNG APIs on the audio thread.
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    return (static_cast<float>(rng & 0x00ffffffu) / 8388607.5f) - 1.0f;
}

void FrequencyAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    const int numOut = getTotalNumOutputChannels();
    for (int ch = 0; ch < getTotalNumInputChannels(); ++ch)
        buffer.clear(ch, 0, numSamples);
    for (int ch = getTotalInputChannels(); ch < numOut; ++ch)
        buffer.clear(ch, 0, numSamples);

    const int count = stationCount.load(std::memory_order_acquire);
    const float targetTune = tune.load(std::memory_order_relaxed);
    const float noiseFloor = count == 0 ? 0.18f : 0.105f;
    const double sourceRateDefault = outputRate;
    const float tuneSmoothing = 0.0025f;

    for (int i = 0; i < numSamples; ++i)
    {
        smoothedTune += (targetTune - smoothedTune) * tuneSmoothing;
        const float position = count > 1 ? smoothedTune * static_cast<float>(count - 1) : 0.0f;
        const int leftIndex = juce::jlimit(0, juce::jmax(0, count - 1), static_cast<int>(position));
        const float fraction = position - static_cast<float>(leftIndex);
        const int rightIndex = juce::jmin(count - 1, leftIndex + 1);

        // A station fades in near its center. Halfway between stations the receiver is mostly static.
        const float leftGain = (count > 0) ? juce::jlimit(0.0f, 1.0f, 1.0f - fraction * 2.0f) : 0.0f;
        const float rightGain = (count > 1 && rightIndex != leftIndex) ? juce::jlimit(0.0f, 1.0f, (fraction - 0.5f) * 2.0f) : 0.0f;
        const float totalGain = juce::jmax(leftGain, rightGain);
        const float staticGain = 1.0f - totalGain;

        const Station* left = (count > 0) ? stationPointers[static_cast<size_t>(leftIndex)].load(std::memory_order_acquire) : nullptr;
        const Station* right = (count > 1 && rightIndex != leftIndex) ? stationPointers[static_cast<size_t>(rightIndex)].load(std::memory_order_acquire) : nullptr;
        float lSample = 0.0f, rSample = 0.0f;

        auto readStation = [&](const Station* station, float gain, float& l, float& r)
        {
            if (station == nullptr || gain <= 0.0f || station->audio.getNumSamples() <= 0)
                return;
            const int length = station->audio.getNumSamples();
            int index = static_cast<int>(station->readPosition);
            if (index >= length || index < 0)
            {
                station->readPosition = 0.0;
                index = 0;
            }
            const int next = (index + 1 < length) ? index + 1 : 0;
            const float frac = static_cast<float>(station->readPosition - static_cast<double>(index));
            const int channels = station->audio.getNumChannels();
            const float aL = station->audio.getSample(0, index);
            const float bL = station->audio.getSample(0, next);
            l += (aL + (bL - aL) * frac) * gain;
            if (channels > 1)
            {
                const float aR = station->audio.getSample(1, index);
                const float bR = station->audio.getSample(1, next);
                r += (aR + (bR - aR) * frac) * gain;
            }
            else
            {
                l += 0.0f;
                r += (aL + (bL - aL) * frac) * gain;
            }
            station->readPosition += station->sampleRate / juce::jmax(1.0, sourceRateDefault);
            if (station->readPosition >= length)
                station->readPosition -= static_cast<double>(length);
        };

        // Keep each station's playback cursor advancing only while it is audible.
        readStation(left, leftGain, lSample, rSample);
        if (right != left)
            readStation(right, rightGain, lSample, rSample);

        float staticSample = nextNoise() * (noiseFloor + 0.035f * std::abs(nextNoise()));
        // Occasional short signal dropouts/clicks, with intervals measured in samples rather than timers.
        if (dropoutSamplesRemaining <= 0)
        {
            const uint32_t span = 2u + (rng % 5u);
            dropoutSamplesRemaining = static_cast<int>(span * static_cast<uint32_t>(outputRate));
        }
        --dropoutSamplesRemaining;
        float radioGain = 1.0f;
        const int dropoutPeriod = static_cast<int>(outputRate * 0.025);
        if (dropoutSamplesRemaining < dropoutPeriod && (rng & 0x7ffu) < 16u)
            radioGain = 0.18f;

        const float stationHiss = totalGain * 0.035f; // faint RF hiss remains even when tuned in
        const float l = lSample * radioGain + staticSample * (staticGain + stationHiss);
        const float r = rSample * radioGain + staticSample * (staticGain + stationHiss);
        for (int ch = 0; ch < numOut; ++ch)
            buffer.setSample(ch, i, ch == 0 ? l : r);
        sampleClock++;
    }

    const float position = count > 1 ? smoothedTune * static_cast<float>(count - 1) : 0.0f;
    const int nearest = count > 0 ? juce::jlimit(0, count - 1, static_cast<int>(std::round(position))) : -1;
    const float distance = nearest >= 0 ? std::abs(position - static_cast<float>(nearest)) : 1.0f;
    tunedStation.store((nearest >= 0 && distance < 0.12f) ? nearest : -1, std::memory_order_relaxed);
    signalLevel.store(count == 0 ? 0.0f : juce::jlimit(0.0f, 1.0f, 1.0f - distance * 2.0f), std::memory_order_relaxed);
}

bool FrequencyAudioProcessor::addStation(const juce::File& file)
{
    const juce::ScopedLock lock(loadLock);
    const int count = stationCount.load(std::memory_order_acquire);
    if (count >= maxStations || !file.existsAsFile())
        return false;

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (!reader || reader->lengthInSamples <= 0 || reader->sampleRate <= 0.0)
        return false;

    // Bound memory use: maximum 3 minutes per station and 256 MiB total decoded audio.
    constexpr double maxSecondsPerStation = 180.0;
    constexpr uint64_t maxTotalBytes = 256ull * 1024ull * 1024ull;
    if (reader->lengthInSamples > static_cast<juce::int64>(reader->sampleRate * maxSecondsPerStation))
        return false;
    uint64_t currentBytes = 0;
    for (int i = 0; i < count; ++i)
    {
        if (const auto* s = stationPointers[static_cast<size_t>(i)].load(std::memory_order_acquire))
            currentBytes += static_cast<uint64_t>(s->audio.getNumChannels()) * static_cast<uint64_t>(s->audio.getNumSamples()) * sizeof(float);
    }
    const int channels = static_cast<int>(juce::jlimit(1, 2, static_cast<int>(reader->numChannels)));
    const uint64_t newBytes = static_cast<uint64_t>(channels) * static_cast<uint64_t>(reader->lengthInSamples) * sizeof(float);
    if (currentBytes + newBytes > maxTotalBytes || reader->lengthInSamples > std::numeric_limits<int>::max())
        return false;

    auto station = std::make_unique<Station>();
    station->name = file.getFileNameWithoutExtension();
    station->sampleRate = reader->sampleRate;
    const int length = static_cast<int>(reader->lengthInSamples);
    station->audio.setSize(channels, length, false, true, false);
    if (!reader->read(&station->audio, 0, length, 0, true, channels > 1))
        return false;

    const Station* raw = station.get();
    stationOwners[static_cast<size_t>(count)] = std::move(station);
    stationPointers[static_cast<size_t>(count)].store(raw, std::memory_order_release);
    stationCount.store(count + 1, std::memory_order_release);
    return true;
}

juce::String FrequencyAudioProcessor::getStationName(int index) const
{
    if (index < 0 || index >= stationCount.load(std::memory_order_acquire))
        return {};
    const auto* station = stationPointers[static_cast<size_t>(index)].load(std::memory_order_acquire);
    return station != nullptr ? station->name : juce::String();
}

void FrequencyAudioProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    juce::XmlElement xml("FREQUENCY_STATE");
    xml.setAttribute("tune", static_cast<double>(tune.load(std::memory_order_relaxed)));
    copyXmlToBinary(xml, dest);
}

void FrequencyAudioProcessor::setStateInformation(const void* data, int size)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, size));
    if (xml != nullptr && xml->hasTagName("FREQUENCY_STATE"))
        tune.store(static_cast<float>(xml->getDoubleAttribute("tune", 0.0)), std::memory_order_relaxed);
}

juce::AudioProcessorEditor* FrequencyAudioProcessor::createEditor()
{
    return new FrequencyAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FrequencyAudioProcessor();
}
