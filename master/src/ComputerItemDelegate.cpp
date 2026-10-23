/*
 * ComputerItemDelegate.cpp - implementation of ComputerItemDelegate

 * Copyright (c) 2025-2026 Tobias Junghans <tobydox@veyon.io>
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

#include <QApplication>
#include <QPainter>

#include "Branding.h"
#include "ComputerControlListModel.h"
#include "ComputerItemDelegate.h"
#include "FeatureManager.h"


ComputerItemDelegate::ComputerItemDelegate(bool fullLengthLabels, QObject* parent) :
	QStyledItemDelegate(parent),
	m_fullLengthLabels(fullLengthLabels)
{
	initFeaturePixmaps();
}



void ComputerItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	if (index.isValid() == false || index.model() == nullptr || painter == nullptr)
	{
		QStyledItemDelegate::paint(painter, option, index);
		return;
	}

	const auto controlInterface = index.model()->data(index, ComputerControlListModel::ControlInterfaceRole).value<ComputerControlInterface::Pointer>();

	// state of the computer. It is drawn as a shape as well as a color, so it can
	// be read without telling green from gray from orange: dot, ring or square
	enum class Indicator { Online, Offline, Problem };
	auto indicator = Indicator::Online;
	if (controlInterface)
	{
		switch (controlInterface->state())
		{
		case ComputerControlInterface::State::Connected:
			break;
		case ComputerControlInterface::State::HostOffline:
		case ComputerControlInterface::State::Disconnected:
		case ComputerControlInterface::State::None:
		case ComputerControlInterface::State::Connecting:
			indicator = Indicator::Offline;
			break;
		default:
			indicator = Indicator::Problem;
			break;
		}
	}

	const bool selected = option.state & QStyle::State_Selected;
	const bool hovered = option.state & QStyle::State_MouseOver;
	const bool dark = option.palette.color(QPalette::Window).lightness() < 128;

	const QRect card = option.rect.adjusted(4, 4, -4, -4);

	// surface of the card: a tone step on hover, the accent tint when selected, no shadow
	QColor surface = dark ? QColor(0x1D, 0x1B, 0x52) : QColor(0xFF, 0xFF, 0xFF);
	if (selected)
	{
		surface = dark ? QColor(0x4A, 0x2F, 0x5A) : Branding::OrangeSoft;
	}
	else if (hovered)
	{
		surface = dark ? QColor(0x2E, 0x2B, 0x7A) : QColor(0xEC, 0xE9, 0xFA);
	}

	painter->save();
	painter->setRenderHint(QPainter::Antialiasing);

	painter->setBrush(surface);
	painter->setPen(QPen(selected ? Branding::OrangeStrong : (dark ? QColor(0x38, 0x34, 0x8A) : QColor(0xDA, 0xD7, 0xEE)),
						 selected ? 3 : 1));
	painter->drawRoundedRect(card, CardRadius, CardRadius);

	painter->restore();

	// let the style draw thumbnail and label inside the card, without its own selection highlight
	QStyleOptionViewItem content(option);
	content.rect = card.adjusted(6, 6, -6, -6);
	content.state &= ~(QStyle::State_Selected | QStyle::State_HasFocus | QStyle::State_MouseOver);
	QStyledItemDelegate::paint(painter, content, index);

	// state indicator (top right)
	painter->save();
	painter->setRenderHint(QPainter::Antialiasing);
	const QRect marker(card.right() - 24, card.top() + 8, 16, 16);
	switch (indicator)
	{
	case Indicator::Online:
		painter->setPen(QPen(surface, 2));
		painter->setBrush(Branding::StatusOnline);
		painter->drawEllipse(marker);
		break;
	case Indicator::Offline:
		painter->setPen(QPen(Branding::StatusOffline, 2));
		painter->setBrush(surface);
		painter->drawEllipse(marker.adjusted(1, 1, -1, -1));
		break;
	case Indicator::Problem:
	{
		painter->setPen(QPen(surface, 2));
		painter->setBrush(Branding::StatusProblem);
		painter->drawRoundedRect(marker, 4, 4);

		auto font = painter->font();
		font.setBold(true);
		font.setPixelSize(11);
		painter->setFont(font);
		painter->setPen(Branding::Navy);
		painter->drawText(marker, Qt::AlignCenter, QStringLiteral("!"));
		break;
	}
	}
	painter->restore();

	drawFeatureIcons(painter, card.topLeft(), controlInterface);
}



QSize ComputerItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	QStyleOptionViewItem opt = option;
	const auto style = QApplication::style();

	if (m_fullLengthLabels && index.model())
	{
		QSize maxSize;
		for (int i = 0; i < index.model()->rowCount(); ++i)
		{
			initStyleOption(&opt, index.model()->index(i, 0));
			const auto rowSize = style->sizeFromContents(QStyle::CT_ItemViewItem, &opt, QSize(), nullptr);
			maxSize = maxSize.expandedTo(rowSize);
		}
		return maxSize + QSize(14, 18);
	}

	initStyleOption(&opt, index);
	const auto size = style->sizeFromContents(QStyle::CT_ItemViewItem, &opt, QSize(), nullptr);

	return {std::min(size.width(), option.decorationSize.width()) + 14, size.height() + 18};
}



void ComputerItemDelegate::initFeaturePixmaps()
{
	for (const auto& feature : VeyonCore::featureManager().features() )
	{
		if (feature.testFlag(Feature::Flag::Master) && !feature.iconUrl().isEmpty())
		{
			m_featurePixmaps[feature.uid()] = QIcon(feature.iconUrl()).pixmap(QSize(OverlayIconSize, OverlayIconSize));
		}
	}
}



void ComputerItemDelegate::drawFeatureIcons(QPainter* painter, const QPoint& pos, ComputerControlInterface::Pointer controlInterface) const
{
	if (painter &&
		controlInterface &&
		controlInterface->state() == ComputerControlInterface::State::Connected)
	{
		auto count = 0;
		for (const auto& feature : controlInterface->activeFeatures())
		{
			if (m_featurePixmaps.contains(feature))
			{
				count++;
			}
		}

		if (count == 0)
		{
			return;
		}

		int x = pos.x() + OverlayIconsPadding;
		const int y = pos.y() + OverlayIconsPadding;

		painter->setRenderHint(QPainter::Antialiasing);
		painter->setBrush(QColor(255, 255, 255, 192));
		painter->setPen(Branding::OrangeStrong);
		painter->drawRoundedRect(QRect(x, y, count * (OverlayIconSize + OverlayIconSpacing), OverlayIconSize),
								 OverlayIconsRadius, OverlayIconsRadius);

		for (const auto& feature : controlInterface->activeFeatures())
		{
			const auto it = m_featurePixmaps.find(feature);
			if (it != m_featurePixmaps.constEnd())
			{
				painter->drawPixmap(QPoint(x, y), *it);
				x += OverlayIconSize + OverlayIconSpacing;
			}
		}
	}
}
