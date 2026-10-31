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

#include "ui_command_palette.h"
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
#include <QGridLayout>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>
#include <QScreen>
#include <QGuiApplication>
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
	item->setData(RoleLabel, title);
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

// يُعيد بناء عناصر الشريط الجانبي باللغة الحالية، ويُبقي التحديد.
// (يُستدعى عند تغيير اللغة/المظهر فقط، لا عند الطي.)
void buildNavItems(QListWidget* nav)
{
	QString selAction;
	int selPage = -1;
	const QListWidgetItem* currentItem = nav->currentItem();
	if (currentItem != nullptr) {
		selAction = currentItem->data(RoleActionName).toString();
		if (currentItem->data(RolePageIndex).isValid())
			selPage = currentItem->data(RolePageIndex).toInt();
	}

	// إعادة البناء بهدوء: لا إشارات ولا إعادة رسم أثناء الإضافة (كان سبب التقطيع
	// عند الطي/التبديل)، ثم نُرسم مرة واحدة في النهاية.
	nav->setUpdatesEnabled(false);
	{
		const QSignalBlocker blocker(nav);
		nav->clear();

		auto nav0 = [&](const QString& text, const QString& icon, const QString& action,
				int page, const QString& tip) {
			addNavItem(nav, text, icon, action, page, tip.isEmpty() ? text : tip);
		};

		nav0(UiI18n::T("Dashboard", "لوحة البداية"), QStringLiteral("home"), QString(),
		     PAGE_HOME, UiI18n::T("Project summary and quick actions",
					  "ملخّص المشروع والإجراءات السريعة"));

		addSection(nav, UiI18n::T("Data", "البيانات"));

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

		addSection(nav, UiI18n::T("Constraints", "القيود"));

		nav0(UiI18n::T("Constraint templates", "القيود بالقوالب"), QStringLiteral("templates"),
		     QStringLiteral("otConstraintTemplatesAction"), -1,
		     UiI18n::T("Add the everyday constraints through a simple form",
			       "إضافة القيود اليومية عبر نموذج مبسّط"));
		nav0(UiI18n::T("All time constraints", "كل القيود الزمنية"), QStringLiteral("constraints"),
		     QStringLiteral("dataAllTimeConstraintsAction"), -1, QString());
		nav0(UiI18n::T("All space constraints", "كل القيود المكانية"), QStringLiteral("space"),
		     QStringLiteral("dataAllSpaceConstraintsAction"), -1, QString());

		addSection(nav, UiI18n::T("Generation and timetables", "التوليد والجداول"));

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

		addSection(nav, UiI18n::T("Publish", "النشر"));

		nav0(UiI18n::T("Publish timetables", "نشر الجداول"), QStringLiteral("publish"),
		     QStringLiteral("filePublishToShareAction"), -1,
		     UiI18n::T("Copy the timetables to a shared folder on the school network",
			       "نسخ الجداول إلى مجلد مشترك على شبكة المؤسسة"));

		addSection(nav, UiI18n::T("Tools", "أدوات"));

		nav0(UiI18n::T("All commands…", "كل الأدوات…"), QStringLiteral("commands"),
		     QStringLiteral("otCommandPaletteAction"), -1,
		     UiI18n::T("Search every command of the program (Ctrl+Shift+P)",
			       "ابحث في كل أوامر البرنامج (Ctrl+Shift+P)"));

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
	nav->setUpdatesEnabled(true);
}

// طيّ/فتح: يُخفي النصوص فقط (لا إعادة بناء ولا رسم أيقونات) ⇒ فوري بلا تقطيع.
void applyCompact(QListWidget* nav, bool compact)
{
	if (nav == nullptr)
		return;

	nav->setUpdatesEnabled(false);
	for (int i = 0; i < nav->count(); ++i) {
		QListWidgetItem* item = nav->item(i);
		const QString label = item->data(RoleLabel).toString();
		item->setText(compact ? QString() : label);
		// في الوضع المطوي يظهر الاسم كتلميح فقط
		item->setToolTip(label);
	}
	nav->setUpdatesEnabled(true);
	nav->update();
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

QString oldBrandName()
{
	// نُبنيه حرفًا حرفًا حتى لا يستبدله سكربت إعادة التسمية عند إعادة تشغيله:
	// نحن نبحث عن الاسم القديم لنستبدله، لا لنُعيد تسمية أنفسنا.
	return QString(QChar('F')) + QChar('E') + QChar('T');
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
		if (text.contains(oldBrandName())) {
			text.replace(oldBrandName(), QStringLiteral("Open-Timetable"));
			action->setText(text);
		}
	}
	const QList<QMenu*> menus = window->findChildren<QMenu*>();
	for (QMenu* menu : menus) {
		QString title = menu->title();
		if (title.contains(oldBrandName())) {
			title.replace(oldBrandName(), QStringLiteral("Open-Timetable"));
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

	// ---------- إجراء «كل الأدوات» (وصول سريع إلى كل أوامر البرنامج) ----------
	if (window->findChild<QAction*>(QStringLiteral("otCommandPaletteAction")) == nullptr) {
		QAction* paletteAction = new QAction(window);
		paletteAction->setObjectName(QStringLiteral("otCommandPaletteAction"));
		paletteAction->setText(UiI18n::T("All commands…", "كل الأدوات…"));
		paletteAction->setToolTip(UiI18n::T("Search every command of the program (Ctrl+Shift+P)",
						    "ابحث في كل أوامر البرنامج (Ctrl+Shift+P)"));
		paletteAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+P")));
		QObject::connect(paletteAction, &QAction::triggered, window,
				 [window]() { UiCommandPalette::run(window); });
		window->addAction(paletteAction);
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

	// ترويسة: الشعار + الاسم + أزرار الطي والمظهر.
	// تتغيّر بنيتها عند الطي (عموديان بدل صفّ) حتى لا تُقتطع أي أيقونة في 66 بكسل.
	QWidget* header = new QWidget(sidebar);
	header->setObjectName(QStringLiteral("otSidebarHeader"));
	QGridLayout* headerLayout = new QGridLayout(header);
	headerLayout->setContentsMargins(10, 10, 10, 6);
	headerLayout->setHorizontalSpacing(6);
	headerLayout->setVerticalSpacing(4);

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
	themeButton->setFixedSize(26, 26);

	QToolButton* collapseButton = new QToolButton(header);
	collapseButton->setObjectName(QStringLiteral("otCollapseButton"));
	collapseButton->setAutoRaise(true);
	collapseButton->setCursor(Qt::PointingHandCursor);
	collapseButton->setIconSize(QSize(18, 18));
	collapseButton->setFixedSize(26, 26);

	QLabel* brandSub = new QLabel(sidebar);
	brandSub->setObjectName(QStringLiteral("otBrandSub"));
	brandSub->setContentsMargins(14, 0, 12, 8);

	QListWidget* nav = new QListWidget(sidebar);
	nav->setObjectName(QStringLiteral("otNav"));
	nav->setFrameShape(QFrame::NoFrame);
	nav->setWordWrap(false);
	nav->setIconSize(QSize(20, 20));
	nav->setSpacing(0);
	nav->setUniformItemSizes(false);
	nav->setSelectionMode(QAbstractItemView::SingleSelection);
	nav->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	nav->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

	sideLayout->addWidget(header);
	sideLayout->addWidget(brandSub);
	sideLayout->addWidget(nav, 1);

	buildNavItems(nav);

	bool compact = settings.value(settingsKey("collapsed"), false).toBool();

	// توزيع الترويسة حسب الوضع، وإظهار/إخفاء النصوص
	auto refreshLayout = [headerLayout, logoLabel, brand, themeButton, collapseButton, brandSub,
			      nav](bool compactMode) {
		headerLayout->removeWidget(logoLabel);
		headerLayout->removeWidget(brand);
		headerLayout->removeWidget(themeButton);
		headerLayout->removeWidget(collapseButton);

		applyCompact(nav, compactMode);
		brand->setVisible(!compactMode);
		logoLabel->setVisible(!compactMode);
		brandSub->setVisible(!compactMode);

		if (compactMode) {
			// أيقونتان فوق بعضهما في عمود ضيّق
			headerLayout->addWidget(themeButton, 0, 0, Qt::AlignCenter);
			headerLayout->addWidget(collapseButton, 1, 0, Qt::AlignCenter);
			headerLayout->setColumnStretch(0, 1);
			headerLayout->setColumnStretch(1, 0);
		}
		else {
			headerLayout->addWidget(logoLabel, 0, 0, Qt::AlignVCenter);
			headerLayout->addWidget(brand, 0, 1, Qt::AlignVCenter);
			headerLayout->addWidget(themeButton, 0, 2, Qt::AlignVCenter);
			headerLayout->addWidget(collapseButton, 0, 3, Qt::AlignVCenter);
			headerLayout->setColumnStretch(0, 0);
			headerLayout->setColumnStretch(1, 1);
		}
	};

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

	// زر الطي: أيقونات مع أسماء ⟷ أيقونات فقط (تغيير فوري، بلا إعادة بناء)
	QObject::connect(collapseButton, &QToolButton::clicked, window,
			 [window, splitter, refreshLayout, nav, collapseButton]() {
		QSettings s(QStringLiteral("fet"), QStringLiteral("fettimetabling"));
		const bool nowCompact = !s.value(settingsKey("collapsed"), false).toBool();
		s.setValue(settingsKey("collapsed"), nowCompact);
		const int previousWidth = s.value(settingsKey("sidebar-width"), 220).toInt();

		// تجميد التحديثات مؤقتًا: تعديل واحد ثم رسم واحد (لا تقطيع)
		splitter->setUpdatesEnabled(false);
		refreshLayout(nowCompact);

		collapseButton->setIcon(UiIcons::icon(nowCompact ? QStringLiteral("sidebar-open")
								 : QStringLiteral("sidebar"),
						      18));
		collapseButton->setToolTip(nowCompact
			? UiI18n::T("Expand the sidebar", "فتح الشريط الجانبي")
			: UiI18n::T("Collapse the sidebar", "طي الشريط الجانبي"));

		QList<int> sizes = splitter->sizes();
		if (sizes.size() == 2) {
			const int sidebarWidth = nowCompact ? COMPACT_WIDTH
							    : qBound(196, previousWidth, 320);
			sizes[0] = sidebarWidth;
			// لا نلمس عرض المحتوى: نحسبه من العرض الكلي الحالي الفعلي
			sizes[1] = qMax(320, splitter->width() - sidebarWidth);
			splitter->setSizes(sizes);
		}
		splitter->setUpdatesEnabled(true);
		splitter->update();
		Q_UNUSED(nav);
	});

	// أيقونة الأزرار حسب الحالة
	themeButton->setIcon(UiIcons::icon(currentTheme() == QLatin1String("dark")
						  ? QStringLiteral("sun")
						  : QStringLiteral("moon"), 18));
	themeButton->setToolTip(UiI18n::T("Switch light / dark mode", "تبديل المظهر الفاتح/الداكن"));
	collapseButton->setIcon(UiIcons::icon(compact ? QStringLiteral("sidebar-open")
						    : QStringLiteral("sidebar"), 18));
	collapseButton->setToolTip(compact ? UiI18n::T("Expand the sidebar", "فتح الشريط الجانبي")
					   : UiI18n::T("Collapse the sidebar", "طي الشريط الجانبي"));

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

	refreshLayout(compact);

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

void fitToScreen(QMainWindow* window)
{
	if (window == nullptr)
		return;

	QScreen* screen = window->screen();
	if (screen == nullptr)
		screen = QGuiApplication::primaryScreen();
	if (screen == nullptr)
		return;

	const QRect available = screen->availableGeometry();
	if (!available.isValid())
		return;

	// هامش صغير حتى لا يلتصق إطار النافذة بحافّة الشاشة (شريط المهام)
	const int maxWidth = qMax(800, available.width() - 16);
	const int maxHeight = qMax(560, available.height() - 16);

	// الحدّ الأدنى للنافذة لا يجوز أن يتجاوز الشاشة نفسها
	const QSize minSize(qMin(window->minimumSize().width() > 0 ? window->minimumSize().width() : 640, maxWidth),
			    qMin(window->minimumSize().height() > 0 ? window->minimumSize().height() : 480, maxHeight));
	window->setMinimumSize(minSize);

	QSize size = window->size();
	const int width = qBound(minSize.width(), size.width(), maxWidth);
	const int height = qBound(minSize.height(), size.height(), maxHeight);
	if (width != size.width() || height != size.height())
		window->resize(width, height);

	// إعادة التوسيط داخل المساحة المتاحة (لا خارجها)
	QRect frame = window->frameGeometry();
	if (!available.contains(frame)) {
		const int x = available.x() + qMax(0, (available.width() - frame.width()) / 2);
		const int y = available.y() + qMax(0, (available.height() - frame.height()) / 2);
		window->move(x, y);
	}
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
	if (QAction* paletteAction = window->findChild<QAction*>(QStringLiteral("otCommandPaletteAction"))) {
		paletteAction->setText(UiI18n::T("All commands…", "كل الأدوات…"));
		paletteAction->setToolTip(UiI18n::T("Search every command of the program (Ctrl+Shift+P)",
						    "ابحث في كل أوامر البرنامج (Ctrl+Shift+P)"));
	}
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
	QSettings settings(QStringLiteral("fet"), QStringLiteral("fettimetabling"));
	const bool compact = settings.value(settingsKey("collapsed"), false).toBool();

	if (QToolButton* collapseButton = window->findChild<QToolButton*>(QStringLiteral("otCollapseButton"))) {
		collapseButton->setIcon(UiIcons::icon(compact ? QStringLiteral("sidebar-open")
							     : QStringLiteral("sidebar"),
						      18));
		collapseButton->setToolTip(compact
			? UiI18n::T("Expand the sidebar", "فتح الشريط الجانبي")
			: UiI18n::T("Collapse the sidebar", "طي الشريط الجانبي"));
	}

	// عناصر التنقّل (تعيد أيضًا رسم الأيقونات بألوان المظهر الحالي)
	if (QListWidget* nav = splitter->findChild<QListWidget*>(QStringLiteral("otNav"))) {
		buildNavItems(nav);
		applyCompact(nav, compact);
	}

	// لوحة البداية
	UiDashboard* dashboard = splitter->findChild<UiDashboard*>();
	if (dashboard != nullptr)
		dashboard->retranslate();

	// بعد تغيير اللغة تُرجع ملفات الترجمة اسم البرنامج الأصلي في القوائم،
	// لذا نُعيد إخفاء روابط الموقع الأصلي وإعادة تسمية ما تبقّى.
	detachExternalBranding(window);
}

} // namespace UiShell
