/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/GuiPerfFloor.h>
#include <AzQtComponents/Tests/SheetTwins.h>
#include <AzQtComponents/Tests/SiteTwins.h>

#include <Editor/View/Windows/Tools/UpgradeTool/Controller.h>

#include <QApplication>
#include <QFrame>
#include <QProgressBar>
#include <QStringList>
#include <QWidget>

namespace UnitTest
{
    // ----------------------------------------------------------------------------------
    // U113, UpgradeTool/View.ui:198-200 (before the flattening): the one bare declaration, `background-color: rgb(47, 47, 47)`, that the
    // form gave the progress frame, which reached the frame and every widget below it. The legacy GUI sets it through
    // StyleManager::setLegacyStyleSheet after setupUi; the flattened GUI sets none, and the elements of Controller::RegisterFlatElements
    // draw the frame and the track of its bar. The twins are the real Controller (the form is built by its constructor, which
    // registers the elements and sets the sheet). The legacy twin of the bar has the sheet the site sets on the frame, which reaches
    // the bar as it did, and the real application sheet (ProgressBar.qss), which the sheet's fill outranks.
    //
    // The roles compared are the text's: the legacy sheet's fill made the Base, Button and Window of the frame, and of the widgets
    // below it, that colour, which nothing there paints with (the spinner and the labels are clear, the bar is drawn by its element).
    // ----------------------------------------------------------------------------------

    namespace UpgradeToolFlatSheetTestsInternal
    {
        namespace Twins = AzQtComponents::SheetTwins;
        using ScriptCanvasEditor::VersionExplorer::Controller;

        class UpgradeToolFlatSheetTests
            : public LeakDetectionFixture
        {
        protected:
            Twins::Context m_context;
        };

        //! The sheet of View.ui:198-200, which the legacy twin of the frame has on the frame
        constexpr const char* FormFrameSheet = "background-color: rgb(47, 47, 47);";

        //! The dialog as MainWindow::RunUpgradeTool makes it, shown so that the form is laid out
        Controller* makeController(QWidget* parent)
        {
            auto* controller = aznew Controller(parent);
            controller->setAttribute(Qt::WA_DontShowOnScreen);
            controller->show();
            return controller;
        }

        QWidget* makeFrame(QWidget* parent)
        {
            return makeController(parent)->findChild<QFrame*>(QStringLiteral("progressFrame"));
        }

        //! The bar is hidden until a scan or an upgrade (Controller.cpp:55) and shows a value while one runs: a value of its range, so that
        //! two readings of one bar do not differ by the moment they were taken
        QWidget* makeBar(QWidget* parent)
        {
            auto* bar = makeController(parent)->findChild<QProgressBar*>(QStringLiteral("progressBar"));
            if (bar)
            {
                bar->setRange(0, 100);
                bar->setValue(40);
                // The maximum height that Style::polish gives a flat bar and no legacy bar has: the form's vertical policy (Fixed) holds
                // the real bar to the height of its hint, 5px, in both GUIs, and the legacy twin gets the same number as the flat one
                bar->setMaximumHeight(5);
                bar->setVisible(true);
            }
            return bar;
        }

        Twins::Row frameRow(const char* site)
        {
            Twins::Row row;
            row.m_site = site;
            row.m_make = &makeFrame;
            row.m_sheets = { { "View.ui:198-200 before the flattening", Twins::SetOn::Widget, FormFrameSheet } };
            row.m_realApplicationSheet = true;
            row.m_register = &Controller::RegisterFlatElements;
            row.m_compare = Twins::Compare::Geometry | Twins::Compare::Palette | Twins::Compare::Pixels;
            row.m_roles = { QPalette::WindowText, QPalette::Text, QPalette::ButtonText };
            // The size the dialog's layout gives the frame in both GUIs (the factory shows the dialog at its minimum width, 240px, and
            // the layout stretches the frame across it); the harness would else size the frame to its hint, and the legacy twin's layout
            // lays it out again at that width
            row.m_size = QSize(230, 42);
            row.m_base = Twins::Base::AzQtComponentsStyle;
            // The site sets the form's sheet itself in the legacy twin (the factory is its code), so the twin without the row's sheets
            // has it too; and the frame is a plain fill, a picture of one colour
            row.m_mayDecideNothing = true;
            row.m_mayDrawNothing = true;
            return row;
        }

        //! The bar: the sheet is the frame's, which the site sets (the factory runs the site's code), over the application's
        Twins::Row barRow(const char* site)
        {
            Twins::Row row;
            row.m_site = site;
            row.m_make = &makeBar;
            row.m_realApplicationSheet = true;
            row.m_register = &Controller::RegisterFlatElements;
            row.m_compare = Twins::Compare::Geometry | Twins::Compare::Fonts | Twins::Compare::Palette | Twins::Compare::Pixels;
            row.m_roles = { QPalette::WindowText, QPalette::Text, QPalette::ButtonText };
            row.m_base = Twins::Base::AzQtComponentsStyle;
            return row;
        }

        //! The sheets of the widgets the dialog holds, one line each, as the census lists them in a rung
        QStringList censusOfController(AzQtComponents::GuiPerfFloor::Rung rung)
        {
            return AzQtComponents::SiteTwins::sheetLinesAt(rung,
                []
                {
                    QWidget host;
                    host.resize(300, 200);
                    makeController(&host);
                    host.setAttribute(Qt::WA_DontShowOnScreen);
                    host.show();
                    for (QWidget* each : host.findChildren<QWidget*>())
                    {
                        each->ensurePolished();
                    }
                });
        }
    } // namespace UpgradeToolFlatSheetTestsInternal

    using UpgradeToolFlatSheetTestsInternal::UpgradeToolFlatSheetTests;
    namespace UpgradeTool = UpgradeToolFlatSheetTestsInternal;

    TEST_F(UpgradeToolFlatSheetTests, Frame_HasTheFillOfTheFormsSheetAtEveryRatio)
    {
        EXPECT_TRUE(UpgradeTool::Twins::check(UpgradeTool::frameRow("U113 the progress frame"), m_context));
    }

    TEST_F(UpgradeToolFlatSheetTests, Bar_HasTheTrackOfTheFormsSheetAndTheChunkOfTheApplicationAtEveryRatio)
    {
        EXPECT_TRUE(UpgradeTool::Twins::check(UpgradeTool::barRow("U113 the progress bar below the frame"), m_context));
    }

    TEST_F(UpgradeToolFlatSheetTests, Elements_AreThoseOfTheFrameAndItsBarAndNoOtherWidgetOfTheSameNames)
    {
        UpgradeTool::Twins::RungScope flat(UpgradeTool::Twins::Rung::Flat);

        QWidget root;
        UpgradeTool::Controller* controller = UpgradeTool::makeController(&root);
        auto* frame = controller->findChild<QFrame*>(QStringLiteral("progressFrame"));
        auto* bar = controller->findChild<QProgressBar*>(QStringLiteral("progressBar"));
        ASSERT_NE(frame, nullptr);
        ASSERT_NE(bar, nullptr);

        // A frame of another name, and a bar of the name that is not below the dialog's frame (progressBar is also the name of the bars
        // of two other forms)
        auto* otherFrame = new QFrame(&root);
        otherFrame->setObjectName(QStringLiteral("someOtherFrame"));
        auto* otherBar = new QProgressBar(&root);
        otherBar->setObjectName(QStringLiteral("progressBar"));

        for (QWidget* each : root.findChildren<QWidget*>())
        {
            each->setStyle(m_context.flatStyle());
            each->ensurePolished();
        }

        const AzQtComponents::BoxElement* frameElement = AzQtComponents::BoxElement::forWidget(frame);
        const AzQtComponents::BoxElement* barElement = AzQtComponents::BoxElement::forWidget(bar);
        ASSERT_NE(frameElement, nullptr);
        ASSERT_NE(barElement, nullptr);
        EXPECT_STREQ(frameElement->m_name, "ScriptCanvas.Box.UpgradeProgressFrame");
        EXPECT_STREQ(barElement->m_name, "ScriptCanvas.Box.UpgradeProgressBar");
        EXPECT_EQ(AzQtComponents::BoxElement::forWidget(otherFrame), nullptr);
        // The bar of another form keeps the Global track (the application's grey), whatever its name
        const AzQtComponents::BoxElement* otherBarElement = AzQtComponents::BoxElement::forWidget(otherBar);
        ASSERT_NE(otherBarElement, nullptr);
        EXPECT_STREQ(otherBarElement->m_name, "Global.Box.ProgressBar");
    }

    TEST_F(UpgradeToolFlatSheetTests, Sheets_TheLegacyGuiSetsTheFormsSheetAndTheFlattenedGuiNone)
    {
        const QStringList legacy = UpgradeTool::censusOfController(AzQtComponents::GuiPerfFloor::Rung::Qss);
        bool foundFrame = false;
        for (const QString& line : legacy)
        {
            foundFrame = foundFrame
                || (AzQtComponents::SiteTwins::namesCarrier(line, QStringLiteral("QFrame#progressFrame"))
                    && line.contains(QStringLiteral("background-color")));
        }
        EXPECT_TRUE(foundFrame) << legacy.join("\n").toStdString();

        const QStringList flat = UpgradeTool::censusOfController(AzQtComponents::GuiPerfFloor::Rung::Flat);
        EXPECT_TRUE(flat.isEmpty()) << flat.join("\n").toStdString();
    }
} // namespace UnitTest
