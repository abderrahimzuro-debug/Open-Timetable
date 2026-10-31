/*
File ui_shell.cpp — Open-Timetable addition

This file is part of Open-Timetable, a modernised user interface for the
timetabling engine. Licensed under the GNU Affero General Public License v3.

البنية:
  QMainWindow
   └─ QSplitter (أفقي)
       ├─ الشريط الجانبي (قابل للطي: أيقونات مع أسماء، أو أيقونات فقط)
       └─ QStackedWidget → لوحة البداية (+ شاشات الإعدادات إن وُجدت)

قرارات هذه الجولة:
- الأيقونات احترافية مرسومة داخليًا (UiIcons) بدل الإيموجي، مع الاسم بجانبها،
  وعند الطي: الأيقونة وحدها ويظهر الاسم في التلميح.
- زر أعلى الشريط لطيّه وفتحه، وزر لتبديل المظهر الفاتح/الداكن (يُحفظ الاختيار).
- الواجهة الكلاسيكية لم تعد جزءًا من الشاشات. نُخرجها من الوسط دون حذف الكائنات
  حتى تبقى كل إجراءات البرنامج تعمل (لا وظيفة تُفقد) — لا يوجد أي زر لها.
- كل نصوص الواجهة من أيقونة/اسم تُترجم فورًا عند تبديل اللغة.
*/

#include "ui_shell.h"

#include "ui_constraints.h"
#include "ui_dashboard.h"
#include "ui_i18n.h"
#include "ui_icons.h"
#include "ui_theme.h"

#include <QAction>
#include <QApplication>
#include <QColor>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>
#include <QSettings>
#include <QSplitter>
#include <QStackedWidget>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

namespace {

const char* const PROP_INSTALLED = "ot_shell_installed";

enum SidebarRole {
	RoleActionName = Qt::UserRole + 1, // QString: اسم إجراء
	RolePageIndex,                     // int: صفحة في QStackedWidget
	RoleLabel                          // QString: الاسم الكامل (للتلميح عند الطي)
};

const int PAGE_HOME = 0;
const int COMPACT_WIDTH = 66;

QString settingsKey(const char* suffix)
{
	return QStringLiteral("OpenTimetable/shell/") + QString::fromLatin1(suffix);
}

// ---------- عناصر الشريط الجانبي ----------

QListWidgetItem* addSection(QListWidget* nav, const QString& title)
{
	QListWidgetItem* item = new QListWidgetItem(title, nav);
	item->setFlags(Qt::NoItemFlags);
	QFont font = item->font();
	font.setBold(true);
	font.setPointSizeF(qMax(8.0, font.pointSizeF() - 0.5));
	item->setFont(font);
	item->setForeground(QColor(QStringLiteral("#7c8794")));
	item->setTextAlignment((UiI18n::isArabic() ? Qt::AlignRight : Qt::AlignLeft) | Qt::AlignVCenter);
	return item;
}

void addNavItem(QListWidget* nav, const QString& text, const QString& iconName,
		const QString& actionName = QString(), int pageIndex = -1,
		const QString& tooltip = QString())
{
	QListWidgetItem* item = new QListWidgetItem(text, nav);
	item->setTextAlignment((UiI18n::isArabic() ? Qt::AlignRight : Qt::AlignLeft) | Qt::AlignVCenter);
	item->setData(RoleLabel, text);
	if (!actionName.isEmpty())
		item->setData(RoleActionName, actionName);
	if (pageIndex >= 0)
		item->setData(RolePageIndex, pageIndex);
	if (!iconName.isEmpty())
		item->setIcon(UiIcons::icon(iconName, 20));
	item->setToolTip(tooltip.isEmpty() ? text : tooltip);
}

// يُعيد بناء عناصر الشريط الجانبي باللغة/الوضع الحاليين، ويُبقي التحديد.
void buildNavItems(QListWidget* nav, bool compact)
{
	QString selAction;
	int selPage = -1;
	const QListWidgetItem* currentItem = nav->currentItem();
	if (currentItem != nullptr) {
		selAction = currentItem->data(RoleActionName).toString();
		if (currentItem->data(RolePageIndex).isValid())
			selPage = currentItem->data(RolePageIndex).toInt();
	}

	nav->clear();

	// في الوضع المطوي: الأيقونة وحدها، والاسم يظهر كتلميح
	auto label = [compact](const QString& text) { return compact ? QString() : text; };
	auto nav0 = [&](const QString& text, const QString& icon, const QString& action,
			int page, const QString& tip) {
		addNavItem(nav, label(text), icon, action, page, tip.isEmpty() ? text : tip);
	};

	nav0(UiI18n::T("Dashboard", "لوحة البداية"), QStringLiteral("home"), QString(),
	     PAGE_HOME, UiI18n::T("Project summary and quick actions",
				  "ملخّص المشروع والإجراءات السريعة"));

	if (!compact)
		addSection(nav, UiI18n::T("Data", "البيانات"));
	else
		addSection(nav, QString());

	nav0(UiI18n::T("Teachers", "الأساتذة"), QStringLiteral("teachers"),
	     QStringLiteral("dataTeachersAction"), -1, QString());
	nav0(UiI18n::T("Subjects", "المواد"), QStringLiteral("subjects"),
	     QStringLiteral("dataSubjectsAction"), -1, QString());
	nav0(UiI18n::T("Years and groups", "الأقسام والأفواج"), QStringLiteral("years"),
	     QStringLiteral("dataYearsAction"), -1, QString());
	nav0(UiI18n::T("Subgroups", "الفرق الفرعية"), QStringLiteral("subgroups"),
	     QStringLiteral("dataSubgroupsAction"), -1, QString());
	nav0(UiI18n::T("Rooms", "القاعات"), QStringLiteral("rooms"),
	     QStringLiteral("dataRoomsAction"), -1, QString());
	nav0(UiI18n::T("Buildings", "المباني"), QStringLiteral("buildings"),
	     QStringLiteral("dataBuildingsAction"), -1, QString());
	nav0(UiI18n::T("Activities", "الأنشطة"), QStringLiteral("activities"),
	     QStringLiteral("dataActivitiesAction"), -1, QString());

	if (!compact)
		addSection(nav, UiI18n::T("Constraints", "القيود"));
	else
		addSection(nav, QString());

	nav0(UiI18n::T("Constraint templates", "القيود بالقوالب"), QStringLiteral("templates"),
	     QStringLiteral("otConstraintTemplatesAction"), -1,
	     UiI18n::T("Add the everyday constraints through a simple form",
		       "إضافة القيود اليومية عبر نموذج مبسّط"));
	nav0(UiI18n::T("All time constraints", "كل القيود الزمنية"), QStringLiteral("constraints"),
	     QStringLiteral("dataAllTimeConstraintsAction"), -1, QString());
	nav0(UiI18n::T("All space constraints", "كل القيود المكانية"), QStringLiteral("space"),
	     QStringLiteral("dataAllSpaceConstraintsAction"), -1, QString());

	if (!compact)
		addSection(nav, UiI18n::T("Generation and timetables", "التوليد والجداول"));
	else
		addSection(nav, QString());

	nav0(UiI18n::T("Generate timetable", "توليد الجدول"), QStringLiteral("generate"),
	     QStringLiteral("timetableGenerateAction"), -1, QString());
	nav0(UiI18n::T("Teachers' timetables", "جداول الأساتذة"), QStringLiteral("view-teachers"),
	     QStringLiteral("timetableViewTeachersTimeHorizontalAction"), -1, QString());
	nav0(UiI18n::T("Students' timetables", "جداول الأقسام"), QStringLiteral("view-students"),
	     QStringLiteral("timetableViewStudentsTimeHorizontalAction"), -1, QString());
	nav0(UiI18n::T("Rooms' timetables", "جداول القاعات"), QStringLiteral("view-rooms"),
	     QStringLiteral("timetableViewRoomsTimeHorizontalAction"), -1, QString());
	nav0(UiI18n::T("Conflicts", "التعارضات"), QStringLiteral("conflicts"),
	     QStringLiteral("timetableShowConflictsAction"), -1, QString());

	if (!compact)
		addSection(nav, UiI18n::T("Publish", "النشر"));
	else
		addSection(nav, QString());

	nav0(UiI18n::T("Publish timetables", "نشر الجداول"), QStringLiteral("publish"),
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
	s.setValue(settingsKey("nav-index"), row);
}

QString currentTheme()
{
	QSettings s(QStringLiteral("fet"), QStringLiteral("fettimetabling"));
	QString name = s.value(QStringLiteral("OpenTimetable/theme"), QStringLiteral("light")).toString();
	return name == QLatin1String("dark") ? QStringLiteral("dark") : QStringLiteral("light");
}

void setTheme(QMainWindow* window, const QString& name)
{
	QSettings s(QStringLiteral("fet"), QStringLiteral("fettimetabling"));
	s.setValue(QStringLiteral("OpenTimetable/theme"), name);

	UiTheme::applyTheme(*qApp, name);

	// الأيقونات تُرسم بألوان المظهر، لذا نعيد بناءها
	if (window != nullptr)
		UiShell::retranslate(window);
}

// إخفاء كل ما يشير إلى موقع المشروع الأصلي أو اسمه التجاري
void detachExternalBranding(QMainWindow* window)
{
	const QStringList onlineActions = {
		QStringLiteral("helpHomepageAction"), // موقع المشروع الأصلي
		QStringLiteral("helpForumAction"),
		QStringLiteral("helpAddressesAction"),
		QStringLiteral("helpContentsAction"),
		QStringLiteral("checkForUpdatesAction") // فحص تحديثات موقع البرنامج الأصلي
	};
	for (const QString& name : onlineActions)
		if (QAction* action = window->findChild<QAction*>(name))
			action->setVisible(false);

	// أي نص متبقٍّ فيه اسم البرنامج الأصلي يُستبدل باسم برنامجنا
	const QList<QAction*> actions = window->findChildren<QAction*>();
	for (QAction* action : actions) {
		QString text = action->text();
		if (text.contains(QLatin1String("Open-Timetable"))) {
			text.replace(QLatin1String("Open-Timetable"), QStringLiteral("Open-Timetable"));
			action->setText(text);
		}
	}
	const QList<QMenu*> menus = window->findChildren<QMenu*>();
	for (QMenu* menu : menus) {
		QString title = menu->title();
		if (title.contains(QLatin1String("Open-Timetable"))) {
			title.replace(QLatin1String("Open-Timetable"), QStringLiteral("Open-Timetable"));
			menu->setTitle(title);
		}
	}
}

} // namespace

namespace UiShell {

void install(QMainWindow* window)
{
	if (window == nullptr)
		return;
	if (window->property(PROP_INSTALLED).toBool())
		return;

	QWidget* classicWidget = window->centralWidget();
	if (classicWidget == nullptr)
		return;

	QSettings settings(QStringLiteral("fet"), QStringLiteral("fettimetabling"));

	// ---------- إجراء «القيود بالقوالب» (المرحلة 4) ----------
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
	// الواجهة الكلاسيكية تخرج من الشاشات: نفصلها عن النافذة قبل استبدال الوسط
	// حتى تبقى كائنات الإجراءات حيّة (لا وظيفة تُفقد، ولا زر يظهر للمستخدم).
	classicWidget->setParent(nullptr);
	classicWidget->hide();

	UiDashboard* dashboard = new UiDashboard(window);
	QStackedWidget* stack = new QStackedWidget(window);
	stack->addWidget(dashboard); // PAGE_HOME

	// ---------- الشريط الجانبي ----------
	QWidget* sidebar = new QWidget(window);
	sidebar->setObjectName(QStringLiteral("otSidebar"));
	sidebar->setMinimumWidth(COMPACT_WIDTH);

	QVBoxLayout* sideLayout = new QVBoxLayout(sidebar);
	sideLayout->setContentsMargins(0, 0, 0, 0);
	sideLayout->setSpacing(0);

	// ترويسة: الشعار + الاسم + أزرار الطي والمظهر
	QWidget* header = new QWidget(sidebar);
	header->setObjectName(QStringLiteral("otSidebarHeader"));
	QHBoxLayout* headerLayout = new QHBoxLayout(header);
	headerLayout->setContentsMargins(12, 12, 8, 6);
	headerLayout->setSpacing(8);

	QLabel* logoLabel = new QLabel(header);
	logoLabel->setObjectName(QStringLiteral("otBrandLogo"));
	logoLabel->setPixmap(UiIcons::logo(26).pixmap(26, 26));
	logoLabel->setFixedSize(26, 26);

	QLabel* brand = new QLabel(QStringLiteral("Open-Timetable"), header);
	brand->setObjectName(QStringLiteral("otBrand"));
	QFont brandFont = brand->font();
	brandFont.setBold(true);
	brandFont.setPointSizeF(brandFont.pointSizeF() + 0.5);
	brand->setFont(brandFont);

	QToolButton* themeButton = new QToolButton(header);
	themeButton->setObjectName(QStringLiteral("otThemeButton"));
	themeButton->setAutoRaise(true);
	themeButton->setCursor(Qt::PointingHandCursor);
	themeButton->setIconSize(QSize(18, 18));

	QToolButton* collapseButton = new QToolButton(header);
	collapseButton->setObjectName(QStringLiteral("otCollapseButton"));
	collapseButton->setAutoRaise(true);
	collapseButton->setCursor(Qt::PointingHandCursor);
	collapseButton->setIconSize(QSize(18, 18));

	headerLayout->addWidget(logoLabel);
	headerLayout->addWidget(brand, 1);
	headerLayout->addWidget(themeButton);
	headerLayout->addWidget(collapseButton);

	QLabel* brandSub = new QLabel(sidebar);
	brandSub->setObjectName(QStringLiteral("otBrandSub"));
	brandSub->setContentsMargins(14, 0, 12, 10);

	QListWidget* nav = new QListWidget(sidebar);
	nav->setObjectName(QStringLiteral("otNav"));
	nav->setFrameShape(QFrame::NoFrame);
	nav->setWordWrap(false);
	nav->setIconSize(QSize(20, 20));
	nav->setSpacing(1);
	nav->setSelectionMode(QAbstractItemView::SingleSelection);
	nav->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	nav->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

	sideLayout->addWidget(header);
	sideLayout->addWidget(brandSub);
	sideLayout->addWidget(nav, 1);

	bool compact = settings.value(settingsKey("collapsed"), false).toBool();
	buildNavItems(nav, compact);

	// ---------- التجميع ----------
	QSplitter* splitter = new QSplitter(Qt::Horizontal, window);
	splitter->setObjectName(QStringLiteral("otShellSplitter"));
	splitter->setChildrenCollapsible(false);
	splitter->addWidget(sidebar);
	splitter->addWidget(stack);
	splitter->setStretchFactor(0, 0);
	splitter->setStretchFactor(1, 1);

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

	QObject::connect(stack, &QStackedWidget::currentChanged, window, [dashboard](int index) {
		if (index == PAGE_HOME && dashboard != nullptr)
			dashboard->refresh();
	});
	QObject::connect(qApp, &QApplication::applicationStateChanged, window,
			 [dashboard](Qt::ApplicationState state) {
				 if (state == Qt::ApplicationActive && dashboard != nullptr)
					 dashboard->refresh();
			 });

	// زر المظهر: فاتح ⟷ داكن (ويُحفظ)
	QObject::connect(themeButton, &QToolButton::clicked, window, [window]() {
		const QString next = currentTheme() == QLatin1String("dark")
					     ? QStringLiteral("light")
					     : QStringLiteral("dark");
		setTheme(window, next);
	});

	// زر الطي: أيقونات مع أسماء ⟷ أيقونات فقط
	QObject::connect(collapseButton, &QToolButton::clicked, window, [window, nav, splitter]() {
		QSettings s(QStringLiteral("fet"), QStringLiteral("fettimetabling"));
		const bool nowCompact = !s.value(settingsKey("collapsed"), false).toBool();
		s.setValue(settingsKey("collapsed"), nowCompact);

		buildNavItems(nav, nowCompact);
		if (QToolButton* button = window->findChild<QToolButton*>(QStringLiteral("otCollapseButton")))
			button->setIcon(UiIcons::icon(QStringLiteral("sidebar"), 18));

		QList<int> sizes = splitter->sizes();
		if (sizes.size() == 2) {
			sizes[0] = nowCompact ? COMPACT_WIDTH : qMax(196, sizes[0]);
			sizes[1] = qMax(400, splitter->width() - sizes[0]);
			splitter->setSizes(sizes);
		}
		UiShell::retranslate(window);
	});

	// أيقونة الأزرار حسب الحالة
	themeButton->setIcon(UiIcons::icon(currentTheme() == QLatin1String("dark")
						  ? QStringLiteral("sun")
						  : QStringLiteral("moon"), 18));
	themeButton->setToolTip(UiI18n::T("Switch light / dark mode", "تبديل المظهر الفاتح/الداكن"));
	collapseButton->setIcon(UiIcons::icon(QStringLiteral("sidebar"), 18));
	collapseButton->setToolTip(UiI18n::T("Collapse / expand the sidebar", "طي/فتح الشريط الجانبي"));

	// ---------- الحفظ والاستعادة ----------
	const int savedIndex = settings.value(settingsKey("nav-index"), PAGE_HOME).toInt();
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

	const int savedWidth = settings.value(settingsKey("sidebar-width"), 220).toInt();
	QList<int> sizes;
	sizes << (compact ? COMPACT_WIDTH : qBound(196, savedWidth, 320)) << 1000;
	splitter->setSizes(sizes);

	QObject::connect(splitter, &QSplitter::splitterMoved, window, [splitter](int, int) {
		QList<int> current = splitter->sizes();
		if (current.isEmpty())
			return;
		// QSettings غير قابل للنسخ، لذا ننشئه هنا (عملية نادرة ورخيصة)
		QSettings s(QStringLiteral("fet"), QStringLiteral("fettimetabling"));
		s.setValue(settingsKey("sidebar-width"), current.first());
	});

	QObject::connect(nav, &QListWidget::currentRowChanged, window, [](int row) {
		saveNavIndex(row);
	});

	// إزالة الإشارات إلى موقع/اسم المشروع الأصلي من القوائم
	detachExternalBranding(window);

	// أيقونة النافذة والشعار الخاص بنا
	qApp->setWindowIcon(UiIcons::logo(64));
	window->setWindowIcon(UiIcons::logo(64));
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

	// نصوص الإجراءات التي أضفناها
	if (QAction* templatesAction = window->findChild<QAction*>(QStringLiteral("otConstraintTemplatesAction"))) {
		templatesAction->setText(UiI18n::T("Constraint templates…", "القيود بالقوالب…"));
		templatesAction->setToolTip(UiI18n::T("Add the everyday constraints through a simple form",
						      "إضافة القيود اليومية عبر نموذج مبسّط"));
	}

	// ترويسة الشريط الجانبي
	const QList<QLabel*> labels = splitter->findChildren<QLabel*>(QStringLiteral("otBrandSub"));
	for (QLabel* label : labels)
		label->setText(UiI18n::T("School timetabling", "جدولة دراسية"));

	if (QToolButton* themeButton = window->findChild<QToolButton*>(QStringLiteral("otThemeButton"))) {
		themeButton->setIcon(UiIcons::icon(currentTheme() == QLatin1String("dark")
							   ? QStringLiteral("sun")
							   : QStringLiteral("moon"), 18));
		themeButton->setToolTip(UiI18n::T("Switch light / dark mode", "تبديل المظهر الفاتح/الداكن"));
	}
	if (QToolButton* collapseButton = window->findChild<QToolButton*>(QStringLiteral("otCollapseButton"))) {
		collapseButton->setIcon(UiIcons::icon(QStringLiteral("sidebar"), 18));
		collapseButton->setToolTip(UiI18n::T("Collapse / expand the sidebar", "طي/فتح الشريط الجانبي"));
	}

	// عناصر التنقّل (تعيد أيضًا رسم الأيقونات بألوان المظهر الحالي)
	QSettings settings(QStringLiteral("fet"), QStringLiteral("fettimetabling"));
	const bool compact = settings.value(settingsKey("collapsed"), false).toBool();
	if (QListWidget* nav = splitter->findChild<QListWidget*>(QStringLiteral("otNav")))
		buildNavItems(nav, compact);

	// لوحة البداية
	UiDashboard* dashboard = splitter->findChild<UiDashboard*>();
	if (dashboard != nullptr)
		dashboard->retranslate();

	// بعد تغيير اللغة تُرجع ملفات الترجمة اسم البرنامج الأصلي في القوائم،
	// لذا نُعيد إخفاء روابط الموقع الأصلي وإعادة تسمية ما تبقّى.
	detachExternalBranding(window);
}

} // namespace UiShell
