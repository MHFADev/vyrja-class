/*
 * MainWindow.cpp - implementation of MainWindow class
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

#include <QCloseEvent>
#include <QFileDialog>
#include <QJsonDocument>
#include <QAbstractItemView>
#include <algorithm>
#include <QDir>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QVBoxLayout>
#include <QProcess>
#include <QPushButton>
#include <QMenu>
#include <QShortcut>
#include <QTime>
#include <QTimer>
#include <QToolButton>
#include <QKeyEvent>
#include <QHostAddress>
#include <QMenu>
#include <QMessageBox>
#include <QSplitter>

#include "AboutDialog.h"
#include "AccessControlProvider.h"
#include "Branding.h"
#include "ClassScenes.h"
#include "CommandPalette.h"
#include "MainWindow.h"
#include "BuiltinFeatures.h"
#include "AuthenticationCredentials.h"
#include "ComputerControlListModel.h"
#include "ComputerManager.h"
#include "ComputerSelectPanel.h"
#include "ScreenshotManagementPanel.h"
#include "FeatureManager.h"
#include "MonitoringMode.h"
#include "NetworkObjectDirectory.h"
#include "NetworkObjectDirectoryManager.h"
#include "PlatformUserFunctions.h"
#include "SlideshowPanel.h"
#include "SpotlightPanel.h"
#include "ToolButton.h"
#include "VeyonConfiguration.h"
#include "VeyonMaster.h"
#include "UserConfig.h"
#include "VyrjaTheme.h"

#include "ui_MainWindow.h"


MainWindow::MainWindow( VeyonMaster &masterCore, QWidget* parent ) :
	QMainWindow( parent ),
	ui( new Ui::MainWindow ),
	m_master( masterCore ),
	m_modeGroup( new QButtonGroup( this ) ),
	m_summaryLabel( new QLabel( this ) ),
	m_summaryTimer( new QTimer( this ) ),
	m_commandPalette( new CommandPalette( this ) ),
	m_classScenes( new ClassScenes( this ) )
{
	ui->setupUi( this );

	restoreState( QByteArray::fromBase64( m_master.userConfig().windowState().toUtf8() ) );
	restoreGeometry( QByteArray::fromBase64( m_master.userConfig().windowGeometry().toUtf8() ) );

	// add widgets to status bar
	ui->statusBar->addWidget( ui->panelButtons );
	ui->statusBar->addWidget( ui->spacerLabel1 );
	ui->statusBar->addWidget( ui->filterLineEdit, 2 );
	ui->statusBar->addWidget( ui->filterPoweredOnComputersButton );
	ui->statusBar->addWidget( ui->filterComputersWithLoggedOnUsersButton );
	ui->statusBar->addWidget( ui->spacerLabel2, 1 );
	ui->statusBar->addWidget( ui->gridSizeSlider, 2 );
	ui->statusBar->addWidget( ui->autoAdjustComputerIconSizeButton );
	ui->statusBar->addWidget( ui->spacerLabel3 );
	ui->statusBar->addWidget( ui->useCustomComputerPositionsButton );
	ui->statusBar->addWidget( ui->alignComputersButton );
	ui->statusBar->addWidget( ui->spacerLabel4 );
	ui->statusBar->addWidget( ui->aboutButton );

	// live summary of computer states and quick command launcher
	m_summaryLabel->setTextFormat( Qt::RichText );
	m_summaryLabel->setContentsMargins( 8, 0, 8, 0 );
	ui->statusBar->addPermanentWidget( m_summaryLabel );

	// brand header bar with mode / timer / clock chips and the quick action launcher
	auto headerBar = new QFrame( this );
	headerBar->setObjectName( QStringLiteral( "HeaderBar" ) );
	auto headerLayout = new QHBoxLayout( headerBar );
	headerLayout->setContentsMargins( 14, 8, 14, 8 );
	headerLayout->setSpacing( 12 );

	auto logo = new QLabel( headerBar );
	logo->setObjectName( QStringLiteral( "HeaderLogo" ) );
	logo->setPixmap( VyrjaTheme::logo( VyrjaTheme::Logo::Mark, 40, devicePixelRatioF() ) );
	logo->setAlignment( Qt::AlignCenter );
	logo->setFixedSize( 52, 52 );
	headerLayout->addWidget( logo );

	auto titleBox = new QVBoxLayout;
	titleBox->setSpacing( 0 );
	auto title = new QLabel( Branding::MasterName, headerBar );
	title->setObjectName( QStringLiteral( "HeaderTitle" ) );
	auto subtitle = new QLabel( tr( "Classroom control center" ), headerBar );
	subtitle->setObjectName( QStringLiteral( "HeaderSubtitle" ) );
	titleBox->addWidget( title );
	titleBox->addWidget( subtitle );
	headerLayout->addLayout( titleBox );
	headerLayout->addStretch( 1 );

	// live counters, the first thing a teacher wants to know: who is ready, who is not,
	// who needs attention. Dot = online, ring = offline, square = problem (see the theme)
	const auto addCounter = [headerBar, headerLayout]( const char* state, QLabel*& valueLabel ) -> QWidget* {
		auto chip = new QFrame( headerBar );
		chip->setObjectName( QStringLiteral( "StatChip" ) );
		auto chipLayout = new QHBoxLayout( chip );
		chipLayout->setContentsMargins( 10, 4, 12, 4 );
		chipLayout->setSpacing( 8 );

		auto dot = new QLabel( chip );
		dot->setObjectName( QStringLiteral( "StatDot" ) );
		dot->setProperty( "state", QString::fromLatin1( state ) );
		dot->setFixedSize( 10, 10 );

		valueLabel = new QLabel( chip );
		valueLabel->setObjectName( QStringLiteral( "StatValue" ) );

		chipLayout->addWidget( dot, 0, Qt::AlignVCenter );
		chipLayout->addWidget( valueLabel );
		headerLayout->addWidget( chip );
		return chip;
	};
	addCounter( "online", m_onlineValue );
	addCounter( "offline", m_offlineValue );
	m_problemChip = addCounter( "problem", m_problemValue );
	m_problemChip->hide();

	m_modeChip = new QLabel( headerBar );
	m_modeChip->setObjectName( QStringLiteral( "HeaderChip" ) );
	m_timerChip = new QLabel( headerBar );
	m_timerChip->setObjectName( QStringLiteral( "HeaderChipAccent" ) );
	m_timerChip->hide();
	m_clockChip = new QLabel( headerBar );
	m_clockChip->setObjectName( QStringLiteral( "HeaderChip" ) );
	headerLayout->addWidget( m_modeChip );
	headerLayout->addWidget( m_timerChip );
	headerLayout->addWidget( m_clockChip );

	auto commandButton = new QToolButton( headerBar );
	commandButton->setObjectName( QStringLiteral( "HeaderSearch" ) );
	commandButton->setText( tr( "Quick actions   Ctrl+K" ) );
	commandButton->setToolTip( tr( "Search and run any action, e.g. lock screens or send a message" ) );
	commandButton->setToolButtonStyle( Qt::ToolButtonTextOnly );
	commandButton->setCursor( Qt::PointingHandCursor );
	headerLayout->addWidget( commandButton );
	connect( commandButton, &QToolButton::clicked, m_commandPalette, &CommandPalette::open );
	setMenuWidget( headerBar );

	auto commandShortcut = new QShortcut( QKeySequence( Qt::CTRL | Qt::Key_K ), this );
	connect( commandShortcut, &QShortcut::activated, m_commandPalette, &CommandPalette::open );

	// empty state: says why the grid is empty and offers the two ways to fill it
	m_emptyHint = new QWidget( ui->computerMonitoringWidget->viewport() );
	m_emptyHint->setObjectName( QStringLiteral( "EmptyState" ) );
	auto emptyLayout = new QVBoxLayout( m_emptyHint );
	emptyLayout->setSpacing( 12 );

	auto emptyLogo = new QLabel( m_emptyHint );
	emptyLogo->setPixmap( VyrjaTheme::logo( VyrjaTheme::Logo::Wordmark, 96, devicePixelRatioF() ) );
	emptyLogo->setAlignment( Qt::AlignCenter );

	auto emptyTitle = new QLabel( tr( "No computers yet" ), m_emptyHint );
	emptyTitle->setObjectName( QStringLiteral( "EmptyTitle" ) );
	emptyTitle->setAlignment( Qt::AlignCenter );

	auto emptyBody = new QLabel( tr( "Pick a room under <b>Locations &amp; computers</b>, or run <b>Quick Setup</b> "
									 "in Vyrja Setup to find student computers automatically." ), m_emptyHint );
	emptyBody->setObjectName( QStringLiteral( "EmptyBody" ) );
	emptyBody->setTextFormat( Qt::RichText );
	emptyBody->setWordWrap( true );
	emptyBody->setAlignment( Qt::AlignCenter );
	emptyBody->setMaximumWidth( 460 );

	auto chooseRoomButton = new QPushButton( tr( "Choose a room" ), m_emptyHint );
	chooseRoomButton->setObjectName( QStringLiteral( "EmptyPrimary" ) );
	chooseRoomButton->setCursor( Qt::PointingHandCursor );
	connect( chooseRoomButton, &QPushButton::clicked, this, [this]() {
		ui->computerSelectPanelButton->setChecked( true );
	} );

	auto searchActionsButton = new QPushButton( tr( "Search actions (Ctrl+K)" ), m_emptyHint );
	searchActionsButton->setObjectName( QStringLiteral( "EmptySecondary" ) );
	searchActionsButton->setCursor( Qt::PointingHandCursor );
	connect( searchActionsButton, &QPushButton::clicked, m_commandPalette, &CommandPalette::open );

	auto emptyButtons = new QHBoxLayout;
	emptyButtons->setSpacing( 10 );
	emptyButtons->addStretch( 1 );
	emptyButtons->addWidget( chooseRoomButton );
	emptyButtons->addWidget( searchActionsButton );
	emptyButtons->addStretch( 1 );

	emptyLayout->addStretch( 1 );
	emptyLayout->addWidget( emptyLogo );
	emptyLayout->addWidget( emptyTitle );
	emptyLayout->addWidget( emptyBody, 0, Qt::AlignHCenter );
	emptyLayout->addLayout( emptyButtons );
	emptyLayout->addStretch( 2 );

	m_summaryTimer->setInterval( 1000 );
	connect( m_summaryTimer, &QTimer::timeout, this, &MainWindow::updateSummary );
	m_summaryTimer->start();
	updateSummary();

	// create all views
	auto mainSplitter = new QSplitter( Qt::Horizontal, ui->centralWidget );
	mainSplitter->setChildrenCollapsible( false );
	mainSplitter->setObjectName( QStringLiteral("MainSplitter") );

	auto monitoringSplitter = new QSplitter( Qt::Vertical, mainSplitter );
	monitoringSplitter->setChildrenCollapsible( false );
	monitoringSplitter->setObjectName( QStringLiteral("MonitoringSplitter") );

	auto slideshowSpotlightSplitter = new QSplitter( Qt::Horizontal, monitoringSplitter );
	slideshowSpotlightSplitter->setChildrenCollapsible( false );
	slideshowSpotlightSplitter->setObjectName( QStringLiteral("SlideshowSpotlightSplitter") );

	auto computerSelectPanel = new ComputerSelectPanel( m_master.computerManager() );
	auto screenshotManagementPanel = new ScreenshotManagementPanel();
	auto slideshowPanel = new SlideshowPanel( m_master.userConfig(), ui->computerMonitoringWidget );
	auto spotlightPanel = new SpotlightPanel( m_master.userConfig(), ui->computerMonitoringWidget );

	slideshowSpotlightSplitter->addWidget( slideshowPanel );
	slideshowSpotlightSplitter->addWidget( spotlightPanel );
	slideshowSpotlightSplitter->setStretchFactor( slideshowSpotlightSplitter->indexOf(slideshowPanel), 1 );
	slideshowSpotlightSplitter->setStretchFactor( slideshowSpotlightSplitter->indexOf(spotlightPanel), 1 );

	monitoringSplitter->addWidget( slideshowSpotlightSplitter );
	monitoringSplitter->addWidget( ui->computerMonitoringWidget );
	monitoringSplitter->setStretchFactor( monitoringSplitter->indexOf(slideshowSpotlightSplitter), 1 );
	monitoringSplitter->setStretchFactor( monitoringSplitter->indexOf(ui->computerMonitoringWidget), 1 );

	mainSplitter->addWidget( computerSelectPanel );
	mainSplitter->addWidget( screenshotManagementPanel );
	mainSplitter->addWidget( monitoringSplitter );

	mainSplitter->setStretchFactor( mainSplitter->indexOf(monitoringSplitter), 1 );


	static const QHash<QWidget *, QAbstractButton *> panelButtons{
		{ computerSelectPanel, ui->computerSelectPanelButton },
		{ screenshotManagementPanel, ui->screenshotManagementPanelButton },
		{ slideshowPanel, ui->slideshowPanelButton },
		{ spotlightPanel, ui->spotlightPanelButton }
	};

	for( auto it = panelButtons.constBegin(), end = panelButtons.constEnd(); it != end; ++it )
	{
		it.key()->hide();
		it.key()->installEventFilter( this );
		connect( *it, &QAbstractButton::toggled, it.key(), &QWidget::setVisible );
	}

	QList<int> splitterSizes;
	for( auto* splitter : { slideshowSpotlightSplitter, monitoringSplitter, mainSplitter } )
	{
		splitter->setHandleWidth( 7 );

		splitter->installEventFilter( this );

		int index = 0;

		const auto splitterStates = m_master.userConfig().splitterStates()[splitter->objectName()].toArray();
		splitterSizes.clear();
		splitterSizes.reserve( splitterStates.size() );

		for( const auto& sizeObject : splitterStates )
		{
			auto size = sizeObject.toInt();
			const auto widget = splitter->widget( index );
			const auto button = panelButtons.value( widget );

			if( widget )
			{
				if( button )
				{
					widget->setVisible( size > 0 );
					button->setChecked( size > 0 );
				}
				size = qAbs( size );
				if( splitter->orientation() == Qt::Horizontal )
				{
					widget->resize( size, widget->height() );
				}
				else
				{
					widget->resize( widget->width(), size );
				}

				widget->setProperty( originalSizePropertyName(), widget->size() );
			}
			splitterSizes.append( size );
			++index;
		}
		splitter->setSizes( splitterSizes );
	}

	const auto SplitterContentBaseSize = 500;

	if( spotlightPanel->property( originalSizePropertyName() ).isNull() ||
		slideshowPanel->property( originalSizePropertyName() ).isNull() )
	{
		slideshowSpotlightSplitter->setSizes( { SplitterContentBaseSize, SplitterContentBaseSize } );
	}

	if( slideshowSpotlightSplitter->property( originalSizePropertyName() ).isNull() ||
		ui->computerMonitoringWidget->property( originalSizePropertyName() ).isNull() )
	{
		monitoringSplitter->setSizes( { SplitterContentBaseSize, SplitterContentBaseSize } );
	}

	ui->centralLayout->addWidget( mainSplitter );

	if( VeyonCore::config().autoOpenComputerSelectPanel() )
	{
		ui->computerSelectPanelButton->setChecked( true );
	}

	// initialize filter controls
	connect( ui->filterLineEdit, &QLineEdit::textChanged,
			 this, [this]( const QString& filter ) { ui->computerMonitoringWidget->setSearchFilter( filter ); } );
	connect( ui->filterPoweredOnComputersButton, &QToolButton::toggled,
			 this, [this]( bool enabled ) { ui->computerMonitoringWidget->setFilterPoweredOnComputers( enabled ); } );
	connect( ui->filterComputersWithLoggedOnUsersButton, &QToolButton::toggled,
			 this, [this]( bool enabled ) { ui->computerMonitoringWidget->setFilterComputersWithLoggedOnUsers( enabled ); } );
	ui->filterPoweredOnComputersButton->setChecked(m_master.userConfig().filterPoweredOnComputers());
	ui->filterComputersWithLoggedOnUsersButton->setChecked(m_master.userConfig().filterComputersWithLoggedOnUsers());

	// initialize monitoring screen size slider
	ui->gridSizeSlider->setMinimum( ComputerMonitoringWidget::MinimumComputerScreenSize );
	ui->gridSizeSlider->setMaximum( ComputerMonitoringWidget::MaximumComputerScreenSize );

	ui->autoAdjustComputerIconSizeButton->setChecked( ui->computerMonitoringWidget->autoAdjustIconSize() );

	connect( ui->gridSizeSlider, &QSlider::valueChanged,
			 this, [this]( int size ) { ui->computerMonitoringWidget->setComputerScreenSize( size ); } );
	connect( ui->computerMonitoringWidget, &ComputerMonitoringWidget::computerScreenSizeAdjusted,
			 ui->gridSizeSlider, &QSlider::setValue );
	connect( ui->autoAdjustComputerIconSizeButton, &QToolButton::toggled,
			 this, [this]( bool enabled ) {
				 ui->computerMonitoringWidget->setAutoAdjustIconSize( enabled );
				 m_master.userConfig().setAutoAdjustMonitoringIconSize( enabled );
			 } );

	int size = ComputerMonitoringWidget::DefaultComputerScreenSize;
	if( m_master.userConfig().monitoringScreenSize() >= ComputerMonitoringWidget::MinimumComputerScreenSize )
	{
		size = m_master.userConfig().monitoringScreenSize();
	}

	ui->gridSizeSlider->setValue( size );
	ui->computerMonitoringWidget->setComputerScreenSize( size );

	// initialize computer placement controls
	auto customComputerPositionsControlMenu = new QMenu(this);
	const auto darkSuffix = VeyonCore::useDarkMode() ? QStringLiteral("-dark") : QString();
	customComputerPositionsControlMenu->addAction(QIcon(QStringLiteral(":/core/document-open%1.png").arg(darkSuffix)),
													tr("Load computer positions"),
													this, &MainWindow::loadComputerPositions);
	customComputerPositionsControlMenu->addAction(QIcon(QStringLiteral(":/core/document-save%1.png").arg(darkSuffix)),
													tr("Save computer positions"),
													this, &MainWindow::saveComputerPositions);
	ui->useCustomComputerPositionsButton->setMenu(customComputerPositionsControlMenu);

	ui->useCustomComputerPositionsButton->setChecked(m_master.userConfig().useCustomComputerPositions());
	connect(ui->useCustomComputerPositionsButton, &QToolButton::toggled,
			ui->computerMonitoringWidget, &ComputerMonitoringWidget::setUseCustomComputerPositions);
	connect(ui->alignComputersButton, &QToolButton::clicked,
			ui->computerMonitoringWidget, &ComputerMonitoringWidget::alignComputers);


	const auto toolButtons = findChildren<QToolButton*>();
	for(auto* btn : toolButtons)
	{
		btn->setIconSize(QSize(20, 20));
	}

	if (VeyonCore::useDarkMode())
	{
		ui->aboutButton->setIcon(QIcon(QStringLiteral(":/core/help-about-dark.png")));
		ui->filterComputersWithLoggedOnUsersButton->setIcon(QIcon(QStringLiteral(":/core/user-group-new-dark.png")));
		ui->autoAdjustComputerIconSizeButton->setIcon(QIcon(QStringLiteral(":/master/zoom-fit-best-dark.png")));
		ui->alignComputersButton->setIcon(QIcon(QStringLiteral(":/master/align-grid-dark.png")));
		ui->useCustomComputerPositionsButton->setIcon(QIcon(QStringLiteral(":/master/exchange-positions-zorder-dark.png")));
		ui->filterPoweredOnComputersButton->setIcon(QIcon(QStringLiteral(":/master/powered-on-dark.png")));
	}

	// create the main toolbar
	ui->toolBar->layout()->setSpacing( 2 );
	ui->toolBar->toggleViewAction()->setEnabled( false );

	addToolBar( Qt::TopToolBarArea, ui->toolBar );

	addFeaturesToToolBar();
	reloadSubFeatures();

	const auto monitoringModeButton = m_modeGroup->button(int(qHash(VeyonCore::builtinFeatures().monitoringMode().feature().uid()))); // clazy:exclude=qt6-qhash-signature
	if (monitoringModeButton)
	{
		monitoringModeButton->setChecked(true);
	}
}



MainWindow::~MainWindow()
{
	ui->computerMonitoringWidget->saveConfiguration();

	delete ui;
}



bool MainWindow::initAuthentication()
{
	if( VeyonCore::instance()->initAuthentication() )
	{
		return true;
	}

	if( VeyonCore::config().authenticationMethod() == VeyonCore::AuthenticationMethod::KeyFileAuthentication )
	{
		QMessageBox box( QMessageBox::Information, tr( "Setup needed" ),
						 tr( "This computer is not set up yet (no valid authentication key was found).\n\n"
							 "Open Quick Setup - it takes about a minute and guides you step by step." ) );
		auto setupButton = box.addButton( tr( "Open Quick Setup" ), QMessageBox::AcceptRole );
		box.addButton( tr( "Later" ), QMessageBox::RejectRole );
		box.exec();
		if( box.clickedButton() == static_cast<QAbstractButton *>( setupButton ) )
		{
			const auto configurator = QDir( QCoreApplication::applicationDirPath() ).filePath(
#ifdef Q_OS_WIN
				QStringLiteral( "vyrja-configurator.exe" )
#else
				QStringLiteral( "veyon-configurator" )
#endif
			);
			QProcess::startDetached( configurator, {} );
		}
	}

	return false;
}



bool MainWindow::initAccessControl()
{
	if (VeyonCore::config().accessControlForMasterEnabled())
	{
		const auto accessingUser = VeyonCore::authenticationCredentials().hasCredentials(AuthenticationCredentials::Type::UserLogon) ?
									   VeyonCore::authenticationCredentials().logonUsername() :
									   VeyonCore::platform().userFunctions().queryCurrentUserProperty(PlatformUserFunctions::UserProperty::LoginName);
		const auto accessControlResult = VeyonCore::builtinFeatures().accessControlProvider()
										 .checkAccess(accessingUser,
													  QHostAddress(QHostAddress::LocalHost).toString(),
													  {});
		if( accessControlResult.access == AccessControlProvider::Access::Deny )
		{
			vWarning() << "user" << VeyonCore::authenticationCredentials().logonUsername()
					   << "is not allowed to access computers";
			QMessageBox::critical( nullptr, tr( "Access denied" ),
								   tr( "According to the local configuration you're not allowed "
									   "to access computers in the network. Please log in with a different "
									   "account or let your system administrator check the local configuration." ) );
			return false;
		}
	}

	return true;
}



void MainWindow::reloadSubFeatures()
{
	for( const auto& feature : m_master.features() )
	{
		auto button = ui->toolBar->findChild<QToolButton *>( feature.name() );
		if( button )
		{
			addSubFeaturesToToolButton( button, feature );
		}
	}
}



ComputerControlInterfaceList MainWindow::selectedComputerControlInterfaces() const
{
	return ui->computerMonitoringWidget->selectedComputerControlInterfaces();
}



void MainWindow::closeEvent( QCloseEvent* event )
{
	if( m_master.currentMode() != VeyonCore::builtinFeatures().monitoringMode().feature().uid() )
	{
		const Feature& activeFeature = VeyonCore::featureManager().feature( m_master.currentMode() );

		QMessageBox::information(this, tr("Feature active"),
								 tr("The feature \"%1\" is still active. Please stop it before closing Vyrja Classroom.")
								 .arg(activeFeature.displayName()));
		event->ignore();
		return;
	}

	QJsonObject splitterStates;
	const auto splitters = findChildren<QSplitter *>();
	for( const auto* splitter : splitters )
	{
		const auto sizes = splitter->sizes();
		QJsonArray splitterSizes;
		int i = 0;
		int hiddenSize = 0;
		for( auto size : sizes )
		{
			auto widget = splitter->widget(i);
			const auto originalSize = widget->property( originalSizePropertyName() ).toSize();
			if( widget->size().isEmpty() && originalSize.isEmpty() == false )
			{
				size = splitter->orientation() == Qt::Horizontal ? -originalSize.width() : -originalSize.height();
				hiddenSize += qAbs(size);
			}
			else
			{
				size -= hiddenSize;
			}

			splitterSizes.append( size );
			++i;
		}
		splitterStates[splitter->objectName()] = splitterSizes;
	}

	m_master.userConfig().setSplitterStates( splitterStates );

	m_master.userConfig().setWindowState( QString::fromLatin1( saveState().toBase64() ) );
	m_master.userConfig().setWindowGeometry( QString::fromLatin1( saveGeometry().toBase64() ) );

	QMainWindow::closeEvent( event );
}



bool MainWindow::eventFilter( QObject* object, QEvent* event )
{
	if( event->type() == QEvent::Resize )
	{
		const auto widget = qobject_cast<QWidget *>( object );
		const auto resizeEvent = static_cast<QResizeEvent *>( event );

		if( resizeEvent->oldSize().isEmpty() == false )
		{
			widget->setProperty( originalSizePropertyName(), resizeEvent->oldSize() );
		}
	}

	return QMainWindow::eventFilter( object, event );
}



void MainWindow::keyPressEvent( QKeyEvent* event )
{
	switch( event->key() )
	{
	case Qt::Key_F5:
		VeyonCore::networkObjectDirectoryManager().configuredDirectory()->update();
		m_master.computerControlListModel().reload();
		event->accept();
		break;
	case Qt::Key_F11:
		QWidget::setWindowState( QWidget::windowState() ^ Qt::WindowFullScreen );
		event->accept();
		break;
	default:
		QMainWindow::keyPressEvent( event );
		break;
	}
}



void MainWindow::showAboutDialog()
{
	AboutDialog( this ).exec();
}



void MainWindow::addFeaturesToToolBar()
{
	// "Quick class" menu with one-click scenes comes first
	auto quickButton = new QToolButton( this );
	quickButton->setText( tr( "Quick class" ) );
	quickButton->setToolTip( tr( "One-click actions: focus, break timer, announcement, attendance, quick selection" ) );
	// the mark on a cream plaque: the plain app icon disappears into the orange button
	quickButton->setIcon( QIcon( QStringLiteral( ":/core/quick-class.png" ) ) );
	quickButton->setIconSize( QSize( 36, 36 ) );
	quickButton->setToolButtonStyle( Qt::ToolButtonTextUnderIcon );
	quickButton->setPopupMode( QToolButton::InstantPopup );
	quickButton->setObjectName( QStringLiteral( "QuickClassButton" ) );

	auto quickMenu = new QMenu( quickButton );
	QString currentGroup;
	for( const auto& scene : m_classScenes->scenes() )
	{
		if( scene.group != currentGroup )
		{
			currentGroup = scene.group;
			quickMenu->addSection( currentGroup );
		}
		auto action = quickMenu->addAction( scene.title );
		action->setToolTip( scene.description );
		action->setStatusTip( scene.description );
		connect( action, &QAction::triggered, this, [run = scene.run]() { run(); } );
	}
	quickButton->setMenu( quickMenu );

	// ribbon groups: Class / Monitor / Teach / Files / Power & sessions
	const auto groupTitles = QStringList{ tr( "Monitor" ), tr( "Teach" ), tr( "Files" ), tr( "Power & sessions" ), tr( "More" ) };
	const auto groupOf = []( const Feature& feature ) {
		const auto name = feature.name();
		if( name == QLatin1String( "MonitoringMode" ) || name == QLatin1String( "RemoteView" ) ||
			name == QLatin1String( "RemoteControl" ) || name == QLatin1String( "Screenshot" ) ) { return 0; }
		if( name == QLatin1String( "Demo" ) || name == QLatin1String( "ScreenLock" ) ||
			name == QLatin1String( "TextMessage" ) || name == QLatin1String( "StartApp" ) ||
			name == QLatin1String( "OpenWebsite" ) ) { return 1; }
		if( name == QLatin1String( "DistributeFiles" ) || name == QLatin1String( "FileCollect" ) ) { return 2; }
		if( name == QLatin1String( "PowerOn" ) || name == QLatin1String( "Reboot" ) || name == QLatin1String( "PowerDown" ) ||
			name == QLatin1String( "UserLogin" ) || name == QLatin1String( "UserLogoff" ) ) { return 3; }
		return 4;
	};

	auto orderedFeatures = QList<Feature>( m_master.features().begin(), m_master.features().end() );
	std::stable_sort( orderedFeatures.begin(), orderedFeatures.end(), [&]( const Feature& a, const Feature& b ) {
		return groupOf( a ) < groupOf( b );
	} );

	const auto makeGroup = [this]( const QString& caption, QHBoxLayout*& row ) {
		auto frame = new QFrame( ui->toolBar );
		frame->setObjectName( QStringLiteral( "RibbonGroup" ) );
		auto column = new QVBoxLayout( frame );
		column->setContentsMargins( 6, 4, 6, 2 );
		column->setSpacing( 0 );
		row = new QHBoxLayout;
		row->setSpacing( 2 );
		column->addLayout( row );
		auto label = new QLabel( caption, frame );
		label->setObjectName( QStringLiteral( "RibbonCaption" ) );
		label->setAlignment( Qt::AlignCenter );
		column->addWidget( label );
		ui->toolBar->addWidget( frame );
		return frame;
	};

	QHBoxLayout* quickRow = nullptr;
	makeGroup( tr( "Class" ), quickRow );
	quickRow->addWidget( quickButton );

	int previousGroup = -1;
	QHBoxLayout* currentRow = nullptr;

	for( const auto& feature : std::as_const( orderedFeatures ) )
	{
		if( feature.testFlag( Feature::Flag::Meta ) )
		{
			continue;
		}

		const auto group = groupOf( feature );
		if( group != previousGroup )
		{
			makeGroup( groupTitles.value( group ), currentRow );
			previousGroup = group;
		}

		auto btn = new ToolButton( QIcon( feature.iconUrl() ),
										  feature.displayName(),
										  feature.displayNameActive(),
										  feature.description(),
										  feature.shortcut() );
		connect(btn, &QToolButton::clicked, this, [=, this] () {
			m_master.runFeature( feature );
			updateModeButtonGroup();
			if( feature.testFlag( Feature::Flag::Mode ) )
			{
				reloadSubFeatures();
			}
		});
		btn->setObjectName( feature.name() );
		btn->setIconSize( QSize( 36, 36 ) );
		currentRow->addWidget( btn );

		if( feature.testFlag( Feature::Flag::Mode ) )
		{
			btn->setCheckable( true );
			m_modeGroup->addButton( btn, buttonId( feature ) );
		}
	}
}



void MainWindow::addSubFeaturesToToolButton( QToolButton* button, const Feature& parentFeature )
{
	if( button->menu() )
	{
		button->menu()->close();
		button->menu()->deleteLater();
		button->setMenu( nullptr );
	}

	const auto parentFeatureIsMode = parentFeature.testFlag( Feature::Flag::Mode );
	const auto subFeatures = m_master.subFeatures( parentFeature.uid() );

	if( subFeatures.isEmpty() ||
		( parentFeatureIsMode && button->isChecked() ) )
	{
		return;
	}

	auto menu = new QMenu( button );
	menu->setObjectName( parentFeature.name() );
	menu->setToolTipsVisible( true );

	for( const auto& subFeature : subFeatures )
	{
		auto action = menu->addAction(QIcon(subFeature.iconUrl()), subFeature.displayName(),
							   #if QT_VERSION >= QT_VERSION_CHECK(6, 3, 0)
									  subFeature.shortcut(),
							   #endif
									  this, [=, this]()
		{
			m_master.runFeature( subFeature );
			if( parentFeatureIsMode )
			{
				if( subFeature.testFlag( Feature::Flag::Option ) == false )
				{
					button->setChecked( true );
				}
				reloadSubFeatures();
			}
		}
#if QT_VERSION < QT_VERSION_CHECK(6, 3, 0)
		,subFeature.shortcut()
#endif
		);
		action->setToolTip( subFeature.description() );
		action->setObjectName( subFeature.uid().toString() );

		if( subFeature.testFlag( Feature::Flag::Option ) )
		{
			action->setCheckable( true );
			action->setChecked( subFeature.testFlag( Feature::Flag::Checked ) );
		}
	}

	button->setMenu( menu );
	button->setPopupMode( ToolButton::InstantPopup );
}



void MainWindow::updateModeButtonGroup()
{
	const auto& monitoringMode = VeyonCore::builtinFeatures().monitoringMode().feature();

	if( m_master.currentMode() == monitoringMode.uid() )
	{
		m_modeGroup->button( buttonId( monitoringMode ) )->setChecked( true );
	}
}



QAbstractItemView* MainWindow::computerView()
{
	return ui->computerMonitoringWidget;
}



void MainWindow::updateSummary()
{
	const auto& model = m_master.computerControlListModel();

	int online = 0;
	int offline = 0;
	int problems = 0;

	for( int row = 0; row < model.rowCount(); ++row )
	{
		const auto controlInterface = model.computerControlInterface( model.index( row ) );
		if( controlInterface.isNull() )
		{
			continue;
		}

		switch( controlInterface->state() )
		{
		case ComputerControlInterface::State::Connected:
			++online;
			break;
		case ComputerControlInterface::State::HostOffline:
			++offline;
			break;
		case ComputerControlInterface::State::ServerNotRunning:
		case ComputerControlInterface::State::AuthenticationFailed:
		case ComputerControlInterface::State::AccessControlFailed:
		case ComputerControlInterface::State::ConnectionFailed:
		case ComputerControlInterface::State::HostNameResolutionFailed:
			++problems;
			break;
		default:
			break;
		}
	}

	const auto selected = selectedComputerControlInterfaces().size();

	m_emptyHint->setVisible( ui->computerMonitoringWidget->model() && ui->computerMonitoringWidget->model()->rowCount() == 0 );
	m_emptyHint->setGeometry( ui->computerMonitoringWidget->viewport()->rect() );

	// counters in the header bar, the selection in the status bar next to the grid controls
	m_onlineValue->setText( tr( "%1 online" ).arg( online ) );
	m_offlineValue->setText( tr( "%1 offline" ).arg( offline ) );
	m_problemValue->setText( tr( "%1 with problems" ).arg( problems ) );
	m_problemChip->setVisible( problems > 0 );
	m_summaryLabel->setText( tr( "%1 selected" ).arg( selected ) );

	const auto timerText = m_classScenes->timerText();
	m_timerChip->setVisible( timerText.isEmpty() == false );
	m_timerChip->setText( timerText );

	const auto modeButton = m_modeGroup->checkedButton();
	m_modeChip->setText( tr( "Mode: %1" ).arg( modeButton ? modeButton->text() : tr( "Monitoring" ) ) );
	m_clockChip->setText( QTime::currentTime().toString( QStringLiteral( "HH:mm" ) ) );
}


void MainWindow::loadComputerPositions()
{
	if (const auto fileName = QFileDialog::getOpenFileName(this, tr("Load computer positions"),
														   QDir::homePath(), tr("JSON files (*.json)"));
		!fileName.isEmpty())
	{
		if (QFile file(fileName); file.open(QFile::ReadOnly))
		{
			const auto& computerPositionsProperty = m_master.userConfig().computerPositionsProperty();
			const auto config = QJsonDocument::fromJson(file.readAll()).object();
			const auto uiConfig = config[computerPositionsProperty.parentKey()].toObject();
			const auto computerPositions = uiConfig[computerPositionsProperty.key()].toObject()[QStringLiteral("JsonStoreArray")].toArray();

			ui->computerMonitoringWidget->loadPositions(computerPositions);
			ui->computerMonitoringWidget->setUseCustomComputerPositions(true);
			ui->computerMonitoringWidget->doItemsLayout();
		}
	}
}



void MainWindow::saveComputerPositions()
{
	if (const auto fileName = QFileDialog::getSaveFileName(this, tr("Save computer positions"),
														   QDir::homePath(), tr("JSON files (*.json)"));
		!fileName.isEmpty())
	{
		if (QFile file(fileName); file.open(QFile::WriteOnly | QFile::Truncate))
		{
			const auto& computerPositionsProperty = m_master.userConfig().computerPositionsProperty();

			// create structure identical to UserConfig so file with positions can be used as template for UserConfig
			QJsonObject computerPositions;
			computerPositions[QStringLiteral("JsonStoreArray")] = ui->computerMonitoringWidget->savePositions();
			QJsonObject uiConfig;
			uiConfig[computerPositionsProperty.key()] = computerPositions;
			QJsonObject config;
			config[computerPositionsProperty.parentKey()] = uiConfig;
			file.write(QJsonDocument(config).toJson());
		}
	}
}
