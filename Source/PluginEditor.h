#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class KeyofPeaceLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, const float rotaryStartAngle,
                           const float rotaryEndAngle, juce::Slider&) override
    {
        auto radius = (float) juce::jmin (width / 2, height / 2) - 6.0f;
        auto centreX = (float) x + (float) width  * 0.5f;
        auto centreY = (float) y + (float) height * 0.5f;
        auto rx = centreX - radius;
        auto ry = centreY - radius;
        auto rw = radius * 2.0f;
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // Cuerpo de la perilla
        g.setColour (juce::Colour (0xff16213e));
        g.fillEllipse (rx, ry, rw, rw);

        // Borde dorado sutil
        g.setColour (juce::Colour (0xffd4af37).withAlpha (0.45f));
        g.drawEllipse (rx, ry, rw, rw, 1.5f);

        // Arco de progreso dorado
        juce::Path arcPath;
        arcPath.addCentredArc (centreX, centreY, radius + 3.0f, radius + 3.0f,
                               0.0f, rotaryStartAngle, angle, true);
        g.setColour (juce::Colour (0xfff3c64f));
        g.strokePath (arcPath, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Indicador de posición
        juce::Path pointer;
        auto pointerLength = radius * 0.65f;
        auto pointerThickness = 3.0f;
        pointer.addRectangle (-pointerThickness * 0.5f, -radius + 3.0f, pointerThickness, pointerLength);
        pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (centreX, centreY));
        g.setColour (juce::Colours::white);
        g.fillPath (pointer);
    }
};

class KeyofPeaceVocalStudioGlueAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    KeyofPeaceVocalStudioGlueAudioProcessorEditor (KeyofPeaceVocalStudioGlueAudioProcessor&);
    ~KeyofPeaceVocalStudioGlueAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    KeyofPeaceVocalStudioGlueAudioProcessor& audioProcessor;
    KeyofPeaceLookAndFeel customLookAndFeel;

    juce::Slider deMudSlider, compSlider, airSlider, deEssSlider, glueSlider;
    juce::Label  deMudLabel,  compLabel,  airLabel,  deEssLabel,  glueLabel;

    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<Attachment> deMudAtt, compAtt, airAtt, deEssAtt, glueAtt;

    void setupKnob (juce::Slider& slider, juce::Label& label, const juce::String& text);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KeyofPeaceVocalStudioGlueAudioProcessorEditor)
};
