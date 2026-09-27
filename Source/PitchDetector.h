#pragma once

#include <optional>
#include <vector>

class PitchDetector
{
public:
    struct Estimate
    {
        double frequencyHz = 0.0;
        float confidence = 0.0f;
        bool voiced = false;
    };

    void prepare(double sourceSampleRate, double minimumHz = 55.0, double maximumHz = 1760.0);
    void reset();
    std::optional<Estimate> pushSample(float sample) noexcept;

private:
    Estimate analyse() noexcept;

    double sourceRate = 48000.0;
    double analysisRate = 12000.0;
    double minHz = 55.0;
    double maxHz = 1760.0;
    int decimation = 4;
    int decimationCount = 0;
    float decimationSum = 0.0f;
    int writeIndex = 0;
    int validSamples = 0;
    int samplesSinceAnalysis = 0;
    int hopSize = 128;
    std::vector<float> ring;
    std::vector<float> window;
    std::vector<float> correlations;
};
