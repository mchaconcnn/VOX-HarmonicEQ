#pragma once

#include <JuceHeader.h>
#include "HarmonicFilterBank.h"
#include "PitchDetector.h"
#include "WaveformMonitor.h"

#include <array>
#include <atomic>

class HarmonicEQAudioProcessor final : public juce::AudioProcessor
{
public:
    HarmonicEQAudioProcessor();
    ~HarmonicEQAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    static juce::String bandParameterID(int bandIndex);
    static juce::String bandDisplayName(int bandIndex);
    static double midiNoteToFrequency(double midiNote, double cents = 0.0) noexcept;
    void copyWaveformSnapshot(std::array<float, WaveformMonitor::capacity>& destination) const noexcept;

    juce::AudioProcessorValueTreeState parameters;
    std::atomic<double> displayedFrequencyHz { 0.0 };
    std::atomic<float> displayedConfidence { 0.0f };
    std::atomic<bool> displayedVoiced { false };
    std::atomic<double> activeFundamentalHz { 220.0 };

private:
    PitchDetector pitchDetector;
    HarmonicFilterBank filterBank;
    WaveformMonitor waveformMonitor;
    juce::AudioBuffer<float> dryBuffer;
    double currentSampleRate = 48000.0;
    double smoothedFundamental = 220.0;
    double lastDetectedFundamental = 220.0;
    int samplesSinceValidPitch = 0;
    bool detectorHasValidPitch = false;

    std::atomic<float>* pitchSource = nullptr;
    std::atomic<float>* manualNote = nullptr;
    std::atomic<float>* manualCents = nullptr;
    std::atomic<float>* pitchGlideMs = nullptr;
    std::atomic<float>* subharmonicCount = nullptr;
    std::atomic<float>* upperHarmonicCount = nullptr;
    std::atomic<float>* filterQ = nullptr;
    std::atomic<float>* wetDry = nullptr;
    std::atomic<float>* outputGainDb = nullptr;
    std::atomic<float>* bypass = nullptr;
    std::array<std::atomic<float>*, HarmonicFilterBank::totalBands> bandGainParameters {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HarmonicEQAudioProcessor)
};
