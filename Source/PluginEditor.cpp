#include "PluginEditor.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace
{
const auto chassisBlack = juce::Colour::fromRGB(12, 14, 14);
const auto chassisMid = juce::Colour::fromRGB(27, 30, 30);
const auto metalLight = juce::Colour::fromRGB(205, 209, 207);
const auto screenBlack = juce::Colour::fromRGB(1, 15, 9);
const auto screenGreen = juce::Colour::fromRGB(80, 255, 139);
const auto screenDim = juce::Colour::fromRGB(28, 151, 79);
const auto eqBlue = juce::Colour::fromRGB(125, 169, 201);
const auto eqGrid = juce::Colour::fromRGB(72, 112, 137);

juce::String formatFrequency(double frequency)
{
    if (frequency <= 0.0 || !std::isfinite(frequency))
        return "--";
    if (frequency >= 1000.0)
        return juce::String(frequency / 1000.0, 2) + " kHz";
    return juce::String(frequency, frequency < 100.0 ? 1 : 0) + " Hz";
}
}

class VoxLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    VoxLookAndFeel()
    {
        setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
        setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour::fromRGB(5, 7, 7));
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colour::fromRGB(70, 75, 74));
        setColour(juce::ComboBox::backgroundColourId, juce::Colour::fromRGB(5, 7, 7));
        setColour(juce::ComboBox::textColourId, juce::Colours::white);
        setColour(juce::ComboBox::outlineColourId, juce::Colour::fromRGB(96, 101, 100));
        setColour(juce::PopupMenu::backgroundColourId, juce::Colour::fromRGB(12, 15, 14));
        setColour(juce::PopupMenu::textColourId, juce::Colours::white);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, screenDim);
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider&) override
    {
        auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                             static_cast<float>(width), static_cast<float>(height)).reduced(7.0f);
        const auto diameter = std::min(bounds.getWidth(), bounds.getHeight());
        bounds = bounds.withSizeKeepingCentre(diameter, diameter);
        const auto centre = bounds.getCentre();
        const auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        g.setColour(juce::Colours::black.withAlpha(0.72f));
        g.fillEllipse(bounds.expanded(4.0f).translated(1.5f, 3.0f));

        juce::ColourGradient outer(metalLight.brighter(0.4f), bounds.getX(), bounds.getY(),
                                   juce::Colour::fromRGB(62, 65, 64), bounds.getRight(), bounds.getBottom(), false);
        g.setGradientFill(outer);
        g.fillEllipse(bounds);

        auto inner = bounds.reduced(4.0f);
        juce::ColourGradient face(juce::Colour::fromRGB(54, 57, 56), inner.getX(), inner.getY(),
                                  juce::Colour::fromRGB(5, 6, 6), inner.getRight(), inner.getBottom(), false);
        g.setGradientFill(face);
        g.fillEllipse(inner);
        g.setColour(juce::Colours::black.withAlpha(0.8f));
        g.drawEllipse(inner, 1.0f);

        juce::Path pointer;
        const auto radius = inner.getWidth() * 0.36f;
        pointer.startNewSubPath(centre);
        pointer.lineTo(centre.x + std::sin(angle) * radius, centre.y - std::cos(angle) * radius);
        g.setColour(juce::Colours::white.withAlpha(0.92f));
        g.strokePath(pointer, juce::PathStrokeType(2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        juce::Path valueArc;
        valueArc.addCentredArc(centre.x, centre.y, diameter * 0.56f, diameter * 0.56f,
                               0.0f, rotaryStartAngle, angle, true);
        g.setColour(screenGreen.withAlpha(0.9f));
        g.strokePath(valueArc, juce::PathStrokeType(2.0f));
    }

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        if (style != juce::Slider::LinearVertical)
        {
            LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos,
                                             minSliderPos, maxSliderPos, style, slider);
            return;
        }

        const auto centreX = static_cast<float>(x + width / 2);
        const auto centreY = (minSliderPos + maxSliderPos) * 0.5f;
        const auto top = static_cast<float>(y + 3);
        const auto bottom = static_cast<float>(y + height - 3);
        g.setColour(juce::Colours::black.withAlpha(0.9f));
        g.fillRoundedRectangle(centreX - 3.0f, top, 6.0f, bottom - top, 3.0f);
        g.setColour(screenDim.withAlpha(0.22f));
        g.drawVerticalLine(static_cast<int>(centreX), top, bottom);

        g.setColour(screenGreen.withAlpha(0.95f));
        g.fillRoundedRectangle(centreX - 1.5f, std::min(sliderPos, centreY), 3.0f,
                               std::abs(sliderPos - centreY), 1.5f);
        g.setColour(metalLight);
        g.fillEllipse(centreX - 5.0f, sliderPos - 5.0f, 10.0f, 10.0f);
        g.setColour(juce::Colours::black);
        g.drawEllipse(centreX - 5.0f, sliderPos - 5.0f, 10.0f, 10.0f, 1.0f);
    }

    void drawComboBox(juce::Graphics& g, int width, int height, bool,
                      int, int, int, int, juce::ComboBox&) override
    {
        auto bounds = juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)).reduced(1.0f);
        g.setColour(juce::Colour::fromRGB(5, 7, 7));
        g.fillRoundedRectangle(bounds, 2.0f);
        g.setColour(metalLight.withAlpha(0.55f));
        g.drawRoundedRectangle(bounds, 2.0f, 1.0f);
        juce::Path arrow;
        arrow.addTriangle(static_cast<float>(width - 18), static_cast<float>(height / 2 - 2),
                          static_cast<float>(width - 8), static_cast<float>(height / 2 - 2),
                          static_cast<float>(width - 13), static_cast<float>(height / 2 + 4));
        g.setColour(metalLight);
        g.fillPath(arrow);
    }
};

class HarmonicBandControl final : public juce::Component
{
public:
    HarmonicBandControl(HarmonicEQAudioProcessor& processor, int index)
        : owner(processor), bandIndex(index),
          attachment(owner.parameters, HarmonicEQAudioProcessor::bandParameterID(index), gainSlider)
    {
        colour = bandIndex == HarmonicFilterBank::maxSubharmonics ? juce::Colours::white : screenGreen;

        gainSlider.setSliderStyle(juce::Slider::LinearVertical);
        gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 54, 18);
        gainSlider.setTextValueSuffix(" dB");
        addAndMakeVisible(gainSlider);

        nameLabel.setText(shortName(), juce::dontSendNotification);
        nameLabel.setJustificationType(juce::Justification::centred);
        nameLabel.setColour(juce::Label::textColourId, colour);
        nameLabel.setFont(juce::Font(juce::FontOptions("Avenir Next Condensed", 13.0f, juce::Font::bold)));
        addAndMakeVisible(nameLabel);

        frequencyLabel.setText("--", juce::dontSendNotification);
        frequencyLabel.setJustificationType(juce::Justification::centred);
        frequencyLabel.setColour(juce::Label::textColourId, screenGreen.withAlpha(0.62f));
        frequencyLabel.setFont(juce::Font(juce::FontOptions("Avenir Next Condensed", 10.0f, juce::Font::plain)));
        addAndMakeVisible(frequencyLabel);
    }

    void setBandState(bool active, double fundamental)
    {
        const auto frequency = HarmonicFilterBank::frequencyForBand(bandIndex, fundamental);
        const auto inRange = frequency >= 20.0 && frequency <= owner.getSampleRate() * 0.475;
        const auto enabled = active && inRange;
        gainSlider.setEnabled(enabled);
        setAlpha(enabled ? 1.0f : 0.34f);
        frequencyLabel.setText(inRange ? formatFrequency(frequency) : "out", juce::dontSendNotification);
    }

    void paint(juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat();
        g.setColour(screenDim.withAlpha(0.13f));
        g.drawVerticalLine(getWidth() - 1, 22.0f, bounds.getBottom() - 20.0f);
        g.setColour(screenDim.withAlpha(0.20f));
        g.drawHorizontalLine(getHeight() / 2, 3.0f, bounds.getRight() - 3.0f);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(4);
        nameLabel.setBounds(area.removeFromTop(22));
        frequencyLabel.setBounds(area.removeFromBottom(20));
        gainSlider.setBounds(area);
    }

private:
    juce::String shortName() const
    {
        if (bandIndex < HarmonicFilterBank::maxSubharmonics)
            return "/" + juce::String(bandIndex + 2);
        if (bandIndex == HarmonicFilterBank::maxSubharmonics)
            return "F0";
        return "x" + juce::String(bandIndex - HarmonicFilterBank::maxSubharmonics + 1);
    }

    HarmonicEQAudioProcessor& owner;
    int bandIndex;
    juce::Colour colour;
    juce::Slider gainSlider;
    juce::Label nameLabel;
    juce::Label frequencyLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment attachment;
};

HarmonicEQAudioProcessorEditor::HarmonicEQAudioProcessorEditor(HarmonicEQAudioProcessor& p)
    : AudioProcessorEditor(&p), pluginProcessor(p)
{
    voxLookAndFeel = std::make_unique<VoxLookAndFeel>();
    setLookAndFeel(voxLookAndFeel.get());
    setOpaque(true);
    setResizable(true, true);
    setResizeLimits(900, 760, 1600, 1100);

    brandLabel.setText("rco.cr  >", juce::dontSendNotification);
    brandLabel.setFont(juce::Font(juce::FontOptions("Avenir Next Condensed", 15.0f, juce::Font::bold)));
    brandLabel.setColour(juce::Label::textColourId, screenGreen.withAlpha(0.82f));
    addAndMakeVisible(brandLabel);

    titleLabel.setText("VOX HARMONIC EQ", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions("Avenir Next Condensed", 38.0f, juce::Font::bold)));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(titleLabel);

    subtitleLabel.setText("Pitch-tracking harmonic equalizer", juce::dontSendNotification);
    subtitleLabel.setFont(juce::Font(juce::FontOptions("Avenir Next Condensed", 17.0f, juce::Font::plain)));
    subtitleLabel.setColour(juce::Label::textColourId, metalLight.withAlpha(0.74f));
    addAndMakeVisible(subtitleLabel);

    for (auto* section : { &trackingSectionLabel, &masterSectionLabel, &visualizerSectionLabel, &bandsSectionLabel })
    {
        section->setFont(juce::Font(juce::FontOptions("Avenir Next Condensed", 14.0f, juce::Font::bold)));
        section->setColour(juce::Label::textColourId, metalLight);
        addAndMakeVisible(*section);
    }
    trackingSectionLabel.setText("PITCH CONTROL", juce::dontSendNotification);
    masterSectionLabel.setText("HARMONIC SHAPING", juce::dontSendNotification);
    visualizerSectionLabel.setText("APPLIED EQ + SPECTRUM", juce::dontSendNotification);
    bandsSectionLabel.setText("HARMONIC GAINS", juce::dontSendNotification);

    for (auto* label : { &detectedLabel, &activeLabel, &confidenceLabel })
    {
        label->setColour(juce::Label::textColourId, screenGreen);
        label->setJustificationType(juce::Justification::centredLeft);
        label->setFont(juce::Font(juce::FontOptions("Avenir Next Condensed", 13.0f, juce::Font::bold)));
        addAndMakeVisible(*label);
    }
    activeLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    pitchSourceBox.addItem("Detected", 1);
    pitchSourceBox.addItem("Manual", 2);
    addLabeledControl(pitchSourceBox, pitchSourceCaption, "SOURCE");

    configureRotary(manualNoteSlider);
    manualNoteSlider.setRange(0.0, 127.0, 1.0);
    manualNoteSlider.textFromValueFunction = [](double value) { return noteNameForMidi(static_cast<int>(std::lround(value))); };
    manualNoteSlider.valueFromTextFunction = [](const juce::String& text)
    {
        for (int note = 0; note <= 127; ++note)
            if (noteNameForMidi(note).equalsIgnoreCase(text.trim()))
                return static_cast<double>(note);
        return 57.0;
    };
    addLabeledControl(manualNoteSlider, manualNoteCaption, "NOTE");

    configureRotary(fineTuneSlider, " ct");
    configureRotary(glideSlider, " ms");
    configureRotary(subCountSlider);
    configureRotary(upperCountSlider);
    configureRotary(qSlider);
    configureRotary(mixSlider, "%");
    configureRotary(outputSlider, " dB");
    addLabeledControl(fineTuneSlider, fineTuneCaption, "FINE");
    addLabeledControl(glideSlider, glideCaption, "GLIDE");
    addLabeledControl(subCountSlider, subCountCaption, "BELOW");
    addLabeledControl(upperCountSlider, upperCountCaption, "ABOVE");
    addLabeledControl(qSlider, qCaption, "Q");
    addLabeledControl(mixSlider, mixCaption, "MIX");
    addLabeledControl(outputSlider, outputCaption, "OUTPUT");

    bypassButton.setColour(juce::ToggleButton::tickColourId, screenGreen);
    bypassButton.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
    addAndMakeVisible(bypassButton);

    websiteLink.setFont(juce::Font(juce::FontOptions("Avenir Next Condensed", 15.0f, juce::Font::plain)), false,
                        juce::Justification::centred);
    websiteLink.setColour(juce::HyperlinkButton::textColourId, metalLight.withAlpha(0.72f));
    websiteLink.setTooltip("Open https://rco.cr/vox/harmonicEQ");
    addAndMakeVisible(websiteLink);

    bandsViewport.setViewedComponent(&bandsContent, false);
    bandsViewport.setScrollBarsShown(false, true);
    bandsViewport.setScrollBarThickness(10);
    bandsViewport.setOpaque(false);
    bandsContent.setOpaque(false);
    bandsViewport.setColour(juce::ScrollBar::thumbColourId, screenGreen.withAlpha(0.72f));
    bandsViewport.setColour(juce::ScrollBar::trackColourId, juce::Colours::black.withAlpha(0.6f));
    addAndMakeVisible(bandsViewport);

    for (int band = 0; band < HarmonicFilterBank::totalBands; ++band)
    {
        bandControls[static_cast<size_t>(band)] = std::make_unique<HarmonicBandControl>(pluginProcessor, band);
        bandsContent.addAndMakeVisible(*bandControls[static_cast<size_t>(band)]);
    }

    pitchSourceAttachment = std::make_unique<ComboAttachment>(pluginProcessor.parameters, "pitchSource", pitchSourceBox);
    manualNoteAttachment = std::make_unique<SliderAttachment>(pluginProcessor.parameters, "manualNote", manualNoteSlider);
    fineTuneAttachment = std::make_unique<SliderAttachment>(pluginProcessor.parameters, "manualCents", fineTuneSlider);
    glideAttachment = std::make_unique<SliderAttachment>(pluginProcessor.parameters, "pitchGlideMs", glideSlider);
    subCountAttachment = std::make_unique<SliderAttachment>(pluginProcessor.parameters, "subharmonicCount", subCountSlider);
    upperCountAttachment = std::make_unique<SliderAttachment>(pluginProcessor.parameters, "upperHarmonicCount", upperCountSlider);
    qAttachment = std::make_unique<SliderAttachment>(pluginProcessor.parameters, "filterQ", qSlider);
    mixAttachment = std::make_unique<SliderAttachment>(pluginProcessor.parameters, "wetDry", mixSlider);
    outputAttachment = std::make_unique<SliderAttachment>(pluginProcessor.parameters, "outputGainDb", outputSlider);
    bypassAttachment = std::make_unique<ButtonAttachment>(pluginProcessor.parameters, "bypass", bypassButton);

    spectrumDb.fill(-100.0f);
    setSize(1180, 900);
    startTimerHz(20);
}

HarmonicEQAudioProcessorEditor::~HarmonicEQAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void HarmonicEQAudioProcessorEditor::configureRotary(juce::Slider& slider, const juce::String& suffix)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 66, 19);
    slider.setTextValueSuffix(suffix);
}

void HarmonicEQAudioProcessorEditor::addLabeledControl(juce::Component& control,
                                                        juce::Label& caption,
                                                        const juce::String& text)
{
    caption.setText(text, juce::dontSendNotification);
    caption.setJustificationType(juce::Justification::centred);
    caption.setColour(juce::Label::textColourId, metalLight.withAlpha(0.9f));
    caption.setFont(juce::Font(juce::FontOptions("Avenir Next Condensed", 12.0f, juce::Font::bold)));
    addAndMakeVisible(caption);
    addAndMakeVisible(control);
}

juce::String HarmonicEQAudioProcessorEditor::noteNameForMidi(int note)
{
    static const juce::StringArray names { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    note = juce::jlimit(0, 127, note);
    return names[note % 12] + juce::String(note / 12 - 1);
}

void HarmonicEQAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
    auto chassis = getLocalBounds().toFloat().reduced(10.0f);

    g.setColour(juce::Colours::black.withAlpha(0.9f));
    g.fillRoundedRectangle(chassis.translated(0.0f, 5.0f), 13.0f);
    juce::ColourGradient chassisGradient(chassisMid.brighter(0.08f), chassis.getX(), chassis.getY(),
                                         chassisBlack, chassis.getRight(), chassis.getBottom(), false);
    g.setGradientFill(chassisGradient);
    g.fillRoundedRectangle(chassis, 12.0f);
    g.setColour(metalLight.withAlpha(0.75f));
    g.drawRoundedRectangle(chassis, 12.0f, 1.4f);
    g.setColour(juce::Colours::black.withAlpha(0.95f));
    g.drawRoundedRectangle(chassis.reduced(4.0f), 9.0f, 2.0f);
    g.setColour(metalLight.withAlpha(0.26f));
    g.drawRoundedRectangle(chassis.reduced(6.0f), 8.0f, 1.0f);

    for (int y = 20; y < getHeight() - 20; y += 4)
    {
        g.setColour((y % 8 == 0 ? juce::Colours::white : juce::Colours::black).withAlpha(0.018f));
        g.drawHorizontalLine(y, 22.0f, static_cast<float>(getWidth() - 22));
    }

    auto drawScrew = [&g](float cx, float cy)
    {
        juce::ColourGradient screwGradient(juce::Colour::fromRGB(115, 119, 117), cx - 7.0f, cy - 7.0f,
                                            juce::Colours::black, cx + 7.0f, cy + 7.0f, false);
        g.setGradientFill(screwGradient);
        g.fillEllipse(cx - 11.0f, cy - 11.0f, 22.0f, 22.0f);
        g.setColour(juce::Colours::black);
        g.drawEllipse(cx - 11.0f, cy - 11.0f, 22.0f, 22.0f, 1.5f);
        g.drawLine(cx - 5.0f, cy, cx + 5.0f, cy, 1.5f);
        g.drawLine(cx, cy - 5.0f, cx, cy + 5.0f, 1.5f);
    };
    drawScrew(30.0f, 30.0f);
    drawScrew(static_cast<float>(getWidth() - 30), 30.0f);
    drawScrew(30.0f, static_cast<float>(getHeight() - 30));
    drawScrew(static_cast<float>(getWidth() - 30), static_cast<float>(getHeight() - 30));

    g.setColour(metalLight.withAlpha(0.28f));
    g.drawHorizontalLine(128, 28.0f, static_cast<float>(getWidth() - 28));
    g.drawHorizontalLine(getHeight() - 55, 28.0f, static_cast<float>(getWidth() - 28));

    const auto drawDisplayFrame = [&g](juce::Rectangle<int> frame)
    {
        const auto bezel = frame.toFloat();
        g.setColour(juce::Colours::black);
        g.fillRoundedRectangle(bezel.expanded(8.0f), 15.0f);
        g.setColour(metalLight.withAlpha(0.75f));
        g.drawRoundedRectangle(bezel.expanded(6.0f), 13.0f, 1.6f);
        g.setColour(juce::Colour::fromRGB(48, 52, 51));
        g.drawRoundedRectangle(bezel.expanded(3.0f), 11.0f, 2.0f);

        juce::ColourGradient screenGradient(screenBlack.brighter(0.04f), bezel.getX(), bezel.getY(),
                                            juce::Colours::black, bezel.getRight(), bezel.getBottom(), false);
        g.setGradientFill(screenGradient);
        g.fillRoundedRectangle(bezel, 10.0f);
        g.setColour(screenDim.withAlpha(0.46f));
        g.drawRoundedRectangle(bezel, 10.0f, 1.0f);
    };

    if (!visualizerFrame.isEmpty())
    {
        drawDisplayFrame(visualizerFrame);
        const auto bezel = visualizerFrame.toFloat();

        auto grid = bezel.reduced(14.0f).withTrimmedTop(34.0f).withTrimmedBottom(18.0f);
        g.setColour(eqGrid.withAlpha(0.22f));
        for (int i = 1; i < 5; ++i)
            g.drawHorizontalLine(static_cast<int>(grid.getY() + grid.getHeight() * static_cast<float>(i) / 5.0f), grid.getX(), grid.getRight());
        for (int i = 1; i < 10; ++i)
            g.drawVerticalLine(static_cast<int>(grid.getX() + grid.getWidth() * static_cast<float>(i) / 10.0f), grid.getY(), grid.getBottom());

        const auto sampleRate = std::max(8000.0, pluginProcessor.getSampleRate());
        const auto maxFrequency = std::min(20000.0, sampleRate * 0.475);
        const auto activeFundamental = pluginProcessor.activeFundamentalHz.load(std::memory_order_relaxed);
        const bool manual = pluginProcessor.parameters.getRawParameterValue("pitchSource")->load() >= 0.5f;
        const bool pitchIsValid = manual || pluginProcessor.displayedVoiced.load(std::memory_order_relaxed);
        const auto subCount = static_cast<int>(std::lround(
            pluginProcessor.parameters.getRawParameterValue("subharmonicCount")->load()));
        const auto upperCount = static_cast<int>(std::lround(
            pluginProcessor.parameters.getRawParameterValue("upperHarmonicCount")->load()));
        const auto q = pluginProcessor.parameters.getRawParameterValue("filterQ")->load();
        std::array<float, HarmonicFilterBank::totalBands> gains {};
        for (int band = 0; band < HarmonicFilterBank::totalBands; ++band)
            gains[static_cast<size_t>(band)] = pluginProcessor.parameters
                .getRawParameterValue(HarmonicEQAudioProcessor::bandParameterID(band))->load();

        const auto xForFrequency = [grid, maxFrequency](double frequency)
        {
            const auto proportion = std::log(frequency / 20.0) / std::log(maxFrequency / 20.0);
            return grid.getX() + static_cast<float>(proportion) * grid.getWidth();
        };
        const auto yForDb = [grid](double gainDb)
        {
            return juce::jmap(static_cast<float>(juce::jlimit(-18.0, 18.0, gainDb)),
                              18.0f, -18.0f, grid.getY(), grid.getBottom());
        };
        const auto zeroY = yForDb(0.0);

        const std::array<double, 10> frequencyMarks { 20.0, 50.0, 100.0, 200.0, 500.0,
                                                       1000.0, 2000.0, 5000.0, 10000.0, 20000.0 };
        g.setFont(juce::Font(juce::FontOptions("Avenir Next Condensed", 9.0f, juce::Font::plain)));
        for (const auto frequency : frequencyMarks)
        {
            if (frequency > maxFrequency)
                continue;
            const auto x = xForFrequency(frequency);
            g.setColour(eqGrid.withAlpha(0.25f));
            g.drawVerticalLine(static_cast<int>(x), grid.getY(), grid.getBottom());
            g.setColour(metalLight.withAlpha(0.64f));
            const auto text = frequency >= 1000.0
                ? juce::String(static_cast<int>(frequency / 1000.0)) + "k"
                : juce::String(static_cast<int>(frequency));
            g.drawText(text, static_cast<int>(x - 14.0f), static_cast<int>(zeroY + 3.0f),
                       28, 12, juce::Justification::centred, false);
        }

        g.setColour(eqBlue.withAlpha(0.45f));
        g.drawHorizontalLine(static_cast<int>(zeroY), grid.getX(), grid.getRight());

        constexpr int responsePoints = 360;
        juce::Path responsePath;
        for (int point = 0; point < responsePoints; ++point)
        {
            const auto proportion = static_cast<double>(point) / static_cast<double>(responsePoints - 1);
            const auto frequency = 20.0 * std::pow(maxFrequency / 20.0, proportion);
            const auto response = HarmonicFilterBank::magnitudeResponseDb(
                frequency, activeFundamental, pitchIsValid, subCount, upperCount, q, gains, sampleRate);
            const auto x = grid.getX() + static_cast<float>(proportion) * grid.getWidth();
            const auto y = yForDb(response);
            if (point == 0)
                responsePath.startNewSubPath(x, y);
            else
                responsePath.lineTo(x, y);
        }

        auto fillPath = responsePath;
        fillPath.lineTo(grid.getRight(), grid.getBottom());
        fillPath.lineTo(grid.getX(), grid.getBottom());
        fillPath.closeSubPath();
        juce::ColourGradient responseFill(eqBlue.withAlpha(0.48f), grid.getCentreX(), grid.getY(),
                                           eqBlue.darker(0.45f).withAlpha(0.14f), grid.getCentreX(), grid.getBottom(), false);
        g.setGradientFill(responseFill);
        g.fillPath(fillPath);

        if (spectrumReady)
        {
            constexpr int spectrumPoints = 420;
            juce::Path spectrumPath;
            for (int point = 0; point < spectrumPoints; ++point)
            {
                const auto proportion = static_cast<double>(point) / static_cast<double>(spectrumPoints - 1);
                const auto frequency = 20.0 * std::pow(maxFrequency / 20.0, proportion);
                const auto binPosition = frequency * static_cast<double>(spectrumFftSize) / sampleRate;
                const auto lowerBin = juce::jlimit(0, static_cast<int>(spectrumDb.size()) - 2,
                                                   static_cast<int>(binPosition));
                const auto fraction = static_cast<float>(binPosition - static_cast<double>(lowerBin));
                const auto level = juce::jmap(fraction, spectrumDb[static_cast<size_t>(lowerBin)],
                                              spectrumDb[static_cast<size_t>(lowerBin + 1)]);
                const auto x = grid.getX() + static_cast<float>(proportion) * grid.getWidth();
                const auto y = juce::jmap(juce::jlimit(-96.0f, -18.0f, level),
                                          -18.0f, -96.0f, grid.getY(), grid.getBottom());
                if (point == 0)
                    spectrumPath.startNewSubPath(x, y);
                else
                    spectrumPath.lineTo(x, y);
            }

            g.setColour(screenDim.darker(0.35f).withAlpha(0.28f));
            g.strokePath(spectrumPath, juce::PathStrokeType(5.0f, juce::PathStrokeType::curved,
                                                            juce::PathStrokeType::rounded));
            g.setColour(screenDim.withAlpha(0.78f));
            g.strokePath(spectrumPath, juce::PathStrokeType(1.35f, juce::PathStrokeType::curved,
                                                            juce::PathStrokeType::rounded));
        }

        g.setColour(eqBlue.withAlpha(0.20f));
        g.strokePath(responsePath, juce::PathStrokeType(7.0f, juce::PathStrokeType::curved,
                                                        juce::PathStrokeType::rounded));
        g.setColour(eqBlue.brighter(0.35f).withAlpha(pitchIsValid ? 0.92f : 0.46f));
        g.strokePath(responsePath, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved,
                                                        juce::PathStrokeType::rounded));

        if (pitchIsValid)
        {
            for (int band = 0; band < HarmonicFilterBank::totalBands; ++band)
            {
                const bool selected = band == HarmonicFilterBank::maxSubharmonics
                    || (band < HarmonicFilterBank::maxSubharmonics && band < subCount)
                    || (band > HarmonicFilterBank::maxSubharmonics
                        && band - HarmonicFilterBank::maxSubharmonics - 1 < upperCount);
                const auto bandFrequency = HarmonicFilterBank::frequencyForBand(band, activeFundamental);
                if (!selected || bandFrequency < 20.0 || bandFrequency > maxFrequency)
                    continue;

                const auto response = HarmonicFilterBank::magnitudeResponseDb(
                    bandFrequency, activeFundamental, true, subCount, upperCount, q, gains, sampleRate);
                const auto centre = juce::Point<float>(xForFrequency(bandFrequency), yForDb(response));
                const auto nodeColour = band == HarmonicFilterBank::maxSubharmonics
                    ? juce::Colours::white : screenGreen;
                g.setColour(nodeColour.withAlpha(0.20f));
                g.fillEllipse(juce::Rectangle<float>(10.0f, 10.0f).withCentre(centre));
                g.setColour(nodeColour.withAlpha(0.90f));
                g.fillEllipse(juce::Rectangle<float>(4.0f, 4.0f).withCentre(centre));
            }
        }
    }

    if (!bandsFrame.isEmpty())
        drawDisplayFrame(bandsFrame);
}

void HarmonicEQAudioProcessorEditor::resized()
{
    if (bandControls.front() == nullptr)
        return;

    const auto width = getWidth();
    const auto height = getHeight();
    brandLabel.setBounds(68, 28, 360, 20);
    titleLabel.setBounds(65, 45, 480, 48);
    subtitleLabel.setBounds(68, 88, 430, 25);
    bypassButton.setBounds(width - 130, 48, 92, 28);

    auto controlsArea = juce::Rectangle<int>(48, 142, width - 96, 132);
    auto tracking = controlsArea.removeFromLeft(controlsArea.getWidth() * 44 / 100);
    controlsArea.removeFromLeft(24);
    auto master = controlsArea;
    trackingSectionLabel.setBounds(tracking.removeFromTop(20));
    masterSectionLabel.setBounds(master.removeFromTop(20));

    auto layoutControls = [](juce::Rectangle<int> area,
                             std::initializer_list<std::pair<juce::Component*, juce::Label*>> items)
    {
        const auto cellWidth = area.getWidth() / static_cast<int>(items.size());
        int index = 0;
        for (const auto& [control, caption] : items)
        {
            auto cell = area.removeFromLeft(index++ == static_cast<int>(items.size()) - 1 ? area.getWidth() : cellWidth).reduced(4, 0);
            caption->setBounds(cell.removeFromTop(18));
            control->setBounds(cell);
        }
    };

    layoutControls(tracking, {
        { &pitchSourceBox, &pitchSourceCaption }, { &manualNoteSlider, &manualNoteCaption },
        { &fineTuneSlider, &fineTuneCaption }, { &glideSlider, &glideCaption }
    });
    layoutControls(master, {
        { &subCountSlider, &subCountCaption }, { &upperCountSlider, &upperCountCaption },
        { &qSlider, &qCaption }, { &mixSlider, &mixCaption }, { &outputSlider, &outputCaption }
    });

    auto lowerArea = juce::Rectangle<int>(54, 310, width - 108, height - 388);
    const auto visualizerHeight = juce::jlimit(150, 210, lowerArea.getHeight() * 39 / 100);
    visualizerFrame = lowerArea.removeFromTop(visualizerHeight);
    lowerArea.removeFromTop(22);
    bandsFrame = lowerArea;

    visualizerSectionLabel.setBounds(visualizerFrame.getX() + 20, visualizerFrame.getY() + 10, 190, 22);
    detectedLabel.setBounds(visualizerFrame.getX() + 215, visualizerFrame.getY() + 10, 220, 22);
    activeLabel.setBounds(visualizerFrame.getX() + 440, visualizerFrame.getY() + 10, 315, 22);
    confidenceLabel.setBounds(visualizerFrame.getRight() - 150, visualizerFrame.getY() + 10, 125, 22);
    bandsSectionLabel.setBounds(bandsFrame.getX() + 20, bandsFrame.getY() + 8, 170, 22);

    bandsViewport.setBounds(bandsFrame.reduced(12).withTrimmedTop(30));
    constexpr int bandWidth = 58;
    const auto contentHeight = std::max(140, bandsViewport.getHeight() - 10);
    bandsContent.setSize(bandWidth * HarmonicFilterBank::totalBands, contentHeight);

    int x = 0;
    for (int band = HarmonicFilterBank::maxSubharmonics - 1; band >= 0; --band)
    {
        bandControls[static_cast<size_t>(band)]->setBounds(x, 0, bandWidth, contentHeight);
        x += bandWidth;
    }
    for (int band = HarmonicFilterBank::maxSubharmonics; band < HarmonicFilterBank::totalBands; ++band)
    {
        bandControls[static_cast<size_t>(band)]->setBounds(x, 0, bandWidth, contentHeight);
        x += bandWidth;
    }

    websiteLink.setBounds(width / 2 - 145, height - 47, 290, 25);
}

void HarmonicEQAudioProcessorEditor::updateSpectrum()
{
    static_assert(WaveformMonitor::capacity == spectrumFftSize);
    std::array<float, WaveformMonitor::capacity> snapshot {};
    pluginProcessor.copyWaveformSnapshot(snapshot);

    spectrumFftData.fill(0.0f);
    std::copy(snapshot.begin(), snapshot.end(), spectrumFftData.begin());
    spectrumWindow.multiplyWithWindowingTable(spectrumFftData.data(), spectrumFftSize);
    spectrumFft.performFrequencyOnlyForwardTransform(spectrumFftData.data());

    float highestTarget = -100.0f;
    for (size_t bin = 0; bin < spectrumDb.size(); ++bin)
    {
        const auto normalizedMagnitude = spectrumFftData[bin] / static_cast<float>(spectrumFftSize * 0.5);
        const auto target = juce::Decibels::gainToDecibels(normalizedMagnitude, -100.0f);
        highestTarget = std::max(highestTarget, target);
        const auto smoothing = target > spectrumDb[bin] ? 0.58f : 0.14f;
        spectrumDb[bin] += (target - spectrumDb[bin]) * smoothing;
    }

    if (highestTarget > -88.0f)
        spectrumReady = true;
    else if (*std::max_element(spectrumDb.begin(), spectrumDb.end()) < -92.0f)
        spectrumReady = false;
}

void HarmonicEQAudioProcessorEditor::timerCallback()
{
    updateSpectrum();
    const auto detected = pluginProcessor.displayedFrequencyHz.load(std::memory_order_relaxed);
    const auto confidence = pluginProcessor.displayedConfidence.load(std::memory_order_relaxed);
    const auto voiced = pluginProcessor.displayedVoiced.load(std::memory_order_relaxed);
    const auto active = pluginProcessor.activeFundamentalHz.load(std::memory_order_relaxed);
    const auto detectedMidi = detected > 0.0 ? static_cast<int>(std::lround(69.0 + 12.0 * std::log2(detected / 440.0))) : 0;
    const auto activeMidi = active > 0.0 ? static_cast<int>(std::lround(69.0 + 12.0 * std::log2(active / 440.0))) : 0;
    const bool manual = pluginProcessor.parameters.getRawParameterValue("pitchSource")->load() >= 0.5f;

    detectedLabel.setText("DETECTED  " + (voiced ? noteNameForMidi(detectedMidi) + "  •  " + formatFrequency(detected) : "--"),
                          juce::dontSendNotification);
    activeLabel.setText("EQ TARGET  " + noteNameForMidi(activeMidi) + "  •  " + formatFrequency(active)
                        + (manual ? "  •  MANUAL" : "  •  AUTO"), juce::dontSendNotification);
    confidenceLabel.setText("CONF  " + juce::String(std::lround(confidence * 100.0f)) + "%",
                            juce::dontSendNotification);

    const auto subCount = static_cast<int>(std::lround(pluginProcessor.parameters.getRawParameterValue("subharmonicCount")->load()));
    const auto upperCount = static_cast<int>(std::lround(pluginProcessor.parameters.getRawParameterValue("upperHarmonicCount")->load()));
    for (int band = 0; band < HarmonicFilterBank::totalBands; ++band)
    {
        const bool selected = band == HarmonicFilterBank::maxSubharmonics
            || (band < HarmonicFilterBank::maxSubharmonics && band < subCount)
            || (band > HarmonicFilterBank::maxSubharmonics
                && band - HarmonicFilterBank::maxSubharmonics - 1 < upperCount);
        bandControls[static_cast<size_t>(band)]->setBandState(selected, active);
    }

    repaint(visualizerFrame);
}
