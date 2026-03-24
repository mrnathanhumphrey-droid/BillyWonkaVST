#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "BWColours.h"

/**
 * BWLookAndFeel — Billy Wonka brand look-and-feel.
 *
 * Applies the BW colour palette and design language to all JUCE components:
 * rotary knobs, sliders, toggles, combo boxes, labels, etc.
 */
class BWLookAndFeel : public juce::LookAndFeel_V4
{
public:
    BWLookAndFeel();
    ~BWLookAndFeel() override = default;

    // --- Rotary Slider (Knob) ---
    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider&) override;

    // --- Linear Slider ---
    void drawLinearSlider(juce::Graphics&, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          juce::Slider::SliderStyle, juce::Slider&) override;

    // --- Toggle Button ---
    void drawToggleButton(juce::Graphics&, juce::ToggleButton&,
                          bool shouldDrawButtonAsHighlighted,
                          bool shouldDrawButtonAsDown) override;

    // --- ComboBox ---
    void drawComboBox(juce::Graphics&, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox&) override;

    // --- Label ---
    void drawLabel(juce::Graphics&, juce::Label&) override;

    // --- PopupMenu ---
    void drawPopupMenuBackground(juce::Graphics&, int width, int height) override;
    void drawPopupMenuItem(juce::Graphics&, const juce::Rectangle<int>& area,
                           bool isSeparator, bool isActive, bool isHighlighted,
                           bool isTicked, bool hasSubMenu, const juce::String& text,
                           const juce::String& shortcutKeyText,
                           const juce::Drawable* icon, const juce::Colour* textColour) override;

    // --- TextButton ---
    void drawButtonBackground(juce::Graphics&, juce::Button&,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;

    // --- Scrollbar ---
    void drawScrollbar(juce::Graphics&, juce::ScrollBar&, int x, int y,
                       int width, int height, bool isScrollbarVertical,
                       int thumbStartPosition, int thumbSize,
                       bool isMouseOver, bool isMouseDown) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BWLookAndFeel)
};
