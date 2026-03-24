#include "ADSRDisplay.h"

ADSRDisplay::ADSRDisplay() {}

ADSRDisplay::~ADSRDisplay()
{
    if (apvtsPtr != nullptr)
    {
        apvtsPtr->removeParameterListener(attackID, this);
        apvtsPtr->removeParameterListener(decayID, this);
        apvtsPtr->removeParameterListener(sustainID, this);
        apvtsPtr->removeParameterListener(releaseID, this);
    }
}

void ADSRDisplay::attachToAPVTS(juce::AudioProcessorValueTreeState& apvts,
                                const juce::String& aID, const juce::String& dID,
                                const juce::String& sID, const juce::String& rID)
{
    apvtsPtr = &apvts;
    attackID = aID; decayID = dID; sustainID = sID; releaseID = rID;

    attack  = *apvts.getRawParameterValue(aID);
    decay   = *apvts.getRawParameterValue(dID);
    sustain = *apvts.getRawParameterValue(sID);
    release = *apvts.getRawParameterValue(rID);

    apvts.addParameterListener(aID, this);
    apvts.addParameterListener(dID, this);
    apvts.addParameterListener(sID, this);
    apvts.addParameterListener(rID, this);
}

void ADSRDisplay::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == attackID)        attack = newValue;
    else if (parameterID == decayID)    decay = newValue;
    else if (parameterID == sustainID)  sustain = newValue;
    else if (parameterID == releaseID)  release = newValue;

    // Safe repaint from any thread
    juce::MessageManager::callAsync([safeThis = juce::Component::SafePointer<ADSRDisplay>(this)]()
    {
        if (safeThis != nullptr) safeThis->repaint();
    });
}

// =============================================================================
// Coordinate helpers
// =============================================================================

juce::Point<float> ADSRDisplay::getAttackPoint() const
{
    float x = drawArea.getX() + (attack / maxTime) * drawArea.getWidth() * 0.3f;
    float y = drawArea.getY();
    return { x, y };
}

juce::Point<float> ADSRDisplay::getDecayPoint() const
{
    auto ap = getAttackPoint();
    float dx = (decay / maxTime) * drawArea.getWidth() * 0.3f;
    float y = drawArea.getY() + (1.0f - sustain) * drawArea.getHeight();
    return { ap.x + dx, y };
}

juce::Point<float> ADSRDisplay::getSustainPoint() const
{
    auto dp = getDecayPoint();
    float sustainWidth = drawArea.getWidth() * 0.2f;
    return { dp.x + sustainWidth, dp.y };
}

juce::Point<float> ADSRDisplay::getReleasePoint() const
{
    auto sp = getSustainPoint();
    float rx = (release / maxTime) * drawArea.getWidth() * 0.3f;
    return { sp.x + rx, drawArea.getBottom() };
}

// =============================================================================
// Paint
// =============================================================================

void ADSRDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(2.0f);
    drawArea = bounds.reduced(8.0f, 8.0f);

    // Background
    g.setColour(BW::Deep);
    g.fillRoundedRectangle(bounds, 6.0f);

    // Grid lines
    g.setColour(BW::Grey.withAlpha(0.15f));
    for (int i = 1; i < 4; ++i)
    {
        float y = drawArea.getY() + drawArea.getHeight() * (i / 4.0f);
        g.drawHorizontalLine(static_cast<int>(y), drawArea.getX(), drawArea.getRight());
    }

    // Compute points
    auto origin = juce::Point<float>(drawArea.getX(), drawArea.getBottom());
    auto ap = getAttackPoint();
    auto dp = getDecayPoint();
    auto sp = getSustainPoint();
    auto rp = getReleasePoint();

    // --- Envelope path ---
    juce::Path envPath;
    envPath.startNewSubPath(origin);
    envPath.lineTo(ap);       // Attack rise
    envPath.lineTo(dp);       // Decay fall
    envPath.lineTo(sp);       // Sustain hold
    envPath.lineTo(rp);       // Release fall

    // Filled gradient underneath
    juce::Path fillPath(envPath);
    fillPath.lineTo(rp.x, drawArea.getBottom());
    fillPath.lineTo(origin.x, drawArea.getBottom());
    fillPath.closeSubPath();

    g.setGradientFill(juce::ColourGradient(
        BW::Pink.withAlpha(0.25f), drawArea.getCentreX(), drawArea.getY(),
        BW::Purple.withAlpha(0.05f), drawArea.getCentreX(), drawArea.getBottom(), false));
    g.fillPath(fillPath);

    // Stroke
    g.setColour(BW::Pink);
    g.strokePath(envPath, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));

    // --- Handle dots ---
    auto drawHandle = [&](juce::Point<float> pt, bool active)
    {
        float r = active ? 6.0f : 4.0f;
        if (active)
        {
            g.setColour(BW::PinkGlow.withAlpha(0.4f));
            g.fillEllipse(pt.x - 8, pt.y - 8, 16, 16);
        }
        g.setColour(BW::Pink);
        g.fillEllipse(pt.x - r, pt.y - r, r * 2, r * 2);
    };

    drawHandle(ap, currentDrag == Attack);
    drawHandle(dp, currentDrag == Decay);
    drawHandle(sp, currentDrag == Sustain);
    drawHandle(rp, currentDrag == Release);

    // Border
    g.setColour(BW::GreyBorder);
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);
}

void ADSRDisplay::resized() {}

// =============================================================================
// Mouse interaction — drag handles to change ADSR
// =============================================================================

static float distSq(juce::Point<float> a, juce::Point<float> b)
{
    float dx = a.x - b.x, dy = a.y - b.y;
    return dx * dx + dy * dy;
}

void ADSRDisplay::mouseDown(const juce::MouseEvent& e)
{
    auto pos = e.position;
    float threshold = 15.0f * 15.0f;

    currentDrag = None;
    if (distSq(pos, getAttackPoint()) < threshold)       currentDrag = Attack;
    else if (distSq(pos, getDecayPoint()) < threshold)   currentDrag = Decay;
    else if (distSq(pos, getSustainPoint()) < threshold)  currentDrag = Sustain;
    else if (distSq(pos, getReleasePoint()) < threshold)  currentDrag = Release;

    repaint();
}

void ADSRDisplay::mouseDrag(const juce::MouseEvent& e)
{
    if (currentDrag == None || apvtsPtr == nullptr) return;

    auto pos = e.position;
    float normX = juce::jlimit(0.0f, 1.0f, (pos.x - drawArea.getX()) / drawArea.getWidth());
    float normY = juce::jlimit(0.0f, 1.0f, (pos.y - drawArea.getY()) / drawArea.getHeight());

    auto setParam = [&](const juce::String& id, float val)
    {
        if (auto* param = apvtsPtr->getParameter(id))
        {
            auto range = param->getNormalisableRange();
            float clamped = juce::jlimit(range.start, range.end, val);
            param->setValueNotifyingHost(range.convertTo0to1(clamped));
        }
    };

    switch (currentDrag)
    {
        case Attack:
            setParam(attackID, normX * maxTime * 0.3f / 0.3f);
            break;
        case Decay:
            setParam(decayID, normX * maxTime);
            setParam(sustainID, 1.0f - normY);
            break;
        case Sustain:
            setParam(sustainID, 1.0f - normY);
            break;
        case Release:
            setParam(releaseID, normX * maxTime);
            break;
        default: break;
    }
}

void ADSRDisplay::mouseUp(const juce::MouseEvent&)
{
    currentDrag = None;
    repaint();
}
