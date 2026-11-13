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
#include <AzQtComponents/Components/Elements/TextElement.h>
#include <QColor>
#endif

namespace AzQtComponents
{
    //! What an inline style sheet gave the tooltip of a widget it reached, and the widget the tooltip is shown for.
    //!
    //! A tooltip (Qt's QTipLabel) is a window of its own, and the legacy sheet style resolved its rules through the widget it was
    //! shown for (QTipLabel::placeTip sets `_q_stylesheet_parent`, qstylesheetstyle.cpp parentObject): the application's `QToolTip`
    //! rules (ToolTip.qss), then the sheet of that widget and of each widget above it. A sheet with no selector is `* { ... }`, and
    //! reached the tip of its widget and of every widget below it; so did a descendant rule `X *` of a widget-level sheet (Editor.qss:63
    //! `Ui--AssetEditorHeader *`, which no look carries yet). A sheet with a type selector did not: the sheet style names a tooltip
    //! "QToolTip" and nothing else, so a `QLabel` or `QFrame` rule never matched one.
    //!
    //! The flattened GUI has no sheet to resolve through, and Qt hangs the tip on no widget (QToolTip::showText: no parent on
    //! Windows). The owner is found here instead: Style's application filter notes the widget each QEvent::ToolTip is delivered
    //! to, before the widget shows its tip, and the tip takes it at its polish. A look is registered with a matcher on the widget
    //! the sheet was set on; it reaches the tip of that widget and of the widgets below it, as the sheet did.
    struct AZ_QT_COMPONENTS_API ToolTipElement
    {
        const char* m_name = "";                //!< Scope.ToolTip.Look
        QColor m_fill;                          //!< `background-color`: the tip's fill; invalid: ToolTip.qss's (the palette's ToolTipBase)
        //! `font` and `color`: the text of the tip, as a text element gives it to a widget; none: ToolTip.qss's
        const TextElement* m_text = nullptr;

        //! What the looks of an owner and of the widgets above it give a tip: the nearest widget's fill and the nearest widget's text
        struct Look
        {
            QColor m_fill;
            const TextElement* m_text = nullptr;

            bool operator==(const Look& other) const
            {
                return m_fill == other.m_fill && m_text == other.m_text;
            }
            bool operator!=(const Look& other) const
            {
                return !(*this == other);
            }
        };

        // ---- Resolution
        //! The tips shown for a widget of ownerClass the matcher accepts, and for the widgets below it, have this look. Of the rows
        //! that match a widget the first wins: a module's before a Global one, each in registration order.
        static void registerElement(const ToolTipElement* element, const QMetaObject* ownerClass, ElementMatcher matcher,
            ElementScope scope = ElementScope::Module);
        //! The look of a tip shown for the owner (none: an empty look)
        static Look lookFor(const QWidget* owner);

        // ---- The widget a tip is shown for
        //! Qt's tooltip label
        static bool isToolTip(const QWidget* widget);
        //! From Style's application filter, for each QEvent::ToolTip, before the widget handles it: the widget is where the tip that
        //! event shows will be shown for (a viewport is its scroll area: QAbstractScrollArea::viewportEvent hands the event to the
        //! area, which shows the tip). Returns that owner.
        static QWidget* noteHelpEvent(QWidget* receiver);
        //! The tip on screen, if there is one
        static QWidget* visibleTip();
        //! From Style::polish, first: the tip takes the widget it is shown for, its parent where Qt gives it one, else the one the
        //! help event being handled noted, and the look of that widget as it is now
        static void claim(QWidget* tip);
        //! The widget a tip is shown for: the one it claimed, else its parent, else the one the help event being handled noted. None:
        //! the tip was shown by code that was not answering a help event, and Qt gives the tip no parent (Windows).
        static const QWidget* ownerOf(const QWidget* tip);
        //! The look a tip was made with: its owner's at the time the tip claimed it (a look can change after, as a link's does when
        //! the pointer leaves it, while the tip stays up for a moment). A tip that was not claimed has its owner's look as it is. None
        //! (an empty look): the tip has no owner, so it is the plain tip of ToolTip.qss.
        static Look lookOf(const QWidget* tip);
        //! The text element of the look of a tip (none: ToolTip.qss's), for TextElement::resolve
        static const TextElement* textOf(const QWidget* tip);
        //! From Style::polish, last: the fill of the look in the roles QRenderRule::configurePalette gave it, which PE_PanelTipLabel
        //! and the window behind the tip's margin read
        static void applyPalette(QWidget* tip);
    };
} // namespace AzQtComponents
