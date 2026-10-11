#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class FrequencyAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit FrequencyAudioProcessorEditor(FrequencyAudioProcessor&);
    ~FrequencyAudioProcessorEditor() override = default;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    void refreshStations();
    FrequencyAudioProcessor& processor;
    juce::Label title, frequencyLabel, stationLabel, statusLabel, noiseLabel;
    juce::ComboBox stations;
    juce::TextButton addButton { "+ ADD STATION" }, scanButton { "SCAN  ›" };
    juce::Slider tuneSlider, noiseSlider;
    juce::FileChooser chooser { "Select audio files", juce::File::getSpecialLocation(juce::File::userMusicDirectory), "*.wav;*.aif;*.aiff;*.flac;*.mp3" };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FrequencyAudioProcessorEditor)
};
