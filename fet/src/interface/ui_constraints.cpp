/*
File ui_constraints.cpp — Open-Timetable addition (المرحلة 4 — القسم الأول)

This file is part of Open-Timetable, a modernised user interface for FET.
FET is free software; you can redistribute it and/or modify it under the terms
of the GNU Affero General Public License version 3 (see LICENSE).

محرّر القيود بالقوالب — «الأساسيات» (15 قالبًا):

  1  أستاذ غير متاح في أوقات          9  أقصى فراغات يومية لكل الأساتذة
  2  قسم/فوج غير متاح في أوقات       10  أقصى فراغات يومية لأستاذ
  3  أقصى ساعات يومية لأستاذ         11  أقصى فراغات يومية لقسم
  4  أقصى ساعات يومية لكل الأساتذة   12  أقصى فراغات يومية لكل الأقسام
  5  أقصى ساعات يومية لقسم           13  أقصى أيام أسبوعيًا لأستاذ
  6  أقصى ساعات يومية لكل الأقسام    14  أقصى أيام أسبوعيًا لكل الأساتذة
  7  أدنى ساعات يومية لقسم           15  قاعة القسم الثابتة (قيد مكاني)
  8  أدنى ساعات يومية لكل الأقسام

الطريقة: نُنشئ كائنات FET الأصلية ونضيفها بـ addTimeConstraint/addSpaceConstraint،
ثم addUndoPoint — أي أن القيود تظهر في «كل القيود الزمنية/المكانية» وتُصدَّر في
ملف .fet كأي قيد أُضيف يدويًا. لا تعديل على المحرّك.
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
#include "utilities.h"
#include "fetmainform.h"

#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QBrush>
#include <QColor>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QMainWindow>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include <utility>

namespace {

enum TemplateId {
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
	T_COUNT
};

// كل قالب: ما يحتاجه من حقول + حدود القيمة الافتراضية
struct TemplateSpec {
	int id;
	bool needsTeacher;
	bool needsStudents;
	bool needsValue;
	bool needsTimes;
	bool needsRoom;
	bool needsAllowEmptyDays;
	int valueDefault;
	QList<int> fetTypes; // أنواع FET لعرض «القيود الموجودة» من النوع نفسه
};

QString templateTitle(int id, bool ar)
{
	switch (id) {
	case T_TEACHER_NOT_AVAILABLE:
		return ar ? QString::fromUtf8("أستاذ غير متاح في أوقات")
			  : QStringLiteral("Teacher not available at times");
	case T_STUDENTS_NOT_AVAILABLE:
		return ar ? QString::fromUtf8("قسم أو فوج غير متاح في أوقات")
			  : QStringLiteral("Students set not available at times");
	case T_TEACHER_MAX_HOURS_DAILY:
		return ar ? QString::fromUtf8("أقصى ساعات يومية — لأستاذ")
			  : QStringLiteral("Max hours daily — one teacher");
	case T_TEACHERS_MAX_HOURS_DAILY:
		return ar ? QString::fromUtf8("أقصى ساعات يومية — لكل الأساتذة")
			  : QStringLiteral("Max hours daily — all teachers");
	case T_STUDENTS_SET_MAX_HOURS_DAILY:
		return ar ? QString::fromUtf8("أقصى ساعات يومية — لقسم")
			  : QStringLiteral("Max hours daily — one students set");
	case T_STUDENTS_MAX_HOURS_DAILY:
		return ar ? QString::fromUtf8("أقصى ساعات يومية — لكل الأقسام")
			  : QStringLiteral("Max hours daily — all students");
	case T_STUDENTS_SET_MIN_HOURS_DAILY:
		return ar ? QString::fromUtf8("أدنى ساعات يومية — لقسم")
			  : QStringLiteral("Min hours daily — one students set");
	case T_STUDENTS_MIN_HOURS_DAILY:
		return ar ? QString::fromUtf8("أدنى ساعات يومية — لكل الأقسام")
			  : QStringLiteral("Min hours daily — all students");
	case T_TEACHERS_MAX_GAPS_PER_DAY:
		return ar ? QString::fromUtf8("أقصى فراغات يومية — لكل الأساتذة")
			  : QStringLiteral("Max gaps daily — all teachers");
	case T_TEACHER_MAX_GAPS_PER_DAY:
		return ar ? QString::fromUtf8("أقصى فراغات يومية — لأستاذ")
			  : QStringLiteral("Max gaps daily — one teacher");
	case T_STUDENTS_SET_MAX_GAPS_PER_DAY:
		return ar ? QString::fromUtf8("أقصى فراغات يومية — لقسم")
			  : QStringLiteral("Max gaps daily — one students set");
	case T_STUDENTS_MAX_GAPS_PER_DAY:
		return ar ? QString::fromUtf8("أقصى فراغات يومية — لكل الأقسام")
			  : QStringLiteral("Max gaps daily — all students");
	case T_TEACHER_MAX_DAYS_PER_WEEK:
		return ar ? QString::fromUtf8("أقصى أيام أسبوعيًا — لأستاذ")
			  : QStringLiteral("Max days per week — one teacher");
	case T_TEACHERS_MAX_DAYS_PER_WEEK:
		return ar ? QString::fromUtf8("أقصى أيام أسبوعيًا — لكل الأساتذة")
			  : QStringLiteral("Max days per week — all teachers");
	case T_STUDENTS_SET_HOME_ROOM:
		return ar ? QString::fromUtf8("قاعة القسم الثابتة")
			  : QStringLiteral("Home room for a students set");
	default:
		return QString();
	}
}

QString templateHint(int id)
{
	switch (id) {
	case T_TEACHER_NOT_AVAILABLE:
		return UiI18n::T("Mark the slots when the teacher cannot teach (red X = unavailable).",
				 "حدّد الأوقات التي لا يستطيع الأستاذ التدريس فيها (علّامات الأوقات الممنوعة).");
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
	default:
		return QString();
	}
}

TemplateSpec specFor(int id)
{
	TemplateSpec s;
	s.id = id;
	s.needsTeacher = false;
	s.needsStudents = false;
	s.needsValue = true;
	s.needsTimes = false;
	s.needsRoom = false;
	s.needsAllowEmptyDays = false;
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
	case T_STUDENTS_SET_MIN_HOURS_DAILY:
	case T_STUDENTS_MIN_HOURS_DAILY:
		return UiI18n::T("Min hours per day:", "أدنى ساعات في اليوم:");
	case T_TEACHERS_MAX_GAPS_PER_DAY:
	case T_TEACHER_MAX_GAPS_PER_DAY:
	case T_STUDENTS_SET_MAX_GAPS_PER_DAY:
	case T_STUDENTS_MAX_GAPS_PER_DAY:
		return UiI18n::T("Max gaps per day:", "أقصى فراغات في اليوم:");
	case T_TEACHER_MAX_DAYS_PER_WEEK:
	case T_TEACHERS_MAX_DAYS_PER_WEEK:
		return UiI18n::T("Max days per week:", "أقصى أيام في الأسبوع:");
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
	QComboBox* templateCombo;
	QLabel* hintLabel;
	QLabel* statusLabel;

	QWidget* teacherRow;
	QComboBox* teacherCombo;

	QWidget* studentsRow;
	QComboBox* studentsCombo;

	QWidget* valueRow;
	QLabel* valueLabel;
	QSpinBox* valueSpin;

	QWidget* weightRow;
	QSpinBox* weightSpin;

	QWidget* allowEmptyRow;
	QCheckBox* allowEmptyCheckBox;

	QWidget* timesRow;
	QTableWidget* timesTable;
	QPushButton* selectAllButton;
	QPushButton* clearAllButton;

	QWidget* roomRow;
	QComboBox* roomCombo;

	QListWidget* existingList;
	QLabel* existingCountLabel;

	int currentId() const { return templateCombo->currentData().toInt(); }

	TemplateSpec currentSpec() const { return specFor(currentId()); }

	void reloadTeachers();
	void reloadStudents();
	void reloadRooms();
	void rebuildTimesGrid();
	void updateRows();
	void refreshExisting();
	void markAllTimes(bool checked);
	void addConstraint();
};

TemplateDialog::TemplateDialog(QWidget* parent)
	: QDialog(parent)
{
	addedCount = 0;

	setWindowTitle(UiI18n::T("Constraint templates — essentials", "محرّر القيود بالقوالب — الأساسيات"));
	setMinimumWidth(760);

	// ---------- اختيار القالب ----------
	templateCombo = new QComboBox(this);
	templateCombo->setObjectName(QStringLiteral("otTemplateCombo"));
	for (int id = 0; id < T_COUNT; ++id) {
		templateCombo->addItem(templateTitle(id, UiI18n::isArabic()), id);
	}

	hintLabel = new QLabel(this);
	hintLabel->setWordWrap(true);
	hintLabel->setStyleSheet(QStringLiteral("color:#4b5563;"));

	QGroupBox* templateBox = new QGroupBox(UiI18n::T("Choose a template", "اختر القالب"), this);
	QVBoxLayout* templateLayout = new QVBoxLayout(templateBox);
	templateLayout->addWidget(templateCombo);
	templateLayout->addWidget(hintLabel);

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

	weightSpin = new QSpinBox(this);
	weightSpin->setObjectName(QStringLiteral("otTemplateWeightSpin"));
	weightSpin->setRange(0, 100);
	weightSpin->setValue(100);
	weightSpin->setSuffix(QStringLiteral(" %"));
	weightSpin->setToolTip(UiI18n::T("100 % = a hard constraint (must be respected). Lower values let the generator trade it for a better overall timetable.",
					 "100 % = قيد صارم (يجب احترامه). القيم الأقل تسمح للبرنامج بالتضحية به مقابل جدول أفضل عمومًا."));
	weightRow = new QWidget(this);
	{
		QHBoxLayout* l = new QHBoxLayout(weightRow);
		l->setContentsMargins(0, 0, 0, 0);
		l->addWidget(new QLabel(UiI18n::T("Weight:", "الوزن:"), weightRow));
		l->addWidget(weightSpin);
		l->addStretch(1);
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

	QWidget* paramsWidget = new QWidget(this);
	QVBoxLayout* paramsLayout = new QVBoxLayout(paramsWidget);
	paramsLayout->setContentsMargins(0, 0, 0, 0);
	paramsLayout->addWidget(teacherRow);
	paramsLayout->addWidget(studentsRow);
	paramsLayout->addWidget(valueRow);
	paramsLayout->addWidget(allowEmptyRow);
	paramsLayout->addWidget(roomRow);
	paramsLayout->addWidget(weightRow);
	paramsLayout->addWidget(timesRow, 1);

	// ---------- الأزرار ----------
	QPushButton* addButton = new QPushButton(UiI18n::T("Add constraint", "إضافة القيد"), this);
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
	existingList->setMaximumHeight(120);
	existingCountLabel = new QLabel(this);
	existingCountLabel->setStyleSheet(QStringLiteral("color:#4b5563;"));

	QGroupBox* existingBox = new QGroupBox(UiI18n::T("Existing constraints of this type", "القيود الموجودة من هذا النوع"), this);
	QVBoxLayout* existingLayout = new QVBoxLayout(existingBox);
	existingLayout->addWidget(existingList, 1);
	existingLayout->addWidget(existingCountLabel);

	// ---------- التركيب ----------
	QHBoxLayout* topLayout = new QHBoxLayout();
	topLayout->addWidget(templateBox, 1);
	QWidget* paramsHost = new QWidget(this);
	QVBoxLayout* paramsHostLayout = new QVBoxLayout(paramsHost);
	paramsHostLayout->setContentsMargins(0, 0, 0, 0);
	paramsHostLayout->addWidget(paramsWidget, 1);
	topLayout->addWidget(paramsHost, 1);

	QVBoxLayout* mainLayout = new QVBoxLayout(this);
	mainLayout->addLayout(topLayout, 1);
	mainLayout->addWidget(existingBox);
	mainLayout->addLayout(buttonRow);

	// ---------- الوصل ----------
	connect(templateCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
		[this](int) { updateRows(); refreshExisting(); });
	connect(addButton, &QPushButton::clicked, this, [this]() { addConstraint(); });
	connect(closeButton, &QPushButton::clicked, this, [this]() { accept(); });
	connect(timesTable, &QTableWidget::cellClicked, this, [this](int row, int column) {
		QTableWidgetItem* item = timesTable->item(row, column);
		setSlotSelected(item, !isSlotSelected(item));
	});
	connect(selectAllButton, &QPushButton::clicked, this, [this]() { markAllTimes(true); });
	connect(clearAllButton, &QPushButton::clicked, this, [this]() { markAllTimes(false); });

	reloadTeachers();
	reloadStudents();
	reloadRooms();
	updateRows();
	refreshExisting();
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
	const TemplateSpec spec = currentSpec();

	hintLabel->setText(templateHint(currentId()));

	teacherRow->setVisible(spec.needsTeacher);
	studentsRow->setVisible(spec.needsStudents);
	roomRow->setVisible(spec.needsRoom);
	valueRow->setVisible(spec.needsValue);
	allowEmptyRow->setVisible(spec.needsAllowEmptyDays);
	timesRow->setVisible(spec.needsTimes);

	valueLabel->setText(valueCaption(currentId()));
	valueSpin->setRange(valueMinimum(currentId()), valueMaximum(currentId()));
	valueSpin->setValue(qBound(valueSpin->minimum(), spec.valueDefault, valueSpin->maximum()));

	if (spec.needsTimes) {
		const bool rebuild = (timesTable->rowCount() != gridRows()
				      || timesTable->columnCount() != gridColumns());
		if (rebuild)
			rebuildTimesGrid();
	}

	if (teacherCombo->count() == 0)
		teacherCombo->setEnabled(false);
	else
		teacherCombo->setEnabled(true);

	if (studentsCombo->count() == 0)
		studentsCombo->setEnabled(false);
	else
		studentsCombo->setEnabled(true);

	if (roomCombo->count() == 0)
		roomCombo->setEnabled(false);
	else
		roomCombo->setEnabled(true);

	// «الوزن» لا معنى له مع قيد «غير متاح» (صارم دائمًا في FET) — نُبقيه ظاهرًا
	// لأنه يظل مقبولًا، لكن نشرح ذلك.
	if (spec.needsTimes)
		weightSpin->setToolTip(UiI18n::T("Weights are accepted, but 'not available' is normally a hard constraint.",
						"الوزن مقبول، لكن «غير متاح» عادةً قيد صارم."));
	else
		weightSpin->setToolTip(UiI18n::T("100 % = a hard constraint (must be respected). Lower values let the generator trade it for a better overall timetable.",
						"100 % = قيد صارم (يجب احترامه). القيم الأقل تسمح للبرنامج بالتضحية به مقابل جدول أفضل عمومًا."));

	statusLabel->clear();
}

void TemplateDialog::refreshExisting()
{
	existingList->clear();
	const QList<int> types = currentSpec().fetTypes;
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
		existingCountLabel->setText(UiI18n::T("None yet for this template.", "لا يوجد أي قيد من هذا القالب بعد."));
	else
		existingCountLabel->setText(UiI18n::T("%1 constraint(s) already use this template.", "عدد القيود الموجودة من هذا القالب: %1.").arg(total));

	existingList->setVisible(total > 0);
}

void TemplateDialog::addConstraint()
{
	if (!gt.rules.initialized) {
		QMessageBox::information(this, UiI18n::T("Constraint templates", "القيود بالقوالب"),
					 UiI18n::T("Open or create a file first.", "افتح ملفًا أو أنشئ ملفًا جديدًا أولًا."));
		return;
	}

	const int id = currentId();
	const TemplateSpec spec = currentSpec();
	const double weight = double(weightSpin->value());
	const QString teacher = teacherCombo->currentText().trimmed();
	const QString students = studentsCombo->currentText().trimmed();
	const QString room = roomCombo->currentText().trimmed();

	if (spec.needsTeacher && teacher.isEmpty()) {
		QMessageBox::warning(this, UiI18n::T("Constraint templates", "القيود بالقوالب"),
				     UiI18n::T("Choose a teacher first.", "اختر أستاذًا أولًا."));
		return;
	}
	if (spec.needsStudents && students.isEmpty()) {
		QMessageBox::warning(this, UiI18n::T("Constraint templates", "القيود بالقوالب"),
				     UiI18n::T("Choose a class or group first.", "اختر قسمًا أو فوجًا أولًا."));
		return;
	}
	if (spec.needsRoom && room.isEmpty()) {
		QMessageBox::warning(this, UiI18n::T("Constraint templates", "القيود بالقوالب"),
				     UiI18n::T("Choose a room first.", "اختر قاعة أولًا."));
		return;
	}

	QList<int> days;
	QList<int> hours;
	if (spec.needsTimes) {
		gridToDaysHours(timesTable, days, hours);
		if (days.isEmpty()) {
			QMessageBox::warning(this, UiI18n::T("Constraint templates", "القيود بالقوالب"),
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
	default:
		break;
	}

	bool ok = false;
	QString description;

	if (tc != nullptr) {
		description = tc->getDetailedDescription(gt.rules);
		ok = gt.rules.addTimeConstraint(tc);
		if (ok) {
			gt.rules.addUndoPoint(
				UiI18n::T("Added the constraint: %1", "أُضيف القيد: %1")
					.arg(QStringLiteral("<br /><br />\n") + description));
		}
		else {
			delete tc;
		}
	}
	else if (sc != nullptr) {
		description = sc->getDetailedDescription(gt.rules);
		ok = gt.rules.addSpaceConstraint(sc);
		if (ok) {
			gt.rules.addUndoPoint(
				UiI18n::T("Added the constraint: %1", "أُضيف القيد: %1")
					.arg(QStringLiteral("<br /><br />\n") + description));
		}
		else {
			delete sc;
		}
	}

	if (!ok) {
		QMessageBox::warning(this, UiI18n::T("Constraint templates", "القيود بالقوالب"),
				     UiI18n::T("The constraint could not be added. A similar constraint of this type probably already exists for the same teacher/class.",
					       "تعذّرت إضافة القيد. الأرجح أن قيدًا مشابهًا من النوع نفسه موجود بالفعل لنفس الأستاذ/القسم."));
		return;
	}

	++addedCount;
	statusLabel->setStyleSheet(QStringLiteral("color:#166534;"));
	statusLabel->setText(UiI18n::T("✔ Constraint added. You can add another one.",
				       "✔ أُضيف القيد. يمكنك إضافة قيد آخر."));

	refreshExisting();
	markAllTimes(false);
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
		QMainWindow* window = qobject_cast<QMainWindow*>(parent != nullptr ? parent->window() : nullptr);
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
