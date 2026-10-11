#include "PluginEditor.h"

namespace { const juce::Colour bg(0xff111416), panel(0xff1a1f21), amber(0xffffbd59), muted(0xff7e8b8e), ink(0xfff2f0e8); }
FrequencyAudioProcessorEditor::FrequencyAudioProcessorEditor(FrequencyAudioProcessor& p) : AudioProcessorEditor(&p), processor(p)
{
    setSize(620, 420);
    title.setText("FREQUENCY", juce::dontSendNotification); title.setFont(juce::Font(juce::FontOptions(27.0f, juce::Font::bold))); title.setColour(juce::Label::textColourId, ink); addAndMakeVisible(title);
    frequencyLabel.setText("FM  087.5", juce::dontSendNotification); frequencyLabel.setFont(juce::Font(juce::FontOptions(42.0f, juce::Font::bold))); frequencyLabel.setColour(juce::Label::textColourId, amber); addAndMakeVisible(frequencyLabel);
    stationLabel.setText("NO STATION TUNED", juce::dontSendNotification); stationLabel.setFont(juce::Font(juce::FontOptions(13.0f))); stationLabel.setColour(juce::Label::textColourId, muted); addAndMakeVisible(stationLabel);
    statusLabel.setText("●  READY TO RECEIVE", juce::dontSendNotification); statusLabel.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold))); statusLabel.setColour(juce::Label::textColourId, amber); addAndMakeVisible(statusLabel);
    noiseLabel.setText("ATMOSPHERE / INTERFERENCE", juce::dontSendNotification); noiseLabel.setFont(juce::Font(juce::FontOptions(11.0f))); noiseLabel.setColour(juce::Label::textColourId, muted); addAndMakeVisible(noiseLabel);
    addButton.onClick = [this] { chooser.launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::canSelectMultipleItems, [this](const juce::FileChooser& fc) { for (auto f : fc.getResults()) processor.addStation(f); refreshStations(); }); };
    addButton.setColour(juce::TextButton::buttonColourId, panel); addButton.setColour(juce::TextButton::textColourOffId, amber); addAndMakeVisible(addButton);
    scanButton.onClick = [this] { processor.searchNext(); refreshStations(); }; scanButton.setColour(juce::TextButton::buttonColourId, amber); scanButton.setColour(juce::TextButton::textColourOffId, bg); addAndMakeVisible(scanButton);
    stations.onChange = [this] { processor.selectStation(stations.getSelectedItemIndex()); refreshStations(); }; stations.setColour(juce::ComboBox::backgroundColourId, panel); stations.setColour(juce::ComboBox::textColourId, ink); addAndMakeVisible(stations);
    tuneSlider.setSliderStyle(juce::Slider::LinearHorizontal); tuneSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0); tuneSlider.setRange(0.0, 1.0, 0.001); tuneSlider.setValue(0.0); tuneSlider.onValueChange = [this] { processor.setTune((float)tuneSlider.getValue()); }; tuneSlider.setColour(juce::Slider::trackColourId, amber); tuneSlider.setColour(juce::Slider::thumbColourId, ink); addAndMakeVisible(tuneSlider);
    noiseSlider.setSliderStyle(juce::Slider::LinearHorizontal); noiseSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0); noiseSlider.setRange(0.0, 1.0, 0.01); noiseSlider.setValue(0.08); noiseSlider.onValueChange = [this] { processor.setNoise((float)noiseSlider.getValue()); }; noiseSlider.setColour(juce::Slider::trackColourId, muted); noiseSlider.setColour(juce::Slider::thumbColourId, amber); addAndMakeVisible(noiseSlider);
    refreshStations(); startTimerHz(8);
}
void FrequencyAudioProcessorEditor::refreshStations()
{
    const int selected = processor.getSelectedStation();
    stations.clear(juce::dontSendNotification);
    for (int i = 0; i < processor.getStationCount(); ++i) stations.addItem(juce::String(i + 1).paddedLeft('0', 2) + "   /   " + processor.getStationName(i), i + 1);
    if (selected >= 0 && selected < processor.getStationCount()) { stations.setSelectedItemIndex(selected, juce::dontSendNotification); stationLabel.setText("NOW PLAYING  /  " + processor.getStationName(selected).toUpperCase(), juce::dontSendNotification); statusLabel.setText("●  SIGNAL LOCKED", juce::dontSendNotification); }
    else { stationLabel.setText("LOAD AUDIO FILES TO CREATE STATIONS", juce::dontSendNotification); statusLabel.setText("●  SEARCHING FOR SIGNAL", juce::dontSendNotification); }
}
void FrequencyAudioProcessorEditor::timerCallback()
{
    const int count = processor.getStationCount();
    const int idx = processor.getSelectedStation();
    const float f = 87.5f + processor.getTune() * 20.0f;
    frequencyLabel.setText("FM  " + juce::String(f, 1), juce::dontSendNotification);
    if (count == 0) statusLabel.setText("●  READY TO RECEIVE", juce::dontSendNotification);
}
void FrequencyAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(bg); g.setColour(panel); g.fillRoundedRectangle(24.0f, 88.0f, 572.0f, 128.0f, 12.0f);
    g.setColour(amber.withAlpha(0.25f)); g.drawRoundedRectangle(24.0f, 88.0f, 572.0f, 128.0f, 12.0f, 1.0f);
    g.setColour(muted); g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold))); g.drawText("ANALOG TUNER     /     FM BAND", 42, 104, 240, 16, juce::Justification::left);
    for (int i = 0; i < 48; ++i) { const float x = 42.0f + i * 11.2f; const float h = (i % 4 == 0) ? 16.0f : 7.0f; g.setColour(i < (int)(processor.getTune() * 48.0f) ? amber : muted.withAlpha(0.35f)); g.drawLine(x, 193.0f, x, 193.0f - h, 2.0f); }
    g.setColour(panel); g.fillRoundedRectangle(24.0f, 238.0f, 572.0f, 78.0f, 10.0f); g.fillRoundedRectangle(24.0f, 330.0f, 572.0f, 66.0f, 10.0f);
}
void FrequencyAudioProcessorEditor::resized()
{
    title.setBounds(26, 20, 300, 42); statusLabel.setBounds(360, 26, 230, 22); frequencyLabel.setBounds(40, 116, 330, 58); stationLabel.setBounds(40, 175, 500, 20);
    addButton.setBounds(40, 252, 150, 40); scanButton.setBounds(468, 252, 104, 40); stations.setBounds(205, 252, 250, 40);
    noiseLabel.setBounds(40, 343, 260, 20); noiseSlider.setBounds(310, 340, 260, 26); tuneSlider.setBounds(40, 203, 530, 10);
}
