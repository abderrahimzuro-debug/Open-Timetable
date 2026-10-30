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
#include "ui_shell.h"

#include <QAction>
#include <QApplication>
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

	// ---- 6) العودة إلى لوحة البداية تعمل ----
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
