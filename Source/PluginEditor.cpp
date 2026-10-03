#include "PluginEditor.h"

namespace colours
{
    const juce::Colour woodDark   { 0xff140a08 };
    const juce::Colour woodMid    { 0xff2b1611 };
    const juce::Colour woodLight  { 0xff4a2618 };
    const juce::Colour brass      { 0xffc99a4b };
    const juce::Colour brassDark  { 0xff6e4f22 };
    const juce::Colour saffron    { 0xffef9a2b };
    const juce::Colour ivory      { 0xfff3e6c8 };
    const juce::Colour copper     { 0xffb8693c };
    const juce::Colour skin       { 0xff3a2219 };
}

static juce::Font uiFont (float size, bool bold = false)
{
    return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
}

static juce::Font devanagariFont (float size)
{
   #if JUCE_WINDOWS
    return juce::Font (juce::FontOptions ("Nirmala UI", size, juce::Font::plain));
   #elif JUCE_MAC
    return juce::Font (juce::FontOptions ("Kohinoor Devanagari", size, juce::Font::plain));
   #else
    return juce::Font (juce::FontOptions (size, juce::Font::plain));
   #endif
}

// ============================================================ LookAndFeel
SarangiLookAndFeel::SarangiLookAndFeel()
{
    setColour (juce::ComboBox::textColourId, colours::ivory);
    setColour (juce::PopupMenu::backgroundColourId, colours::woodMid);
    setColour (juce::PopupMenu::textColourId, colours::ivory);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, colours::saffron.withAlpha (0.8f));
    setColour (juce::PopupMenu::highlightedTextColourId, colours::woodDark);
    setColour (juce::Slider::textBoxTextColourId, colours::ivory);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}

void SarangiLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                           float startAngle, float endAngle, juce::Slider&)
{
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (6.0f);
    const float size = juce::jmin (bounds.getWidth(), bounds.getHeight());
    auto r = juce::Rectangle<float> (size, size).withCentre (bounds.getCentre());
    const auto c = r.getCentre();
    const float radius = size * 0.5f;
    const float angle = startAngle + pos * (endAngle - startAngle);

    // track
    juce::Path track;
    track.addCentredArc (c.x, c.y, radius - 2.0f, radius - 2.0f, 0.0f, startAngle, endAngle, true);
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.strokePath (track, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // value arc
    juce::Path arc;
    arc.addCentredArc (c.x, c.y, radius - 2.0f, radius - 2.0f, 0.0f, startAngle, angle, true);
    g.setColour (colours::saffron.withAlpha (0.35f));
    g.strokePath (arc, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (colours::saffron);
    g.strokePath (arc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // brass body
    auto body = r.reduced (radius * 0.24f);
    g.setGradientFill (juce::ColourGradient (colours::brass.brighter (0.25f), body.getX(), body.getY(),
                                             colours::brassDark, body.getRight(), body.getBottom(), false));
    g.fillEllipse (body);
    g.setColour (colours::woodDark.withAlpha (0.8f));
    g.drawEllipse (body, 1.2f);

    // inner engraved ring
    auto inner = body.reduced (body.getWidth() * 0.16f);
    g.setColour (colours::brassDark.withAlpha (0.55f));
    g.drawEllipse (inner, 1.0f);

    // pointer: small ivory bead
    const float pr = body.getWidth() * 0.5f - 6.0f;
    const juce::Point<float> tip (c.x + pr * std::sin (angle), c.y - pr * std::cos (angle));
    g.setColour (colours::ivory);
    g.fillEllipse (juce::Rectangle<float> (6.0f, 6.0f).withCentre (tip));
}

void SarangiLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&)
{
    auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (1.0f);
    g.setColour (colours::woodDark.withAlpha (0.7f));
    g.fillRoundedRectangle (r, h * 0.5f);
    g.setColour (colours::brass.withAlpha (0.8f));
    g.drawRoundedRectangle (r, h * 0.5f, 1.2f);
}

juce::Font SarangiLookAndFeel::getComboBoxFont (juce::ComboBox&) { return uiFont (15.0f, true); }

void SarangiLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (box.getLocalBounds());
    label.setJustificationType (juce::Justification::centred);
    label.setFont (getComboBoxFont (box));
}

// ================================================================ Editor
SarangiEditor::SarangiEditor (SarangiProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);

    const char* ids[6]   = { "bow", "meend", "gamak", "taraf", "space", "volume" };
    const char* names[6] = { "BOW", "MEEND", "GAMAK", "TARAF", "SPACE", "VOLUME" };
    const char* hints[6] = { "pressure & grit", "glide", "vibrato", "sympathetic strings", "room", "level" };

    for (int i = 0; i < 6; ++i)
    {
        auto& k = knobs[(size_t) i];
        k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        k.slider.setPopupDisplayEnabled (true, true, this);
        k.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
        addAndMakeVisible (k.slider);
        k.attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, ids[i], k.slider);

        k.label.setText (names[i], juce::dontSendNotification);
        k.label.setFont (uiFont (13.0f, true));
        k.label.setColour (juce::Label::textColourId, colours::ivory);
        k.label.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (k.label);

        k.hint.setText (hints[i], juce::dontSendNotification);
        k.hint.setFont (uiFont (11.0f));
        k.hint.setColour (juce::Label::textColourId, colours::brass.withAlpha (0.75f));
        k.hint.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (k.hint);
    }

    saBox.addItemList ({ "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 1);
    saBox.setTooltip ("Tonic (Sa) that the sympathetic strings are tuned to");
    addAndMakeVisible (saBox);
    saAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "sa", saBox);

    saLabel.setText ("Sa =", juce::dontSendNotification);
    saLabel.setFont (uiFont (13.0f, true));
    saLabel.setColour (juce::Label::textColourId, colours::brass);
    saLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (saLabel);

    setSize (700, 420);
    startTimerHz (40);
}

SarangiEditor::~SarangiEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void SarangiEditor::resized()
{
    auto area = getLocalBounds();
    auto header = area.removeFromTop (78);
    saBox.setBounds (header.getRight() - 84, 26, 62, 28);
    saLabel.setBounds (header.getRight() - 140, 26, 54, 28);

    stringArea = area.removeFromTop (172).reduced (28, 8).toFloat();

    auto knobRow = area.reduced (18, 10);
    const int w = knobRow.getWidth() / 6;
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto col = knobRow.removeFromLeft (w);
        auto& k = knobs[i];
        k.hint.setBounds (col.removeFromBottom (16));
        k.label.setBounds (col.removeFromBottom (18));
        k.slider.setBounds (col.reduced (8, 2));
    }
}

void SarangiEditor::timerCallback()
{
    level = proc.uiLevel.load();
    hz = proc.uiHz.load();
    bowing = proc.uiBowing.load();

    phase += 0.85f;
    bowAlpha += ((bowing ? 1.0f : 0.0f) - bowAlpha) * 0.15f;
    if (bowing)
    {
        bowPos += 0.012f * bowDir;
        if (bowPos > 0.85f) { bowPos = 0.85f; bowDir = -1.0f; }
        if (bowPos < 0.15f) { bowPos = 0.15f; bowDir = 1.0f; }
    }
    repaint();
}

juce::String SarangiEditor::currentSwar() const
{
    if (level < 0.01f || hz <= 0.0f) return {};
    static const char* swar[12] = { "Sa", "re", "Re", "ga", "Ga", "ma", "Ma'", "Pa", "dha", "Dha", "ni", "Ni" };
    const int sa = (int) proc.apvts.getRawParameterValue ("sa")->load();
    const int midi = juce::roundToInt (69.0 + 12.0 * std::log2 (hz / 440.0));
    const int deg = ((midi - 60 - sa) % 12 + 12) % 12;
    return swar[deg];
}

void SarangiEditor::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();

    // wood background with grain
    g.setGradientFill (juce::ColourGradient (colours::woodLight, b.getCentreX(), 0.0f,
                                             colours::woodDark, b.getCentreX(), b.getBottom(), false));
    g.fillAll();
    juce::Random rnd (7);
    for (int i = 0; i < 70; ++i)
    {
        const float y = rnd.nextFloat() * b.getHeight();
        const float amp = 2.0f + rnd.nextFloat() * 6.0f;
        juce::Path grain;
        grain.startNewSubPath (0.0f, y);
        for (float x = 0.0f; x <= b.getWidth(); x += 20.0f)
            grain.lineTo (x, y + amp * std::sin (x * 0.012f + (float) i));
        g.setColour (juce::Colours::black.withAlpha (0.05f + rnd.nextFloat() * 0.06f));
        g.strokePath (grain, juce::PathStrokeType (0.6f + rnd.nextFloat()));
    }

    // header
    g.setColour (colours::brass);
    g.setFont (uiFont (30.0f, true));
    g.drawText ("S A R A N G I", 28, 18, 260, 40, juce::Justification::centredLeft);
    g.setColour (colours::saffron.withAlpha (0.9f));
    g.setFont (devanagariFont (26.0f));
    g.drawText (juce::String (juce::CharPointer_UTF8 ("\xe0\xa4\xb8\xe0\xa4\xbe\xe0\xa4\xb0\xe0\xa4\x82\xe0\xa4\x97\xe0\xa5\x80")),
                262, 16, 120, 44, juce::Justification::centredLeft);

    // swar readout (current note in sargam)
    const auto swar = currentSwar();
    g.setColour (colours::ivory.withAlpha (0.25f + 0.75f * juce::jlimit (0.0f, 1.0f, level * 4.0f)));
    g.setFont (uiFont (34.0f, true));
    g.drawText (swar.isEmpty() ? juce::String ("-") : swar, 400, 14, 140, 48, juce::Justification::centred);

    // divider with inlay dots
    g.setColour (colours::brass.withAlpha (0.5f));
    g.drawHorizontalLine (74, 28.0f, b.getWidth() - 28.0f);
    for (int i = 0; i < 9; ++i)
        g.fillEllipse (juce::Rectangle<float> (4.0f, 4.0f).withCentre ({ 28.0f + i * (b.getWidth() - 56.0f) / 8.0f, 74.5f }));

    drawStrings (g, stringArea);
    drawBow (g, stringArea);
}

void SarangiEditor::drawStrings (juce::Graphics& g, juce::Rectangle<float> area)
{
    // skin / fingerboard panel
    g.setColour (colours::skin.withAlpha (0.85f));
    g.fillRoundedRectangle (area, 14.0f);
    g.setColour (colours::brass.withAlpha (0.55f));
    g.drawRoundedRectangle (area, 14.0f, 1.3f);
    g.setColour (colours::brass.withAlpha (0.25f));
    g.drawRoundedRectangle (area.reduced (5.0f), 10.0f, 0.8f);

    const float x0 = area.getX() + 26.0f, x1 = area.getRight() - 26.0f;
    // bridge & nut
    g.setColour (colours::ivory.withAlpha (0.6f));
    g.fillRoundedRectangle (x0 - 6.0f, area.getY() + 14.0f, 4.0f, area.getHeight() - 28.0f, 2.0f);
    g.fillRoundedRectangle (x1 + 2.0f, area.getY() + 14.0f, 4.0f, area.getHeight() - 28.0f, 2.0f);

    const float taraf = proc.apvts.getRawParameterValue ("taraf")->load();
    const float lv = juce::jlimit (0.0f, 1.0f, level * 2.5f);

    // which main string is "being played" (top = highest)
    const int active = hz < 200.0f ? 3 : hz < 350.0f ? 2 : hz < 600.0f ? 1 : 0;

    auto drawString = [&] (float y, float thickness, juce::Colour col, float amp, float speed, float seed, float glow)
    {
        juce::Path p;
        const int segs = 64;
        for (int s = 0; s <= segs; ++s)
        {
            const float t = (float) s / (float) segs;
            const float x = x0 + t * (x1 - x0);
            const float disp = amp * std::sin (juce::MathConstants<float>::pi * t)
                               * std::sin (phase * speed + seed)
                             + amp * 0.35f * std::sin (juce::MathConstants<float>::pi * 2.0f * t)
                               * std::sin (phase * speed * 1.9f + seed * 2.0f);
            if (s == 0) p.startNewSubPath (x, y + disp); else p.lineTo (x, y + disp);
        }
        if (glow > 0.02f)
        {
            g.setColour (colours::saffron.withAlpha (0.25f * glow));
            g.strokePath (p, juce::PathStrokeType (thickness + 6.0f * glow));
        }
        g.setColour (col);
        g.strokePath (p, juce::PathStrokeType (thickness));
    };

    // sympathetic (taraf) strings: thin copper, lower half
    const int numSym = 11;
    const float symTop = area.getY() + area.getHeight() * 0.58f;
    const float symGap = (area.getBottom() - 16.0f - symTop) / (float) (numSym - 1);
    for (int i = 0; i < numSym; ++i)
    {
        const float amp = lv * taraf * (1.5f + 1.5f * std::sin ((float) i * 1.7f));
        drawString (symTop + i * symGap, 0.8f, colours::copper.withAlpha (0.55f + 0.4f * taraf),
                    amp, 0.55f + 0.07f * i, (float) i * 0.9f, lv * taraf * 0.5f);
    }

    // four main gut strings
    const float mainTop = area.getY() + 26.0f;
    const float mainGap = (area.getHeight() * 0.45f - 26.0f) / 3.0f;
    for (int i = 0; i < 4; ++i)
    {
        const bool isActive = (i == active);
        const float amp = lv * (isActive ? 9.0f : 1.6f);
        drawString (mainTop + i * mainGap, 1.6f + 0.5f * i, colours::ivory.withAlpha (isActive ? 0.95f : 0.7f),
                    amp, 1.4f + 0.2f * i, (float) i, isActive ? lv : 0.0f);
    }
}

void SarangiEditor::drawBow (juce::Graphics& g, juce::Rectangle<float> area)
{
    if (bowAlpha < 0.02f) return;

    juce::Graphics::ScopedSaveState save (g);
    g.reduceClipRegion (area.expanded (0.0f, 2.0f).toNearestInt());

    // The bow crosses the strings near the bridge and slides along its length.
    const float x = area.getX() + area.getWidth() * 0.78f;
    const float len = area.getHeight() * 1.9f;
    const float yTop = area.getY() - len * bowPos * 0.55f;
    const float yBot = yTop + len;
    const float tilt = 22.0f;

    juce::Path hair;
    hair.startNewSubPath (x - tilt * 0.5f, yTop);
    hair.lineTo (x + tilt * 0.5f, yBot);
    g.setColour (colours::ivory.withAlpha (0.85f * bowAlpha));
    g.strokePath (hair, juce::PathStrokeType (2.6f));

    juce::Path stick;
    stick.startNewSubPath (x - tilt * 0.5f, yTop);
    stick.quadraticTo (x + 26.0f, (yTop + yBot) * 0.5f, x + tilt * 0.5f, yBot);
    g.setColour (colours::woodLight.brighter (0.4f).withAlpha (0.95f * bowAlpha));
    g.strokePath (stick, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (colours::brass.withAlpha (0.8f * bowAlpha));
    g.strokePath (stick, juce::PathStrokeType (1.0f));
}
