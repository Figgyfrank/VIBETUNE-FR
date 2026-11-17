#pragma once

#include <JuceHeader.h>
#include "DSP/AutoTuneEngine.h"
#include "Presets.h"

class VibeTuneAudioProcessor : public juce::AudioProcessor,
                               private juce::AudioProcessorValueTreeState::Listener,
                               private juce::AsyncUpdater
{
public:
    VibeTuneAudioProcessor();
    ~VibeTuneAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    // Factory presets show up in Ableton's plugin preset menu too
    int getNumPrograms() override { return vt::kNumPresets; }
    int getCurrentProgram() override { return currentPreset; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static std::array<bool, 12> buildAllowed (int key, int scale);
    static juce::StringArray noteNames();
    static juce::StringArray scaleNames();

    juce::AudioProcessorValueTreeState apvts;

    // live meters for the UI
    float getDetectedMidi() const { return engine.getDetectedMidi(); }
    float getTargetMidi() const { return engine.getTargetMidi(); }
    int editorWidth = 900; // remembered between sessions

private:
    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void handleAsyncUpdate() override;

    vt::AutoTuneEngine engine;
    bool engineLowLatency = false;
    double currentSampleRate = 48000.0;
    int currentPreset = 0;

    std::atomic<float> *pKey, *pScale, *pSpeed, *pAmount, *pHumanize,
                       *pTranspose, *pFine, *pMix, *pGain, *pLowLat;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VibeTuneAudioProcessor)
};
