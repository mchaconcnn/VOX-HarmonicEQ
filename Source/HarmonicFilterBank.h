#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>

class HarmonicFilterBank
{
public:
    static constexpr int maxSubharmonics = 8;
    static constexpr int maxUpperHarmonics = 32;
    static constexpr int totalBands = maxSubharmonics + 1 + maxUpperHarmonics;

    void prepare(double newSampleRate, int channels);
    void reset();
    void process(juce::AudioBuffer<float>& buffer,
                 double fundamentalHz,
                 bool pitchIsValid,
                 int subharmonicCount,
                 int upperHarmonicCount,
                 float q,
                 const std::array<float, totalBands>& gainsDb) noexcept;

    static double frequencyForBand(int bandIndex, double fundamentalHz) noexcept;
    static double magnitudeResponseDb(double queryFrequency,
                                      double fundamentalHz,
                                      bool pitchIsValid,
                                      int subharmonicCount,
                                      int upperHarmonicCount,
                                      float q,
                                      const std::array<float, totalBands>& gainsDb,
                                      double sampleRate) noexcept;

private:
    struct Coefficients
    {
        double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;
    };

    struct State
    {
        double z1 = 0.0, z2 = 0.0;
    };

    static Coefficients makePeak(double frequency, double q, double gainDb, double sampleRate) noexcept;
    static Coefficients subtract(Coefficients a, const Coefficients& b) noexcept;
    static Coefficients scaled(Coefficients value, double scalar) noexcept;
    static void addInPlace(Coefficients& value, const Coefficients& increment) noexcept;

    double sampleRate = 48000.0;
    int channelCount = 2;
    std::array<Coefficients, totalBands> currentCoefficients {};
    std::array<std::array<State, totalBands>, 2> states {};
};
