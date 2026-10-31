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

#include "ui_icons.h"

#include <QApplication>
#include <QFont>
#include <QFontDatabase>
#include <QPalette>
#include <QColor>
#include <QStyleFactory>
#include <QSettings>
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

/* ===== Open-Timetable: الشريط الجانبي ولوحة البداية ===== */
QWidget#otSidebar {
	background-color: #ffffff;
	border: none;
	border-right: 1px solid #e3e7ed;
}

QLabel#otBrand {
	font-size: 13pt;
	font-weight: 700;
	color: #0f172a;
}

QLabel#otBrandSub {
	color: #7a869a;
	font-size: 9pt;
}

QListWidget#otNav {
	background: transparent;
	border: none;
	padding: 4px 6px;
}

QListWidget#otNav::item {
	padding: 7px 10px;
	border-radius: 6px;
	margin: 0px 2px;
	color: #344054;
}

QListWidget#otNav::item:selected {
	background-color: #e8efff;
	color: #1d4ed8;
	font-weight: 600;
}

QListWidget#otNav::item:hover:!selected {
	background-color: #f1f4f8;
}

QListWidget#otNav::item:disabled {
	color: #98a2b3;
	background: transparent;
}


/* أزرار ترويسة الشريط الجانبي (الطي والمظهر) */
QToolButton#otThemeButton, QToolButton#otCollapseButton {
	background: transparent;
	border: 1px solid transparent;
	border-radius: 6px;
	padding: 3px;
}
QToolButton#otThemeButton:hover, QToolButton#otCollapseButton:hover {
	background-color: #eef1f6;
	border-color: #e1e6ee;
}
QToolButton#otThemeButton:pressed, QToolButton#otCollapseButton:pressed {
	background-color: #e3e8f0;
}


/* لوحة الأوامر (كل الأدوات) */
QLabel#otPaletteHint {
	color: #5b6675;
	font-size: 9.5pt;
}

QLabel#otPaletteEmpty {
	color: #7a869a;
	padding: 18px;
}

QLineEdit#otPaletteSearch {
	padding: 7px 10px;
	font-size: 11pt;
}

QTreeWidget#otPaletteTree {
	background-color: #ffffff;
	border: 1px solid #e3e7ed;
	border-radius: 8px;
}

QLabel#otDashTitle {
	font-size: 20pt;
	font-weight: 700;
	color: #0f172a;
}

QLabel#otDashSubtitle {
	color: #475467;
	font-size: 11pt;
}

QLabel#otDashFile {
	color: #7a869a;
	font-size: 9.5pt;
}

QLabel#otSectionTitle {
	font-weight: 600;
	color: #344054;
	font-size: 11pt;
}

QFrame#otCard {
	background-color: #ffffff;
	border: 1px solid #e3e7ed;
	border-radius: 12px;
}

QFrame#otCard QLabel {
	background: transparent;
	border: none;
}

QLabel#otCardValue {
	font-size: 16pt;
	font-weight: 700;
}

/* ألوان أرقام البطاقات: تُختار بالمظهر، فتبقى واضحة على الخلفيتين */
QLabel#otCardValue[otAccent="blue"]   { color: #1d4ed8; }
QLabel#otCardValue[otAccent="teal"]   { color: #0f766e; }
QLabel#otCardValue[otAccent="violet"] { color: #6d28d9; }
QLabel#otCardValue[otAccent="purple"] { color: #9333ea; }
QLabel#otCardValue[otAccent="orange"] { color: #c2410c; }
QLabel#otCardValue[otAccent="sky"]    { color: #0369a1; }
QLabel#otCardValue[otAccent="amber"]  { color: #b45309; }
QLabel#otCardValue[otAccent="rose"]   { color: #be123c; }
QLabel#otCardValue[otAccent="olive"]  { color: #4d7c0f; }

QLabel#otCardLabel {
	color: #5b6675;
	font-size: 9pt;
}

QScrollArea#otDashScroll, QWidget#otDashPage {
	background: transparent;
	border: none;
}
)QSS";

const char* const DARK_QSS = R"QSS(
/* ===== أساسيات ===== */
QMainWindow, QDialog {
	background-color: #12161d;
}

QLabel {
	color: #e6eaf0;
}

QLabel#titleLabel, QLabel[class="title"] {
	font-size: 15pt;
	font-weight: 600;
	color: #f1f5f9;
}

QLabel#subtitleLabel, QLabel[class="subtitle"] {
	color: #9aa7b8;
	font-size: 10pt;
}

/* ===== أزرار ===== */
QPushButton {
	background-color: #171d27;
	border: 1px solid #33405a;
	border-radius: 6px;
	color: #e6eaf0;
	padding: 6px 14px;
	min-height: 22px;
}

QPushButton:hover {
	background-color: #1f2836;
	border-color: #3a465c;
}

QPushButton:pressed {
	background-color: #2a3546;
}

QPushButton:disabled {
	color: #8b97a8;
	background-color: #1f2836;
	border-color: #262f3f;
}

QPushButton:default {
	background-color: #2563eb;
	border-color: #3b82f6;
	color: #171d27;
	font-weight: 600;
}

QPushButton:default:hover {
	background-color: #3b82f6;
}

QPushButton:default:pressed {
	background-color: #1d4ed8;
}

/* ===== حقول الإدخال ===== */
QLineEdit, QPlainTextEdit, QTextEdit, QSpinBox, QDoubleSpinBox, QComboBox {
	background-color: #171d27;
	border: 1px solid #33405a;
	border-radius: 6px;
	padding: 5px 8px;
	selection-background-color: #1e40af;
	selection-color: #f1f5f9;
	min-height: 20px;
}

QLineEdit:focus, QPlainTextEdit:focus, QTextEdit:focus,
QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus {
	border-color: #2563eb;
}

QLineEdit:disabled, QSpinBox:disabled, QDoubleSpinBox:disabled, QComboBox:disabled {
	background-color: #1f2836;
	color: #8b97a8;
}

QComboBox::drop-down {
	border: none;
	width: 20px;
}

QComboBox QAbstractItemView {
	background-color: #171d27;
	border: 1px solid #33405a;
	border-radius: 6px;
	padding: 4px;
	selection-background-color: #1e3a8a;
	selection-color: #f1f5f9;
}

/* ===== مجموعات ومربّعات اختيار ===== */
QGroupBox {
	border: 1px solid #2a3344;
	border-radius: 8px;
	margin-top: 14px;
	padding: 12px 10px 10px 10px;
	font-weight: 600;
	color: #f1f5f9;
}

QGroupBox::title {
	subcontrol-origin: margin;
	subcontrol-position: top start;
	left: 10px;
	padding: 0 4px;
	color: #cbd5e1;
}

QCheckBox, QRadioButton {
	spacing: 7px;
	color: #e6eaf0;
}

/* ===== جداول وشجرات ===== */
QTableView, QTableWidget, QTreeView, QTreeWidget, QListView, QListWidget {
	background-color: #171d27;
	border: 1px solid #2a3344;
	border-radius: 6px;
	gridline-color: #212a37;
	selection-background-color: #1e3a8a;
	selection-color: #f1f5f9;
	alternate-background-color: #1a212b;
}

QHeaderView::section {
	background-color: #1a212c;
	color: #cbd5e1;
	border: none;
	border-right: 1px solid #eef2f8;
	border-bottom: 1px solid #2a3344;
	padding: 7px 8px;
	font-weight: 600;
}

QTableView::item, QTableWidget::item, QTreeView::item {
	padding: 3px 5px;
}

/* ===== تبويبات ===== */
QTabWidget::pane {
	border: 1px solid #2a3344;
	border-radius: 8px;
	background-color: #171d27;
	top: -1px;
}

QTabBar::tab {
	background-color: #222c3a;
	color: #b6c0cf;
	border: 1px solid #2a3344;
	border-bottom: none;
	border-top-left-radius: 7px;
	border-top-right-radius: 7px;
	padding: 7px 16px;
	margin-right: 2px;
}

QTabBar::tab:selected {
	background-color: #171d27;
	color: #3b82f6;
	font-weight: 600;
}

QTabBar::tab:hover:!selected {
	background-color: #26313f;
}

/* ===== قوائم وأشرطة ===== */
QMenuBar {
	background-color: #171d27;
	border-bottom: 1px solid #2a3344;
	padding: 2px;
}

QMenuBar::item {
	padding: 6px 10px;
	border-radius: 5px;
	background: transparent;
}

QMenuBar::item:selected {
	background-color: #222c3a;
}

QMenu {
	background-color: #171d27;
	border: 1px solid #33405a;
	border-radius: 8px;
	padding: 6px;
}

QMenu::item {
	padding: 6px 26px;
	border-radius: 5px;
}

QMenu::item:selected {
	background-color: #1e3a8a;
	color: #f1f5f9;
}

QMenu::separator {
	height: 1px;
	background-color: #212a37;
	margin: 5px 8px;
}

QToolBar {
	background-color: #171d27;
	border: none;
	border-bottom: 1px solid #2a3344;
	spacing: 4px;
	padding: 4px;
}

QToolBar QToolButton {
	border: 1px solid transparent;
	border-radius: 6px;
	padding: 4px 8px;
}

QToolBar QToolButton:hover {
	background-color: #222c3a;
	border-color: #33405a;
}

QStatusBar {
	background-color: #171d27;
	border-top: 1px solid #2a3344;
	color: #b6c0cf;
}

/* ===== أشرطة التمرير ===== */
QScrollBar:vertical {
	background: transparent;
	width: 12px;
	margin: 0;
}

QScrollBar::handle:vertical {
	background: #3a465c;
	border-radius: 6px;
	min-height: 28px;
}

QScrollBar::handle:vertical:hover {
	background: #7d8b9e;
}

QScrollBar:horizontal {
	background: transparent;
	height: 12px;
	margin: 0;
}

QScrollBar::handle:horizontal {
	background: #3a465c;
	border-radius: 6px;
	min-width: 28px;
}

QScrollBar::handle:horizontal:hover {
	background: #7d8b9e;
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
	border: 1px solid #2a3344;
	border-radius: 6px;
	background-color: #1f2836;
	text-align: center;
	height: 18px;
}

QProgressBar::chunk {
	background-color: #2563eb;
	border-radius: 5px;
}

QToolTip {
	background-color: #e6eaf0;
	color: #171d27;
	border: none;
	border-radius: 6px;
	padding: 6px 8px;
}

QSplitter::handle {
	background-color: #212a37;
}

QSplitter::handle:horizontal {
	width: 4px;
}

QSplitter::handle:vertical {
	height: 4px;
}

/* ===== Open-Timetable: الشريط الجانبي ولوحة البداية ===== */
QWidget#otSidebar {
	background-color: #171d27;
	border: none;
	border-right: 1px solid #262f3f;
}

QLabel#otBrand {
	font-size: 13pt;
	font-weight: 700;
	color: #f1f5f9;
}

QLabel#otBrandSub {
	color: #8b97a8;
	font-size: 9pt;
}

QListWidget#otNav {
	background: transparent;
	border: none;
	padding: 4px 6px;
}

QListWidget#otNav::item {
	padding: 7px 10px;
	border-radius: 6px;
	margin: 0px 2px;
	color: #cbd5e1;
}

QListWidget#otNav::item:selected {
	background-color: #1e3a8a;
	color: #3b82f6;
	font-weight: 600;
}

QListWidget#otNav::item:hover:!selected {
	background-color: #1f2836;
}

QListWidget#otNav::item:disabled {
	color: #7f8b9c;
	background: transparent;
}


/* أزرار ترويسة الشريط الجانبي (الطي والمظهر) */
QToolButton#otThemeButton, QToolButton#otCollapseButton {
	background: transparent;
	border: 1px solid transparent;
	border-radius: 6px;
	padding: 3px;
}
QToolButton#otThemeButton:hover, QToolButton#otCollapseButton:hover {
	background-color: #222c3a;
	border-color: #2c3748;
}
QToolButton#otThemeButton:pressed, QToolButton#otCollapseButton:pressed {
	background-color: #2a3546;
}


/* لوحة الأوامر (كل الأدوات) */
QLabel#otPaletteHint {
	color: #9aa7b8;
	font-size: 9.5pt;
}

QLabel#otPaletteEmpty {
	color: #8b97a8;
	padding: 18px;
}

QLineEdit#otPaletteSearch {
	padding: 7px 10px;
	font-size: 11pt;
}

QTreeWidget#otPaletteTree {
	background-color: #171d27;
	border: 1px solid #262f3f;
	border-radius: 8px;
}

QLabel#otDashTitle {
	font-size: 20pt;
	font-weight: 700;
	color: #f1f5f9;
}

QLabel#otDashSubtitle {
	color: #b6c0cf;
	font-size: 11pt;
}

QLabel#otDashFile {
	color: #8b97a8;
	font-size: 9.5pt;
}

QLabel#otSectionTitle {
	font-weight: 600;
	color: #cbd5e1;
	font-size: 11pt;
}

QFrame#otCard {
	background-color: #171d27;
	border: 1px solid #262f3f;
	border-radius: 12px;
}

QFrame#otCard QLabel {
	background: transparent;
	border: none;
}

QLabel#otCardValue {
	font-size: 16pt;
	font-weight: 700;
}

/* ألوان أرقام البطاقات: تُختار بالمظهر، فتبقى واضحة على الخلفيتين */
QLabel#otCardValue[otAccent="blue"]   { color: #3b82f6; }
QLabel#otCardValue[otAccent="teal"]   { color: #2dd4bf; }
QLabel#otCardValue[otAccent="violet"] { color: #a78bfa; }
QLabel#otCardValue[otAccent="purple"] { color: #c084fc; }
QLabel#otCardValue[otAccent="orange"] { color: #fb923c; }
QLabel#otCardValue[otAccent="sky"]    { color: #38bdf8; }
QLabel#otCardValue[otAccent="amber"]  { color: #fbbf24; }
QLabel#otCardValue[otAccent="rose"]   { color: #fb7185; }
QLabel#otCardValue[otAccent="olive"]  { color: #a3e635; }

QLabel#otCardLabel {
	color: #9aa7b8;
	font-size: 9pt;
}

QScrollArea#otDashScroll, QWidget#otDashPage {
	background: transparent;
	border: none;
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

void UiTheme::applyTheme(QApplication& app, const QString& name)
{
	const bool dark = (name == QStringLiteral("dark"));

	// أيقونات بألوان تناسب خلفية المظهر
	UiIcons::setTone(dark ? UiIcons::OnDark : UiIcons::OnLight);

	QPalette palette;
	if (dark) {
		palette.setColor(QPalette::Window, QColor(0x14, 0x1a, 0x22));
		palette.setColor(QPalette::WindowText, QColor(0xe6, 0xeb, 0xf2));
		palette.setColor(QPalette::Base, QColor(0x1b, 0x23, 0x2e));
		palette.setColor(QPalette::AlternateBase, QColor(0x21, 0x2b, 0x38));
		palette.setColor(QPalette::Text, QColor(0xe6, 0xeb, 0xf2));
		palette.setColor(QPalette::Button, QColor(0x21, 0x2b, 0x38));
		palette.setColor(QPalette::ButtonText, QColor(0xe6, 0xeb, 0xf2));
		palette.setColor(QPalette::BrightText, QColor(0xff, 0x6b, 0x6b));
		palette.setColor(QPalette::ToolTipBase, QColor(0x2a, 0x35, 0x44));
		palette.setColor(QPalette::ToolTipText, QColor(0xf1, 0xf5, 0xf9));
		palette.setColor(QPalette::Highlight, QColor(0x3b, 0x82, 0xf6));
		palette.setColor(QPalette::HighlightedText, QColor(0xff, 0xff, 0xff));
		palette.setColor(QPalette::Disabled, QPalette::Text, QColor(0x7d, 0x8a, 0x9a));
		palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(0x7d, 0x8a, 0x9a));
		palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(0x7d, 0x8a, 0x9a));
	}
	else {
		palette.setColor(QPalette::Window, QColor(0xf7, 0xf8, 0xfa));
		palette.setColor(QPalette::WindowText, QColor(0x17, 0x20, 0x2a));
		palette.setColor(QPalette::Base, QColor(0xff, 0xff, 0xff));
		palette.setColor(QPalette::AlternateBase, QColor(0xfa, 0xfb, 0xfc));
		palette.setColor(QPalette::Text, QColor(0x17, 0x20, 0x2a));
		palette.setColor(QPalette::Button, QColor(0xff, 0xff, 0xff));
		palette.setColor(QPalette::ButtonText, QColor(0x17, 0x20, 0x2a));
		palette.setColor(QPalette::ToolTipBase, QColor(0x17, 0x20, 0x2a));
		palette.setColor(QPalette::ToolTipText, QColor(0xff, 0xff, 0xff));
		palette.setColor(QPalette::Highlight, QColor(0x25, 0x63, 0xeb));
		palette.setColor(QPalette::HighlightedText, QColor(0xff, 0xff, 0xff));
	}
	app.setPalette(palette);
	app.setStyleSheet(styleSheet(name));
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

	// 4) ورقة الأنماط: المظهر المحفوظ في الإعدادات (الوضع الداكن مدعوم).
	QString saved;
	{
		QSettings settings(QStringLiteral("fet"), QStringLiteral("fettimetabling"));
		saved = settings.value(QStringLiteral("OpenTimetable/theme"),
				       QStringLiteral("light")).toString();
	}
	if (saved != QStringLiteral("dark"))
		saved = QStringLiteral("light");

	applyTheme(app, saved);
}
