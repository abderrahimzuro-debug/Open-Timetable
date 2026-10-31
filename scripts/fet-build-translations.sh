#!/usr/bin/env bash
# بناء ملفات الترجمة (.qm) من الأرشيف المحفوظ — بدون حفظها في Git (5.9MB للعربية وحدها).
#
# الاستخدام:
#   scripts/fet-build-translations.sh            # العربية فقط
#   scripts/fet-build-translations.sh ar en_GB   # لغات محدّدة
#   scripts/fet-build-translations.sh --all      # كل لغات FET (نحو 40 لغة)
#
# الناتج: build/translations/fet_<lang>.qm
# ضع الملفات بجانب تنفيذي fet (أو في مجلّد translations بجانبه) ليحمّلها البرنامج.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VERSION="7.12.0"
ARCHIVE="$ROOT/vendor/fet/fet-${VERSION}.tar.xz"
WORK="$ROOT/.fet-src/translations-src"
OUT="$ROOT/build/translations"

LANGS=("$@")
if [[ ${#LANGS[@]} -eq 0 ]]; then
  LANGS=(ar)
fi

ALL=false
if [[ "${LANGS[0]}" == "--all" ]]; then
  ALL=true
  LANGS=()
fi

[[ -f "$ARCHIVE" ]] || { echo "خطأ: الأرشيف غير موجود: $ARCHIVE" >&2; exit 1; }

# أداة lrelease: أسماء مختلفة بين التوزيعات
LRELEASE=""
for c in lrelease lrelease-qt5 lrelease6 lrelease-qt6; do
  if command -v "$c" >/dev/null 2>&1; then LRELEASE="$c"; break; fi
done

if [[ -z "$LRELEASE" ]]; then
  echo "خطأ: lrelease غير مثبّت." >&2
  echo "  Debian/Ubuntu: sudo apt install qttools5-dev-tools" >&2
  exit 1
fi

mkdir -p "$WORK" "$OUT"

if [[ "$ALL" == true ]]; then
  echo "== استخراج قائمة اللغات من الأرشيف =="
  mapfile -t LANGS < <(tar -tJf "$ARCHIVE" \
    | grep -oE 'translations/fet_[A-Za-z_]+\.ts$' \
    | sed -E 's#translations/fet_(.*)\.ts#\1#' \
    | grep -v '^untranslated$' \
    | sort -u)
  echo "عدد اللغات: ${#LANGS[@]}"
fi

if [[ ${#LANGS[@]} -eq 0 ]]; then
  echo "خطأ: لم تُحدَّد أي لغة." >&2
  exit 1
fi

for lang in "${LANGS[@]}"; do
  ts="fet-${VERSION}/translations/fet_${lang}.ts"
  echo "== $lang: استخراج $ts =="
  if ! tar -xJf "$ARCHIVE" -C "$WORK" --strip-components=2 "$ts" 2>/dev/null; then
    echo "خطأ: الترجمة '$lang' غير موجودة في الأرشيف." >&2
    exit 1
  fi

  # اسم برنامجنا بدل الاسم الأصلي، حتى تبقى الترجمات مطابقة لنصوص الواجهة
  python3 "$ROOT/scripts/rebrand-strings.py" "$WORK/fet_${lang}.ts" >/dev/null

  echo "== $lang: lrelease =="
  "$LRELEASE" -silent "$WORK/fet_${lang}.ts" -qm "$OUT/fet_${lang}.qm"
  ls -lh "$OUT/fet_${lang}.qm"
done

echo
echo "الترجمات جاهزة في: $OUT"
