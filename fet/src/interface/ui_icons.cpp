/*
File ui_icons.cpp — Open-Timetable addition

This file is part of Open-Timetable, a modernised user interface for the
timetabling engine. Licensed under the GNU Affero General Public License v3.

رسم الأيقونات:
كل أيقونة تُوصف بقائمة بسيطة من الأشكال داخل شبكة 24×24 (خط، مستطيل مستدير،
دائرة، قوس، نقطة مطموسة)، ثم يرسمها محرّك واحد بسماكة خط موحّدة (1.7) ونهايات
مستديرة. النتيجة: مجموعة متناسقة تشبه أيقونات الأنظمة الحديثة، وتُعاد رسمها بأي
حجم مطلوب (بما في ذلك شاشات 2× و3×) دون تشويش.
*/

#include "ui_icons.h"

#include <QColor>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QVector>
#include <QtMath>

namespace {

// أنواع الأشكال
enum PrimType {
	P_LINE = 0,   // (x1,y1) → (x2,y2)
	P_RECT,       // (x,y,w,h, r)
	P_CIRCLE,     // (cx,cy,r)
	P_ARC,        // (x,y,w,h, start, span)
	P_DOT,        // نقطة مطموسة بلون الخلفية
	P_MOON        // هلال: (cx,cy,r) ناقص دائرة (a2,b2,r2)
};

struct Prim {
	int type;
	qreal a, b, c, d, e, f;
};

Prim L(qreal x1, qreal y1, qreal x2, qreal y2)
{
	return Prim{ P_LINE, x1, y1, x2, y2, 0, 0 };
}

Prim R(qreal x, qreal y, qreal w, qreal h, qreal radius = 2.0)
{
	return Prim{ P_RECT, x, y, w, h, radius, 0 };
}

Prim C(qreal cx, qreal cy, qreal r)
{
	return Prim{ P_CIRCLE, cx, cy, r, 0, 0, 0 };
}

Prim A(qreal x, qreal y, qreal w, qreal h, qreal start, qreal span)
{
	return Prim{ P_ARC, x, y, w, h, start, span };
}

Prim D(qreal cx, qreal cy, qreal r)
{
	return Prim{ P_DOT, cx, cy, r, 0, 0, 0 };
}

// هلال حقيقي (طرح دائرتين) — لا يعتمد على لون الخلفية فيبقى صحيحًا في كل مظهر
Prim M(qreal cx, qreal cy, qreal r, qreal sx, qreal sy, qreal sr)
{
	return Prim{ P_MOON, cx, cy, r, sx, sy, sr };
}

UiIcons::Tone g_tone = UiIcons::OnLight;

QVector<Prim> geometry(const QString& name)
{
	QVector<Prim> p;

	if (name == QLatin1String("home")) {
		p << L(3, 11, 12, 4) << L(12, 4, 21, 11)
		  << R(5, 10.5, 14, 10.5, 2) << R(10, 15, 4, 6, 1);
	}
	else if (name == QLatin1String("teachers")) {
		p << C(12, 8, 3.4) << A(4.5, 12.5, 15, 11.5, 0, 180);
	}
	else if (name == QLatin1String("subjects")) {
		p << R(4, 3.5, 16, 17, 2) << L(12, 3.5, 12, 20.5)
		  << L(6.5, 8, 9.5, 8) << L(14.5, 8, 17.5, 8);
	}
	else if (name == QLatin1String("years")) {
		p << L(2.5, 8.5, 12, 4) << L(12, 4, 21.5, 8.5)
		  << L(21.5, 8.5, 12, 13) << L(12, 13, 2.5, 8.5)
		  << L(7, 11, 7, 16.5) << L(17, 11, 17, 16.5)
		  << L(7, 16.5, 12, 19) << L(17, 16.5, 12, 19);
	}
	else if (name == QLatin1String("subgroups")) {
		p << C(8.5, 8, 3) << A(2, 12, 13, 11, 0, 180)
		  << C(16.5, 9.5, 2.4) << A(12.5, 13, 10.5, 9, 0, 180);
	}
	else if (name == QLatin1String("rooms")) {
		p << R(4, 3.5, 16, 17, 2) << C(15.5, 12, 1.1);
	}
	else if (name == QLatin1String("buildings")) {
		p << R(3.5, 9, 7, 12, 1) << R(13, 3.5, 7.5, 17.5, 1)
		  << L(6, 12.5, 8, 12.5) << L(6, 16, 8, 16)
		  << L(15.5, 7.5, 18, 7.5) << L(15.5, 11.5, 18, 11.5)
		  << L(15.5, 15.5, 18, 15.5);
	}
	else if (name == QLatin1String("activities")) {
		p << R(3, 5, 18, 16, 2) << L(3, 10, 21, 10)
		  << L(8, 3, 8, 7.5) << L(16, 3, 16, 7.5)
		  << L(8.5, 14.5, 11, 17) << L(11, 17, 15.5, 12.5);
	}
	else if (name == QLatin1String("constraints")) {
		p << L(3, 7.5, 21, 7.5) << C(9, 7.5, 2.2)
		  << L(3, 16.5, 21, 16.5) << C(15, 16.5, 2.2);
	}
	else if (name == QLatin1String("space")) {
		p << R(3, 3, 7.5, 7.5, 1.5) << R(13.5, 3, 7.5, 7.5, 1.5)
		  << R(3, 13.5, 7.5, 7.5, 1.5) << R(13.5, 13.5, 7.5, 7.5, 1.5);
	}
	else if (name == QLatin1String("templates")) {
		p << L(3.5, 20.5, 13.5, 10.5) << L(11, 8, 16, 13)
		  << C(18.5, 5.5, 1.8) << C(20.5, 13, 1.3);
	}
	else if (name == QLatin1String("generate")) {
		p << C(12, 12, 8.5)
		  << L(10, 8, 16, 12) << L(16, 12, 10, 16) << L(10, 16, 10, 8);
	}
	else if (name == QLatin1String("view-teachers")) {
		p << C(9, 8.5, 3) << A(3.5, 13, 12, 10, 0, 180)
		  << R(15, 4.5, 6.5, 6.5, 1.2) << R(15, 13, 6.5, 6.5, 1.2);
	}
	else if (name == QLatin1String("view-students")) {
		p << R(3, 4, 18, 16, 2)
		  << L(9, 4, 9, 20) << L(15, 4, 15, 20)
		  << L(3, 9.5, 21, 9.5) << L(3, 15, 21, 15);
	}
	else if (name == QLatin1String("view-rooms")) {
		p << R(3, 4, 18, 16, 2) << L(3, 9, 21, 9)
		  << C(8, 14.5, 1.2) << C(12, 14.5, 1.2) << C(16, 14.5, 1.2);
	}
	else if (name == QLatin1String("conflicts")) {
		p << L(12, 3.5, 21, 20) << L(21, 20, 3, 20) << L(3, 20, 12, 3.5)
		  << L(12, 9.5, 12, 14) << D(12, 16.6, 1.15);
	}
	else if (name == QLatin1String("publish")) {
		p << L(12, 15.5, 12, 3.5) << L(7, 8.5, 12, 3.5) << L(17, 8.5, 12, 3.5)
		  << L(4.5, 14, 4.5, 20.5) << L(19.5, 14, 19.5, 20.5)
		  << L(4.5, 20.5, 19.5, 20.5);
	}
	else if (name == QLatin1String("sidebar")) {
		// سهم يشير إلى الداخل (طي)
		p << R(3, 4, 18, 16, 2) << L(9, 4, 9, 20)
		  << L(15.5, 9, 12.8, 12) << L(12.8, 12, 15.5, 15);
	}
	else if (name == QLatin1String("sidebar-open")) {
		// سهم يشير إلى الخارج (فتح)
		p << R(3, 4, 18, 16, 2) << L(9, 4, 9, 20)
		  << L(12.8, 9, 15.5, 12) << L(15.5, 12, 12.8, 15);
	}
	else if (name == QLatin1String("commands")) {
		// عدسة بحث + سهم أمر داخل الإطار
		p << C(10.5, 10.5, 6.0) << L(15.0, 15.0, 20.5, 20.5)
		  << L(8.0, 8.8, 10.0, 10.9) << L(10.0, 10.9, 12.6, 8.2);
	}
	else if (name == QLatin1String("moon")) {
		p << M(11.4, 12.0, 7.0, 15.6, 9.2, 6.0);
	}
	else if (name == QLatin1String("sun")) {
		p << C(12, 12, 4)
		  << L(12, 2.6, 12, 5) << L(12, 19, 12, 21.4)
		  << L(2.6, 12, 5, 12) << L(19, 12, 21.4, 12)
		  << L(5.2, 5.2, 7, 7) << L(17, 17, 18.8, 18.8)
		  << L(18.8, 5.2, 17, 7) << L(7, 17, 5.2, 18.8);
	}
	else {
		// احتياطي: دائرة ونقطة (لا تُترك الأيقونة فارغة أبدًا)
		p << C(12, 12, 8) << D(12, 12, 1.6);
	}

	return p;
}

QColor strokeColor()
{
	return g_tone == UiIcons::OnDark ? QColor(0xe2e8f0) : QColor(0x334155);
}

QPixmap render(const QString& name, int size)
{
	QPixmap pixmap(size, size);
	pixmap.fill(Qt::transparent);

	QPainter painter(&pixmap);
	painter.setRenderHint(QPainter::Antialiasing, true);

	const qreal scale = qreal(size) / 24.0;
	painter.scale(scale, scale);

	QPen pen(strokeColor());
	pen.setWidthF(1.7);
	pen.setCapStyle(Qt::RoundCap);
	pen.setJoinStyle(Qt::RoundJoin);
	painter.setPen(pen);
	painter.setBrush(Qt::NoBrush);

	const QVector<Prim> prims = geometry(name);
	for (const Prim& prim : prims) {
		switch (prim.type) {
		case P_LINE:
			painter.drawLine(QPointF(prim.a, prim.b), QPointF(prim.c, prim.d));
			break;
		case P_RECT: {
			QPainterPath path;
			path.addRoundedRect(QRectF(prim.a, prim.b, prim.c, prim.d), prim.e, prim.e);
			painter.drawPath(path);
			break;
		}
		case P_CIRCLE:
			painter.drawEllipse(QPointF(prim.a, prim.b), prim.c, prim.c);
			break;
		case P_ARC: {
			// Qt يقيس الزوايا بستّ عشر من الدرجة، عكس اتجاه الساعة
			painter.drawArc(QRectF(prim.a, prim.b, prim.c, prim.d),
					int(prim.e * 16), int(prim.f * 16));
			break;
		}
		case P_MOON: {
			QPainterPath moon;
			moon.addEllipse(QPointF(prim.a, prim.b), prim.c, prim.c);
			QPainterPath bite;
			bite.addEllipse(QPointF(prim.d, prim.e), prim.f, prim.f);
			painter.save();
			painter.setPen(Qt::NoPen);
			painter.setBrush(strokeColor());
			painter.drawPath(moon.subtracted(bite));
			painter.restore();
			break;
		}
		case P_DOT:
			painter.save();
			painter.setPen(Qt::NoPen);
			painter.setBrush(g_tone == UiIcons::OnDark ? QColor(0x1b232e)
								   : QColor(0xf7f8fa));
			painter.drawEllipse(QPointF(prim.a, prim.b), prim.c, prim.c);
			painter.restore();
			break;
		default:
			break;
		}
	}

	painter.end();
	return pixmap;
}

// شعار البرنامج: مربّع مستدير بتدرّج أزرق/بنفسجي + أربعة مربّعات تمثّل خلايا
// جدول زمني (الخلية الرابعة بلون مميّز). بسيط ومقروء حتى في 16×16.
QPixmap renderLogo(int size)
{
	QPixmap pixmap(size, size);
	pixmap.fill(Qt::transparent);

	QPainter painter(&pixmap);
	painter.setRenderHint(QPainter::Antialiasing, true);

	const qreal scale = qreal(size) / 64.0;
	painter.scale(scale, scale);

	// الخلفية بتدرّج قطري
	QLinearGradient gradient(2, 2, 62, 62);
	gradient.setColorAt(0.0, QColor(0x4f, 0x46, 0xe5));
	gradient.setColorAt(1.0, QColor(0x25, 0x63, 0xeb));

	QPainterPath background;
	background.addRoundedRect(QRectF(2, 2, 60, 60), 15, 15);
	painter.fillPath(background, gradient);

	painter.setPen(Qt::NoPen);
	painter.setBrush(QColor(0xff, 0xff, 0xff));
	painter.drawRoundedRect(QRectF(14, 14, 11, 11), 2.5, 2.5);  // أعلى يسار
	painter.drawRoundedRect(QRectF(28, 14, 22, 11), 2.5, 2.5);  // أعلى يمين
	painter.drawRoundedRect(QRectF(14, 28, 11, 22), 2.5, 2.5);  // أسفل يسار

	painter.setBrush(QColor(0xc7, 0xd2, 0xfe));
	painter.drawRoundedRect(QRectF(28, 28, 22, 22), 3.0, 3.0);  // الخلية المميّزة

	painter.end();
	return pixmap;
}

} // namespace

namespace UiIcons {

void setTone(Tone tone)
{
	g_tone = tone;
}

Tone tone()
{
	return g_tone;
}

QIcon icon(const QString& name, int size)
{
	QIcon result;
	if (size <= 0)
		return result;

	result.addPixmap(render(name, size), QIcon::Normal, QIcon::Off);
	result.addPixmap(render(name, size * 2), QIcon::Normal, QIcon::Off);

	// حالة التعطيل: نفس المسار بلون أسطع قليلًا
	const Tone old = g_tone;
	setTone(g_tone == OnDark ? OnDark : OnLight);
	result.addPixmap(render(name, size), QIcon::Disabled, QIcon::Off);
	setTone(old);

	return result;
}

QIcon logo(int size)
{
	QIcon result;
	result.addPixmap(renderLogo(size), QIcon::Normal, QIcon::Off);
	result.addPixmap(renderLogo(size * 2), QIcon::Normal, QIcon::Off);
	return result;
}

QStringList names()
{
	return QStringList{ QStringLiteral("home"),     QStringLiteral("teachers"),
			   QStringLiteral("subjects"),  QStringLiteral("years"),
			   QStringLiteral("subgroups"), QStringLiteral("rooms"),
			   QStringLiteral("buildings"), QStringLiteral("activities"),
			   QStringLiteral("constraints"), QStringLiteral("space"),
			   QStringLiteral("templates"), QStringLiteral("generate"),
			   QStringLiteral("view-teachers"), QStringLiteral("view-students"),
			   QStringLiteral("view-rooms"), QStringLiteral("conflicts"),
			   QStringLiteral("publish"),   QStringLiteral("sidebar"),
			   QStringLiteral("moon"),      QStringLiteral("sun"),
			   QStringLiteral("sidebar-open"), QStringLiteral("commands") };
}

} // namespace UiIcons
