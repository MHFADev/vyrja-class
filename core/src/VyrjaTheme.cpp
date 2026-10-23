/*
 *  VyrjaTheme.cpp - application-wide stylesheet using the Vyrja brand colors
 *
 *  This file is part of Vyrja Classroom, a fork of Veyon (https://veyon.io).
 *  Licensed under GPL-2.0-or-later.
 */

#include <QApplication>
#include <QFile>
#include <QHash>
#include <QPalette>
#include <QToolTip>

#include "Branding.h"
#include "VyrjaTheme.h"


QString VyrjaTheme::styleSheet(bool darkMode)
{
	QFile file(QStringLiteral(":/core/vyrja-theme.qss"));
	if (file.open(QFile::ReadOnly) == false)
	{
		return {};
	}

	auto css = QString::fromUtf8(file.readAll());

	const QHash<QString, QString> light{
		{ QStringLiteral("@BG@"),          Branding::Cream.name() },
		{ QStringLiteral("@SURFACE@"),     QStringLiteral("#FFFFFF") },
		{ QStringLiteral("@SURFACE_ALT@"), QStringLiteral("#F1EFFA") },
		{ QStringLiteral("@BORDER@"),      QStringLiteral("#DAD7EE") },
		{ QStringLiteral("@TEXT@"),        QStringLiteral("#1E1B4B") },
		{ QStringLiteral("@TEXT_MUTED@"),  QStringLiteral("#6B678F") },
		{ QStringLiteral("@PRIMARY@"),     Branding::Navy.name() },
		{ QStringLiteral("@PRIMARY_HOVER@"), Branding::NavySoft.name() },
		{ QStringLiteral("@ON_PRIMARY@"),  QStringLiteral("#FFFFFF") },
		{ QStringLiteral("@ACCENT@"),      Branding::Orange.name() },
		{ QStringLiteral("@ACCENT_SOFT@"), Branding::OrangeSoft.name() },
		{ QStringLiteral("@ACCENT_STRONG@"), Branding::OrangeStrong.name() },
		{ QStringLiteral("@ON_ACCENT@"),   Branding::Navy.name() },
		{ QStringLiteral("@CONTROL_BORDER@"), QStringLiteral("#8582AC") },
		{ QStringLiteral("@HEADER@"),      Branding::Navy.name() },
		{ QStringLiteral("@HEADING@"),     Branding::Navy.name() },
		{ QStringLiteral("@ERROR@"),       QStringLiteral("#B3261E") },
		{ QStringLiteral("@ONLINE@"),      Branding::StatusOnline.name() },
		{ QStringLiteral("@OFFLINE@"),     Branding::StatusOffline.name() },
		{ QStringLiteral("@PROBLEM@"),     Branding::StatusProblem.name() },
		{ QStringLiteral("@HOVER@"),       QStringLiteral("#ECE9FA") },
		{ QStringLiteral("@DISABLED@"),    QStringLiteral("#A9A6C4") }
	};

	const QHash<QString, QString> dark{
		{ QStringLiteral("@BG@"),          QStringLiteral("#141238") },
		{ QStringLiteral("@SURFACE@"),     QStringLiteral("#1D1B52") },
		{ QStringLiteral("@SURFACE_ALT@"), QStringLiteral("#25226A") },
		{ QStringLiteral("@BORDER@"),      QStringLiteral("#38348A") },
		{ QStringLiteral("@TEXT@"),        QStringLiteral("#ECEBFA") },
		{ QStringLiteral("@TEXT_MUTED@"),  QStringLiteral("#A5A2D0") },
		{ QStringLiteral("@PRIMARY@"),     QStringLiteral("#5B56D6") },
		{ QStringLiteral("@PRIMARY_HOVER@"), QStringLiteral("#7470E6") },
		{ QStringLiteral("@ON_PRIMARY@"),  QStringLiteral("#FFFFFF") },
		{ QStringLiteral("@ACCENT@"),      Branding::Orange.name() },
		{ QStringLiteral("@ACCENT_SOFT@"), QStringLiteral("#4A2F5A") },
		{ QStringLiteral("@ACCENT_STRONG@"), Branding::OrangeStrong.name() },
		{ QStringLiteral("@ON_ACCENT@"),   Branding::Navy.name() },
		{ QStringLiteral("@CONTROL_BORDER@"), QStringLiteral("#7773C4") },
		{ QStringLiteral("@HEADER@"),      Branding::Navy.name() },
		{ QStringLiteral("@HEADING@"),     QStringLiteral("#CFCDF7") },
		{ QStringLiteral("@ERROR@"),       QStringLiteral("#FF9C8A") },
		{ QStringLiteral("@ONLINE@"),      Branding::StatusOnline.name() },
		{ QStringLiteral("@OFFLINE@"),     Branding::StatusOffline.name() },
		{ QStringLiteral("@PROBLEM@"),     Branding::StatusProblem.name() },
		{ QStringLiteral("@HOVER@"),       QStringLiteral("#2E2B7A") },
		{ QStringLiteral("@DISABLED@"),    QStringLiteral("#6663A0") }
	};

	css.replace(QStringLiteral("@ICON_SUFFIX@"), darkMode ? QStringLiteral("-dark") : QString());

	const auto& tokens = darkMode ? dark : light;
	for (auto it = tokens.constBegin(); it != tokens.constEnd(); ++it)
	{
		css.replace(it.key(), it.value());
	}

	return css;
}



QPixmap VyrjaTheme::logo(Logo kind, int height, qreal devicePixelRatio)
{
	QPixmap source(kind == Logo::Mark ? QStringLiteral(":/core/vyrja-mark.png")
									  : QStringLiteral(":/core/vyrja-logo-wide.png"));
	if (source.isNull() || height <= 0 || devicePixelRatio <= 0)
	{
		return source;
	}

	auto scaled = source.scaledToHeight(qRound(height * devicePixelRatio), Qt::SmoothTransformation);
	scaled.setDevicePixelRatio(devicePixelRatio);
	return scaled;
}



void VyrjaTheme::apply(QApplication* app, bool darkMode)
{
	if (app == nullptr)
	{
		return;
	}

	app->setStyleSheet(styleSheet(darkMode));

	// base palette so that widgets not covered by the stylesheet (menu bar,
	// scroll area backgrounds, ...) also follow the brand colors
	auto palette = app->palette();
	if (darkMode)
	{
		palette.setColor(QPalette::Window, QColor(0x14, 0x12, 0x38));
		palette.setColor(QPalette::Base, QColor(0x1D, 0x1B, 0x52));
		palette.setColor(QPalette::AlternateBase, QColor(0x25, 0x22, 0x6A));
		palette.setColor(QPalette::Button, QColor(0x1D, 0x1B, 0x52));
		palette.setColor(QPalette::WindowText, QColor(0xEC, 0xEB, 0xFA));
		palette.setColor(QPalette::Text, QColor(0xEC, 0xEB, 0xFA));
		palette.setColor(QPalette::ButtonText, QColor(0xEC, 0xEB, 0xFA));
	}
	else
	{
		palette.setColor(QPalette::Window, Branding::Cream);
		palette.setColor(QPalette::Base, Qt::white);
		palette.setColor(QPalette::AlternateBase, QColor(0xF1, 0xEF, 0xFA));
		palette.setColor(QPalette::Button, Qt::white);
		palette.setColor(QPalette::WindowText, QColor(0x1E, 0x1B, 0x4B));
		palette.setColor(QPalette::Text, QColor(0x1E, 0x1B, 0x4B));
		palette.setColor(QPalette::ButtonText, QColor(0x1E, 0x1B, 0x4B));
	}
	// navy text on the orange selection: 4.9:1, white would only reach 2.8:1
	palette.setColor(QPalette::Highlight, Branding::Orange);
	palette.setColor(QPalette::HighlightedText, Branding::Navy);
	app->setPalette(palette);

	auto toolTipPalette = QToolTip::palette();
	toolTipPalette.setColor(QPalette::Window, Branding::Navy);
	toolTipPalette.setColor(QPalette::ToolTipBase, Branding::Navy);
	toolTipPalette.setColor(QPalette::ToolTipText, Qt::white);
	QToolTip::setPalette(toolTipPalette);
}
