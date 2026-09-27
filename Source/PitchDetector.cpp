#include "PitchDetector.h"

#include <algorithm>
#include <cmath>
#include <numeric>

void PitchDetector::prepare(double sourceSampleRate, double minimumHz, double maximumHz)
{
    sourceRate = std::max(8000.0, sourceSampleRate);
    minHz = std::clamp(minimumHz, 20.0, 1000.0);
    maxHz = std::clamp(maximumHz, minHz * 2.0, 5000.0);

    decimation = std::max(1, static_cast<int>(std::round(sourceRate / 12000.0)));
    analysisRate = sourceRate / static_cast<double>(decimation);

    const auto minimumWindow = static_cast<int>(std::ceil(analysisRate / minHz * 4.0));
    int windowSize = 256;
    while (windowSize < minimumWindow)
        windowSize *= 2;
    windowSize = std::clamp(windowSize, 512, 4096);

    ring.assign(static_cast<size_t>(windowSize), 0.0f);
    window.assign(static_cast<size_t>(windowSize), 0.0f);
    correlations.assign(static_cast<size_t>(windowSize), 0.0f);
    hopSize = std::max(32, static_cast<int>(std::round(analysisRate * 0.010)));
    reset();
}

void PitchDetector::reset()
{
    std::fill(ring.begin(), ring.end(), 0.0f);
    std::fill(window.begin(), window.end(), 0.0f);
    std::fill(correlations.begin(), correlations.end(), 0.0f);
    writeIndex = 0;
    validSamples = 0;
    samplesSinceAnalysis = 0;
    decimationCount = 0;
    decimationSum = 0.0f;
}

std::optional<PitchDetector::Estimate> PitchDetector::pushSample(float sample) noexcept
{
    decimationSum += sample;
    if (++decimationCount < decimation)
        return std::nullopt;

    const auto downsampled = decimationSum / static_cast<float>(decimationCount);
    decimationSum = 0.0f;
    decimationCount = 0;

    if (ring.empty())
        return std::nullopt;

    ring[static_cast<size_t>(writeIndex)] = downsampled;
    writeIndex = (writeIndex + 1) % static_cast<int>(ring.size());
    validSamples = std::min(validSamples + 1, static_cast<int>(ring.size()));

    if (validSamples < static_cast<int>(ring.size()) || ++samplesSinceAnalysis < hopSize)
        return std::nullopt;

    samplesSinceAnalysis = 0;
    return analyse();
}

PitchDetector::Estimate PitchDetector::analyse() noexcept
{
    const auto size = static_cast<int>(ring.size());
    for (int i = 0; i < size; ++i)
        window[static_cast<size_t>(i)] = ring[static_cast<size_t>((writeIndex + i) % size)];

    const auto mean = std::accumulate(window.begin(), window.end(), 0.0) / static_cast<double>(size);
    double squareSum = 0.0;
    for (auto& value : window)
    {
        value -= static_cast<float>(mean);
        squareSum += static_cast<double>(value) * value;
    }

    const auto rms = std::sqrt(squareSum / static_cast<double>(size));
    if (rms < 0.0003)
        return {};

    const int minLag = std::max(2, static_cast<int>(std::floor(analysisRate / maxHz)));
    const int maxLag = std::min(size / 2, static_cast<int>(std::ceil(analysisRate / minHz)));
    float best = -1.0f;

    for (int lag = minLag; lag <= maxLag; ++lag)
    {
        double cross = 0.0;
        double energyA = 0.0;
        double energyB = 0.0;
        const int count = size - lag;
        for (int i = 0; i < count; ++i)
        {
            const auto a = static_cast<double>(window[static_cast<size_t>(i)]);
            const auto b = static_cast<double>(window[static_cast<size_t>(i + lag)]);
            cross += a * b;
            energyA += a * a;
            energyB += b * b;
        }

        const auto denominator = energyA + energyB;
        const auto score = denominator > 1.0e-12 ? static_cast<float>(2.0 * cross / denominator) : 0.0f;
        correlations[static_cast<size_t>(lag)] = score;
        best = std::max(best, score);
    }

    if (best < 0.55f)
        return { 0.0, std::max(0.0f, best), false };

    int chosenLag = 0;
    const auto acceptance = std::max(0.55f, best * 0.90f);
    for (int lag = minLag + 1; lag < maxLag; ++lag)
    {
        const auto value = correlations[static_cast<size_t>(lag)];
        if (value >= acceptance
            && value > correlations[static_cast<size_t>(lag - 1)]
            && value >= correlations[static_cast<size_t>(lag + 1)])
        {
            chosenLag = lag;
            break;
        }
    }

    if (chosenLag == 0)
        return { 0.0, best, false };

    const auto left = static_cast<double>(correlations[static_cast<size_t>(chosenLag - 1)]);
    const auto centre = static_cast<double>(correlations[static_cast<size_t>(chosenLag)]);
    const auto right = static_cast<double>(correlations[static_cast<size_t>(chosenLag + 1)]);
    const auto curvature = left - 2.0 * centre + right;
    const auto offset = std::abs(curvature) > 1.0e-9 ? 0.5 * (left - right) / curvature : 0.0;
    const auto refinedLag = static_cast<double>(chosenLag) + std::clamp(offset, -1.0, 1.0);
    const auto frequency = analysisRate / refinedLag;

    return { frequency, correlations[static_cast<size_t>(chosenLag)], true };
}
