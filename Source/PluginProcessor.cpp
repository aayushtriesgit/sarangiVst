#include "PluginProcessor.h"
#include "PluginEditor.h"

SarangiProcessor::SarangiProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "SARANGI", createLayout())
{
    pBow   = apvts.getRawParameterValue ("bow");
    pMeend = apvts.getRawParameterValue ("meend");
    pGamak = apvts.getRawParameterValue ("gamak");
    pTaraf = apvts.getRawParameterValue ("taraf");
    pSpace = apvts.getRawParameterValue ("space");
    pVol   = apvts.getRawParameterValue ("volume");
    pSa    = apvts.getRawParameterValue ("sa");
}

juce::AudioProcessorValueTreeState::ParameterLayout SarangiProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;
    auto pct = AudioParameterFloatAttributes().withStringFromValueFunction (
        [] (float v, int) { return String (juce::roundToInt (v * 100.0f)) + "%"; });

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "bow", 1 },    "Bow",    NormalisableRange<float> (0.0f, 1.0f), 0.55f, pct));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "meend", 1 },  "Meend",  NormalisableRange<float> (0.0f, 1.0f), 0.45f, pct));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "gamak", 1 },  "Gamak",  NormalisableRange<float> (0.0f, 1.0f), 0.35f, pct));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "taraf", 1 },  "Taraf",  NormalisableRange<float> (0.0f, 1.0f), 0.50f, pct));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "space", 1 },  "Space",  NormalisableRange<float> (0.0f, 1.0f), 0.30f, pct));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "volume", 1 }, "Volume", NormalisableRange<float> (0.0f, 1.0f), 0.75f, pct));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "sa", 1 }, "Sa",
        StringArray { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 0));
    return layout;
}

void SarangiProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate);
}

bool SarangiProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void SarangiProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    sarangi::Params p;
    p.bow    = pBow->load();
    p.meend  = pMeend->load();
    p.gamak  = pGamak->load();
    p.taraf  = pTaraf->load();
    p.space  = pSpace->load();
    p.volume = pVol->load();
    p.sa     = (int) pSa->load();
    engine.setParams (p);

    const int numSamples = buffer.getNumSamples();
    juce::AudioBuffer<float> tmp; // only used for mono output
    float* left  = buffer.getWritePointer (0);
    float* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
    if (right == nullptr)
    {
        tmp.setSize (1, numSamples, false, false, true);
        right = tmp.getWritePointer (0);
    }

    int pos = 0;
    for (const auto meta : midi)
    {
        const int when = juce::jlimit (0, numSamples, meta.samplePosition);
        if (when > pos) { engine.render (left + pos, right + pos, when - pos); pos = when; }

        const auto m = meta.getMessage();
        if (m.isNoteOn())                         engine.noteOn (m.getNoteNumber(), m.getFloatVelocity());
        else if (m.isNoteOff())                   engine.noteOff (m.getNoteNumber());
        else if (m.isAllNotesOff() || m.isAllSoundOff()) engine.allNotesOff();
    }
    if (pos < numSamples) engine.render (left + pos, right + pos, numSamples - pos);

    if (buffer.getNumChannels() == 1)
        for (int i = 0; i < numSamples; ++i) left[i] = 0.5f * (left[i] + right[i]);

    uiLevel.store (engine.getLevel());
    uiHz.store (engine.getCurrentHz());
    uiBowing.store (engine.isBowing());
}

juce::AudioProcessorEditor* SarangiProcessor::createEditor()
{
    return new SarangiEditor (*this);
}

void SarangiProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void SarangiProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SarangiProcessor();
}
