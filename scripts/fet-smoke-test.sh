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

# عند 1: نتحقّق أيضًا من أن ملفات HTML المصدَّرة تحمل خطًّا صريحًا (وإلا يعرض
# المتصفّح الحروف اللاتينية رموزًا يونانية على بعض أجهزة ويندوز).
REQUIRE_HTML_FONT="${REQUIRE_HTML_FONT:-0}"
# عند 1: نُصدّر أيضًا بلغة عربية للتأكد من اتجاه RTL وترجمة النصوص
CHECK_ARABIC="${CHECK_ARABIC:-0}"

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
  if [[ "$REQUIRE_HTML_FONT" == "1" ]]; then
    local sample
    sample="$(find "$out" -type f -name '*.html' | head -1)"
    if [[ -z "$sample" ]]; then
      # لا نُفشل الحالة إن لم يُنتج المحرّك HTML، لكن نُنبّه بوضوح.
      echo "   ! لم يُنتج هذا التشغيل ملف HTML — تخطّي فحص الخط"
    else
      if grep -q "font-family:" "$sample"; then
        echo "   ✓ الخط صريح في HTML المصدَّر"
      else
        echo "   ✗ HTML المصدَّر بلا خط صريح (خطر ظهور رموز يونانية بدل الحروف)" >&2
        fail=1
      fi
      # ملف الأنماط الخارجي أيضًا يجب أن يحمل الخط
      local css
      css="$(find "$out" -type f -name '*stylesheet.css' | head -1)"
      if [[ -n "$css" ]] && ! grep -q "font-family:" "$css"; then
        echo "   ✗ ملف الأنماط بلا خط صريح" >&2
        fail=1
      fi
    fi
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
