#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "Curves.h"

class KikoStartAudioProcessor : public juce::AudioProcessor
{
public:
    static constexpr int numWaveBins = 1024;

    KikoStartAudioProcessor();
    ~KikoStartAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "KikoStart GRO"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    juce::AudioProcessorValueTreeState apvts;

    // Shared with the editor (lock-free)
    std::array<std::atomic<float>, numWaveBins> waveMin, waveMax;   // entrada principal
    std::array<std::atomic<float>, numWaveBins> scMin, scMax;       // entrada sidechain
    std::atomic<float> playPhase { -1.0f };   // 0..1 inside the cycle, <0 = idle

private:
    std::atomic<float>* mixP = nullptr;
    std::atomic<float>* rateP = nullptr;
    std::atomic<float>* modeP = nullptr;
    std::atomic<float>* curveP = nullptr;
    std::atomic<float>* bandP = nullptr;
    std::atomic<float>* xoverP = nullptr;
    std::atomic<float>* listenP = nullptr;
    std::atomic<float>* shiftP = nullptr;
    std::atomic<float>* lengthP = nullptr;
    std::atomic<float>* threshP = nullptr;

    juce::dsp::LinkwitzRileyFilter<float> crossover;
    float lastXover = 150.0f;

    double sampleRate = 44100.0;
    double freePpq = 0.0;
    double samplesSinceTrig = 1.0e12;

    // audio-trigger detector (onset detection: fast envelope vs slow envelope)
    float lp1 = 0.0f, lp2 = 0.0f;
    float fastEnv = 0.0f, slowEnv = 0.0f;
    float lpA = 0.0f, fastRel = 0.0f, slowAtk = 0.0f, slowRel = 0.0f;
    bool scActive = false;   // true once the host has really sent signal on the sidechain

    // gain smoothing
    float curGain = 1.0f, gainCoef = 0.01f;

    // waveform capture
    int curBin = -1;
    float curMin = 0.0f, curMax = 0.0f, curScMin = 0.0f, curScMax = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KikoStartAudioProcessor)
};
