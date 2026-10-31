#ifndef UI_SHELL_H
#define UI_SHELL_H

/*
File ui_shell.h — Open-Timetable addition

This file is part of Open-Timetable, a modernised user interface for FET.
FET is free software licensed under the GNU Affero General Public License
version 3. See the LICENSE file in the root of this repository.

"الهيكل" (Shell): يُضيف إلى النافذة الرئيسية
- شريط تنقّل جانبي حديث (بدل البحث في القوائم المتشعّبة)،
- لوحة بداية (UiDashboard)،
- زرًا أعلى الشريط لطيّه، وزرًا لتبديل المظهر الفاتح/الداكن.
الواجهة الكلاسيكية لم تبقَ شاشةً: أُخرجت من النافذة نهائيًا؛ كائناتها وإجراءاتها
تبقى حيّة حتى لا تُفقد أي وظيفة، لكن لا يظهر منها شيء للمستخدم.

كل عناصر الشريط الجانبي تُشغّل إجراءات المحرّك الأصلية بالاسم؛ فلا يوجد منطق مكرّر.
كل نصوص الشريط واللوحة تُترجم عبر UiI18n::T() فتُحدَّث فورًا عند تغيير اللغة.
*/

class QMainWindow;

namespace UiShell {
	// يُثبّت الشريط الجانبي ولوحة البداية داخل النافذة. آمن للنداء مرة واحدة
	// بعد setupUi؛ والنداء الثاني لا يفعل شيئًا.
	void install(QMainWindow* window);

	// يعيد اللوحة إلى الصفحة الرئيسية (لوحة البداية) إن كانت مثبّتة.
	void showHome(QMainWindow* window);

	// يُحدّث بطاقات لوحة البداية (بعد إضافة قيود أو تغيير البيانات مثلًا).
	void refreshDashboard(QMainWindow* window);

	// يُلائم النافذة مع مساحة الشاشة المتاحة (1366×768 مثلًا): يُصغّرها إن كانت
	// أكبر من الشاشة ويُعيد توسيطها، فيبقى كل شيء ظاهرًا بلا إخفاء شريط المهام.
	void fitToScreen(QMainWindow* window);

	// يُعيد بناء نصوص الشريط الجانبي واللوحة حسب لغة الواجهة الحالية.
	// يُستدعى تلقائيًا عند تغيير اللغة (انظر fet.cpp).
	void retranslate(QMainWindow* window);
}

#endif // UI_SHELL_H
