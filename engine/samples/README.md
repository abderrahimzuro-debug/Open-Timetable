# ملفات نموذجية لتجربة المحرّك (FET .fet)

هذه الملفات مأخوذة كما هي من أمثلة FET الرسمية (الإصدار 7.12.0) وتُستخدم في
`scripts/fet-smoke-test.sh` للتأكد من أن المحرّك يعمل ويُنتج جداول صحيحة.

| الملف | المصدر داخل أرشيف FET | الوصف |
| --- | --- | --- |
| `small-test.fet` | `examples/tests/2025-10-18-activities-max-number-of-students/small-test.fet` | أصغر ملف اختبار (~7KB) — سريع جدًا، مناسب للتحقق من البناء |
| `denmark-small-school.fet` | `examples/FET-5-official/Denmark/small-school.fet` | مدرسة دنماركية صغيرة (~23KB) — حالة مدرسية واقعية |
| `algeria-univ-exam-s1.fet` | `examples/FET-5-official/Algeria/Mechanical-Batna_Univ/EXAM-2013-2014-S1.fet` | جدول امتحانات جامعية (قسم الميكانيك، باتنة) — حالة جزائرية واقعية |

> ملاحظة تقنية: الملفات تبدأ بعلامة BOM (`\uFEFF`) قبل `<?xml ...>`، وهذا شائع في ملفات FET
> المولّدة على ويندوز. أي قارئ XML نكتبه يجب أن يتعامل معها.

## صيغة ملف `.fet`

ملف XML جذره `<fet version="...">` ويحتوي أقسامًا مثل:

```xml
<fet version="5.41.0">
  <Institution_Name>...</Institution_Name>
  <Comments>...</Comments>
  <Days_List><Number_of_Days>5</Number_of_Days><Day><Name>Monday</Name></Day>...</Days_List>
  <Hours_List><Number_of_Hours>7</Number_of_Hours><Hour><Name>1</Name></Hour>...</Hours_List>
  <Subjects_List>...</Subjects_List>
  <Teachers_List>...</Teachers_List>
  <Students_List>...</Students_List>   <!-- Years / Groups / Subgroups -->
  <Activities_List>...</Activities_List>
  <Buildings_List>...</Buildings_List>
  <Rooms_List>...</Rooms_List>
  <Time_Constraints_List>...</Time_Constraints_List>
  <Space_Constraints_List>...</Space_Constraints_List>
</fet>
```

مخرجات `fet-cl` تُكتب في: `<outputdir>/timetables/<اسم-الملف>/` وتشمل HTML لكل صف/أستاذ/قاعة
وملف `<الاسم>_activities.xml` (يُستخدم لتبادل البيانات مع أدوات أخرى).
