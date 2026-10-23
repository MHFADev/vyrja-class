/*
 *  CommandPalette.h - quick searchable launcher for all features (Ctrl+K)
 *
 *  This file is part of Vyrja Classroom, a fork of Veyon (https://veyon.io).
 *  Licensed under GPL-2.0-or-later.
 */

#pragma once

#include <QDialog>

#include "Feature.h"

class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;

class MainWindow;

class CommandPalette : public QDialog
{
	Q_OBJECT
public:
	explicit CommandPalette(MainWindow* mainWindow);

	// (re)builds the list and shows the palette centered over the main window
	void open() override;

protected:
	bool eventFilter(QObject* object, QEvent* event) override;

private:
	struct Entry
	{
		Feature feature;
		Feature parent;
		bool isSubFeature;
		QString title;
		QString searchText;
		int sceneIndex = -1;
	};

	void rebuildEntries();
	void applyFilter(const QString& filter);
	void updateTargetLabel();
	void activateItem(QListWidgetItem* item);

	MainWindow* m_mainWindow;
	QLineEdit* m_searchEdit;
	QListWidget* m_list;
	QLabel* m_targetLabel;
	QList<Entry> m_entries;
};
