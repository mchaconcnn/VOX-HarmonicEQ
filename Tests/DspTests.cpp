#include "PitchDetector.h"
#include "HarmonicFilterBank.h"
#include "WaveformMonitor.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <array>
#include <cmath>
#include <iostream>

namespace
{
int failures = 0;

void expect(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

void testPitch(double expectedHz)
{
    constexpr double sampleRate = 48000.0;
    PitchDetector detector;
    detector.prepare(sampleRate);
    PitchDetector::Estimate last;

    for (int sample = 0; sample < static_cast<int>(sampleRate); ++sample)
    {
        const auto phase = 2.0 * juce::MathConstants<double>::pi * expectedHz * sample / sampleRate;
        const auto signal = static_cast<float>(0.65 * std::sin(phase) + 0.18 * std::sin(phase * 2.0));
        if (auto estimate = detector.pushSample(signal); estimate && estimate->voiced)
            last = *estimate;
    }

    const auto centsError = 1200.0 * std::log2(last.frequencyHz / expectedHz);
    expect(last.voiced, "pitch detector should mark a harmonic tone as voiced");
    expect(std::abs(centsError) < 8.0, "pitch estimate should be within eight cents");
    expect(last.confidence > 0.75f, "clean harmonic tone should have strong confidence");
}

void testBandFrequencies()
{
    constexpr double fundamental = 220.0;
    expect(std::abs(HarmonicFilterBank::frequencyForBand(0, fundamental) - 110.0) < 0.001,
           "first lower band should be fundamental divided by two");
    expect(std::abs(HarmonicFilterBank::frequencyForBand(7, fundamental) - fundamental / 9.0) < 0.001,
           "eighth lower band should be fundamental divided by nine");
    expect(std::abs(HarmonicFilterBank::frequencyForBand(8, fundamental) - fundamental) < 0.001,
           "centre band should be the fundamental");
    expect(std::abs(HarmonicFilterBank::frequencyForBand(9, fundamental) - 440.0) < 0.001,
           "first upper band should be the second harmonic");
}

void testFilterGain()
{
    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 256;
    HarmonicFilterBank bank;
    bank.prepare(sampleRate, 1);
    std::array<float, HarmonicFilterBank::totalBands> gains {};
    gains[9] = 6.0f;

    double inputSquares = 0.0;
    double outputSquares = 0.0;
    int measured = 0;
    int absoluteSample = 0;

    for (int block = 0; block < 80; ++block)
    {
        juce::AudioBuffer<float> buffer(1, blockSize);
        for (int sample = 0; sample < blockSize; ++sample, ++absoluteSample)
        {
            const auto value = static_cast<float>(0.1 * std::sin(2.0 * juce::MathConstants<double>::pi
                                                                  * 440.0 * absoluteSample / sampleRate));
            buffer.setSample(0, sample, value);
            if (block >= 60)
                inputSquares += static_cast<double>(value) * value;
        }

        bank.process(buffer, 220.0, true, 0, 1, 10.0f, gains);
        if (block >= 60)
        {
            for (int sample = 0; sample < blockSize; ++sample)
            {
                const auto value = static_cast<double>(buffer.getSample(0, sample));
                outputSquares += value * value;
                ++measured;
            }
        }
    }

    const auto ratio = std::sqrt(outputSquares / measured) / std::sqrt(inputSquares / measured);
    expect(ratio > 1.85 && ratio < 2.15, "+6 dB harmonic band should approximately double centre-frequency amplitude");
}

void testResponseVisualization()
{
    constexpr double sampleRate = 48000.0;
    std::array<float, HarmonicFilterBank::totalBands> gains {};
    gains[9] = 6.0f;

    const auto atHarmonic = HarmonicFilterBank::magnitudeResponseDb(
        440.0, 220.0, true, 0, 1, 10.0f, gains, sampleRate);
    const auto shiftedHarmonic = HarmonicFilterBank::magnitudeResponseDb(
        880.0, 440.0, true, 0, 1, 10.0f, gains, sampleRate);
    const auto oldFrequencyAfterShift = HarmonicFilterBank::magnitudeResponseDb(
        440.0, 440.0, true, 0, 1, 10.0f, gains, sampleRate);

    expect(std::abs(atHarmonic - 6.0) < 0.05, "visual response should match the +6 dB audio band");
    expect(std::abs(shiftedHarmonic - 6.0) < 0.05, "visual response should move with the fundamental");
    expect(oldFrequencyAfterShift < 1.0, "old harmonic position should flatten after the note moves");
}

void testWaveformMonitor()
{
    WaveformMonitor monitor;
    std::array<float, WaveformMonitor::capacity> snapshot {};

    for (int sample = 1; sample <= WaveformMonitor::decimation * 3; ++sample)
        monitor.pushSample(static_cast<float>(sample));
    monitor.copySnapshot(snapshot);

    expect(std::abs(snapshot[WaveformMonitor::capacity - 3]
                    - static_cast<float>(WaveformMonitor::decimation)) < 0.0001f,
           "waveform monitor should publish the first decimated output sample");
    expect(std::abs(snapshot[WaveformMonitor::capacity - 2]
                    - static_cast<float>(WaveformMonitor::decimation * 2)) < 0.0001f,
           "waveform monitor should preserve chronological order");
    expect(std::abs(snapshot[WaveformMonitor::capacity - 1]
                    - static_cast<float>(WaveformMonitor::decimation * 3)) < 0.0001f,
           "waveform monitor should expose the latest output sample");

    monitor.reset();
    monitor.copySnapshot(snapshot);
    expect(snapshot.back() == 0.0f, "waveform monitor reset should clear the visible history");
}
}

int main()
{
    testPitch(110.0);
    testPitch(220.0);
    testPitch(440.0);
    testBandFrequencies();
    testFilterGain();
    testResponseVisualization();
    testWaveformMonitor();

    if (failures == 0)
    {
        std::cout << "All VOX HarmonicEQ DSP tests passed.\n";
        return 0;
    }

    std::cerr << failures << " test(s) failed.\n";
    return 1;
}
