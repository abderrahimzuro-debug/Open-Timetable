#!/usr/bin/env bash
# التحقق من سلامة أرشيف FET المحفوظ في المستودع (بصمات SHA-256).
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT/vendor/fet"

echo "== التحقق من بصمات SHA-256 =="
sha256sum -c SHA256SUMS

echo
echo "== مقارنة البصمة بالموصى به في المخطّط =="
EXPECTED="ff3d1e0be7708a648f6c4a5078f49d0b0926283c065791105229753c7b697cd4"
ACTUAL="$(sha256sum fet-7.12.0.tar.xz | cut -d' ' -f1)"
if [[ "$ACTUAL" != "$EXPECTED" ]]; then
  echo "خطأ: بصمة الأرشيف لا تطابق البصمة المعتمدة." >&2
  exit 1
fi
echo "OK: $ACTUAL"

echo
if command -v gpg >/dev/null 2>&1; then
  echo "== التوقيع الرقمي (يتطلب مفتاح المطوّر العام) =="
  if gpg --verify fet-7.12.0.tar.xz.asc fet-7.12.0.tar.xz 2>/dev/null; then
    echo "OK: التوقيع صحيح."
  else
    echo "تنبيه: لم يتم التحقق من التوقيع (المفتاح العام للمطوّر غير مستورد)."
    echo "        المفتاح متاح على: https://lalescu.ro/liviu/fet/download.html"
  fi
else
  echo "تنبيه: gpg غير مثبّت، تم تخطي التحقق من التوقيع."
fi

echo
echo "الأرشيف سليم."
