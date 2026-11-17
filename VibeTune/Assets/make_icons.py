"""Generates VibeTune icons: vibetune.ico (desktop/app), setup.ico (installer), PNGs for JUCE.
Run:  python make_icons.py     (needs: pip install pillow numpy)"""
import numpy as np
from PIL import Image, ImageDraw, ImageFilter
import math, os

S = 1024
HERE = os.path.dirname(os.path.abspath(__file__))


def gradient():
    ys, xs = np.mgrid[0:S, 0:S]
    t = (xs + ys) / (2.0 * (S - 1))
    stops = [(0.0, (255, 46, 147)), (0.5, (123, 47, 247)), (1.0, (0, 229, 255))]
    out = np.zeros((S, S, 3), dtype=np.float32)
    for (t0, c0), (t1, c1) in zip(stops[:-1], stops[1:]):
        m = (t >= t0) & (t <= t1)
        k = ((t - t0) / (t1 - t0))[m][:, None]
        out[m] = np.array(c0) * (1 - k) + np.array(c1) * k
    img = Image.fromarray(out.astype(np.uint8), "RGB").convert("RGBA")
    mask = Image.new("L", (S, S), 0)
    ImageDraw.Draw(mask).rounded_rectangle((0, 0, S - 1, S - 1), radius=int(S * 0.22), fill=255)
    img.putalpha(mask)
    return img


def plateau(x):
    # stepped "snapped" pitch line (x in 0..1) -> y
    pts = [(0.16, .66), (0.35, .66), (0.40, .49), (0.57, .49), (0.62, .32), (0.84, .32)]
    if x <= pts[0][0]:
        return pts[0][1]
    for (x0, y0), (x1, y1) in zip(pts[:-1], pts[1:]):
        if x0 <= x <= x1:
            k = (x - x0) / (x1 - x0)
            k = k * k * (3 - 2 * k)
            return y0 + (y1 - y0) * k
    return pts[-1][1]


def make_base():
    img = gradient()
    glyph = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    d = ImageDraw.Draw(glyph)

    # faint staff lines
    for y in (.32, .49, .66):
        d.line([(S * .12, S * y), (S * .88, S * y)], fill=(255, 255, 255, 70), width=10)

    # wobbly "raw voice"
    pts = []
    for i in range(0, 241):
        x = .14 + .72 * i / 240.0
        y = plateau(x) + .045 * math.sin(2 * math.pi * x * 9) + .02 * math.sin(2 * math.pi * x * 3.1 + 1)
        pts.append((S * x, S * y))
    d.line(pts, fill=(255, 255, 255, 110), width=22, joint="curve")

    # tuned line (crisp, bright) with glow
    tuned = [(S * (.14 + .72 * i / 240.0), S * plateau(.14 + .72 * i / 240.0)) for i in range(241)]
    glow = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    ImageDraw.Draw(glow).line(tuned, fill=(255, 255, 255, 255), width=70, joint="curve")
    glow = glow.filter(ImageFilter.GaussianBlur(26))
    img = Image.alpha_composite(img, glow)
    glyph_d = ImageDraw.Draw(glyph)
    glyph_d.line(tuned, fill=(255, 255, 255, 255), width=44, joint="curve")
    for x, y in (tuned[0], tuned[-1]):
        glyph_d.ellipse((x - 22, y - 22, x + 22, y + 22), fill=(255, 255, 255, 255))
    for xs_, ys_ in ((.38, .49), (.60, .32)):
        r = 30
        cx, cy = S * xs_ + S * .02, S * ys_
        glyph_d.ellipse((cx - r, cy - r, cx + r, cy + r), fill=(20, 8, 60, 255))
        glyph_d.ellipse((cx - r + 12, cy - r + 12, cx + r - 12, cy + r - 12), fill=(0, 229, 255, 255))

    img = Image.alpha_composite(img, glyph)
    # keep inside the rounded square
    mask = Image.new("L", (S, S), 0)
    ImageDraw.Draw(mask).rounded_rectangle((0, 0, S - 1, S - 1), radius=int(S * 0.22), fill=255)
    img.putalpha(mask)
    return img


def setup_version(base):
    img = base.copy()
    d = ImageDraw.Draw(img)
    cx, cy, r = int(S * .73), int(S * .73), int(S * .25)
    shadow = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    ImageDraw.Draw(shadow).ellipse((cx - r, cy - r + 16, cx + r, cy + r + 16), fill=(0, 0, 0, 120))
    img = Image.alpha_composite(img, shadow.filter(ImageFilter.GaussianBlur(18)))
    d = ImageDraw.Draw(img)
    d.ellipse((cx - r, cy - r, cx + r, cy + r), fill=(255, 255, 255, 255))
    dark = (60, 20, 140, 255)
    w = int(r * .26)
    d.rectangle((cx - w // 2, cy - int(r * .55), cx + w // 2, cy + int(r * .05)), fill=dark)
    d.polygon([(cx - int(r * .52), cy - int(r * .05)), (cx + int(r * .52), cy - int(r * .05)),
               (cx, cy + int(r * .55))], fill=dark)
    d.rectangle((cx - int(r * .58), cy + int(r * .62), cx + int(r * .58), cy + int(r * .62) + w // 2 + 4), fill=dark)
    return img


def save_ico(img, path):
    img.save(path, format="ICO", sizes=[(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)])


if __name__ == "__main__":
    base = make_base()
    save_ico(base, os.path.join(HERE, "vibetune.ico"))
    save_ico(setup_version(base), os.path.join(HERE, "setup.ico"))
    for px in (512, 256, 64):
        base.resize((px, px), Image.LANCZOS).save(os.path.join(HERE, f"icon_{px}.png"))
    setup_version(base).resize((256, 256), Image.LANCZOS).save(os.path.join(HERE, "setup_256.png"))
    print("icons written to", HERE)
