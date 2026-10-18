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
#include <QColor>
#include <QFrame>
#endif

class QPainter;
class QStyleOptionFrame;
class QWidget;
struct QMetaObject;

namespace AzQtComponents
{
    //! A frame element of the flattened GUI: how the legacy GUI finally drew a frame (a QFrame line, box or panel, the
    //! frame of a scroll area) that its rules gave a background and no border. QStyleSheetStyle drew it in two steps:
    //! the rule's background over the widget (PE_Widget), then the frame by the base style from a palette it derived
    //! from that background (QRenderRule::configurePalette: the light and dark shades of a bevel, the outline of a
    //! panel, the lines of a divider). The element carries what the rules gave the two steps, and draws the second one
    //! the same way, by the base style: a sunken divider has the bevel's two lines, and a frame whose background is
    //! transparent has lines no one can see. A frame whose rule declared a border was drawn by the sheet itself: that
    //! is the box element's.
    //!
    //! Sites that draw identically share one element, and no element is built from another. Global elements are
    //! defined in FrameElement.cpp; a window or feature module defines its own and registers it with a class and a
    //! matcher (registerElement), checked before the Global ones (as TextElement and TabElement work).
    struct AZ_QT_COMPONENTS_API FrameElement
    {
        // ---- the bits of m_fillShapes (bit n is QFrame::Shape n): a [frameShape] selector's list of shapes
        //! `.QFrame[frameShape="1"|"2"|"4"|"5"|"6"]` (BaseStyleSheet.qss:41-45): Box, Panel, HLine, VLine, StyledPanel
        static constexpr unsigned PanelShapes =
            (1u << QFrame::Box) | (1u << QFrame::Panel) | (1u << QFrame::HLine) | (1u << QFrame::VLine) | (1u << QFrame::StyledPanel);
        static constexpr unsigned AnyShape = ~0u;

        const char* m_name = "";                    //!< Scope.Frame.Look
        QColor m_fill = QColor(0, 0, 0, 0);         //!< background-color. '*', BaseStyleSheet.qss:17. TOKENIZE: FrameFill
        unsigned m_fillShapes = 0;                  //!< bit n: the fill covers QFrame::Shape n; a frame of any other shape is as transparent as '*' makes it
        QColor m_foreground = QColor(255, 255, 255); //!< color: what a Plain frame's lines draw in. '*', BaseStyleSheet.qss:18. TOKENIZE: FrameLine

        // ---- use (flattened mode, from Style::drawControl for CE_ShapedFrame)
        //! The fill of a frame of this shape: m_fill, or transparent
        QColor fillFor(int frameShape) const;
        //! The look the legacy GUI gave the frame: the background fills the widget, and the option carries the palette
        //! the base style draws the frame from (Window, Base and Button the fill, Light, Midlight, Dark and Shadow its
        //! shades as QRenderRule::configurePalette derives them, WindowText and Text the foreground).
        //! A widget still drawn through a style sheet keeps what the sheet gave it (see FrameElement.cpp).
        void apply(QStyleOptionFrame* option, QPainter* painter, const QWidget* widget) const;

        // ---- resolution: once per widget at polish, kept on the widget
        using Matcher = bool (*)(const QWidget* widget);
        //! The widgets of widgetClass that the matcher picks have this element, before the Global ones. Widgets
        //! polished before a registration are resolved again, so the order of calls does not matter. The class only
        //! narrows the lookup (a selector by name or tag names no class): give the narrowest, as every widget of it is
        //! asked once at polish, and keeps the answer.
        static void registerElement(const FrameElement* element, const QMetaObject* widgetClass, Matcher matcher);
        //! From Style::polish(QWidget*): the element of the widget, or null for most widgets (a label, a button, a menu)
        static const FrameElement* resolve(const QWidget* widget);
        //! From Style::drawControl: what resolve found, with no lookup per paint
        static const FrameElement* forFrame(const QWidget* widget);
    };
} // namespace AzQtComponents
