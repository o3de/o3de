/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/PlatformDef.h>

AZ_PUSH_DISABLE_WARNING(4251 4800 4244, "-Wunknown-warning-option")
#include <QColor>
#include <QPen>
#include <QVariant>
AZ_POP_DISABLE_WARNING

#include <AzQtComponents/Components/Elements/Foundations.h>

//=========================================================================
// GRAPH GRID LOOKS (the background of a node graph, and the lines of its grid)
//
// GridGraphicsItem::CacheStencils paints one tile of the grid: a fill, solid major lines and dotted minor lines. It asked the style the
// graph resolved for the seven values of the tile, which the tool sheets give in the `graph` rule of their JSON, and the legacy GUI
// still does. The flattened GUI asks no style: it draws the values below, the ones the shipped tool sheets give. The sheets are those
// of Material Canvas, Pass Canvas, Script Canvas and Landscape Canvas (every GraphCanvas tool in the tree loads one of them), and the
// `graph` rule is the same in the four, at the same lines (:2-14). The pitches of the lines are not here: they are fields of
// GridComponent (MajorPitch and MinorPitch), which the user of a graph edits.
//
// A graph sheet that a user writes, or the fallback sheet of GraphCanvas (default_style.json:2-14, which gives a background of
// #222326, lines of #6D7072 and #969B9E, widths of 0.5 and 0.4), is run-time data that no static look can hold: the flattened GUI does
// not read the `graph` rule of any sheet, and draws these values. The legacy GUI (style sheets on) is what such a sheet keeps working in.
//
// This home depends on Qt and AzQtComponents only, so that the tests of GraphCanvas can compile it and twin every pen.
//=========================================================================

namespace GraphCanvas
{
    //! Everything of the grid's flattened looks. A named namespace: a unity build joins files into one translation unit.
    namespace GridVisualFlat
    {
        // ---- Values (the `graph` rule: materialcanvas_style.json:3-13, passcanvas_style.json:3-13, ScriptCanvas graphcanvas_style.json:3-13
        // and LandscapeCanvas graphcanvas_style.json:3-13, the same lines in the four)

        //! `background-color` (:5): the BG tier of the grey ladder, which is #222222. TOKENIZE: GraphGridBackground
        inline constexpr QColor Background = AzQtComponents::Foundations::BG;

        //! `grid-major-color` (:7). TOKENIZE: GraphGridMajorColor
        inline constexpr QColor MajorColor(0x28, 0x28, 0x28);

        //! `grid-major-width` (:8). TOKENIZE: GraphGridMajorWidth
        inline constexpr qreal MajorWidth = 1.0;

        //! `grid-major-style` (:9), `solid`. TOKENIZE: GraphGridMajorStyle
        inline constexpr Qt::PenStyle MajorStyle = Qt::SolidLine;

        //! `grid-minor-color` (:11). TOKENIZE: GraphGridMinorColor
        inline constexpr QColor MinorColor(0x28, 0x28, 0x28);

        //! `grid-minor-width` (:12). TOKENIZE: GraphGridMinorWidth
        inline constexpr qreal MinorWidth = 0.5;

        //! `grid-minor-style` (:13), `dotted`. TOKENIZE: GraphGridMinorStyle
        inline constexpr Qt::PenStyle MinorStyle = Qt::DotLine;

        // ---- The pens

        //! The pen of a line of the grid as StyleHelper::GetPen made it for the tile (StyleHelper.cpp:223-233, asked with cosmetic true):
        //! the colour, the line, a cosmetic pen (always a pixel wide, whatever the transform of the painter), the square cap that is
        //! GetPen's fallback (no `cap-style` in the rule), and a width of whole pixels. GetPen asks the width of the style as an int, so
        //! the 0.5 of the minor lines is the int that QVariant makes of it, which this takes the same way.
        inline QPen LinePen(const QColor& color, qreal width, Qt::PenStyle style)
        {
            QPen pen;
            pen.setColor(color);
            pen.setWidth(QVariant(width).toInt());
            pen.setStyle(style);
            pen.setCapStyle(Qt::SquareCap);
            pen.setCosmetic(true);
            return pen;
        }

        //! The pen of the major lines
        inline QPen MajorPen()
        {
            return LinePen(MajorColor, MajorWidth, MajorStyle);
        }

        //! The pen of the minor lines
        inline QPen MinorPen()
        {
            return LinePen(MinorColor, MinorWidth, MinorStyle);
        }
    } // namespace GridVisualFlat
} // namespace GraphCanvas
