/*
File ui_shell.cpp — Open-Timetable addition

This file is part of Open-Timetable, a modernised user interface for FET.
FET is free software; you can redistribute it and/or modify it under the terms
of the GNU Affero General Public License version 3 (see LICENSE).

البنية:
  QMainWindow
   └─ QSplitter (اتجاه أفقي)
       ├─ الشريط الجانبي (QListWidget مخصّص)
       └─ QStackedWidget
            ├─ [0] لوحة البداية      (UiDashboard)
            └─ [1] الواجهة الكلاسيكية (mainwidget الأصلية كما هي)

لا نحذف أي عنصر من الواجهة الأصلية: هي فقط تنتقل إلى القائمة الجانبية
كصفحة «الواجهة الكلاسيكية»، وكل أزرارها وقوائمها تعمل كما كانت.

كل النصوص تمرّ عبر UiI18n::T() حتى تتبدّل فور تغيير اللغة (عربي/إنجليزي).
*/

#include "ui_shell.h"

#include "ui_constraints.h"
#include "ui_dashboard.h"
#include "ui_i18n.h"

#include <QAction>
#include <QKeySequence>
#include <QMenu>
#include <QApplication>
#include <QColor>
#include <QFont>
#include <QFrame>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMainWindow>
#include <QPushButton>
#include <QSettings>
#include <QSplitter>
#include <QStackedWidget>
#include <QStyle>
#include <QVBoxLayout>
#include <QWidget>

namespace {

const char* const PROP_INSTALLED = "ot_shell_installed";

enum SidebarRole {
	RoleActionName = Qt::UserRole + 1, // QString: اسم إجراء FET
	RolePageIndex                      // int: صفحة في QStackedWidget
};

const int PAGE_HOME = 0;
const int PAGE_CLASSIC = 1;

QListWidgetItem* addSection(QListWidget* nav, const QString& title)
{
	QListWidgetItem* item = new QListWidgetItem(title, nav);
	item->setFlags(Qt::NoItemFlags);
	QFont font = item->font();
	font.setBold(true);
	font.setPointSizeF(qMax(8.0, font.pointSizeF() - 0.5));
	item->setFont(font);
	item->setForeground(QColor(QStringLiteral("#8a94a3")));
	item->setTextAlignment((UiI18n::isArabic() ? Qt::AlignRight : Qt::AlignLeft) | Qt::AlignVCenter);
	return item;
}

QListWidgetItem* addNavItem(QListWidget* nav, const QString& text,
			    const QString& actionName = QString(),
			    int pageIndex = -1,
			    const QString& tooltip = QString())
{
	QListWidgetItem* item = new QListWidgetItem(text, nav);
	item->setTextAlignment((UiI18n::isArabic() ? Qt::AlignRight : Qt::AlignLeft) | Qt::AlignVCenter);
	if (!actionName.isEmpty())
		item->setData(RoleActionName, actionName);
	if (pageIndex >= 0)
		item->setData(RolePageIndex, pageIndex);
	if (!tooltip.isEmpty())
		item->setToolTip(tooltip);
	return item;
}

// يُعيد بناء عناصر الشريط الجانبي باللغة الحالية، ويُبقي التحديد على نفس العنصر.
void buildNavItems(QListWidget* nav)
{
	// نحفظ العنصر المحدّد (بمفتاحه: إجراء أو صفحة) لاستعادته بعد إعادة البناء
	QString selAction;
	int selPage = -1;
	const QListWidgetItem* currentItem = nav->currentItem();
	if (currentItem != nullptr) {
		selAction = currentItem->data(RoleActionName).toString();
		if (currentItem->data(RolePageIndex).isValid())
			selPage = currentItem->data(RolePageIndex).toInt();
	}

	nav->clear();

	addNavItem(nav, UiI18n::T("Home dashboard", "لوحة البداية"), QString(), PAGE_HOME,
		   UiI18n::T("Project summary and quick actions",
			     "ملخّص المشروع والإجراءات السريعة"));
	addNavItem(nav, UiI18n::T("Classic interface", "الواجهة الكلاسيكية"), QString(), PAGE_CLASSIC,
		   UiI18n::T("Original FET buttons (file, data, constraints, timetable)",
			     "أزرار FET الأصلية (الملف، البيانات، القيود، الجدول)"));

	addSection(nav, UiI18n::T("Data", "البيانات"));
	addNavItem(nav, UiI18n::T("Teachers", "الأساتذة"), QStringLiteral("dataTeachersAction"));
	addNavItem(nav, UiI18n::T("Subjects", "المواد"), QStringLiteral("dataSubjectsAction"));
	addNavItem(nav, UiI18n::T("Years and groups", "الأقسام والأفواج"), QStringLiteral("dataYearsAction"));
	addNavItem(nav, UiI18n::T("Subgroups", "الفرق الفرعية"), QStringLiteral("dataSubgroupsAction"));
	addNavItem(nav, UiI18n::T("Rooms", "القاعات"), QStringLiteral("dataRoomsAction"));
	addNavItem(nav, UiI18n::T("Buildings", "المباني"), QStringLiteral("dataBuildingsAction"));
	addNavItem(nav, UiI18n::T("Activities", "الأنشطة"), QStringLiteral("dataActivitiesAction"));

	addSection(nav, UiI18n::T("Constraints", "القيود"));
	addNavItem(nav, UiI18n::T("Constraint templates…", "القيود بالقوالب…"),
		   QStringLiteral("otConstraintTemplatesAction"), -1,
		   UiI18n::T("Add the everyday constraints through a simple form",
			     "إضافة القيود اليومية عبر نموذج مبسّط"));
	addNavItem(nav, UiI18n::T("All time constraints", "كل القيود الزمنية"),
		   QStringLiteral("dataAllTimeConstraintsAction"));
	addNavItem(nav, UiI18n::T("All space constraints", "كل القيود المكانية"),
		   QStringLiteral("dataAllSpaceConstraintsAction"));

	addSection(nav, UiI18n::T("Generation and timetables", "التوليد والجداول"));
	addNavItem(nav, UiI18n::T("Generate timetable", "توليد الجدول"),
		   QStringLiteral("timetableGenerateAction"));
	addNavItem(nav, UiI18n::T("Teachers' timetables", "جداول الأساتذة"),
		   QStringLiteral("timetableViewTeachersTimeHorizontalAction"));
	addNavItem(nav, UiI18n::T("Students' timetables", "جداول الأقسام"),
		   QStringLiteral("timetableViewStudentsTimeHorizontalAction"));
	addNavItem(nav, UiI18n::T("Rooms' timetables", "جداول القاعات"),
		   QStringLiteral("timetableViewRoomsTimeHorizontalAction"));
	addNavItem(nav, UiI18n::T("Conflicts", "التعارضات"),
		   QStringLiteral("timetableShowConflictsAction"));

	addSection(nav, UiI18n::T("Publish", "النشر"));
	addNavItem(nav, UiI18n::T("Publish timetables…", "نشر الجداول…"),
		   QStringLiteral("filePublishToShareAction"), -1,
		   UiI18n::T("Copy the timetables to a shared folder on the school network",
			     "نسخ الجداول إلى مجلد مشترك على شبكة المؤسسة"));

	// استعادة التحديد
	int rowToSelect = 0;
	for (int i = 0; i < nav->count(); ++i) {
		QListWidgetItem* item = nav->item(i);
		if (!selAction.isEmpty() && item->data(RoleActionName).toString() == selAction) {
			rowToSelect = i;
			break;
		}
		if (selPage >= 0 && item->data(RolePageIndex).isValid()
		    && item->data(RolePageIndex).toInt() == selPage) {
			rowToSelect = i;
			break;
		}
	}
	nav->setCurrentRow(rowToSelect);
}

void saveNavIndex(int row)
{
	QSettings s(QStringLiteral("fet"), QStringLiteral("fettimetabling"));
	s.setValue(QStringLiteral("OpenTimetable/shell/nav-index"), row);
}

} // namespace

namespace UiShell {

void install(QMainWindow* window)
{
	if (window == nullptr)
		return;
	if (window->property(PROP_INSTALLED).toBool())
		return;

	QWidget* classicPage = window->centralWidget();
	if (classicPage == nullptr)
		return;

	QSettings settings(QStringLiteral("fet"), QStringLiteral("fettimetabling"));

	// ---------- إجراء «القيود بالقوالب» (المرحلة 4) ----------
	// نُنشئه كإجراء حقيقي على النافذة حتى يعمل مع آلية الشريط الجانبي نفسها،
	// ويظهر أيضًا في قائمة «البيانات» الأصلية.
	if (window->findChild<QAction*>(QStringLiteral("otConstraintTemplatesAction")) == nullptr) {
		QAction* templatesAction = new QAction(window);
		templatesAction->setObjectName(QStringLiteral("otConstraintTemplatesAction"));
		templatesAction->setText(UiI18n::T("Constraint templates…", "القيود بالقوالب…"));
		templatesAction->setToolTip(UiI18n::T("Add the everyday constraints through a simple form",
						       "إضافة القيود اليومية عبر نموذج مبسّط"));
		templatesAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+T")));
		QObject::connect(templatesAction, &QAction::triggered, window,
				 [window]() { UiConstraints::run(window); });
		window->addAction(templatesAction);

		QMenu* dataMenu = window->findChild<QMenu*>(QStringLiteral("dataMenu"));
		if (dataMenu != nullptr)
			dataMenu->insertAction(dataMenu->actions().value(0), templatesAction);
	}

	// ---------- الصفحات ----------
	UiDashboard* dashboard = new UiDashboard(window);
	QStackedWidget* stack = new QStackedWidget(window);
	stack->addWidget(dashboard);   // PAGE_HOME
	stack->addWidget(classicPage); // PAGE_CLASSIC

	// ---------- الشريط الجانبي ----------
	QWidget* sidebar = new QWidget(window);
	sidebar->setObjectName(QStringLiteral("otSidebar"));
	sidebar->setMinimumWidth(196);
	sidebar->setMaximumWidth(320);

	QVBoxLayout* sideLayout = new QVBoxLayout(sidebar);
	sideLayout->setContentsMargins(0, 0, 0, 0);
	sideLayout->setSpacing(0);

	QLabel* brand = new QLabel(QStringLiteral("Open-Timetable"), sidebar);
	brand->setObjectName(QStringLiteral("otBrand"));
	brand->setContentsMargins(16, 16, 16, 2);

	QLabel* brandSub = new QLabel(sidebar);
	brandSub->setObjectName(QStringLiteral("otBrandSub"));
	brandSub->setContentsMargins(16, 0, 16, 12);
	brandSub->setText(UiI18n::T("Timetables powered by FET", "جداول دراسية بمعايير FET"));

	QListWidget* nav = new QListWidget(sidebar);
	nav->setObjectName(QStringLiteral("otNav"));
	nav->setFrameShape(QFrame::NoFrame);
	nav->setWordWrap(true);
	nav->setSelectionMode(QAbstractItemView::SingleSelection);
	nav->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	nav->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

	sideLayout->addWidget(brand);
	sideLayout->addWidget(brandSub);
	sideLayout->addWidget(nav, 1);

	buildNavItems(nav);

	// ---------- التجميع ----------
	QSplitter* splitter = new QSplitter(Qt::Horizontal, window);
	splitter->setObjectName(QStringLiteral("otShellSplitter"));
	splitter->setChildrenCollapsible(false);
	splitter->addWidget(sidebar);
	splitter->addWidget(stack);
	splitter->setStretchFactor(0, 0);
	splitter->setStretchFactor(1, 1);

	// العرض حسب اتجاه الواجهة (RTL: الشريط على اليمين تلقائيًا)
	window->setCentralWidget(splitter);
	window->setProperty(PROP_INSTALLED, true);

	// ---------- السلوك ----------
	QObject::connect(nav, &QListWidget::itemClicked, window, [window, stack](QListWidgetItem* item) {
		if (item == nullptr)
			return;

		if (item->data(RolePageIndex).isValid()) {
			stack->setCurrentIndex(item->data(RolePageIndex).toInt());
			return;
		}

		const QString actionName = item->data(RoleActionName).toString();
		if (actionName.isEmpty())
			return;

		QAction* action = window->findChild<QAction*>(actionName);
		if (action != nullptr && action->isEnabled())
			action->trigger();
	});

	// تحديث اللوحة عند العودة إليها أو عند تفعيل النافذة
	QObject::connect(stack, &QStackedWidget::currentChanged, window, [dashboard](int index) {
		if (index == PAGE_HOME && dashboard != nullptr)
			dashboard->refresh();
	});
	QObject::connect(qApp, &QApplication::applicationStateChanged, window,
			 [dashboard](Qt::ApplicationState state) {
				 if (state == Qt::ApplicationActive && dashboard != nullptr)
					 dashboard->refresh();
			 });

	// ---------- الحفظ والاستعادة ----------
	const int savedIndex = settings.value(QStringLiteral("OpenTimetable/shell/nav-index"), PAGE_HOME).toInt();
	if (savedIndex >= 0 && savedIndex < nav->count()) {
		nav->setCurrentRow(savedIndex);
		const QVariant page = nav->item(savedIndex)->data(RolePageIndex);
		if (page.isValid())
			stack->setCurrentIndex(page.toInt());
	}
	else {
		stack->setCurrentIndex(PAGE_HOME);
		nav->setCurrentRow(0);
	}

	const int savedWidth = settings.value(QStringLiteral("OpenTimetable/shell/sidebar-width"), 220).toInt();
	QList<int> sizes;
	sizes << qBound(196, savedWidth, 320) << 1000;
	splitter->setSizes(sizes);

	QObject::connect(splitter, &QSplitter::splitterMoved, window, [splitter](int pos, int index) {
		Q_UNUSED(index);
		Q_UNUSED(pos);
		QList<int> current = splitter->sizes();
		if (current.isEmpty())
			return;
		// QSettings غير قابل للنسخ، لذا ننشئه هنا (عملية نادرة ورخيصة)
		QSettings s(QStringLiteral("fet"), QStringLiteral("fettimetabling"));
		s.setValue(QStringLiteral("OpenTimetable/shell/sidebar-width"), current.first());
	});

	QObject::connect(nav, &QListWidget::currentRowChanged, window, [](int row) {
		saveNavIndex(row);
	});
}

void refreshDashboard(QMainWindow* window)
{
	if (window == nullptr)
		return;
	QSplitter* splitter = window->findChild<QSplitter*>(QStringLiteral("otShellSplitter"));
	if (splitter == nullptr)
		return;
	UiDashboard* dashboard = splitter->findChild<UiDashboard*>();
	if (dashboard != nullptr)
		dashboard->refresh();
}

void showHome(QMainWindow* window)
{
	if (window == nullptr)
		return;

	QSplitter* splitter = window->findChild<QSplitter*>(QStringLiteral("otShellSplitter"));
	if (splitter == nullptr)
		return;
	QStackedWidget* stack = splitter->findChild<QStackedWidget*>();
	if (stack == nullptr)
		return;
	stack->setCurrentIndex(PAGE_HOME);

	QListWidget* nav = splitter->findChild<QListWidget*>(QStringLiteral("otNav"));
	if (nav != nullptr && nav->count() > 0)
		nav->setCurrentRow(0);
}

void retranslate(QMainWindow* window)
{
	if (window == nullptr || !window->property(PROP_INSTALLED).toBool())
		return;

	QSplitter* splitter = window->findChild<QSplitter*>(QStringLiteral("otShellSplitter"));
	if (splitter == nullptr)
		return;

	// نص إجراء «القيود بالقوالب» (يظهر في قائمة البيانات أيضًا)
	if (QAction* templatesAction = window->findChild<QAction*>(QStringLiteral("otConstraintTemplatesAction"))) {
		templatesAction->setText(UiI18n::T("Constraint templates…", "القيود بالقوالب…"));
		templatesAction->setToolTip(UiI18n::T("Add the everyday constraints through a simple form",
						      "إضافة القيود اليومية عبر نموذج مبسّط"));
	}

	// نصوص الشريط الجانبي
	QListWidget* nav = splitter->findChild<QListWidget*>(QStringLiteral("otNav"));
	if (nav != nullptr)
		buildNavItems(nav);

	// النص الفرعي للعلامة
	const QList<QLabel*> labels = splitter->findChildren<QLabel*>(QStringLiteral("otBrandSub"));
	for (QLabel* label : labels)
		label->setText(UiI18n::T("Timetables powered by FET", "جداول دراسية بمعايير FET"));

	// لوحة البداية
	UiDashboard* dashboard = splitter->findChild<UiDashboard*>();
	if (dashboard != nullptr)
		dashboard->retranslate();
}

} // namespace UiShell
