# Preview sheet of the VPNX mark candidates (same language as Ruma's "R": a grid of rounded tiles, accent colour, one white tile).
from PIL import Image, ImageDraw, ImageFont

ACCENT, WHITE, OFF = (0xFF, 0xB0, 0x20), (255, 255, 255), (0x1F, 0x27, 0x33)
BG, CHROME, LINE = (0x0B, 0x0E, 0x13), (0x0F, 0x13, 0x19), (0x1E, 0x25, 0x2F)
FONT = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"

# 1 = lit tile, 2 = the ball (white), 0 = off. Rails are the table walls, the slanted tiles the flippers, the gap between them the drain.
VARIANTS = [
    ("A  V de flippers entre trilhos", 5, [
        "10001",
        "10001",
        "10201",
        "01010",
        "01010",
        "00100"]),
    ("B  flippers com o dreno aberto", 7, [
        "1000001",
        "1000001",
        "1002001",
        "1000001",
        "0100010",
        "0010100"]),
    ("C  V com a bola acima", 5, [
        "00200",
        "00000",
        "10001",
        "01010",
        "01010",
        "00100"]),
]

def mark(draw, x, y, cols, rows, tile, gap, dim=False, radius=None):
    r = radius if radius is not None else max(1, tile // 3)
    for i, line in enumerate(rows):
        for j, ch in enumerate(line):
            if ch == "0" and not dim:
                continue
            col = OFF if ch == "0" else (WHITE if ch == "2" else ACCENT)
            x0, y0 = x + j * (tile + gap), y + i * (tile + gap)
            draw.rounded_rectangle([x0, y0, x0 + tile - 1, y0 + tile - 1], radius=r, fill=col)

W, H = 1620, 700
img = Image.new("RGB", (W, H), BG)
d = ImageDraw.Draw(img)
title = ImageFont.truetype(FONT, 22)
d.text((40, 24), "VPNX - candidatos a logo (peças em grade, como o R do Ruma)", font=title, fill=(230, 234, 240))

colw = 530
for k, (name, cols, rows) in enumerate(VARIANTS):
    ox = 40 + k * colw
    # large: tile 26 / gap 7, then the word
    tile, gap = 26, 7
    mw, mh = cols * tile + (cols - 1) * gap, len(rows) * tile + (len(rows) - 1) * gap
    d.rounded_rectangle([ox - 10, 80, ox + colw - 30, 390], radius=10, fill=CHROME, outline=LINE)
    mark(d, ox + 20, 110, cols, rows, tile, gap, dim=True)
    word = ImageFont.truetype(FONT, 56)
    d.text((ox + 20 + mw + 24, 110 + mh // 2 - 36), "VPNX", font=word, fill=WHITE)
    d.text((ox + 6, 345), name, font=ImageFont.truetype(FONT, 16), fill=(174, 182, 194))
    # header strip at the real proportions (36 px high, mark tile 3 / gap 1), shown at 4x
    sx, sy, s = ox - 10, 430, 4
    strip = Image.new("RGB", (330, 36), CHROME)
    sd = ImageDraw.Draw(strip)
    t3, g1 = 3, 1
    mh2 = len(rows) * (t3 + g1) - g1
    mark(sd, 14, (36 - mh2) // 2, cols, rows, t3, g1)
    mw2 = cols * (t3 + g1) - g1
    sd.text((14 + mw2 + 8, 7), "VPNX", font=ImageFont.truetype(FONT, 15), fill=WHITE)
    sd.line([0, 35, 330, 35], fill=LINE)
    img.paste(strip.resize((660, 72), Image.NEAREST).crop((0, 0, 495, 72)), (sx, sy + 20))
    # and at 1:1 for the real feel
    img.paste(strip, (sx, sy + 130))
    d.text((sx + 350, sy + 40), "cabecalho, 2x", font=ImageFont.truetype(FONT, 12), fill=(122, 132, 148))
    d.text((sx + 350, sy + 140), "tamanho real", font=ImageFont.truetype(FONT, 12), fill=(122, 132, 148))

d.text((40, 650), "Amarelo/ambar = acento do VPNX; a peça branca é a bola; trilhos = laterais da mesa; peças inclinadas = flippers (triggers).",
       font=ImageFont.truetype(FONT, 15), fill=(140, 150, 165))
img.save("vpnx_logo_preview.png")
print("ok", img.size)
