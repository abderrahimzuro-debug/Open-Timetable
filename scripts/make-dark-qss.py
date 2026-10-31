#!/usr/bin/env python3
"""يولّد ورقة الأنماط الداكنة (DARK_QSS) من الفاتحة (LIGHT_QSS).

السبب: صيانة نسختين يدويًا تُنتج ثغرات — عناصر تبقى فاتحة داخل مظهر داكن
(جداول، قوائم، أشرطة تمرير، أزرار، حقول…). هنا نأخذ **كل** قواعد المظهر الفاتح
ونستبدل ألوانها بجدول تعيين صريح، فنحصل على تغطية كاملة ومقاسات متطابقة.

الاستخدام:
    python3 scripts/make-dark-qss.py            # يحديث DARK_QSS داخل ui_theme.cpp
    python3 scripts/make-dark-qss.py --dump     # يطبع الناتج فقط
"""

import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
THEME = os.path.join(ROOT, "fet", "src", "interface", "ui_theme.cpp")

# فاتح → داكن (كل لون في المظهر الفاتح له مقابل محدّد)
COLOR_MAP = {
    # أسطح وخلفيات
    "#ffffff": "#171d27",   # البطاقات والحقول
    "#fafbfc": "#1a212b",   # صفوف متبادلة
    "#f7f8fa": "#12161d",   # خلفية النافذة
    "#f4f6f9": "#1a212c",
    "#f4f5f7": "#1f2836",   # تحويم خفيف
    "#f1f4f8": "#1f2836",
    "#eef1f6": "#222c3a",   # تحويم
    "#eef1f5": "#212a37",
    "#e6ebf3": "#26313f",
    "#e6ebf2": "#2a3546",
    "#e3e8f0": "#2a3546",   # ضغط
    # حدود
    "#e3e7ed": "#262f3f",
    "#e1e5eb": "#2a3344",
    "#e1e6ee": "#2c3748",
    "#d5dae2": "#33405a",
    "#c8d0db": "#3a465c",
    "#c3cad5": "#3a465c",
    # نصوص
    "#0f172a": "#f1f5f9",
    "#17202a": "#e6eaf0",
    "#344054": "#cbd5e1",
    "#475467": "#b6c0cf",
    "#5b6675": "#9aa7b8",
    "#7a869a": "#8b97a8",
    "#98a2b3": "#7f8b9c",
    "#9aa4b2": "#8b97a8",
    "#aab6c4": "#6d7a8c",
    "#e6eaf0": "#eef2f8",
    # تحديد وتمييز
    "#e8efff": "#1e3a8a",
    "#dbe7ff": "#1e3a8a",
    "#bfd4ff": "#1e40af",
    # ألوان أرقام البطاقات: تُفتَّح لتبقى مقروءة على خلفية داكنة
    "#0f766e": "#2dd4bf",
    "#6d28d9": "#a78bfa",
    "#9333ea": "#c084fc",
    "#c2410c": "#fb923c",
    "#0369a1": "#38bdf8",
    "#b45309": "#fbbf24",
    "#be123c": "#fb7185",
    "#4d7c0f": "#a3e635",
    # الأزرق يبقى كما هو (يعمل على المظهرين)
    "#2563eb": "#2563eb",
    "#1d4ed8": "#3b82f6",
    "#1e40af": "#1d4ed8",
}

HEADER = """/* ===== المظهر الداكن =====
   مُولَّد آليًا من المظهر الفاتح بـ scripts/make-dark-qss.py — لا تُحرّره يدويًا،
   بل عدّل الجدول في السكربت ثم أعد التوليد (تغطية كاملة ومقاسات متطابقة). */"""


def extract(path, name):
    source = open(path, encoding="utf-8").read()
    start = source.index("const char* const %s = R\"QSS(" % name)
    start = source.index("(", start) + len('R"QSS(')
    end = source.index(")QSS\";", start)
    return source, start, end, source[start:end]


def darken(light):
    result = light

    def replace(match):
        color = match.group(0)
        return COLOR_MAP.get(color.lower(), color)

    result = re.sub(r"#[0-9a-fA-F]{6}", replace, result)

    # أيقونات الأشرطة (أسهم) كانت رمادية فاتحة، نُفتّحها قليلًا
    result = result.replace("#6d7a8c", "#7d8b9e")

    # عنوان الملف: نسخة داكنة رأسها تعليق التوليد
    if result.startswith("/*"):
        result = HEADER + result[result.index("*/") + 2:]
    return result


def main():
    # نقرأ المظهر الفاتح، ونستبدل موضع DARK_QSS تحديدًا (لا موضع الفاتح).
    source, light_start, light_end, light = extract(THEME, "LIGHT_QSS")
    dark = darken(light)

    if "--dump" in sys.argv:
        sys.stdout.write(dark)
        return 0

    _, dark_start, dark_end, _ = extract(THEME, "DARK_QSS")
    updated = source[:dark_start] + dark + source[dark_end:]
    open(THEME, "w", encoding="utf-8").write(updated)
    print("DARK_QSS regenerated:", len(dark.splitlines()), "lines")

    # تحقّق: لا ألوان فاتحة متبقّية في المظهر الداكن
    light_colors = {c for c in re.findall(r"#[0-9a-fA-F]{6}", light) if c.lower() not in COLOR_MAP}
    if light_colors:
        print("تحذير: ألوان بلا مقابل داكن:", sorted(light_colors))
    return 0


if __name__ == "__main__":
    sys.exit(main())
