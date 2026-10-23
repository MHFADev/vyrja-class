/*
 *  VyrjaTheme.h - application-wide stylesheet using the Vyrja brand colors
 *
 *  This file is part of Vyrja Classroom, a fork of Veyon (https://veyon.io).
 *  Licensed under GPL-2.0-or-later.
 */

#pragma once

#include <QPixmap>
#include <QString>

#include "VeyonCore.h"

class QApplication;

class VEYON_CORE_EXPORT VyrjaTheme
{
public:
	// the two forms of the Vyrja Studio logo: the cat-and-V mark, and the full wordmark
	enum class Logo
	{
		Mark,
		Wordmark
	};

	// installs the stylesheet (light or dark variant) and tooltip palette
	static void apply(QApplication* app, bool darkMode);

	// returns the generated stylesheet, useful for tests
	static QString styleSheet(bool darkMode);

	// the logo scaled to a height in device independent pixels, rendered at the
	// given device pixel ratio so that it stays sharp on high-DPI screens
	static QPixmap logo(Logo kind, int height, qreal devicePixelRatio = 1.0);
};
