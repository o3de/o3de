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
#include <QRect>
#include <QSize>
#include <QStyle>
#endif

class QPainter;
class QStyleOptionButton;
class QWidget;
struct QMetaObject;

namespace AzQtComponents
{
    //! An indicator element of the flattened GUI: the box of a check box, toggle switch or radio button, with
    //! the label beside it. Values are the ones the legacy GUI finally renders (the winners of its stylesheet
    //! cascade, or Qt's own behavior where no rule sets one); each definition names where its values come from.
    //! Sites that render identically in every state share one element, and no element is built from another.
    //!
    //! The legacy box is the indicator's width and height plus its margin on every side (the sheet's
    //! QAbstractButton::indicator rule: Qt asks the box for the metrics, qstylesheetstyle.cpp:5041-5050). The box
    //! sits at the left edge of the widget, centered vertically; the image lies inside its margin; the label
    //! starts after the box and the label spacing (qstylesheetstyle.cpp:6184-6207). A check box or radio button
    //! asks its element for its size (sizeFromContents) and draws through it (draw); a check box answers where a
    //! click lands through it as well (clickRect), a radio button is clicked anywhere in its rectangle (the sheet
    //! returns the widget's rect, qstylesheetstyle.cpp:6215-6220: Style::subElementRect). The sheet also set
    //! `outline: 0` on them: no focus rectangle.
    struct AZ_QT_COMPONENTS_API IndicatorElement
    {
        const char* m_name = "";                //!< Scope.Element.Look
        QSize m_imageSize;                      //!< ::indicator { width; height }
        int m_margin = 0;                       //!< ::indicator { margin }, every side
        QSize m_focusImageSize;                 //!< ::indicator:focus { width; height }
        int m_focusMargin = 0;                  //!< ::indicator:focus { margin }
        //! The box has nothing in it: `::indicator { image: url(...) }` of a file that is no resource (ProjectManager.qss:97-99, the
        //! wizard's steps, `url(:/nothing.png)`, which ProjectManager.qrc does not hold). The image of a rule replaces the images of every
        //! state (the declaration is the rule's own, and a more specific selector outranks the :checked and :unchecked images of the
        //! application sheet), an image that does not load is an icon with no pixmap (qcssparser.cpp:1455-1475, QRenderRule::drawImage,
        //! qstylesheetstyle.cpp:1379-1384) and so the indicator paints nothing. Its width, height and margin still make the box, which
        //! sizes the button and moves its label. The label is drawn as ever.
        bool m_hidden = false;

        // ---- use
        //! The widget's indicator box, margin included
        QSize boxSize() const;
        //! Where the image is drawn, for the widget's rectangle
        QRect imageRect(const QRect& widgetRect, bool focused, Qt::LayoutDirection direction) const;
        //! Where the label goes
        QRect contentsRect(const QRect& widgetRect, int spacing, Qt::LayoutDirection direction) const;
        //! Size hint for the size of the label's contents (its text; the legacy sheet counted the box and the
        //! spacing even without text)
        QSize sizeFromContents(const QSize& contents, int spacing) const;
        //! A check box: the box, and the text of the label with 3px at its sides and 2px above and below
        QRect clickRect(const QStyle* style, const QStyleOptionButton& option, int spacing) const;
        //! The same for a box element around it: the indicator and the label are laid out in the area (its contents rect), the
        //! label's text with its margin is cut by the option's rect
        QRect clickRect(const QStyle* style, const QStyleOptionButton& option, int spacing, const QRect& area) const;

        // ---- resolution: once per widget at polish, kept on the widget
        static const IndicatorElement* resolve(const QWidget* widget);
        static const IndicatorElement* forWidget(const QWidget* widget);

        using Matcher = bool (*)(const QWidget* widget);
        //! The element for the check boxes and radio buttons (widgetClass and its subclasses) that a matcher accepts, before the Global
        //! ones: a module's sheet gave its own indicator rules to widgets of its own (ProjectManager.qss:97-99 `#createAGemLHS
        //! QRadioButton::indicator`). Of the registrations that match a widget the first registered wins. A widget polished before the
        //! call is resolved again (a module builds some windows before it sets up its styles); in the legacy GUI the call only records.
        static void registerElement(const IndicatorElement* element, const QMetaObject* widgetClass, Matcher matcher);

        //! The indicator and the label of a check box or radio button, through the style; false if the widget
        //! has no element
        static bool draw(const QStyle* style, const QStyleOptionButton* option, QPainter* painter, const QWidget* widget);
    };
} // namespace AzQtComponents
