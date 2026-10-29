/*
File ui_theme.cpp — Open-Timetable addition

This file is part of Open-Timetable, a modernised user interface for FET.
FET is free software; you can redistribute it and/or modify it under the terms
of the GNU Affero General Public License version 3 (see LICENSE).

الهدف: مظهر حديث وموحّد للواجهة الرسومية دون المساس بمنطق العمل.
- نمط Fusion (يضمن نفس الشكل على ويندوز ولينكس وماك).
- خط عربي مقروء إذا كان متوفّرًا على النظام.
- ورقة أنماط (QSS) تُحسّن الحشو والحدود والزوايا وجداول البيانات والقوائم.

ملاحظة: ورقة الأنماط هنا لا تلمس عناصر الجداول الزمنية المخصّصة (تُرسم يدويًا
داخل البرنامج)، ولا تعيّن خلفية عامة لكل QWidget حتى لا تُفسد الرسم المخصّص.
*/

#include "ui_theme.h"

#include <QApplication>
#include <QFont>
#include <QFontDatabase>
#include <QPalette>
#include <QColor>
#include <QStyleFactory>
#include <QStringList>

namespace {

const char* const LIGHT_QSS = R"QSS(
/* ===== أساسيات ===== */
QMainWindow, QDialog {
	background-color: #f7f8fa;
}

QLabel {
	color: #17202a;
}

QLabel#titleLabel, QLabel[class="title"] {
	font-size: 15pt;
	font-weight: 600;
	color: #0f172a;
}

QLabel#subtitleLabel, QLabel[class="subtitle"] {
	color: #5b6675;
	font-size: 10pt;
}

/* ===== أزرار ===== */
QPushButton {
	background-color: #ffffff;
	border: 1px solid #d5dae2;
	border-radius: 6px;
	color: #17202a;
	padding: 6px 14px;
	min-height: 22px;
}

QPushButton:hover {
	background-color: #f1f4f8;
	border-color: #c3cad5;
}

QPushButton:pressed {
	background-color: #e6ebf2;
}

QPushButton:disabled {
	color: #9aa4b2;
	background-color: #f4f5f7;
	border-color: #e3e7ed;
}

QPushButton:default {
	background-color: #2563eb;
	border-color: #1d4ed8;
	color: #ffffff;
	font-weight: 600;
}

QPushButton:default:hover {
	background-color: #1d4ed8;
}

QPushButton:default:pressed {
	background-color: #1e40af;
}

/* ===== حقول الإدخال ===== */
QLineEdit, QPlainTextEdit, QTextEdit, QSpinBox, QDoubleSpinBox, QComboBox {
	background-color: #ffffff;
	border: 1px solid #d5dae2;
	border-radius: 6px;
	padding: 5px 8px;
	selection-background-color: #bfd4ff;
	selection-color: #0f172a;
	min-height: 20px;
}

QLineEdit:focus, QPlainTextEdit:focus, QTextEdit:focus,
QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus {
	border-color: #2563eb;
}

QLineEdit:disabled, QSpinBox:disabled, QDoubleSpinBox:disabled, QComboBox:disabled {
	background-color: #f4f5f7;
	color: #9aa4b2;
}

QComboBox::drop-down {
	border: none;
	width: 20px;
}

QComboBox QAbstractItemView {
	background-color: #ffffff;
	border: 1px solid #d5dae2;
	border-radius: 6px;
	padding: 4px;
	selection-background-color: #e8efff;
	selection-color: #0f172a;
}

/* ===== مجموعات ومربّعات اختيار ===== */
QGroupBox {
	border: 1px solid #e1e5eb;
	border-radius: 8px;
	margin-top: 14px;
	padding: 12px 10px 10px 10px;
	font-weight: 600;
	color: #0f172a;
}

QGroupBox::title {
	subcontrol-origin: margin;
	subcontrol-position: top start;
	left: 10px;
	padding: 0 4px;
	color: #344054;
}

QCheckBox, QRadioButton {
	spacing: 7px;
	color: #17202a;
}

/* ===== جداول وشجرات ===== */
QTableView, QTableWidget, QTreeView, QTreeWidget, QListView, QListWidget {
	background-color: #ffffff;
	border: 1px solid #e1e5eb;
	border-radius: 6px;
	gridline-color: #eef1f5;
	selection-background-color: #dbe7ff;
	selection-color: #0f172a;
	alternate-background-color: #fafbfc;
}

QHeaderView::section {
	background-color: #f4f6f9;
	color: #344054;
	border: none;
	border-right: 1px solid #e6eaf0;
	border-bottom: 1px solid #e1e5eb;
	padding: 7px 8px;
	font-weight: 600;
}

QTableView::item, QTableWidget::item, QTreeView::item {
	padding: 3px 5px;
}

/* ===== تبويبات ===== */
QTabWidget::pane {
	border: 1px solid #e1e5eb;
	border-radius: 8px;
	background-color: #ffffff;
	top: -1px;
}

QTabBar::tab {
	background-color: #eef1f6;
	color: #475467;
	border: 1px solid #e1e5eb;
	border-bottom: none;
	border-top-left-radius: 7px;
	border-top-right-radius: 7px;
	padding: 7px 16px;
	margin-right: 2px;
}

QTabBar::tab:selected {
	background-color: #ffffff;
	color: #1d4ed8;
	font-weight: 600;
}

QTabBar::tab:hover:!selected {
	background-color: #e6ebf3;
}

/* ===== قوائم وأشرطة ===== */
QMenuBar {
	background-color: #ffffff;
	border-bottom: 1px solid #e1e5eb;
	padding: 2px;
}

QMenuBar::item {
	padding: 6px 10px;
	border-radius: 5px;
	background: transparent;
}

QMenuBar::item:selected {
	background-color: #eef1f6;
}

QMenu {
	background-color: #ffffff;
	border: 1px solid #d5dae2;
	border-radius: 8px;
	padding: 6px;
}

QMenu::item {
	padding: 6px 26px;
	border-radius: 5px;
}

QMenu::item:selected {
	background-color: #e8efff;
	color: #0f172a;
}

QMenu::separator {
	height: 1px;
	background-color: #eef1f5;
	margin: 5px 8px;
}

QToolBar {
	background-color: #ffffff;
	border: none;
	border-bottom: 1px solid #e1e5eb;
	spacing: 4px;
	padding: 4px;
}

QToolBar QToolButton {
	border: 1px solid transparent;
	border-radius: 6px;
	padding: 4px 8px;
}

QToolBar QToolButton:hover {
	background-color: #eef1f6;
	border-color: #d5dae2;
}

QStatusBar {
	background-color: #ffffff;
	border-top: 1px solid #e1e5eb;
	color: #475467;
}

/* ===== أشرطة التمرير ===== */
QScrollBar:vertical {
	background: transparent;
	width: 12px;
	margin: 0;
}

QScrollBar::handle:vertical {
	background: #c8d0db;
	border-radius: 6px;
	min-height: 28px;
}

QScrollBar::handle:vertical:hover {
	background: #aab6c4;
}

QScrollBar:horizontal {
	background: transparent;
	height: 12px;
	margin: 0;
}

QScrollBar::handle:horizontal {
	background: #c8d0db;
	border-radius: 6px;
	min-width: 28px;
}

QScrollBar::handle:horizontal:hover {
	background: #aab6c4;
}

QScrollBar::add-line, QScrollBar::sub-line {
	width: 0;
	height: 0;
}

QScrollBar::add-page, QScrollBar::sub-page {
	background: transparent;
}

/* ===== متفرقات ===== */
QProgressBar {
	border: 1px solid #e1e5eb;
	border-radius: 6px;
	background-color: #f4f5f7;
	text-align: center;
	height: 18px;
}

QProgressBar::chunk {
	background-color: #2563eb;
	border-radius: 5px;
}

QToolTip {
	background-color: #17202a;
	color: #ffffff;
	border: none;
	border-radius: 6px;
	padding: 6px 8px;
}

QSplitter::handle {
	background-color: #eef1f5;
}

QSplitter::handle:horizontal {
	width: 4px;
}

QSplitter::handle:vertical {
	height: 4px;
}
)QSS";

const char* const DARK_QSS = R"QSS(
/* ===== أساسيات (مظهر داكن) ===== */
QMainWindow, QDialog {
	background-color: #12161d;
}

QLabel, QCheckBox, QRadioButton {
	color: #e6eaf0;
}

QPushButton {
	background-color: #1c2330;
	border: 1px solid #2b3446;
	border-radius: 6px;
	color: #e6eaf0;
	padding: 6px 14px;
	min-height: 22px;
}

QPushButton:hover {
	background-color: #242d3d;
	border-color: #3a465c;
}

QPushButton:pressed {
	background-color: #2c374a;
}

QPushButton:disabled {
	color: #6b7686;
	background-color: #171d27;
	border-color: #232b39;
}

QPushButton:default {
	background-color: #3b82f6;
	border-color: #2563eb;
	color: #ffffff;
	font-weight: 600;
}

QLineEdit, QPlainTextEdit, QTextEdit, QSpinBox, QDoubleSpinBox, QComboBox {
	background-color: #171d27;
	border: 1px solid #2b3446;
	border-radius: 6px;
	padding: 5px 8px;
	color: #e6eaf0;
	selection-background-color: #1d4ed8;
	selection-color: #ffffff;
	min-height: 20px;
}

QLineEdit:focus, QPlainTextEdit:focus, QTextEdit:focus,
QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus {
	border-color: #3b82f6;
}

QComboBox QAbstractItemView {
	background-color: #171d27;
	border: 1px solid #2b3446;
	color: #e6eaf0;
	selection-background-color: #1d4ed8;
	selection-color: #ffffff;
}

QGroupBox {
	border: 1px solid #262f3f;
	border-radius: 8px;
	margin-top: 14px;
	padding: 12px 10px 10px 10px;
	font-weight: 600;
	color: #e6eaf0;
}

QGroupBox::title {
	subcontrol-origin: margin;
	subcontrol-position: top start;
	left: 10px;
	padding: 0 4px;
	color: #b6c0cf;
}

QTableView, QTableWidget, QTreeView, QTreeWidget, QListView, QListWidget {
	background-color: #171d27;
	border: 1px solid #262f3f;
	border-radius: 6px;
	gridline-color: #232b39;
	color: #e6eaf0;
	selection-background-color: #1e3a8a;
	selection-color: #ffffff;
	alternate-background-color: #1b222d;
}

QHeaderView::section {
	background-color: #1c2330;
	color: #b6c0cf;
	border: none;
	border-right: 1px solid #262f3f;
	border-bottom: 1px solid #262f3f;
	padding: 7px 8px;
	font-weight: 600;
}

QTabWidget::pane {
	border: 1px solid #262f3f;
	border-radius: 8px;
	background-color: #171d27;
	top: -1px;
}

QTabBar::tab {
	background-color: #1a212c;
	color: #9aa5b4;
	border: 1px solid #262f3f;
	border-bottom: none;
	border-top-left-radius: 7px;
	border-top-right-radius: 7px;
	padding: 7px 16px;
	margin-right: 2px;
}

QTabBar::tab:selected {
	background-color: #171d27;
	color: #93c5fd;
	font-weight: 600;
}

QMenuBar {
	background-color: #151b25;
	border-bottom: 1px solid #262f3f;
	color: #e6eaf0;
}

QMenuBar::item:selected {
	background-color: #242d3d;
}

QMenu {
	background-color: #171d27;
	border: 1px solid #2b3446;
	color: #e6eaf0;
	padding: 6px;
}

QMenu::item:selected {
	background-color: #1e3a8a;
}

QStatusBar {
	background-color: #151b25;
	border-top: 1px solid #262f3f;
	color: #9aa5b4;
}

QScrollBar::handle:vertical, QScrollBar::handle:horizontal {
	background: #37415a;
	border-radius: 6px;
}

QToolTip {
	background-color: #e6eaf0;
	color: #12161d;
	border: none;
	border-radius: 6px;
	padding: 6px 8px;
}
)QSS";

QString pickUiFontFamily()
{
	// نفضّل خطوطًا تدعم العربية بشكل جيد وتكون متوفرة على الأنظمة الشائعة.
	static const char* const candidates[] = {
		"Cairo",
		"Noto Sans Arabic",
		"Noto Naskh Arabic",
		"Segoe UI",
		"Tahoma",
		"DejaVu Sans",
		"Liberation Sans",
		nullptr
	};

	const QStringList families = QFontDatabase().families();
	for (int i = 0; candidates[i] != nullptr; ++i) {
		const QString candidate = QString::fromLatin1(candidates[i]);
		for (const QString& family : families) {
			if (family.compare(candidate, Qt::CaseInsensitive) == 0)
				return family;
		}
	}

	return QString();
}

} // namespace

QStringList UiTheme::availableThemes()
{
	QStringList list;
	list << QStringLiteral("light") << QStringLiteral("dark");
	return list;
}

QString UiTheme::styleSheet(const QString& name)
{
	if (name == QStringLiteral("dark"))
		return QString::fromUtf8(DARK_QSS);

	return QString::fromUtf8(LIGHT_QSS);
}

void UiTheme::apply(QApplication& app)
{
	// 1) نمط Fusion: يضمن مظهرًا متسقًا على المنصّات ويحترم ورقة الأنماط بالكامل.
	if (QStyleFactory::keys().contains(QStringLiteral("Fusion"), Qt::CaseInsensitive))
		app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

	// 2) خط الواجهة (يدعم العربية) إن كان متوفّرًا.
	QFont appFont = app.font();
	const QString family = pickUiFontFamily();
	if (!family.isEmpty())
		appFont.setFamily(family);
	appFont.setPointSizeF(10.0);
	app.setFont(appFont);

	// 3) لوحة ألوان أساسية (تظهر خلف عناصر لم تُشمل بورقة الأنماط).
	QPalette palette;
	palette.setColor(QPalette::Window, QColor(0xf7, 0xf8, 0xfa));
	palette.setColor(QPalette::Base, QColor(0xff, 0xff, 0xff));
	palette.setColor(QPalette::AlternateBase, QColor(0xfa, 0xfb, 0xfc));
	palette.setColor(QPalette::Text, QColor(0x17, 0x20, 0x2a));
	palette.setColor(QPalette::WindowText, QColor(0x17, 0x20, 0x2a));
	palette.setColor(QPalette::ButtonText, QColor(0x17, 0x20, 0x2a));
	palette.setColor(QPalette::Button, QColor(0xff, 0xff, 0xff));
	palette.setColor(QPalette::Highlight, QColor(0x25, 0x63, 0xeb));
	palette.setColor(QPalette::HighlightedText, QColor(0xff, 0xff, 0xff));
	palette.setColor(QPalette::ToolTipBase, QColor(0x17, 0x20, 0x2a));
	palette.setColor(QPalette::ToolTipText, QColor(0xff, 0xff, 0xff));
	app.setPalette(palette);

	// 4) ورقة الأنماط.
	app.setStyleSheet(styleSheet(QStringLiteral("light")));
}
