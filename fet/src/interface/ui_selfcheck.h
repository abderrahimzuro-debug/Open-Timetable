#ifndef UI_SELFCHECK_H
#define UI_SELFCHECK_H

/*
File ui_selfcheck.h — Open-Timetable addition

This file is part of Open-Timetable, a modernised user interface for FET.
FET is free software licensed under the GNU Affero General Public License
version 3. See the LICENSE file in the root of this repository.

فحص ذاتي للواجهة الجديدة، يعمل بلا شاشة (QT_QPA_PLATFORM=offscreen) للتأكد
من أن: الهيكل الجديد مثبّت، الشريط الجانبي مبنيّ، الصفحتان موجودتان، لوحة
البداية تعمل، والمظهر مطبّق. يُستعمل في CI وفي تشخيص مشاكل التثبيت.

الاستخدام: fet --ot-selfcheck   (يطبع تقريرًا ويعيد 0 عند النجاح)
*/

class QMainWindow;

namespace UiSelfCheck {
	// ينفّذ الفحص ويطبع التقرير على المخرجات القياسية.
	// يعيد 0 إذا نجحت كل الفحوص، وإلا عدد الإخفاقات.
	int run(QMainWindow* mainWindow);
}

#endif // UI_SELFCHECK_H
