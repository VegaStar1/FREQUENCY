#pragma once
#include <JuceHeader.h>
#include <vector>

class FrequencyAudioProcessor : public juce::AudioProcessor
{
public:
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
    int getStationCount() const;
    juce::String getStationName(int index) const;
    void selectStation(int index);
    int getSelectedStation() const { return selectedStation.load(); }
    void setTune(float value) { tune.store(juce::jlimit(0.0f, 1.0f, value)); }
    float getTune() const { return tune.load(); }
    void setNoise(float value) { noiseAmount.store(juce::jlimit(0.0f, 1.0f, value)); }
    float getNoise() const { return noiseAmount.load(); }
    void searchNext();

private:
    struct Station { juce::String name; juce::AudioBuffer<float> audio; int sampleRate = 44100; };
    mutable juce::CriticalSection stationsLock;
    std::vector<std::unique_ptr<Station>> stations;
    std::atomic<int> selectedStation { -1 };
    std::atomic<float> tune { 0.0f }, noiseAmount { 0.08f };
    double outputRate = 44100.0;
    double readPosition = 0.0;
    float smoothedTune = 0.0f;
    juce::Random random;
    juce::AudioFormatManager formatManager;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FrequencyAudioProcessor)
};
