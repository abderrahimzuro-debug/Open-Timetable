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

	QLabel* heading = new QLabel(QStringLiteral("نظرة عامة على البيانات"), container);
	heading->setObjectName(QStringLiteral("otSectionTitle"));
	outer->addWidget(heading);

	QWidget* gridHost = new QWidget(container);
	QGridLayout* grid = new QGridLayout(gridHost);
	grid->setContentsMargins(0, 0, 0, 0);
	grid->setHorizontalSpacing(12);
	grid->setVerticalSpacing(12);

	stats_.teachers = addStatCard(grid, 0, 0, QStringLiteral("الأساتذة"), QStringLiteral("#1d4ed8"));
	stats_.subjects = addStatCard(grid, 0, 1, QStringLiteral("المواد"), QStringLiteral("#0f766e"));
	stats_.years = addStatCard(grid, 0, 2, QStringLiteral("الأقسام (السنوات)"), QStringLiteral("#7c3aed"));
	stats_.groups = addStatCard(grid, 0, 3, QStringLiteral("الأفواج"), QStringLiteral("#9333ea"));

	stats_.subgroups = addStatCard(grid, 1, 0, QStringLiteral("الفرق الفرعية"), QStringLiteral("#c2410c"));
	stats_.rooms = addStatCard(grid, 1, 1, QStringLiteral("القاعات"), QStringLiteral("#0369a1"));
	stats_.activities = addStatCard(grid, 1, 2, QStringLiteral("الأنشطة"), QStringLiteral("#b45309"));
	stats_.timeConstraints = addStatCard(grid, 1, 3, QStringLiteral("قيود زمنية"), QStringLiteral("#be123c"));

	stats_.spaceConstraints = addStatCard(grid, 2, 0, QStringLiteral("قيود مكانية"), QStringLiteral("#4d7c0f"));

	outer->addWidget(gridHost);
	return container;
}

QWidget* UiDashboard::addQuickButton(QGridLayout* grid, int row, int column,
				     const QString& actionName, const QString& text,
				     const QString& tooltip)
{
	QPushButton* button = new QPushButton(text, this);
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

	QLabel* heading = new QLabel(QStringLiteral("إجراءات سريعة"), container);
	heading->setObjectName(QStringLiteral("otSectionTitle"));
	outer->addWidget(heading);

	QWidget* gridHost = new QWidget(container);
	QGridLayout* grid = new QGridLayout(gridHost);
	grid->setContentsMargins(0, 0, 0, 0);
	grid->setHorizontalSpacing(10);
	grid->setVerticalSpacing(10);

	addQuickButton(grid, 0, 0, QStringLiteral("dataTeachersAction"),
		       QStringLiteral("الأساتذة"), QStringLiteral("إضافة/تعديل قائمة الأساتذة"));
	addQuickButton(grid, 0, 1, QStringLiteral("dataSubjectsAction"),
		       QStringLiteral("المواد"), QStringLiteral("إضافة/تعديل المواد"));
	addQuickButton(grid, 0, 2, QStringLiteral("dataYearsAction"),
		       QStringLiteral("الأقسام والأفواج"), QStringLiteral("إدارة السنوات والأفواج والفرق"));
	addQuickButton(grid, 0, 3, QStringLiteral("dataRoomsAction"),
		       QStringLiteral("القاعات"), QStringLiteral("إدارة القاعات والمباني"));

	addQuickButton(grid, 1, 0, QStringLiteral("dataActivitiesAction"),
		       QStringLiteral("الأنشطة"), QStringLiteral("إدارة أنشطة التدريس"));
	addQuickButton(grid, 1, 1, QStringLiteral("dataAllTimeConstraintsAction"),
		       QStringLiteral("القيود الزمنية"), QStringLiteral("كل القيود الزمنية"));
	addQuickButton(grid, 1, 2, QStringLiteral("dataAllSpaceConstraintsAction"),
		       QStringLiteral("القيود المكانية"), QStringLiteral("كل قيود القاعات"));
	addQuickButton(grid, 1, 3, QStringLiteral("timetableGenerateAction"),
		       QStringLiteral("توليد الجدول"), QStringLiteral("بدء توليد الجدول الزمني"));

	addQuickButton(grid, 2, 0, QStringLiteral("timetableViewTeachersTimeHorizontalAction"),
		       QStringLiteral("جداول الأساتذة"), QStringLiteral("عرض جداول الأساتذة بعد التوليد"));
	addQuickButton(grid, 2, 1, QStringLiteral("timetableViewStudentsTimeHorizontalAction"),
		       QStringLiteral("جداول الأقسام"), QStringLiteral("عرض جداول الأقسام بعد التوليد"));
	addQuickButton(grid, 2, 2, QStringLiteral("timetableViewRoomsTimeHorizontalAction"),
		       QStringLiteral("جداول القاعات"), QStringLiteral("عرض جداول القاعات بعد التوليد"));
	addQuickButton(grid, 2, 3, QStringLiteral("filePublishToShareAction"),
		       QStringLiteral("نشر الجداول"), QStringLiteral("نسخ الجداول إلى مجلد مشترك على الشبكة"));

	outer->addWidget(gridHost);
	return container;
}

QString UiDashboard::formatCount(int value) const
{
	return QString::number(value);
}

void UiDashboard::refresh()
{
	const bool initialized = gt.rules.initialized;

	// العنوان
	QString institution = gt.rules.institutionName.trimmed();
	if (institution.isEmpty())
		institution = QStringLiteral("مشروع جديد");
	titleLabel_->setText(initialized ? institution : QStringLiteral("مرحبًا بك في Open-Timetable"));

	// السطر الفرعي
	if (initialized) {
		subtitleLabel_->setText(QStringLiteral("%1 أستاذًا • %2 مادة • %3 قسمًا • %4 قاعة")
						.arg(gt.rules.teachersList.count())
						.arg(gt.rules.subjectsList.count())
						.arg(gt.rules.yearsList.count())
						.arg(gt.rules.roomsList.count()));
	}
	else {
		subtitleLabel_->setText(QStringLiteral("ابدأ ملفًا جديدًا أو افتح ملفًا موجودًا من قائمة «ملف»."));
	}

	// اسم الملف
	if (INPUT_FILENAME_XML.isEmpty())
		fileNameLabel_->setText(QStringLiteral("الملف: — (لم يُحفظ بعد)"));
	else
		fileNameLabel_->setText(QStringLiteral("الملف: %1").arg(QDir::toNativeSeparators(INPUT_FILENAME_XML)));

	// حالة الجدول
	const bool scheduleReady = students_schedule_ready && teachers_schedule_ready && rooms_buildings_schedule_ready;
	if (!initialized) {
		statusLabel_->setText(QStringLiteral("لا توجد بيانات بعد."));
		statusLabel_->setStyleSheet(QStringLiteral("color:#5b6675; font-size:10pt;"));
	}
	else if (scheduleReady) {
		statusLabel_->setText(QStringLiteral("✔ الجدول مولّد وجاهز للعرض والنشر."));
		statusLabel_->setStyleSheet(QStringLiteral("color:#166534; font-size:10pt; font-weight:600;"));
	}
	else {
		statusLabel_->setText(QStringLiteral("● البيانات جاهزة، والجدول لم يُولّد بعد — استخدم «توليد الجدول»."));
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
