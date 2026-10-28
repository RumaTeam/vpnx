# The VPNX icon (256x256 JPEG for the NRO): the approved mark "A" (rails, flipper V, the ball) on the dark app background.
from PIL import Image, ImageDraw, ImageFont

ACCENT, WHITE, OFF = (0xFF, 0xB0, 0x20), (255, 255, 255), (0x24, 0x2C, 0x38)
BG = (0x0B, 0x0E, 0x13)
FONT = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
ROWS = ["10001", "10001", "10201", "01010", "01010", "00100"]

img = Image.new("RGB", (256, 256), BG)
d = ImageDraw.Draw(img)
tile, gap = 22, 6
mw = 5 * tile + 4 * gap
mh = 6 * tile + 5 * gap
x0, y0 = (256 - mw) // 2, 20
for i, line in enumerate(ROWS):
    for j, ch in enumerate(line):
        col = OFF if ch == "0" else (WHITE if ch == "2" else ACCENT)
        x, y = x0 + j * (tile + gap), y0 + i * (tile + gap)
        d.rounded_rectangle([x, y, x + tile - 1, y + tile - 1], radius=tile // 3, fill=col)
font = ImageFont.truetype(FONT, 38)
w = d.textlength("VPNX", font=font)
d.text(((256 - w) / 2, 20 + mh + 14), "VPNX", font=font, fill=WHITE)
img.save("icon.jpg", quality=95)
print("ok")
