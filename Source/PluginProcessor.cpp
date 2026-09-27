#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>

namespace
{
constexpr auto stateTreeName = "HarmonicEQState";
}

HarmonicEQAudioProcessor::HarmonicEQAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, stateTreeName, createParameterLayout())
{
    pitchSource = parameters.getRawParameterValue("pitchSource");
    manualNote = parameters.getRawParameterValue("manualNote");
    manualCents = parameters.getRawParameterValue("manualCents");
    pitchGlideMs = parameters.getRawParameterValue("pitchGlideMs");
    subharmonicCount = parameters.getRawParameterValue("subharmonicCount");
    upperHarmonicCount = parameters.getRawParameterValue("upperHarmonicCount");
    filterQ = parameters.getRawParameterValue("filterQ");
    wetDry = parameters.getRawParameterValue("wetDry");
    outputGainDb = parameters.getRawParameterValue("outputGainDb");
    bypass = parameters.getRawParameterValue("bypass");

    for (int band = 0; band < HarmonicFilterBank::totalBands; ++band)
        bandGainParameters[static_cast<size_t>(band)] = parameters.getRawParameterValue(bandParameterID(band));
}

juce::AudioProcessorValueTreeState::ParameterLayout HarmonicEQAudioProcessor::createParameterLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> layout;

    layout.push_back(std::make_unique<AudioParameterChoice>(
        ParameterID { "pitchSource", 1 }, "Pitch Source", StringArray { "Detected", "Manual" }, 0));
    layout.push_back(std::make_unique<AudioParameterInt>(
        ParameterID { "manualNote", 1 }, "Manual Note", 0, 127, 57,
        AudioParameterIntAttributes()
            .withStringFromValueFunction([](int note, int)
            {
                static const StringArray names { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
                note = jlimit(0, 127, note);
                return names[note % 12] + String(note / 12 - 1);
            })
            .withValueFromStringFunction([](const String& original)
            {
                const auto text = original.trim().toUpperCase();
                if (text.isEmpty())
                    return 57;
                if (text.containsOnly("-0123456789"))
                    return jlimit(0, 127, text.getIntValue());

                const String noteLetter = text.substring(0, 1);
                int pitchClass = StringArray { "C", "D", "E", "F", "G", "A", "B" }.indexOf(noteLetter);
                static constexpr int naturalPitchClasses[] { 0, 2, 4, 5, 7, 9, 11 };
                if (pitchClass < 0)
                    return 57;
                pitchClass = naturalPitchClasses[pitchClass];

                int octaveStart = 1;
                if (text.length() > 1 && (text[1] == '#' || text[1] == 'B'))
                {
                    pitchClass += text[1] == '#' ? 1 : -1;
                    octaveStart = 2;
                }
                const auto octave = text.substring(octaveStart).getIntValue();
                return jlimit(0, 127, (octave + 1) * 12 + pitchClass);
            })));
    layout.push_back(std::make_unique<AudioParameterFloat>(
        ParameterID { "manualCents", 1 }, "Manual Fine Tune", NormalisableRange<float> { -100.0f, 100.0f, 0.1f }, 0.0f,
        AudioParameterFloatAttributes().withLabel("cents")));
    layout.push_back(std::make_unique<AudioParameterFloat>(
        ParameterID { "pitchGlideMs", 1 }, "Pitch Glide", NormalisableRange<float> { 0.0f, 500.0f, 1.0f, 0.5f }, 35.0f,
        AudioParameterFloatAttributes().withLabel("ms")));
    layout.push_back(std::make_unique<AudioParameterInt>(
        ParameterID { "subharmonicCount", 1 }, "Subharmonics Below", 0, HarmonicFilterBank::maxSubharmonics, 0));
    layout.push_back(std::make_unique<AudioParameterInt>(
        ParameterID { "upperHarmonicCount", 1 }, "Harmonics Above", 0, HarmonicFilterBank::maxUpperHarmonics, 8));
    layout.push_back(std::make_unique<AudioParameterFloat>(
        ParameterID { "filterQ", 1 }, "Filter Q", NormalisableRange<float> { 0.5f, 30.0f, 0.1f, 0.35f }, 10.0f,
        AudioParameterFloatAttributes {}));
    layout.push_back(std::make_unique<AudioParameterFloat>(
        ParameterID { "wetDry", 1 }, "Wet Dry", NormalisableRange<float> { 0.0f, 100.0f, 0.1f }, 100.0f,
        AudioParameterFloatAttributes().withLabel("%")));
    layout.push_back(std::make_unique<AudioParameterFloat>(
        ParameterID { "outputGainDb", 1 }, "Output Gain", NormalisableRange<float> { -24.0f, 12.0f, 0.1f }, 0.0f,
        AudioParameterFloatAttributes().withLabel("dB")));
    layout.push_back(std::make_unique<AudioParameterBool>(
        ParameterID { "bypass", 1 }, "Bypass", false));

    for (int band = 0; band < HarmonicFilterBank::totalBands; ++band)
    {
        layout.push_back(std::make_unique<AudioParameterFloat>(
            ParameterID { bandParameterID(band), 1 }, bandDisplayName(band),
            NormalisableRange<float> { -12.0f, 12.0f, 0.1f }, 0.0f,
            AudioParameterFloatAttributes().withLabel("dB")));
    }

    return { layout.begin(), layout.end() };
}

juce::String HarmonicEQAudioProcessor::bandParameterID(int bandIndex)
{
    if (bandIndex < HarmonicFilterBank::maxSubharmonics)
        return "subGain" + juce::String(bandIndex + 2);
    if (bandIndex == HarmonicFilterBank::maxSubharmonics)
        return "fundamentalGain";
    return "harmonicGain" + juce::String(bandIndex - HarmonicFilterBank::maxSubharmonics + 1);
}

juce::String HarmonicEQAudioProcessor::bandDisplayName(int bandIndex)
{
    if (bandIndex < HarmonicFilterBank::maxSubharmonics)
        return "Subharmonic /" + juce::String(bandIndex + 2);
    if (bandIndex == HarmonicFilterBank::maxSubharmonics)
        return "Fundamental";
    return "Harmonic x" + juce::String(bandIndex - HarmonicFilterBank::maxSubharmonics + 1);
}

double HarmonicEQAudioProcessor::midiNoteToFrequency(double note, double cents) noexcept
{
    return 440.0 * std::pow(2.0, (note - 69.0 + cents / 100.0) / 12.0);
}

void HarmonicEQAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    pitchDetector.prepare(sampleRate);
    filterBank.prepare(sampleRate, getTotalNumOutputChannels());
    dryBuffer.setSize(std::max(1, getTotalNumOutputChannels()), std::max(8192, samplesPerBlock), false, false, true);
    smoothedFundamental = midiNoteToFrequency(manualNote->load(), manualCents->load());
    lastDetectedFundamental = smoothedFundamental;
    samplesSinceValidPitch = 0;
    detectorHasValidPitch = false;
    waveformMonitor.reset();
}

void HarmonicEQAudioProcessor::releaseResources()
{
}

bool HarmonicEQAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto input = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();
    return input == output && (input == juce::AudioChannelSet::mono() || input == juce::AudioChannelSet::stereo());
}

void HarmonicEQAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const auto channels = buffer.getNumChannels();
    const auto samples = buffer.getNumSamples();
    if (channels <= 0 || samples <= 0)
        return;

    jassert(dryBuffer.getNumChannels() >= channels && dryBuffer.getNumSamples() >= samples);
    if (dryBuffer.getNumChannels() < channels || dryBuffer.getNumSamples() < samples)
        return;
    for (int channel = 0; channel < channels; ++channel)
        dryBuffer.copyFrom(channel, 0, buffer, channel, 0, samples);

    std::optional<PitchDetector::Estimate> newestEstimate;
    for (int sample = 0; sample < samples; ++sample)
    {
        float mono = 0.0f;
        for (int channel = 0; channel < channels; ++channel)
            mono += buffer.getSample(channel, sample);
        mono /= static_cast<float>(channels);

        if (auto estimate = pitchDetector.pushSample(mono))
            newestEstimate = estimate;
    }

    if (newestEstimate.has_value())
    {
        displayedFrequencyHz.store(newestEstimate->frequencyHz, std::memory_order_relaxed);
        displayedConfidence.store(newestEstimate->confidence, std::memory_order_relaxed);
        displayedVoiced.store(newestEstimate->voiced, std::memory_order_relaxed);

        if (newestEstimate->voiced && newestEstimate->confidence >= 0.60f)
        {
            lastDetectedFundamental = newestEstimate->frequencyHz;
            samplesSinceValidPitch = 0;
            detectorHasValidPitch = true;
        }
    }

    if (!newestEstimate.has_value() || !newestEstimate->voiced || newestEstimate->confidence < 0.60f)
    {
        samplesSinceValidPitch += samples;
        if (samplesSinceValidPitch > static_cast<int>(currentSampleRate * 0.20))
            detectorHasValidPitch = false;
    }

    const bool useManualPitch = pitchSource->load(std::memory_order_relaxed) >= 0.5f;
    const auto targetFundamental = useManualPitch
        ? midiNoteToFrequency(manualNote->load(std::memory_order_relaxed), manualCents->load(std::memory_order_relaxed))
        : lastDetectedFundamental;
    const bool pitchValid = useManualPitch || detectorHasValidPitch;
    const auto glideSeconds = static_cast<double>(pitchGlideMs->load(std::memory_order_relaxed)) / 1000.0;
    if (glideSeconds <= 0.0)
        smoothedFundamental = targetFundamental;
    else
    {
        const auto amount = 1.0 - std::exp(-static_cast<double>(samples) / (currentSampleRate * glideSeconds));
        smoothedFundamental += (targetFundamental - smoothedFundamental) * amount;
    }
    activeFundamentalHz.store(smoothedFundamental, std::memory_order_relaxed);

    std::array<float, HarmonicFilterBank::totalBands> gains {};
    for (int band = 0; band < HarmonicFilterBank::totalBands; ++band)
        gains[static_cast<size_t>(band)] = bandGainParameters[static_cast<size_t>(band)]->load(std::memory_order_relaxed);

    filterBank.process(buffer,
                       smoothedFundamental,
                       pitchValid,
                       static_cast<int>(std::lround(subharmonicCount->load(std::memory_order_relaxed))),
                       static_cast<int>(std::lround(upperHarmonicCount->load(std::memory_order_relaxed))),
                       filterQ->load(std::memory_order_relaxed),
                       gains);

    const auto mix = juce::jlimit(0.0f, 1.0f, wetDry->load(std::memory_order_relaxed) / 100.0f);
    const auto outputGain = juce::Decibels::decibelsToGain(outputGainDb->load(std::memory_order_relaxed));
    const bool isBypassed = bypass->load(std::memory_order_relaxed) >= 0.5f;

    for (int channel = 0; channel < channels; ++channel)
    {
        auto* wet = buffer.getWritePointer(channel);
        const auto* dry = dryBuffer.getReadPointer(channel);
        for (int sample = 0; sample < samples; ++sample)
            wet[sample] = (isBypassed ? dry[sample] : dry[sample] + (wet[sample] - dry[sample]) * mix) * outputGain;
    }

    for (int sample = 0; sample < samples; ++sample)
    {
        float outputMono = 0.0f;
        for (int channel = 0; channel < channels; ++channel)
            outputMono += buffer.getSample(channel, sample);
        waveformMonitor.pushSample(outputMono / static_cast<float>(channels));
    }
}

void HarmonicEQAudioProcessor::copyWaveformSnapshot(
    std::array<float, WaveformMonitor::capacity>& destination) const noexcept
{
    waveformMonitor.copySnapshot(destination);
}

juce::AudioProcessorEditor* HarmonicEQAudioProcessor::createEditor()
{
    return new HarmonicEQAudioProcessorEditor(*this);
}

void HarmonicEQAudioProcessor::getStateInformation(juce::MemoryBlock& destination)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary(*xml, destination);
}

void HarmonicEQAudioProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HarmonicEQAudioProcessor();
}
