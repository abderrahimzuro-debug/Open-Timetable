#!/usr/bin/env python3
"""يولّد شعار Open-Timetable (PNG و ICO) بلا أي مكتبات خارجية.

الرسم مطابق لشعار الواجهة داخل البرنامج (fet/src/interface/ui_icons.cpp):
مربّع مستدير بتدرّج أزرق/بنفسجي + أربعة مربّعات تمثّل خلايا جدول زمني.

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

TOP = (0x4F, 0x46, 0xE5)     # بداية التدرّج
BOTTOM = (0x25, 0x63, 0xEB)  # نهاية التدرّج
WHITE = (0xFF, 0xFF, 0xFF)
ACCENT = (0xC7, 0xD2, 0xFE)

# الخلايا الأربع في شبكة 64×64: (يسار، أعلى، يمين، أسفل، نصف قطر، هل هي المميّزة)
CELLS = [
    (14.0, 14.0, 25.0, 25.0, 2.5, False),
    (28.0, 14.0, 50.0, 25.0, 2.5, False),
    (14.0, 28.0, 25.0, 50.0, 2.5, False),
    (28.0, 28.0, 50.0, 50.0, 3.0, True),
]


def rounded_rect_contains(x, y, left, top, right, bottom, radius):
    if x < left or x > right or y < top or y > bottom:
        return False
    cx = min(max(x, left + radius), right - radius)
    cy = min(max(y, top + radius), bottom - radius)
    return (x - cx) ** 2 + (y - cy) ** 2 <= radius ** 2


def mix(a, b, t):
    return tuple(int(round(a[i] + (b[i] - a[i]) * t)) for i in range(3))


def sample(x, y):
    """يعيد اللون (r,g,b,a) عند نقطة داخل شبكة 64×64، أو None خارج الشعار."""
    if not rounded_rect_contains(x, y, 2, 2, 62, 62, 15):
        return None

    for left, top, right, bottom, radius, accent in CELLS:
        if rounded_rect_contains(x, y, left, top, right, bottom, radius):
            color = ACCENT if accent else WHITE
            return (color[0], color[1], color[2], 255)

    t = min(1.0, max(0.0, ((x - 2.0) + (y - 2.0)) / 120.0))
    base = mix(TOP, BOTTOM, t)
    return (base[0], base[1], base[2], 255)


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
