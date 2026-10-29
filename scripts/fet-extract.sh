#!/usr/bin/env bash
# فكّ أرشيف FET المحفوظ إلى مجلد مستثنى من Git (.fet-src/).
# الاستخدام: scripts/fet-extract.sh [--force]
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_DIR="$ROOT/.fet-src"
VERSION="7.12.0"
ARCHIVE="$ROOT/vendor/fet/fet-${VERSION}.tar.xz"
TARGET="$SRC_DIR/fet-${VERSION}"
FORCE="${1:-}"

[[ -f "$ARCHIVE" ]] || { echo "خطأ: الأرشيف غير موجود: $ARCHIVE" >&2; exit 1; }

if [[ -d "$TARGET" && "$FORCE" != "--force" ]]; then
  echo "المصدر مفكوك مسبقًا في: $TARGET"
  echo "(استخدم --force لإعادة الفكّ)"
  echo "$TARGET"
  exit 0
fi

echo "== التحقق من الأرشيف =="
"$ROOT/scripts/fet-verify.sh" >/dev/null

mkdir -p "$SRC_DIR"
echo "== فكّ الضغط (≈352 ميغابايت، قد يستغرق دقيقة) =="
rm -rf "$TARGET"
tar -xJf "$ARCHIVE" -C "$SRC_DIR"

echo
echo "تم. مسار المصدر:"
echo "$TARGET"
