#include "PluginProcessor.h"
#include "PluginEditor.h"

KeyofPeaceVocalStudioGlueAudioProcessorEditor::KeyofPeaceVocalStudioGlueAudioProcessorEditor (KeyofPeaceVocalStudioGlueAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&customLookAndFeel);

    setupKnob (deMudSlider, deMudLabel, "DE-MUD");
    setupKnob (compSlider,  compLabel,  "OPTO-COMP");
    setupKnob (airSlider,   airLabel,   "SUNO AIR");
    setupKnob (deEssSlider, deEssLabel, "DE-ESSER");
    setupKnob (glueSlider,  glueLabel,  "ANALOG GLUE");

    deMudAtt = std::make_unique<Attachment> (audioProcessor.apvts, "demud", deMudSlider);
    compAtt  = std::make_unique<Attachment> (audioProcessor.apvts, "comp",  compSlider);
    airAtt   = std::make_unique<Attachment> (audioProcessor.apvts, "air",   airSlider);
    deEssAtt = std::make_unique<Attachment> (audioProcessor.apvts, "deess", deEssSlider);
    glueAtt  = std::make_unique<Attachment> (audioProcessor.apvts, "glue",  glueSlider);

    setSize (620, 270);
}

KeyofPeaceVocalStudioGlueAudioProcessorEditor::~KeyofPeaceVocalStudioGlueAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void KeyofPeaceVocalStudioGlueAudioProcessorEditor::setupKnob (juce::Slider& slider, juce::Label& label, const juce::String& text)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 20);
    slider.setTextValueSuffix (" %");
    slider.setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xffe8e8e8));
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (slider);

    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::Font (13.0f, juce::Font::bold));
    label.setColour (juce::Label::textColourId, juce::Colour (0xffd4af37));
    addAndMakeVisible (label);
}

void KeyofPeaceVocalStudioGlueAudioProcessorEditor::paint (juce::Graphics& g)
{
    juce::ColourGradient bg (juce::Colour (0xff0b132b), 0.0f, 0.0f,
                             juce::Colour (0xff1c2541), 0.0f, (float) getHeight(), false);
    g.setGradientFill (bg);
    g.fillAll();

    g.setColour (juce::Colour (0xffd4af37).withAlpha (0.35f));
    g.drawRoundedRectangle (8.0f, 8.0f, (float) getWidth() - 16.0f, (float) getHeight() - 16.0f, 10.0f, 1.5f);

    g.setColour (juce::Colour (0xfff3c64f));
    g.setFont (juce::Font (21.0f, juce::Font::bold));
    g.drawText ("KEYOFPEACE", 24, 18, 300, 28, juce::Justification::left);

    g.setColour (juce::Colour (0xffcbd5e1));
    g.setFont (juce::Font (13.0f, juce::Font::plain));
    g.drawText ("VOCAL & STUDIO GLUE  |  SUNO STEM BLENDER", 24, 44, 420, 20, juce::Justification::left);
}

void KeyofPeaceVocalStudioGlueAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (22);
    area.removeFromTop (64);

    int knobWidth = area.getWidth() / 5;

    auto placeColumn = [&] (juce::Slider& s, juce::Label& l)
    {
        auto col = area.removeFromLeft (knobWidth);
        l.setBounds (col.removeFromTop (22));
        s.setBounds (col.reduced (6, 4));
    };

    placeColumn (deMudSlider, deMudLabel);
    placeColumn (compSlider,  compLabel);
    placeColumn (airSlider,   airLabel);
    placeColumn (deEssSlider, deEssLabel);
    placeColumn (glueSlider,  glueLabel);
}
