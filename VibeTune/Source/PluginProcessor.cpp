#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
juce::StringArray VibeTuneAudioProcessor::noteNames()
{
    return { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
}

juce::StringArray VibeTuneAudioProcessor::scaleNames()
{
    return { "Chromatic", "Major", "Minor", "Harmonic Minor", "Pentatonic Minor",
             "Pentatonic Major", "Dorian", "Phrygian", "Blues" };
}

std::array<bool, 12> VibeTuneAudioProcessor::buildAllowed (int key, int scale)
{
    static const std::vector<std::vector<int>> intervals =
    {
        { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 }, // Chromatic
        { 0, 2, 4, 5, 7, 9, 11 },                 // Major
        { 0, 2, 3, 5, 7, 8, 10 },                 // Minor
        { 0, 2, 3, 5, 7, 8, 11 },                 // Harmonic minor
        { 0, 3, 5, 7, 10 },                       // Pentatonic minor
        { 0, 2, 4, 7, 9 },                        // Pentatonic major
        { 0, 2, 3, 5, 7, 9, 10 },                 // Dorian
        { 0, 1, 3, 5, 7, 8, 10 },                 // Phrygian
        { 0, 3, 5, 6, 7, 10 }                     // Blues
    };

    std::array<bool, 12> mask {};
    const auto& iv = intervals[(size_t) juce::jlimit (0, (int) intervals.size() - 1, scale)];
    for (int i : iv)
        mask[(size_t) ((key + i) % 12)] = true;
    return mask;
}

juce::AudioProcessorValueTreeState::ParameterLayout VibeTuneAudioProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "key", 1 }, "Key", noteNames(), 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "scale", 1 }, "Scale", scaleNames(), 2));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { "speed", 1 }, "Retune Speed",
        NormalisableRange<float> (0.0f, 400.0f, 0.1f, 0.4f), 20.0f,
        AudioParameterFloatAttributes().withLabel ("ms")));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { "amount", 1 }, "Amount",
        NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f,
        AudioParameterFloatAttributes().withLabel ("%")));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { "humanize", 1 }, "Humanize",
        NormalisableRange<float> (0.0f, 100.0f, 0.1f), 10.0f,
        AudioParameterFloatAttributes().withLabel ("%")));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { "transpose", 1 }, "Transpose",
        NormalisableRange<float> (-12.0f, 12.0f, 1.0f), 0.0f,
        AudioParameterFloatAttributes().withLabel ("st")));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { "fine", 1 }, "Fine Tune",
        NormalisableRange<float> (-50.0f, 50.0f, 1.0f), 0.0f,
        AudioParameterFloatAttributes().withLabel ("ct")));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { "mix", 1 }, "Mix",
        NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f,
        AudioParameterFloatAttributes().withLabel ("%")));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { "gain", 1 }, "Output",
        NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f,
        AudioParameterFloatAttributes().withLabel ("dB")));

    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "lowlat", 1 }, "Low Latency", false));

    return layout;
}

//==============================================================================
VibeTuneAudioProcessor::VibeTuneAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "VIBETUNE", createLayout())
{
    pKey       = apvts.getRawParameterValue ("key");
    pScale     = apvts.getRawParameterValue ("scale");
    pSpeed     = apvts.getRawParameterValue ("speed");
    pAmount    = apvts.getRawParameterValue ("amount");
    pHumanize  = apvts.getRawParameterValue ("humanize");
    pTranspose = apvts.getRawParameterValue ("transpose");
    pFine      = apvts.getRawParameterValue ("fine");
    pMix       = apvts.getRawParameterValue ("mix");
    pGain      = apvts.getRawParameterValue ("gain");
    pLowLat    = apvts.getRawParameterValue ("lowlat");

    apvts.addParameterListener ("lowlat", this);
}

VibeTuneAudioProcessor::~VibeTuneAudioProcessor()
{
    apvts.removeParameterListener ("lowlat", this);
    cancelPendingUpdate();
}

//==============================================================================
void VibeTuneAudioProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = sampleRate;
    engine.prepare (sampleRate);

    engineLowLatency = pLowLat->load() > 0.5f;
    engine.setLowLatency (engineLowLatency);
    setLatencySamples (engine.getLatencySamples());
}

void VibeTuneAudioProcessor::parameterChanged (const juce::String& parameterID, float)
{
    if (parameterID == "lowlat")
        triggerAsyncUpdate(); // tell the host the new latency from the message thread
}

void VibeTuneAudioProcessor::handleAsyncUpdate()
{
    const bool ll = pLowLat->load() > 0.5f;
    setLatencySamples (vt::AutoTuneEngine::latencyFor (currentSampleRate, ll));
    updateHostDisplay (juce::AudioProcessorListener::ChangeDetails().withLatencyChanged (true));
}

bool VibeTuneAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return out == layouts.getMainInputChannelSet();
}

void VibeTuneAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const bool ll = pLowLat->load() > 0.5f;
    if (ll != engineLowLatency)
    {
        engine.setLowLatency (ll);
        engineLowLatency = ll;
    }

    vt::TuneParams tp;
    tp.allowed        = buildAllowed (juce::roundToInt (pKey->load()), juce::roundToInt (pScale->load()));
    tp.speedMs        = pSpeed->load();
    tp.amount         = pAmount->load() * 0.01f;
    tp.humanizeCents  = pHumanize->load() * 0.4f;                 // 0..100 % -> 0..40 cents dead-zone
    tp.transposeSemis = pTranspose->load() + pFine->load() * 0.01f;
    tp.mix            = pMix->load() * 0.01f;
    tp.outGain        = juce::Decibels::decibelsToGain (pGain->load());

    const int numCh = juce::jmin (buffer.getNumChannels(), 2);
    engine.process (buffer.getArrayOfWritePointers(), numCh, buffer.getNumSamples(), tp);
}

//==============================================================================
void VibeTuneAudioProcessor::setCurrentProgram (int index)
{
    index = juce::jlimit (0, vt::kNumPresets - 1, index);
    currentPreset = index;
    const auto& p = vt::kPresets[index];

    auto set = [this] (const char* id, float value)
    {
        if (auto* param = apvts.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
    };

    set ("scale",     (float) p.scale);
    set ("speed",     p.speedMs);
    set ("amount",    p.amountPct);
    set ("humanize",  p.humanizePct);
    set ("transpose", p.transposeSt);
    set ("fine",      p.fineCents);
    set ("mix",       p.mixPct);
    set ("gain",      p.gainDb);
}

const juce::String VibeTuneAudioProcessor::getProgramName (int index)
{
    if (index < 0 || index >= vt::kNumPresets)
        return {};
    return vt::kPresets[index].name;
}

//==============================================================================
juce::AudioProcessorEditor* VibeTuneAudioProcessor::createEditor()
{
    return new VibeTuneAudioProcessorEditor (*this);
}

void VibeTuneAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("preset", currentPreset, nullptr);
    state.setProperty ("editorWidth", editorWidth, nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void VibeTuneAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            currentPreset = juce::jlimit (0, vt::kNumPresets - 1, (int) tree.getProperty ("preset", 0));
            editorWidth   = juce::jlimit (630, 1800, (int) tree.getProperty ("editorWidth", 900));
            apvts.replaceState (tree);
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VibeTuneAudioProcessor();
}
