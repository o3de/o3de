/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <AzQtComponents/Components/Elements/Foundations.h>

#include <Components/GridVisualFlat.h>

#include <QApplication>
#include <QColor>
#include <QImage>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QVariant>

#include <memory>

namespace UnitTest
{
    // ----------------------------------------------------------------------------------
    // The grid of a node graph, GridGraphicsItem::CacheStencils. The legacy GUI asks the style the graph resolved for the seven values of
    // the tile; the flattened GUI draws the values of GridVisualFlat, the ones the `graph` rule of the four tool sheets gives (lines 2-14
    // of materialcanvas_style.json, passcanvas_style.json, ScriptCanvas graphcanvas_style.json and LandscapeCanvas graphcanvas_style.json).
    //
    // The module that holds the grid cannot be linked into this test, so the twins are what the legacy GUI draws a tile from: the pens
    // StyleHelper::GetPen builds from the values the parser stores for the rule (quoted below), and the loops of CacheStencils (copied
    // below, and cited). The flat side is the home's values and pens, through the same loops.
    // ----------------------------------------------------------------------------------

    namespace GridVisualFlatTestsInternal
    {
        // The pitches of GridComponent (GridComponent.cpp:52-53)
        constexpr int MajorPitch = 100;
        constexpr int MinorPitch = 20;

        // The `graph` rule of the tool sheets, as the parser stores it (Parser.cpp:696-822): a colour as a QColor; a width as the int of a
        // JSON integer and the double of any other number; a line as a Qt::PenStyle
        const QColor SheetBackground(QStringLiteral("#222222"));
        const QColor SheetMajorColor(QStringLiteral("#282828"));
        const QVariant SheetMajorWidth(1);
        constexpr Qt::PenStyle SheetMajorStyle = Qt::SolidLine;         // "solid"
        const QColor SheetMinorColor(QStringLiteral("#282828"));
        const QVariant SheetMinorWidth(0.5);
        constexpr Qt::PenStyle SheetMinorStyle = Qt::DotLine;           // "dotted"

        //! The pen StyleHelper::GetPen gave the tile (StyleHelper.cpp:223-233, asked with cosmetic true): the width asked as an int, the
        //! cap its fallback (the rule has no `cap-style`)
        QPen legacyPen(const QColor& color, const QVariant& storedWidth, Qt::PenStyle style)
        {
            QPen pen;
            pen.setColor(color);
            pen.setWidth(storedWidth.value<int>());
            pen.setStyle(style);
            pen.setCapStyle(Qt::SquareCap);
            pen.setCosmetic(true);
            return pen;
        }

        //! One tile painted as GridGraphicsItem::CacheStencils paints the first level of detail (GridVisualComponent.cpp:294-338): the
        //! fill, the major lines, then the minor lines that are not on a major line
        QImage paintTile(const QColor& background, const QPen& majorPen, const QPen& minorPen)
        {
            QPixmap stencil(MajorPitch, MajorPitch);
            QPainter painter(&stencil);

            painter.fillRect(0, 0, MajorPitch, MajorPitch, background);

            painter.setPen(majorPen);
            for (int i = 0; i <= MajorPitch; i += MajorPitch)
            {
                painter.drawLine(QPoint(i, 0), QPoint(i, MajorPitch));
            }
            for (int i = 0; i <= MajorPitch; i += MajorPitch)
            {
                painter.drawLine(QPoint(0, i), QPoint(MajorPitch, i));
            }

            painter.setPen(minorPen);
            for (int i = MinorPitch; i < MajorPitch; i += MinorPitch)
            {
                if (i % MajorPitch == 0)
                {
                    continue;
                }
                painter.drawLine(QPoint(i, 0), QPoint(i, MajorPitch));
            }
            for (int i = MinorPitch; i < MajorPitch; i += MinorPitch)
            {
                if (i % MajorPitch == 0)
                {
                    continue;
                }
                painter.drawLine(QPoint(0, i), QPoint(MajorPitch, i));
            }
            painter.end();
            return stencil.toImage();
        }

        //! The pixels of a column that have a colour
        int pixelsOfColumn(const QImage& image, int x, const QColor& color)
        {
            int found = 0;
            for (int y = 0; y < image.height(); ++y)
            {
                found += image.pixelColor(x, y) == color ? 1 : 0;
            }
            return found;
        }

        class GridVisualFlatTests
            : public LeakDetectionFixture
        {
        public:
            GridVisualFlatTests()
            {
                // A pixmap needs an application
                if (!QApplication::instance())
                {
                    m_application = std::make_unique<QApplication>(m_argc, m_argv);
                }
            }

            ~GridVisualFlatTests() override
            {
                m_application.reset();
            }

        protected:
            int m_argc = 1;                                     // QApplication keeps a reference to it
            char m_programName[5] = "test";
            char* m_argv[2] = { m_programName, nullptr };
            std::unique_ptr<QApplication> m_application;
        };
    } // namespace GridVisualFlatTestsInternal

    using GridVisualFlatTestsInternal::GridVisualFlatTests;

    TEST_F(GridVisualFlatTests, Values_AreTheGraphRuleOfTheToolSheets)
    {
        namespace Tests = GridVisualFlatTestsInternal;
        namespace Flat = GraphCanvas::GridVisualFlat;

        EXPECT_EQ(Flat::Background, Tests::SheetBackground);
        EXPECT_EQ(Flat::MajorColor, Tests::SheetMajorColor);
        EXPECT_EQ(Flat::MajorWidth, Tests::SheetMajorWidth.toDouble());
        EXPECT_EQ(Flat::MajorStyle, Tests::SheetMajorStyle);
        EXPECT_EQ(Flat::MinorColor, Tests::SheetMinorColor);
        EXPECT_EQ(Flat::MinorWidth, Tests::SheetMinorWidth.toDouble());
        EXPECT_EQ(Flat::MinorStyle, Tests::SheetMinorStyle);
    }

    TEST_F(GridVisualFlatTests, Background_IsTheBgTier)
    {
        EXPECT_EQ(GraphCanvas::GridVisualFlat::Background, AzQtComponents::Foundations::BG);
    }

    TEST_F(GridVisualFlatTests, Pens_AreThePensTheStyleGave)
    {
        namespace Tests = GridVisualFlatTestsInternal;
        namespace Flat = GraphCanvas::GridVisualFlat;

        // QPen's equality is every fact of a pen: its colour, width, line, caps, joins and whether it is cosmetic
        EXPECT_TRUE(Flat::MajorPen() == Tests::legacyPen(Tests::SheetMajorColor, Tests::SheetMajorWidth, Tests::SheetMajorStyle));
        EXPECT_TRUE(Flat::MinorPen() == Tests::legacyPen(Tests::SheetMinorColor, Tests::SheetMinorWidth, Tests::SheetMinorStyle));
        EXPECT_TRUE(Flat::MajorPen().isCosmetic());
        EXPECT_TRUE(Flat::MinorPen().isCosmetic());
    }

    TEST_F(GridVisualFlatTests, Tile_IsThePixelsTheStylePaintedItFrom)
    {
        namespace Tests = GridVisualFlatTestsInternal;
        namespace Flat = GraphCanvas::GridVisualFlat;

        const QImage legacy = Tests::paintTile(Tests::SheetBackground,
            Tests::legacyPen(Tests::SheetMajorColor, Tests::SheetMajorWidth, Tests::SheetMajorStyle),
            Tests::legacyPen(Tests::SheetMinorColor, Tests::SheetMinorWidth, Tests::SheetMinorStyle));
        const QImage flat = Tests::paintTile(Flat::Background, Flat::MajorPen(), Flat::MinorPen());

        ASSERT_EQ(legacy.size(), flat.size());
        EXPECT_TRUE(legacy == flat);

        // And the tile is a grid: a major line at its edge, a dotted minor line (both colours in its column), a clear middle
        EXPECT_EQ(flat.pixelColor(0, 50), Flat::MajorColor);
        EXPECT_EQ(flat.pixelColor(50, 50), Flat::Background);
        EXPECT_GT(Tests::pixelsOfColumn(flat, Tests::MinorPitch, Flat::MinorColor), 0);
        EXPECT_GT(Tests::pixelsOfColumn(flat, Tests::MinorPitch, Flat::Background), 0);
    }
} // namespace UnitTest
