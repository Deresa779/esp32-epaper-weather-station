# -*- coding: utf-8 -*-
"""
Layout preview: draws the actual e-paper layout in Chinese mode as ASCII art so
it can be checked by eye.

Purpose: run it after changing coordinates in main.cpp to confirm that text does
not overlap or spill outside its box.

IMPORTANT: the coordinates here are copied from Paint_Basicinf() in
           src/main.cpp. If you change the coordinates there, update them here
           too, otherwise the preview will lie to you.

Run:
    python tools/preview_layout.py            # top-left area (date/weekday/title)
    python tools/preview_layout.py full       # whole screen
"""
import sys
from PIL import Image, ImageDraw, ImageFont

FONT_PATH = "C:/Windows/Fonts/msyh.ttc"
SCREEN_W, SCREEN_H = 400, 300

# ---- Coordinates kept in sync with main.cpp ----
DATE_X, DATE_Y, DATE_W, DATE_H = 15, 20, 200, 26          # Font24, ASCII
WEEK_CN = (15, 64, 140, 32)                                # FontCN_zh 32x32
TITLE_CN = (15, 106, 140, 32)                             # FontCN_zh 32x32
LINE_Y = 140
BOX = (220, 15, 380, 140)
BOX_MID_Y = 118

CN_SIZE = 32     # FontCN_zh.Width / Height


def draw_cn(d, text, x0, y0, box_w, box_h, font, center=False):
    """Reproduces the alignment logic of DrawStringWithAlignmentCN."""
    tw = len(text) * CN_SIZE
    th = CN_SIZE
    x1 = x0 + (box_w - tw) // 2 if center else x0
    y1 = y0 + (box_h - th) // 2
    d.text((max(x1, 0), max(y1, 0)), text, font=font, fill=0)


def build():
    img = Image.new("1", (SCREEN_W, SCREEN_H), 1)   # 1 = white background
    d = ImageDraw.Draw(img)
    f_ascii = ImageFont.truetype("C:/Windows/Fonts/msyh.ttc", 24)   # Approximates the on-device Font24
    f_cn = ImageFont.truetype("tools/fonts/SourceHanSerifSC-Bold.otf", 32)

    # Date (ASCII; Chinese mode renders it as year/month/day)
    d.text((DATE_X, DATE_Y), "2026/10/16", font=f_ascii, fill=0)
    # Weekday
    draw_cn(d, "星期三", *WEEK_CN, f_cn)
    # Title
    draw_cn(d, "未来天气", *TITLE_CN, f_cn)
    # Divider line
    d.line([(15, LINE_Y), (160, LINE_Y)], fill=0, width=2)
    # Temperature box, top right
    d.rectangle(list(BOX), outline=0, width=2)
    d.line([(BOX[0], BOX_MID_Y), (BOX[2], BOX_MID_Y)], fill=0, width=2)
    return img


def ascii_art(img, x0, y0, x1, y1, step=3):
    px = img.load()
    out = []
    for row in range(y0, min(y1, SCREEN_H), step):
        line = ''
        for col in range(x0, min(x1, SCREEN_W), step):
            dark = False
            for r in range(row, min(row + step, SCREEN_H)):
                for c in range(col, min(col + step, SCREEN_W)):
                    if px[c, r] == 0:
                        dark = True
                        break
                if dark:
                    break
            line += '#' if dark else '.'
        out.append(line)
    return '\n'.join(out)


if __name__ == '__main__':
    if hasattr(sys.stdout, 'reconfigure'):
        sys.stdout.reconfigure(encoding='utf-8', errors='replace')
    img = build()
    mode = sys.argv[1] if len(sys.argv) > 1 else 'left'
    print('=== 中文模式版面预览（step=3，即每个字符约 3x3 像素）===')
    if mode == 'full':
        print(ascii_art(img, 0, 0, SCREEN_W, SCREEN_H, 4))
    else:
        print('坐标刻度：每字符 3 像素；下方 150 行 = 屏上 150 像素')
        print(ascii_art(img, 0, 0, 180, 150, 3))
