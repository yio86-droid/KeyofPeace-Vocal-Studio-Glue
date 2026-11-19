#include "PluginProcessor.h"
#include "PluginEditor.h"

KeyofPeaceVocalStudioGlueAudioProcessor::KeyofPeaceVocalStudioGlueAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

KeyofPeaceVocalStudioGlueAudioProcessor::~KeyofPeaceVocalStudioGlueAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout KeyofPeaceVocalStudioGlueAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> ("demud",   "De-Mud",      0.0f, 100.0f, 45.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("comp",    "Opto-Comp",   0.0f, 100.0f, 40.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("air",     "Suno Air",    0.0f, 100.0f, 35.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("deess",   "De-Esser",    0.0f, 100.0f, 30.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("glue",    "Analog Glue", 0.0f, 100.0f, 25.0f));

    return { params.begin(), params.end() };
}

void KeyofPeaceVocalStudioGlueAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32> (getTotalNumOutputChannels());

    hpfFilter.prepare (spec);
    mudFilter.prepare (spec);
    presenceFilter.prepare (spec);
    airFilter.prepare (spec);
    deEssDetectFilter.prepare (spec);

    optoEnvelope = 0.0f;
    deEssEnvelope = 0.0f;

    updateDSP();
}

void KeyofPeaceVocalStudioGlueAudioProcessor::releaseResources() {}

bool KeyofPeaceVocalStudioGlueAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono()
        || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void KeyofPeaceVocalStudioGlueAudioProcessor::updateDSP()
{
    float deMudNorm = apvts.getRawParameterValue ("demud")->load() / 100.0f;
    float airNorm   = apvts.getRawParameterValue ("air")->load()   / 100.0f;

    // 1. High-Pass (80 Hz -> 160 Hz) + De-Mud (300 Hz hasta -4.5 dB)
    float hpfFreq = 80.0f + deMudNorm * 80.0f;
    *hpfFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass (currentSampleRate, hpfFreq, 0.707f);

    float mudGainDb = -deMudNorm * 4.5f;
    *mudFilter.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter (
        currentSampleRate, 300.0f, 1.2f, juce::Decibels::decibelsToGain (mudGainDb));

    // 3. Suno Air Match: Presencia (3.5 kHz, +3 dB) + Aire (10 kHz, +4.5 dB)
    float presGainDb = airNorm * 3.0f;
    *presenceFilter.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter (
        currentSampleRate, 3500.0f, 1.1f, juce::Decibels::decibelsToGain (presGainDb));

    float airGainDb = airNorm * 4.5f;
    *airFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf (
        currentSampleRate, 10000.0f, 0.707f, juce::Decibels::decibelsToGain (airGainDb));

    // 4. Filtro pasa-banda detector de sibilancia (6.5 kHz)
    *deEssDetectFilter.state = *juce::dsp::IIR::Coefficients<float>::makeBandPass (
        currentSampleRate, 6500.0f, 2.0f);
}

void KeyofPeaceVocalStudioGlueAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    updateDSP();

    float compNorm  = apvts.getRawParameterValue ("comp")->load()  / 100.0f;
    float deEssNorm = apvts.getRawParameterValue ("deess")->load() / 100.0f;
    float glueNorm  = apvts.getRawParameterValue ("glue")->load()  / 100.0f;

    juce::AudioBuffer<float> sibilanceBuffer;
    sibilanceBuffer.makeCopyOf (buffer);

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);

    hpfFilter.process (context);
    mudFilter.process (context);
    presenceFilter.process (context);
    airFilter.process (context);

    juce::dsp::AudioBlock<float> sibBlock (sibilanceBuffer);
    juce::dsp::ProcessContextReplacing<float> sibContext (sibBlock);
    deEssDetectFilter.process (sibContext);

    float optoAtt = std::exp (-1.0f / (0.012f * static_cast<float> (currentSampleRate)));
    float optoRel = std::exp (-1.0f / (0.140f * static_cast<float> (currentSampleRate)));
    float deEssAtt = std::exp (-1.0f / (0.002f * static_cast<float> (currentSampleRate)));
    float deEssRel = std::exp (-1.0f / (0.045f * static_cast<float> (currentSampleRate)));

    float compThreshold = juce::Decibels::decibelsToGain (-24.0f + (1.0f - compNorm) * 18.0f);
    float compRatio = 1.0f + compNorm * 3.5f;
    float autoMakeup = juce::Decibels::decibelsToGain (compNorm * 4.5f);

    float deEssThreshold = juce::Decibels::decibelsToGain (-28.0f + (1.0f - deEssNorm) * 20.0f);
    float drive = 1.0f + glueNorm * 2.2f;
    float driveNorm = 1.0f / std::tanh (drive);

    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float peakIn = 0.0f;
        float peakSib = 0.0f;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            peakIn  = std::max (peakIn,  std::abs (buffer.getSample (ch, sample)));
            peakSib = std::max (peakSib, std::abs (sibilanceBuffer.getSample (ch, sample)));
        }

        if (peakIn > optoEnvelope)
            optoEnvelope = optoAtt * optoEnvelope + (1.0f - optoAtt) * peakIn;
        else
            optoEnvelope = optoRel * optoEnvelope + (1.0f - optoRel) * peakIn;

        float compGain = 1.0f;
        if (compNorm > 0.01f && optoEnvelope > compThreshold)
        {
            float overDb = juce::Decibels::gainToDecibels (optoEnvelope / compThreshold);
            float reducedDb = overDb * (1.0f - (1.0f / compRatio));
            compGain = juce::Decibels::decibelsToGain (-reducedDb);
        }

        if (peakSib > deEssEnvelope)
            deEssEnvelope = deEssAtt * deEssEnvelope + (1.0f - deEssAtt) * peakSib;
        else
            deEssEnvelope = deEssRel * deEssEnvelope + (1.0f - deEssRel) * peakSib;

        float deEssGain = 1.0f;
        if (deEssNorm > 0.01f && deEssEnvelope > deEssThreshold)
        {
            float sibOverDb = juce::Decibels::gainToDecibels (deEssEnvelope / deEssThreshold);
            float sibCutDb = std::min (sibOverDb * deEssNorm * 0.75f, 9.0f);
            deEssGain = juce::Decibels::decibelsToGain (-sibCutDb);
        }

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float x = buffer.getSample (ch, sample);
            x *= (compGain * autoMakeup * deEssGain);

            if (glueNorm > 0.01f)
                x = std::tanh (x * drive) * driveNorm;

            x = juce::jlimit (-0.891f, 0.891f, x);
            buffer.setSample (ch, sample, x);
        }
    }
}

juce::AudioProcessorEditor* KeyofPeaceVocalStudioGlueAudioProcessor::createEditor()
{
    return new KeyofPeaceVocalStudioGlueAudioProcessorEditor (*this);
}

void KeyofPeaceVocalStudioGlueAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void KeyofPeaceVocalStudioGlueAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new KeyofPeaceVocalStudioGlueAudioProcessor();
}
