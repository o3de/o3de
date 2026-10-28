/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/AzQtComponentsAPI.h>
#include <AzQtComponents/Components/Elements/ElementCommon.h>
#include <QColor>
#include <QFrame>
#endif

class QPainter;
class QStyleOptionFrame;

namespace AzQtComponents
{
    //! The look of a QFrame, or a scroll area's frame, that its rules gave a background and no border: the background over the
    //! widget and the palette the base style draws the frame's lines from. A frame whose rule has a border is a BoxElement.
    struct AZ_QT_COMPONENTS_API FrameElement
    {
        //! The shapes `.QFrame[frameShape="1"|"2"|"4"|"5"|"6"]` covers: Box, Panel, HLine, VLine, StyledPanel
        static constexpr unsigned PanelShapes =
            (1u << QFrame::Box) | (1u << QFrame::Panel) | (1u << QFrame::HLine) | (1u << QFrame::VLine) | (1u << QFrame::StyledPanel);

        const char* m_name = "";                        //!< Scope.Frame.Look
        QColor m_fill = QColor(0x00, 0x00, 0x00, 0x00); //!< The background. BaseStyleSheet.qss:17 $fill.surface.idle.Clear
        unsigned m_fillShapes = 0;                      //!< Bit n: the background covers QFrame::Shape n
        QColor m_foreground = QColor(0xFF, 0xFF, 0xFF); //!< The lines of a Plain frame. BaseStyleSheet.qss:18 $text.surface.idle.Alt1

        // ---- Drawing
        //! CE_ShapedFrame: paints the background and gives the option the palette the base style draws the frame from
        void drawBackground(QStyleOptionFrame* option, QPainter* painter, const QWidget* widget) const;

        // ---- Resolution
        //! The widgets of widgetClass the matcher accepts have this element: module rows before Global ones, each in registration order
        static void registerElement(const FrameElement* element, const QMetaObject* widgetClass, ElementMatcher matcher,
            ElementScope scope = ElementScope::Module);
        //! From Style::polish: finds the widget's element (none for most widgets) and keeps it on the widget
        static void resolve(QWidget* widget);
        //! The kept element, else the one resolve would find (nothing is kept)
        static const FrameElement* forWidget(const QWidget* widget);
    };
} // namespace AzQtComponents
