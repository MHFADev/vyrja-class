/*
 * RmmAgentPlugin.h - declaration of RmmAgentPlugin class
 *
 * Copyright (c) 2026 Vyrja Studio
 *
 * This file is part of Vyrja Classroom, a fork of Veyon
 * (https://veyon.io). It is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 */

#pragma once

#include "PluginInterface.h"

// Minimal RMM agent plugin (Phase 3A scaffold, see RMM.md).
//
// The plugin is only built for the RMM edition (WITH_RMM=ON) and is
// intentionally inert for now: all RMM behaviour is gated behind the
// RMM/* configuration properties which default to off. Phase 3B adds
// the telemetry collector and the service bridge client here.
class RmmAgentPlugin : public QObject, PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.RmmAgent")
	Q_INTERFACES(PluginInterface)
public:
	explicit RmmAgentPlugin( QObject* parent = nullptr );
	~RmmAgentPlugin() override = default;

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{ QStringLiteral("db436531-b2ae-4a31-8ad0-271b489e4afe") };
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 0, 1 );
	}

	QString name() const override
	{
		return QStringLiteral( "RmmAgent" );
	}

	QString description() const override
	{
		return tr( "Vyrja RMM agent (experimental, disabled by default via the RMM/* configuration gates)" );
	}

	QString vendor() const override
	{
		return QStringLiteral( "Vyrja Studio" );
	}

	QString copyright() const override
	{
		return QStringLiteral( "Vyrja Studio" );
	}
};
