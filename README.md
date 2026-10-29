# Open-Timetable

[![FET engine (build + smoke test)](https://github.com/abderrahimzuro-debug/Open-Timetable/actions/workflows/engine.yml/badge.svg?branch=arena%2Ff9075371-open-timetable)](https://github.com/abderrahimzuro-debug/Open-Timetable/actions/workflows/engine.yml)

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
├── fet/                    كود FET المعدَّل (واجهة حديثة: مظهر + عربية)
├── vendor/fet/             FET 7.12.0 كأرشيف رسمي موقّع (AGPL-3.0)
├── engine/samples/         ملفات .fet حقيقية للاختبار
├── scripts/                تحقق، فكّ، استيراد، بناء، ترجمة، اختبار
├── docker/                 بناء المحرّك في حاوية
├── docs/                   العوائق + التصميم + خطة تحديث الواجهة
└── .github/workflows/      بناء واختبار آلي (المحرّك + واجهتنا)
```

## تحديث الواجهة (المرحلة الحالية)

اخترت تحديث واجهة Qt في مكانها. أُنجز في هذه الجولة:

- **مظهر حديث موحّد** (`fet/src/interface/ui_theme.*`): نمط Fusion، ألوان عصرية، QSS للأزرار
  والحقول والجداول والتبويبات والقوائم، وخط عربي تلقائي.
- **العربية افتراضيًا** + دعم RTL (كان موجودًا في FET، وفعّلناه كافتراضي).
- **بناء الترجمة العربية** (`scripts/fet-build-translations.sh ar` → `fet_ar.qm`).
- **CI يبني واجهتنا** على Linux (Qt5) وويندوز (Qt6) للتأكد أن كل تعديل يُصرَّف.

الخطة التفصيلية والمراحل القادمة: `docs/UI-PLAN.md`. مصدر الشجرة وتعديلاتنا: `fet/README-SOURCE.md`.

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

## ما تم التحقق منه فعليًا

سير CI يبني محرّك FET من الأرشيف المحفوظ (Qt 5.15 + qmake)، ويشغّله على 3 ملفات `.fet` حقيقية،
ويتحقق من إنتاج الجداول — وكل ذلك ينجح. كما يتحقق أن الواجهة الرسومية الأصلية تُبنى أيضًا بـ Qt5.

## الرخصة

هذا المشروع تحت **GNU AGPL v3** (راجع `LICENSE`)، لأن FET المرتبط به مرخّص بالرخصة نفسها.
التفاصيل والالتزامات في `NOTICE.md`.
