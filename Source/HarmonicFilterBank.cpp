#include "HarmonicFilterBank.h"

#include <algorithm>
#include <cmath>
#include <complex>

void HarmonicFilterBank::prepare(double newSampleRate, int channels)
{
    sampleRate = std::max(8000.0, newSampleRate);
    channelCount = std::clamp(channels, 1, 2);
    reset();
}

void HarmonicFilterBank::reset()
{
    currentCoefficients.fill({});
    for (auto& channel : states)
        channel.fill({});
}

double HarmonicFilterBank::frequencyForBand(int bandIndex, double fundamentalHz) noexcept
{
    if (bandIndex < 0 || bandIndex >= totalBands || fundamentalHz <= 0.0)
        return 0.0;
    if (bandIndex < maxSubharmonics)
        return fundamentalHz / static_cast<double>(bandIndex + 2);
    if (bandIndex == maxSubharmonics)
        return fundamentalHz;
    return fundamentalHz * static_cast<double>(bandIndex - maxSubharmonics + 1);
}

double HarmonicFilterBank::magnitudeResponseDb(double queryFrequency,
                                               double fundamentalHz,
                                               bool pitchIsValid,
                                               int subharmonicCount,
                                               int upperHarmonicCount,
                                               float q,
                                               const std::array<float, totalBands>& gainsDb,
                                               double rate) noexcept
{
    if (!pitchIsValid || queryFrequency < 20.0 || rate < 8000.0
        || queryFrequency > rate * 0.475)
        return 0.0;

    subharmonicCount = std::clamp(subharmonicCount, 0, maxSubharmonics);
    upperHarmonicCount = std::clamp(upperHarmonicCount, 0, maxUpperHarmonics);

    const auto omega = 2.0 * juce::MathConstants<double>::pi * queryFrequency / rate;
    const std::complex<double> z1 { std::cos(omega), -std::sin(omega) };
    const std::complex<double> z2 { std::cos(2.0 * omega), -std::sin(2.0 * omega) };
    double responseDb = 0.0;

    for (int band = 0; band < totalBands; ++band)
    {
        const bool isSub = band < maxSubharmonics;
        const bool isFundamental = band == maxSubharmonics;
        const int upperIndex = band - maxSubharmonics - 1;
        const bool selected = isFundamental
                           || (isSub && band < subharmonicCount)
                           || (upperIndex >= 0 && upperIndex < upperHarmonicCount);
        if (!selected)
            continue;

        const auto bandFrequency = frequencyForBand(band, fundamentalHz);
        if (bandFrequency < 20.0 || bandFrequency > rate * 0.475)
            continue;

        const auto coefficients = makePeak(bandFrequency, q, gainsDb[static_cast<size_t>(band)], rate);
        const auto numerator = coefficients.b0 + coefficients.b1 * z1 + coefficients.b2 * z2;
        const auto denominator = 1.0 + coefficients.a1 * z1 + coefficients.a2 * z2;
        const auto magnitude = std::abs(numerator / denominator);
        responseDb += 20.0 * std::log10(std::max(1.0e-12, magnitude));
    }

    return responseDb;
}

HarmonicFilterBank::Coefficients HarmonicFilterBank::makePeak(double frequency,
                                                               double q,
                                                               double gainDb,
                                                               double rate) noexcept
{
    if (std::abs(gainDb) < 0.0001 || frequency < 20.0 || frequency > rate * 0.475)
        return {};

    const auto A = std::pow(10.0, gainDb / 40.0);
    const auto omega = 2.0 * juce::MathConstants<double>::pi * frequency / rate;
    const auto alpha = std::sin(omega) / (2.0 * std::max(0.1, q));
    const auto cosOmega = std::cos(omega);
    const auto a0 = 1.0 + alpha / A;

    return {
        (1.0 + alpha * A) / a0,
        (-2.0 * cosOmega) / a0,
        (1.0 - alpha * A) / a0,
        (-2.0 * cosOmega) / a0,
        (1.0 - alpha / A) / a0
    };
}

HarmonicFilterBank::Coefficients HarmonicFilterBank::subtract(Coefficients a,
                                                               const Coefficients& b) noexcept
{
    a.b0 -= b.b0; a.b1 -= b.b1; a.b2 -= b.b2; a.a1 -= b.a1; a.a2 -= b.a2;
    return a;
}

HarmonicFilterBank::Coefficients HarmonicFilterBank::scaled(Coefficients value, double scalar) noexcept
{
    value.b0 *= scalar; value.b1 *= scalar; value.b2 *= scalar;
    value.a1 *= scalar; value.a2 *= scalar;
    return value;
}

void HarmonicFilterBank::addInPlace(Coefficients& value, const Coefficients& increment) noexcept
{
    value.b0 += increment.b0; value.b1 += increment.b1; value.b2 += increment.b2;
    value.a1 += increment.a1; value.a2 += increment.a2;
}

void HarmonicFilterBank::process(juce::AudioBuffer<float>& buffer,
                                 double fundamentalHz,
                                 bool pitchIsValid,
                                 int subharmonicCount,
                                 int upperHarmonicCount,
                                 float q,
                                 const std::array<float, totalBands>& gainsDb) noexcept
{
    const auto samples = buffer.getNumSamples();
    const auto channels = std::min(channelCount, buffer.getNumChannels());
    if (samples <= 0 || channels <= 0)
        return;

    subharmonicCount = std::clamp(subharmonicCount, 0, maxSubharmonics);
    upperHarmonicCount = std::clamp(upperHarmonicCount, 0, maxUpperHarmonics);

    for (int band = 0; band < totalBands; ++band)
    {
        const bool isSub = band < maxSubharmonics;
        const bool isFundamental = band == maxSubharmonics;
        const int upperIndex = band - maxSubharmonics - 1;
        const bool selected = isFundamental
                           || (isSub && band < subharmonicCount)
                           || (upperIndex >= 0 && upperIndex < upperHarmonicCount);
        const auto frequency = frequencyForBand(band, fundamentalHz);
        const bool inRange = frequency >= 20.0 && frequency <= sampleRate * 0.475;
        const auto gain = pitchIsValid && selected && inRange ? gainsDb[static_cast<size_t>(band)] : 0.0f;
        const auto target = makePeak(frequency, q, gain, sampleRate);
        const auto start = currentCoefficients[static_cast<size_t>(band)];
        const auto increment = scaled(subtract(target, start), 1.0 / static_cast<double>(samples));

        for (int channel = 0; channel < channels; ++channel)
        {
            auto coefficients = start;
            auto state = states[static_cast<size_t>(channel)][static_cast<size_t>(band)];
            auto* data = buffer.getWritePointer(channel);

            for (int sample = 0; sample < samples; ++sample)
            {
                addInPlace(coefficients, increment);
                const auto input = static_cast<double>(data[sample]);
                const auto output = coefficients.b0 * input + state.z1;
                state.z1 = coefficients.b1 * input - coefficients.a1 * output + state.z2;
                state.z2 = coefficients.b2 * input - coefficients.a2 * output;
                data[sample] = static_cast<float>(output);
            }

            states[static_cast<size_t>(channel)][static_cast<size_t>(band)] = state;
        }

        currentCoefficients[static_cast<size_t>(band)] = target;
    }
}
