# -*- coding: utf-8 -*-
"""
Chinese bitmap font comparison: renders several candidate typefaces at 1bpp,
scales the results up and tiles them into a single image.

Purpose: preview before committing to a font. Small bitmap glyphs look very
different from their vector rendering, so they must be judged at the final
bitmap size.

Run  : python tools/font_compare.py
Output: tools/font_compare.png
"""
import sys
from PIL import Image, ImageDraw, ImageFont

if hasattr(sys.stdout, 'reconfigure'):
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')

CHARS = "未来天气星期三"
CANDIDATES = [
    ("思源宋体 Bold（当前）", r"tools/fonts/SourceHanSerifSC-Bold.otf", 0),
    ("马善政毛笔楷书", r"tools/fonts/MaShanZheng-Regular.ttf", 0),
    ("站酷小薇（文艺楷）", r"tools/fonts/ZCOOLXiaoWei-Regular.ttf", 0),
    ("微软雅黑 Regular", r"C:/Windows/Fonts/msyh.ttc", 0),
]
BOXES = [32, 40]
SCALE = 5
LABEL_W = 200
PAD = 20
OUT = "tools/font_compare.png"


def render_char(ch, box, path, index):
    """Renders one glyph into a box x box 1bpp canvas, auto-fitting the size so
    nothing is clipped. Returns (image, actual font size)."""
    img = Image.new("1", (box, box), 0)
    d = ImageDraw.Draw(img)
    font, bbox = None, None
    for size in range(box, 8, -1):
        try:
            f = ImageFont.truetype(path, size, index=index)
        except OSError:
            return None, 0
        b = d.textbbox((0, 0), ch, font=f)
        if b[2] - b[0] <= box and b[3] - b[1] <= box:
            font, bbox = f, b
            break
    if font is None:
        return None, 0
    w, h = bbox[2] - bbox[0], bbox[3] - bbox[1]
    d.text(((box - w) // 2 - bbox[0], (box - h) // 2 - bbox[1]), ch, font=font, fill=1)
    return img, getattr(font, "size", 0)


def main():
    cell = max(BOXES) * SCALE
    row_h = cell + 34
    width = PAD * 2 + LABEL_W + len(CHARS) * cell + 10
    height = PAD + 40 + len(CANDIDATES) * len(BOXES) * row_h

    canvas = Image.new("RGB", (width, height), (255, 255, 255))
    d = ImageDraw.Draw(canvas)
    title_font = ImageFont.truetype("C:/Windows/Fonts/msyh.ttc", 22, index=0)
    name_font = ImageFont.truetype("C:/Windows/Fonts/msyhbd.ttc", 18, index=0)
    small_font = ImageFont.truetype("C:/Windows/Fonts/msyh.ttc", 15, index=0)
    d.text((PAD, PAD - 8), "中文点阵字体对比（每格 = 屏幕上的真实像素，放大 %d 倍）" % SCALE,
           font=title_font, fill=(30, 30, 30))

    y = PAD + 36
    for name, path, idx in CANDIDATES:
        d.text((PAD, y), name, font=name_font, fill=(30, 30, 30))
        y += 30
        for box in BOXES:
            d.text((PAD, y + cell // 2 - 8), "%d×%d 点阵" % (box, box), font=small_font, fill=(120, 120, 120))
            x = PAD + LABEL_W
            used = 0
            for ch in CHARS:
                img, used = render_char(ch, box, path, idx)
                size_px = box * SCALE
                if img is None:
                    canvas.paste(Image.new("RGB", (size_px, size_px), (240, 240, 240)), (x, y))
                else:
                    gray = img.convert("L").point(lambda v: 0 if v else 255)
                    canvas.paste(gray.resize((size_px, size_px), Image.NEAREST).convert("RGB"), (x, y))
                d.rectangle([x, y, x + size_px - 1, y + size_px - 1], outline=(215, 215, 215))
                x += size_px
            d.text((x + 8, y + cell // 2 - 8), "字号 %d" % used, font=small_font, fill=(150, 150, 150))
            y += cell + 14

    canvas.save(OUT)
    print("已生成 %s（%d×%d）" % (OUT, width, height))
    print("对比字：%s" % CHARS)


if __name__ == "__main__":
    main()
