/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/Math/Color.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <AzQtComponents/Components/GuiPerfFloor.h>
#include <AzQtComponents/Tests/SheetTwins.h>
#include <AzQtComponents/Tests/SiteTwins.h>

#include <Editor/View/Widgets/CanvasWidget.h>

#include <QApplication>
#include <QFrame>
#include <QStringList>
#include <QWidget>

namespace UnitTest
{
    // ----------------------------------------------------------------------------------
    // C184, CanvasWidget.cpp: the frame around the graph view of each graph tab (#graphicsViewFrame, CanvasWidget.ui:39-83), a 5px
    // ring of the colour SetDefaultBorderColor is given (SourceDescription::GetDisplayColor(), Core.h:328, which GetR8 and the others
    // make rgb(127,127,127)). The legacy GUI sets the sheet `QFrame#graphicsViewFrame { background-color: rgb(%1,%2,%3) }`; the
    // flattened GUI hands the colour to the palette of the frame, which fills itself with it (autoFillBackground). The twins are the
    // real CanvasWidget with the colour of the tabs. The roles compared are the fill's (Base, Button and the background role, Window:
    // what the sheet's background set), since the frame is a plain fill.
    // ----------------------------------------------------------------------------------

    namespace CanvasWidgetFlatSheetTestsInternal
    {
        namespace Twins = AzQtComponents::SheetTwins;

        class CanvasWidgetFlatSheetTests
            : public LeakDetectionFixture
        {
        protected:
            Twins::Context m_context;
        };

        //! The sheet SetDefaultBorderColor sets for the colour of the tabs (CanvasWidget.cpp:72 before the flattening)
        constexpr const char* FrameSheet = "QFrame#graphicsViewFrame { background-color: rgb(127,127,127) }";

        //! The colour the tabs pass (GraphTabBar.cpp:81 and :154)
        AZ::Color displayColor()
        {
            return AZ::Color(0.5f, 0.5f, 0.5f, 0.5f);
        }

        //! A graph tab's canvas widget as the tab bar makes it, and its frame
        QWidget* makeFrame(QWidget* parent)
        {
            auto* canvas = aznew ScriptCanvasEditor::Widget::CanvasWidget(ScriptCanvas::SourceHandle(), parent);
            canvas->SetDefaultBorderColor(displayColor());
            return canvas->findChild<QFrame*>(QStringLiteral("graphicsViewFrame"));
        }

        Twins::Row frameRow(const char* site)
        {
            Twins::Row row;
            row.m_site = site;
            row.m_make = &makeFrame;
            row.m_sheets = { { "CanvasWidget.cpp:72 before the flattening", Twins::SetOn::Widget, FrameSheet } };
            // The graph view inside the frame is under the application's sheet in the real GUI, which makes its viewport clear: the
            // frame's fill shows through it (no scene is set in the twin), and the whole picture is that one colour
            row.m_realApplicationSheet = true;
            row.m_compare = Twins::Compare::Geometry | Twins::Compare::Palette | Twins::Compare::Pixels;
            row.m_roles = { QPalette::Base, QPalette::Button, QPalette::Window };
            row.m_size = QSize(120, 90);
            row.m_base = Twins::Base::AzQtComponentsStyle;
            row.m_mayDrawNothing = true;
            return row;
        }

        //! The sheets of the widgets a tab's canvas widget holds, one line each, as the census lists them in a rung
        QStringList censusOfCanvasWidget(AzQtComponents::GuiPerfFloor::Rung rung)
        {
            return AzQtComponents::SiteTwins::sheetLinesAt(rung,
                []
                {
                    QWidget host;
                    host.resize(300, 200);
                    makeFrame(&host);
                    host.setAttribute(Qt::WA_DontShowOnScreen);
                    host.show();
                    for (QWidget* each : host.findChildren<QWidget*>())
                    {
                        each->ensurePolished();
                    }
                });
        }
    } // namespace CanvasWidgetFlatSheetTestsInternal

    using CanvasWidgetFlatSheetTestsInternal::CanvasWidgetFlatSheetTests;
    namespace CanvasFrame = CanvasWidgetFlatSheetTestsInternal;

    TEST_F(CanvasWidgetFlatSheetTests, Frame_FillsItselfWithTheColourOfTheTabsAsTheSheetFilledItAtEveryRatio)
    {
        EXPECT_TRUE(CanvasFrame::Twins::check(CanvasFrame::frameRow("C184 the frame around a graph view"), m_context));
    }

    TEST_F(CanvasWidgetFlatSheetTests, Frame_TheLegacyGuiSetsTheSheetAndTheFlattenedGuiNone)
    {
        const QStringList legacy = CanvasFrame::censusOfCanvasWidget(AzQtComponents::GuiPerfFloor::Rung::Qss);
        bool foundFrame = false;
        for (const QString& line : legacy)
        {
            foundFrame = foundFrame
                || (AzQtComponents::SiteTwins::namesCarrier(line, QStringLiteral("QFrame#graphicsViewFrame"))
                    && line.contains(QStringLiteral("rgb(127,127,127)")));
        }
        EXPECT_TRUE(foundFrame) << legacy.join("\n").toStdString();

        const QStringList flat = CanvasFrame::censusOfCanvasWidget(AzQtComponents::GuiPerfFloor::Rung::Flat);
        EXPECT_TRUE(flat.isEmpty()) << flat.join("\n").toStdString();
    }
} // namespace UnitTest
