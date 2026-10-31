/*
File helpaboutform.cpp — Open-Timetable

This file is part of Open-Timetable, a modernised user interface for the
timetabling engine. Licensed under the GNU Affero General Public License v3.

نافذة «عن البرنامج» أعيدت كتابتها: بلا اسم البرنامج الأصلي أو موقعه أو شعاره،
وبلا قوائم المترجمين والروابط الخارجية. تُبقي فقط ما يفرضه الترخيص: بيان أن
المحرّك مفتوح المصدر تحت AGPLv3 مع اسم مؤلّفه الأصلي كما في NOTICE.md.
*/

#include <QCoreApplication>
#include <QSize>
#include <QString>
#include <QTabWidget>
#include <QTextBrowser>

#include "helpaboutform.h"
#include "ui_icons.h"
#include "ui_i18n.h"
#include "ui_theme.h"

#include "timetable_defs.h"

#include "utilities.h"

HelpAboutForm::HelpAboutForm(QWidget* parent)
	: QDialog(parent)
{
	setupUi(this);

	closePushButton->setDefault(true);

	const bool arabic = UiI18n::isArabic();

	setWindowTitle(UiI18n::T("About Open-Timetable", "عن Open-Timetable"));

	const QString body = (arabic
		? QStringLiteral(
			"<p style=\"font-size:15pt; font-weight:700;\">Open-Timetable</p>"
			"<p>برنامج جدولة دراسية بواجهة عربية مبسّطة للمدارس والثانويات.</p>"
			"<p>الإصدار: %1</p>"
			"<p>محرّك الجدولة المدمج: الإصدار %2</p>")
		: QStringLiteral(
			"<p style=\"font-size:15pt; font-weight:700;\">Open-Timetable</p>"
			"<p>A school timetabling program with a simple, modern interface.</p>"
			"<p>Version: %1</p>"
			"<p>Embedded timetabling engine: version %2</p>"));

	aboutTextBrowser->setHtml(
		body.arg(QStringLiteral("4"), FET_VERSION)
		+ (arabic
			   ? QStringLiteral(
				   "<p>الترخيص: GNU Affero GPL الإصدار 3.</p>"
				   "<p>محرّك الجدولة مفتوح المصدر، مؤلّفه الأصلي Liviu Lalescu، "
				   "وتفاصيل الإسناد والترخيص في الملفين NOTICE.md وLICENSE في جذر المشروع.</p>")
			   : QStringLiteral(
				   "<p>Licence: GNU Affero GPL version 3.</p>"
				   "<p>The timetabling engine is open source; original author Liviu Lalescu. "
				   "Attribution and licence details: NOTICE.md and LICENSE in the project root.</p>")));

	// بقية علامات التبويب لم تعد تعرض شيئًا؛ نُخفيها لتبقى النافذة بسيطة.
	const QList<QWidget*> extraTabs = { authorsTab, contributorsTab, translatorsTab,
					    referencesTab, thanksToTab };
	for (QWidget* tab : extraTabs) {
		const int index = tabWidget->indexOf(tab);
		if (index >= 0)
			tabWidget->removeTab(index);
	}

	for (QTextBrowser* browser : { aboutTextBrowser, authorsTextBrowser, contributorsTextBrowser,
				       translatorsTextBrowser, referencesTextBrowser, thanksToTextBrowser }) {
		if (browser != nullptr) {
			browser->setReadOnly(true);
			browser->setOpenExternalLinks(false);
		}
	}

	connect(closePushButton, &QPushButton::clicked, this, &HelpAboutForm::close);

	centerWidgetOnScreenAndOtherThings(this);
	restoreFETDialogGeometry(this);

	tabWidget->setCurrentIndex(0);
}

HelpAboutForm::~HelpAboutForm()
{
}
