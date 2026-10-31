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
#include "ui_constraints.h"
#include "ui_i18n.h"
#include "ui_shell.h"

#include <QAction>
#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QTranslator>
#include <QListWidget>
#include <QMainWindow>
#include <QSplitter>
#include <QStackedWidget>
#include <QStyleFactory>
#include <QTextStream>

namespace {

struct Report {
	int failures;
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
	if (stack != nullptr)
		r.ok(QStringLiteral("الصفحات موجودة"), QStringLiteral("%1 صفحات").arg(stack->count()));
	else
		r.fail(QStringLiteral("الصفحات موجودة"));

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
		if (templates >= 29)
			r.ok(QStringLiteral("قوالب القيود (أساسيات + ضروريات)"),
			     QStringLiteral("%1 قالبًا").arg(templates));
		else
			r.fail(QStringLiteral("قوالب القيود (أساسيات + ضروريات)"),
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

	// ---- 7) العودة إلى لوحة البداية تعمل ----
	UiShell::showHome(mainWindow);
	if (stack != nullptr && stack->currentIndex() == 0)
		r.ok(QStringLiteral("العودة إلى لوحة البداية تعمل"));
	else
		r.fail(QStringLiteral("العودة إلى لوحة البداية تعمل"));

	r.out << "\n";
	r.out << (r.failures == 0 ? QStringLiteral("SELFCHECK OK")
				  : QStringLiteral("SELFCHECK FAILED (%1)").arg(r.failures))
	      << "\n";
	r.out.flush();

	return r.failures;
}

} // namespace UiSelfCheck
