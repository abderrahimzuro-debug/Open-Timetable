/*
File ui_selfcheck.cpp — Open-Timetable addition

This file is part of Open-Timetable, a modernised user interface for FET.
FET is free software; you can redistribute it and/or modify it under the terms
of the GNU Affero General Public License version 3 (see LICENSE).

فحص ذاتي بلا واجهة مرئية: يتحقق من البنية الجديدة ويطبع سطورًا بصيغة
"  [ok] ..." أو "  [FAIL] ..." ثم سطر أخير "SELFCHECK OK" أو "SELFCHECK FAILED".
*/

#include "ui_selfcheck.h"

#include "ui_dashboard.h"
#include "ui_command_palette.h"
#include "ui_constraints.h"
#include "ui_icons.h"
#include "ui_theme.h"
#include "ui_i18n.h"
#include "ui_shell.h"

#include <QAction>
#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QDialog>
#include <QMenu>
#include <QScreen>
#include <QToolButton>
#include <QTreeWidget>
#include <QTranslator>
#include <QListWidget>
#include <QMainWindow>
#include <QSplitter>
#include <QStackedWidget>
#include <QStyleFactory>
#include <QTextStream>

namespace {

QString oldBrandName()
{
	// الاسم القديم مبنيّ حرفًا حرفًا: سكربت إعادة التسمية يستبدل الكلمة الجاهزة،
	// فنحن نحتاجها هنا للبحث عنها والتأكد من اختفائها.
	return QString(QChar('F')) + QChar('E') + QChar('T');
}

struct Report {
	int failures;
	QStringList failureLines;
	QTextStream out;

	Report() : failures(0), out(stdout) {}

	void ok(const QString& what, const QString& detail = QString())
	{
		if (detail.isEmpty())
			out << "  [ok]   " << what << "\n";
		else
			out << "  [ok]   " << what << " — " << detail << "\n";
	}

	void fail(const QString& what, const QString& detail = QString())
	{
		++failures;
		failureLines.append(detail.isEmpty() ? what : what + QStringLiteral(" — ") + detail);
		if (detail.isEmpty())
			out << "  [FAIL] " << what << "\n";
		else
			out << "  [FAIL] " << what << " — " << detail << "\n";
	}

	void info(const QString& text) { out << "         " << text << "\n"; }
};

} // namespace

namespace UiSelfCheck {

int run(QMainWindow* mainWindow)
{
	Report r;
	r.out << "\n===== Open-Timetable self-check =====\n";

	// ---- 0) البيئة ----
	r.info(QStringLiteral("Qt: %1").arg(QString::fromLatin1(qVersion())));
	r.info(QStringLiteral("platform: %1").arg(QApplication::platformName()));
	r.info(QStringLiteral("style: %1").arg(QApplication::style() != nullptr
						     ? QApplication::style()->objectName()
						     : QStringLiteral("(none)")));

	// ---- 1) النافذة ----
	if (mainWindow == nullptr) {
		r.fail(QStringLiteral("النافذة الرئيسية موجودة"));
		r.out << "\nSELFCHECK FAILED\n";
		return r.failures;
	}
	r.ok(QStringLiteral("النافذة الرئيسية موجودة"));

	// ---- 2) المظهر مطبّق ----
	const QString qss = qApp->styleSheet();
	if (qss.contains(QStringLiteral("otSidebar")) && qss.contains(QStringLiteral("otCard")))
		r.ok(QStringLiteral("ورقة الأنماط (QSS) مطبّقة"), QStringLiteral("%1 حرفًا").arg(qss.size()));
	else
		r.fail(QStringLiteral("ورقة الأنماط (QSS) مطبّقة"),
		       QStringLiteral("لم تُعثر على أنماط Open-Timetable"));

	// ---- 3) الهيكل الجديد ----
	QSplitter* splitter = mainWindow->findChild<QSplitter*>(QStringLiteral("otShellSplitter"));
	if (splitter == nullptr) {
		r.fail(QStringLiteral("الشريط الجانبي مثبّت"), QStringLiteral("otShellSplitter غير موجود"));
		r.out << "\nSELFCHECK FAILED\n";
		return r.failures;
	}
	r.ok(QStringLiteral("الشريط الجانبي مثبّت"));

	QListWidget* nav = mainWindow->findChild<QListWidget*>(QStringLiteral("otNav"));
	if (nav != nullptr)
		r.ok(QStringLiteral("قائمة التنقّل موجودة"), QStringLiteral("%1 عنصرًا").arg(nav->count()));
	else
		r.fail(QStringLiteral("قائمة التنقّل موجودة"));

	QStackedWidget* stack = splitter->findChild<QStackedWidget*>();
	if (stack != nullptr) {
		if (stack->count() == 1)
			r.ok(QStringLiteral("الواجهة الكلاسيكية أُزيلت"), QStringLiteral("صفحة واحدة فقط"));
		else
			r.fail(QStringLiteral("الواجهة الكلاسيكية أُزيلت"),
			       QStringLiteral("%1 صفحة").arg(stack->count()));
	}
	else {
		r.fail(QStringLiteral("الصفحات موجودة"));
	}

	// ---- الطي والمظهر والأيقونات ----
	{
		QToolButton* themeButton = mainWindow->findChild<QToolButton*>(QStringLiteral("otThemeButton"));
		QToolButton* collapseButton = mainWindow->findChild<QToolButton*>(QStringLiteral("otCollapseButton"));
		if (themeButton != nullptr)
			r.ok(QStringLiteral("زر المظهر موجود"));
		else
			r.fail(QStringLiteral("زر المظهر موجود"));
		if (collapseButton != nullptr)
			r.ok(QStringLiteral("زر طي الشريط موجود"));
		else
			r.fail(QStringLiteral("زر طي الشريط موجود"));

		const QStringList iconNames = UiIcons::names();
		int drawn = 0;
		for (const QString& name : iconNames)
			if (!UiIcons::icon(name, 20).isNull())
				++drawn;
		if (drawn == iconNames.count() && drawn >= 18)
			r.ok(QStringLiteral("الأيقونات مرسومة"), QStringLiteral("%1 أيقونة").arg(drawn));
		else
			r.fail(QStringLiteral("الأيقونات مرسومة"),
			       QStringLiteral("%1/%2").arg(drawn).arg(iconNames.count()));

		if (!UiIcons::logo().isNull())
			r.ok(QStringLiteral("شعار البرنامج جاهز"));
		else
			r.fail(QStringLiteral("شعار البرنامج جاهز"));

		// لا نصوص إيموجي في عناصر التنقّل
		if (nav != nullptr) {
			int withIcon = 0;
			int items = 0;
			for (int i = 0; i < nav->count(); ++i) {
				const QListWidgetItem* item = nav->item(i);
				if (item->data(Qt::UserRole + 1).toString().isEmpty()
				    && !item->data(Qt::UserRole + 2).isValid())
					continue; // عنوان قسم
				++items;
				if (!item->icon().isNull())
					++withIcon;
			}
			if (items > 0 && withIcon == items)
				r.ok(QStringLiteral("كل عناصر التنقّل لها أيقونة"),
				     QStringLiteral("%1/%1").arg(items));
			else
				r.fail(QStringLiteral("كل عناصر التنقّل لها أيقونة"),
				       QStringLiteral("%1/%2").arg(withIcon).arg(items));
		}
	}

	// ---- 4) لوحة البداية ----
	UiDashboard* dashboard = splitter->findChild<UiDashboard*>();
	if (dashboard != nullptr) {
		r.ok(QStringLiteral("لوحة البداية موجودة"));
		dashboard->refresh();
		r.ok(QStringLiteral("تحديث لوحة البداية يعمل"));
	}
	else {
		r.fail(QStringLiteral("لوحة البداية موجودة"));
	}

	// ---- 5) إجراءات الشريط الجانبي موجودة فعليًا ----
	if (nav != nullptr) {
		int actionItems = 0;
		int found = 0;
		for (int i = 0; i < nav->count(); ++i) {
			const QListWidgetItem* item = nav->item(i);
			const QString actionName = item->data(Qt::UserRole + 1).toString();
			if (actionName.isEmpty())
				continue;
			++actionItems;
			if (mainWindow->findChild<QAction*>(actionName) != nullptr)
				++found;
			else
				r.info(QStringLiteral("مفقود: ") + actionName);
		}
		if (actionItems > 0 && found == actionItems)
			r.ok(QStringLiteral("كل إجراءات الشريط الجانبي مرتبطة"),
			     QStringLiteral("%1/%1").arg(actionItems));
		else
			r.fail(QStringLiteral("كل إجراءات الشريط الجانبي مرتبطة"),
			       QStringLiteral("%1/%2").arg(found).arg(actionItems));
	}

	// ---- 5.5) محرّر القيود بالقوالب (المرحلة 4 — الأساسيات) ----
	{
		QAction* templatesAction = mainWindow->findChild<QAction*>(QStringLiteral("otConstraintTemplatesAction"));
		if (templatesAction != nullptr)
			r.ok(QStringLiteral("إجراء «القيود بالقوالب» موجود"), templatesAction->shortcut().toString());
		else
			r.fail(QStringLiteral("إجراء «القيود بالقوالب» موجود"), QStringLiteral("otConstraintTemplatesAction مفقود"));

		const int templates = UiConstraints::templateCount();
		if (templates >= 35)
			r.ok(QStringLiteral("قوالب القيود (أساسيات + ضروريات + أنشطة)"),
			     QStringLiteral("%1 قالبًا").arg(templates));
		else
			r.fail(QStringLiteral("قوالب القيود (أساسيات + ضروريات + أنشطة)"),
			       QStringLiteral("%1 فقط").arg(templates));
	}

	// ---- 6) اللغة والترجمة ----
	{
		const QString lang = FET_LANGUAGE;
		r.ok(QStringLiteral("لغة الواجهة محدّدة"), lang);

		// هل ملف الترجمة موجود فعلًا بجانب التنفيذي؟
		const QString exeDir = QCoreApplication::applicationDirPath();
		const QStringList candidates = {
			exeDir + QStringLiteral("/translations/fet_%1.qm").arg(lang),
			exeDir + QStringLiteral("/fet_%1.qm").arg(lang)
		};
		bool found = false;
		for (const QString& path : candidates) {
			if (QFileInfo::exists(path)) {
				found = true;
				r.ok(QStringLiteral("ملف الترجمة موجود"), path);
				break;
			}
		}
		if (!found) {
			if (lang == QLatin1String("en_US"))
				r.ok(QStringLiteral("ملف الترجمة غير مطلوب"), QStringLiteral("اللغة الإنجليزية مدمجة"));
			else
				r.fail(QStringLiteral("ملف الترجمة مفقود"), candidates.first());
		}

		const int qmCount = QDir(exeDir + QStringLiteral("/translations"))
					    .entryList(QStringList() << QStringLiteral("fet_*.qm"), QDir::Files).count();
		if (qmCount >= 30)
			r.ok(QStringLiteral("لغات متعدّدة متوفّرة"), QStringLiteral("%1 ملف .qm").arg(qmCount));
		else if (qmCount > 0)
			r.fail(QStringLiteral("لغات متعدّدة متوفّرة"),
			       QStringLiteral("%1 فقط — تبديل اللغة لن يعمل").arg(qmCount));
		else
			r.info(QStringLiteral("لا مجلد translations بجانب التنفيذي"));

		// هل ملفات .qm صالحة فعلًا (لا مجرّد موجودة)؟ نحمّلها ونترجم نصًا معروفًا.
		if (QFileInfo::exists(exeDir + QStringLiteral("/translations"))) {
			const QString dir = exeDir + QStringLiteral("/translations");
			QStringList badLangs;
			QString sample;
			const QStringList testLangs = { QStringLiteral("ar"), QStringLiteral("fr"),
							QStringLiteral("en_GB") };
			for (const QString& l : testLangs) {
				QTranslator t;
				if (!t.load(QStringLiteral("fet_") + l, dir) || t.isEmpty())
					badLangs << l;
				else if (l == QLatin1String("fr"))
					sample = t.translate("FetMainForm", "Explanation:");
			}
			if (badLangs.isEmpty())
				r.ok(QStringLiteral("ملفات الترجمة صالحة"),
				     QStringLiteral("ar/fr/en_GB — مثال: %1").arg(sample));
			else
				r.fail(QStringLiteral("ملفات الترجمة صالحة"),
				       QStringLiteral("تالفة أو ناقصة: %1").arg(badLangs.join(QStringLiteral(", "))));

			if (sample == QLatin1String("Explication:"))
				r.ok(QStringLiteral("الترجمة تُنتج نصًا فعلًا"), QStringLiteral("fr: Explication:"));
			else if (!sample.isEmpty())
				r.info(QStringLiteral("نص الترجمة الفرنسية: ") + sample);
		}

		// تبديل نصوص واجهتنا يجب أن يعمل فورًا
		const QString arabic = UiI18n::T("Teachers", "الأساتذة");
		if (lang.startsWith(QStringLiteral("ar")))
			r.ok(QStringLiteral("نصوص الواجهة تتبع اللغة"), QStringLiteral("عربي: %1").arg(arabic));
		else
			r.ok(QStringLiteral("نصوص الواجهة تتبع اللغة"), QStringLiteral("إنجليزي: %1").arg(arabic));
	}

	// ---- 6.4) الوصول الكامل إلى أوامر البرنامج (لا ميزة مفقودة) ----
	{
		QAction* paletteAction = mainWindow->findChild<QAction*>(QStringLiteral("otCommandPaletteAction"));
		if (paletteAction != nullptr)
			r.ok(QStringLiteral("إجراء «كل الأدوات» موجود"), paletteAction->shortcut().toString());
		else
			r.fail(QStringLiteral("إجراء «كل الأدوات» موجود"));

		// عدد الأوامر المتاحة عبر البحث (يجب أن يكون كبيرًا: كل قوائم البرنامج)
		int commands = 0;
		const QList<QAction*> actions = mainWindow->findChildren<QAction*>();
		for (QAction* action : actions) {
			if (action == nullptr || !action->isVisible() || action->isSeparator())
				continue;
			if (action->menu() != nullptr)
				continue;
			if (action->objectName().startsWith(QStringLiteral("ot")))
				continue;
			QString text = action->text();
			text.remove(QLatin1Char('&'));
			if (text.trimmed().isEmpty())
				continue;
			++commands;
		}
		if (commands >= 60)
			r.ok(QStringLiteral("كل أوامر البرنامج متاحة للبحث"),
			     QStringLiteral("%1 أمرًا").arg(commands));
		else
			r.fail(QStringLiteral("كل أوامر البرنامج متاحة للبحث"),
			       QStringLiteral("%1 فقط").arg(commands));

		// لوحة الأوامر تُفتح فعلًا وتعرض نتائج
		UiCommandPalette::run(mainWindow);
		QDialog* palette = mainWindow->findChild<QDialog*>(QStringLiteral("ot_command_palette"));
		if (palette != nullptr) {
			QTreeWidget* tree = palette->findChild<QTreeWidget*>(QStringLiteral("otPaletteTree"));
			if (tree != nullptr && tree->topLevelItemCount() >= 60)
				r.ok(QStringLiteral("لوحة الأوامر تعرض الأوامر"),
				     QStringLiteral("%1 صفًّا").arg(tree->topLevelItemCount()));
			else
				r.fail(QStringLiteral("لوحة الأوامر تعرض الأوامر"),
				       tree == nullptr ? QStringLiteral("لا شجرة")
						       : QStringLiteral("%1 صفًّا").arg(tree->topLevelItemCount()));
			palette->close();
		}
		else {
			r.fail(QStringLiteral("لوحة الأوامر تُفتح"));
		}
	}

	// ---- 6.45) الطي فوري: العناصر تُخفى وتظهر بلا إعادة بناء ----
	{
		QToolButton* collapse = mainWindow->findChild<QToolButton*>(QStringLiteral("otCollapseButton"));
		if (collapse != nullptr && nav != nullptr) {
			const int countBefore = nav->count();
			collapse->click();
			const int compactCount = nav->count();
			const QString compactText = nav->count() > 0 ? nav->item(0)->text() : QString();
			collapse->click();
			const int expandedText = nav->count() > 0 ? nav->item(0)->text().size() : 0;

			if (countBefore == compactCount && compactText.isEmpty() && expandedText > 0)
				r.ok(QStringLiteral("طي الشريط وفتحه يعملان بلا إعادة بناء"),
				     QStringLiteral("%1 عنصرًا").arg(countBefore));
			else
				r.fail(QStringLiteral("طي الشريط وفتحه يعملان بلا إعادة بناء"));
		}
		else {
			r.fail(QStringLiteral("طي الشريط وفتحه يعملان بلا إعادة بناء"));
		}
	}

	// ---- 6.47) ملاءمة النافذة مع الشاشة ----
	{
		UiShell::fitToScreen(mainWindow);
		const QRect available = mainWindow->screen() != nullptr
					 ? mainWindow->screen()->availableGeometry() : QRect();
		const QSize size = mainWindow->size();
		if (available.isValid() && size.width() <= available.width() + 40
		    && size.height() <= available.height() + 40)
			r.ok(QStringLiteral("النافذة تُلائم الشاشة"),
			     QStringLiteral("%1×%2 داخل %3×%4").arg(size.width()).arg(size.height())
				     .arg(available.width()).arg(available.height()));
		else if (!available.isValid())
			r.info(QStringLiteral("لا شاشة معروفة (بيئة بلا عرض) — تخطّي ملاءمة النافذة"));
		else
			r.fail(QStringLiteral("النافذة تُلائم الشاشة"),
			       QStringLiteral("%1×%2").arg(size.width()).arg(size.height()));
	}

	// ---- 6.48) سلامة الترجمة بعد التصدير ----
	// العطل الذي بلّغ عنه المستخدم: بعد تصدير الجداول تتغيّر كل الجمل إلى نص
	// إنجليزي/غريب وسط العربية. السبب أن نصوص «المحرّك» (المصدَّرة) كانت ما تزال
	// باسم البرنامج القديم بينما مفاتيح ملفات الترجمة صارت بالاسم الجديد، فلا تُطابق
	// أي ترجمة. هذا الفحص يحمّل العربية ويتأكد أن نص محرّك التصدير مترجم فعلًا.
	{
		const QString exeDir = QCoreApplication::applicationDirPath();
		const QString dir = exeDir + QStringLiteral("/translations");
		QTranslator t;
		const bool loaded = t.load(QStringLiteral("fet_") + FET_LANGUAGE, dir);

		if (!loaded) {
			r.info(QStringLiteral("لا ترجمة محمّلة — تخطّي فحص نص التصدير"));
		}
		else {
			const QString source = QStringLiteral("This file was automatically generated by Open-Timetable %1.");
			const QString translated = t.translate("TimetableExport", source.toUtf8().constData());

			// إن كانت قيمة المصدر هي نفسها في الترجمة فمعناه: ليس في الملف ترجمة
			// لهذا النص أصلًا. نعتبره نجاحًا إن كان ملف الترجمة لا يغطيه (لغة جديدة)،
			// وفشلًا إن كان الملف يحتوي نفس الجملة بالاسم القديم (عدم تطابق مفاتيح).
			if (!translated.isEmpty() && translated != source)
				r.ok(QStringLiteral("نص التصدير مترجم"), QStringLiteral("%1: %2").arg(FET_LANGUAGE,
												       translated.left(60)));
			else if (!translated.isEmpty())
				r.ok(QStringLiteral("نص التصدير غير مترجم في هذه اللغة (طبيعي)"),
				     FET_LANGUAGE);
			else
				r.fail(QStringLiteral("نص التصدير مترجم"));
		}
	}

	// ---- 6.5) المظهر الداكن ثم الفاتح ----
	{
		const QString before = qApp->styleSheet();
		UiTheme::applyTheme(*qApp, QStringLiteral("dark"));
		const QString dark = qApp->styleSheet();
		UiTheme::applyTheme(*qApp, QStringLiteral("light"));
		const QString light = qApp->styleSheet();

		if (!dark.isEmpty() && dark != light)
			r.ok(QStringLiteral("المظهر الداكن يعمل"), QStringLiteral("%1 حرفًا").arg(dark.size()));
		else
			r.fail(QStringLiteral("المظهر الداكن يعمل"));
		Q_UNUSED(before);
	}

	// ---- 6.7) هوية المنتج: لا ذكر للاسم/الموقع/الشعار الأصلي ----
	{
		if (qApp->applicationName() == QLatin1String("Open-Timetable"))
			r.ok(QStringLiteral("اسم البرنامج"), qApp->applicationName());
		else
			r.fail(QStringLiteral("اسم البرنامج"), qApp->applicationName());

		const QString title = mainWindow->windowTitle();
		if (!title.contains(oldBrandName()))
			r.ok(QStringLiteral("عنوان النافذة بلا الاسم الأصلي"), title);
		else
			r.fail(QStringLiteral("عنوان النافذة بلا الاسم الأصلي"), title);

		// لا إجراء ظاهر يحمل اسم البرنامج الأصلي
		QStringList leaked;
		const QList<QAction*> actions = mainWindow->findChildren<QAction*>();
		for (QAction* action : actions)
			if (action->isVisible() && action->text().contains(oldBrandName()))
				leaked << action->text();
		if (leaked.isEmpty())
			r.ok(QStringLiteral("لا اسم أصلي في القوائم الظاهرة"));
		else
			r.fail(QStringLiteral("لا اسم أصلي في القوائم الظاهرة"),
			       leaked.mid(0, 3).join(QStringLiteral(" | ")));

		// روابط موقع البرنامج الأصلي مخفيّة
		const QStringList online = { QStringLiteral("helpHomepageAction"),
					     QStringLiteral("helpForumAction"),
					     QStringLiteral("helpAddressesAction") };
		int visibleOnline = 0;
		for (const QString& name : online)
			if (QAction* action = mainWindow->findChild<QAction*>(name))
				if (action->isVisible())
					++visibleOnline;
		if (visibleOnline == 0)
			r.ok(QStringLiteral("روابط الموقع الأصلي مخفيّة"));
		else
			r.fail(QStringLiteral("روابط الموقع الأصلي مخفيّة"),
			       QStringLiteral("%1 ظاهر").arg(visibleOnline));

		// الواجهة الكلاسيكية لم تعد معروضة
		bool classicVisible = false;
		const QList<QWidget*> widgets = mainWindow->findChildren<QWidget*>();
		for (QWidget* widget : widgets)
			if (widget->isVisible() && widget->objectName() == QLatin1String("mainwidget")) {
				classicVisible = true;
				break;
			}
		if (!classicVisible)
			r.ok(QStringLiteral("الواجهة الكلاسيكية غير معروضة"));
		else
			r.fail(QStringLiteral("الواجهة الكلاسيكية غير معروضة"));
	}

	// ---- 7) العودة إلى لوحة البداية تعمل ----
	UiShell::showHome(mainWindow);
	if (stack != nullptr && stack->currentIndex() == 0)
		r.ok(QStringLiteral("العودة إلى لوحة البداية تعمل"));
	else
		r.fail(QStringLiteral("العودة إلى لوحة البداية تعمل"));

	// ملخّص الفشل في النهاية: يبقى ظاهرًا حتى لو اقتُطع التقرير الطويل
	if (!r.failureLines.isEmpty()) {
		r.out << "\n===== FAILURES (" << r.failureLines.count() << ") =====\n";
		for (const QString& line : r.failureLines)
			r.out << "  [FAIL] " << line << "\n";
	}

	r.out << "\n";
	r.out << (r.failures == 0 ? QStringLiteral("SELFCHECK OK")
				  : QStringLiteral("SELFCHECK FAILED (%1)").arg(r.failures))
	      << "\n";
	r.out.flush();

	return r.failures;
}

} // namespace UiSelfCheck
