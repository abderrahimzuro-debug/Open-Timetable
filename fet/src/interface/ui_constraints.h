#ifndef UI_CONSTRAINTS_H
#define UI_CONSTRAINTS_H

/*
File ui_constraints.h — Open-Timetable addition (المرحلة 4 — القسم الأول)

This file is part of Open-Timetable, a modernised user interface for FET.
FET is free software licensed under the GNU Affero General Public License
version 3. See the LICENSE file in the root of this repository.

محرّر القيود بالقوالب — «الأساسيات»:
تغطية القيود التي تستخدمها المؤسسة فعليًا (غير متاح، أقصى/أدنى ساعات يومية،
فراغات، أقصى أيام أسبوعيًا، قاعة القسم) في نموذج واحد بسيط بالعربية.

المبدأ الحاكم:
- نبني كائنات قيود FET الأصلية نفسها ونضيفها عبر `gt.rules.addTimeConstraint()`
  و`gt.rules.addSpaceConstraint()` — **لا تعديل على المحرّك ولا صيغة ملف جديد**.
- كل ما يمكن عمله هنا يمكن أيضًا من الواجهة الكلاسيكية (لم نفقد أي قدرة).
- القسم الثاني (الضروريات) سيضيف الأنواع المتقدّمة على نفس الأساس.
*/

class QWidget;

namespace UiConstraints {

	// يفتح محرّر القيود بالقوالب. يعيد true إن أُضيف قيد واحد على الأقل.
	bool run(QWidget* parent);

	// عدد القوالب المتوفّرة في «الأساسيات» (يُستخدم في الفحص الذاتي).
	int templateCount();

}

#endif // UI_CONSTRAINTS_H
