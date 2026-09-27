#pragma once

#include <array>
#include <atomic>
#include <cstdint>

class WaveformMonitor
{
public:
    static constexpr size_t capacity = 2048;
    static constexpr int decimation = 1;

    WaveformMonitor() noexcept
    {
        reset();
    }

    void reset() noexcept
    {
        for (auto& sample : samples)
            sample.store(0.0f, std::memory_order_relaxed);
        publishedSamples.store(0, std::memory_order_relaxed);
        decimationCounter = 0;
    }

    void pushSample(float sample) noexcept
    {
        if (++decimationCounter < decimation)
            return;

        decimationCounter = 0;
        const auto writePosition = publishedSamples.load(std::memory_order_relaxed);
        samples[static_cast<size_t>(writePosition % capacity)].store(sample, std::memory_order_relaxed);
        publishedSamples.store(writePosition + 1, std::memory_order_release);
    }

    void copySnapshot(std::array<float, capacity>& destination) const noexcept
    {
        destination.fill(0.0f);
        const auto end = publishedSamples.load(std::memory_order_acquire);
        const auto available = static_cast<size_t>(end < capacity ? end : capacity);
        const auto start = end - available;
        const auto destinationOffset = capacity - available;

        for (size_t index = 0; index < available; ++index)
            destination[destinationOffset + index] = samples[static_cast<size_t>((start + index) % capacity)]
                .load(std::memory_order_relaxed);
    }

private:
    std::array<std::atomic<float>, capacity> samples;
    std::atomic<std::uint64_t> publishedSamples { 0 };
    int decimationCounter = 0;
};
