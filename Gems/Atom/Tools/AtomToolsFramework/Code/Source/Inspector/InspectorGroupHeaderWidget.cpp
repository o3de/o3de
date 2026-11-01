/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Inspector/InspectorGroupHeaderWidget.h>
#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/Widgets/Text.h>

#include <QApplication>
#include <QPainter>
#include <QPixmap>
#include <QStyle>
#include <QStyleOptionViewItem>

namespace AtomToolsFramework
{
    //! Everything private to this file. A named namespace, not an anonymous one: a unity build joins files into one
    //! translation unit, where anonymous namespaces merge.
    namespace InspectorGroupHeaderWidgetInternal
    {
        // ------------------------------------------------------------------------------
        // Flattened GUI: the box of the header. The legacy sheets gave it, together, the vertical margin of every label
        // (`.Label, QLabel`, Text.qss:89-95) and its own sheet below, set in the constructor (a fill, and a line above and below
        // it). That sheet still runs in the flattened GUI, and QStyleSheetStyle draws what its rule has and leaves nothing to the
        // style beneath (qstylesheetstyle.cpp:4681-4727): it drew the line at the widget's edge and missed the margin. So the element
        // carries both and the constructor sets no sheet. The widget draws its icon and text itself (paintEvent).
        // ------------------------------------------------------------------------------

        const AzQtComponents::BoxElement GroupHeader{
            .m_name = "AtomTools.Box.InspectorGroupHeader",
            .m_margin = QMargins(0, 1, 0, 1),                                       // margin-top, margin-bottom: Text.qss:93, :94
            .m_nativeBorder = false,                                                // border-style solid: a border of its own (:74)
            .m_idle = {
                .m_fill = QBrush(QColor(0x33, 0x33, 0x33)),                         // background-color (:74). TOKENIZE: InspectorGroupHeaderFill
                // border-color #1B1B1B, border-width 1px; border-left and border-right none (:74). TOKENIZE: InspectorGroupHeaderBorder
                .m_edges = { { 1, QColor(0x1B, 0x1B, 0x1B) }, {}, { 1, QColor(0x1B, 0x1B, 0x1B) }, {} },
            },
        };

        bool isGroupHeader(const QWidget*)
        {
            return true;                // the class is the selector: every InspectorGroupHeaderWidget set this sheet
        }
    } // namespace InspectorGroupHeaderWidgetInternal

    void InspectorGroupHeaderWidget::RegisterFlatElement()
    {
        static bool registered = false;
        if (!registered)
        {
            registered = true;
            AzQtComponents::BoxElement::registerElement(
                &InspectorGroupHeaderWidgetInternal::GroupHeader, &InspectorGroupHeaderWidget::staticMetaObject,
                &InspectorGroupHeaderWidgetInternal::isGroupHeader);
        }
    }

    InspectorGroupHeaderWidget::InspectorGroupHeaderWidget(QWidget* parent)
        : ExtendedLabel(parent)
    {
        AzQtComponents::Text::addPrimaryStyle(this);
        AzQtComponents::Text::addLabelStyle(this);
        if (AzQtComponents::StyleManager::stylesheetsDisabled())
        {
            RegisterFlatElement();
        }
        else
        {
            setStyleSheet("background-color: #333333; border-style: solid; border-color: #1B1B1B; border-width: 1px; border-left: none; border-right: none;");
        }
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        setFixedHeight(24);
        setContentsMargins(0, 0, 0, 0);
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

