#!/usr/bin/env python3
"""يُصلح ملفات HTML التي صُدِّرت قبل التحديث (خطأ الحروف اليونانية).

المشكلة: الملفات المصدَّرة قديمًا لا تحدّد خطًّا، فيختار المتصفّح خطًّا بديلًا حسب
إعدادات الجهاز — وقد يقع على خط "Symbol" الذي يعرض الحروف اللاتينية رموزًا
يونانية (Exp2Fet تُعرض «Εξπ2Φετ»). النص نفسه سليم، والمشكلة في العرض فقط.

هذا السكربت يضيف سطر الخط إلى كل ملف HTML في المجلد (وأي مجلد فرعي)، فلا حاجة
إلى إعادة التصدير. لا يحذف ولا يعيد كتابة أي نص آخر.

الاستخدام:
    python3 scripts/fix-exported-html-font.py "المسار/إلى/مجلد/الجداول"
    python3 scripts/fix-exported-html-font.py "المسار" --dry-run
"""

import glob
import os
import sys

FONT = ("'Segoe UI', Tahoma, 'Noto Naskh Arabic', 'Noto Sans Arabic', "
        "'DejaVu Sans', Arial, sans-serif")

STYLE_RULE = ("<style>\n  html, body, table, thead, tbody, tfoot, tr, th, td,\n"
              "  caption, div, span, p { font-family: %s; }\n</style>\n" % FONT)


def fix_file(path, dry_run):
    with open(path, encoding="utf-8", errors="replace") as handle:
        text = handle.read()

    if "font-family:" in text:
        return False  # مُصلَح أصلًا

    patched = text
    if "</head>" in patched:
        patched = patched.replace("</head>", STYLE_RULE + "</head>", 1)
    elif "<body" in patched:
        patched = patched.replace("<body", '<body style="font-family: %s"' % FONT, 1)
    else:
        return False

    if not dry_run:
        with open(path, "w", encoding="utf-8") as handle:
            handle.write(patched)
    return True


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    dry_run = "--dry-run" in sys.argv

    if not args:
        print(__doc__)
        return 1

    root = os.path.abspath(args[0])
    if not os.path.isdir(root):
        print("خطأ: ليس مجلدًا:", root)
        return 1

    files = sorted(glob.glob(os.path.join(root, "**", "*.html"), recursive=True))
    if not files:
        print("لا توجد ملفات HTML في:", root)
        return 0

    changed = 0
    for path in files:
        if fix_file(path, dry_run):
            changed += 1
            print(("كان سيُصلَح: " if dry_run else "أُصلح: ") + os.path.relpath(path, root))

    print()
    print("المجموع: %d ملفًا من %d" % (changed, len(files)))
    if dry_run:
        print("(تجربة فقط — لم يُكتب شيء. أعد التنفيذ بدون --dry-run للتطبيق.)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
