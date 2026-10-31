#!/usr/bin/env python3
"""يولّد شعار Open-Timetable (PNG و ICO) بلا أي مكتبات خارجية.

الرسم مطابق لشعار الواجهة داخل البرنامج (fet/src/interface/ui_icons.cpp):
مربّع مستدير أزرق + رأس جدول + علامة صح.

الاستخدام:
    python3 scripts/make-app-icon.py

الناتج:
    fet/icons/open-timetable.png   (256×256)
    fet/icons/open-timetable.ico   (16…256)
"""

import math
import os
import struct
import zlib

SIZE = 256
SS = 4  # معامل التنقية (supersampling)

BLUE = (0x25, 0x63, 0xEB)
WHITE = (0xFF, 0xFF, 0xFF)

# أبعاد الشعار في شبكة 64×64 (نفس أبعاد ui_icons.cpp)
GRID_LINES = [
    (14.0, 22.0, 50.0, 22.0, 2.4, 0.85),
    (24.0, 14.0, 24.0, 20.0, 2.4, 0.85),
    (40.0, 14.0, 40.0, 20.0, 2.4, 0.85),
    (14.0, 34.0, 50.0, 34.0, 2.4, 0.85),
]
CHECK_LINES = [
    (20.0, 46.0, 28.0, 54.0, 5.0, 1.0),
    (28.0, 54.0, 44.0, 38.0, 5.0, 1.0),
]


def rounded_rect_contains(x, y, left, top, right, bottom, radius):
    if x < left or x > right or y < top or y > bottom:
        return False
    cx = min(max(x, left + radius), right - radius)
    cy = min(max(y, top + radius), bottom - radius)
    return (x - cx) ** 2 + (y - cy) ** 2 <= radius ** 2


def distance_to_segment(px, py, x1, y1, x2, y2):
    dx, dy = x2 - x1, y2 - y1
    length_sq = dx * dx + dy * dy
    if length_sq == 0:
        return math.hypot(px - x1, py - y1)
    t = max(0.0, min(1.0, ((px - x1) * dx + (py - y1) * dy) / length_sq))
    return math.hypot(px - (x1 + t * dx), py - (y1 + t * dy))


def sample(x, y):
    """يعيد اللون (r,g,b,a) عند نقطة داخل شبكة 64×64."""
    if not rounded_rect_contains(x, y, 2, 2, 62, 62, 14):
        return None

    best = None  # (الشفافية، اللون)
    for x1, y1, x2, y2, width, alpha in GRID_LINES + CHECK_LINES:
        d = distance_to_segment(x, y, x1, y1, x2, y2)
        if d <= width / 2.0:
            a = alpha
            if best is None or a >= best[0]:
                best = (a, WHITE)

    if best is None:
        return (BLUE[0], BLUE[1], BLUE[2], 255)

    a = best[0]
    color = best[1]
    r = int(round(BLUE[0] * (1 - a) + color[0] * a))
    g = int(round(BLUE[1] * (1 - a) + color[1] * a))
    b = int(round(BLUE[2] * (1 - a) + color[2] * a))
    return (r, g, b, 255)


def render(size):
    rows = []
    step = 64.0 / (size * SS)
    for py in range(size):
        row = bytearray()
        row.append(0)  # filter type 0
        for px in range(size):
            r = g = b = a = 0
            for sy in range(SS):
                for sx in range(SS):
                    x = (px * SS + sx + 0.5) * step
                    y = (py * SS + sy + 0.5) * step
                    color = sample(x, y)
                    if color is not None:
                        r += color[0]
                        g += color[1]
                        b += color[2]
                        a += 255
            n = SS * SS
            if a == 0:
                row += bytes((0, 0, 0, 0))
            else:
                covered = a // 255
                row += bytes((r // covered, g // covered, b // covered, a // n))
        rows.append(bytes(row))
    raw = b"".join(rows)

    def chunk(tag, data):
        return (struct.pack(">I", len(data)) + tag + data
                + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF))

    ihdr = struct.pack(">IIBBBBB", size, size, 8, 6, 0, 0, 0)
    return (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr)
            + chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b""))


def write_ico(path, png_by_size, sizes):
    header = struct.pack("<HHH", 0, 1, len(sizes))
    entries = b""
    offset = 6 + 16 * len(sizes)
    payload = b""
    for size in sizes:
        data = png_by_size[size]
        dim = 0 if size >= 256 else size
        entries += struct.pack("<BBBBHHII", dim, dim, 0, 0, 1, 32, len(data), offset)
        offset += len(data)
        payload += data
    with open(path, "wb") as handle:
        handle.write(header + entries + payload)


def main():
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    icons = os.path.join(root, "fet", "icons")
    os.makedirs(icons, exist_ok=True)

    png256 = render(256)
    png_path = os.path.join(icons, "open-timetable.png")
    with open(png_path, "wb") as handle:
        handle.write(png256)
    print("wrote", png_path, len(png256), "bytes")

    sizes = [16, 32, 48, 64, 128, 256]
    pngs = {256: png256}
    for size in sizes:
        if size != 256:
            pngs[size] = render(size)
    ico_path = os.path.join(icons, "open-timetable.ico")
    write_ico(ico_path, pngs, sizes)
    print("wrote", ico_path)


if __name__ == "__main__":
    main()
