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
- ويُبقي الواجهة الكلاسيكية الأصلية كما هي في صفحة مستقلة (لا تُحذف أي وظيفة).

كل عناصر الشريط الجانبي تُشغّل إجراءات FET الأصلية بالاسم؛ فلا يوجد منطق مكرّر.
*/

class QMainWindow;

namespace UiShell {
	// يُثبّت الشريط الجانبي ولوحة البداية داخل النافذة. آمن للنداء مرة واحدة
	// بعد setupUi؛ والنداء الثاني لا يفعل شيئًا.
	void install(QMainWindow* window);

	// يعيد اللوحة إلى الصفحة الرئيسية (لوحة البداية) إن كانت مثبّتة.
	void showHome(QMainWindow* window);
}

#endif // UI_SHELL_H
