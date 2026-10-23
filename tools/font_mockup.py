# -*- coding: utf-8 -*-
"""
Screen mockup: lays out the Chinese text as a 1bpp bitmap following the real
400x300 device layout, then scales it up for inspection.

The Chinese text uses the real bitmap (identical to what gets written into
fontCN_zh.c). Icon areas are drawn as grey placeholders and do not represent the
real icons.

Run  : python tools/font_mockup.py
Output: tools/font_mockup.png
"""
import sys
from PIL import Image, ImageDraw, ImageFont

if hasattr(sys.stdout, 'reconfigure'):
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')

FONTS = {
    'songs':  ('tools/fonts/SourceHanSerifSC-Bold.otf', '思源宋体 Bold'),
    'zcool':  ('tools/fonts/ZCOOLXiaoWei-Regular.ttf',  '站酷小薇（手写文艺楷）'),
    'mashan': ('tools/fonts/MaShanZheng-Regular.ttf',   '马善政毛笔楷书'),
}
KEY = sys.argv[1] if len(sys.argv) > 1 else 'songs'
if KEY not in FONTS:
    raise SystemExit('可选：' + ' / '.join(FONTS))
CN_FONT, CN_LABEL = FONTS[KEY]
CN_BOX = 32
SCALE = 2.4
OUT = 'tools/font_mockup_%s.png' % KEY

SCREEN_W, SCREEN_H = 400, 300


def cn_bitmap(ch, box=CN_BOX):
    """Renders one Han character as a 1bpp bitmap, using the exact same algorithm
    as gen_fontcn.py."""
    img = Image.new("1", (box, box), 0)
    d = ImageDraw.Draw(img)
    font, bbox = None, None
    for size in range(box, 8, -1):
        f = ImageFont.truetype(CN_FONT, size, index=0)
        b = d.textbbox((0, 0), ch, font=f)
        if b[2] - b[0] <= box and b[3] - b[1] <= box:
            font, bbox = f, b
            break
    w, h = bbox[2] - bbox[0], bbox[3] - bbox[1]
    d.text(((box - w) // 2 - bbox[0], (box - h) // 2 - bbox[1]), ch, font=font, fill=1)
    return img


def put_cn(canvas, text, x, y, box=CN_BOX):
    for i, ch in enumerate(text):
        canvas.paste((0, 0, 0), (x + i * box, y), cn_bitmap(ch, box).convert("L"))


def main():
    canvas = Image.new("RGB", (SCREEN_W, SCREEN_H), (255, 255, 255))
    d = ImageDraw.Draw(canvas)

    # Date: the device uses Font24; approximated here with a system font.
    f_date = ImageFont.truetype("C:/Windows/Fonts/msyh.ttc", 26)
    d.text((15, 18), "2026/10/16", font=f_date, fill=(0, 0, 0))

    # Weekday (real bitmap, 32x32, box y0=64)
    put_cn(canvas, "星期三", 15, 64)

    # Title (real bitmap, 32x32, box y0=106)
    put_cn(canvas, "未来天气", 15, 106)

    # Divider line
    d.line([(15, 140), (160, 140)], fill=(0, 0, 0), width=2)

    # Temperature box, top right
    d.rectangle([220, 15, 380, 140], outline=(0, 0, 0), width=2)
    d.line([(220, 118), (380, 118)], fill=(0, 0, 0), width=2)
    f_temp = ImageFont.truetype("C:/Windows/Fonts/msyh.ttc", 17)
    d.text((240, 123), "16.3 / 24.7", font=f_temp, fill=(0, 0, 0))

    # Travel-advisory placeholder
    d.rectangle([227, 18, 371, 108], outline=(170, 170, 170), width=1)
    d.text((262, 56), "建议图标 144×90", font=ImageFont.truetype("C:/Windows/Fonts/msyh.ttc", 12), fill=(150, 150, 150))

    # Current-conditions large icon placeholder
    d.rectangle([140, 60, 188, 108], outline=(170, 170, 170), width=1)
    d.text((146, 76), "48×48", font=ImageFont.truetype("C:/Windows/Fonts/msyh.ttc", 10), fill=(150, 150, 150))

    # Next-12-hours grid (placeholders)
    xs = [15, 78, 141, 204, 267, 330]
    ys = [145, 215]
    tfont = ImageFont.truetype("C:/Windows/Fonts/msyh.ttc", 11)
    n = 0
    for l, y in enumerate(ys):
        for x in xs:
            d.rectangle([x, y, x + 47, y + 47], outline=(200, 200, 200), width=1)
            d.text((x + 4, y + 55), "%02d:00" % ((12 + n) % 24), font=tfont, fill=(120, 120, 120))
            n += 1

    # Scale up
    big = canvas.resize((int(SCREEN_W * SCALE), int(SCREEN_H * SCALE)), Image.NEAREST)

    # Add the title bar
    pad = 24
    out = Image.new("RGB", (big.width + pad * 2, big.height + pad * 2 + 52), (255, 255, 255))
    od = ImageDraw.Draw(out)
    tf = ImageFont.truetype("C:/Windows/Fonts/msyhbd.ttc", 20)
    sf = ImageFont.truetype("C:/Windows/Fonts/msyh.ttc", 14)
    od.text((pad, pad), "%s · 32×32 · 中文界面效果模拟" % CN_LABEL, font=tf, fill=(30, 30, 30))
    od.text((pad, pad + 28), "汉字为真实点阵（与烧进固件的完全一致）；灰色框仅为图标位置占位", font=sf, fill=(130, 130, 130))
    out.paste(big, (pad, pad + 52))
    out.save(OUT)
    print("已生成 %s（%d×%d）" % (OUT, out.width, out.height))


if __name__ == "__main__":
    main()
