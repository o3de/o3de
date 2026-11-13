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
#include <QMargins>
#include <QRect>
#include <QSize>
#include <QStyle>
#endif

class QPainter;
class QStyleOptionButton;

namespace AzQtComponents
{
    //! The indicator box of a check box, toggle switch or radio button, and the label beside it. The box is the image, the padding of
    //! the rule (none, but where a module's rule gives the indicator one) and the margin on every side, at the widget's left edge and
    //! centered vertically; the label starts after the box and the label spacing.
    struct AZ_QT_COMPONENTS_API IndicatorElement
    {
        const char* m_name = "";            //!< Scope.Indicator.Look
        QSize m_imageSize;                  //!< The image, at rest
        int m_margin = 0;                   //!< Around the image on every side, at rest
        QSize m_focusImageSize;             //!< The image, focused
        int m_focusMargin = 0;              //!< Around the image on every side, focused
        bool m_hidden = false;              //!< The box draws no image (a rule's image that does not load); it still places the label
        //! `padding` of the rule of the indicator: left, top, right, bottom. Between the margin and the image, on the physical sides
        //! (the box of a right to left check box is mirrored as a whole, its padding is not)
        QMargins m_padding;
        //! `image` of the rule of the indicator ("": the images of the style, one for each state): a resource drawn in every state, fitted
        //! in the m_imageSize rect and centered in it, as the sheet draws the image of a rule (QRenderRule::drawImage)
        const char* m_image = "";

        //! The pseudo-states a state image stands for, as the sheet style reads them from the option (QStyleSheetStyle::pseudoClass)
        enum State : unsigned
        {
            Unchecked = 1,                  //!< QStyle::State_Off
            Checked = 2,                    //!< QStyle::State_On
            Indeterminate = 4,              //!< QStyle::State_NoChange: a partially checked box is neither checked nor unchecked
            Hover = 8,                      //!< enabled and under the pointer
            Focus = 16,                     //!< QStyle::State_HasFocus
            Disabled = 32,                  //!< not QStyle::State_Enabled
        };

        //! The `image` of the rule `QCheckBox::indicator:<states>`
        struct StateImage
        {
            unsigned m_states = 0;          //!< State flags that all hold, as a compound pseudo-class does (0: unused entry)
            const char* m_image = "";       //!< resource, drawn as m_image is
        };

        //! The images the rules of the indicator give the states it names (`QCheckBox::indicator:checked { image }`, one entry for each
        //! pseudo-class compound). Of the entries whose states all hold the one naming the most states is the image (the most
        //! pseudo-classes make the most specific rule), and of equal ones the later: the entries may stand in the order of the sheet,
        //! and it outranks m_image. A state with no entry draws m_image, else the style's own images (CheckBox.qss:30-73): an indicator
        //! that names the checked and unchecked states leaves the partially checked box its own image. The fixed capacity is six.
        StateImage m_stateImages[6];

        //! The image the indicator draws in the state: the entry of m_stateImages whose states all hold and name the most (the later of
        //! equals), else m_image ("": the images of the style)
        const char* imageOf(QStyle::State state) const;

        //! The image of a check box and of a radio button, at rest: CheckBox.qss:25-26, RadioButton.qss:24-25
        //! $size.check-indicator.idle.width.Alt1 $size.check-indicator.idle.height
        static constexpr QSize ImageSize{ 16, 16 };

        // ---- The expander's own box (QCheckBox.Expander, a caret with no label, has no indicator element)
        //! CheckBox.qss:129, :127 $size.check.idle.min-width.Alt1 $size.check.idle.min-height
        static constexpr QSize ExpanderMinSize{ 16, 16 };
        //! CheckBox.qss:130, :128 $size.check.idle.max-width $size.check.idle.max-height
        static constexpr QSize ExpanderMaxSize{ 16, 16 };

        // ---- Geometry
        //! The box: the image and its margin
        QSize boxSize() const;
        //! Where the image is drawn in the widget's rectangle
        QRect imageRect(const QRect& widgetRect, bool focused, Qt::LayoutDirection direction) const;
        //! Where the label goes
        QRect contentsRect(const QRect& widgetRect, int spacing, Qt::LayoutDirection direction) const;
        //! The widget's size for its label's contents: the box and the spacing count even without text
        QSize sizeFromContents(const QSize& contents, int spacing) const;
        //! Where a click lands on a check box laid out in area: the box, and the label's text with the focus rectangle's margins
        QRect clickRect(const QStyle* style, const QStyleOptionButton& option, int spacing, const QRect& area) const;

        // ---- Drawing
        //! The indicator and the label, through the style; false if the widget has no element
        static bool draw(const QStyle* style, const QStyleOptionButton* option, QPainter* painter, const QWidget* widget);

        // ---- Resolution
        //! The widgets of widgetClass the matcher accepts have this element: module rows before Global ones, each in registration order
        static void registerElement(const IndicatorElement* element, const QMetaObject* widgetClass, ElementMatcher matcher,
            ElementScope scope = ElementScope::Module);
        //! From Style::polish: finds the widget's element (none for a check box with a box of its own) and keeps it on the widget
        static void resolve(QWidget* widget);
        //! The kept element, else the one resolve would find (nothing is kept)
        static const IndicatorElement* forWidget(const QWidget* widget);
    };
} // namespace AzQtComponents
