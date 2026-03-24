#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * BWColours — Billy Wonka brand colour palette.
 * Single source of truth for the entire UI colour system.
 */
namespace BW
{
    inline const juce::Colour Black       { 0xFF0A0A0A };
    inline const juce::Colour Deep        { 0xFF111118 };
    inline const juce::Colour PurpleDark  { 0xFF1E0A3C };
    inline const juce::Colour Purple      { 0xFF6B21A8 };
    inline const juce::Colour PurpleGlow  { 0xFF9333EA };
    inline const juce::Colour Pink        { 0xFFEC4899 };
    inline const juce::Colour PinkSoft    { 0xFFF9A8D4 };
    inline const juce::Colour White       { 0xFFF8F8FF };
    inline const juce::Colour Grey        { 0xFF3A3A4A };

    // Derived / transparency variants
    inline const juce::Colour PurpleGlass   { 0x301E0A3C };  // glassmorphism panel bg
    inline const juce::Colour GreyBorder    { 0x553A3A4A };  // subtle borders
    inline const juce::Colour PurpleBorder  { 0x446B21A8 };  // active borders
    inline const juce::Colour GlowShadow   { 0x559333EA };  // glow box-shadow
    inline const juce::Colour PinkGlow      { 0x55EC4899 };  // pink glow for active states
    inline const juce::Colour TextMuted     { 0xFF6A6A7A };  // muted label text
    inline const juce::Colour KnobFace      { 0xFF1A1A2E };  // knob centre fill
    inline const juce::Colour Green         { 0xFF22C55E };  // connection status
    inline const juce::Colour Red           { 0xFFEF4444 };  // danger/error
}
