#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "SarangiEngine.h"

class SarangiProcessor : public juce::AudioProcessor
{
public:
    SarangiProcessor();
    ~SarangiProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Sarangi"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::AudioProcessorValueTreeState apvts;

    // Read by the editor's animation timer.
    std::atomic<float> uiLevel { 0.0f };
    std::atomic<float> uiHz { 0.0f };
    std::atomic<bool>  uiBowing { false };

private:
    sarangi::Engine engine;
    std::atomic<float>* pBow   = nullptr;
    std::atomic<float>* pMeend = nullptr;
    std::atomic<float>* pGamak = nullptr;
    std::atomic<float>* pTaraf = nullptr;
    std::atomic<float>* pSpace = nullptr;
    std::atomic<float>* pVol   = nullptr;
    std::atomic<float>* pSa    = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SarangiProcessor)
};
