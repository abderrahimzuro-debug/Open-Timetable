#ifndef UI_I18N_H
#define UI_I18N_H

/*
File ui_i18n.h — Open-Timetable addition

This file is part of Open-Timetable, a modernised user interface for FET.
FET is free software licensed under the GNU Affero General Public License
version 3. See the LICENSE file in the root of this repository.

ترجمة نصوص واجهتنا الجديدة:
- FET يترجم نصوصه من ملفات .qm (نحو 40 لغة).
- واجهتنا الجديدة (الشريط الجانبي، لوحة البداية، حوارات النشر والتوليد) نصوصها
  مضمنة في الكود؛ لذلك نستعمل T("English", "العربية") التي تختار النص حسب
  لغة الواجهة الحالية (FET_LANGUAGE). بهذا يتبدّل كل شيء فورًا عند تغيير اللغة
  دون الحاجة إلى إعادة تشغيل، ودون ملفات .ts إضافية.
*/

#include <QString>
#include <QStringList>

extern QString FET_LANGUAGE; // معرّف اللغة الحالي في FET (مثل "ar" أو "en_US")

namespace UiI18n {

	// اللغة الحالية عربية؟ (أي لغة تبدأ بـ "ar")
	inline bool isArabic()
	{
		return FET_LANGUAGE.startsWith(QStringLiteral("ar"));
	}

	// يختار النص العربي أو الإنجليزي حسب اللغة الحالية.
	inline QString T(const char* english, const char* arabic)
	{
		return isArabic() ? QString::fromUtf8(arabic) : QString::fromUtf8(english);
	}

	// اسم اللغة بالعربية/الإنجليزية للعرض (اختياري للاستخدام في رسائل الحالة).
	inline QString languageName()
	{
		return FET_LANGUAGE;
	}
}

#endif // UI_I18N_H
