#ifndef UI_THEME_H
#define UI_THEME_H

/*
File ui_theme.h — Open-Timetable addition

This file is part of Open-Timetable, a modernised user interface for FET.
FET is free software licensed under the GNU Affero General Public License
version 3. See the LICENSE file in the root of this repository.

المسؤولية: تطبيق المظهر الحديث (خط، ألوان، أنماط عناصر الواجهة) على التطبيق
كله مرة واحدة، بحيث لا نحتاج لتعديل مئات الملفات لتغيير الشكل.
*/

#include <QString>
#include <QStringList>

class QApplication;

namespace UiTheme {
	// يطبّق نمط Fusion + لوحة ألوان عصرية + ورقة أنماط (QSS) على التطبيق.
	void apply(QApplication& app);

	// يطبّق مظهرًا معيّنًا فورًا على تطبيق يعمل («light» أو «dark»).
	// يستبدل اللوحة والألوان وورقة الأنماط، ويضبط ألوان الأيقونات لتتبع المظهر.
	void applyTheme(QApplication& app, const QString& name);

	// يعيد نص ورقة الأنماط لاسم مظهر معيّن ("light" أو "dark").
	QString styleSheet(const QString& name = QStringLiteral("light"));

	// أسماء المظاهر المتاحة.
	QStringList availableThemes();
}

#endif // UI_THEME_H
