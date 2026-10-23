/*
 *  Branding.h - central place for product branding (name, colors)
 *
 *  This file is part of Vyrja Classroom, a fork of Veyon (https://veyon.io).
 *  Licensed under GPL-2.0-or-later, same as upstream.
 *
 *  Change the constants here to re-brand the user interface.
 */

#pragma once

#include <QColor>
#include <QString>

namespace Branding
{
// product names shown to users
static inline const QString ProductName = QStringLiteral("Vyrja Classroom");
static inline const QString MasterName = QStringLiteral("Vyrja Classroom");
static inline const QString ConfiguratorName = QStringLiteral("Vyrja Setup");
static inline const QString ServiceName = QStringLiteral("Vyrja Service");
static inline const QString Studio = QStringLiteral("Vyrja Studio");
static inline const QString UpstreamCredit =
	QStringLiteral("Based on Veyon (https://veyon.io), (c) 2004-2026 Tobias Junghans and contributors, GPL-2.0");

// brand colors, taken from the Vyrja Studio logo
static inline const QColor Navy{0x26, 0x23, 0x6B};
static inline const QColor NavyDeep{0x1B, 0x19, 0x52};
static inline const QColor NavySoft{0x3A, 0x36, 0x8F};
static inline const QColor Orange{0xFF, 0x6B, 0x4A};
static inline const QColor OrangeSoft{0xFF, 0xE6, 0xDF};
static inline const QColor Cream{0xFA, 0xF9, 0xFD};

// the same orange, deepened for strokes and small indicators that sit on light
// surfaces (3.7:1 on white, the brand orange itself is 2.8:1)
static inline const QColor OrangeStrong{0xE5, 0x52, 0x2E};

// computer states, always paired with a shape (dot / ring / square) so that they
// never depend on color alone. Each is at least 3:1 on white, on the navy header
// and on the dark theme surfaces
static inline const QColor StatusOnline{0x2E, 0x9E, 0x5B};
static inline const QColor StatusOffline{0x85, 0x82, 0xAA};
static inline const QColor StatusProblem = OrangeStrong;
}
