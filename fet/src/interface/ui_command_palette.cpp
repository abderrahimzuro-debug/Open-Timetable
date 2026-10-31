/*
File ui_command_palette.cpp — Open-Timetable addition

This file is part of Open-Timetable, a modernised user interface for the
timetabling engine. Licensed under the GNU Affero General Public License v3.

التصميم:
- نأخذ كل QAction في النافذة الرئيسية (وهي كل أوامر البرنامج فعليًا: القوائم
  وأزرار الواجهة القديمة تُنشئ نفس الإجراءات).
- نستبعد: الإجراءات غير المرئية/المعطّلة، ومفاتيح الاختصار الصامتة، وما أضفناه
  نحن (حتى لا تتكرّر «القيود بالقوالب» مرتين).
- نعرض لكل أمر: نصّه، ومساره في القوائم (مثل: بيانات ‹ الأساتذة).
- الكتابة تُرشّح فورًا؛ Enter أو نقرة تُشغّل الأمر وتغلق النافذة.
*/

#include "ui_command_palette.h"

#include "ui_i18n.h"

#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QFont>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>
#include <QShortcut>
#include <QToolButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVariant>
#include <QVBoxLayout>

namespace {

const char* const PALETTE_PROPERTY = "ot_command_palette";

// يبني مسار الإجراء في القوائم: "بيانات ‹ الأساتذة"
QString menuPathFor(QMainWindow* window, QAction* action)
{
	if (window == nullptr || action == nullptr)
		return QString();

	QStringList path;
	const QList<QMenu*> menus = window->findChildren<QMenu*>();

	for (QMenu* menu : menus) {
		if (menu == nullptr)
			continue;
		if (!menu->actions().contains(action))
			continue;

		QWidget* parent = menu->parentWidget();
		while (parent != nullptr) {
			if (QMenu* parentMenu = qobject_cast<QMenu*>(parent)) {
				if (!parentMenu->title().isEmpty())
					path.prepend(parentMenu->title());
				parent = parentMenu->parentWidget();
			}
			else {
				break;
			}
		}
		if (!menu->title().isEmpty())
			path.append(menu->title());
		break;
	}

	// إزالة علامات الاختصار (&) من المسار
	for (QString& part : path)
		part.remove(QLatin1Char('&'));

	return path.join(QStringLiteral(" ‹ "));
}

QString cleanText(QAction* action)
{
	QString text = action->text();
	text.remove(QLatin1Char('&'));
	// بعض الإجراءات تخزّن نصًّا طويلًا جدًّا؛ نأخذ أول سطر فقط
	const int newline = text.indexOf(QLatin1Char('\n'));
	if (newline > 0)
		text = text.left(newline);
	return text.trimmed();
}

void activate(QMainWindow* window, QAction* action)
{
	if (action == nullptr || !action->isEnabled())
		return;
	// إغلاق النافذة قبل التنفيذ حتى لا تظهر وراء الحوارات التي يفتحها الأمر
	QDialog* dialog = window->findChild<QDialog*>(QString::fromLatin1(PALETTE_PROPERTY));
	if (dialog != nullptr)
		dialog->close();

	action->trigger();
}

} // namespace

namespace UiCommandPalette {

void run(QMainWindow* window)
{
	if (window == nullptr)
		return;

	// نافذة واحدة فقط
	if (QDialog* existing = window->findChild<QDialog*>(QString::fromLatin1(PALETTE_PROPERTY))) {
		existing->show();
		existing->raise();
		existing->activateWindow();
		return;
	}

	QDialog* dialog = new QDialog(window);
	dialog->setObjectName(QString::fromLatin1(PALETTE_PROPERTY));
	dialog->setProperty(PALETTE_PROPERTY, true);
	dialog->setWindowTitle(UiI18n::T("All commands", "كل الأدوات"));
	dialog->resize(560, 460);
	dialog->setModal(false);

	QVBoxLayout* layout = new QVBoxLayout(dialog);
	layout->setContentsMargins(14, 12, 14, 12);
	layout->setSpacing(8);

	QLabel* hint = new QLabel(UiI18n::T("Type to search every command of the program",
					    "اكتب للبحث في كل أوامر البرنامج"), dialog);
	hint->setObjectName(QStringLiteral("otPaletteHint"));
	layout->addWidget(hint);

	QLineEdit* search = new QLineEdit(dialog);
	search->setObjectName(QStringLiteral("otPaletteSearch"));
	search->setPlaceholderText(UiI18n::T("Search…  (Ctrl+Shift+P)", "بحث…  (Ctrl+Shift+P)"));
	search->setClearButtonEnabled(true);
	layout->addWidget(search);

	QTreeWidget* tree = new QTreeWidget(dialog);
	tree->setObjectName(QStringLiteral("otPaletteTree"));
	tree->setColumnCount(2);
	tree->setHeaderLabels({ UiI18n::T("Command", "الأمر"), UiI18n::T("Where", "المكان") });
	tree->setRootIsDecorated(false);
	tree->setUniformRowHeights(true);
	tree->setAlternatingRowColors(true);
	tree->setIconSize(QSize(16, 16));
	tree->header()->setStretchLastSection(false);
	tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	layout->addWidget(tree, 1);

	QLabel* empty = new QLabel(UiI18n::T("No matching command", "لا يوجد أمر مطابق"), dialog);
	empty->setObjectName(QStringLiteral("otPaletteEmpty"));
	empty->setAlignment(Qt::AlignCenter);
	empty->setVisible(false);
	layout->addWidget(empty);

	// تجميع الأوامر
	const QList<QAction*> actions = window->findChildren<QAction*>();
	for (QAction* action : actions) {
		if (action == nullptr || !action->isVisible())
			continue;
		if (action->isSeparator())
			continue;
		if (action->menu() != nullptr)
			continue; // عناصر القوائم نفسها (فروع) ليست أوامر
		if (action->objectName().startsWith(QStringLiteral("ot")))
			continue; // أوامرنا لها أماكنها في الشريط الجانبي

		const QString text = cleanText(action);
		if (text.isEmpty())
			continue;

		const QString path = menuPathFor(window, action);

		QTreeWidgetItem* item = new QTreeWidgetItem(tree);
		item->setText(0, text);
		item->setText(1, path);
		// لا نعرض أيقونات الإجراءات: كثير منها أيقونات موارد لم تعد موجودة في
		// حزمتنا، فتظهر مربّعات فارغة. الاعتماد على النص أوضح وأسرع.

		item->setData(0, Qt::UserRole, QVariant::fromValue(static_cast<QObject*>(action)));
		if (action->shortcut().isEmpty())
			item->setToolTip(0, text);
		else
			item->setToolTip(0, text + QStringLiteral("  (") + action->shortcut().toString() + QLatin1Char(')'));
		if (!action->isEnabled())
			item->setDisabled(true);
	}

	const int total = tree->topLevelItemCount();
	hint->setText(UiI18n::T("Type to search every command of the program", "اكتب للبحث في كل أوامر البرنامج")
		      + QStringLiteral(" — ") + QString::number(total));

	auto activateCurrent = [window, tree]() {
		QTreeWidgetItem* item = tree->currentItem();
		if (item == nullptr)
			return;
		QAction* action = qobject_cast<QAction*>(item->data(0, Qt::UserRole).value<QObject*>());
		activate(window, action);
	};

	QObject::connect(tree, &QTreeWidget::itemActivated, dialog, [activateCurrent](QTreeWidgetItem*) {
		activateCurrent();
	});
	QObject::connect(tree, &QTreeWidget::itemClicked, dialog,
			 [activateCurrent](QTreeWidgetItem*, int) { activateCurrent(); });

	// ترشيح فوري
	QObject::connect(search, &QLineEdit::textChanged, dialog, [tree, empty](const QString& text) {
		const QString needle = text.trimmed();
		int visible = 0;
		for (int i = 0; i < tree->topLevelItemCount(); ++i) {
			QTreeWidgetItem* item = tree->topLevelItem(i);
			const bool match = needle.isEmpty()
					   || item->text(0).contains(needle, Qt::CaseInsensitive)
					   || item->text(1).contains(needle, Qt::CaseInsensitive);
			item->setHidden(!match);
			if (match)
				++visible;
		}
		empty->setVisible(visible == 0);
	});

	// Enter من حقل البحث يشغّل أول نتيجة
	QObject::connect(search, &QLineEdit::returnPressed, dialog, [tree, activateCurrent]() {
		for (int i = 0; i < tree->topLevelItemCount(); ++i) {
			if (!tree->topLevelItem(i)->isHidden()) {
				tree->setCurrentItem(tree->topLevelItem(i));
				break;
			}
		}
		activateCurrent();
	});

	QShortcut* closeShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), dialog);
	QObject::connect(closeShortcut, &QShortcut::activated, dialog, &QDialog::close);

	QObject::connect(dialog, &QDialog::finished, dialog, &QObject::deleteLater);

	dialog->show();
	search->setFocus();
}

} // namespace UiCommandPalette
