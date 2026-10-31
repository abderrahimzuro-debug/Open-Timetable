#ifndef UI_ICONS_H
#define UI_ICONS_H

/*
File ui_icons.h — Open-Timetable addition

This file is part of Open-Timetable, a modernised user interface for the
timetabling engine. Licensed under the GNU Affero General Public License v3.

أيقونات احترافية بديلة عن الإيموجي:
- تُرسم داخل البرنامج بالمتجهات (QPainter) داخل شبكة 24×24، بخطوط موحّدة
  السماكة ونهايات مستديرة — فتظهر حادّة على أي دقّة شاشة وبأي حجم.
- لا تحتاج ملفات صور ولا موارد (qrc) ولا إضافة رسم SVG: لا شيء يمكن أن ينكسر
  عند النقل بين ويندوز ولينكس.
- تتلوّن حسب المظهر: فاتح أو داكن، وتُعاد الرسم عند تبديل المظهر.

الاستخدام:
    QIcon icon = UiIcons::icon("teachers");
    QIcon logo = UiIcons::logo();
*/

#include <QIcon>
#include <QString>
#include <QStringList>

namespace UiIcons {

	// لون الرسم يتبع المظهر الحالي (يُضبط من UiTheme عند تبديل المظهر).
	enum Tone {
		OnLight = 0, // رسم داكن على خلفية فاتحة
		OnDark       // رسم فاتح على خلفية داكنة
	};

	void setTone(Tone tone);
	Tone tone();

	// أيقونة باسم معيّن؛ إن لم يوجد الاسم يُعاد رمز دائري عام (لا أيقونة فارغة).
	QIcon icon(const QString& name, int size = 20);

	// شعار البرنامج (يُستعمل أيقونة للنافذة وشريط المهام).
	QIcon logo(int size = 64);

	// أسماء الأيقونات المتوفّرة (للفحص الذاتي).
	QStringList names();
}

#endif // UI_ICONS_H
