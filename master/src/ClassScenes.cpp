/*
 *  ClassScenes.cpp - one-click classroom actions ("Kelas Cepat")
 *
 *  This file is part of Vyrja Classroom, a fork of Veyon (https://veyon.io).
 *  Licensed under GPL-2.0-or-later.
 */

#include <QAbstractItemView>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QInputDialog>
#include <QItemSelectionModel>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardPaths>
#include <QTextStream>
#include <QTimer>
#include <QToolButton>
#include <QUrl>

#include "BuiltinFeatures.h"
#include "ClassScenes.h"
#include "ComputerControlInterface.h"
#include "ComputerControlListModel.h"
#include "FeatureManager.h"
#include "MainWindow.h"
#include "MonitoringMode.h"
#include "VeyonCore.h"
#include "VeyonMaster.h"


namespace
{

QString csvField(QString value)
{
	value.replace(QLatin1Char('"'), QStringLiteral("\"\""));
	return QLatin1Char('"') + value + QLatin1Char('"');
}

QString stateText(ComputerControlInterface::State state)
{
	switch (state)
	{
	case ComputerControlInterface::State::Connected:
		return ClassScenes::tr("Online");
	case ComputerControlInterface::State::HostOffline:
		return ClassScenes::tr("Offline");
	case ComputerControlInterface::State::Connecting:
		return ClassScenes::tr("Connecting");
	default:
		return ClassScenes::tr("Problem");
	}
}

bool isProblem(ComputerControlInterface::State state)
{
	switch (state)
	{
	case ComputerControlInterface::State::ServerNotRunning:
	case ComputerControlInterface::State::AuthenticationFailed:
	case ComputerControlInterface::State::AccessControlFailed:
	case ComputerControlInterface::State::ConnectionFailed:
	case ComputerControlInterface::State::HostNameResolutionFailed:
		return true;
	default:
		return false;
	}
}

} // namespace



ClassScenes::ClassScenes(MainWindow* mainWindow) :
	QObject(mainWindow),
	m_mainWindow(mainWindow),
	m_timer(new QTimer(this))
{
	m_timer->setInterval(1000);
	connect(m_timer, &QTimer::timeout, this, &ClassScenes::tick);

	const auto classGroup = tr("Quick class");
	const auto selectGroup = tr("Selection");

	m_scenes = {
		{tr("Focus: lock all screens"),
		 tr("Locks screens and input devices so everybody looks at you."), classGroup, [this]() { focusClass(); }},
		{tr("Continue learning: unlock screens"),
		 tr("Unlocks the screens and returns to monitoring."), classGroup, [this]() { resumeClass(); }},
		{tr("Send announcement…"),
		 tr("Type a message that pops up on the selected (or all) computers."), classGroup, [this]() { announce(); }},
		{tr("Start break (auto unlock)…"),
		 tr("Locks screens with a message and unlocks them automatically when the break is over."), classGroup, [this]() { startBreak(); }},
		{tr("Work timer with reminders…"),
		 tr("Counts down e.g. an exercise and warns the students 5 minutes before the end."), classGroup, [this]() { startTimer(); }},
		{tr("Cancel running timer"),
		 tr("Stops the break or work timer."), classGroup, [this]() { cancelTimer(); }},
		{tr("Save attendance list"),
		 tr("Saves who is logged in on which computer right now to a CSV file (opens in Excel)."), classGroup, [this]() { saveAttendance(); }},

		{tr("Select all online computers"), tr("Selects every computer that can be controlled."), selectGroup, [this]() { selectByState(true); }},
		{tr("Select all offline computers"), tr("Handy for Wake-on-LAN (\"Power on\")."), selectGroup, [this]() { selectByState(false); }},
		{tr("Select computers with problems"), tr("Selects computers where the service is missing or authentication failed."), selectGroup, [this]() { selectProblems(); }},
		{tr("Invert selection"), tr("Selects what is not selected and vice versa."), selectGroup, [this]() { invertSelection(); }},
		{tr("Clear selection"), tr("Deselects all computers (actions then apply to all visible computers)."), selectGroup, [this]() { clearSelection(); }},
	};
}



bool ClassScenes::timerActive() const
{
	return m_timer->isActive();
}



QString ClassScenes::timerText() const
{
	if (timerActive() == false)
	{
		return {};
	}

	const auto seconds = qMax<qint64>(0, QDateTime::currentDateTime().secsTo(m_endTime));
	return QStringLiteral("%1 %2:%3").arg(m_timerLabel).arg(seconds / 60, 2, 10, QLatin1Char('0')).arg(seconds % 60, 2, 10, QLatin1Char('0'));
}



void ClassScenes::sendMessage(const QString& text, const QString& title)
{
	auto& master = m_mainWindow->masterCore();

	for (const auto& feature : master.features())
	{
		if (feature.name() == QLatin1String("TextMessage"))
		{
			VeyonCore::featureManager().controlFeature(feature.uid(), FeatureProviderInterface::Operation::Start,
													   {
														   {QStringLiteral("text"), text},
														   {QStringLiteral("icon"), int(QMessageBox::Information)},
														   {QStringLiteral("title"), title}
													   },
													   master.filteredComputerControlInterfaces());
			return;
		}
	}
}



void ClassScenes::lockScreens(bool lock)
{
	auto toolBar = m_mainWindow;
	auto lockButton = toolBar->findChild<QToolButton*>(QStringLiteral("ScreenLock"));
	auto monitoringButton = toolBar->findChild<QToolButton*>(VeyonCore::builtinFeatures().monitoringMode().feature().name());

	if (lockButton == nullptr)
	{
		return;
	}

	if (lock)
	{
		if (lockButton->isChecked() == false)
		{
			lockButton->click();
		}
	}
	else if (lockButton->isChecked() && monitoringButton)
	{
		monitoringButton->click();
	}
}



void ClassScenes::focusClass()
{
	lockScreens(true);
}



void ClassScenes::resumeClass()
{
	lockScreens(false);
}



void ClassScenes::announce()
{
	bool ok = false;
	const auto text = QInputDialog::getMultiLineText(m_mainWindow, tr("Send announcement"),
													 tr("Message for the students:"), {}, &ok);
	if (ok && text.trimmed().isEmpty() == false)
	{
		sendMessage(text.trimmed(), tr("Announcement"));
	}
}



void ClassScenes::startBreak()
{
	bool ok = false;
	const auto minutes = QInputDialog::getInt(m_mainWindow, tr("Start break"), tr("Break length in minutes:"), 10, 1, 180, 1, &ok);
	if (ok == false)
	{
		return;
	}

	sendMessage(tr("Break time! Please be back in %1 minutes.").arg(minutes), tr("Break"));
	lockScreens(true);

	startCountdown(tr("Break"), minutes, 0, {}, [this]() {
		lockScreens(false);
		sendMessage(tr("The break is over. Let's continue!"), tr("Break"));
	});
}



void ClassScenes::startTimer()
{
	bool ok = false;
	const auto minutes = QInputDialog::getInt(m_mainWindow, tr("Work timer"), tr("Time for the task in minutes:"), 15, 1, 240, 1, &ok);
	if (ok == false)
	{
		return;
	}

	sendMessage(tr("You have %1 minutes for this task.").arg(minutes), tr("Task"));

	startCountdown(tr("Task"), minutes, minutes > 5 ? 5 : 0, tr("5 minutes left!"), [this]() {
		sendMessage(tr("Time is up. Please stop working and look to the front."), tr("Task"));
	});
}



void ClassScenes::startCountdown(const QString& label, int minutes, int warnMinutes, const QString& warnText,
								 std::function<void()> atEnd)
{
	m_timerLabel = label;
	m_endTime = QDateTime::currentDateTime().addSecs(qint64(minutes) * 60);
	m_warnMinutes = warnMinutes;
	m_warnText = warnText;
	m_warned = false;
	m_atEnd = std::move(atEnd);
	m_timer->start();
}



void ClassScenes::cancelTimer()
{
	m_timer->stop();
	m_atEnd = nullptr;
}



void ClassScenes::tick()
{
	const auto remaining = QDateTime::currentDateTime().secsTo(m_endTime);

	if (m_warnMinutes > 0 && m_warned == false && remaining <= qint64(m_warnMinutes) * 60)
	{
		m_warned = true;
		sendMessage(m_warnText, m_timerLabel);
	}

	if (remaining <= 0)
	{
		m_timer->stop();
		auto atEnd = std::move(m_atEnd);
		m_atEnd = nullptr;
		if (atEnd)
		{
			atEnd();
		}
	}
}



void ClassScenes::saveAttendance()
{
	auto& model = m_mainWindow->masterCore().computerControlListModel();

	const auto dir = QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).filePath(QStringLiteral("Vyrja Attendance"));
	QDir().mkpath(dir);

	const auto now = QDateTime::currentDateTime();
	const auto fileName = QDir(dir).filePath(QStringLiteral("attendance-%1.csv").arg(now.toString(QStringLiteral("yyyyMMdd-HHmm"))));

	QFile file(fileName);
	if (file.open(QFile::WriteOnly | QFile::Truncate) == false)
	{
		QMessageBox::warning(m_mainWindow, tr("Attendance"), tr("Could not write %1").arg(fileName));
		return;
	}

	QTextStream out(&file);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	out.setEncoding(QStringConverter::Utf8);
#else
	out.setCodec("UTF-8");
#endif
	out << QChar(0xFEFF); // BOM so that Excel opens UTF-8 correctly
	out << "Date,Time,Computer,Address,Login name,Full name,Status\r\n";

	int present = 0;
	int total = 0;
	for (int row = 0; row < model.rowCount(); ++row)
	{
		const auto controlInterface = model.computerControlInterface(model.index(row));
		if (controlInterface.isNull())
		{
			continue;
		}

		++total;
		const auto loggedIn = controlInterface->state() == ComputerControlInterface::State::Connected &&
							  controlInterface->userLoginName().isEmpty() == false;
		if (loggedIn)
		{
			++present;
		}

		out << csvField(now.toString(QStringLiteral("yyyy-MM-dd"))) << ','
			<< csvField(now.toString(QStringLiteral("HH:mm"))) << ','
			<< csvField(controlInterface->computer().displayName()) << ','
			<< csvField(controlInterface->computer().hostAddress().toString()) << ','
			<< csvField(controlInterface->userLoginName()) << ','
			<< csvField(controlInterface->userFullName()) << ','
			<< csvField(stateText(controlInterface->state())) << "\r\n";
	}
	file.close();

	QMessageBox box(QMessageBox::Information, tr("Attendance saved"),
					tr("%1 of %2 computers have a logged-in user.\n\nSaved to:\n%3").arg(present).arg(total).arg(fileName),
					QMessageBox::Ok, m_mainWindow);
	auto openFolder = box.addButton(tr("Open folder"), QMessageBox::ActionRole);
	box.exec();
	if (box.clickedButton() == static_cast<QAbstractButton*>(openFolder))
	{
		QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
	}
}



void ClassScenes::selectByState(bool online)
{
	auto view = m_mainWindow->computerView();
	auto model = view->model();
	view->selectionModel()->clearSelection();

	for (int row = 0; row < model->rowCount(); ++row)
	{
		const auto index = model->index(row, 0);
		const auto controlInterface = index.data(ComputerControlListModel::ControlInterfaceRole).value<ComputerControlInterface::Pointer>();
		if (controlInterface.isNull())
		{
			continue;
		}

		const auto isOnline = controlInterface->state() == ComputerControlInterface::State::Connected;
		if (isOnline == online)
		{
			view->selectionModel()->select(index, QItemSelectionModel::Select);
		}
	}
}



void ClassScenes::selectProblems()
{
	auto view = m_mainWindow->computerView();
	auto model = view->model();
	view->selectionModel()->clearSelection();

	for (int row = 0; row < model->rowCount(); ++row)
	{
		const auto index = model->index(row, 0);
		const auto controlInterface = index.data(ComputerControlListModel::ControlInterfaceRole).value<ComputerControlInterface::Pointer>();
		if (controlInterface.isNull() == false && isProblem(controlInterface->state()))
		{
			view->selectionModel()->select(index, QItemSelectionModel::Select);
		}
	}
}



void ClassScenes::clearSelection()
{
	m_mainWindow->computerView()->selectionModel()->clearSelection();
}



void ClassScenes::invertSelection()
{
	auto view = m_mainWindow->computerView();
	auto model = view->model();
	auto selection = view->selectionModel();

	for (int row = 0; row < model->rowCount(); ++row)
	{
		const auto index = model->index(row, 0);
		selection->select(index, QItemSelectionModel::Toggle);
	}
}
