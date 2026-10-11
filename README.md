# FREQUENCY — private VST3 prototype

A JUCE-based Windows VST3 prototype for Ableton Live. Audio files are loaded as stations; the interface provides station selection, scan-next, a tuning control, and adjustable noise/interference.

## Build without installing a compiler locally

This project includes a GitHub Actions workflow that builds on a hosted Windows runner. Create a GitHub repository, upload these project files, then open **Actions → Build FREQUENCY VST3 (Windows) → Run workflow**. Download the `FREQUENCY-Windows-VST3` artifact from the completed run.

## Local build

Requires CMake, Visual Studio C++ build tools, and internet access to fetch JUCE.

## Prototype limitations

- The current tuner control changes the displayed frequency and introduces a subtle playback-rate shift; it is a prototype rather than a broadcast-accurate RF model.
- Station files are loaded into memory, with a maximum of 16 stations and a 20-minute limit per file.
- The current state stores controls and selected station index, not embedded audio files; re-add audio after reopening a project if the host does not retain the plugin instance's loaded state.
- Test thoroughly before relying on it in a production session.
