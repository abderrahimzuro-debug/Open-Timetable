#ifndef UI_PUBLISH_H
#define UI_PUBLISH_H

/*
File ui_publish.h — Open-Timetable addition

This file is part of Open-Timetable, a modernised user interface for FET.
FET is free software licensed under the GNU Affero General Public License
version 3. See the LICENSE file in the root of this repository.

"نشر إلى مجلد مشترك" — الخيار (أ) لخدمة الأدوار الثلاثة دون خادم:
المدير يولّد الجدول ثم ينشره في مجلد على شبكة المؤسسة (أو أي مجلد)، فيجد
الأساتذة والطلاب جداولهم في صفحة بداية عربية جاهزة للتصفح والطباعة.
*/

#include <QString>

class QWidget;

namespace UiPublish {
	// يفتح مربّع اختيار مجلد، يولّد كل الجداول (أساتذة/أفواج/قاعات/مواد/XML/CSV)
	// داخل مجلد جديد مؤرّخ، وينشئ index.html عربيًا + README.txt.
	void run(QWidget* parent);

	// نشر تلقائي بلا أي حوار: يستعمل آخر مجلد نشر محفوظ.
	// يعيد true عند النجاح، ويضع مسار مجلد النشر في targetPath إن طُلب.
	bool runAutomatic(QWidget* parent, QString* targetPath = nullptr);

	// هل البيانات جاهزة للنشر؟ (يُستخدم لتفعيل/تعطيل الزر أو للاختبار)
	bool canPublish();

	// اسم المجلد المقترح (بدون مسار): <اسم المؤسسة>-<التاريخ>
	QString suggestedFolderName();
}

#endif // UI_PUBLISH_H
