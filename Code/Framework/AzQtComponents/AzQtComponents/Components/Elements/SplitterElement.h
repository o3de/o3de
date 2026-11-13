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
#include <QStyle>
#endif

class QPainter;
class QStyleOption;

namespace AzQtComponents
{
    //! The handle of a QSplitter, per splitter: what `QSplitter::handle` gave it, in the sheets of the application (Splitter.qss) and in
    //! the sheet a splitter sets on itself. The handle is a sub-control of the splitter, so the rule is read through the splitter
    //! (CE_Splitter and PM_SplitterWidth are asked with the splitter as the widget).
    //!
    //! The Global element is Splitter.qss; a module registers its own for the splitters its sheet restyles (registerElement), checked
    //! before it. An element is the whole definition of its handle: a fill it leaves out is not the Global one.
    struct AZ_QT_COMPONENTS_API SplitterElement
    {
        const char* m_name = "";                //!< Scope.Splitter.Look

        //! The width of the handle: the pixels between the panes, whatever the splitter's orientation. QSplitter asks PM_SplitterWidth with
        //! no option, and the sheet style then read the rule of the handle in the state of a vertical one, and its `height`
        //! (QStyleSheetStyle::pixelMetric): `QSplitter::handle { height: 4px }` is the width of the handle of a horizontal splitter too.
        int m_thickness = 0;
        //! `background` of the handle at rest. A colour with no alpha is a background the rule has and shows nothing (`transparent`: the
        //! splitter behind the handle shows, and the base style does not draw it); an invalid colour is a rule with no background: the
        //! base style draws the handle.
        QColor m_fill;
        //! `background` of `QSplitter::handle:hover`, with the pointer over an enabled handle; invalid: the rule of the handle at rest
        QColor m_hoverFill;

        // ---- Drawing
        //! The fill of the state: the hover fill while an enabled handle is under the pointer, else the one at rest. Invalid: none.
        QColor fillOf(const QStyleOption* option) const;
        //! CE_Splitter: the handle's rect filled as a rule fills its background (QRenderRule::drawBackground); nothing for a colour that
        //! shows nothing. False: the rule has no background, and the base style draws the handle.
        bool draw(QPainter* painter, const QStyleOption* option) const;

        // ---- Resolution
        //! The splitters the matcher accepts have this element: module rows before the Global one, each in registration order
        static void registerElement(const SplitterElement* element, const QMetaObject* widgetClass, ElementMatcher matcher,
            ElementScope scope = ElementScope::Module);
        //! From Style::polish: finds the splitter's element and keeps it. When the width of its handles changed, the splitter lays them
        //! out again, as the style change of a sheet made it do.
        static void resolve(QWidget* widget);
        //! The kept element, else the one resolve would find (nothing is kept). The Global one for a widget that is no splitter.
        static const SplitterElement& forWidget(const QWidget* widget);
    };
} // namespace AzQtComponents
