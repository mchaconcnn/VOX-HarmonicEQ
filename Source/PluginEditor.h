#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

#include <array>
#include <memory>

class HarmonicBandControl;
class VoxLookAndFeel;

class HarmonicEQAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                             private juce::Timer
{
public:
    explicit HarmonicEQAudioProcessorEditor(HarmonicEQAudioProcessor&);
    ~HarmonicEQAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    void timerCallback() override;
    void updateSpectrum();
    void configureRotary(juce::Slider&, const juce::String& suffix = {});
    void addLabeledControl(juce::Component&, juce::Label&, const juce::String&);
    static juce::String noteNameForMidi(int note);

    HarmonicEQAudioProcessor& pluginProcessor;
    std::unique_ptr<VoxLookAndFeel> voxLookAndFeel;

    juce::Label brandLabel;
    juce::Label titleLabel;
    juce::Label subtitleLabel;
    juce::Label trackingSectionLabel;
    juce::Label masterSectionLabel;
    juce::Label visualizerSectionLabel;
    juce::Label bandsSectionLabel;
    juce::Label detectedLabel;
    juce::Label activeLabel;
    juce::Label confidenceLabel;
    juce::HyperlinkButton websiteLink {
        "rco.cr/vox/harmonicEQ",
        juce::URL("https://rco.cr/vox/harmonicEQ")
    };

    juce::ComboBox pitchSourceBox;
    juce::Slider manualNoteSlider;
    juce::Slider fineTuneSlider;
    juce::Slider glideSlider;
    juce::Slider subCountSlider;
    juce::Slider upperCountSlider;
    juce::Slider qSlider;
    juce::Slider mixSlider;
    juce::Slider outputSlider;
    juce::ToggleButton bypassButton { "Bypass" };

    juce::Label pitchSourceCaption;
    juce::Label manualNoteCaption;
    juce::Label fineTuneCaption;
    juce::Label glideCaption;
    juce::Label subCountCaption;
    juce::Label upperCountCaption;
    juce::Label qCaption;
    juce::Label mixCaption;
    juce::Label outputCaption;

    juce::Viewport bandsViewport;
    juce::Component bandsContent;
    juce::Rectangle<int> visualizerFrame;
    juce::Rectangle<int> bandsFrame;
    std::array<std::unique_ptr<HarmonicBandControl>, HarmonicFilterBank::totalBands> bandControls;

    static constexpr int spectrumFftOrder = 11;
    static constexpr int spectrumFftSize = 1 << spectrumFftOrder;
    juce::dsp::FFT spectrumFft { spectrumFftOrder };
    juce::dsp::WindowingFunction<float> spectrumWindow {
        spectrumFftSize, juce::dsp::WindowingFunction<float>::hann, true
    };
    std::array<float, spectrumFftSize * 2> spectrumFftData {};
    std::array<float, spectrumFftSize / 2> spectrumDb {};
    bool spectrumReady = false;

    std::unique_ptr<ComboAttachment> pitchSourceAttachment;
    std::unique_ptr<SliderAttachment> manualNoteAttachment;
    std::unique_ptr<SliderAttachment> fineTuneAttachment;
    std::unique_ptr<SliderAttachment> glideAttachment;
    std::unique_ptr<SliderAttachment> subCountAttachment;
    std::unique_ptr<SliderAttachment> upperCountAttachment;
    std::unique_ptr<SliderAttachment> qAttachment;
    std::unique_ptr<SliderAttachment> mixAttachment;
    std::unique_ptr<SliderAttachment> outputAttachment;
    std::unique_ptr<ButtonAttachment> bypassAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HarmonicEQAudioProcessorEditor)
};
