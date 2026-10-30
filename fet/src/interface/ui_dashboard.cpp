/*
File ui_dashboard.cpp — Open-Timetable addition

This file is part of Open-Timetable, a modernised user interface for FET.
FET is free software; you can redistribute it and/or modify it under the terms
of the GNU Affero General Public License version 3 (see LICENSE).

ملاحظات تصميمية:
- اللوحة تقرأ فقط من `gt.rules` ولا تعدّل شيئًا.
- كل زر سريع يُشغّل إجراءً أصليًا موجودًا في النافذة الرئيسية (بالاسم)،
  فلا يوجد ازدواج للمنطق ولا نسخ لأكواد النوافذ.
- التحديث يتم بمؤقّت دوري أثناء ظهور اللوحة فقط (تكلفة مهملة).
*/

#include "ui_dashboard.h"

#include "fet.h"
#include "rules.h"
#include "studentsset.h"
#include "ui_i18n.h"

#include <QAction>
#include <QApplication>
#include <QDir>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QShowEvent>
#include <QHideEvent>
#include <QTimer>
#include <QVBoxLayout>

extern bool students_schedule_ready;
extern bool teachers_schedule_ready;
extern bool rooms_buildings_schedule_ready;

namespace {
// الأفواج والفرق الفرعية متداخلة داخل السنوات في بنية FET
int countGroups(const StudentsYearsList& years)
{
	int total = 0;
	for (const StudentsYear* year : years) {
		if (year != nullptr)
			total += year->groupsList.count();
	}
	return total;
}

int countSubgroups(const StudentsYearsList& years)
{
	int total = 0;
	for (const StudentsYear* year : years) {
		if (year == nullptr)
			continue;
		for (const StudentsGroup* group : year->groupsList) {
			if (group != nullptr)
				total += group->subgroupsList.count();
		}
	}
	return total;
}
}

UiDashboard::UiDashboard(QMainWindow* mainWindow, QWidget* parent)
	: QWidget(parent), mainWindow_(mainWindow)
{
	titleLabel_ = nullptr;
	subtitleLabel_ = nullptr;
	fileNameLabel_ = nullptr;
	statusLabel_ = nullptr;

	stats_.teachers = nullptr;
	stats_.subjects = nullptr;
	stats_.years = nullptr;
	stats_.groups = nullptr;
	stats_.subgroups = nullptr;
	stats_.rooms = nullptr;
	stats_.activities = nullptr;
	stats_.timeConstraints = nullptr;
	stats_.spaceConstraints = nullptr;

	QVBoxLayout* root = new QVBoxLayout(this);
	root->setContentsMargins(26, 22, 26, 22);
	root->setSpacing(16);

	buildHeader(this);
	root->addWidget(titleLabel_);
	root->addWidget(subtitleLabel_);
	root->addWidget(fileNameLabel_);
	root->addWidget(buildStatusSection(this));
	root->addWidget(buildStatsSection(this));
	root->addWidget(buildQuickActionsSection(this));
	root->addStretch(1);

	refreshTimer_ = new QTimer(this);
	refreshTimer_->setInterval(2000);
	QObject::connect(refreshTimer_, &QTimer::timeout, this, &UiDashboard::refresh);
	refreshTimer_->stop();

	refresh();
}

UiDashboard::~UiDashboard()
{
}

void UiDashboard::buildHeader(QWidget* host)
{
	Q_UNUSED(host);

	titleLabel_ = new QLabel(this);
	titleLabel_->setObjectName(QStringLiteral("otDashTitle"));
	titleLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);

	subtitleLabel_ = new QLabel(this);
	subtitleLabel_->setObjectName(QStringLiteral("otDashSubtitle"));
	subtitleLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);

	fileNameLabel_ = new QLabel(this);
	fileNameLabel_->setObjectName(QStringLiteral("otDashFile"));
	fileNameLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
}

QWidget* UiDashboard::buildStatusSection(QWidget* host)
{
	Q_UNUSED(host);

	statusLabel_ = new QLabel(this);
	statusLabel_->setObjectName(QStringLiteral("otDashStatus"));
	statusLabel_->setWordWrap(true);
	return statusLabel_;
}

QLabel* UiDashboard::addStatCard(QGridLayout* grid, int row, int column,
				 const QString& label, const QString& accentColor)
{
	QFrame* card = new QFrame(this);
	card->setObjectName(QStringLiteral("otCard"));
	card->setMinimumHeight(84);
	card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

	QVBoxLayout* layout = new QVBoxLayout(card);
	layout->setContentsMargins(16, 12, 16, 12);
	layout->setSpacing(2);

	QLabel* value = new QLabel(card);
	value->setObjectName(QStringLiteral("otCardValue"));
	value->setStyleSheet(QStringLiteral("font-size:22pt; font-weight:700; color:%1;").arg(accentColor));

	QLabel* caption = new QLabel(label, card);
	caption->setObjectName(QStringLiteral("otCardLabel"));
	caption->setStyleSheet(QStringLiteral("color:#5b6675; font-size:9.5pt;"));
	cardCaptions_.append(caption);

	layout->addWidget(value);
	layout->addWidget(caption);

	grid->addWidget(card, row, column);
	return value;
}

QWidget* UiDashboard::buildStatsSection(QWidget* host)
{
	Q_UNUSED(host);

	QWidget* container = new QWidget(this);
	QVBoxLayout* outer = new QVBoxLayout(container);
	outer->setContentsMargins(0, 0, 0, 0);
	outer->setSpacing(10);

	QLabel* heading = new QLabel(UiI18n::T("Data overview", "نظرة عامة على البيانات"), container);
	heading->setObjectName(QStringLiteral("otSectionTitle"));
	heading->setProperty("otKey", QStringLiteral("data-overview"));
	sectionTitles_.append(heading);
	outer->addWidget(heading);

	QWidget* gridHost = new QWidget(container);
	QGridLayout* grid = new QGridLayout(gridHost);
	grid->setContentsMargins(0, 0, 0, 0);
	grid->setHorizontalSpacing(12);
	grid->setVerticalSpacing(12);

	stats_.teachers = addStatCard(grid, 0, 0, UiI18n::T("Teachers", "الأساتذة"), QStringLiteral("#1d4ed8"));
	stats_.subjects = addStatCard(grid, 0, 1, UiI18n::T("Subjects", "المواد"), QStringLiteral("#0f766e"));
	stats_.years = addStatCard(grid, 0, 2, UiI18n::T("Years (classes)", "الأقسام (السنوات)"), QStringLiteral("#7c3aed"));
	stats_.groups = addStatCard(grid, 0, 3, UiI18n::T("Groups", "الأفواج"), QStringLiteral("#9333ea"));

	stats_.subgroups = addStatCard(grid, 1, 0, UiI18n::T("Subgroups", "الفرق الفرعية"), QStringLiteral("#c2410c"));
	stats_.rooms = addStatCard(grid, 1, 1, UiI18n::T("Rooms", "القاعات"), QStringLiteral("#0369a1"));
	stats_.activities = addStatCard(grid, 1, 2, UiI18n::T("Activities", "الأنشطة"), QStringLiteral("#b45309"));
	stats_.timeConstraints = addStatCard(grid, 1, 3, UiI18n::T("Time constraints", "قيود زمنية"), QStringLiteral("#be123c"));

	stats_.spaceConstraints = addStatCard(grid, 2, 0, UiI18n::T("Space constraints", "قيود مكانية"), QStringLiteral("#4d7c0f"));

	outer->addWidget(gridHost);
	return container;
}

QWidget* UiDashboard::addQuickButton(QGridLayout* grid, int row, int column,
				     const QString& actionName, const QString& text,
				     const QString& tooltip)
{
	QPushButton* button = new QPushButton(text, this);
	button->setProperty("otAction", actionName);
	button->setCursor(Qt::PointingHandCursor);
	button->setMinimumHeight(38);
	button->setToolTip(tooltip);

	QAction* action = nullptr;
	if (mainWindow_ != nullptr)
		action = mainWindow_->findChild<QAction*>(actionName);
	if (action == nullptr) {
		button->setEnabled(false);
		button->setToolTip(QStringLiteral("غير متاح في هذه النسخة"));
	}
	else {
		// نربط الزر بالإجراء الأصلي: نفس السلوك تمامًا، بلا منطق مكرّر.
		QObject::connect(button, &QPushButton::clicked, action, &QAction::trigger);
	}

	if (action != nullptr)
		quickActions_.append(action);
	grid->addWidget(button, row, column);
	quickButtons_.append(button);
	return button;
}

QWidget* UiDashboard::buildQuickActionsSection(QWidget* host)
{
	Q_UNUSED(host);

	QWidget* container = new QWidget(this);
	QVBoxLayout* outer = new QVBoxLayout(container);
	outer->setContentsMargins(0, 6, 0, 0);
	outer->setSpacing(10);

	QLabel* heading = new QLabel(UiI18n::T("Quick actions", "إجراءات سريعة"), container);
	heading->setObjectName(QStringLiteral("otSectionTitle"));
	heading->setProperty("otKey", QStringLiteral("quick-actions"));
	sectionTitles_.append(heading);
	outer->addWidget(heading);

	QWidget* gridHost = new QWidget(container);
	QGridLayout* grid = new QGridLayout(gridHost);
	grid->setContentsMargins(0, 0, 0, 0);
	grid->setHorizontalSpacing(10);
	grid->setVerticalSpacing(10);

	// النصوص تُشتق من اسم إجراء FET عبر quickButtonText() حتى تتبدّل مع اللغة
	const char* const actionNames[] = {
		"dataTeachersAction", "dataSubjectsAction", "dataYearsAction", "dataRoomsAction",
		"dataActivitiesAction", "dataAllTimeConstraintsAction", "dataAllSpaceConstraintsAction",
		"timetableGenerateAction",
		"timetableViewTeachersTimeHorizontalAction", "timetableViewStudentsTimeHorizontalAction",
		"timetableViewRoomsTimeHorizontalAction", "filePublishToShareAction"
	};
	const char* const tooltipsEn[] = {
		"Add or edit teachers", "Add or edit subjects", "Manage years, groups and subgroups",
		"Manage rooms and buildings", "Manage teaching activities", "All time constraints",
		"All room constraints", "Start timetable generation", "View teachers' timetables",
		"View students' timetables", "View rooms' timetables",
		"Copy the timetables to a shared folder on the school network"
	};
	const char* const tooltipsAr[] = {
		"إضافة/تعديل قائمة الأساتذة", "إضافة/تعديل المواد", "إدارة السنوات والأفواج والفرق",
		"إدارة القاعات والمباني", "إدارة أنشطة التدريس", "كل القيود الزمنية",
		"كل قيود القاعات", "بدء توليد الجدول الزمني", "عرض جداول الأساتذة بعد التوليد",
		"عرض جداول الأقسام بعد التوليد", "عرض جداول القاعات بعد التوليد",
		"نسخ الجداول إلى مجلد مشترك على شبكة المؤسسة"
	};

	const int count = int(sizeof(actionNames) / sizeof(actionNames[0]));
	for (int i = 0; i < count; ++i) {
		const QString actionName = QString::fromLatin1(actionNames[i]);
		addQuickButton(grid, i / 4, i % 4, actionName,
			       quickButtonText(actionName),
			       UiI18n::T(tooltipsEn[i], tooltipsAr[i]));
	}

	outer->addWidget(gridHost);
	return container;
}

QString UiDashboard::formatCount(int value) const
{
	return QString::number(value);
}

QString UiDashboard::quickButtonText(const QString& actionName)
{
	if (actionName == QLatin1String("dataTeachersAction"))
		return UiI18n::T("Teachers", "الأساتذة");
	if (actionName == QLatin1String("dataSubjectsAction"))
		return UiI18n::T("Subjects", "المواد");
	if (actionName == QLatin1String("dataYearsAction"))
		return UiI18n::T("Years and groups", "الأقسام والأفواج");
	if (actionName == QLatin1String("dataRoomsAction"))
		return UiI18n::T("Rooms", "القاعات");
	if (actionName == QLatin1String("dataActivitiesAction"))
		return UiI18n::T("Activities", "الأنشطة");
	if (actionName == QLatin1String("dataAllTimeConstraintsAction"))
		return UiI18n::T("Time constraints", "القيود الزمنية");
	if (actionName == QLatin1String("dataAllSpaceConstraintsAction"))
		return UiI18n::T("Space constraints", "القيود المكانية");
	if (actionName == QLatin1String("timetableGenerateAction"))
		return UiI18n::T("Generate timetable", "توليد الجدول");
	if (actionName == QLatin1String("timetableViewTeachersTimeHorizontalAction"))
		return UiI18n::T("Teachers' timetables", "جداول الأساتذة");
	if (actionName == QLatin1String("timetableViewStudentsTimeHorizontalAction"))
		return UiI18n::T("Students' timetables", "جداول الأقسام");
	if (actionName == QLatin1String("timetableViewRoomsTimeHorizontalAction"))
		return UiI18n::T("Rooms' timetables", "جداول القاعات");
	if (actionName == QLatin1String("filePublishToShareAction"))
		return UiI18n::T("Publish timetables", "نشر الجداول");
	return actionName;
}

void UiDashboard::retranslate()
{
	// عناوين الأقسام
	for (QLabel* label : sectionTitles_) {
		if (label == nullptr)
			continue;
		const QString key = label->property("otKey").toString();
		if (key == QLatin1String("data-overview"))
			label->setText(UiI18n::T("Data overview", "نظرة عامة على البيانات"));
		else if (key == QLatin1String("quick-actions"))
			label->setText(UiI18n::T("Quick actions", "إجراءات سريعة"));
	}

	// نصوص البطاقات (بنفس ترتيب الإنشاء)
	static const char* const captionsEn[] = {
		"Teachers", "Subjects", "Years (classes)", "Groups", "Subgroups",
		"Rooms", "Activities", "Time constraints", "Space constraints"
	};
	static const char* const captionsAr[] = {
		"الأساتذة", "المواد", "الأقسام (السنوات)", "الأفواج", "الفرق الفرعية",
		"القاعات", "الأنشطة", "قيود زمنية", "قيود مكانية"
	};
	const int captionCount = int(sizeof(captionsAr) / sizeof(captionsAr[0]));
	for (int i = 0; i < cardCaptions_.size() && i < captionCount; ++i) {
		if (cardCaptions_.at(i) != nullptr)
			cardCaptions_.at(i)->setText(UiI18n::T(captionsEn[i], captionsAr[i]));
	}

	// نصوص الأزرار السريعة
	for (QWidget* widget : quickButtons_) {
		QPushButton* button = qobject_cast<QPushButton*>(widget);
		if (button == nullptr)
			continue;
		const QString actionName = button->property("otAction").toString();
		if (!actionName.isEmpty())
			button->setText(quickButtonText(actionName));
	}

	// بقية النصوص (العنوان، الحالة) تُحسب في refresh()
	refresh();
}

void UiDashboard::refresh()
{
	const bool initialized = gt.rules.initialized;

	// العنوان
	QString institution = gt.rules.institutionName.trimmed();
	if (institution.isEmpty())
		institution = UiI18n::T("New project", "مشروع جديد");
	titleLabel_->setText(initialized ? institution
					 : UiI18n::T("Welcome to Open-Timetable", "مرحبًا بك في Open-Timetable"));

	// السطر الفرعي
	if (initialized) {
		subtitleLabel_->setText(UiI18n::T("%1 teachers • %2 subjects • %3 years • %4 rooms",
						  "%1 أستاذًا • %2 مادة • %3 قسمًا • %4 قاعة")
						.arg(gt.rules.teachersList.count())
						.arg(gt.rules.subjectsList.count())
						.arg(gt.rules.yearsList.count())
						.arg(gt.rules.roomsList.count()));
	}
	else {
		subtitleLabel_->setText(UiI18n::T("Start a new file or open an existing one from the File menu.",
						  "ابدأ ملفًا جديدًا أو افتح ملفًا موجودًا من قائمة «ملف»."));
	}

	// اسم الملف
	if (INPUT_FILENAME_XML.isEmpty())
		fileNameLabel_->setText(UiI18n::T("File: — (not saved yet)", "الملف: — (لم يُحفظ بعد)"));
	else
		fileNameLabel_->setText(UiI18n::T("File: %1", "الملف: %1")
						.arg(QDir::toNativeSeparators(INPUT_FILENAME_XML)));

	// حالة الجدول
	const bool scheduleReady = students_schedule_ready && teachers_schedule_ready && rooms_buildings_schedule_ready;
	if (!initialized) {
		statusLabel_->setText(UiI18n::T("No data yet.", "لا توجد بيانات بعد."));
		statusLabel_->setStyleSheet(QStringLiteral("color:#5b6675; font-size:10pt;"));
	}
	else if (scheduleReady) {
		statusLabel_->setText(UiI18n::T("✔ Timetable generated and ready to view or publish.",
						"✔ الجدول مولّد وجاهز للعرض والنشر."));
		statusLabel_->setStyleSheet(QStringLiteral("color:#166534; font-size:10pt; font-weight:600;"));
	}
	else {
		statusLabel_->setText(UiI18n::T("● Data ready, timetable not generated yet — use \"Generate timetable\".",
						"● البيانات جاهزة، والجدول لم يُولّد بعد — استخدم «توليد الجدول»."));
		statusLabel_->setStyleSheet(QStringLiteral("color:#b45309; font-size:10pt; font-weight:600;"));
	}

	// الأعداد
	const QString dash = QStringLiteral("—");
	stats_.teachers->setText(initialized ? formatCount(gt.rules.teachersList.count()) : dash);
	stats_.subjects->setText(initialized ? formatCount(gt.rules.subjectsList.count()) : dash);
	stats_.years->setText(initialized ? formatCount(gt.rules.yearsList.count()) : dash);
	stats_.groups->setText(initialized ? formatCount(countGroups(gt.rules.yearsList)) : dash);
	stats_.subgroups->setText(initialized ? formatCount(countSubgroups(gt.rules.yearsList)) : dash);
	stats_.rooms->setText(initialized ? formatCount(gt.rules.roomsList.count()) : dash);
	stats_.activities->setText(initialized ? formatCount(gt.rules.activitiesList.count()) : dash);
	stats_.timeConstraints->setText(initialized ? formatCount(gt.rules.timeConstraintsList.count()) : dash);
	stats_.spaceConstraints->setText(initialized ? formatCount(gt.rules.spaceConstraintsList.count()) : dash);
}

void UiDashboard::showEvent(QShowEvent* event)
{
	QWidget::showEvent(event);
	refresh();
	if (refreshTimer_ != nullptr)
		refreshTimer_->start();
}

void UiDashboard::hideEvent(QHideEvent* event)
{
	if (refreshTimer_ != nullptr)
		refreshTimer_->stop();
	QWidget::hideEvent(event);
}
