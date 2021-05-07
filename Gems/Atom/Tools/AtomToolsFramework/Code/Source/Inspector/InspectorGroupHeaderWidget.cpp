/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

    void InspectorGroupHeaderWidget::SetExpanded(bool expanded)
    {
        m_expanded = expanded;
        update();
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
