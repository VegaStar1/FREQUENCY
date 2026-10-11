#include "PluginEditor.h"

namespace
{
    const juce::Colour bg(0xff101214), panel(0xff1a1d20), amber(0xffffbd59), dim(0xff778087), text(0xfff1eee7);
}

FrequencyAudioProcessorEditor::FrequencyAudioProcessorEditor(FrequencyAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(520, 430);
    setResizable(false, false);

    title.setText("FREQUENCY", juce::dontSendNotification);
    title.setFont(juce::Font(juce::FontOptions(19.0f, juce::Font::bold)));
    title.setColour(juce::Label::textColourId, text);
    title.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(title);

    frequencyLabel.setText("FM  87.5", juce::dontSendNotification);
    frequencyLabel.setFont(juce::Font(juce::FontOptions(25.0f, juce::Font::bold)));
    frequencyLabel.setColour(juce::Label::textColourId, amber);
    frequencyLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(frequencyLabel);

    stationLabel.setText("NO STATIONS", juce::dontSendNotification);
    stationLabel.setFont(juce::Font(juce::FontOptions(12.0f)));
    stationLabel.setColour(juce::Label::textColourId, text);
    stationLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(stationLabel);

    hintLabel.setText("DROP AUDIO FILES HERE", juce::dontSendNotification);
    hintLabel.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
    hintLabel.setColour(juce::Label::textColourId, dim);
    hintLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(hintLabel);

    dial.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    dial.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    dial.setRange(0.0, 1.0, 0.0005);
    dial.setValue(0.0, juce::dontSendNotification);
    dial.setDoubleClickReturnValue(true, 0.0);
    dial.setColour(juce::Slider::rotarySliderFillColourId, amber);
    dial.setColour(juce::Slider::rotarySliderOutlineColourId, dim.withAlpha(0.45f));
    dial.setColour(juce::Slider::thumbColourId, text);
    dial.onValueChange = [this] { processor.setTune(static_cast<float>(dial.getValue())); };
    addAndMakeVisible(dial);

    loadButton.setColour(juce::TextButton::buttonColourId, panel);
    loadButton.setColour(juce::TextButton::textColourOffId, amber);
    loadButton.onClick = [this]
    {
        chooser.launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::canSelectMultiple,
            [this](const juce::FileChooser& fc) {
                juce::StringArray paths;
                for (const auto& f : fc.getResults()) paths.add(f.getFullPathName());
                loadFiles(paths);
            });
    };
    addAndMakeVisible(loadButton);
    startTimerHz(6);
}

bool FrequencyAudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (const auto& path : files)
    {
        const juce::File f(path);
        const auto ext = f.getFileExtension().toLowerCase();
        if (ext == ".wav" || ext == ".aif" || ext == ".aiff" || ext == ".flac" || ext == ".mp3" || ext == ".ogg")
            return true;
    }
    return false;
}

void FrequencyAudioProcessorEditor::filesDropped(const juce::StringArray& files, int, int)
{
    loadFiles(files);
}

void FrequencyAudioProcessorEditor::loadFiles(const juce::StringArray& files)
{
    int added = 0;
    int rejected = 0;
    for (const auto& path : files)
    {
        if (processor.addStation(juce::File(path))) ++added;
        else ++rejected;
    }
    const int count = processor.getStationCount();
    if (count > 0)
        stationLabel.setText(processor.getStationName(processor.getTunedStation() >= 0 ? processor.getTunedStation() : 0).toUpperCase(), juce::dontSendNotification);
    if (rejected > 0)
        hintLabel.setText("LIMIT: 8 STATIONS / 3 MIN EACH / 256 MB", juce::dontSendNotification);
    else if (added > 0)
        hintLabel.setText("STATION ADDED  /  TURN THE DIAL", juce::dontSendNotification);
}

void FrequencyAudioProcessorEditor::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (dial.getBounds().expanded(12).contains(e.getPosition()))
    {
        dial.setValue(juce::jlimit(0.0, 1.0, dial.getValue() - wheel.deltaY * 0.08));
        return;
    }
    AudioProcessorEditor::mouseWheelMove(e, wheel);
}

void FrequencyAudioProcessorEditor::timerCallback()
{
    const int count = processor.getStationCount();
    const float tune = processor.getTune();
    const float frequency = 87.5f + tune * 20.0f;
    frequencyLabel.setText("FM  " + juce::String(frequency, 1), juce::dontSendNotification);
    if (count == 0)
    {
        stationLabel.setText("NO STATIONS", juce::dontSendNotification);
        return;
    }
    const int tuned = processor.getTunedStation();
    const float signal = processor.getSignal();
    if (tuned >= 0)
        stationLabel.setText(processor.getStationName(tuned).toUpperCase(), juce::dontSendNotification);
    else if (signal > 0.08f)
        stationLabel.setText("TUNING...", juce::dontSendNotification);
    else
        stationLabel.setText("STATIC / SEARCHING", juce::dontSendNotification);
}

void FrequencyAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(bg);
    g.setColour(panel);
    g.fillRoundedRectangle(24.0f, 62.0f, 472.0f, 328.0f, 18.0f);
    g.setColour(amber.withAlpha(0.35f));
    g.drawRoundedRectangle(24.0f, 62.0f, 472.0f, 328.0f, 18.0f, 1.0f);

    // Analog scale: inexpensive static drawing; only the needle position changes per repaint.
    const auto scale = juce::Rectangle<float>(70.0f, 94.0f, 380.0f, 32.0f);
    g.setColour(dim.withAlpha(0.6f));
    for (int i = 0; i <= 40; ++i)
    {
        const float x = scale.getX() + scale.getWidth() * static_cast<float>(i) / 40.0f;
        const float h = (i % 5 == 0) ? 18.0f : 8.0f;
        g.drawLine(x, scale.getBottom(), x, scale.getBottom() - h, (i % 5 == 0) ? 1.5f : 1.0f);
    }
    const float needleX = scale.getX() + scale.getWidth() * processor.getTune();
    g.setColour(amber);
    g.drawLine(needleX, scale.getY() - 3.0f, needleX, scale.getBottom() + 4.0f, 2.0f);
    g.setColour(dim);
    g.setFont(juce::Font(juce::FontOptions(9.0f)));
    g.drawText("87.5", 64, 132, 50, 14, juce::Justification::left);
    g.drawText("107.5", 407, 132, 50, 14, juce::Justification::right);

    // Small signal meter, no animated waveform or expensive visualizer.
    const float signal = processor.getSignal();
    g.setColour(bg);
    g.fillRoundedRectangle(90.0f, 350.0f, 340.0f, 5.0f, 2.5f);
    g.setColour(amber.withAlpha(0.85f));
    g.fillRoundedRectangle(90.0f, 350.0f, 340.0f * signal, 5.0f, 2.5f);
}

void FrequencyAudioProcessorEditor::resized()
{
    title.setBounds(40, 18, 440, 30);
    frequencyLabel.setBounds(100, 145, 320, 38);
    dial.setBounds(135, 177, 250, 150);
    stationLabel.setBounds(52, 325, 416, 22);
    loadButton.setBounds(170, 398, 180, 24);
    hintLabel.setBounds(50, 365, 420, 18);
}
