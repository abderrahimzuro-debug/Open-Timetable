# Notice — Third-party components

يحتوي هذا المستودع على برنامج طرف ثالث محفوظ كأرشيف رسمي غير معدَّل.

## FET — Free Timetabling Software

| | |
| --- | --- |
| Component | FET (Free Timetabling Software) |
| Version | 7.12.0 (released 2026-10-14) |
| Copyright | Gheorghe Liviu Lalescu and contributors |
| License | GNU Affero General Public License v3.0 (AGPL-3.0) |
| Upstream site | https://lalescu.ro/liviu/fet/ |
| Upstream source | https://github.com/lalesculiviu/fet (file `fet-7.12.0.tar.xz`) |
| Stored in repo | `vendor/fet/fet-7.12.0.tar.xz` (+ `.asc` detached signature) |
| SHA-256 | `ff3d1e0be7708a648f6c4a5078f49d0b0926283c065791105229753c7b697cd4` |
| Full license text | copied to `LICENSE` in this repository |

الأرشيف غير معدَّل. الملفات النموذجية في `engine/samples/` مأخوذة كما هي من أمثلة FET الرسمية
وتحمل الرخصة نفسها.

### الالتزامات المترتبة (AGPL-3.0)

1. **عند إعادة التوزيع:** يجب توزيع نص الرخصة (`LICENSE`) وإتاحة الكود المصدري الكامل (بما فيه أي
   تعديل نجريه على FET).
2. **عند التشغيل كخدمة عبر الشبكة:** يجب إتاحة الكود المصدري المقابل لمستخدمي الخدمة. لذلك يُنشر
   هذا المستودع — بما فيه واجهة الويب والخادم — تحت AGPL-3.0 أيضًا.
3. **عند تعديل كود FET:** تُوثَّق التعديلات كـ patch فوق الأرشيف (وليس بتغيير الأرشيف نفسه) حتى
   تبقى الأصالة قابلة للتحقق بالبصمة، وحتى يكون التعديل واضحًا وقابلًا للمراجعة.

### تنبيه

`vendor/fet/` غير مرتبط رسميًا بمشروع Open-Timetable؛ هو نسخة موقّعة من مشروع FET الأصلي.
أي إشكال في تسمية أو علامة تجارية يخصّ المشروع الأصلي.
