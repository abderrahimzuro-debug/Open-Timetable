#!/usr/bin/env bash
# بناء محرّك FET بوضع سطر الأوامر (fet-cl) من الأرشيف المحفوظ.
#
# المتطلبات:
#   - الطريقة السريعة (qmake): Qt 5.15 أو أحدث (حزمة qtbase5-dev على Debian/Ubuntu) + g++ + make
#   - الطريقة البديلة (cmake): Qt 6.10 أو أحدث + CMake 3.28 أو أحدث
#
# الاستخدام:
#   scripts/fet-build-cl.sh            # يختار تلقائيًا (qmake إن وُجد، وإلا cmake)
#   scripts/fet-build-cl.sh --qmake
#   scripts/fet-build-cl.sh --cmake
#   scripts/fet-build-cl.sh --docker   # يطبع أمر البناء عبر Docker
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MODE="${1:-auto}"
SRC_DIR="$ROOT/.fet-src/fet-7.12.0"
BUILD_DIR="$ROOT/build"
OUT_BIN="$BUILD_DIR/fet-cl"

if [[ "$MODE" == "--docker" ]]; then
  cat <<'EOF'
البناء عبر Docker (لا يحتاج Qt على جهازك):

  docker build -f docker/engine.Dockerfile -t open-timetable-engine .
  # تشغيل تجربة:
  docker run --rm -v "$PWD/engine/samples:/in" -v "$PWD/.fet-runs:/out" \
      open-timetable-engine fet-cl --inputfile=/in/small-test.fet --outputdir=/out --timelimitseconds=30
EOF
  exit 0
fi

# 1) تجهيز المصدر
if [[ ! -d "$SRC_DIR" ]]; then
  echo "المصدر غير مفكوك، سيتم فكّه الآن..."
  "$ROOT/scripts/fet-extract.sh" >/dev/null
fi

mkdir -p "$BUILD_DIR"

has() { command -v "$1" >/dev/null 2>&1; }

qmake_bin=""
for c in qmake6 qmake-qt6 qmake; do
  if has "$c"; then qmake_bin="$c"; break; fi
done

# 2) بناء
if [[ "$MODE" != "--cmake" ]] && [[ -n "$qmake_bin" ]]; then
  echo "== البناء بـ qmake ($qmake_bin) — مشروع سطر الأوامر فقط =="
  mkdir -p "$BUILD_DIR/cl-build"
  cd "$BUILD_DIR/cl-build"
  "$qmake_bin" "$SRC_DIR/src/src-cl.pro" CONFIG+=release
  make -j"$(nproc 2>/dev/null || echo 2)"
elif [[ "$MODE" != "--qmake" ]] && has cmake; then
  echo "== البناء بـ CMake (COMMAND_LINE_ONLY) =="
  cmake -S "$SRC_DIR" -B "$BUILD_DIR/cmake-cl" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCOMMAND_LINE_ONLY=ON
  cmake --build "$BUILD_DIR/cmake-cl" -j"$(nproc 2>/dev/null || echo 2)"
  find "$BUILD_DIR/cmake-cl" -type f -name 'fet-cl*' -perm -u+x -exec cp {} "$OUT_BIN" \;
else
  echo "خطأ: لا يوجد qmake ولا cmake على هذا النظام." >&2
  echo "  - Debian/Ubuntu: sudo apt install build-essential qtbase5-dev" >&2
  echo "  - أو استخدم Docker: scripts/fet-build-cl.sh --docker" >&2
  exit 1
fi

# src-cl.pro يضع الناتج في DESTDIR=.. أي build/fet-cl
if [[ -x "$BUILD_DIR/fet-cl" ]]; then
  echo
  echo "تم البناء: $BUILD_DIR/fet-cl"
  "$BUILD_DIR/fet-cl" --version || true
else
  echo "تحذير: لم يُعثر على الملف التنفيذي في $BUILD_DIR/fet-cl" >&2
  find "$BUILD_DIR" -maxdepth 3 -type f -name 'fet-cl*' >&2 || true
  exit 1
fi
