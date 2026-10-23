/*
 * MainWindow.h - main window of Veyon Master Application
 *
 * Copyright (c) 2004-2026 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#pragma once

#include <QMainWindow>

#include "ComputerControlInterface.h"

class QButtonGroup;
class QLabel;
class QTimer;
class QToolButton;

class ClassScenes;
class CommandPalette;
class QAbstractItemView;

class VeyonMaster;

namespace Ui {
class MainWindow;
}


class MainWindow : public QMainWindow
{
	Q_OBJECT
public:
	explicit MainWindow( VeyonMaster& masterCore, QWidget* parent = nullptr );
	~MainWindow() override;

	static bool initAuthentication();
	static bool initAccessControl();

	QAbstractItemView* computerView();

	ClassScenes* classScenes() const
	{
		return m_classScenes;
	}

	VeyonMaster& masterCore()
	{
		return m_master;
	}

	void reloadSubFeatures();

	ComputerControlInterfaceList selectedComputerControlInterfaces() const;

protected:
	void closeEvent( QCloseEvent* event ) override;
	bool eventFilter( QObject* object, QEvent* event ) override;
	void keyPressEvent( QKeyEvent *e ) override;


private Q_SLOTS:
	void showAboutDialog();

private:
	static int buttonId(const Feature& feature) // clazy:exclude=qt6-qhash-signature
	{
		return int(qHash(feature.uid()));
	}

	static constexpr const char* originalSizePropertyName()
	{
		return "originalSize";
	}

	void addFeaturesToToolBar();
	void addSubFeaturesToToolButton( QToolButton* button, const Feature& parentFeature );

	void updateModeButtonGroup();
	void updateSummary();

	void loadComputerPositions();
	void saveComputerPositions();

	Ui::MainWindow* ui;

	VeyonMaster& m_master;

	QButtonGroup* m_modeGroup;

	QLabel* m_summaryLabel;
	QWidget* m_emptyHint = nullptr;
	QLabel* m_onlineValue = nullptr;
	QLabel* m_offlineValue = nullptr;
	QLabel* m_problemValue = nullptr;
	QWidget* m_problemChip = nullptr;
	QLabel* m_modeChip = nullptr;
	QLabel* m_timerChip = nullptr;
	QLabel* m_clockChip = nullptr;
	QTimer* m_summaryTimer;
	CommandPalette* m_commandPalette;
	ClassScenes* m_classScenes;

} ;
