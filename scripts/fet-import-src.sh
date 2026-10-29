#!/usr/bin/env bash
# استيراد شجرة مصدر FET من الأرشيف المحفوظ إلى fet/ (نسخة العمل التي نعدّلها).
#
# نستورد فقط ما يلزم لبناء البرنامج: الملفات العلوية + src/ + icons/ + licenses/.
# لا نستورد translations/ (190MB) ولا examples/ (142MB) ولا doc/.
#
# الاستخدام:
#   scripts/fet-import-src.sh            # إنشاء fet/ إن لم تكن موجودة
#   scripts/fet-import-src.sh --force    # مسح fet/ واستيراد نسخة أصلية نظيفة
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VERSION="7.12.0"
ARCHIVE="$ROOT/vendor/fet/fet-${VERSION}.tar.xz"
DEST="$ROOT/fet"
FORCE="${1:-}"

[[ -f "$ARCHIVE" ]] || { echo "خطأ: الأرشيف غير موجود: $ARCHIVE" >&2; exit 1; }

if [[ -d "$DEST" && "$FORCE" != "--force" ]]; then
  echo "المجلد fet/ موجود مسبقًا — لم يُلمس."
  echo "لإعادة استيراد نسخة أصلية (يمسح تعديلاتنا): scripts/fet-import-src.sh --force"
  exit 0
fi

echo "== التحقق من الأرشيف =="
"$ROOT/scripts/fet-verify.sh" >/dev/null

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

echo "== استخراج الملفات المطلوبة =="
tar -xJf "$ARCHIVE" -C "$TMP" \
  "fet-${VERSION}/CMakeLists.txt" \
  "fet-${VERSION}/VERSION" \
  "fet-${VERSION}/fet.pro" \
  "fet-${VERSION}/fet.desktop" \
  "fet-${VERSION}/fet.xml" \
  "fet-${VERSION}/COPYING" \
  "fet-${VERSION}/COPYING.FDL" \
  "fet-${VERSION}/AUTHORS" \
  "fet-${VERSION}/README" \
  "fet-${VERSION}/src" \
  "fet-${VERSION}/icons" \
  "fet-${VERSION}/licenses"

rm -rf "$DEST"
mv "$TMP/fet-${VERSION}" "$DEST"

# نضيف نسخة من نص الرخصة داخل الشجرة (متطلب AGPL عند التوزيع)
cp "$ROOT/LICENSE" "$DEST/LICENSE"

echo
echo "تم الاستيراد في: $DEST"
echo "الملفات: $(find "$DEST" -type f | wc -l) — الحجم: $(du -sh "$DEST" | cut -f1)"
echo
echo "تنبيه: هذا المجلد نسخة معدّلة من FET — راجع fet/README-SOURCE.md"
