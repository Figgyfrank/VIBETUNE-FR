#include "PluginEditor.h"

using namespace juce;

namespace vt
{
static Font boldFont (float h) { return Font (FontOptions (h, Font::bold)); }

static String midiToName (int midi)
{
    static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    return String (names[((midi % 12) + 12) % 12]) + String ((int) std::floor (midi / 12.0) - 1);
}

//==============================================================================
VibeLookAndFeel::VibeLookAndFeel()
{
    setColour (PopupMenu::backgroundColourId, Colour (0xff1d1048));
    setColour (PopupMenu::textColourId, Colours::white);
    setColour (PopupMenu::highlightedBackgroundColourId, col::magenta);
    setColour (PopupMenu::highlightedTextColourId, Colours::white);
    setColour (ComboBox::textColourId, Colours::white);
    setColour (Label::textColourId, Colours::white);
    setColour (TextButton::textColourOffId, Colours::white);
    setColour (TextButton::textColourOnId, Colours::white);
    setColour (TooltipWindow::backgroundColourId, Colour (0xff0e0726));
    setColour (TooltipWindow::textColourId, Colours::white);
    setColour (TooltipWindow::outlineColourId, col::cyan);
}

Font VibeLookAndFeel::getLabelFont (Label& l)         { return boldFont (jmax (10.0f, l.getHeight() * 0.62f)); }
Font VibeLookAndFeel::getComboBoxFont (ComboBox& b)   { return boldFont (jmax (11.0f, b.getHeight() * 0.44f)); }
Font VibeLookAndFeel::getPopupMenuFont()              { return Font (FontOptions (16.0f)); }
Font VibeLookAndFeel::getTextButtonFont (TextButton&, int h) { return boldFont ((float) h * 0.55f); }

void VibeLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h,
                                        float pos, float start, float end, Slider& s)
{
    auto bounds = Rectangle<int> (x, y, w, h).toFloat().reduced (jmax (2.0f, (float) w * 0.06f));
    const float radius = jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto c = bounds.getCentre();
    const float lw = radius * 0.17f;
    const float arcR = radius - lw * 0.5f;
    const auto accent = s.findColour (Slider::rotarySliderFillColourId);
    const float angle = start + pos * (end - start);
    const bool bipolar = s.getProperties().contains ("bipolar");
    const float from = bipolar ? start + 0.5f * (end - start) : start;

    Path track;
    track.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, start, end, true);
    g.setColour (Colours::white.withAlpha (0.10f));
    g.strokePath (track, PathStrokeType (lw, PathStrokeType::curved, PathStrokeType::rounded));

    if (std::abs (angle - from) > 0.002f)
    {
        Path v;
        v.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, jmin (from, angle), jmax (from, angle), true);
        g.setColour (accent.withAlpha (0.28f));
        g.strokePath (v, PathStrokeType (lw * 1.9f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (accent);
        g.strokePath (v, PathStrokeType (lw, PathStrokeType::curved, PathStrokeType::rounded));
    }

    const float bodyR = arcR - lw * 1.1f;
    g.setGradientFill (ColourGradient (Colour (0xff3d2b80), c.x, c.y - bodyR,
                                       Colour (0xff140a36), c.x, c.y + bodyR, false));
    g.fillEllipse (c.x - bodyR, c.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);
    g.setColour (accent.withAlpha (0.55f));
    g.drawEllipse (c.x - bodyR, c.y - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.2f);

    Path pointer;
    pointer.addRoundedRectangle (-lw * 0.28f, -bodyR * 0.92f, lw * 0.56f, bodyR * 0.46f, lw * 0.28f);
    g.setColour (Colours::white);
    g.fillPath (pointer, AffineTransform::rotation (angle).translated (c.x, c.y));
}

void VibeLookAndFeel::drawComboBox (Graphics& g, int w, int h, bool, int, int, int, int, ComboBox& box)
{
    auto r = Rectangle<float> (0.0f, 0.0f, (float) w, (float) h).reduced (1.0f);
    const float cr = (float) h * 0.3f;
    g.setColour (Colours::white.withAlpha (0.09f));
    g.fillRoundedRectangle (r, cr);
    g.setColour (box.findColour (ComboBox::outlineColourId).withAlpha (0.9f));
    g.drawRoundedRectangle (r, cr, 1.5f);

    const float ax = (float) w - (float) h * 0.5f, ay = (float) h * 0.5f, sz = (float) h * 0.13f;
    Path p;
    p.addTriangle (ax - sz, ay - sz * 0.6f, ax + sz, ay - sz * 0.6f, ax, ay + sz * 0.8f);
    g.setColour (Colours::white);
    g.fillPath (p);
}

void VibeLookAndFeel::positionComboBoxText (ComboBox& box, Label& label)
{
    label.setBounds (10, 1, box.getWidth() - box.getHeight() - 8, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
    label.setJustificationType (Justification::centredLeft);
}

void VibeLookAndFeel::drawToggleButton (Graphics& g, ToggleButton& b, bool highlighted, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = b.getToggleState();
    const auto accent = col::lime;
    const float cr = r.getHeight() * 0.5f;

    g.setColour (on ? accent.withAlpha (0.22f) : Colours::white.withAlpha (highlighted ? 0.12f : 0.07f));
    g.fillRoundedRectangle (r, cr);
    g.setColour (on ? accent : Colours::white.withAlpha (0.3f));
    g.drawRoundedRectangle (r, cr, 1.6f);

    const float d = r.getHeight() * 0.36f;
    const float lx = r.getX() + r.getHeight() * 0.5f, ly = r.getCentreY();
    if (on)
    {
        g.setColour (accent.withAlpha (0.3f));
        g.fillEllipse (lx - d, ly - d, d * 2.0f, d * 2.0f);
    }
    g.setColour (on ? accent : Colours::grey);
    g.fillEllipse (lx - d * 0.5f, ly - d * 0.5f, d, d);

    g.setFont (boldFont (r.getHeight() * 0.4f));
    g.setColour (Colours::white);
    g.drawText (b.getButtonText(), r.withTrimmedLeft (r.getHeight() * 0.95f), Justification::centredLeft);
}

void VibeLookAndFeel::drawButtonBackground (Graphics& g, Button& b, const Colour&, bool highlighted, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (Colours::white.withAlpha (down ? 0.25f : highlighted ? 0.16f : 0.08f));
    g.fillRoundedRectangle (r, r.getHeight() * 0.3f);
    g.setColour (Colours::white.withAlpha (0.25f));
    g.drawRoundedRectangle (r, r.getHeight() * 0.3f, 1.2f);
}

//==============================================================================
PitchDisplay::PitchDisplay (VibeTuneAudioProcessor& p) : proc (p)
{
    setTooltip ("Live pitch display. Left: the note you are singing. Right: the note it snaps to. "
                "The bar shows how far off you are. Lit keys are the notes allowed by your Key and Scale.");
    startTimerHz (30);
}

void PitchDisplay::timerCallback()
{
    detected = proc.getDetectedMidi();
    target = proc.getTargetMidi();
    const float devNow = (detected >= 0.0f && target >= 0.0f) ? (detected - target) * 100.0f : 0.0f;
    dispDev += 0.4f * (devNow - dispDev);

    allowed = VibeTuneAudioProcessor::buildAllowed (
        roundToInt (proc.apvts.getRawParameterValue ("key")->load()),
        roundToInt (proc.apvts.getRawParameterValue ("scale")->load()));
    repaint();
}

void PitchDisplay::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const float sc = r.getHeight() / 236.0f;
    auto S = [sc] (float v) { return v * sc; };

    g.setColour (Colours::white.withAlpha (0.06f));
    g.fillRoundedRectangle (r, S (18));
    g.setColour (Colours::white.withAlpha (0.12f));
    g.drawRoundedRectangle (r.reduced (0.5f), S (18), 1.2f);

    const bool voiced = detected >= 0.0f && target >= 0.0f;

    g.setFont (boldFont (S (11)));
    g.setColour (Colours::white.withAlpha (0.55f));
    g.drawText ("YOU SING", Rectangle<float> (S (24), S (14), S (160), S (16)), Justification::centredLeft);
    g.drawText ("SNAPS TO", Rectangle<float> (S (262), S (14), S (160), S (16)), Justification::centredLeft);
    g.drawText ("OFF BY", Rectangle<float> (r.getWidth() - S (116), S (14), S (92), S (16)), Justification::centredRight);

    g.setFont (boldFont (S (74)));
    g.setColour (voiced ? col::cyan : Colours::white.withAlpha (0.22f));
    g.drawText (voiced ? midiToName ((int) std::lround (detected)) : String ("--"),
                Rectangle<float> (S (24), S (30), S (170), S (88)), Justification::centredLeft);

    g.setColour (voiced ? col::magenta : Colours::white.withAlpha (0.22f));
    g.drawText (voiced ? midiToName ((int) std::lround (target)) : String ("--"),
                Rectangle<float> (S (262), S (30), S (170), S (88)), Justification::centredLeft);

    g.setColour (Colours::white.withAlpha (voiced ? 0.6f : 0.2f));
    g.drawArrow (Line<float> (S (196), S (76), S (250), S (76)), S (3), S (14), S (12));

    g.setFont (boldFont (S (26)));
    g.setColour (voiced ? Colours::white : Colours::white.withAlpha (0.22f));
    g.drawText (voiced ? (dispDev > 0 ? "+" : "") + String (roundToInt (dispDev)) + " ct" : String ("--"),
                Rectangle<float> (r.getWidth() - S (140), S (50), S (116), S (40)), Justification::centredRight);

    // cents bar
    Rectangle<float> bar (S (24), S (136), r.getWidth() - S (48), S (14));
    g.setColour (Colours::white.withAlpha (0.08f));
    g.fillRoundedRectangle (bar, S (7));

    const float cx = bar.getCentreX();
    const float dev = jlimit (-50.0f, 50.0f, voiced ? dispDev : 0.0f);
    const float px = cx + dev / 50.0f * (bar.getWidth() * 0.5f - S (7));
    if (voiced)
    {
        g.setColour (col::lime.interpolatedWith (col::magenta, std::abs (dev) / 50.0f));
        g.fillRoundedRectangle (Rectangle<float> (jmin (cx, px), bar.getY(), std::abs (px - cx), bar.getHeight()), S (7));
        g.setColour (Colours::white);
        g.fillEllipse (px - S (8), bar.getCentreY() - S (8), S (16), S (16));
    }
    g.setColour (Colours::white.withAlpha (0.6f));
    g.fillRect (cx - 1.0f, bar.getY() - S (4), 2.0f, bar.getHeight() + S (8));

    g.setFont (boldFont (S (10)));
    g.setColour (Colours::white.withAlpha (0.45f));
    g.drawText ("FLAT",    Rectangle<float> (bar.getX(), S (153), S (80), S (14)), Justification::centredLeft);
    g.drawText ("IN TUNE", Rectangle<float> (cx - S (40), S (153), S (80), S (14)), Justification::centred);
    g.drawText ("SHARP",   Rectangle<float> (bar.getRight() - S (80), S (153), S (80), S (14)), Justification::centredRight);

    // scale keys
    const float gap = S (6);
    const float chipW = (bar.getWidth() - gap * 11.0f) / 12.0f;
    const auto names = VibeTuneAudioProcessor::noteNames();
    const int detectedPc = voiced ? (((int) std::lround (detected) % 12) + 12) % 12 : -1;
    const int targetPc = voiced ? (((int) std::lround (target) % 12) + 12) % 12 : -1;

    for (int i = 0; i < 12; ++i)
    {
        Rectangle<float> chip (bar.getX() + (float) i * (chipW + gap), S (180), chipW, S (40));
        const bool on = allowed[(size_t) i];

        g.setColour (on ? col::violet.interpolatedWith (col::magenta, (float) i / 11.0f).withAlpha (0.85f)
                        : Colours::white.withAlpha (0.05f));
        g.fillRoundedRectangle (chip, S (9));

        if (i == targetPc)
        {
            g.setColour (Colours::white.withAlpha (0.25f));
            g.fillRoundedRectangle (chip, S (9));
            g.setColour (Colours::white);
            g.drawRoundedRectangle (chip, S (9), 2.4f);
        }

        g.setFont (boldFont (S (14)));
        g.setColour (on ? Colours::white : Colours::white.withAlpha (0.3f));
        g.drawText (names[i], chip, Justification::centred);

        if (i == detectedPc)
        {
            g.setColour (col::cyan);
            g.fillEllipse (chip.getCentreX() - S (4), S (168), S (8), S (8));
        }
    }
}

//==============================================================================
} // namespace vt

VibeTuneAudioProcessorEditor::VibeTuneAudioProcessorEditor (VibeTuneAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p), display (p)
{
    setLookAndFeel (&lnf);

    // ---- presets ----
    for (int i = 0; i < vt::kNumPresets; ++i)
        presetBox.addItem (vt::kPresets[i].name, i + 1);
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, dontSendNotification);
    presetBox.onChange = [this] { proc.setCurrentProgram (presetBox.getSelectedId() - 1); };
    presetBox.setColour (ComboBox::outlineColourId, vt::col::magenta);
    presetBox.setTooltip ("20 trap presets. Each one sets scale, retune speed, amount, humanize, transpose, "
                          "fine tune, mix and output. It never changes your Key or Low Latency.");
    prevButton.onClick = [this] { stepPreset (-1); };
    nextButton.onClick = [this] { stepPreset (1); };
    prevButton.setTooltip ("Previous preset");
    nextButton.setTooltip ("Next preset");

    // ---- low latency ----
    lowLatButton.setTooltip ("Low Latency mode. Cuts the processing delay from about 14.5 ms to about 8.5 ms "
                             "so it feels better while recording. Voices below roughly 120 Hz pass through "
                             "untuned in this mode. Switch it before you record, because it changes the "
                             "latency Ableton compensates for.");
    latencyLabel.setJustificationType (Justification::centred);
    latencyLabel.setColour (Label::textColourId, Colours::white.withAlpha (0.6f));

    // ---- key / scale ----
    keyBox.addItemList (VibeTuneAudioProcessor::noteNames(), 1);
    scaleBox.addItemList (VibeTuneAudioProcessor::scaleNames(), 1);
    keyBox.setColour (ComboBox::outlineColourId, vt::col::cyan);
    scaleBox.setColour (ComboBox::outlineColourId, vt::col::cyan);
    keyBox.setTooltip ("Key. The root note of your song. Match it to the key of your beat.");
    scaleBox.setTooltip ("Scale. The notes your voice is allowed to land on. Minor and Pentatonic Minor are "
                         "the usual trap picks. Chromatic snaps to the nearest semitone.");
    for (auto* l : { &keyCaption, &scaleCaption })
        l->setColour (Label::textColourId, Colours::white.withAlpha (0.55f));
    keyCaption.setText ("KEY", dontSendNotification);
    scaleCaption.setText ("SCALE", dontSendNotification);

    // ---- knobs ----
    setupKnob (speed,     "RETUNE SPEED", vt::col::cyan,
               "Retune Speed. How fast the voice snaps to the note. 0 ms = instant hard-tune robot sound, "
               "10 to 30 ms = smooth melodic trap, 100+ ms = natural and subtle. Double-click to reset.", 20.0, false);
    setupKnob (amount,    "AMOUNT", vt::col::magenta,
               "Amount. How much of the correction is applied. 100% puts the voice right on the note, "
               "lower values keep more of your original pitch. Double-click to reset.", 100.0, false);
    setupKnob (humanize,  "HUMANIZE", vt::col::lime,
               "Humanize. Leaves small natural drift and vibrato alone (up to 40 cents) and only corrects "
               "bigger misses. Keep it at 0% for the hardest tune. Double-click to reset.", 10.0, false);
    setupKnob (transpose, "TRANSPOSE", vt::col::orange,
               "Transpose the tuned voice up or down in semitones (plus or minus 12). Double-click to reset.", 0.0, true);
    setupKnob (fine,      "FINE TUNE", vt::col::violet,
               "Fine Tune in cents. Pair it with Mix around 50% for a quick detuned vocal double. "
               "Double-click to reset.", 0.0, true);
    setupKnob (mix,       "MIX", vt::col::pink,
               "Mix. Blend between your dry vocal (0%) and the tuned vocal (100%). The dry signal is "
               "delay-matched so it never phases. Double-click to reset.", 100.0, false);
    setupKnob (gain,      "OUTPUT", vt::col::blue,
               "Output level, plus or minus 12 dB. Double-click to reset.", 0.0, true);

    addAndMakeVisible (display);
    for (auto* c : { (Component*) &presetBox, (Component*) &keyBox, (Component*) &scaleBox,
                     (Component*) &prevButton, (Component*) &nextButton, (Component*) &lowLatButton,
                     (Component*) &latencyLabel, (Component*) &keyCaption, (Component*) &scaleCaption })
        addAndMakeVisible (c);

    // ---- attachments ----
    auto& v = proc.apvts;
    speedAtt     = std::make_unique<SliderAtt> (v, "speed",     speed.slider);
    amountAtt    = std::make_unique<SliderAtt> (v, "amount",    amount.slider);
    humanizeAtt  = std::make_unique<SliderAtt> (v, "humanize",  humanize.slider);
    transposeAtt = std::make_unique<SliderAtt> (v, "transpose", transpose.slider);
    fineAtt      = std::make_unique<SliderAtt> (v, "fine",      fine.slider);
    mixAtt       = std::make_unique<SliderAtt> (v, "mix",       mix.slider);
    gainAtt      = std::make_unique<SliderAtt> (v, "gain",      gain.slider);
    keyAtt       = std::make_unique<ComboAtt>  (v, "key",       keyBox);
    scaleAtt     = std::make_unique<ComboAtt>  (v, "scale",     scaleBox);
    lowLatAtt    = std::make_unique<ButtonAtt> (v, "lowlat",    lowLatButton);

    // value read-outs (set after the attachments, which install their own formatters)
    auto fmt = [] (Knob& k, std::function<String (double)> f)
    {
        k.slider.textFromValueFunction = std::move (f);
        k.slider.valueFromTextFunction = [] (const String& t) { return t.retainCharacters ("+-.0123456789").getDoubleValue(); };
        k.slider.updateText();
    };
    auto sign = [] (double x) { return String (x > 0.0 ? "+" : ""); };

    fmt (speed,     [] (double x) { return String (x, x < 10.0 ? 1 : 0) + " ms"; });
    fmt (amount,    [] (double x) { return String (roundToInt (x)) + " %"; });
    fmt (humanize,  [] (double x) { return String (roundToInt (x)) + " %"; });
    fmt (transpose, [sign] (double x) { return sign (x) + String (roundToInt (x)) + " st"; });
    fmt (fine,      [sign] (double x) { return sign (x) + String (roundToInt (x)) + " ct"; });
    fmt (mix,       [] (double x) { return String (roundToInt (x)) + " %"; });
    fmt (gain,      [sign] (double x) { return sign (x) + String (x, 1) + " dB"; });

    // ---- resizable, fixed aspect ratio ----
    setResizable (true, true);
    setResizeLimits (630, 406, 1800, 1161);
    getConstrainer()->setFixedAspectRatio (900.0 / 580.0);
    const int w = jlimit (630, 1800, proc.editorWidth);
    setSize (w, roundToInt (w * 580.0 / 900.0));

    startTimerHz (8);
}

VibeTuneAudioProcessorEditor::~VibeTuneAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void VibeTuneAudioProcessorEditor::setupKnob (Knob& k, const String& title, Colour colour,
                                              const String& tip, double defaultValue, bool bipolar)
{
    k.slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    k.slider.setRotaryParameters (MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
    k.slider.setTextBoxStyle (Slider::TextBoxBelow, false, 90, 22);
    k.slider.setColour (Slider::rotarySliderFillColourId, colour);
    k.slider.setColour (Slider::textBoxTextColourId, Colours::white);
    k.slider.setColour (Slider::textBoxBackgroundColourId, Colours::transparentBlack);
    k.slider.setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    k.slider.setDoubleClickReturnValue (true, defaultValue);
    k.slider.setTooltip (tip);
    if (bipolar)
        k.slider.getProperties().set ("bipolar", true);

    k.label.setText (title, dontSendNotification);
    k.label.setJustificationType (Justification::centred);
    k.label.setColour (Label::textColourId, colour);
    k.label.setInterceptsMouseClicks (false, false);

    addAndMakeVisible (k.slider);
    addAndMakeVisible (k.label);
}

void VibeTuneAudioProcessorEditor::stepPreset (int delta)
{
    const int idx = (proc.getCurrentProgram() + delta + vt::kNumPresets) % vt::kNumPresets;
    presetBox.setSelectedId (idx + 1, sendNotification);
}

void VibeTuneAudioProcessorEditor::timerCallback()
{
    if (presetBox.getSelectedId() != proc.getCurrentProgram() + 1)
        presetBox.setSelectedId (proc.getCurrentProgram() + 1, dontSendNotification);

    const double sr = proc.getSampleRate();
    if (sr > 0.0)
    {
        const bool ll = lowLatButton.getToggleState();
        latencyLabel.setText ("delay " + String (1000.0 * vt::AutoTuneEngine::latencyFor (sr, ll) / sr, 1) + " ms",
                              dontSendNotification);
    }
}

//==============================================================================
void VibeTuneAudioProcessorEditor::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const float s = r.getWidth() / 900.0f;

    g.setGradientFill (ColourGradient (Colour (0xff1b0a4a), 0.0f, 0.0f,
                                       Colour (0xff062c5c), r.getRight(), r.getBottom(), false));
    g.fillAll();

    g.setGradientFill (ColourGradient (vt::col::magenta.withAlpha (0.30f), r.getRight() * 0.88f, 0.0f,
                                       Colours::transparentBlack, r.getRight() * 0.88f - 460.0f * s, 280.0f * s, true));
    g.fillAll();
    g.setGradientFill (ColourGradient (vt::col::cyan.withAlpha (0.18f), r.getWidth() * 0.08f, r.getBottom(),
                                       Colours::transparentBlack, r.getWidth() * 0.08f + 420.0f * s, r.getBottom() - 220.0f * s, true));
    g.fillAll();

    auto card = [&] (Rectangle<float> rc)
    {
        g.setColour (Colours::white.withAlpha (0.06f));
        g.fillRoundedRectangle (rc, 18.0f * s);
        g.setColour (Colours::white.withAlpha (0.12f));
        g.drawRoundedRectangle (rc.reduced (0.5f), 18.0f * s, 1.2f);
    };
    card (Rectangle<float> (580.0f * s, 84.0f * s, 296.0f * s, 236.0f * s));
    card (Rectangle<float> (24.0f * s, 336.0f * s, 852.0f * s, 228.0f * s));

    // logo
    const auto big = vt::boldFont (32.0f * s);
    GlyphArrangement ga;
    ga.addLineOfText (big, "VIBE", 0.0f, 0.0f);
    const float vibeW = ga.getBoundingBox (0, -1, true).getWidth();

    g.setFont (big);
    g.setColour (Colours::white);
    g.drawText ("VIBE", Rectangle<float> (24.0f * s, 8.0f * s, vibeW + 6.0f * s, 40.0f * s), Justification::centredLeft);
    g.setColour (vt::col::magenta);
    g.drawText ("TUNE", Rectangle<float> (24.0f * s + vibeW, 8.0f * s, 120.0f * s, 40.0f * s), Justification::centredLeft);
    g.setFont (vt::boldFont (11.0f * s));
    g.setColour (vt::col::cyan);
    g.drawText ("AUTO-PITCH  /  TRAP EDITION", Rectangle<float> (26.0f * s, 46.0f * s, 260.0f * s, 16.0f * s),
                Justification::centredLeft);

    // right-card tip
    g.setFont (Font (FontOptions (13.0f * s)));
    g.setColour (Colours::white.withAlpha (0.75f));
    g.drawFittedText ("Pick the key of your beat, then choose a preset.\n\n"
                      "Retune Speed at 0 ms = hard-tune robot sound.\n"
                      "10 to 30 ms = smooth melodic. 100+ ms = natural.\n\n"
                      "Hover over anything for help.",
                      Rectangle<float> (598.0f * s, 178.0f * s, 262.0f * s, 134.0f * s).toNearestInt(),
                      Justification::topLeft, 8);
}

void VibeTuneAudioProcessorEditor::resized()
{
    const float s = getWidth() / 900.0f;
    auto R = [s] (float x, float y, float w, float h)
    {
        return Rectangle<int> (roundToInt (x * s), roundToInt (y * s), roundToInt (w * s), roundToInt (h * s));
    };

    prevButton.setBounds (R (318, 18, 36, 34));
    presetBox.setBounds  (R (358, 18, 230, 34));
    nextButton.setBounds (R (592, 18, 36, 34));
    lowLatButton.setBounds (R (690, 18, 186, 34));
    latencyLabel.setBounds (R (690, 54, 186, 20));

    display.setBounds (R (24, 84, 536, 236));

    keyCaption.setBounds   (R (598, 98, 80, 18));
    scaleCaption.setBounds (R (696, 98, 80, 18));
    keyBox.setBounds   (R (598, 120, 88, 38));
    scaleBox.setBounds (R (696, 120, 168, 38));

    const float cellW = 852.0f / 7.0f;
    Knob* knobs[] = { &speed, &amount, &humanize, &transpose, &fine, &mix, &gain };
    for (int i = 0; i < 7; ++i)
    {
        const float x = 24.0f + cellW * (float) i;
        knobs[i]->label.setBounds  (R (x + 4, 348, cellW - 8, 20));
        knobs[i]->slider.setTextBoxStyle (Slider::TextBoxBelow, false, roundToInt (96.0f * s), roundToInt (22.0f * s));
        knobs[i]->slider.setBounds (R (x + 6, 370, cellW - 12, 188));
    }

    proc.editorWidth = getWidth();
}
