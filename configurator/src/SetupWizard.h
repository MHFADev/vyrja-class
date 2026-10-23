/*
 *  SetupWizard.h - guided quick setup for teacher and student computers
 *
 *  This file is part of Vyrja Classroom, a fork of Veyon (https://veyon.io).
 *  Licensed under GPL-2.0-or-later.
 */

#pragma once

#include <QWizard>

class SetupWizard : public QWizard
{
public:
	enum PageId {
		IntroPage,
		TeacherKeyPage,
		TeacherExportPage,
		TeacherComputersPage,
		TeacherFinishPage,
		StudentImportPage,
		StudentServicePage,
		StudentFinishPage
	};

	enum class Role {
		Teacher,
		Student
	};

	explicit SetupWizard(QWidget* parent = nullptr);

	// text shown for the localized wizard (wraps QCoreApplication::translate)
	static QString T(const char* source);

	// runs veyon-cli from the application directory
	static bool runCli(const QStringList& arguments, QString* output = nullptr);

	// returns a non-loopback IPv4 address of this computer (empty if none)
	static QString localAddress();

	// remembers that the wizard has been shown at least once
	static bool wasShown();
	static void setShown();

	QString keyName() const;
};
