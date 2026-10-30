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
*/

#include "ui_shell.h"

#include "ui_dashboard.h"

#include <QAction>
#include <QApplication>
#include <QColor>
#include <QFont>
#include <QFrame>
#include <QIcon>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMainWindow>
#include <QSettings>
#include <QSplitter>
#include <QStackedWidget>
#include <QStyle>
#include <QVBoxLayout>
#include <QWidget>

namespace {

const char* const PROP_INSTALLED = "ot_shell_installed";
const char* const SETTINGS_GROUP = "OpenTimetable/shell";

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
	item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
	return item;
}

QListWidgetItem* addNavItem(QListWidget* nav, const QString& text,
			    const QString& actionName = QString(),
			    int pageIndex = -1,
			    const QString& tooltip = QString())
{
	QListWidgetItem* item = new QListWidgetItem(text, nav);
	item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
	if (!actionName.isEmpty())
		item->setData(RoleActionName, actionName);
	if (pageIndex >= 0)
		item->setData(RolePageIndex, pageIndex);
	if (!tooltip.isEmpty())
		item->setToolTip(tooltip);
	return item;
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

	QLabel* brandSub = new QLabel(QStringLiteral("جداول دراسية بمعايير FET"), sidebar);
	brandSub->setObjectName(QStringLiteral("otBrandSub"));
	brandSub->setContentsMargins(16, 0, 16, 12);

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

	// عناصر التنقّل: نبدأ بالصفحتين، ثم اختصارات تُشغّل إجراءات FET مباشرة.
	addNavItem(nav, QStringLiteral("لوحة البداية"), QString(), PAGE_HOME,
		   QStringLiteral("ملخّص المشروع والإجراءات السريعة"));
	addNavItem(nav, QStringLiteral("الواجهة الكلاسيكية"), QString(), PAGE_CLASSIC,
		   QStringLiteral("أزرار FET الأصلية (الملف، البيانات، القيود، الجدول)"));

	addSection(nav, QStringLiteral("البيانات"));
	addNavItem(nav, QStringLiteral("الأساتذة"), QStringLiteral("dataTeachersAction"));
	addNavItem(nav, QStringLiteral("المواد"), QStringLiteral("dataSubjectsAction"));
	addNavItem(nav, QStringLiteral("الأقسام والأفواج"), QStringLiteral("dataYearsAction"));
	addNavItem(nav, QStringLiteral("الفرق الفرعية"), QStringLiteral("dataSubgroupsAction"));
	addNavItem(nav, QStringLiteral("القاعات"), QStringLiteral("dataRoomsAction"));
	addNavItem(nav, QStringLiteral("المباني"), QStringLiteral("dataBuildingsAction"));
	addNavItem(nav, QStringLiteral("الأنشطة"), QStringLiteral("dataActivitiesAction"));

	addSection(nav, QStringLiteral("القيود"));
	addNavItem(nav, QStringLiteral("كل القيود الزمنية"), QStringLiteral("dataAllTimeConstraintsAction"));
	addNavItem(nav, QStringLiteral("كل القيود المكانية"), QStringLiteral("dataAllSpaceConstraintsAction"));

	addSection(nav, QStringLiteral("التوليد والجداول"));
	addNavItem(nav, QStringLiteral("توليد الجدول"), QStringLiteral("timetableGenerateAction"));
	addNavItem(nav, QStringLiteral("جداول الأساتذة"), QStringLiteral("timetableViewTeachersTimeHorizontalAction"));
	addNavItem(nav, QStringLiteral("جداول الأقسام"), QStringLiteral("timetableViewStudentsTimeHorizontalAction"));
	addNavItem(nav, QStringLiteral("جداول القاعات"), QStringLiteral("timetableViewRoomsTimeHorizontalAction"));
	addNavItem(nav, QStringLiteral("التعارضات"), QStringLiteral("timetableShowConflictsAction"));

	addSection(nav, QStringLiteral("النشر"));
	addNavItem(nav, QStringLiteral("نشر الجداول…"), QStringLiteral("filePublishToShareAction"),
		   -1, QStringLiteral("نسخ الجداول إلى مجلد مشترك على شبكة المؤسسة"));

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
	// النقر المفرد يُنفّذ أيضًا (مريح أكثر من النقر المزدوج في شريط تنقّل)
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
		// نستعيد الصفحة فقط (لا الإجراءات، حتى لا تُفتح نوافذ عند بدء التشغيل)
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
		QSettings s(QStringLiteral("fet"), QStringLiteral("fettimetabling"));
		s.setValue(QStringLiteral("OpenTimetable/shell/nav-index"), row);
	});
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

} // namespace UiShell
