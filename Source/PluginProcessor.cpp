#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr double beatsForRate[4] = { 0.5, 1.0, 2.0, 4.0 };   // 1/8, 1/4, 1/2, 1/1
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout KikoStartAudioProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { "mix", 1 }, "Mix",
        NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f,
        AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v * 100.0f)) + " %"; })
            .withValueFromStringFunction ([] (const String& s) { return s.getFloatValue() / 100.0f; })));

    layout.add (std::make_unique<AudioParameterChoice> (
        ParameterID { "rate", 1 }, "Rate", StringArray { "1/8", "1/4", "1/2", "1/1" }, 1));

    layout.add (std::make_unique<AudioParameterChoice> (
        ParameterID { "mode", 1 }, "Trigger", StringArray { "Sync", "MIDI", "Audio" }, 0));

    StringArray names;
    for (int i = 0; i < kg::numCurves; ++i)
        names.add (kg::curveNames[i]);
    layout.add (std::make_unique<AudioParameterChoice> (
        ParameterID { "curve", 1 }, "Curve", names, 3));

    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "band", 1 }, "Band", true));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { "xover", 1 }, "Crossover",
        NormalisableRange<float> (30.0f, 1000.0f, 0.0f, 0.35f), 150.0f,
        AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v)) + " Hz"; })));

    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "listen", 1 }, "Listen", false));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { "shift", 1 }, "Shift",
        NormalisableRange<float> (-0.5f, 0.5f, 0.0f), 0.0f));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { "length", 1 }, "Length",
        NormalisableRange<float> (0.05f, 1.0f, 0.0f), 0.8f));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { "thresh", 1 }, "Audio Threshold",
        NormalisableRange<float> (-60.0f, 0.0f, 0.1f), -24.0f,
        AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int) { return String (v, 1) + " dB"; })));

    return layout;
}

//==============================================================================
KikoStartAudioProcessor::KikoStartAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",     juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output",    juce::AudioChannelSet::stereo(), true)
                          .withInput  ("Sidechain", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    mixP    = apvts.getRawParameterValue ("mix");
    rateP   = apvts.getRawParameterValue ("rate");
    modeP   = apvts.getRawParameterValue ("mode");
    curveP  = apvts.getRawParameterValue ("curve");
    bandP   = apvts.getRawParameterValue ("band");
    xoverP  = apvts.getRawParameterValue ("xover");
    listenP = apvts.getRawParameterValue ("listen");
    shiftP  = apvts.getRawParameterValue ("shift");
    lengthP = apvts.getRawParameterValue ("length");
    threshP = apvts.getRawParameterValue ("thresh");

    for (auto& a : waveMin) a.store (0.0f);
    for (auto& a : waveMax) a.store (0.0f);
    for (auto& a : scMin)   a.store (0.0f);
    for (auto& a : scMax)   a.store (0.0f);

    kg::lut (0);   // build the curve tables now, not on the audio thread
}

bool KikoStartAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono())
        return false;
    if (layouts.getMainInputChannelSet() != out)
        return false;

    const auto& sc = layouts.getChannelSet (true, 1);
    return sc.isDisabled() || sc == juce::AudioChannelSet::mono() || sc == juce::AudioChannelSet::stereo();
}

void KikoStartAudioProcessor::prepareToPlay (double sr, int samplesPerBlock)
{
    sampleRate = sr;

    juce::dsp::ProcessSpec spec { sr, (juce::uint32) juce::jmax (1, samplesPerBlock), 2 };
    crossover.prepare (spec);
    crossover.setType (juce::dsp::LinkwitzRileyFilterType::lowpass);
    crossover.setCutoffFrequency (150.0f);
    crossover.reset();
    lastXover = 150.0f;

    constexpr double twoPi = 6.283185307179586;
    lpA     = (float) (1.0 - std::exp (-twoPi * 200.0 / sr));   // kick-detector low-pass ~200 Hz
    fastRel = (float) std::exp (-1.0 / (0.020 * sr));           // fast envelope: instant attack, 20 ms release
    slowAtk = (float) (1.0 - std::exp (-1.0 / (0.015 * sr)));   // slow envelope: 15 ms attack
    slowRel = (float) std::exp (-1.0 / (0.050 * sr));           //                50 ms release
    gainCoef = (float) (1.0 - std::exp (-1.0 / (0.0004 * sr))); // 0.4 ms anti-click smoothing

    curGain = 1.0f;
    lp1 = lp2 = fastEnv = slowEnv = 0.0f;
    scActive = false;
    samplesSinceTrig = 1.0e12;
    freePpq = 0.0;
    curBin = -1;
}

//==============================================================================
void KikoStartAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    auto mainBus = getBusBuffer (buffer, true, 0);
    auto scBus   = getBusBuffer (buffer, true, 1);

    const int numCh = juce::jmin (2, mainBus.getNumChannels());
    if (numCh == 0 || numSamples == 0)
        return;

    const int scCh = juce::jmin (2, scBus.getNumChannels());

    // ---- parameters -------------------------------------------------------
    const float mix     = mixP->load();
    const int   rateIdx = juce::jlimit (0, 3, (int) rateP->load());
    const int   mode    = juce::jlimit (0, 2, (int) modeP->load());
    const int   curve   = juce::jlimit (0, kg::numCurves - 1, (int) curveP->load());
    const bool  band    = bandP->load() > 0.5f;
    const bool  listen  = listenP->load() > 0.5f;
    const float shift   = shiftP->load();
    const float length  = juce::jmax (0.05f, lengthP->load());
    const float thrLin  = juce::Decibels::decibelsToGain (threshP->load());

    const float xover = xoverP->load();
    if (xover != lastXover)
    {
        crossover.setCutoffFrequency (xover);
        lastXover = xover;
    }

    // ---- host timing ------------------------------------------------------
    double bpm = 120.0, ppqStart = 0.0;
    bool havePpq = false, playing = true;

    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm())
                bpm = juce::jmax (20.0, *b);
            playing = pos->getIsPlaying();
            if (auto p = pos->getPpqPosition())
            {
                ppqStart = *p;
                havePpq = true;
            }
        }
    }

    const bool syncActive = havePpq ? playing : true;   // no playhead (standalone) -> free-run
    if (! havePpq)
        ppqStart = freePpq;

    const double beats = beatsForRate[rateIdx];
    const double ppqPerSample = bpm / (60.0 * sampleRate);
    const double samplesPerCycle = beats / ppqPerSample;
    const int minGap = (int) (0.025 * sampleRate);

    // ---- pointers ---------------------------------------------------------
    float* data[2] = { mainBus.getWritePointer (0), numCh > 1 ? mainBus.getWritePointer (1) : nullptr };
    const float* scData[2] = { scCh > 0 ? scBus.getReadPointer (0) : nullptr,
                               scCh > 1 ? scBus.getReadPointer (1) : nullptr };

    // The host may enable the sidechain bus but leave it unrouted (silence). Only trust it once
    // real signal has been seen; until then the trigger falls back to the main input.
    if (scCh > 0 && ! scActive)
    {
        for (int c = 0; c < scCh && ! scActive; ++c)
            for (int i = 0; i < numSamples; ++i)
                if (std::abs (scData[c][i]) > 1.0e-4f) { scActive = true; break; }
    }

    auto storeBin = [this]
    {
        waveMin[(size_t) curBin].store (curMin,   std::memory_order_relaxed);
        waveMax[(size_t) curBin].store (curMax,   std::memory_order_relaxed);
        scMin  [(size_t) curBin].store (curScMin, std::memory_order_relaxed);
        scMax  [(size_t) curBin].store (curScMax, std::memory_order_relaxed);
    };

    auto midiIt = midi.begin();
    float lastT = -1.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        // pre-duck mono signal (for the waveform view and fallback trigger)
        float mono = 0.0f;
        for (int c = 0; c < numCh; ++c)
            mono += data[c][i];
        mono /= (float) numCh;

        // -- MIDI trigger
        while (midiIt != midi.end() && (*midiIt).samplePosition <= i)
        {
            if (mode == 1 && (*midiIt).getMessage().isNoteOn())
                samplesSinceTrig = 0.0;
            ++midiIt;
        }

        // -- sidechain mono sample (also used for the waveform display)
        float scMono = 0.0f;
        if (scCh > 0)
        {
            for (int c = 0; c < scCh; ++c)
                scMono += scData[c][i];
            scMono /= (float) scCh;
        }

        // -- audio trigger: onset detection.
        //    A kick is detected when the fast envelope jumps well above the slow one, so it keeps
        //    working with rapid kick rolls, long tails or a sustained bass underneath.
        if (mode == 2)
        {
            const float s = (scCh > 0 && scActive) ? scMono : mono;   // no real sidechain -> main input

            lp1 += lpA * (s - lp1);
            lp2 += lpA * (lp1 - lp2);
            const float a = std::abs (lp2);

            fastEnv = a > fastEnv ? a : fastEnv * fastRel;

            if (fastEnv > slowEnv)
            {
                slowEnv += slowAtk * (fastEnv - slowEnv);
            }
            else
            {
                const float d = slowEnv * slowRel;
                slowEnv = d > fastEnv ? d : fastEnv;
            }

            if (fastEnv > thrLin && fastEnv > 1.7f * slowEnv && samplesSinceTrig > (double) minGap)
                samplesSinceTrig = 0.0;
        }

        // -- position inside the cycle -> target gain
        float g = 1.0f;
        float t = -1.0f;   // display position

        if (mode == 0)
        {
            if (syncActive)
            {
                double tt = (ppqStart + (double) i * ppqPerSample) / beats;
                tt -= std::floor (tt);
                double u = tt - (double) shift;
                u -= std::floor (u);
                g = kg::gainAt (curve, (float) u, length);
                t = (float) tt;
            }
        }
        else
        {
            const double tt = samplesSinceTrig / samplesPerCycle;
            const float u = (float) (juce::jmin (tt, 4.0) - (double) shift);
            g = kg::gainAt (curve, u, length);
            if (tt < 1.0)
                t = (float) tt;
            samplesSinceTrig += 1.0;
        }

        // -- waveform capture for the editor
        if (t >= 0.0f)
        {
            const int bin = juce::jlimit (0, numWaveBins - 1, (int) (t * (float) numWaveBins));
            if (bin != curBin)
            {
                if (curBin >= 0)
                    storeBin();
                curBin = bin;
                curMin = curMax = mono;
                curScMin = curScMax = scMono;
            }
            else
            {
                curMin   = juce::jmin (curMin,   mono);
                curMax   = juce::jmax (curMax,   mono);
                curScMin = juce::jmin (curScMin, scMono);
                curScMax = juce::jmax (curScMax, scMono);
            }
        }
        else if (curBin >= 0)
        {
            storeBin();
            curBin = -1;
        }
        lastT = t;

        // -- apply ducking
        curGain += gainCoef * (g - curGain);
        const float gm = 1.0f - mix * (1.0f - curGain);

        for (int c = 0; c < numCh; ++c)
        {
            const float x = data[c][i];

            if (band)
            {
                float lo = 0.0f, hi = 0.0f;
                crossover.processSample (c, x, lo, hi);
                lo *= gm;
                data[c][i] = listen ? lo : (lo + hi);
            }
            else
            {
                data[c][i] = x * gm;
            }
        }
    }

    playPhase.store (lastT, std::memory_order_relaxed);

    if (! havePpq)
        freePpq = std::fmod (freePpq + (double) numSamples * ppqPerSample, 64.0);
}

//==============================================================================
juce::AudioProcessorEditor* KikoStartAudioProcessor::createEditor()
{
    return new KikoStartAudioProcessorEditor (*this);
}

void KikoStartAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void KikoStartAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new KikoStartAudioProcessor();
}
