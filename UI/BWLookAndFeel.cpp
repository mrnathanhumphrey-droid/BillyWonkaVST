#include "BWLookAndFeel.h"

static constexpr float PI = 3.14159265358979323846f;

BWLookAndFeel::BWLookAndFeel()
{
    // --- Global defaults ---
    setColour(juce::ResizableWindow::backgroundColourId, BW::Black);
    setColour(juce::DocumentWindow::backgroundColourId, BW::Black);

    // Label
    setColour(juce::Label::textColourId, BW::White);
    setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

    // TextButton
    setColour(juce::TextButton::buttonColourId, BW::PurpleDark);
    setColour(juce::TextButton::textColourOnId, BW::White);
    setColour(juce::TextButton::textColourOffId, BW::PinkSoft);

    // ComboBox
    setColour(juce::ComboBox::backgroundColourId, BW::Deep);
    setColour(juce::ComboBox::textColourId, BW::White);
    setColour(juce::ComboBox::outlineColourId, BW::Grey);
    setColour(juce::ComboBox::arrowColourId, BW::PurpleGlow);

    // PopupMenu
    setColour(juce::PopupMenu::backgroundColourId, BW::Deep);
    setColour(juce::PopupMenu::textColourId, BW::White);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, BW::Purple);
    setColour(juce::PopupMenu::highlightedTextColourId, BW::White);

    // ScrollBar
    setColour(juce::ScrollBar::thumbColourId, BW::Purple);
    setColour(juce::ScrollBar::trackColourId, BW::Deep);

    // TextEditor
    setColour(juce::TextEditor::backgroundColourId, BW::Deep);
    setColour(juce::TextEditor::textColourId, BW::White);
    setColour(juce::TextEditor::outlineColourId, BW::Grey);
    setColour(juce::TextEditor::focusedOutlineColourId, BW::Purple);
    setColour(juce::TextEditor::highlightColourId, BW::Purple.withAlpha(0.4f));

    // Slider
    setColour(juce::Slider::textBoxTextColourId, BW::PinkSoft);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);

    // Caret
    setColour(juce::CaretComponent::caretColourId, BW::Pink);
}

// =============================================================================
// Rotary Slider (Knob)
// =============================================================================

void BWLookAndFeel::drawRotarySlider(juce::Graphics& g,
                                     int x, int y, int width, int height,
                                     float sliderPos,
                                     float rotaryStartAngle,
                                     float rotaryEndAngle,
                                     juce::Slider& slider)
{
    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(4.0f);
    float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    float centreX = bounds.getCentreX();
    float centreY = bounds.getCentreY();

    float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // --- Outer rim (purple gradient ring) ---
    float rimThickness = radius * 0.12f;
    {
        juce::Path rimArc;
        rimArc.addCentredArc(centreX, centreY, radius, radius,
                             0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(BW::Grey.withAlpha(0.4f));
        g.strokePath(rimArc, juce::PathStrokeType(rimThickness,
                     juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // --- Filled arc (value indicator) ---
    {
        juce::Path valueArc;
        valueArc.addCentredArc(centreX, centreY, radius, radius,
                               0.0f, rotaryStartAngle, angle, true);
        g.setColour(BW::Purple);
        g.strokePath(valueArc, juce::PathStrokeType(rimThickness,
                     juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // --- Knob face (dark circle) ---
    float knobRadius = radius - rimThickness - 2.0f;
    if (knobRadius > 2.0f)
    {
        // Drop shadow / glow on hover
        if (slider.isMouseOverOrDragging())
        {
            g.setColour(BW::GlowShadow.withAlpha(0.3f));
            g.fillEllipse(centreX - knobRadius - 2, centreY - knobRadius - 2,
                          (knobRadius + 2) * 2, (knobRadius + 2) * 2);
        }

        // Main face
        g.setColour(BW::KnobFace);
        g.fillEllipse(centreX - knobRadius, centreY - knobRadius,
                      knobRadius * 2, knobRadius * 2);

        // Thin border
        g.setColour(BW::Grey.withAlpha(0.5f));
        g.drawEllipse(centreX - knobRadius, centreY - knobRadius,
                      knobRadius * 2, knobRadius * 2, 1.0f);
    }

    // --- Indicator dot (pink) ---
    float dotRadius = juce::jmax(2.0f, radius * 0.08f);
    float dotDistance = knobRadius * 0.7f;
    float dotX = centreX + dotDistance * std::cos(angle - PI * 0.5f);
    float dotY = centreY + dotDistance * std::sin(angle - PI * 0.5f);

    g.setColour(BW::Pink);
    g.fillEllipse(dotX - dotRadius, dotY - dotRadius,
                  dotRadius * 2.0f, dotRadius * 2.0f);
}

// =============================================================================
// Linear Slider
// =============================================================================

void BWLookAndFeel::drawLinearSlider(juce::Graphics& g,
                                     int x, int y, int width, int height,
                                     float sliderPos, float minSliderPos, float maxSliderPos,
                                     juce::Slider::SliderStyle style, juce::Slider& slider)
{
    bool isVertical = (style == juce::Slider::LinearVertical ||
                       style == juce::Slider::LinearBarVertical);

    auto trackBounds = juce::Rectangle<float>(
        static_cast<float>(x), static_cast<float>(y),
        static_cast<float>(width), static_cast<float>(height));

    float trackThickness = 3.0f;

    if (isVertical)
    {
        float trackX = trackBounds.getCentreX() - trackThickness * 0.5f;
        // Background track
        g.setColour(BW::Grey);
        g.fillRoundedRectangle(trackX, trackBounds.getY(),
                               trackThickness, trackBounds.getHeight(), 1.5f);
        // Filled portion
        g.setColour(BW::Purple);
        g.fillRoundedRectangle(trackX, sliderPos,
                               trackThickness, trackBounds.getBottom() - sliderPos, 1.5f);
        // Thumb
        float thumbW = 14.0f, thumbH = 6.0f;
        g.setColour(BW::Pink);
        g.fillRoundedRectangle(trackBounds.getCentreX() - thumbW * 0.5f,
                               sliderPos - thumbH * 0.5f, thumbW, thumbH, 3.0f);
    }
    else
    {
        float trackY = trackBounds.getCentreY() - trackThickness * 0.5f;
        g.setColour(BW::Grey);
        g.fillRoundedRectangle(trackBounds.getX(), trackY,
                               trackBounds.getWidth(), trackThickness, 1.5f);
        g.setColour(BW::Purple);
        g.fillRoundedRectangle(trackBounds.getX(), trackY,
                               sliderPos - trackBounds.getX(), trackThickness, 1.5f);
        float thumbW = 6.0f, thumbH = 14.0f;
        g.setColour(BW::Pink);
        g.fillRoundedRectangle(sliderPos - thumbW * 0.5f,
                               trackBounds.getCentreY() - thumbH * 0.5f,
                               thumbW, thumbH, 3.0f);
    }
}

// =============================================================================
// Toggle Button
// =============================================================================

void BWLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                     bool shouldDrawButtonAsHighlighted,
                                     bool /*shouldDrawButtonAsDown*/)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(2.0f);
    bool isOn = button.getToggleState();

    // Background
    g.setColour(isOn ? BW::Purple : BW::Deep);
    g.fillRoundedRectangle(bounds, 4.0f);

    // Border
    g.setColour(isOn ? BW::PurpleGlow : BW::Grey);
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

    // Glow when on
    if (isOn && shouldDrawButtonAsHighlighted)
    {
        g.setColour(BW::PinkGlow.withAlpha(0.15f));
        g.fillRoundedRectangle(bounds.expanded(2.0f), 6.0f);
    }

    // Text
    g.setColour(isOn ? BW::PinkSoft : BW::TextMuted);
    g.setFont(juce::Font(12.0f));
    g.drawText(button.getButtonText(), bounds, juce::Justification::centred);
}

// =============================================================================
// ComboBox
// =============================================================================

void BWLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height,
                                 bool /*isButtonDown*/,
                                 int /*buttonX*/, int /*buttonY*/,
                                 int /*buttonW*/, int /*buttonH*/,
                                 juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<float>(0, 0, (float)width, (float)height);

    g.setColour(BW::Deep);
    g.fillRoundedRectangle(bounds, 4.0f);

    g.setColour(box.hasKeyboardFocus(false) ? BW::Purple : BW::Grey);
    g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);

    // Arrow
    float arrowX = (float)width - 20.0f;
    float arrowY = (float)height * 0.5f;
    juce::Path arrow;
    arrow.addTriangle(arrowX - 4.0f, arrowY - 3.0f,
                      arrowX + 4.0f, arrowY - 3.0f,
                      arrowX, arrowY + 3.0f);
    g.setColour(BW::PurpleGlow);
    g.fillPath(arrow);
}

// =============================================================================
// Label
// =============================================================================

void BWLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label)
{
    g.fillAll(label.findColour(juce::Label::backgroundColourId));

    auto textArea = label.getBorderSize().subtractedFrom(label.getLocalBounds());
    g.setColour(label.findColour(juce::Label::textColourId));
    g.setFont(label.getFont());
    g.drawText(label.getText(), textArea, label.getJustificationType(), false);
}

// =============================================================================
// PopupMenu
// =============================================================================

void BWLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height)
{
    g.fillAll(BW::Deep);
    g.setColour(BW::Grey);
    g.drawRect(0, 0, width, height, 1);
}

void BWLookAndFeel::drawPopupMenuItem(juce::Graphics& g,
                                      const juce::Rectangle<int>& area,
                                      bool isSeparator, bool isActive,
                                      bool isHighlighted, bool isTicked,
                                      bool /*hasSubMenu*/, const juce::String& text,
                                      const juce::String& /*shortcutKeyText*/,
                                      const juce::Drawable* /*icon*/,
                                      const juce::Colour* /*textColour*/)
{
    if (isSeparator)
    {
        g.setColour(BW::Grey.withAlpha(0.5f));
        g.fillRect(area.reduced(8, 0).withHeight(1));
        return;
    }

    if (isHighlighted)
    {
        g.setColour(BW::Purple);
        g.fillRect(area);
    }

    g.setColour(isActive ? (isHighlighted ? BW::White : BW::PinkSoft) : BW::TextMuted);
    g.setFont(juce::Font(13.0f));

    auto textArea = area.reduced(12, 0);
    if (isTicked)
    {
        g.setColour(BW::Pink);
        g.drawText(juce::String::charToString(0x2713), textArea.removeFromLeft(20),
                   juce::Justification::centred);
    }
    g.drawText(text, textArea, juce::Justification::centredLeft);
}

// =============================================================================
// TextButton
// =============================================================================

void BWLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                         const juce::Colour& /*backgroundColour*/,
                                         bool shouldDrawButtonAsHighlighted,
                                         bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
    bool isOn = button.getToggleState();

    if (shouldDrawButtonAsDown || isOn)
        g.setColour(BW::Purple);
    else if (shouldDrawButtonAsHighlighted)
        g.setColour(BW::PurpleDark.brighter(0.15f));
    else
        g.setColour(BW::PurpleDark);

    g.fillRoundedRectangle(bounds, 6.0f);

    // Border — pink glow when active, purple outline otherwise
    g.setColour(isOn ? BW::PurpleGlow : BW::PurpleBorder);
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);
}

// =============================================================================
// Scrollbar
// =============================================================================

void BWLookAndFeel::drawScrollbar(juce::Graphics& g, juce::ScrollBar&,
                                  int x, int y, int width, int height,
                                  bool isScrollbarVertical,
                                  int thumbStartPosition, int thumbSize,
                                  bool isMouseOver, bool /*isMouseDown*/)
{
    g.setColour(BW::Deep);
    g.fillRect(x, y, width, height);

    auto thumbColour = isMouseOver ? BW::PurpleGlow : BW::Purple.withAlpha(0.6f);
    g.setColour(thumbColour);

    if (isScrollbarVertical)
        g.fillRoundedRectangle((float)x + 1, (float)thumbStartPosition,
                               (float)width - 2, (float)thumbSize, 3.0f);
    else
        g.fillRoundedRectangle((float)thumbStartPosition, (float)y + 1,
                               (float)thumbSize, (float)height - 2, 3.0f);
}
