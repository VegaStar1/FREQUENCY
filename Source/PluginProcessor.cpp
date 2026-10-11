#include "PluginProcessor.h"
#include "PluginEditor.h"

FrequencyAudioProcessor::FrequencyAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    formatManager.registerBasicFormats();
}

void FrequencyAudioProcessor::prepareToPlay(double sampleRate, int) { outputRate = sampleRate; readPosition = 0.0; }
void FrequencyAudioProcessor::releaseResources() {}
bool FrequencyAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void FrequencyAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    const int numOut = getTotalNumOutputChannels();
    for (int ch = getTotalNumInputChannels(); ch < numOut; ++ch) buffer.clear(ch, 0, numSamples);
    juce::AudioBuffer<float> stationCopy;
    int stationRate = 44100;
    {
        const juce::ScopedLock lock(stationsLock);
        const int idx = selectedStation.load();
        if (idx >= 0 && idx < (int)stations.size()) {
            stationCopy.makeCopyOf(stations[(size_t)idx]->audio);
            stationRate = stations[(size_t)idx]->sampleRate;
        }
    }
    if (stationCopy.getNumSamples() <= 0) {
        for (int ch = 0; ch < numOut; ++ch) buffer.clear(ch, 0, numSamples);
        return;
    }
    const int srcChannels = stationCopy.getNumChannels();
    const int srcLength = stationCopy.getNumSamples();
    const float targetTune = tune.load();
    const float noise = noiseAmount.load();
    const double rateRatio = (double)stationRate / juce::jmax(1.0, outputRate);
    for (int i = 0; i < numSamples; ++i) {
        smoothedTune += (targetTune - smoothedTune) * 0.0015f;
        const double basePos = readPosition;
        const int a = ((int)basePos) % srcLength;
        const int b = (a + 1) % srcLength;
        const float frac = (float)(basePos - std::floor(basePos));
        const float fade = juce::jlimit(0.0f, 1.0f, (1.0f - noise * 0.75f));
        const float staticLevel = noise * (0.025f + 0.10f * std::abs(std::sin((float)basePos * 0.0031f)));
        for (int ch = 0; ch < numOut; ++ch) {
            const int srcCh = juce::jmin(ch, srcChannels - 1);
            const float x0 = stationCopy.getSample(srcCh, a);
            const float x1 = stationCopy.getSample(srcCh, b);
            float sample = (x0 + (x1 - x0) * frac) * fade;
            sample += (random.nextFloat() * 2.0f - 1.0f) * staticLevel;
            if (noise > 0.45f && random.nextFloat() < noise * 0.0008f) sample *= 0.12f;
            buffer.setSample(ch, i, sample);
        }
        readPosition += rateRatio * (0.985 + 0.03 * smoothedTune);
        if (readPosition >= srcLength) readPosition = std::fmod(readPosition, (double)srcLength);
    }
}

bool FrequencyAudioProcessor::addStation(const juce::File& file)
{
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (!reader || reader->lengthInSamples <= 0 || reader->lengthInSamples > (juce::int64)reader->sampleRate * 60 * 20) return false;
    const juce::ScopedLock lock(stationsLock);
    if (stations.size() >= 16) return false;
    auto s = std::make_unique<Station>();
    s->name = file.getFileNameWithoutExtension();
    s->sampleRate = (int)reader->sampleRate;
    const int length = (int)reader->lengthInSamples;
    s->audio.setSize((int)juce::jlimit((juce::int64)1, (juce::int64)2, (juce::int64)reader->numChannels), length);
    reader->read(&s->audio, 0, length, 0, true, true);
    stations.push_back(std::move(s));
    if (selectedStation.load() < 0) selectedStation.store(0);
    return true;
}
int FrequencyAudioProcessor::getStationCount() const { const juce::ScopedLock lock(stationsLock); return (int)stations.size(); }
juce::String FrequencyAudioProcessor::getStationName(int i) const { const juce::ScopedLock lock(stationsLock); return (i >= 0 && i < (int)stations.size()) ? stations[(size_t)i]->name : juce::String(); }
void FrequencyAudioProcessor::selectStation(int i) { if (i >= 0 && i < getStationCount()) { selectedStation.store(i); readPosition = 0.0; } }
void FrequencyAudioProcessor::searchNext() { const int n = getStationCount(); if (n > 0) selectStation((selectedStation.load() + 1) % n); }

void FrequencyAudioProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    juce::XmlElement xml("FREQUENCY_STATE");
    xml.setAttribute("station", selectedStation.load()); xml.setAttribute("tune", (double)tune.load()); xml.setAttribute("noise", (double)noiseAmount.load());
    copyXmlToBinary(xml, dest);
}
void FrequencyAudioProcessor::setStateInformation(const void* data, int size)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, size));
    if (xml && xml->hasTagName("FREQUENCY_STATE")) {
        tune.store((float)xml->getDoubleAttribute("tune", 0.0)); noiseAmount.store((float)xml->getDoubleAttribute("noise", 0.08));
        selectedStation.store(xml->getIntAttribute("station", -1));
    }
}
juce::AudioProcessorEditor* FrequencyAudioProcessor::createEditor() { return new FrequencyAudioProcessorEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new FrequencyAudioProcessor(); }
