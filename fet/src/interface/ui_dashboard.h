#ifndef UI_DASHBOARD_H
#define UI_DASHBOARD_H

/*
File ui_dashboard.h — Open-Timetable addition

This file is part of Open-Timetable, a modernised user interface for FET.
FET is free software licensed under the GNU Affero General Public License
version 3. See the LICENSE file in the root of this repository.

لوحة البداية: صفحة ترحيب تعرض حالة المشروع (أعداد الأساتذة والمواد والأفواج
والقاعات والأنشطة والقيود) وأزرارًا سريعة تُنفّذ نفس إجراءات البرنامج الأصلية،
بدل البحث عنها في القوائم المتشعّبة.
*/

#include <QWidget>
#include <QString>
#include <QList>

class QMainWindow;
class QLabel;
class QAction;
class QTimer;
class QGridLayout;
class QShowEvent;
class QHideEvent;

class UiDashboard : public QWidget
{
	Q_OBJECT

public:
	explicit UiDashboard(QMainWindow* mainWindow, QWidget* parent = nullptr);
	~UiDashboard() override;

public Q_SLOTS:
	// يعيد قراءة البيانات من المحرّك ويحدّث البطاقات (آمن للنداء في أي وقت).
	void refresh();

	// يُحدّث كل النصوص الثابتة حسب لغة الواجهة الحالية (عربي/إنجليزي).
	void retranslate();

protected:
	void showEvent(QShowEvent* event) override;
	void hideEvent(QHideEvent* event) override;

private:
	// بناء الأقسام
	void buildHeader(QWidget* host);
	QWidget* buildStatsSection(QWidget* host);
	QWidget* buildStatusSection(QWidget* host);
	QWidget* buildQuickActionsSection(QWidget* host);

	// بطاقة إحصاء واحدة، ويعيد مؤشّر قيمة تُحدّثها refresh()
	QLabel* addStatCard(QGridLayout* grid, int row, int column,
			    const QString& label, const QString& accentColor);

	// نصّ زر سريع حسب لغة الواجهة (يعتمد على اسم الإجراء)
	static QString quickButtonText(const QString& actionName);

	// اسم الأيقونة المناسب لكل إجراء (UiIcons)
	static QString quickButtonIcon(const QString& actionName);

	// زر سريع مرتبط بإجراء FET بالاسم (يعود nullptr إذا لم يوجد الإجراء)
	QWidget* addQuickButton(QGridLayout* grid, int row, int column,
				const QString& actionName, const QString& text,
				const QString& tooltip);

	// نص العدد أو "—" عند غياب البيانات
	QString formatCount(int value) const;

	QMainWindow* mainWindow_;

	QLabel* titleLabel_;
	QLabel* subtitleLabel_;
	QLabel* fileNameLabel_;
	QLabel* statusLabel_;

	struct StatLabels {
		QLabel* teachers;
		QLabel* subjects;
		QLabel* years;
		QLabel* groups;
		QLabel* subgroups;
		QLabel* rooms;
		QLabel* activities;
		QLabel* timeConstraints;
		QLabel* spaceConstraints;
	};
	StatLabels stats_;

	QList<QWidget*> quickButtons_;
	QList<QLabel*> cardCaptions_;      // نصوص بطاقات الإحصاء
	QList<QLabel*> sectionTitles_;     // عناوين الأقسام
	QList<QAction*> quickActions_;     // إجراءات الأزرار السريعة (لبناء النص)
	QTimer* refreshTimer_;
};

#endif // UI_DASHBOARD_H
