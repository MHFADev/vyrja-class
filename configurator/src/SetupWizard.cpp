/*
 *  SetupWizard.cpp - guided quick setup for teacher and student computers
 *
 *  This file is part of Vyrja Classroom, a fork of Veyon (https://veyon.io).
 *  Licensed under GPL-2.0-or-later.
 */

#include <QApplication>
#include <QCheckBox>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHostAddress>
#include <QHostInfo>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QNetworkInterface>
#include <QPixmap>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QTcpSocket>
#include <QTimer>
#include <QUuid>
#include <QVBoxLayout>

#include "../../plugins/builtindirectory/BuiltinDirectoryConfiguration.h"

#include "ConfigurationManager.h"
#include "NetworkObject.h"
#include "ObjectManager.h"
#include "SetupWizard.h"
#include "VeyonConfiguration.h"
#include "VeyonServiceControl.h"
#include "VyrjaTheme.h"


// header-only config proxy of the builtin directory plugin; constructor is defined here
IMPLEMENT_CONFIG_PROXY(BuiltinDirectoryConfiguration)

namespace
{

const QUuid BuiltinDirectoryPluginUid{QStringLiteral("14bacaaa-ebe5-449c-b881-5b382f952571")};

QLabel* makeLabel(const QString& text, QWidget* parent, bool muted = false)
{
	auto label = new QLabel(text, parent);
	label->setWordWrap(true);
	label->setTextFormat(Qt::RichText);
	if (muted)
	{
		// colored by the theme (#WizardMuted), so it follows light and dark mode
		label->setObjectName(QStringLiteral("WizardMuted"));
	}
	return label;
}


void showWizardError(QWidget* parent, const QString& message)
{
	auto box = new QLabel(message, parent);
	box->setWordWrap(true);
	box->setObjectName(QStringLiteral("wizardError"));

	const auto old = parent->findChild<QLabel*>(QStringLiteral("wizardError"));
	if (old)
	{
		old->deleteLater();
	}

	parent->layout()->addWidget(box);
}


QStringList candidateKeyFiles()
{
	QStringList dirs{
		QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
		QStandardPaths::writableLocation(QStandardPaths::DownloadLocation),
		QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
		QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
	};

	// removable drives / mounted shares
	for (const auto& root : QDir::drives())
	{
		dirs.append(root.absoluteFilePath());
	}
	dirs.append(QStringLiteral("/media"));
	dirs.append(QStringLiteral("/mnt"));

	QStringList files;
	for (const auto& dirPath : std::as_const(dirs))
	{
		const QDir dir(dirPath);
		const auto entries = dir.entryInfoList({QStringLiteral("*_public_key.pem")}, QDir::Files, QDir::Time);
		for (const auto& entry : entries)
		{
			files.append(entry.absoluteFilePath());
		}
	}

	files.removeDuplicates();
	return files;
}


QString keyNameFromFile(const QString& fileName)
{
	static const QRegularExpression rx{QStringLiteral("^(.*)_(.*)_key\\.pem$")};
	const auto match = rx.match(QFileInfo(fileName).fileName());
	return match.hasMatch() ? match.captured(1) : QString();
}

} // namespace



// ---------------------------------------------------------------------------
// pages
// ---------------------------------------------------------------------------

class IntroWizardPage : public QWizardPage
{
public:
	explicit IntroWizardPage(QWidget* parent = nullptr) :
		QWizardPage(parent),
		m_teacher(new QRadioButton(QCoreApplication::translate("SetupWizard", "Teacher computer"), this)),
		m_student(new QRadioButton(QCoreApplication::translate("SetupWizard", "Student computer"), this))
	{
		setTitle(QCoreApplication::translate("SetupWizard", "Welcome! What will this computer be used for?"));
		setSubTitle(QCoreApplication::translate("SetupWizard", "Pick one – the wizard will do the rest. It only takes about a minute."));

		auto layout = new QVBoxLayout(this);

		m_teacher->setChecked(true);
		m_teacher->setObjectName(QStringLiteral("WizardChoice"));
		m_student->setObjectName(QStringLiteral("WizardChoice"));

		layout->addWidget(m_teacher);
		layout->addWidget(makeLabel(QCoreApplication::translate("SetupWizard", "I want to monitor and control student computers (show my screen, lock screens, send messages, …)."), this, true));
		layout->addSpacing(14);
		layout->addWidget(m_student);
		layout->addWidget(makeLabel(QCoreApplication::translate("SetupWizard", "This computer will be controlled by the teacher. Needs the key file from the teacher computer."), this, true));
		layout->addStretch(1);
	}

	int nextId() const override
	{
		return m_teacher->isChecked() ? SetupWizard::TeacherKeyPage : SetupWizard::StudentImportPage;
	}

	bool isTeacher() const { return m_teacher->isChecked(); }

private:
	QRadioButton* m_teacher;
	QRadioButton* m_student;
};



class TeacherKeyWizardPage : public QWizardPage
{
public:
	explicit TeacherKeyWizardPage(QWidget* parent = nullptr) :
		QWizardPage(parent),
		m_name(new QLineEdit(QStringLiteral("kelas"), this))
	{
		setTitle(QCoreApplication::translate("SetupWizard", "Step 1 – Create the class security key"));
		setSubTitle(QCoreApplication::translate("SetupWizard", "The key makes sure only YOUR computer can control the students' computers."));

		auto layout = new QVBoxLayout(this);
		layout->addWidget(makeLabel(QCoreApplication::translate("SetupWizard", "Give the key a short name (letters and numbers only), for example the class or lab name:"), this));
		layout->addWidget(m_name);
		layout->addWidget(makeLabel(QCoreApplication::translate("SetupWizard", "If a key with this name already exists on this computer it will be reused."), this, true));
		layout->addStretch(1);

		registerField(QStringLiteral("keyName"), m_name);
	}

	bool validatePage() override
	{
		const auto name = m_name->text().trimmed();
		static const QRegularExpression rx{QStringLiteral("^[A-Za-z0-9_-]+$")};
		if (rx.match(name).hasMatch() == false)
		{
			showWizardError(this, QCoreApplication::translate("SetupWizard", "Please use only letters, numbers, \"-\" and \"_\" in the key name."));
			return false;
		}

		QString output;
		const auto exists = SetupWizard::runCli({QStringLiteral("authkeys"), QStringLiteral("list")}, &output) &&
							output.contains(name + QLatin1Char('/'));

		if (exists == false && SetupWizard::runCli({QStringLiteral("authkeys"), QStringLiteral("create"), name}, &output) == false)
		{
			showWizardError(this, QCoreApplication::translate("SetupWizard", "The key could not be created. Please run Vyrja Setup as administrator.<br><small>%1</small>").arg(output.toHtmlEscaped()));
			return false;
		}

		VeyonCore::config().setAuthenticationMethod(VeyonCore::AuthenticationMethod::KeyFileAuthentication);
		return true;
	}

private:
	QLineEdit* m_name;
};



class TeacherExportWizardPage : public QWizardPage
{
public:
	explicit TeacherExportWizardPage(QWidget* parent = nullptr) :
		QWizardPage(parent),
		m_folder(new QLineEdit(this)),
		m_result(makeLabel({}, this))
	{
		setTitle(QCoreApplication::translate("SetupWizard", "Step 2 – Save the key for the student computers"));
		setSubTitle(QCoreApplication::translate("SetupWizard", "This small file is the \"door key\" for students' computers. It is safe to share – it only lets them recognize your computer."));

		auto layout = new QVBoxLayout(this);
		layout->addWidget(makeLabel(QCoreApplication::translate("SetupWizard", "Where do you want to save it? Choose a USB drive or a shared folder:"), this));

		auto row = new QHBoxLayout;
		m_folder->setText(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
		auto browse = new QPushButton(QCoreApplication::translate("SetupWizard", "Browse…"), this);
		row->addWidget(m_folder, 1);
		row->addWidget(browse);
		layout->addLayout(row);
		layout->addWidget(m_result);
		layout->addStretch(1);

		QObject::connect(browse, &QPushButton::clicked, this, [this]() {
			const auto dir = QFileDialog::getExistingDirectory(this, QCoreApplication::translate("SetupWizard", "Choose folder"), m_folder->text());
			if (dir.isEmpty() == false)
			{
				m_folder->setText(dir);
			}
		});

	}

	bool validatePage() override
	{
		const auto name = static_cast<SetupWizard*>(wizard())->keyName();
		const auto file = QDir(m_folder->text()).filePath(name + QStringLiteral("_public_key.pem"));

		QString output;
		if (SetupWizard::runCli({QStringLiteral("authkeys"), QStringLiteral("export"), name + QStringLiteral("/public"), file, QStringLiteral("-f")}, &output) == false)
		{
			showWizardError(this, QCoreApplication::translate("SetupWizard", "The key file could not be saved to this folder.<br><small>%1</small>").arg(output.toHtmlEscaped()));
			return false;
		}

		m_exportFile = file;
		return true;
	}

	QString m_exportFile;

private:
	QLineEdit* m_folder;
	QLabel* m_result;
};



struct FoundComputer
{
	QString name;
	QString address;
};


class TeacherComputersWizardPage : public QWizardPage
{
public:
	explicit TeacherComputersWizardPage(QWidget* parent = nullptr) :
		QWizardPage(parent),
		m_room(new QLineEdit(QCoreApplication::translate("SetupWizard", "Computer Lab"), this)),
		m_from(new QLineEdit(this)),
		m_to(new QLineEdit(this)),
		m_scanButton(new QPushButton(QCoreApplication::translate("SetupWizard", "Search the network"), this)),
		m_progress(new QProgressBar(this)),
		m_list(new QListWidget(this)),
		m_manual(new QLineEdit(this)),
		m_status(makeLabel({}, this, true))
	{
		setTitle(QCoreApplication::translate("SetupWizard", "Step 3 – Add the student computers"));
		setSubTitle(QCoreApplication::translate("SetupWizard", "Vyrja can find students' computers by itself. They must be switched on and already set up as \"Student computer\"."));

		auto layout = new QVBoxLayout(this);

		auto roomRow = new QHBoxLayout;
		roomRow->addWidget(new QLabel(QCoreApplication::translate("SetupWizard", "Room / group name:"), this));
		roomRow->addWidget(m_room, 1);
		layout->addLayout(roomRow);

		const auto local = SetupWizard::localAddress();
		const auto parts = local.split(QLatin1Char('.'));
		if (parts.size() == 4)
		{
			const auto prefix = parts.mid(0, 3).join(QLatin1Char('.'));
			m_from->setText(prefix + QStringLiteral(".1"));
			m_to->setText(prefix + QStringLiteral(".254"));
		}

		auto rangeRow = new QHBoxLayout;
		rangeRow->addWidget(new QLabel(QCoreApplication::translate("SetupWizard", "Search from"), this));
		rangeRow->addWidget(m_from, 1);
		rangeRow->addWidget(new QLabel(QCoreApplication::translate("SetupWizard", "to"), this));
		rangeRow->addWidget(m_to, 1);
		rangeRow->addWidget(m_scanButton);
		layout->addLayout(rangeRow);

		m_progress->setRange(0, 100);
		m_progress->setVisible(false);
		layout->addWidget(m_progress);

		m_list->setMinimumHeight(140);
		layout->addWidget(m_list, 1);

		auto manualRow = new QHBoxLayout;
		m_manual->setPlaceholderText(QCoreApplication::translate("SetupWizard", "Add by hand: IP address or computer name"));
		auto addButton = new QPushButton(QCoreApplication::translate("SetupWizard", "Add"), this);
		auto selectAll = new QPushButton(QCoreApplication::translate("SetupWizard", "Select all"), this);
		manualRow->addWidget(m_manual, 1);
		manualRow->addWidget(addButton);
		manualRow->addWidget(selectAll);
		layout->addLayout(manualRow);

		layout->addWidget(m_status);

		QObject::connect(m_scanButton, &QPushButton::clicked, this, [this]() { startScan(); });
		QObject::connect(addButton, &QPushButton::clicked, this, [this]() {
			const auto host = m_manual->text().trimmed();
			if (host.isEmpty() == false)
			{
				addEntry(host, host, true);
				m_manual->clear();
			}
		});
		QObject::connect(selectAll, &QPushButton::clicked, this, [this]() {
			for (int i = 0; i < m_list->count(); ++i)
			{
				m_list->item(i)->setCheckState(Qt::Checked);
			}
		});

		setCommitPage(false);
	}

	void initializePage() override
	{
		// skip scanning silently when unattended; user starts it manually
		m_status->setText(QCoreApplication::translate("SetupWizard", "Tip: you can also skip this step and add computers later in Vyrja Classroom."));
	}

	bool validatePage() override
	{
		if (m_scanning)
		{
			showWizardError(this, QCoreApplication::translate("SetupWizard", "Please wait until the search is finished."));
			return false;
		}

		ObjectManager<NetworkObject> manager{BuiltinDirectoryConfiguration(&VeyonCore::config()).networkObjects()};

		const auto room = m_room->text().trimmed().isEmpty() ? QCoreApplication::translate("SetupWizard", "Computer Lab") : m_room->text().trimmed();

		QUuid roomUid;
		int added = 0;
		for (int i = 0; i < m_list->count(); ++i)
		{
			const auto item = m_list->item(i);
			if (item->checkState() != Qt::Checked)
			{
				continue;
			}

			if (roomUid.isNull())
			{
				roomUid = QUuid::createUuid();
				manager.add(NetworkObject{NetworkObject::Type::Location, room, {}, {}, {}, roomUid});
			}

			manager.add(NetworkObject{NetworkObject::Type::Host, item->text(),
									  item->data(Qt::UserRole).toString(), {}, {}, QUuid::createUuid(), roomUid});
			++added;
		}

		if (added > 0)
		{
			BuiltinDirectoryConfiguration(&VeyonCore::config()).setNetworkObjects(manager.objects());
		}

		VeyonCore::config().setNetworkObjectDirectoryPlugin(BuiltinDirectoryPluginUid);

		m_added = added;
		return true;
	}

	int m_added = 0;

private:
	void addEntry(const QString& name, const QString& address, bool checked)
	{
		for (int i = 0; i < m_list->count(); ++i)
		{
			if (m_list->item(i)->data(Qt::UserRole).toString() == address)
			{
				return;
			}
		}

		auto item = new QListWidgetItem(name, m_list);
		item->setData(Qt::UserRole, address);
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
		item->setToolTip(address);
	}

	void startScan()
	{
		QHostAddress from, to;
		if (from.setAddress(m_from->text().trimmed()) == false || to.setAddress(m_to->text().trimmed()) == false ||
			from.protocol() != QAbstractSocket::IPv4Protocol || to.protocol() != QAbstractSocket::IPv4Protocol)
		{
			m_status->setText(QCoreApplication::translate("SetupWizard", "Please enter valid IPv4 addresses, e.g. 192.168.1.1"));
			return;
		}

		const quint32 first = from.toIPv4Address();
		const quint32 last = to.toIPv4Address();
		if (last < first || last - first > 1023)
		{
			m_status->setText(QCoreApplication::translate("SetupWizard", "Please search at most 1024 addresses at once (e.g. one /24 network)."));
			return;
		}

		m_scanning = true;
		m_scanButton->setEnabled(false);
		m_progress->setVisible(true);
		m_progress->setValue(0);
		m_next = first;
		m_last = last;
		m_total = int(last - first + 1);
		m_done = 0;
		m_found = 0;
		m_status->setText(QCoreApplication::translate("SetupWizard", "Searching…"));

		const int workers = 48;
		for (int i = 0; i < workers; ++i)
		{
			scanNext();
		}
	}

	void scanNext()
	{
		if (m_next > m_last)
		{
			return;
		}

		const QHostAddress address(m_next++);
		auto socket = new QTcpSocket(this);
		auto timer = new QTimer(socket);
		timer->setSingleShot(true);

		const auto finish = [this, socket, address](bool open) {
			if (socket->property("finished").toBool())
			{
				return;
			}
			socket->setProperty("finished", true);

			if (open)
			{
				const auto ip = address.toString();
				++m_found;
				addEntry(ip, ip, true);
				QHostInfo::lookupHost(ip, this, [this, ip](const QHostInfo& info) {
					if (info.error() == QHostInfo::NoError && info.hostName().isEmpty() == false && info.hostName() != ip)
					{
						for (int i = 0; i < m_list->count(); ++i)
						{
							auto item = m_list->item(i);
							if (item->data(Qt::UserRole).toString() == ip)
							{
								item->setText(info.hostName().section(QLatin1Char('.'), 0, 0));
							}
						}
					}
				});
			}

			socket->abort();
			socket->deleteLater();

			++m_done;
			m_progress->setValue(m_done * 100 / m_total);

			if (m_done >= m_total)
			{
				m_scanning = false;
				m_scanButton->setEnabled(true);
				m_progress->setVisible(false);
				m_status->setText(m_found > 0 ?
									  QCoreApplication::translate("SetupWizard", "Found %1 student computer(s). Untick any you don't want.").arg(m_found) :
									  QCoreApplication::translate("SetupWizard", "No student computers found. Make sure they are switched on and set up, or add them by hand."));
			}
			else
			{
				scanNext();
			}
		};

		QObject::connect(socket, &QTcpSocket::connected, this, [finish]() { finish(true); });
		QObject::connect(socket, &QTcpSocket::errorOccurred, this, [finish]() { finish(false); });
		QObject::connect(timer, &QTimer::timeout, this, [finish]() { finish(false); });

		timer->start(700);
		socket->connectToHost(address, quint16(VeyonCore::config().veyonServerPort()));
	}

	QLineEdit* m_room;
	QLineEdit* m_from;
	QLineEdit* m_to;
	QPushButton* m_scanButton;
	QProgressBar* m_progress;
	QListWidget* m_list;
	QLineEdit* m_manual;
	QLabel* m_status;

	bool m_scanning = false;
	quint32 m_next = 0;
	quint32 m_last = 0;
	int m_total = 0;
	int m_done = 0;
	int m_found = 0;
};



class TeacherFinishWizardPage : public QWizardPage
{
public:
	explicit TeacherFinishWizardPage(QWidget* parent = nullptr) :
		QWizardPage(parent),
		m_summary(makeLabel({}, this)),
		m_openMaster(new QCheckBox(QCoreApplication::translate("SetupWizard", "Open Vyrja Classroom now"), this))
	{
		setTitle(QCoreApplication::translate("SetupWizard", "All set!"));
		setFinalPage(true);

		auto layout = new QVBoxLayout(this);
		layout->addWidget(m_summary);
		m_openMaster->setChecked(true);
		layout->addWidget(m_openMaster);
		layout->addStretch(1);
	}

	int nextId() const override { return -1; }

	void initializePage() override
	{
		auto wiz = static_cast<SetupWizard*>(wizard());
		const auto exportFile = static_cast<TeacherExportWizardPage*>(wiz->page(SetupWizard::TeacherExportPage))->m_exportFile;
		const auto added = static_cast<TeacherComputersWizardPage*>(wiz->page(SetupWizard::TeacherComputersPage))->m_added;

		m_summary->setText(QCoreApplication::translate("SetupWizard", 
			"<p><b>This computer is ready as the teacher computer.</b></p>"
			"<ul>"
			"<li>Class key: <b>%1</b></li>"
			"<li>Key file for students: <b>%2</b></li>"
			"<li>Student computers added: <b>%3</b></li>"
			"</ul>"
			"<p>Next: on every student computer, run <i>Vyrja Setup</i>, choose <b>Student computer</b> and pick the key file above.</p>")
			.arg(wiz->keyName().toHtmlEscaped(), exportFile.toHtmlEscaped()).arg(added));
	}

	bool validatePage() override
	{
		ConfigurationManager configurationManager;
		if (configurationManager.saveConfiguration() == false || configurationManager.applyConfiguration() == false)
		{
			showWizardError(this, configurationManager.errorString().toHtmlEscaped());
			return false;
		}

		if (m_openMaster->isChecked())
		{
			const auto master = QDir(QCoreApplication::applicationDirPath()).filePath(
#ifdef Q_OS_WIN
				QStringLiteral("vyrja-master.exe")
#else
				QStringLiteral("veyon-master")
#endif
			);
			QProcess::startDetached(master, {});
		}

		return true;
	}

private:
	QLabel* m_summary;
	QCheckBox* m_openMaster;
};



class StudentImportWizardPage : public QWizardPage
{
public:
	explicit StudentImportWizardPage(QWidget* parent = nullptr) :
		QWizardPage(parent),
		m_file(new QLineEdit(this)),
		m_list(new QListWidget(this))
	{
		setTitle(QCoreApplication::translate("SetupWizard", "Step 1 – Select the key file from the teacher"));
		setSubTitle(QCoreApplication::translate("SetupWizard", "It is the file ending in \"_public_key.pem\" that the teacher saved on a USB drive or shared folder."));

		auto layout = new QVBoxLayout(this);

		layout->addWidget(makeLabel(QCoreApplication::translate("SetupWizard", "Key files found on this computer:"), this));
		m_list->setMaximumHeight(110);
		layout->addWidget(m_list);

		auto row = new QHBoxLayout;
		auto browse = new QPushButton(QCoreApplication::translate("SetupWizard", "Browse…"), this);
		row->addWidget(m_file, 1);
		row->addWidget(browse);
		layout->addWidget(makeLabel(QCoreApplication::translate("SetupWizard", "…or choose the file yourself:"), this));
		layout->addLayout(row);
		layout->addStretch(1);

		QObject::connect(m_list, &QListWidget::currentTextChanged, this, [this](const QString& text) {
			if (text.isEmpty() == false)
			{
				m_file->setText(text);
			}
		});
		QObject::connect(browse, &QPushButton::clicked, this, [this]() {
			const auto file = QFileDialog::getOpenFileName(this, QCoreApplication::translate("SetupWizard", "Select key file"), {}, QCoreApplication::translate("SetupWizard", "Key files (*.pem)"));
			if (file.isEmpty() == false)
			{
				m_file->setText(file);
			}
		});
	}

	void initializePage() override
	{
		m_list->clear();
		const auto files = candidateKeyFiles();
		for (const auto& file : files)
		{
			m_list->addItem(file);
		}
		if (m_list->count() > 0)
		{
			m_list->setCurrentRow(0);
		}
	}

	bool validatePage() override
	{
		const auto file = m_file->text().trimmed();
		const auto name = keyNameFromFile(file);

		if (QFileInfo::exists(file) == false || name.isEmpty())
		{
			showWizardError(this, QCoreApplication::translate("SetupWizard", "Please select a valid key file (named like <i>kelas_public_key.pem</i>)."));
			return false;
		}

		QString output;
		if (SetupWizard::runCli({QStringLiteral("authkeys"), QStringLiteral("import"), name + QStringLiteral("/public"), file}, &output) == false)
		{
			showWizardError(this, QCoreApplication::translate("SetupWizard", "The key could not be imported. Please run Vyrja Setup as administrator.<br><small>%1</small>").arg(output.toHtmlEscaped()));
			return false;
		}

		VeyonCore::config().setAuthenticationMethod(VeyonCore::AuthenticationMethod::KeyFileAuthentication);
		wizard()->setProperty("importedKeyName", name);
		return true;
	}

private:
	QLineEdit* m_file;
	QListWidget* m_list;
};



class StudentServiceWizardPage : public QWizardPage
{
public:
	explicit StudentServiceWizardPage(QWidget* parent = nullptr) :
		QWizardPage(parent),
		m_autostart(new QCheckBox(QCoreApplication::translate("SetupWizard", "Start automatically whenever this computer is switched on (recommended)"), this))
	{
		setTitle(QCoreApplication::translate("SetupWizard", "Step 2 – Allow the teacher to connect"));
		setSubTitle(QCoreApplication::translate("SetupWizard", "This starts the background service that lets the teacher see and control this computer during class."));

		auto layout = new QVBoxLayout(this);
		m_autostart->setChecked(true);
		layout->addWidget(m_autostart);
		layout->addWidget(makeLabel(QCoreApplication::translate("SetupWizard", "Windows may ask to allow the program through the firewall – please choose <b>Allow</b>."), this, true));
		layout->addStretch(1);
	}

	bool validatePage() override
	{
		VeyonCore::config().setServiceAutostart(m_autostart->isChecked());

		ConfigurationManager configurationManager;
		if (configurationManager.saveConfiguration() == false || configurationManager.applyConfiguration() == false)
		{
			showWizardError(this, configurationManager.errorString().toHtmlEscaped());
			return false;
		}

		VeyonServiceControl serviceControl(this);
		if (serviceControl.isServiceRunning())
		{
			serviceControl.restartService();
		}
		else
		{
			serviceControl.startService();
		}

		return true;
	}

private:
	QCheckBox* m_autostart;
};



class StudentFinishWizardPage : public QWizardPage
{
public:
	explicit StudentFinishWizardPage(QWidget* parent = nullptr) :
		QWizardPage(parent),
		m_summary(makeLabel({}, this))
	{
		setTitle(QCoreApplication::translate("SetupWizard", "This computer is ready!"));
		setFinalPage(true);

		auto layout = new QVBoxLayout(this);
		layout->addWidget(m_summary);
		layout->addStretch(1);
	}

	int nextId() const override { return -1; }

	void initializePage() override
	{
		const auto running = VeyonServiceControl(this).isServiceRunning();
		m_summary->setText(QCoreApplication::translate("SetupWizard", 
			"<p><b>Student computer is set up.</b></p>"
			"<ul>"
			"<li>Computer name: <b>%1</b></li>"
			"<li>IP address: <b>%2</b></li>"
			"<li>Service status: <b>%3</b></li>"
			"</ul>"
			"<p>The teacher can now find this computer from the teacher's wizard (Step 3), or add it by hand using the name or IP address above.</p>")
			.arg(QHostInfo::localHostName().toHtmlEscaped(), SetupWizard::localAddress().toHtmlEscaped(),
				 running ? QCoreApplication::translate("SetupWizard", "running ✔") : QCoreApplication::translate("SetupWizard", "starting… (check again in a few seconds)")));
	}

private:
	QLabel* m_summary;
};



// ---------------------------------------------------------------------------
// wizard
// ---------------------------------------------------------------------------

SetupWizard::SetupWizard(QWidget* parent) :
	QWizard(parent)
{
	setWindowTitle(QCoreApplication::translate("SetupWizard", "Vyrja Quick Setup"));
	setWizardStyle(QWizard::ModernStyle);
	setPixmap(QWizard::LogoPixmap, VyrjaTheme::logo(VyrjaTheme::Logo::Wordmark, 48, devicePixelRatioF()));
	setPixmap(QWizard::BannerPixmap, QPixmap());
	setOption(QWizard::NoBackButtonOnStartPage, true);
	setMinimumSize(640, 460);

	setButtonText(QWizard::NextButton, QCoreApplication::translate("SetupWizard", "Next >"));
	setButtonText(QWizard::BackButton, QCoreApplication::translate("SetupWizard", "< Back"));
	setButtonText(QWizard::CancelButton, QCoreApplication::translate("SetupWizard", "Cancel"));
	setButtonText(QWizard::FinishButton, QCoreApplication::translate("SetupWizard", "Finish"));

	setPage(IntroPage, new IntroWizardPage(this));
	setPage(TeacherKeyPage, new TeacherKeyWizardPage(this));
	setPage(TeacherExportPage, new TeacherExportWizardPage(this));
	setPage(TeacherComputersPage, new TeacherComputersWizardPage(this));
	setPage(TeacherFinishPage, new TeacherFinishWizardPage(this));
	setPage(StudentImportPage, new StudentImportWizardPage(this));
	setPage(StudentServicePage, new StudentServiceWizardPage(this));
	setPage(StudentFinishPage, new StudentFinishWizardPage(this));

	setStartId(IntroPage);
}



QString SetupWizard::keyName() const
{
	return field(QStringLiteral("keyName")).toString().trimmed();
}



QString SetupWizard::T(const char* source)
{
	return QCoreApplication::translate("SetupWizard", source);
}



bool SetupWizard::runCli(const QStringList& arguments, QString* output)
{
	const auto cli = QDir(QCoreApplication::applicationDirPath()).filePath(
#ifdef Q_OS_WIN
		QStringLiteral("vyrja-cli.exe")
#else
		QStringLiteral("veyon-cli")
#endif
	);

	QProcess process;
	process.setProcessChannelMode(QProcess::MergedChannels);
	process.start(cli, arguments);
	if (process.waitForStarted(5000) == false)
	{
		if (output)
		{
			*output = QCoreApplication::translate("SetupWizard", "vyrja-cli not found");
		}
		return false;
	}

	process.waitForFinished(60000);

	if (output)
	{
		*output = QString::fromUtf8(process.readAll()).trimmed();
	}

	return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
}



QString SetupWizard::localAddress()
{
	const auto addresses = QNetworkInterface::allAddresses();
	for (const auto& address : addresses)
	{
		if (address.protocol() == QAbstractSocket::IPv4Protocol && address.isLoopback() == false &&
			address.isLinkLocal() == false)
		{
			return address.toString();
		}
	}

	return {};
}



bool SetupWizard::wasShown()
{
	return QSettings(QStringLiteral("Vyrja Studio"), QStringLiteral("Vyrja Classroom"))
		.value(QStringLiteral("SetupWizardShown"), false).toBool();
}



void SetupWizard::setShown()
{
	QSettings(QStringLiteral("Vyrja Studio"), QStringLiteral("Vyrja Classroom"))
		.setValue(QStringLiteral("SetupWizardShown"), true);
}
