#!/usr/bin/env python3
"""يستبدل اسم البرنامج الأصلي في النصوص الظاهرة للمستخدم.

يُطبَّق على:
  fet/src/interface/*.cpp   → داخل النصوص بين علامتي تنصيص فقط
  fet/src/interface/*.ui    → داخل عناصر <string> فقط
  --ts ملف.ts               → داخل <source> و<translation> و<comment>

لا يمسّ أسماء الملفات ولا الروابط ولا أسماء الأصناف/المتغيّرات، ولا النصوص
التقنية مثل امتداد .fet أو ملفات fet_*.qm.

الاستخدام:
    python3 scripts/rebrand-strings.py            # يعدّل الملفات في مكانها
    python3 scripts/rebrand-strings.py --check    # يعرض ما سيتغيّر فقط
"""

import glob
import os
import re
import sys

BRAND = re.compile(r"\bFET\b")
REPLACEMENT = "Open-Timetable"

# نصوص لا نلمسها: روابط، أسماء ملفات، صيغ تقنية
SKIP_MARKERS = ("http://", "https://", ".qm", ".fet", "fet_", "FET_")


def rebrand_text(text, relaxed=False):
    if any(marker in text for marker in SKIP_MARKERS):
        # في ملفات .ts نسمح بالاستبدال رغم امتداد .fet، حتى تبقى الترجمات مطابقة
        if not (relaxed and ".fet" in text and "http" not in text):
            return text, 0
    new, count = BRAND.subn(REPLACEMENT, text)
    return new, count


CPP_LITERAL = re.compile(r'"(?:[^"\\]|\\.)*"')


def process_cpp(path, apply_changes):
    with open(path, encoding="utf-8") as handle:
        source = handle.read()

    total = 0

    def replace(match):
        nonlocal total
        new, count = rebrand_text(match.group(0))
        total += count
        return new

    updated = CPP_LITERAL.sub(replace, source)
    if total:
        if apply_changes:
            with open(path, "w", encoding="utf-8") as handle:
                handle.write(updated)
        print(f"{path}: {total}")
    return total


UI_STRING = re.compile(r"(<string[^>]*>)(.*?)(</string>)", re.S)


def process_ui(path, apply_changes):
    with open(path, encoding="utf-8") as handle:
        source = handle.read()

    total = 0

    def replace(match):
        nonlocal total
        new, count = rebrand_text(match.group(2))
        total += count
        return match.group(1) + new + match.group(3)

    updated = UI_STRING.sub(replace, source)
    if total:
        if apply_changes:
            with open(path, "w", encoding="utf-8") as handle:
                handle.write(updated)
        print(f"{path}: {total}")
    return total


TS_TEXT = re.compile(r"(<(?:source|translation|comment)>)(.*?)(</(?:source|translation|comment)>)", re.S)


def process_ts(path, apply_changes):
    with open(path, encoding="utf-8") as handle:
        source = handle.read()

    total = 0

    def replace(match):
        nonlocal total
        new, count = rebrand_text(match.group(2), relaxed=True)
        total += count
        return match.group(1) + new + match.group(3)

    updated = TS_TEXT.sub(replace, source)
    if total and apply_changes:
        with open(path, "w", encoding="utf-8") as handle:
            handle.write(updated)
    return total


def main():
    args = [a for a in sys.argv[1:]]
    check_only = "--check" in args
    args = [a for a in args if a != "--check"]

    apply_changes = not check_only

    if args and args[0].endswith(".ts"):
        for path in args:
            count = process_ts(path, apply_changes)
            if count:
                print(f"{path}: {count}")
        return 0

    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    interface = os.path.join(root, "fet", "src", "interface")

    total = 0
    for path in sorted(glob.glob(os.path.join(interface, "*.cpp"))):
        total += process_cpp(path, apply_changes)
    for path in sorted(glob.glob(os.path.join(interface, "*.ui"))):
        total += process_ui(path, apply_changes)

    # نافذة «عن البرنامج» كُتبت من جديد، لا حاجة لمسها
    print(f"\nالمجموع: {total} نصًا" + (" (لم يُطبَّق شيء)" if check_only else ""))
    return 0


if __name__ == "__main__":
    sys.exit(main())
