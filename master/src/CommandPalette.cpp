/*
 *  CommandPalette.cpp - quick searchable launcher for all features (Ctrl+K)
 *
 *  This file is part of Vyrja Classroom, a fork of Veyon (https://veyon.io).
 *  Licensed under GPL-2.0-or-later.
 */

#include <QAction>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>

#include "ClassScenes.h"
#include "CommandPalette.h"
#include "MainWindow.h"
#include "VeyonMaster.h"


CommandPalette::CommandPalette(MainWindow* mainWindow) :
	QDialog(mainWindow, Qt::Dialog | Qt::FramelessWindowHint),
	m_mainWindow(mainWindow),
	m_searchEdit(new QLineEdit(this)),
	m_list(new QListWidget(this)),
	m_targetLabel(new QLabel(this))
{
	setModal(true);
	setObjectName(QStringLiteral("CommandPalette"));
	resize(560, 420);

	m_searchEdit->setPlaceholderText(tr("Type what you want to do, e.g. \"lock\", \"message\" or \"screenshot\" …"));
	m_searchEdit->setClearButtonEnabled(true);
	m_searchEdit->installEventFilter(this);

	m_list->setIconSize(QSize(24, 24));
	m_list->setUniformItemSizes(true);
	m_list->setSpacing(1);

	m_targetLabel->setObjectName(QStringLiteral("CommandTarget"));

	auto layout = new QVBoxLayout(this);
	layout->setContentsMargins(12, 12, 12, 10);
	layout->setSpacing(8);
	layout->addWidget(m_searchEdit);
	layout->addWidget(m_list, 1);
	layout->addWidget(m_targetLabel);

	connect(m_searchEdit, &QLineEdit::textChanged, this, &CommandPalette::applyFilter);
	connect(m_list, &QListWidget::itemActivated, this, &CommandPalette::activateItem);
	connect(m_list, &QListWidget::itemClicked, this, &CommandPalette::activateItem);
}



void CommandPalette::open()
{
	rebuildEntries();

	m_searchEdit->clear();
	applyFilter({});
	updateTargetLabel();

	move(m_mainWindow->geometry().center() - QPoint(width() / 2, height() / 2 + 60));

	QDialog::open();
	m_searchEdit->setFocus();
}



bool CommandPalette::eventFilter(QObject* object, QEvent* event)
{
	if (object == m_searchEdit && event->type() == QEvent::KeyPress)
	{
		const auto key = static_cast<QKeyEvent *>(event)->key();
		switch (key)
		{
		case Qt::Key_Down:
		case Qt::Key_Up:
		{
			const auto count = m_list->count();
			if (count > 0)
			{
				const auto step = key == Qt::Key_Down ? 1 : -1;
				m_list->setCurrentRow((m_list->currentRow() + step + count) % count);
			}
			return true;
		}
		case Qt::Key_Return:
		case Qt::Key_Enter:
			if (m_list->currentItem())
			{
				activateItem(m_list->currentItem());
			}
			return true;
		default:
			break;
		}
	}

	return QDialog::eventFilter(object, event);
}



void CommandPalette::rebuildEntries()
{
	m_entries.clear();

	auto& master = m_mainWindow->masterCore();

	for (const auto& feature : master.features())
	{
		if (feature.testFlag(Feature::Flag::Meta))
		{
			continue;
		}

		m_entries.append({feature, feature, false, feature.displayName(),
						  (feature.displayName() + QLatin1Char(' ') + feature.description()).toLower()});

		for (const auto& subFeature : master.subFeatures(feature.uid()))
		{
			m_entries.append({subFeature, feature, true,
							  feature.displayName() + QStringLiteral("  ›  ") + subFeature.displayName(),
							  (feature.displayName() + QLatin1Char(' ') + subFeature.displayName() +
							   QLatin1Char(' ') + subFeature.description()).toLower()});
		}
	}

	const auto& scenes = m_mainWindow->classScenes()->scenes();
	for (int i = 0; i < scenes.size(); ++i)
	{
		const auto& scene = scenes.at(i);
		Entry entry{Feature{Feature::Uid{}}, Feature{Feature::Uid{}}, false, scene.group + QStringLiteral("  ›  ") + scene.title,
					(scene.group + QLatin1Char(' ') + scene.title + QLatin1Char(' ') + scene.description).toLower()};
		entry.sceneIndex = i;
		m_entries.append(entry);
	}
}



void CommandPalette::applyFilter(const QString& filter)
{
	m_list->clear();

	const auto words = filter.toLower().split(QLatin1Char(' '), Qt::SkipEmptyParts);

	QList<QPair<int, int>> matches; // score, entry index

	for (int i = 0; i < m_entries.size(); ++i)
	{
		const auto& entry = m_entries.at(i);
		const auto title = entry.title.toLower();

		int score = 0;
		bool matchesAll = true;
		for (const auto& word : words)
		{
			if (title.contains(word))
			{
				score += title.startsWith(word) ? 3 : 2;
			}
			else if (entry.searchText.contains(word))
			{
				score += 1;
			}
			else
			{
				matchesAll = false;
				break;
			}
		}

		if (matchesAll)
		{
			matches.append({-score, i}); // negative score => best first when sorted ascending
		}
	}

	std::stable_sort(matches.begin(), matches.end(), [](const QPair<int, int>& a, const QPair<int, int>& b) {
		return a.first < b.first;
	});

	for (const auto& match : std::as_const(matches))
	{
		const auto& entry = m_entries.at(match.second);
		auto item = new QListWidgetItem(entry.sceneIndex >= 0 ? QIcon(QStringLiteral(":/core/icon32.png")) : QIcon(entry.feature.iconUrl()),
										entry.title, m_list);
		item->setToolTip(entry.sceneIndex >= 0 ? m_mainWindow->classScenes()->scenes().at(entry.sceneIndex).description :
												 entry.feature.description());
		item->setData(Qt::UserRole, match.second);
	}

	if (m_list->count() > 0)
	{
		m_list->setCurrentRow(0);
	}
}



void CommandPalette::updateTargetLabel()
{
	const auto selected = m_mainWindow->selectedComputerControlInterfaces().size();

	m_targetLabel->setText(selected > 0 ?
							   tr("Applies to the %1 selected computer(s)").arg(selected) :
							   tr("No computer selected – applies to all visible computers"));
}



void CommandPalette::activateItem(QListWidgetItem* item)
{
	if (item == nullptr)
	{
		return;
	}

	const auto entry = m_entries.value(item->data(Qt::UserRole).toInt());

	close();

	if (entry.sceneIndex >= 0)
	{
		m_mainWindow->classScenes()->scenes().at(entry.sceneIndex).run();
		return;
	}

	// trigger the real toolbar controls so that button states stay in sync
	auto toolBar = m_mainWindow->findChild<QToolBar *>();
	auto button = toolBar ? toolBar->findChild<QToolButton *>(entry.parent.name()) : nullptr;

	if (button == nullptr)
	{
		m_mainWindow->masterCore().runFeature(entry.feature);
		return;
	}

	if (entry.isSubFeature == false)
	{
		button->click();
		return;
	}

	if (button->menu())
	{
		if (auto action = button->menu()->findChild<QAction *>(entry.feature.uid().toString()))
		{
			action->trigger();
			return;
		}
	}

	m_mainWindow->masterCore().runFeature(entry.feature);
}
