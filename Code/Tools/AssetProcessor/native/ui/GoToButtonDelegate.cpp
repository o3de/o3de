/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "GoToButtonDelegate.h"
#include <qpainter.h>
#include <qevent.h>

namespace AssetProcessor
{
    GoToButtonDelegate::GoToButtonDelegate(QObject* parent)
        : QStyledItemDelegate(parent)
        , m_icon(":/AssetProcessor_goto.svg")
        , m_hoverIcon(":/AssetProcessor_goto_hover.svg")
    {

    }

    void GoToButtonDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        QStyledItemDelegate::paint(painter, option, index);

        if (index.data().canConvert<GoToButtonData>())
        {
            constexpr int MarginPX = 3;
            auto marginRect = option.rect.marginsRemoved(QMargins(MarginPX, MarginPX, MarginPX, MarginPX));

            if (option.state & QStyle::State_MouseOver)
            {
                m_hoverIcon.paint(painter, marginRect);
            }
            else
            {
                m_icon.paint(painter, marginRect);
            }
        }
    }

    bool GoToButtonDelegate::editorEvent(
        QEvent* event, QAbstractItemModel* /*model*/, const QStyleOptionViewItem& /*option*/, const QModelIndex& index)
    {
        if (index.data().canConvert<GoToButtonData>())
        {
            if (event->type() == QEvent::MouseButtonPress)
            {
                Q_EMIT Clicked(index.data().value<GoToButtonData>());
            }
        }

        return false;
    }
}

