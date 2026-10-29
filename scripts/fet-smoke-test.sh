#!/usr/bin/env bash
# اختبار دخان (smoke test): تشغيل المحرّك على ملفات نموذجية والتأكد من إخراج جداول صحيحة.
#
# الاستخدام: scripts/fet-smoke-test.sh [مسار-fet-cl]
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="${1:-$ROOT/build/fet-cl}"
SAMPLES_DIR="$ROOT/engine/samples"
RUNS_DIR="$ROOT/fet-runs"
TIME_LIMIT="${TIME_LIMIT:-60}"

[[ -x "$BIN" ]] || { echo "خطأ: المحرّك غير موجود أو غير قابل للتنفيذ: $BIN" >&2; exit 1; }

echo "== المحرّك =="
"$BIN" --version || true
echo

fail=0
run_case() {
  local name="$1" file="$2" limit="$3"
  local out="$RUNS_DIR/$name"
  rm -rf "$out"; mkdir -p "$out"

  echo "== الحالة: $name (حد زمني ${limit}s) =="
  set +e
  "$BIN" --inputfile="$SAMPLES_DIR/$file" \
         --outputdir="$out" \
         --timelimitseconds="$limit" \
         --writetimetablesxml=true \
         --htmllevel=2 >"$out/stdout.log" 2>&1
  local status=$?
  set -e

  local results
  results="$(find "$out" -type f -name '*.html' | wc -l)"
  local xml
  xml="$(find "$out" -type f -name '*_activities.xml' | wc -l)"

  echo "   exit=$status  html=$results  activities_xml=$xml"
  tail -3 "$out/stdout.log" | sed 's/^/   | /'

  if [[ $status -ne 0 ]]; then
    echo "   ✗ فشل التشغيل" >&2; fail=1; return
  fi
  if [[ "$xml" -lt 1 ]]; then
    echo "   ✗ لم يُنتج ملف activities XML" >&2; fail=1; return
  fi
  echo "   ✓ نجح"
  echo
}

# ملفات نموذجية حقيقية مأخوذة من أمثلة FET الرسمية
run_case "small-test"               "small-test.fet"               "$TIME_LIMIT"
run_case "denmark-small-school"     "denmark-small-school.fet"     "$TIME_LIMIT"
run_case "algeria-univ-exam-s1"     "algeria-univ-exam-s1.fet"     "$TIME_LIMIT"

if [[ $fail -ne 0 ]]; then
  echo "بعض الحالات فشلت. راجع السجلات في: $RUNS_DIR" >&2
  exit 1
fi

echo "كل الحالات نجحت. المخرجات في: $RUNS_DIR"
