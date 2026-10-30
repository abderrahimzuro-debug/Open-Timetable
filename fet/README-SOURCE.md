# شجرة مصدر FET داخل هذا المستودع (نسخة معدّلة)

هذا المجلد (`fet/`) نسخة من **كود FET المصدري** الإصدار **7.12.0**، أُدخلت إلى
المستودع حتى نتمكّن من **تحديث الواجهة الرسومية في مكانها** (وهذا ما اخترته).

## المصدر والأصالة

- الأصل: `vendor/fet/fet-7.12.0.tar.xz` (أرشيف رسمي موقّع، بصمته `ff3d1e0b…b697cd4`).
- أُدخلت عبر: `scripts/fet-import-src.sh` (يستخرج فقط: الملفات العلوية + `src/` + `icons/` + `licenses/`).
- **لم** نُدخل: `translations/` (190MB) و`examples/` (142MB) و`doc/` — تُبنى الترجمة العربية عند
  الحاجة بـ `scripts/fet-build-translations.sh ar`.
- الرخصة: **AGPL-3.0** — انظر `LICENSE` في هذا المجلد و`NOTICE.md` في الجذر.

## تعديلات Open-Timetable على الشجرة (حتى الآن)

| الملف | التعديل |
| --- | --- |
| `src/interface/ui_theme.h` | **جديد** — واجهة نظام المظهر الحديث. |
| `src/interface/ui_theme.cpp` | **جديد** — نمط Fusion + خط عربي + ورقة أنماط (QSS) فاتحة وداكنة. |
| `src/interface/ui_shell.h` | **جديد** — واجهة الشريط الجانبي/الصفحات. |
| `src/interface/ui_shell.cpp` | **جديد** — الشريط الجانبي + QStackedWidget + الإبقاء على الواجهة الكلاسيكية. |
| `src/interface/ui_dashboard.h` | **جديد** — واجهة لوحة البداية. |
| `src/interface/ui_dashboard.cpp` | **جديد** — البطاقات الإحصائية والأزرار السريعة. |
| `src/interface/ui_publish.h` | **جديد** — واجهة ميزة «نشر الجداول إلى مجلد مشترك». |
| `src/interface/ui_publish.cpp` | **جديد** — توليد كل الملفات + مجلد مؤرّخ + صفحة بداية عربية + README. |
| `src/interface/fetmainform_template.ui` | إضافة إجراء `filePublishToShareAction` داخل قائمة Export. |
| `src/interface/fetmainform.cpp` | ربط إجراء النشر، واستدعاء `UiShell::install(this)` بعد `setupUi`. |
| `src/interface/fet.cpp` | استدعاء `UiTheme::apply()` بعد إنشاء `QApplication`، وجعل اللغة الافتراضية `ar`. |
| `src/src.pro` | إضافة الملفات الجديدة إلى `SOURCES` و`HEADERS`، وإضافة `/utf-8` لـ MSVC. |
| `src/interface/CMakeLists.txt` | إضافة الملفات الجديدة، وإضافة `/utf-8` لـ MSVC. |

قاعدة العمل: **لا نعدّل منطق المحرّك** (`src/engine`, `src/cl`). التعديلات محصورة في الواجهة
(`src/interface`) وفي ملفات البناء.

## كيف نعرف ما تغيّر عن الأصل؟

```bash
# 1) استخراج نسخة أصلية نظيفة للمقارنة
scripts/fet-import-src.sh --force    # يحذف fet/ ويعيدها من الأرشيف (يفقد تعديلاتنا!)
# أو الأفضل: قارن مع نسخة مؤقتة
mkdir -p /tmp/upstream && tar -xJf vendor/fet/fet-7.12.0.tar.xz -C /tmp/upstream \
  fet-7.12.0/src fet-7.12.0/CMakeLists.txt fet-7.12.0/fet.pro
diff -ru /tmp/upstream/fet-7.12.0/src fet/src | less
```

## البناء

```bash
# الواجهة + المحرّك (Qt 5.15 على Debian/Ubuntu)
sudo apt install build-essential qtbase5-dev qtbase5-dev-tools qttools5-dev-tools
mkdir -p build/gui && cd build/gui
qmake ../../fet/fet.pro CONFIG+=release
make -j"$(nproc)"
# الناتج: fet (الواجهة) و fet-cl (سطر الأوامر)

# ترجمة عربية
cd ../.. && scripts/fet-build-translations.sh ar   # → build/translations/fet_ar.qm
# ضع fet_ar.qm بجانب تنفيذي fet (أو في مجلد translations بجانبه)
```

ملاحظة: مسار CMake في FET يشترط Qt 6.10+، أما مسار qmake فيعمل مع Qt 5.15 (وهو ما نستخدمه
في CI لتسريع البناء). الكود متوافق مع Qt5 وQt6.

## تحديث FET مستقبلًا

عند صدور إصدار جديد: حمّل الأرشيف إلى `vendor/fet/` (حدّث `SHA256SUMS`)، ثم
`scripts/fet-import-src.sh --force` ثم أعِد تطبيق تعديلاتنا من الجدول أعلاه (أو استخدم
`git diff` بين النسختين).
