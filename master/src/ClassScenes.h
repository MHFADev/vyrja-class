/*
 *  ClassScenes.h - one-click classroom actions ("Kelas Cepat"): focus, break
 *  with timer, announcements, attendance, quick selection
 *
 *  This file is part of Vyrja Classroom, a fork of Veyon (https://veyon.io).
 *  Licensed under GPL-2.0-or-later.
 */

#pragma once

#include <QDateTime>
#include <QList>
#include <QObject>
#include <QString>

#include <functional>

class QTimer;

class MainWindow;

class ClassScenes : public QObject
{
	Q_OBJECT
public:
	struct Scene
	{
		QString title;
		QString description;
		QString group;
		std::function<void()> run;
	};

	explicit ClassScenes(MainWindow* mainWindow);

	const QList<Scene>& scenes() const
	{
		return m_scenes;
	}

	// e.g. "Istirahat 07:12", empty if no timer is running
	QString timerText() const;

	bool timerActive() const;

private:
	void focusClass();
	void resumeClass();
	void announce();
	void startBreak();
	void startTimer();
	void cancelTimer();
	void saveAttendance();

	void selectByState(bool online);
	void selectProblems();
	void clearSelection();
	void invertSelection();

	void startCountdown(const QString& label, int minutes, int warnMinutes, const QString& warnText,
						std::function<void()> atEnd);
	void tick();

	void sendMessage(const QString& text, const QString& title);
	void lockScreens(bool lock);

	MainWindow* m_mainWindow;
	QList<Scene> m_scenes;

	QTimer* m_timer;
	QDateTime m_endTime;
	QString m_timerLabel;
	int m_warnMinutes = 0;
	QString m_warnText;
	bool m_warned = false;
	std::function<void()> m_atEnd;
};
