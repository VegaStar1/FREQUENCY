#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class FrequencyAudioProcessorEditor : public juce::AudioProcessorEditor,
                                      private juce::Timer,
                                      private juce::FileDragAndDropTarget
{
public:
    explicit FrequencyAudioProcessorEditor(FrequencyAudioProcessor&);
    ~FrequencyAudioProcessorEditor() override = default;
    void paint(juce::Graphics&) override;
    void resized() override;
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    void timerCallback() override;
    void loadFiles(const juce::StringArray& files);
    FrequencyAudioProcessor& processor;
    juce::Slider dial;
    juce::TextButton loadButton { "LOAD AUDIO" };
    juce::Label title, frequencyLabel, stationLabel, hintLabel;
    juce::FileChooser chooser { "Add radio stations", juce::File::getSpecialLocation(juce::File::userMusicDirectory), "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg" };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FrequencyAudioProcessorEditor)
};
