/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Inspector/InspectorGroupHeaderWidget.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/Widgets/Text.h>

#include <QApplication>
#include <QPainter>
#include <QPixmap>
#include <QStyle>
#include <QStyleOptionViewItem>

namespace AtomToolsFramework
{
    InspectorGroupHeaderWidget::InspectorGroupHeaderWidget(QWidget* parent)
        : ExtendedLabel(parent)
    {
        AzQtComponents::Text::addPrimaryStyle(this);
        AzQtComponents::Text::addLabelStyle(this);
        setStyleSheet("background-color: #333333; border-style: solid; border-color: #1B1B1B; border-width: 1px; border-left: none; border-right: none;");
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        setFixedHeight(24);
        setMargin(0);
    }

    void InspectorGroupHeaderWidget::SetExpanded(bool expand)
    {
        if (m_expanded != expand)
        {
            m_expanded = expand;
            if (m_expanded)
            {
                emit expanded();
            }
            else
            {
                emit collapsed();
            }
            update();
        }
    }

    bool InspectorGroupHeaderWidget::IsExpanded() const
    {
        return m_expanded;
    }

    void InspectorGroupHeaderWidget::mousePressEvent(QMouseEvent* event)
    {
        emit clicked(event);
    }

    void InspectorGroupHeaderWidget::paintEvent([[maybe_unused]] QPaintEvent* event)
    {
        QPainter painter(this);
        QStyle* style = QApplication::style();
        QSize iconSize(16, 16);
        auto& icon = m_expanded ? m_iconExpanded : m_iconCollapsed;

        const QRect iconRect(5, (geometry().height() / 2) - (iconSize.height() / 2), iconSize.width(), iconSize.height());
        style->drawItemPixmap(&painter, iconRect, Qt::AlignLeft | Qt::AlignVCenter, icon.scaledToWidth(iconSize.width()));

        const auto textRect = QRect(25, 0, geometry().width() - 21, geometry().height());
        style->drawItemText(&painter, textRect, Qt::AlignLeft | Qt::AlignVCenter, QPalette(), true, text(), QPalette::HighlightedText);
    }
} // namespace AtomToolsFramework

#include <AtomToolsFramework/Inspector/moc_InspectorGroupHeaderWidget.cpp>
