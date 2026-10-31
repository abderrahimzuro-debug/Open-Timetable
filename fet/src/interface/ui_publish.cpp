/*
File ui_publish.cpp — Open-Timetable addition

This file is part of Open-Timetable, a modernised user interface for FET.
FET is free software; you can redistribute it and/or modify it under the terms
of the GNU Affero General Public License version 3 (see LICENSE).

"نشر إلى مجلد مشترك" — الخيار (أ) لخدمة الأدوار الثلاثة بلا خادم:
- يولّد كل ملفات الجداول (أساتذة، أفواج، سنوات، قاعات، مواد، XML، إحصاءات).
- يكتبها داخل مجلد جديد مؤرّخ يختاره المدير (مثلًا مجلد على شبكة المؤسسة).
- ينشئ صفحة بداية عربية (index.html) تجمع كل الجداول بروابط واضحة.
- ينسخ ملف البيانات الأصلي (.fet) ويشرح في README.txt كيف يستعملها كل دور.

ملاحظة تقنية: التوليد والكتابة يتمّان عبر TimetableExport (نفس ما يستعمله وضع سطر
الأوامر fet-cl)، فلا يوجد أي منطق مكرّر ولا تعديل على المحرّك.
*/

#include "ui_i18n.h"
#include "ui_publish.h"

#include "fet.h"
#include "fetmainform.h" // من أجل COMPANY / PROGRAM (إعدادات البرنامج)

#include <QApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QUrl>

#include "timetableexport.h"

extern bool students_schedule_ready;
extern bool teachers_schedule_ready;
extern bool rooms_buildings_schedule_ready;

namespace {

/// أسماء الملفات التي يولّدها FET، مع عنوان عربي لكل واحدة.
struct HtmlEntry {
	QString suffix;   // لاحقة اسم الملف (بدون البادئة)
	QString title;    // العنوان في صفحة البداية
	QString section;  // القسم
};

const HtmlEntry HTML_ENTRIES[] = {
	// الأفواج والسنوات
	{ "years_time_horizontal.html", "السنوات — الأسابيع × الحصص", "students" },
	{ "years_days_horizontal.html", "السنوات — الأيام × الحصص", "students" },
	{ "groups_time_horizontal.html", "الأفواج — الأسابيع × الحصص", "students" },
	{ "groups_days_horizontal.html", "الأفواج — الأيام × الحصص", "students" },
	{ "subgroups_time_horizontal.html", "الفرق الفرعية — الأسابيع × الحصص", "students" },
	{ "subgroups_days_horizontal.html", "الفرق الفرعية — الأيام × الحصص", "students" },
	// الأساتذة
	{ "teachers_time_horizontal.html", "الأساتذة — الأسابيع × الحصص", "teachers" },
	{ "teachers_days_horizontal.html", "الأساتذة — الأيام × الحصص", "teachers" },
	{ "teachers_free_periods_days_horizontal.html", "حصص الفراغ عند الأساتذة", "teachers" },
	{ "teachers_statistics.html", "إحصاءات الأساتذة", "teachers" },
	// القاعات
	{ "rooms_time_horizontal.html", "القاعات — الأسابيع × الحصص", "rooms" },
	{ "rooms_days_horizontal.html", "القاعات — الأيام × الحصص", "rooms" },
	{ "buildings_time_horizontal.html", "المباني — الأسابيع × الحصص", "rooms" },
	// المواد والوسوم
	{ "subjects_time_horizontal.html", "المواد — الأسابيع × الحصص", "misc" },
	{ "activity_tags_time_horizontal.html", "وسوم الأنشطة — الأسابيع × الحصص", "misc" },
	{ "activities_time_horizontal.html", "كل الأنشطة", "misc" },
	{ "students_statistics.html", "إحصاءات الأفواج", "misc" },
};

QString sanitizeFileName(QString name)
{
	name = name.trimmed();
	if (name.isEmpty())
		return QString();
	// نستبدل ما لا يصلح لأسماء المجلدات في الأنظمة الشائعة
	const QString invalid = QString::fromLatin1("\\/:*?\"<>|");
	for (const QChar& c : invalid)
		name.replace(c, QLatin1Char('-'));
	name.replace(QRegularExpression(QStringLiteral("\\s+")), QStringLiteral(" "));
	while (name.contains(QStringLiteral("--")))
		name.replace(QStringLiteral("--"), QStringLiteral("-"));
	return name.left(60).trimmed();
}

QString humanSize(qint64 bytes)
{
	if (bytes < 1024)
		return QStringLiteral("%1 بايت").arg(bytes);
	if (bytes < 1024 * 1024)
		return QStringLiteral("%1 ك.ب").arg(QString::number(bytes / 1024.0, 'f', 1));
	return QStringLiteral("%1 م.ب").arg(QString::number(bytes / (1024.0 * 1024.0), 'f', 1));
}

bool writeUtf8File(const QString& path, const QString& content)
{
	QFile file(path);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
		return false;
	file.write(content.toUtf8());
	file.close();
	return true;
}

QString buildLandingPage(const QString& dirPath, const QFileInfoList& htmlFiles, const QString& sourceFetName)
{
	QDir dir(dirPath);

	const QString institution = gt.rules.institutionName.trimmed().isEmpty()
		? QStringLiteral("مؤسسة تعليمية")
		: gt.rules.institutionName.trimmed();

	const QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm"));

	// خريطة: لاحقة الملف -> (القسم، العنوان)
	auto findEntry = [](const QString& fileName) -> const HtmlEntry* {
		for (const HtmlEntry& e : HTML_ENTRIES) {
			if (fileName.endsWith(e.suffix))
				return &e;
		}
		return nullptr;
	};

	QString studentsHtml, teachersHtml, roomsHtml, miscHtml;

	for (const QFileInfo& fi : htmlFiles) {
		const QString name = fi.fileName();
		if (name == QStringLiteral("index.html"))
			continue;

		const HtmlEntry* entry = findEntry(name);
		QString title = entry ? entry->title : name;
		QString section = entry ? entry->section : QStringLiteral("misc");

		const QString row = QStringLiteral(
			"      <li><a href=\"%1\">%2</a><span class=\"size\">%3</span></li>\n")
			.arg(name.toHtmlEscaped(),
			     title.toHtmlEscaped(),
			     humanSize(fi.size()));

		if (section == QLatin1String("students"))
			studentsHtml += row;
		else if (section == QLatin1String("teachers"))
			teachersHtml += row;
		else if (section == QLatin1String("rooms"))
			roomsHtml += row;
		else
			miscHtml += row;
	}

	// ملفات مساعدة (XML، بيانات، سجل التعارضات)
	QString extraHtml;
	const QStringList extraNames = {
		QStringLiteral("activities.xml"),
		QStringLiteral("teachers.xml"),
		QStringLiteral("subgroups.xml"),
		QStringLiteral("data_and_timetable.fet"),
		QStringLiteral("soft_conflicts.txt")
	};
	for (const QString& base : extraNames) {
		// الملفات تحمل بادئة اسم ملف البيانات، لذا نبحث باللاحقة
		const QFileInfoList found = dir.entryInfoList(QStringList() << QStringLiteral("*") + base,
							    QDir::Files, QDir::Name);
		for (const QFileInfo& fi : found) {
			extraHtml += QStringLiteral("      <li><a href=\"%1\">%1</a><span class=\"size\">%2</span></li>\n")
					     .arg(fi.fileName().toHtmlEscaped(), humanSize(fi.size()));
		}
	}
	if (!sourceFetName.isEmpty())
		extraHtml += QStringLiteral("      <li><a href=\"%1\">ملف البيانات الأصلي (يمكن فتحه في البرنامج)</a></li>\n")
				     .arg(sourceFetName.toHtmlEscaped());

	auto sectionBlock = [](const QString& title, const QString& subtitle, const QString& items) {
		if (items.isEmpty())
			return QString();
		return QStringLiteral(
			"  <section class=\"card\">\n"
			"    <h2>%1</h2>\n"
			"    <p class=\"sub\">%2</p>\n"
			"    <ul>\n%3    </ul>\n"
			"  </section>\n")
			.arg(title, subtitle, items);
	};

	QString html;
	html += QStringLiteral(
		"<!DOCTYPE html>\n"
		"<html lang=\"ar\" dir=\"rtl\">\n"
		"<head>\n"
		"  <meta charset=\"utf-8\">\n"
		"  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
		"  <title>جداول %1</title>\n"
		"  <style>\n"
		"    :root { --bg:#f6f7f9; --card:#ffffff; --text:#17202a; --muted:#5b6675; --accent:#2563eb; --line:#e3e7ed; }\n"
		"    * { box-sizing: border-box; }\n"
		"    body { margin:0; font-family: 'Segoe UI', Tahoma, 'Noto Sans Arabic', 'Cairo', sans-serif;\n"
		"           background:var(--bg); color:var(--text); line-height:1.7; }\n"
		"    header { background:#ffffff; border-bottom:1px solid var(--line); padding:28px 24px 22px; }\n"
		"    header h1 { margin:0 0 6px; font-size:1.6rem; }\n"
		"    header p { margin:0; color:var(--muted); }\n"
		"    main { max-width:1000px; margin:0 auto; padding:24px; display:grid; gap:18px;\n"
		"           grid-template-columns: repeat(auto-fit, minmax(320px, 1fr)); }\n"
		"    .card { background:var(--card); border:1px solid var(--line); border-radius:12px; padding:18px 20px; }\n"
		"    .card h2 { margin:0 0 4px; font-size:1.1rem; }\n"
		"    .card .sub { margin:0 0 10px; color:var(--muted); font-size:.9rem; }\n"
		"    ul { list-style:none; margin:0; padding:0; }\n"
		"    li { display:flex; justify-content:space-between; align-items:center; gap:12px;\n"
		"         padding:8px 0; border-top:1px solid var(--line); }\n"
		"    li:first-child { border-top:none; }\n"
		"    a { color:var(--accent); text-decoration:none; }\n"
		"    a:hover { text-decoration:underline; }\n"
		"    .size { color:var(--muted); font-size:.82rem; white-space:nowrap; }\n"
		"    footer { max-width:1000px; margin:0 auto; padding:8px 24px 40px; color:var(--muted); font-size:.85rem; }\n"
		"    @media print { header, footer { display:none; } .card { break-inside: avoid; } }\n"
		"  </style>\n"
		"</head>\n"
		"<body>\n"
		"<header>\n"
		"  <h1>جداول %1</h1>\n"
		"  <p>%2 — أُنشئت في %3</p>\n"
		"</header>\n"
		"<main>\n")
		.arg(institution.toHtmlEscaped(),
		     QStringLiteral("%1 أستاذًا، %2 فوجًا، %3 قاعة").arg(gt.rules.teachersList.count())
			     .arg(gt.rules.nInternalSubgroups)
			     .arg(gt.rules.roomsList.count()),
		     timestamp.toHtmlEscaped());

	html += sectionBlock(QStringLiteral("جداول الأفواج والسنوات"),
	                     QStringLiteral("للتلاميذ والأولياء: اختر القسم/الفوج ثم جدولك."),
	                     studentsHtml);
	html += sectionBlock(QStringLiteral("جداول الأساتذة"),
	                     QStringLiteral("لكل أستاذ جدوله الخاص داخل الصفحة."),
	                     teachersHtml);
	html += sectionBlock(QStringLiteral("جداول القاعات"),
	                     QStringLiteral("لتوزيع القاعات على الحصص."),
	                     roomsHtml);
	html += sectionBlock(QStringLiteral("ملفات أخرى وبيانات"),
	                     QStringLiteral("ملفات XML وبيانات البرنامج وسجل التعارضات."),
	                     miscHtml + extraHtml);

	html += QStringLiteral(
		"</main>\n"
		"<footer>\n"
		"  <p>لطباعة جدول: افتحه ثم <strong>Ctrl+P</strong>. الصفحة مهيّأة للطباعة وتخفي الأشرطة عند الطباعة.</p>\n"
		"  <p>نظام Open-Timetable — يعتمد محرّك Open-Timetable (AGPL-3.0).</p>\n"
		"</footer>\n"
		"</body>\n"
		"</html>\n");

	return html;
}

QString buildReadme(const QString& folderName, const QString& institution)
{
	return QStringLiteral(
		       "جداول %1\n"
		       "=====================================\n"
		       "\n"
		       "المجلد: %2\n"
		       "تاريخ الإنشاء: %3\n"
		       "\n"
		       "كيف تستعمل هذه الملفات؟\n"
		       "------------------------\n"
		       "1) ابدأ دائمًا من الملف: index.html\n"
		       "   افتحه بأي متصفح (Chrome / Edge / Firefox) وستجد روابط كل الجداول.\n"
		       "\n"
		       "2) الأستاذ: يفتح index.html ثم قسم \"جداول الأساتذة\" ويبحث عن اسمه\n"
		       "   داخل الصفحة (Ctrl+F) ليجد جدوله.\n"
		       "\n"
		       "3) التلميذ/الولي: يفتح \"جداول الأفواج والسنوات\" ويختار القسم أو الفوج.\n"
		       "\n"
		       "4) الطباعة: افتح الجدول ثم Ctrl+P. الصفحات مهيّأة للطباعة (A4 عرضي).\n"
		       "\n"
		       "ملفات مساعدة:\n"
		       "  - activities.xml / teachers.xml / subgroups.xml : بيانات للتبادل مع برامج أخرى.\n"
		       "  - soft_conflicts.txt : القيود غير المحقّقة (إن وُجدت).\n"
		       "  - source.fet : ملف البيانات الأصلي، يمكن فتحه في البرنامج لتعديله.\n"
		       "\n"
		       "ملاحظة: إن لم يظهر الجدول بشكل صحيح، استعمل متصفحًا حديثًا.\n"
		       "لا تحذف مجلد stylesheet (إن وُجد) لأنه يستعمله ملف index الأصلي للبرنامج.\n")
		.arg(institution.isEmpty() ? folderName : institution,
		     folderName,
		     QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm")));
}

} // namespace

namespace UiPublish {

bool canPublish()
{
	if (!gt.rules.initialized)
		return false;
	return students_schedule_ready && teachers_schedule_ready && rooms_buildings_schedule_ready;
}

QString suggestedFolderName()
{
	QString base = sanitizeFileName(gt.rules.institutionName);
	if (base.isEmpty())
		base = QStringLiteral("جداول");
	base += QLatin1Char('-') + QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd_HHmm"));
	return base;
}

namespace {

// نواة النشر: تُنشئ مجلدًا مؤرّخًا داخل baseDir، تولّد كل الملفات، وتكتب بوابة عربية.
// (بلا أي حوار — يستدعيها الزر اليدوي والنشر التلقائي بعد التوليد.)
bool publishCore(QWidget* parent, const QString& baseDir, QString* targetPathOut)
{
	if (!gt.rules.initialized || !canPublish())
		return false;

	const QString folderName = suggestedFolderName();
	QString targetPath = baseDir + QLatin1Char('/') + folderName;
	int counter = 2;
	while (QDir(targetPath).exists() && counter < 100) {
		targetPath = baseDir + QLatin1Char('/') + folderName + QStringLiteral("-%1").arg(counter);
		++counter;
	}

	QDir dir;
	if (!dir.mkpath(targetPath))
		return false;

	QApplication::setOverrideCursor(Qt::WaitCursor);

	TimetableExport::writeGenerationResultsCommandLine(parent, targetPath + QLatin1Char('/'));

	QString sourceFetName;
	if (!INPUT_FILENAME_XML.isEmpty()) {
		const QFileInfo srcInfo(INPUT_FILENAME_XML);
		if (srcInfo.exists()) {
			sourceFetName = QStringLiteral("source.fet");
			const QString destFile = targetPath + QLatin1Char('/') + sourceFetName;
			if (!QFile::copy(INPUT_FILENAME_XML, destFile))
				sourceFetName.clear();
		}
	}

	const QString fetIndex = targetPath + QLatin1Char('/') + QStringLiteral("index.html");
	const QString fetIndexBackup = targetPath + QLatin1Char('/') + QStringLiteral("fet-index.html");
	if (QFileInfo::exists(fetIndex))
		QFile::rename(fetIndex, fetIndexBackup);

	QDir targetDir(targetPath);
	const QFileInfoList htmlFiles = targetDir.entryInfoList(QStringList() << QStringLiteral("*.html"),
								QDir::Files, QDir::Name);
	const QString institution = gt.rules.institutionName.trimmed();
	writeUtf8File(fetIndex, buildLandingPage(targetPath, htmlFiles, sourceFetName));
	writeUtf8File(targetPath + QLatin1Char('/') + QStringLiteral("README.txt"),
		      buildReadme(targetDir.dirName(), institution));

	QApplication::restoreOverrideCursor();

	if (targetPathOut != nullptr)
		*targetPathOut = targetPath;

	return true;
}

} // namespace

bool runAutomatic(QWidget* parent, QString* targetPath)
{
	QSettings settings(COMPANY, PROGRAM);
	const QString dir = settings.value(QStringLiteral("publish-directory")).toString();
	if (dir.isEmpty() || !QDir(dir).exists())
		return false;

	QString target;
	const bool ok = publishCore(parent, dir, &target);
	if (ok && targetPath != nullptr)
		*targetPath = target;
	return ok;
}

void run(QWidget* parent)
{
	if (!gt.rules.initialized) {
		QMessageBox::information(parent, UiI18n::T("Publish timetables","نشر الجداول"),
					 UiI18n::T("Start a new file or open an existing one before publishing.","ابدأ ملفًا جديدًا أو افتح ملفًا موجودًا قبل النشر."));
		return;
	}

	if (!canPublish()) {
		QMessageBox::information(parent, UiI18n::T("Publish timetables","نشر الجداول"),
					 UiI18n::T("You must generate the timetable first. Press \"Generate\" and try again.","يجب توليد الجدول أولًا. اضغط على «توليد» ثم أعد المحاولة."));
		return;
	}

	QSettings settings(COMPANY, PROGRAM);
	const QString lastDir = settings.value(QStringLiteral("publish-directory"),
					       QDir::homePath() + QStringLiteral("/fet-published")).toString();

	const QString chosen = QFileDialog::getExistingDirectory(
		parent,
		UiI18n::T("Choose the sharing folder (for example a folder on the school network)","اختر مجلد المشاركة (مثلًا مجلد على شبكة المؤسسة)"),
		lastDir,
		QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

	if (chosen.isEmpty())
		return;

	// مجلد جديد مؤرّخ داخل المجلد المختار (بدون الكتابة فوق نشر سابق)
	QString targetPath;
	if (!publishCore(parent, chosen, &targetPath)) {
		QMessageBox::critical(parent, UiI18n::T("Publish timetables","نشر الجداول"),
				      UiI18n::T("Could not publish to:\n%1\n\nCheck the write permissions.","تعذّر النشر في:\n%1\n\nتأكّد من صلاحيات الكتابة.")
					      .arg(QDir::toNativeSeparators(chosen)));
		return;
	}

	QDir targetDir(targetPath);
	settings.setValue(QStringLiteral("publish-directory"), chosen);

	// 6) رسالة نجاح + فتح المجلد
	const int htmlCount = targetDir.entryInfoList(QStringList() << QStringLiteral("*.html"), QDir::Files).count();

	QMessageBox box(parent);
	box.setWindowTitle(UiI18n::T("Publish timetables","نشر الجداول"));
	box.setIcon(QMessageBox::Information);
	box.setText(UiI18n::T("The timetables were published successfully.","تم نشر الجداول بنجاح."));
	box.setInformativeText(
		UiI18n::T("Folder:\n%1\n\nTimetable pages: %2\n\n"
			       "Share this folder with teachers and students (school network or a USB copy). "
			       "Always start from index.html.",
			       "المجلد:\n%1\n\nعدد صفحات الجداول: %2\n\n"
			       "شارك هذا المجلد مع الأساتذة والتلاميذ (شبكة المؤسسة أو نسخة على USB). "
			       "نقطة البداية دائمًا هو الملف index.html.")
			.arg(QDir::toNativeSeparators(targetPath))
			.arg(htmlCount));
	QPushButton* openButton = box.addButton(UiI18n::T("Open folder","فتح المجلد"), QMessageBox::AcceptRole);
	box.addButton(UiI18n::T("Close","إغلاق"), QMessageBox::RejectRole);
	box.exec();

	if (box.clickedButton() == openButton)
		QDesktopServices::openUrl(QUrl::fromLocalFile(targetPath));
}

} // namespace UiPublish
