# Open-Timetable

مشروع مفتوح المصدر لإنشاء وإدارة الجداول الدراسية، بواجهة عربية حديثة، مبني على محرّك
[FET](https://lalescu.ro/liviu/fet/) المعروف بقوة نتائجه.

## الحالة الحالية

المرحلة 1 (تهيئة البنية التحتية) — **منجَزة**: المصدر الرسمي لـ FET محفوظ وموثّق، وسكربتات
البناء والاختبار وسير CI جاهزة. الخطوة التالية هي الواجهة (راجع `docs/ARCHITECTURE.md`).

اقرأ أولًا:
- **`docs/BLOCKERS.md`** — كل العوائق التي كانت تعيق العمل، وما أُصلح منها وما ينتظر قرارك.
- **`docs/ARCHITECTURE.md`** — التصميم المقترح وخطة التنفيذ.
- `vendor/fet/README.md` — تفاصيل محرّك FET المحفوظ (إصدار، بصمة، بناء، رخصة).

## البنية

```
Open-Timetable/
├── vendor/fet/             FET 7.12.0 كأرشيف رسمي موقّع (AGPL-3.0)
├── engine/samples/         ملفات .fet حقيقية للاختبار
├── scripts/                تحقق، فكّ، بناء، اختبار الدخان
├── docker/                 بناء المحرّك في حاوية
├── docs/                   تقرير العوائق + التصميم
└── .github/workflows/      بناء واختبار آلي للمحرّك
```

## أوامر سريعة

```bash
# 1) التحقق من سلامة الأرشيف
./scripts/fet-verify.sh

# 2) فكّ المصدر إلى .fet-src/ (مستثنى من Git)
./scripts/fet-extract.sh

# 3) بناء المحرّك (يحتاج Qt 5.15: sudo apt install build-essential qtbase5-dev)
./scripts/fet-build-cl.sh

# 4) اختبار التوليد على نماذج حقيقية
./scripts/fet-smoke-test.sh

# بديل بلا Qt على جهازك: البناء في حاوية
docker build -f docker/engine.Dockerfile -t open-timetable-engine .
docker run --rm -v "$PWD/engine/samples:/in:ro" -v "$PWD/fet-runs:/out" \
  open-timetable-engine fet-cl --inputfile=/in/small-test.fet --outputdir=/out --timelimitseconds=60
```

## الرخصة

هذا المشروع تحت **GNU AGPL v3** (راجع `LICENSE`)، لأن FET المرتبط به مرخّص بالرخصة نفسها.
التفاصيل والالتزامات في `NOTICE.md`.
