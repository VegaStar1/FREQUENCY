#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <memory>

class FrequencyAudioProcessor : public juce::AudioProcessor
{
public:
    static constexpr int maxStations = 8;

    FrequencyAudioProcessor();
    ~FrequencyAudioProcessor() override = default;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 1.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    bool addStation(const juce::File& file);
    int getStationCount() const { return stationCount.load(std::memory_order_acquire); }
    juce::String getStationName(int index) const;
    void setTune(float value) { tune.store(juce::jlimit(0.0f, 1.0f, value), std::memory_order_relaxed); }
    float getTune() const { return tune.load(std::memory_order_relaxed); }
    float getSignal() const { return signalLevel.load(std::memory_order_relaxed); }
    int getTunedStation() const { return tunedStation.load(std::memory_order_relaxed); }

private:
    struct Station
    {
        juce::String name;
        juce::AudioBuffer<float> audio;
        double sampleRate = 44100.0;
        mutable double readPosition = 0.0; // Audio thread only
    };

    std::array<std::unique_ptr<Station>, maxStations> stationOwners;
    std::array<std::atomic<const Station*>, maxStations> stationPointers {};
    std::atomic<int> stationCount { 0 };
    std::atomic<float> tune { 0.0f }, signalLevel { 0.0f };
    std::atomic<int> tunedStation { -1 };
    juce::CriticalSection loadLock; // UI/file-loading thread only; never used in processBlock
    juce::AudioFormatManager formatManager;
    double outputRate = 44100.0;
    float smoothedTune = 0.0f;
    uint32_t rng = 0xA341316Cu;
    uint64_t sampleClock = 0;
    int dropoutSamplesRemaining = 0;

    float nextNoise() noexcept;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FrequencyAudioProcessor)
};
