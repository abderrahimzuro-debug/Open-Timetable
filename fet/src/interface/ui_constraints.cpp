/*
File ui_constraints.cpp — Open-Timetable addition (المرحلة 4)

This file is part of Open-Timetable, a modernised user interface for FET.
FET is free software; you can redistribute it and/or modify it under the terms
of the GNU Affero General Public License version 3 (see LICENSE).

محرّر القيود بالقوالب — قسمان:

القسم الأول — الأساسيات (15 قالبًا): ما تستخدمه المؤسسة كل يوم
  أستاذ/قسم غير متاح • أقصى وأدنى ساعات يومية • فراغات يومية • أقصى أيام
  أسبوعيًا • قاعة القسم الثابتة.

القسم الثاني — الضروريات (14 قالبًا): الضبط الدقيق الذي تحتاجه المؤسسة أحيانًا
  أدنى ساعات/أيام الأستاذ • أقصى ساعات متواصلة • أقصى امتداد يومي (span)
  • بدايات مبكرة في الحصة الثانية • قاعة غير متاحة • قاعة مفضّلة لمادة/وسم
  • قاعات المادة المتعدّدة.

الطريقة: نُنشئ كائنات FET الأصلية ونضيفها بـ addTimeConstraint/addSpaceConstraint،
ثم addUndoPoint — أي أن القيود تظهر في «كل القيود الزمنية/المكانية» وتُصدَّر في
ملف .fet كأي قيد أُضيف يدويًا. لا تعديل على المحرّك.

النافذة فيها:
  - بحث فوري في القوالب (29 قالبًا)،
  - قائمة مقسّمة إلى «الأساسيات» و«الضروريات»،
  - قائمة حيّة بالقيود الموجودة من نوع القالب المختار.
*/

#include "ui_constraints.h"

#include "ui_i18n.h"
#include "ui_shell.h"

#include "fet.h"
#include "rules.h"
#include "timeconstraint.h"
#include "spaceconstraint.h"
#include "teacher.h"
#include "studentsset.h"
#include "room.h"
#include "subject.h"
#include "activitytag.h"
#include "activity.h"
#include "utilities.h"
#include "fetmainform.h"

#include <QAction>
#include <QBrush>
#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QDialog>
#include <QFont>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QSet>
#include <QStringList>
#include <QListWidget>
#include <QMainWindow>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QVariant>

#include <utility>

namespace {

enum Section {
	SEC_ESSENTIALS = 0, // الأساسيات
	SEC_NECESSITIES,    // الضروريات
	SEC_ACTIVITIES      // الأنشطة (قسم ثالث)
};

enum TemplateId {
	// ---------- القسم الأول: الأساسيات ----------
	T_TEACHER_NOT_AVAILABLE = 0,
	T_STUDENTS_NOT_AVAILABLE,
	T_TEACHER_MAX_HOURS_DAILY,
	T_TEACHERS_MAX_HOURS_DAILY,
	T_STUDENTS_SET_MAX_HOURS_DAILY,
	T_STUDENTS_MAX_HOURS_DAILY,
	T_STUDENTS_SET_MIN_HOURS_DAILY,
	T_STUDENTS_MIN_HOURS_DAILY,
	T_TEACHERS_MAX_GAPS_PER_DAY,
	T_TEACHER_MAX_GAPS_PER_DAY,
	T_STUDENTS_SET_MAX_GAPS_PER_DAY,
	T_STUDENTS_MAX_GAPS_PER_DAY,
	T_TEACHER_MAX_DAYS_PER_WEEK,
	T_TEACHERS_MAX_DAYS_PER_WEEK,
	T_STUDENTS_SET_HOME_ROOM,
	// ---------- القسم الثاني: الضروريات ----------
	T_TEACHER_MIN_HOURS_DAILY,
	T_TEACHER_MIN_DAYS_PER_WEEK,
	T_TEACHER_MAX_HOURS_CONTINUOUSLY,
	T_TEACHER_MAX_SPAN_PER_DAY,
	T_STUDENTS_SET_MAX_HOURS_CONTINUOUSLY,
	T_STUDENTS_MAX_HOURS_CONTINUOUSLY,
	T_STUDENTS_SET_MAX_SPAN_PER_DAY,
	T_STUDENTS_MAX_SPAN_PER_DAY,
	T_STUDENTS_EARLY_MAX_BEGINNINGS,
	T_STUDENTS_SET_EARLY_MAX_BEGINNINGS,
	T_ROOM_NOT_AVAILABLE,
	T_SUBJECT_PREFERRED_ROOM,
	T_ACTIVITY_TAG_PREFERRED_ROOM,
	T_SUBJECT_PREFERRED_ROOMS,
	// ---------- القسم الثالث: الأنشطة ----------
	T_ACTIVITIES_NOT_OVERLAPPING,
	T_ACTIVITIES_SAME_STARTING_TIME,
	T_ACTIVITIES_SAME_STARTING_DAY,
	T_MIN_DAYS_BETWEEN_ACTIVITIES,
	T_ACTIVITY_PREFERRED_ROOM,
	T_ACTIVITY_PREFERRED_ROOMS,
	T_COUNT
};

// ما يحتاجه كل قالب من حقول
struct TemplateSpec {
	int id;
	int section;
	bool needsTeacher;
	bool needsStudents;
	bool needsSubject;
	bool needsActivityTag;
	bool needsRoom;
	bool needsRoomList;
	bool needsValue;
	bool needsTimes;
	bool needsAllowEmptyDays;
	bool needsExceptCheck;
	bool needsActivities;
	bool singleActivity;
	bool needsConsecutiveCheck;
	bool needsPermanentCheck;
	int valueDefault;
	QList<int> fetTypes; // أنواع FET لعرض «القيود الموجودة» من النوع نفسه
};

QString sectionTitle(int section)
{
	switch (section) {
	case SEC_ESSENTIALS:
		return UiI18n::T("Essentials", "الأساسيات");
	case SEC_NECESSITIES:
		return UiI18n::T("Necessities (fine tuning)", "الضروريات (ضبط دقيق)");
	case SEC_ACTIVITIES:
		return UiI18n::T("Activities (pick specific lessons)", "الأنشطة (اختيار حصص بعينها)");
	default:
		return QString();
	}
}

QString templateTitle(int id)
{
	switch (id) {
	case T_TEACHER_NOT_AVAILABLE:
		return UiI18n::T("Teacher not available at times", "أستاذ غير متاح في أوقات");
	case T_STUDENTS_NOT_AVAILABLE:
		return UiI18n::T("Class or group not available at times", "قسم أو فوج غير متاح في أوقات");
	case T_TEACHER_MAX_HOURS_DAILY:
		return UiI18n::T("Max hours daily — one teacher", "أقصى ساعات يومية — لأستاذ");
	case T_TEACHERS_MAX_HOURS_DAILY:
		return UiI18n::T("Max hours daily — all teachers", "أقصى ساعات يومية — لكل الأساتذة");
	case T_STUDENTS_SET_MAX_HOURS_DAILY:
		return UiI18n::T("Max hours daily — one class", "أقصى ساعات يومية — لقسم");
	case T_STUDENTS_MAX_HOURS_DAILY:
		return UiI18n::T("Max hours daily — all classes", "أقصى ساعات يومية — لكل الأقسام");
	case T_STUDENTS_SET_MIN_HOURS_DAILY:
		return UiI18n::T("Min hours daily — one class", "أدنى ساعات يومية — لقسم");
	case T_STUDENTS_MIN_HOURS_DAILY:
		return UiI18n::T("Min hours daily — all classes", "أدنى ساعات يومية — لكل الأقسام");
	case T_TEACHERS_MAX_GAPS_PER_DAY:
		return UiI18n::T("Max gaps daily — all teachers", "أقصى فراغات يومية — لكل الأساتذة");
	case T_TEACHER_MAX_GAPS_PER_DAY:
		return UiI18n::T("Max gaps daily — one teacher", "أقصى فراغات يومية — لأستاذ");
	case T_STUDENTS_SET_MAX_GAPS_PER_DAY:
		return UiI18n::T("Max gaps daily — one class", "أقصى فراغات يومية — لقسم");
	case T_STUDENTS_MAX_GAPS_PER_DAY:
		return UiI18n::T("Max gaps daily — all classes", "أقصى فراغات يومية — لكل الأقسام");
	case T_TEACHER_MAX_DAYS_PER_WEEK:
		return UiI18n::T("Max days per week — one teacher", "أقصى أيام أسبوعيًا — لأستاذ");
	case T_TEACHERS_MAX_DAYS_PER_WEEK:
		return UiI18n::T("Max days per week — all teachers", "أقصى أيام أسبوعيًا — لكل الأساتذة");
	case T_STUDENTS_SET_HOME_ROOM:
		return UiI18n::T("Home room for a class", "قاعة القسم الثابتة");

	// ---------- الضروريات ----------
	case T_TEACHER_MIN_HOURS_DAILY:
		return UiI18n::T("Min hours daily — one teacher", "أدنى ساعات يومية — لأستاذ");
	case T_TEACHER_MIN_DAYS_PER_WEEK:
		return UiI18n::T("Min days per week — one teacher", "أدنى أيام أسبوعيًا — لأستاذ");
	case T_TEACHER_MAX_HOURS_CONTINUOUSLY:
		return UiI18n::T("Max continuous hours — one teacher", "أقصى ساعات متواصلة — لأستاذ");
	case T_TEACHER_MAX_SPAN_PER_DAY:
		return UiI18n::T("Max span per day — one teacher", "أقصى امتداد يومي — لأستاذ");
	case T_STUDENTS_SET_MAX_HOURS_CONTINUOUSLY:
		return UiI18n::T("Max continuous hours — one class", "أقصى ساعات متواصلة — لقسم");
	case T_STUDENTS_MAX_HOURS_CONTINUOUSLY:
		return UiI18n::T("Max continuous hours — all classes", "أقصى ساعات متواصلة — لكل الأقسام");
	case T_STUDENTS_SET_MAX_SPAN_PER_DAY:
		return UiI18n::T("Max span per day — one class", "أقصى امتداد يومي — لقسم");
	case T_STUDENTS_MAX_SPAN_PER_DAY:
		return UiI18n::T("Max span per day — all classes", "أقصى امتداد يومي — لكل الأقسام");
	case T_STUDENTS_EARLY_MAX_BEGINNINGS:
		return UiI18n::T("Early beginnings at second hour — all classes",
				 "أقصى بدايات في الحصة الثانية — لكل الأقسام");
	case T_STUDENTS_SET_EARLY_MAX_BEGINNINGS:
		return UiI18n::T("Early beginnings at second hour — one class",
				 "أقصى بدايات في الحصة الثانية — لقسم");
	case T_ROOM_NOT_AVAILABLE:
		return UiI18n::T("Room not available at times", "قاعة غير متاحة في أوقات");
	case T_SUBJECT_PREFERRED_ROOM:
		return UiI18n::T("Preferred room for a subject", "قاعة مفضّلة لمادة");
	case T_ACTIVITY_TAG_PREFERRED_ROOM:
		return UiI18n::T("Preferred room for an activity tag", "قاعة مفضّلة لوسم نشاط");
	case T_SUBJECT_PREFERRED_ROOMS:
		return UiI18n::T("Allowed rooms for a subject (several)", "قاعات مادة (عدة قاعات مسموحة)");

	// ---------- الأنشطة ----------
	case T_ACTIVITIES_NOT_OVERLAPPING:
		return UiI18n::T("Activities must not overlap", "أنشطة لا تتزامن");
	case T_ACTIVITIES_SAME_STARTING_TIME:
		return UiI18n::T("Activities start at the same time", "أنشطة في نفس وقت البدء");
	case T_ACTIVITIES_SAME_STARTING_DAY:
		return UiI18n::T("Activities in the same day", "أنشطة في نفس اليوم");
	case T_MIN_DAYS_BETWEEN_ACTIVITIES:
		return UiI18n::T("Min days between activities", "أدنى أيام بين الأنشطة");
	case T_ACTIVITY_PREFERRED_ROOM:
		return UiI18n::T("Preferred room for one activity", "قاعة لنشاط واحد");
	case T_ACTIVITY_PREFERRED_ROOMS:
		return UiI18n::T("Allowed rooms for one activity (several)",
				 "قاعات نشاط واحد (عدة قاعات مسموحة)");
	default:
		return QString();
	}
}

QString templateHint(int id)
{
	switch (id) {
	case T_TEACHER_NOT_AVAILABLE:
		return UiI18n::T("Mark the slots when the teacher cannot teach.",
				 "حدّد الأوقات التي لا يستطيع الأستاذ التدريس فيها.");
	case T_STUDENTS_NOT_AVAILABLE:
		return UiI18n::T("Mark the slots when this class/group is unavailable (for example Friday afternoon).",
				 "حدّد الأوقات التي لا يكون فيها القسم متاحًا (مثلًا مساء الجمعة).");
	case T_TEACHER_MAX_HOURS_DAILY:
		return UiI18n::T("Stops the generator from giving this teacher more lessons in one day.",
				 "يمنع البرنامج من إعطاء هذا الأستاذ أكثر من العدد المحدّد في اليوم.");
	case T_TEACHERS_MAX_HOURS_DAILY:
		return UiI18n::T("The same daily maximum for every teacher.",
				 "الحدّ نفسه مطبَّق على جميع الأساتذة.");
	case T_STUDENTS_SET_MAX_HOURS_DAILY:
		return UiI18n::T("Useful for young classes or afternoons-only classes.",
				 "مفيد للأقسام الصغيرة أو الأقسام التي تدرس في الفترة المسائية.");
	case T_STUDENTS_MAX_HOURS_DAILY:
		return UiI18n::T("The same daily maximum for every class.",
				 "الحدّ نفسه مطبَّق على كل الأقسام.");
	case T_STUDENTS_SET_MIN_HOURS_DAILY:
		return UiI18n::T("Keeps the class busy: no day with fewer hours than this.",
				 "يمنع الأيام القصيرة: لا يوم بأقل من العدد المحدّد من الساعات.");
	case T_STUDENTS_MIN_HOURS_DAILY:
		return UiI18n::T("The same daily minimum for every class.",
				 "الحدّ الأدنى نفسه مطبَّق على كل الأقسام.");
	case T_TEACHERS_MAX_GAPS_PER_DAY:
		return UiI18n::T("Free periods between lessons. 0 means no gaps at all.",
				 "الفراغات بين الحصص. القيمة 0 تعني بلا فراغات إطلاقًا.");
	case T_TEACHER_MAX_GAPS_PER_DAY:
		return UiI18n::T("Limits the free periods in this teacher's day.",
				 "يحدّد عدد الفراغات في يوم هذا الأستاذ.");
	case T_STUDENTS_SET_MAX_GAPS_PER_DAY:
		return UiI18n::T("Avoids long waiting periods for this class.",
				 "يمنع فترات انتظار طويلة لهذا القسم.");
	case T_STUDENTS_MAX_GAPS_PER_DAY:
		return UiI18n::T("Free periods for students, counted per day.",
				 "الفراغات للتلاميذ، محسوبة لكل يوم.");
	case T_TEACHER_MAX_DAYS_PER_WEEK:
		return UiI18n::T("For part-time teachers: they come only on a few days.",
				 "للأساتذة غير المتفرّغين: الحضور في عدد محدود من الأيام.");
	case T_TEACHERS_MAX_DAYS_PER_WEEK:
		return UiI18n::T("The same weekly maximum for every teacher.",
				 "الحدّ الأسبوعي نفسه لكل الأساتذة.");
	case T_STUDENTS_SET_HOME_ROOM:
		return UiI18n::T("The class stays in its own room (laboratory, workshop…).",
				 "يبقى القسم في قاعته (مخبر، ورشة…).");

	// ---------- الضروريات ----------
	case T_TEACHER_MIN_HOURS_DAILY:
		return UiI18n::T("Full-time teachers get at least this many lessons a day (avoids one-hour days).",
				 "ضمان حدّ أدنى من الحصص في اليوم للأستاذ المتفرّغ (يمنع أيامًا بساعة واحدة).");
	case T_TEACHER_MIN_DAYS_PER_WEEK:
		return UiI18n::T("The teacher comes at least on this many days.",
				 "يضمن حضور الأستاذ في عدد أيام لا يقل عن المحدّد.");
	case T_TEACHER_MAX_HOURS_CONTINUOUSLY:
		return UiI18n::T("No more than N lessons in a row without a break.",
				 "لا يزيد عدد الحصص المتتالية عن المحدّد دون راحة.");
	case T_TEACHER_MAX_SPAN_PER_DAY:
		return UiI18n::T("Span = from the first to the last lesson of the day, including gaps.",
				 "الامتداد = من أول حصة إلى آخر حصة في اليوم، بما في ذلك الفراغات.");
	case T_STUDENTS_SET_MAX_HOURS_CONTINUOUSLY:
		return UiI18n::T("Splits long blocks of lessons for this class.",
				 "يمنع بقاء القسم في حصص متتالية طويلة.");
	case T_STUDENTS_MAX_HOURS_CONTINUOUSLY:
		return UiI18n::T("The same limit for every class.",
				 "الحدّ نفسه لكل الأقسام.");
	case T_STUDENTS_SET_MAX_SPAN_PER_DAY:
		return UiI18n::T("From first to last lesson (gaps included) for this class.",
				 "من أول حصة إلى آخر حصة (مع الفراغات) لهذا القسم.");
	case T_STUDENTS_MAX_SPAN_PER_DAY:
		return UiI18n::T("The same limit for every class.",
				 "الحدّ نفسه لكل الأقسام.");
	case T_STUDENTS_EARLY_MAX_BEGINNINGS:
		return UiI18n::T("Limits how often classes start their day at the second hour (common in Algeria).",
				 "يحدّد عدد المرات التي يبدأ فيها القسم يومه من الحصة الثانية (شائع في الجزائر).");
	case T_STUDENTS_SET_EARLY_MAX_BEGINNINGS:
		return UiI18n::T("Same limit, for one class only.",
				 "الحدّ نفسه، لقسم واحد فقط.");
	case T_ROOM_NOT_AVAILABLE:
		return UiI18n::T("Mark the slots when the room cannot be used (workshop, lab maintenance…).",
				 "حدّد الأوقات التي لا يمكن استعمال القاعة فيها (ورشة، صيانة مخبر…).");
	case T_SUBJECT_PREFERRED_ROOM:
		return UiI18n::T("Every lesson of this subject goes to this room (100 % = always).",
				 "كل حصص هذه المادة في هذه القاعة (100 % = دائمًا).");
	case T_ACTIVITY_TAG_PREFERRED_ROOM:
		return UiI18n::T("Every activity carrying this tag goes to this room (e.g. 'Lab', 'Sport').",
				 "كل نشاط يحمل هذا الوسم يُوضع في هذه القاعة (مثل «مخبر»، «رياضة»).");
	case T_SUBJECT_PREFERRED_ROOMS:
		return UiI18n::T("Tick the rooms this subject may use; the generator picks among them.",
				 "حدّد القاعات التي يُسمح للمادة باستعمالها؛ يختار البرنامج من بينها.");

	// ---------- الأنشطة ----------
	case T_ACTIVITIES_NOT_OVERLAPPING:
		return UiI18n::T("The picked lessons are never placed at the same time (e.g. a teacher's two classes).",
				 "الحصص المختارة لا تُوضع في نفس الوقت أبدًا (مثلًا حصتان لأستاذ واحد).");
	case T_ACTIVITIES_SAME_STARTING_TIME:
		return UiI18n::T("The picked lessons begin at the same hour and day.",
				 "الحصص المختارة تبدأ في نفس اليوم والساعة.");
	case T_ACTIVITIES_SAME_STARTING_DAY:
		return UiI18n::T("The picked lessons fall on the same day (hours may differ).",
				 "الحصص المختارة تقع في نفس اليوم (قد تختلف الساعة).");
	case T_MIN_DAYS_BETWEEN_ACTIVITIES:
		return UiI18n::T("Spreads the picked lessons over the week (e.g. two maths lessons, 1 day apart).",
				 "توزّع الحصص المختارة على الأسبوع (مثلًا حصتا رياضيات بفارق يوم واحد).");
	case T_ACTIVITY_PREFERRED_ROOM:
		return UiI18n::T("One lesson goes to a specific room (tick ONE activity).",
				 "حصة واحدة تُوضع في قاعة محدّدة (علّم نشاطًا واحدًا).");
	case T_ACTIVITY_PREFERRED_ROOMS:
		return UiI18n::T("One lesson may use only the ticked rooms (tick ONE activity).",
				 "حصة واحدة تستعمل القاعات المعلّمة فقط (علّم نشاطًا واحدًا).");
	default:
		return QString();
	}
}

TemplateSpec specFor(int id)
{
	TemplateSpec s;
	s.id = id;
	if (id <= T_STUDENTS_SET_HOME_ROOM)
		s.section = SEC_ESSENTIALS;
	else if (id <= T_SUBJECT_PREFERRED_ROOMS)
		s.section = SEC_NECESSITIES;
	else
		s.section = SEC_ACTIVITIES;
	s.needsTeacher = false;
	s.needsStudents = false;
	s.needsSubject = false;
	s.needsActivityTag = false;
	s.needsRoom = false;
	s.needsRoomList = false;
	s.needsValue = true;
	s.needsTimes = false;
	s.needsAllowEmptyDays = false;
	s.needsExceptCheck = false;
	s.needsActivities = false;
	s.singleActivity = false;
	s.needsConsecutiveCheck = false;
	s.needsPermanentCheck = false;
	s.valueDefault = 6;

	switch (id) {
	case T_TEACHER_NOT_AVAILABLE:
		s.needsTeacher = true;
		s.needsValue = false;
		s.needsTimes = true;
		s.fetTypes << CONSTRAINT_TEACHER_NOT_AVAILABLE_TIMES;
		break;
	case T_STUDENTS_NOT_AVAILABLE:
		s.needsStudents = true;
		s.needsValue = false;
		s.needsTimes = true;
		s.fetTypes << CONSTRAINT_STUDENTS_SET_NOT_AVAILABLE_TIMES;
		break;
	case T_TEACHER_MAX_HOURS_DAILY:
		s.needsTeacher = true;
		s.fetTypes << CONSTRAINT_TEACHER_MAX_HOURS_DAILY;
		break;
	case T_TEACHERS_MAX_HOURS_DAILY:
		s.fetTypes << CONSTRAINT_TEACHERS_MAX_HOURS_DAILY;
		break;
	case T_STUDENTS_SET_MAX_HOURS_DAILY:
		s.needsStudents = true;
		s.fetTypes << CONSTRAINT_STUDENTS_SET_MAX_HOURS_DAILY;
		break;
	case T_STUDENTS_MAX_HOURS_DAILY:
		s.fetTypes << CONSTRAINT_STUDENTS_MAX_HOURS_DAILY;
		break;
	case T_STUDENTS_SET_MIN_HOURS_DAILY:
		s.needsStudents = true;
		s.needsAllowEmptyDays = true;
		s.fetTypes << CONSTRAINT_STUDENTS_SET_MIN_HOURS_DAILY;
		break;
	case T_STUDENTS_MIN_HOURS_DAILY:
		s.needsAllowEmptyDays = true;
		s.fetTypes << CONSTRAINT_STUDENTS_MIN_HOURS_DAILY;
		break;
	case T_TEACHERS_MAX_GAPS_PER_DAY:
		s.valueDefault = 0;
		s.fetTypes << CONSTRAINT_TEACHERS_MAX_GAPS_PER_DAY;
		break;
	case T_TEACHER_MAX_GAPS_PER_DAY:
		s.needsTeacher = true;
		s.valueDefault = 0;
		s.fetTypes << CONSTRAINT_TEACHER_MAX_GAPS_PER_DAY;
		break;
	case T_STUDENTS_SET_MAX_GAPS_PER_DAY:
		s.needsStudents = true;
		s.valueDefault = 0;
		s.fetTypes << CONSTRAINT_STUDENTS_SET_MAX_GAPS_PER_DAY;
		break;
	case T_STUDENTS_MAX_GAPS_PER_DAY:
		s.valueDefault = 0;
		s.fetTypes << CONSTRAINT_STUDENTS_MAX_GAPS_PER_DAY;
		break;
	case T_TEACHER_MAX_DAYS_PER_WEEK:
		s.needsTeacher = true;
		s.valueDefault = 5;
		s.fetTypes << CONSTRAINT_TEACHER_MAX_DAYS_PER_WEEK;
		break;
	case T_TEACHERS_MAX_DAYS_PER_WEEK:
		s.valueDefault = 5;
		s.fetTypes << CONSTRAINT_TEACHERS_MAX_DAYS_PER_WEEK;
		break;
	case T_STUDENTS_SET_HOME_ROOM:
		s.needsStudents = true;
		s.needsRoom = true;
		s.needsValue = false;
		s.fetTypes << CONSTRAINT_STUDENTS_SET_HOME_ROOM;
		break;

	// ---------- الضروريات ----------
	case T_TEACHER_MIN_HOURS_DAILY:
		s.needsTeacher = true;
		s.needsAllowEmptyDays = true;
		s.valueDefault = 4;
		s.fetTypes << CONSTRAINT_TEACHER_MIN_HOURS_DAILY;
		break;
	case T_TEACHER_MIN_DAYS_PER_WEEK:
		s.needsTeacher = true;
		s.valueDefault = 5;
		s.fetTypes << CONSTRAINT_TEACHER_MIN_DAYS_PER_WEEK;
		break;
	case T_TEACHER_MAX_HOURS_CONTINUOUSLY:
		s.needsTeacher = true;
		s.valueDefault = 3;
		s.fetTypes << CONSTRAINT_TEACHER_MAX_HOURS_CONTINUOUSLY;
		break;
	case T_TEACHER_MAX_SPAN_PER_DAY:
		s.needsTeacher = true;
		s.needsExceptCheck = true;
		s.valueDefault = 8;
		s.fetTypes << CONSTRAINT_TEACHER_MAX_SPAN_PER_DAY;
		break;
	case T_STUDENTS_SET_MAX_HOURS_CONTINUOUSLY:
		s.needsStudents = true;
		s.valueDefault = 6;
		s.fetTypes << CONSTRAINT_STUDENTS_SET_MAX_HOURS_CONTINUOUSLY;
		break;
	case T_STUDENTS_MAX_HOURS_CONTINUOUSLY:
		s.valueDefault = 6;
		s.fetTypes << CONSTRAINT_STUDENTS_MAX_HOURS_CONTINUOUSLY;
		break;
	case T_STUDENTS_SET_MAX_SPAN_PER_DAY:
		s.needsStudents = true;
		s.valueDefault = 8;
		s.fetTypes << CONSTRAINT_STUDENTS_SET_MAX_SPAN_PER_DAY;
		break;
	case T_STUDENTS_MAX_SPAN_PER_DAY:
		s.valueDefault = 8;
		s.fetTypes << CONSTRAINT_STUDENTS_MAX_SPAN_PER_DAY;
		break;
	case T_STUDENTS_EARLY_MAX_BEGINNINGS:
		s.valueDefault = 4;
		s.fetTypes << CONSTRAINT_STUDENTS_EARLY_MAX_BEGINNINGS_AT_SECOND_HOUR;
		break;
	case T_STUDENTS_SET_EARLY_MAX_BEGINNINGS:
		s.needsStudents = true;
		s.valueDefault = 4;
		s.fetTypes << CONSTRAINT_STUDENTS_SET_EARLY_MAX_BEGINNINGS_AT_SECOND_HOUR;
		break;
	case T_ROOM_NOT_AVAILABLE:
		s.needsRoom = true;
		s.needsValue = false;
		s.needsTimes = true;
		s.fetTypes << CONSTRAINT_ROOM_NOT_AVAILABLE_TIMES;
		break;
	case T_SUBJECT_PREFERRED_ROOM:
		s.needsSubject = true;
		s.needsRoom = true;
		s.needsValue = false;
		s.fetTypes << CONSTRAINT_SUBJECT_PREFERRED_ROOM;
		break;
	case T_ACTIVITY_TAG_PREFERRED_ROOM:
		s.needsActivityTag = true;
		s.needsRoom = true;
		s.needsValue = false;
		s.fetTypes << CONSTRAINT_ACTIVITY_TAG_PREFERRED_ROOM;
		break;
	case T_SUBJECT_PREFERRED_ROOMS:
		s.needsSubject = true;
		s.needsRoomList = true;
		s.needsValue = false;
		s.fetTypes << CONSTRAINT_SUBJECT_PREFERRED_ROOMS;
		break;

	// ---------- الأنشطة ----------
	case T_ACTIVITIES_NOT_OVERLAPPING:
		s.needsActivities = true;
		s.needsValue = false;
		s.fetTypes << CONSTRAINT_ACTIVITIES_NOT_OVERLAPPING;
		break;
	case T_ACTIVITIES_SAME_STARTING_TIME:
		s.needsActivities = true;
		s.needsValue = false;
		s.fetTypes << CONSTRAINT_ACTIVITIES_SAME_STARTING_TIME;
		break;
	case T_ACTIVITIES_SAME_STARTING_DAY:
		s.needsActivities = true;
		s.needsValue = false;
		s.fetTypes << CONSTRAINT_ACTIVITIES_SAME_STARTING_DAY;
		break;
	case T_MIN_DAYS_BETWEEN_ACTIVITIES:
		s.needsActivities = true;
		s.needsConsecutiveCheck = true;
		s.valueDefault = 1;
		s.fetTypes << CONSTRAINT_MIN_DAYS_BETWEEN_ACTIVITIES;
		break;
	case T_ACTIVITY_PREFERRED_ROOM:
		s.needsActivities = true;
		s.singleActivity = true;
		s.needsRoom = true;
		s.needsValue = false;
		s.needsPermanentCheck = true;
		s.fetTypes << CONSTRAINT_ACTIVITY_PREFERRED_ROOM;
		break;
	case T_ACTIVITY_PREFERRED_ROOMS:
		s.needsActivities = true;
		s.singleActivity = true;
		s.needsRoomList = true;
		s.needsValue = false;
		s.fetTypes << CONSTRAINT_ACTIVITY_PREFERRED_ROOMS;
		break;
	default:
		break;
	}

	return s;
}

QString valueCaption(int id)
{
	switch (id) {
	case T_TEACHER_MAX_HOURS_DAILY:
	case T_TEACHERS_MAX_HOURS_DAILY:
	case T_STUDENTS_SET_MAX_HOURS_DAILY:
	case T_STUDENTS_MAX_HOURS_DAILY:
		return UiI18n::T("Max hours per day:", "أقصى ساعات في اليوم:");
	case T_TEACHER_MIN_HOURS_DAILY:
	case T_STUDENTS_SET_MIN_HOURS_DAILY:
	case T_STUDENTS_MIN_HOURS_DAILY:
		return UiI18n::T("Min hours per day:", "أدنى ساعات في اليوم:");
	case T_TEACHER_MAX_HOURS_CONTINUOUSLY:
	case T_STUDENTS_SET_MAX_HOURS_CONTINUOUSLY:
	case T_STUDENTS_MAX_HOURS_CONTINUOUSLY:
		return UiI18n::T("Max hours in a row:", "أقصى ساعات متتالية:");
	case T_TEACHER_MAX_SPAN_PER_DAY:
	case T_STUDENTS_SET_MAX_SPAN_PER_DAY:
	case T_STUDENTS_MAX_SPAN_PER_DAY:
		return UiI18n::T("Max span per day:", "أقصى امتداد في اليوم:");
	case T_TEACHERS_MAX_GAPS_PER_DAY:
	case T_TEACHER_MAX_GAPS_PER_DAY:
	case T_STUDENTS_SET_MAX_GAPS_PER_DAY:
	case T_STUDENTS_MAX_GAPS_PER_DAY:
		return UiI18n::T("Max gaps per day:", "أقصى فراغات في اليوم:");
	case T_TEACHER_MAX_DAYS_PER_WEEK:
	case T_TEACHERS_MAX_DAYS_PER_WEEK:
		return UiI18n::T("Max days per week:", "أقصى أيام في الأسبوع:");
	case T_TEACHER_MIN_DAYS_PER_WEEK:
		return UiI18n::T("Min days per week:", "أدنى أيام في الأسبوع:");
	case T_STUDENTS_EARLY_MAX_BEGINNINGS:
	case T_STUDENTS_SET_EARLY_MAX_BEGINNINGS:
		return UiI18n::T("Max early beginnings:", "أقصى عدد للبدايات المبكرة:");
	case T_MIN_DAYS_BETWEEN_ACTIVITIES:
		return UiI18n::T("Min days between them:", "أدنى أيام بينها:");
	default:
		return UiI18n::T("Value:", "القيمة:");
	}
}

int valueMinimum(int id)
{
	switch (id) {
	case T_TEACHERS_MAX_GAPS_PER_DAY:
	case T_TEACHER_MAX_GAPS_PER_DAY:
	case T_STUDENTS_SET_MAX_GAPS_PER_DAY:
	case T_STUDENTS_MAX_GAPS_PER_DAY:
	case T_STUDENTS_EARLY_MAX_BEGINNINGS:
	case T_STUDENTS_SET_EARLY_MAX_BEGINNINGS:
		return 0;
	default:
		return 1;
	}
}

int valueMaximum(int id)
{
	switch (id) {
	case T_TEACHER_MAX_DAYS_PER_WEEK:
	case T_TEACHERS_MAX_DAYS_PER_WEEK:
	case T_TEACHER_MIN_DAYS_PER_WEEK:
	case T_STUDENTS_EARLY_MAX_BEGINNINGS:
	case T_STUDENTS_SET_EARLY_MAX_BEGINNINGS:
	case T_MIN_DAYS_BETWEEN_ACTIVITIES:
		return qMax(1, gt.rules.nDaysPerWeek);
	default:
		return qMax(1, gt.rules.nHoursPerDay);
	}
}

// نصوص الأوقات: يدعم الوضع العادي ووضع «صباح/مساء» (MORNINGS_AFTERNOONS)
int gridRows()
{
	return (gt.rules.mode == MORNINGS_AFTERNOONS) ? 2 * gt.rules.nHoursPerDay
						       : gt.rules.nHoursPerDay;
}

int gridColumns()
{
	return (gt.rules.mode == MORNINGS_AFTERNOONS) ? gt.rules.nDaysPerWeek / 2
						      : gt.rules.nDaysPerWeek;
}

QString gridColumnTitle(int j)
{
	if (gt.rules.mode == MORNINGS_AFTERNOONS)
		return j < gt.rules.realDaysOfTheWeek.count() ? gt.rules.realDaysOfTheWeek.at(j)
							     : QString::number(j + 1);
	return j < gt.rules.daysOfTheWeek.count() ? gt.rules.daysOfTheWeek.at(j)
						 : QString::number(j + 1);
}

QString gridRowTitle(int i)
{
	if (gt.rules.mode == MORNINGS_AFTERNOONS) {
		const int hour = i % gt.rules.nHoursPerDay;
		return hour < gt.rules.hoursOfTheDay.count() ? gt.rules.hoursOfTheDay.at(hour)
							     : QString::number(hour + 1);
	}
	return i < gt.rules.hoursOfTheDay.count() ? gt.rules.hoursOfTheDay.at(i)
						 : QString::number(i + 1);
}

// الخانات تُعلَّم بالنقر (بلا مربّعات اختيار): الخانة المختارة تحمل ✓ بخلفية ملوّنة.
void setSlotSelected(QTableWidgetItem* item, bool selected)
{
	if (item == nullptr)
		return;
	item->setData(Qt::UserRole, selected);
	item->setText(selected ? QStringLiteral("✓") : QString());
	item->setBackground(selected ? QBrush(QColor(QStringLiteral("#dbeafe")))
				     : QBrush(Qt::NoBrush));
}

bool isSlotSelected(const QTableWidgetItem* item)
{
	return item != nullptr && item->data(Qt::UserRole).toBool();
}

void gridToDaysHours(QTableWidget* table, QList<int>& days, QList<int>& hours)
{
	days.clear();
	hours.clear();
	for (int i = 0; i < table->rowCount(); ++i)
		for (int j = 0; j < table->columnCount(); ++j) {
			QTableWidgetItem* item = table->item(i, j);
			if (!isSlotSelected(item))
				continue;
			if (gt.rules.mode == MORNINGS_AFTERNOONS) {
				days.append(2 * j + i / gt.rules.nHoursPerDay);
				hours.append(i % gt.rules.nHoursPerDay);
			}
			else {
				days.append(j);
				hours.append(i);
			}
		}
}

// ---------------------------------------------------------------------------

class TemplateDialog : public QDialog
{
public:
	explicit TemplateDialog(QWidget* parent);

	int addedCount;

private:
	QLineEdit* searchEdit;
	QListWidget* templateList;
	QLabel* hintLabel;
	QLabel* statusLabel;

	QWidget* teacherRow;
	QComboBox* teacherCombo;

	QWidget* studentsRow;
	QComboBox* studentsCombo;

	QWidget* subjectRow;
	QComboBox* subjectCombo;

	QWidget* tagRow;
	QComboBox* tagCombo;

	QWidget* roomRow;
	QComboBox* roomCombo;

	QWidget* roomListRow;
	QListWidget* roomListWidget;

	QWidget* valueRow;
	QLabel* valueLabel;
	QSpinBox* valueSpin;

	QWidget* weightRow;
	QSpinBox* weightSpin;

	QWidget* allowEmptyRow;
	QCheckBox* allowEmptyCheckBox;

	QWidget* exceptRow;
	QCheckBox* exceptCheckBox;

	QWidget* timesRow;
	QTableWidget* timesTable;
	QPushButton* selectAllButton;
	QPushButton* clearAllButton;

	// منتقي الأنشطة (القسم الثالث)
	QWidget* activityRow;
	QComboBox* activityTeacherFilter;
	QComboBox* activityStudentsFilter;
	QComboBox* activitySubjectFilter;
	QLineEdit* activitySearch;
	QListWidget* activityList;
	QLabel* activityCountLabel;
	QSet<int> checkedActivityIds;

	QWidget* consecutiveRow;
	QCheckBox* consecutiveCheckBox;
	QWidget* permanentRow;
	QCheckBox* permanentCheckBox;

	QPushButton* addButton;

	QListWidget* existingList;
	QLabel* existingCountLabel;

	void populateTemplateList(const QString& filter);
	void reloadTeachers();
	void reloadStudents();
	void reloadSubjects();
	void reloadActivityTags();
	void reloadRooms();
	void reloadRoomChecks();
	void reloadActivityFilters();
	void rebuildActivityList();
	QList<int> selectedActivityIds() const;
	void updateActivityCount();
	void rebuildTimesGrid();
	void updateRows();
	void refreshExisting();
	void markAllTimes(bool checked);

	int currentId() const;
	void addConstraint();
};

TemplateDialog::TemplateDialog(QWidget* parent)
	: QDialog(parent)
{
	addedCount = 0;

	setWindowTitle(UiI18n::T("Constraint templates", "محرّر القيود بالقوالب"));
	setMinimumWidth(980);
	resize(1020, 720);

	// ---------- البحث والقوالب ----------
	searchEdit = new QLineEdit(this);
	searchEdit->setObjectName(QStringLiteral("otTemplateSearch"));
	searchEdit->setPlaceholderText(UiI18n::T("Search templates…", "ابحث في القوالب…"));
	searchEdit->setClearButtonEnabled(true);

	templateList = new QListWidget(this);
	templateList->setObjectName(QStringLiteral("otTemplateList"));
	templateList->setMinimumWidth(300);
	templateList->setWordWrap(true);

	hintLabel = new QLabel(this);
	hintLabel->setWordWrap(true);
	hintLabel->setStyleSheet(QStringLiteral("color:#4b5563;"));
	hintLabel->setMinimumHeight(40);
	hintLabel->setAlignment(Qt::AlignTop | Qt::AlignRight);

	QGroupBox* templatesBox = new QGroupBox(UiI18n::T("Templates", "القوالب"), this);
	QVBoxLayout* templatesLayout = new QVBoxLayout(templatesBox);
	templatesLayout->addWidget(searchEdit);
	templatesLayout->addWidget(templateList, 1);
	templatesLayout->addWidget(hintLabel);

	// ---------- الحقول ----------
	teacherCombo = new QComboBox(this);
	teacherCombo->setObjectName(QStringLiteral("otTemplateTeacherCombo"));
	teacherRow = new QWidget(this);
	{
		QHBoxLayout* l = new QHBoxLayout(teacherRow);
		l->setContentsMargins(0, 0, 0, 0);
		l->addWidget(new QLabel(UiI18n::T("Teacher:", "الأستاذ:"), teacherRow));
		l->addWidget(teacherCombo, 1);
	}

	studentsCombo = new QComboBox(this);
	studentsCombo->setObjectName(QStringLiteral("otTemplateStudentsCombo"));
	studentsRow = new QWidget(this);
	{
		QHBoxLayout* l = new QHBoxLayout(studentsRow);
		l->setContentsMargins(0, 0, 0, 0);
		l->addWidget(new QLabel(UiI18n::T("Class / group:", "القسم / الفوج:"), studentsRow));
		l->addWidget(studentsCombo, 1);
	}

	subjectCombo = new QComboBox(this);
	subjectCombo->setObjectName(QStringLiteral("otTemplateSubjectCombo"));
	subjectRow = new QWidget(this);
	{
		QHBoxLayout* l = new QHBoxLayout(subjectRow);
		l->setContentsMargins(0, 0, 0, 0);
		l->addWidget(new QLabel(UiI18n::T("Subject:", "المادة:"), subjectRow));
		l->addWidget(subjectCombo, 1);
	}

	tagCombo = new QComboBox(this);
	tagCombo->setObjectName(QStringLiteral("otTemplateTagCombo"));
	tagRow = new QWidget(this);
	{
		QHBoxLayout* l = new QHBoxLayout(tagRow);
		l->setContentsMargins(0, 0, 0, 0);
		l->addWidget(new QLabel(UiI18n::T("Activity tag:", "وسم النشاط:"), tagRow));
		l->addWidget(tagCombo, 1);
	}

	roomCombo = new QComboBox(this);
	roomCombo->setObjectName(QStringLiteral("otTemplateRoomCombo"));
	roomRow = new QWidget(this);
	{
		QHBoxLayout* l = new QHBoxLayout(roomRow);
		l->setContentsMargins(0, 0, 0, 0);
		l->addWidget(new QLabel(UiI18n::T("Room:", "القاعة:"), roomRow));
		l->addWidget(roomCombo, 1);
	}

	roomListWidget = new QListWidget(this);
	roomListWidget->setObjectName(QStringLiteral("otTemplateRoomList"));
	roomListWidget->setMaximumHeight(130);
	roomListWidget->setSelectionMode(QAbstractItemView::NoSelection);
	roomListRow = new QWidget(this);
	{
		QVBoxLayout* l = new QVBoxLayout(roomListRow);
		l->setContentsMargins(0, 0, 0, 0);
		l->addWidget(new QLabel(UiI18n::T("Allowed rooms (tick one or more):",
						   "القاعات المسموحة (علّم واحدة أو أكثر):"), roomListRow));
		l->addWidget(roomListWidget, 1);
	}

	valueLabel = new QLabel(this);
	valueSpin = new QSpinBox(this);
	valueSpin->setObjectName(QStringLiteral("otTemplateValueSpin"));
	valueRow = new QWidget(this);
	{
		QHBoxLayout* l = new QHBoxLayout(valueRow);
		l->setContentsMargins(0, 0, 0, 0);
		l->addWidget(valueLabel);
		l->addWidget(valueSpin);
		l->addStretch(1);
	}

	allowEmptyCheckBox = new QCheckBox(
		UiI18n::T("Allow days with no lessons", "السماح بأيام بدون حصص"), this);
	allowEmptyRow = new QWidget(this);
	{
		QHBoxLayout* l = new QHBoxLayout(allowEmptyRow);
		l->setContentsMargins(0, 0, 0, 0);
		l->addWidget(allowEmptyCheckBox);
		l->addStretch(1);
	}

	exceptCheckBox = new QCheckBox(
		UiI18n::T("Allow one day with +1 hour (exception)",
			  "السماح بيوم واحد بزيادة ساعة (استثناء)"),
		this);
	exceptRow = new QWidget(this);
	{
		QHBoxLayout* l = new QHBoxLayout(exceptRow);
		l->setContentsMargins(0, 0, 0, 0);
		l->addWidget(exceptCheckBox);
		l->addStretch(1);
	}

	weightSpin = new QSpinBox(this);
	weightSpin->setObjectName(QStringLiteral("otTemplateWeightSpin"));
	weightSpin->setRange(0, 100);
	weightSpin->setValue(100);
	weightSpin->setSuffix(QStringLiteral(" %"));
	weightRow = new QWidget(this);
	{
		QHBoxLayout* l = new QHBoxLayout(weightRow);
		l->setContentsMargins(0, 0, 0, 0);
		l->addWidget(new QLabel(UiI18n::T("Weight:", "الوزن:"), weightRow));
		l->addWidget(weightSpin);
		l->addStretch(1);
	}

	timesTable = new QTableWidget(this);
	timesTable->setObjectName(QStringLiteral("otTemplateTimesTable"));
	timesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
	timesTable->horizontalHeader()->setStretchLastSection(true);
	selectAllButton = new QPushButton(UiI18n::T("Select all", "تحديد الكل"), this);
	clearAllButton = new QPushButton(UiI18n::T("Clear", "مسح التحديد"), this);
	timesRow = new QWidget(this);
	{
		QVBoxLayout* l = new QVBoxLayout(timesRow);
		l->setContentsMargins(0, 0, 0, 0);
		l->addWidget(new QLabel(UiI18n::T("Click a slot to mark it, then press Add.",
						   "اضغط على خانة لتحديدها، ثم اضغط «إضافة القيد»."), timesRow));
		l->addWidget(timesTable, 1);
		QHBoxLayout* buttons = new QHBoxLayout();
		buttons->addWidget(selectAllButton);
		buttons->addWidget(clearAllButton);
		buttons->addStretch(1);
		l->addLayout(buttons);
	}

	QGroupBox* paramsBox = new QGroupBox(UiI18n::T("Constraint details", "تفاصيل القيد"), this);
	QVBoxLayout* paramsLayout = new QVBoxLayout(paramsBox);
	paramsLayout->addWidget(teacherRow);
	paramsLayout->addWidget(studentsRow);
	paramsLayout->addWidget(subjectRow);
	paramsLayout->addWidget(tagRow);
	paramsLayout->addWidget(roomRow);
	paramsLayout->addWidget(roomListRow);
	paramsLayout->addWidget(valueRow);
	paramsLayout->addWidget(allowEmptyRow);
	paramsLayout->addWidget(exceptRow);
	paramsLayout->addWidget(weightRow);
	paramsLayout->addWidget(activityRow, 1);
	paramsLayout->addWidget(consecutiveRow);
	paramsLayout->addWidget(permanentRow);
	paramsLayout->addWidget(timesRow, 1);
	paramsLayout->addStretch(0);

	// ---------- الأزرار ----------
	addButton = new QPushButton(UiI18n::T("Add constraint", "إضافة القيد"), this);
	addButton->setObjectName(QStringLiteral("otTemplateAddButton"));
	addButton->setDefault(true);
	QPushButton* closeButton = new QPushButton(UiI18n::T("Close", "إغلاق"), this);
	closeButton->setObjectName(QStringLiteral("otTemplateCloseButton"));

	statusLabel = new QLabel(this);
	statusLabel->setWordWrap(true);
	statusLabel->setObjectName(QStringLiteral("otTemplateStatus"));

	QHBoxLayout* buttonRow = new QHBoxLayout();
	buttonRow->addWidget(statusLabel, 1);
	buttonRow->addWidget(addButton);
	buttonRow->addWidget(closeButton);

	// ---------- القيود الموجودة ----------
	existingList = new QListWidget(this);
	existingList->setObjectName(QStringLiteral("otTemplateExistingList"));
	existingList->setMaximumHeight(110);
	existingCountLabel = new QLabel(this);
	existingCountLabel->setStyleSheet(QStringLiteral("color:#4b5563;"));

	QGroupBox* existingBox = new QGroupBox(
		UiI18n::T("Existing constraints of this type", "القيود الموجودة من هذا النوع"), this);
	QVBoxLayout* existingLayout = new QVBoxLayout(existingBox);
	existingLayout->addWidget(existingList, 1);
	existingLayout->addWidget(existingCountLabel);

	// ---------- التركيب ----------
	QHBoxLayout* topLayout = new QHBoxLayout();
	topLayout->addWidget(templatesBox, 0);
	topLayout->addWidget(paramsBox, 1);

	QVBoxLayout* mainLayout = new QVBoxLayout(this);
	mainLayout->addLayout(topLayout, 1);
	mainLayout->addWidget(existingBox);
	mainLayout->addLayout(buttonRow);

	// ---------- الوصل ----------
	connect(searchEdit, &QLineEdit::textChanged, this,
		[this](const QString& text) { populateTemplateList(text); });
	connect(templateList, &QListWidget::currentItemChanged, this,
		[this](QListWidgetItem*, QListWidgetItem*) {
			updateRows();
			refreshExisting();
		});
	connect(addButton, &QPushButton::clicked, this, [this]() { addConstraint(); });
	connect(closeButton, &QPushButton::clicked, this, [this]() { accept(); });
	connect(timesTable, &QTableWidget::cellClicked, this, [this](int row, int column) {
		QTableWidgetItem* item = timesTable->item(row, column);
		setSlotSelected(item, !isSlotSelected(item));
	});
	connect(activityTeacherFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
		[this](int) { rebuildActivityList(); });
	connect(activityStudentsFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
		[this](int) { rebuildActivityList(); });
	connect(activitySubjectFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
		[this](int) { rebuildActivityList(); });
	connect(activitySearch, &QLineEdit::textChanged, this,
		[this](const QString&) { rebuildActivityList(); });
	connect(activityList, &QListWidget::itemChanged, this, [this](QListWidgetItem* item) {
		if (item == nullptr)
			return;
		const int id = item->data(Qt::UserRole).toInt();
		if (item->checkState() == Qt::Checked)
			checkedActivityIds.insert(id);
		else
			checkedActivityIds.remove(id);
		updateActivityCount();
	});
	connect(selectAllButton, &QPushButton::clicked, this, [this]() { markAllTimes(true); });
	connect(clearAllButton, &QPushButton::clicked, this, [this]() { markAllTimes(false); });

	reloadTeachers();
	reloadStudents();
	reloadSubjects();
	reloadActivityTags();
	reloadRooms();
	reloadRoomChecks();
	reloadActivityFilters();
	populateTemplateList(QString());
	updateRows();
	refreshExisting();
}

int TemplateDialog::currentId() const
{
	QListWidgetItem* item = templateList->currentItem();
	if (item == nullptr)
		return -1;
	const QVariant data = item->data(Qt::UserRole);
	if (!data.isValid())
		return -1; // عنوان قسم
	return data.toInt();
}

void TemplateDialog::populateTemplateList(const QString& filter)
{
	const QString needle = filter.trimmed();

	// نحفظ القالب المختار لاستعادته بعد الفلترة
	const int previousId = currentId();

	templateList->clear();

	for (int section = SEC_ESSENTIALS; section <= SEC_ACTIVITIES; ++section) {
		QList<int> matches;
		for (int id = 0; id < T_COUNT; ++id) {
			if (specFor(id).section != section)
				continue;
			if (needle.isEmpty()
			    || templateTitle(id).contains(needle, Qt::CaseInsensitive)
			    || templateHint(id).contains(needle, Qt::CaseInsensitive))
				matches.append(id);
		}
		if (matches.isEmpty())
			continue;

		QListWidgetItem* header = new QListWidgetItem(
			QStringLiteral("%1 (%2)").arg(sectionTitle(section)).arg(matches.count()),
			templateList);
		header->setFlags(Qt::NoItemFlags);
		QFont headerFont = header->font();
		headerFont.setBold(true);
		header->setFont(headerFont);
		header->setForeground(QColor(QStringLiteral("#1d4ed8")));
		header->setBackground(QColor(QStringLiteral("#eef2ff")));
		header->setTextAlignment((UiI18n::isArabic() ? Qt::AlignRight : Qt::AlignLeft)
					 | Qt::AlignVCenter);

		for (int id : std::as_const(matches)) {
			QListWidgetItem* item = new QListWidgetItem(templateTitle(id), templateList);
			item->setData(Qt::UserRole, id);
			item->setTextAlignment((UiI18n::isArabic() ? Qt::AlignRight : Qt::AlignLeft)
					       | Qt::AlignVCenter);
		}
	}

	// استعادة الاختيار أو اختيار أول قالب
	int rowToSelect = -1;
	for (int i = 0; i < templateList->count(); ++i) {
		QListWidgetItem* item = templateList->item(i);
		if (!item->data(Qt::UserRole).isValid())
			continue;
		if (rowToSelect < 0)
			rowToSelect = i;
		if (item->data(Qt::UserRole).toInt() == previousId) {
			rowToSelect = i;
			break;
		}
	}
	if (rowToSelect >= 0)
		templateList->setCurrentRow(rowToSelect);
	else {
		hintLabel->setText(UiI18n::T("No template matches this search.",
					     "لا يوجد قالب يطابق هذا البحث."));
		addButton->setEnabled(false);
	}
}

void TemplateDialog::reloadTeachers()
{
	const QString previous = teacherCombo->currentText();
	teacherCombo->clear();
	const QList<Teacher*>& teachers = gt.rules.teachersList;
	for (Teacher* t : teachers)
		teacherCombo->addItem(t->name);
	const int idx = teacherCombo->findText(previous);
	if (idx >= 0)
		teacherCombo->setCurrentIndex(idx);
}

void TemplateDialog::reloadStudents()
{
	// نستعمل دالة FET الأصلية حتى يحترم إعداد عرض الفرق الفرعية
	populateStudentsComboBox(studentsCombo, studentsCombo->currentText(), false);
}

void TemplateDialog::reloadSubjects()
{
	const QString previous = subjectCombo->currentText();
	subjectCombo->clear();
	const QList<Subject*>& subjects = gt.rules.subjectsList;
	for (Subject* s : subjects)
		subjectCombo->addItem(s->name);
	const int idx = subjectCombo->findText(previous);
	if (idx >= 0)
		subjectCombo->setCurrentIndex(idx);
}

void TemplateDialog::reloadActivityTags()
{
	const QString previous = tagCombo->currentText();
	tagCombo->clear();
	const QList<ActivityTag*>& tags = gt.rules.activityTagsList;
	for (ActivityTag* t : tags)
		tagCombo->addItem(t->name);
	const int idx = tagCombo->findText(previous);
	if (idx >= 0)
		tagCombo->setCurrentIndex(idx);
}

void TemplateDialog::reloadRooms()
{
	const QString previous = roomCombo->currentText();
	roomCombo->clear();
	const QList<Room*>& rooms = gt.rules.roomsList;
	for (Room* r : rooms)
		roomCombo->addItem(r->name);
	const int idx = roomCombo->findText(previous);
	if (idx >= 0)
		roomCombo->setCurrentIndex(idx);
}

void TemplateDialog::reloadRoomChecks()
{
	roomListWidget->clear();
	const QList<Room*>& rooms = gt.rules.roomsList;
	for (Room* r : rooms) {
		QListWidgetItem* item = new QListWidgetItem(r->name, roomListWidget);
		item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
		item->setCheckState(Qt::Unchecked);
	}
}

void TemplateDialog::reloadActivityFilters()
{
	// الأستاذ
	activityTeacherFilter->clear();
	activityTeacherFilter->addItem(UiI18n::T("All teachers", "كل الأساتذة"), QString());
	for (Teacher* t : std::as_const(gt.rules.teachersList))
		activityTeacherFilter->addItem(t->name, t->name);

	// القسم (يستعمل دالة FET الأصلية، مع عنصر فارغ = الكل)
	activityStudentsFilter->clear();
	activityStudentsFilter->addItem(UiI18n::T("All classes", "كل الأقسام"), QString());
	populateStudentsComboBox(activityStudentsFilter, QString(), false);
	for (int i = activityStudentsFilter->count() - 1; i >= 0; --i)
		if (activityStudentsFilter->itemText(i).isEmpty())
			activityStudentsFilter->removeItem(i);
	activityStudentsFilter->insertItem(0, UiI18n::T("All classes", "كل الأقسام"), QString());

	// المادة
	activitySubjectFilter->clear();
	activitySubjectFilter->addItem(UiI18n::T("All subjects", "كل المواد"), QString());
	for (Subject* subj : std::as_const(gt.rules.subjectsList))
		activitySubjectFilter->addItem(subj->name, subj->name);

	rebuildActivityList();
}

void TemplateDialog::rebuildActivityList()
{
	const QString teacher = activityTeacherFilter->currentData().toString();
	const QString students = activityStudentsFilter->currentData().toString();
	const QString subject = activitySubjectFilter->currentData().toString();
	const QString needle = activitySearch->text().trimmed();

	activityList->clear();

	const int limit = 600;
	int shown = 0;
	for (Activity* act : std::as_const(gt.rules.activitiesList)) {
		if (!act->active)
			continue;
		if (!teacher.isEmpty() && !act->teachersNames.contains(teacher))
			continue;
		if (!students.isEmpty() && !act->studentsNames.contains(students))
			continue;
		if (!subject.isEmpty() && act->subjectName != subject)
			continue;

		const QString description = act->getDescription(gt.rules);
		if (!needle.isEmpty() && !description.contains(needle, Qt::CaseInsensitive))
			continue;

		if (shown >= limit)
			break;

		QListWidgetItem* item = new QListWidgetItem(description, activityList);
		item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
		item->setData(Qt::UserRole, act->id);
		item->setCheckState(checkedActivityIds.contains(act->id) ? Qt::Checked
									  : Qt::Unchecked);
		item->setTextAlignment((UiI18n::isArabic() ? Qt::AlignRight : Qt::AlignLeft)
				       | Qt::AlignVCenter);
		++shown;
	}

	if (gt.rules.activitiesList.count() > limit && shown >= limit) {
		activityCountLabel->setText(
			UiI18n::T("Showing the first %1 lessons — narrow the filters to see others.",
				  "تُعرض أول %1 حصة — ضيّق الترشيح لرؤية غيرها.").arg(limit));
	}
	updateActivityCount();
}

QList<int> TemplateDialog::selectedActivityIds() const
{
	QList<int> ids;
	// نحافظ على ترتيب النشاطات في الملف (مهم لبعض القيود)
	for (Activity* act : std::as_const(gt.rules.activitiesList))
		if (checkedActivityIds.contains(act->id))
			ids.append(act->id);
	return ids;
}

void TemplateDialog::updateActivityCount()
{
	const int selected = int(checkedActivityIds.count());
	const bool single = currentId() >= 0 && specFor(currentId()).singleActivity;

	if (selected == 0)
		activityCountLabel->setText(
			single ? UiI18n::T("Tick exactly one lesson in the list.",
					   "علّم حصة واحدة فقط في القائمة.")
			       : UiI18n::T("No lesson selected yet — tick at least two.",
					   "لم تُحدَّد أي حصة بعد — علّم حصتين على الأقل."));
	else if (single && selected > 1)
		activityCountLabel->setText(
			UiI18n::T("%1 lessons selected — this template needs exactly one.",
				  "الحصص المحدّدة: %1 — هذا القالب يحتاج حصة واحدة فقط.").arg(selected));
	else
		activityCountLabel->setText(UiI18n::T("%1 lesson(s) selected.",
						      "الحصص المحدّدة: %1.").arg(selected));
}

void TemplateDialog::rebuildTimesGrid()
{
	const int rows = gridRows();
	const int columns = gridColumns();

	timesTable->clear();
	timesTable->setRowCount(rows);
	timesTable->setColumnCount(columns);

	QStringList headers;
	for (int j = 0; j < columns; ++j)
		headers << gridColumnTitle(j);
	timesTable->setHorizontalHeaderLabels(headers);

	QStringList rowHeaders;
	for (int i = 0; i < rows; ++i)
		rowHeaders << gridRowTitle(i);
	timesTable->setVerticalHeaderLabels(rowHeaders);

	for (int i = 0; i < rows; ++i)
		for (int j = 0; j < columns; ++j) {
			QTableWidgetItem* item = new QTableWidgetItem();
			item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
			item->setTextAlignment(Qt::AlignCenter);
			setSlotSelected(item, false);
			timesTable->setItem(i, j, item);
		}

	timesTable->resizeColumnsToContents();
}

void TemplateDialog::markAllTimes(bool checked)
{
	for (int i = 0; i < timesTable->rowCount(); ++i)
		for (int j = 0; j < timesTable->columnCount(); ++j)
			setSlotSelected(timesTable->item(i, j), checked);
}

void TemplateDialog::updateRows()
{
	const int id = currentId();
	if (id < 0)
		return;

	const TemplateSpec spec = specFor(id);

	hintLabel->setText(templateHint(id));

	teacherRow->setVisible(spec.needsTeacher);
	studentsRow->setVisible(spec.needsStudents);
	subjectRow->setVisible(spec.needsSubject);
	tagRow->setVisible(spec.needsActivityTag);
	roomRow->setVisible(spec.needsRoom);
	roomListRow->setVisible(spec.needsRoomList);
	valueRow->setVisible(spec.needsValue);
	allowEmptyRow->setVisible(spec.needsAllowEmptyDays);
	exceptRow->setVisible(spec.needsExceptCheck);
	timesRow->setVisible(spec.needsTimes);
	activityRow->setVisible(spec.needsActivities);
	consecutiveRow->setVisible(spec.needsConsecutiveCheck);
	permanentRow->setVisible(spec.needsPermanentCheck);

	if (spec.needsActivities) {
		activityList->setSelectionMode(spec.singleActivity
						       ? QAbstractItemView::SingleSelection
						       : QAbstractItemView::NoSelection);
		updateActivityCount();
	}

	valueLabel->setText(valueCaption(id));
	valueSpin->setRange(valueMinimum(id), valueMaximum(id));
	valueSpin->setValue(qBound(valueSpin->minimum(), spec.valueDefault, valueSpin->maximum()));

	if (spec.needsTimes) {
		const bool rebuild = (timesTable->rowCount() != gridRows()
				      || timesTable->columnCount() != gridColumns());
		if (rebuild)
			rebuildTimesGrid();
	}

	teacherCombo->setEnabled(teacherCombo->count() > 0);
	studentsCombo->setEnabled(studentsCombo->count() > 0);
	subjectCombo->setEnabled(subjectCombo->count() > 0);
	tagCombo->setEnabled(tagCombo->count() > 0);
	roomCombo->setEnabled(roomCombo->count() > 0);
	roomListWidget->setEnabled(roomListWidget->count() > 0);

	addButton->setEnabled(true);
	statusLabel->clear();

	if (spec.needsTimes)
		weightSpin->setToolTip(UiI18n::T("Weights are accepted, but 'not available' is normally a hard constraint.",
						"الوزن مقبول، لكن «غير متاح» عادةً قيد صارم."));
	else if (spec.needsRoomList || spec.needsRoom)
		weightSpin->setToolTip(UiI18n::T("100 % = the room is mandatory. Lower values let the generator use other rooms if needed.",
						"100 % = القاعة إلزامية. القيم الأقل تسمح للبرنامج باستعمال قاعات أخرى عند الحاجة."));
	else
		weightSpin->setToolTip(UiI18n::T("100 % = a hard constraint (must be respected). Lower values let the generator trade it for a better overall timetable.",
						"100 % = قيد صارم (يجب احترامه). القيم الأقل تسمح للبرنامج بالتضحية به مقابل جدول أفضل عمومًا."));
}

void TemplateDialog::refreshExisting()
{
	existingList->clear();

	const int id = currentId();
	if (id < 0) {
		existingCountLabel->clear();
		return;
	}

	const QList<int> types = specFor(id).fetTypes;
	int total = 0;

	for (TimeConstraint* c : std::as_const(gt.rules.timeConstraintsList))
		if (types.contains(c->type)) {
			++total;
			if (existingList->count() < 100)
				existingList->addItem(c->getDescription(gt.rules));
		}
	for (SpaceConstraint* c : std::as_const(gt.rules.spaceConstraintsList))
		if (types.contains(c->type)) {
			++total;
			if (existingList->count() < 100)
				existingList->addItem(c->getDescription(gt.rules));
		}

	if (total == 0)
		existingCountLabel->setText(UiI18n::T("None yet for this template.",
						      "لا يوجد أي قيد من هذا القالب بعد."));
	else
		existingCountLabel->setText(UiI18n::T("%1 constraint(s) already use this template.",
						      "عدد القيود الموجودة من هذا القالب: %1.").arg(total));

	existingList->setVisible(total > 0);
}

void TemplateDialog::addConstraint()
{
	const int id = currentId();
	if (id < 0)
		return;

	if (!gt.rules.initialized) {
		QMessageBox::information(this, UiI18n::T("Constraint templates", "القيود بالقوالب"),
					 UiI18n::T("Open or create a file first.", "افتح ملفًا أو أنشئ ملفًا جديدًا أولًا."));
		return;
	}

	const TemplateSpec spec = specFor(id);
	const double weight = double(weightSpin->value());
	const QString teacher = teacherCombo->currentText().trimmed();
	const QString students = studentsCombo->currentText().trimmed();
	const QString subject = subjectCombo->currentText().trimmed();
	const QString activityTag = tagCombo->currentText().trimmed();
	const QString room = roomCombo->currentText().trimmed();

	const QString dialogTitle = UiI18n::T("Constraint templates", "القيود بالقوالب");

	if (spec.needsTeacher && teacher.isEmpty()) {
		QMessageBox::warning(this, dialogTitle,
				     UiI18n::T("Choose a teacher first.", "اختر أستاذًا أولًا."));
		return;
	}
	if (spec.needsStudents && students.isEmpty()) {
		QMessageBox::warning(this, dialogTitle,
				     UiI18n::T("Choose a class or group first.", "اختر قسمًا أو فوجًا أولًا."));
		return;
	}
	if (spec.needsSubject && subject.isEmpty()) {
		QMessageBox::warning(this, dialogTitle,
				     UiI18n::T("Choose a subject first.", "اختر مادة أولًا."));
		return;
	}
	if (spec.needsActivityTag && activityTag.isEmpty()) {
		QMessageBox::warning(this, dialogTitle,
				     UiI18n::T("Choose an activity tag first.", "اختر وسم نشاط أولًا."));
		return;
	}
	if (spec.needsRoom && room.isEmpty()) {
		QMessageBox::warning(this, dialogTitle,
				     UiI18n::T("Choose a room first.", "اختر قاعة أولًا."));
		return;
	}

	QStringList checkedRooms;
	if (spec.needsRoomList) {
		for (int i = 0; i < roomListWidget->count(); ++i) {
			QListWidgetItem* item = roomListWidget->item(i);
			if (item != nullptr && item->checkState() == Qt::Checked)
				checkedRooms.append(item->text());
		}
		if (checkedRooms.isEmpty()) {
			QMessageBox::warning(this, dialogTitle,
					     UiI18n::T("Tick at least one room.", "علّم قاعة واحدة على الأقل."));
			return;
		}
	}

	QList<int> activityIds;
	if (spec.needsActivities) {
		activityIds = selectedActivityIds();
		if (spec.singleActivity && activityIds.count() != 1) {
			QMessageBox::warning(this, dialogTitle,
					     UiI18n::T("Tick exactly one lesson in the list.",
						       "علّم حصة واحدة فقط في القائمة."));
			return;
		}
		if (!spec.singleActivity && activityIds.count() < 2) {
			QMessageBox::warning(this, dialogTitle,
					     UiI18n::T("Tick at least two lessons in the list.",
						       "علّم حصتين على الأقل في القائمة."));
			return;
		}
	}

	QList<int> days;
	QList<int> hours;
	if (spec.needsTimes) {
		gridToDaysHours(timesTable, days, hours);
		if (days.isEmpty()) {
			QMessageBox::warning(this, dialogTitle,
					     UiI18n::T("Mark at least one time slot.",
						       "حدّد وقتًا واحدًا على الأقل من الشبكة."));
			return;
		}
	}

	// إنشاء كائن FET الأصلي المناسب
	TimeConstraint* tc = nullptr;
	SpaceConstraint* sc = nullptr;
	const int value = valueSpin->value();
	const bool allowEmpty = allowEmptyCheckBox->isChecked();
	const bool exceptDay = exceptCheckBox->isChecked();

	switch (id) {
	case T_TEACHER_NOT_AVAILABLE:
		tc = new ConstraintTeacherNotAvailableTimes(weight, teacher, days, hours);
		break;
	case T_STUDENTS_NOT_AVAILABLE:
		tc = new ConstraintStudentsSetNotAvailableTimes(weight, students, days, hours);
		break;
	case T_TEACHER_MAX_HOURS_DAILY:
		tc = new ConstraintTeacherMaxHoursDaily(weight, value, teacher);
		break;
	case T_TEACHERS_MAX_HOURS_DAILY:
		tc = new ConstraintTeachersMaxHoursDaily(weight, value);
		break;
	case T_STUDENTS_SET_MAX_HOURS_DAILY:
		tc = new ConstraintStudentsSetMaxHoursDaily(weight, value, students);
		break;
	case T_STUDENTS_MAX_HOURS_DAILY:
		tc = new ConstraintStudentsMaxHoursDaily(weight, value);
		break;
	case T_STUDENTS_SET_MIN_HOURS_DAILY:
		tc = new ConstraintStudentsSetMinHoursDaily(weight, value, students, allowEmpty);
		break;
	case T_STUDENTS_MIN_HOURS_DAILY:
		tc = new ConstraintStudentsMinHoursDaily(weight, value, allowEmpty);
		break;
	case T_TEACHERS_MAX_GAPS_PER_DAY:
		tc = new ConstraintTeachersMaxGapsPerDay(weight, value);
		break;
	case T_TEACHER_MAX_GAPS_PER_DAY:
		tc = new ConstraintTeacherMaxGapsPerDay(weight, teacher, value);
		break;
	case T_STUDENTS_SET_MAX_GAPS_PER_DAY:
		tc = new ConstraintStudentsSetMaxGapsPerDay(weight, value, students);
		break;
	case T_STUDENTS_MAX_GAPS_PER_DAY:
		tc = new ConstraintStudentsMaxGapsPerDay(weight, value);
		break;
	case T_TEACHER_MAX_DAYS_PER_WEEK:
		tc = new ConstraintTeacherMaxDaysPerWeek(weight, value, teacher);
		break;
	case T_TEACHERS_MAX_DAYS_PER_WEEK:
		tc = new ConstraintTeachersMaxDaysPerWeek(weight, value);
		break;
	case T_STUDENTS_SET_HOME_ROOM:
		sc = new ConstraintStudentsSetHomeRoom(weight, students, room);
		break;

	// ---------- الضروريات ----------
	case T_TEACHER_MIN_HOURS_DAILY:
		tc = new ConstraintTeacherMinHoursDaily(weight, value, teacher, allowEmpty);
		break;
	case T_TEACHER_MIN_DAYS_PER_WEEK:
		tc = new ConstraintTeacherMinDaysPerWeek(weight, value, teacher);
		break;
	case T_TEACHER_MAX_HOURS_CONTINUOUSLY:
		tc = new ConstraintTeacherMaxHoursContinuously(weight, value, teacher);
		break;
	case T_TEACHER_MAX_SPAN_PER_DAY:
		tc = new ConstraintTeacherMaxSpanPerDay(weight, value, exceptDay, teacher);
		break;
	case T_STUDENTS_SET_MAX_HOURS_CONTINUOUSLY:
		tc = new ConstraintStudentsSetMaxHoursContinuously(weight, value, students);
		break;
	case T_STUDENTS_MAX_HOURS_CONTINUOUSLY:
		tc = new ConstraintStudentsMaxHoursContinuously(weight, value);
		break;
	case T_STUDENTS_SET_MAX_SPAN_PER_DAY:
		tc = new ConstraintStudentsSetMaxSpanPerDay(weight, value, students);
		break;
	case T_STUDENTS_MAX_SPAN_PER_DAY:
		tc = new ConstraintStudentsMaxSpanPerDay(weight, value);
		break;
	case T_STUDENTS_EARLY_MAX_BEGINNINGS:
		tc = new ConstraintStudentsEarlyMaxBeginningsAtSecondHour(weight, value);
		break;
	case T_STUDENTS_SET_EARLY_MAX_BEGINNINGS:
		tc = new ConstraintStudentsSetEarlyMaxBeginningsAtSecondHour(weight, value, students);
		break;
	case T_ROOM_NOT_AVAILABLE:
		sc = new ConstraintRoomNotAvailableTimes(weight, room, days, hours);
		break;
	case T_SUBJECT_PREFERRED_ROOM:
		sc = new ConstraintSubjectPreferredRoom(weight, subject, room);
		break;
	case T_ACTIVITY_TAG_PREFERRED_ROOM:
		sc = new ConstraintActivityTagPreferredRoom(weight, activityTag, room);
		break;
	case T_SUBJECT_PREFERRED_ROOMS:
		sc = new ConstraintSubjectPreferredRooms(weight, subject, checkedRooms);
		break;

	// ---------- الأنشطة ----------
	case T_ACTIVITIES_NOT_OVERLAPPING:
		tc = new ConstraintActivitiesNotOverlapping(weight, activityIds.count(), activityIds);
		break;
	case T_ACTIVITIES_SAME_STARTING_TIME:
		tc = new ConstraintActivitiesSameStartingTime(weight, activityIds.count(), activityIds);
		break;
	case T_ACTIVITIES_SAME_STARTING_DAY:
		tc = new ConstraintActivitiesSameStartingDay(weight, activityIds.count(), activityIds);
		break;
	case T_MIN_DAYS_BETWEEN_ACTIVITIES:
		tc = new ConstraintMinDaysBetweenActivities(weight, consecutiveCheckBox->isChecked(),
							    activityIds.count(), activityIds, value);
		break;
	case T_ACTIVITY_PREFERRED_ROOM:
		sc = new ConstraintActivityPreferredRoom(weight, activityIds.first(), room, QStringList(),
							 permanentCheckBox->isChecked());
		break;
	case T_ACTIVITY_PREFERRED_ROOMS:
		sc = new ConstraintActivityPreferredRooms(weight, activityIds.first(), checkedRooms);
		break;
	default:
		break;
	}

	bool ok = false;
	QString description;

	if (tc != nullptr) {
		description = tc->getDetailedDescription(gt.rules);
		ok = gt.rules.addTimeConstraint(tc);
		if (ok)
			gt.rules.addUndoPoint(
				UiI18n::T("Added the constraint: %1", "أُضيف القيد: %1")
					.arg(QStringLiteral("<br /><br />\n") + description));
		else
			delete tc;
	}
	else if (sc != nullptr) {
		description = sc->getDetailedDescription(gt.rules);
		ok = gt.rules.addSpaceConstraint(sc);
		if (ok)
			gt.rules.addUndoPoint(
				UiI18n::T("Added the constraint: %1", "أُضيف القيد: %1")
					.arg(QStringLiteral("<br /><br />\n") + description));
		else
			delete sc;
	}

	if (!ok) {
		QMessageBox::warning(this, dialogTitle,
				     UiI18n::T("The constraint could not be added. A similar constraint of this type probably already exists for the same teacher/class/subject.",
					       "تعذّرت إضافة القيد. الأرجح أن قيدًا مشابهًا من النوع نفسه موجود بالفعل لنفس الأستاذ/القسم/المادة."));
		return;
	}

	++addedCount;
	statusLabel->setStyleSheet(QStringLiteral("color:#166534;"));
	statusLabel->setText(UiI18n::T("✔ Constraint added. You can add another one.",
				       "✔ أُضيف القيد. يمكنك إضافة قيد آخر."));

	refreshExisting();
	markAllTimes(false);

	// بعد الإضافة: نُفرّغ اختيار الأنشطة حتى لا يُضاف القيد نفسه مرتين بالخطأ
	if (spec.needsActivities) {
		checkedActivityIds.clear();
		rebuildActivityList();
	}
}

} // namespace

namespace UiConstraints {

bool run(QWidget* parent)
{
	if (gt.rules.initialized == false) {
		QMessageBox::information(parent, UiI18n::T("Constraint templates", "القيود بالقوالب"),
					 UiI18n::T("Open or create a file first (File ▸ New).",
						   "افتح ملفًا أو أنشئ ملفًا جديدًا أولًا (ملف ▸ جديد)."));
		return false;
	}

	TemplateDialog dialog(parent);
	const int result = dialog.exec();

	// نُحدّث عنوان النافذة (علامة التعديل) ولوحة البداية بعد أي إضافة
	if (dialog.addedCount > 0) {
		QMainWindow* window = qobject_cast<QMainWindow*>(parent != nullptr ? parent->window()
										  : nullptr);
		if (FetMainForm* mainForm = qobject_cast<FetMainForm*>(window)) {
			// تحديث عنوان النافذة (علامة «معدَّل») واسم الملف الحالي
			mainForm->setCurrentFile(INPUT_FILENAME_XML);
			mainForm->setWindowModified(gt.rules.modified);
		}
		if (window != nullptr)
			UiShell::refreshDashboard(window);
	}

	return dialog.addedCount > 0 || result == QDialog::Accepted;
}

int templateCount()
{
	return T_COUNT;
}

} // namespace UiConstraints
