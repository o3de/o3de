/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <AzQtComponents/Components/GuiPerfFloor.h>
#include <AzQtComponents/Components/StyleManager.h>

#include <QApplication>
#include <QFont>
#include <QFontInfo>
#include <QImage>
#include <QPainter>
#include <QPalette>
#include <QStandardItemModel>
#include <QStyle>
#include <QStyleFactory>
#include <QTreeView>
#include <QVBoxLayout>
#include <QWidget>

#include <string>

namespace UnitTest
{
    // ----------------------------------------------------------------------------------
    // The left side of the Atom tools' asset browser (AtomToolsAssetBrowser.ui gave its widget m_leftLayout the sheet `vertical-align:
    // top`): Qt's style sheet style reads no `vertical-align` of a widget's rule (QCss parses the property, no extractor uses it), so
    // the sheet changes nothing that the widget or the tree below it draws, lays out or inherits, and the flattened GUI carries it with
    // no element at all: it sets no sheet, which leaves the whole subtree of the widget off the style sheet style.
    //
    // The twins: the left side under Qt's style sheet style over Fusion with the application's rules that reach it, with the sheet and
    // without it, which must not differ; and the same left side in the flattened GUI, which has no sheet style. AtomToolsAssetBrowser
    // itself needs the asset browser model, so the tests build the widgets the form built: m_leftLayout > a layout > the tree.
    // ----------------------------------------------------------------------------------

    namespace AtomToolsAssetBrowserFlatSheetTestsInternal
    {
        //! The sheet the form gave m_leftLayout (AtomToolsAssetBrowser.ui:68-70 before the flattening), which the legacy GUI sets.
        //! Copied, not read from the product: the constructor needs the asset browser model, so no test runs it. If the text of the
        //! setLegacyStyleSheet call in the constructor changes, change this one with it.
        constexpr const char* LeftSideSheet = "vertical-align: top";

        //! The rules of the application's sheet that reach a plain widget and a tree view below it, which the legacy cascade laid
        //! under the widget's own sheet (the rest of the application sheet reaches the tree the same with the sheet and without it)
        constexpr const char* ApplicationRules =
            // BaseStyleSheet.qss:15-21, the universal rule
            "* { background-color: transparent; color: white; font-family: \"Open Sans\"; font-weight: 400; }"
            // BaseStyleSheet.qss:23-30, and :32-38 for a view whose parent is a widget
            "QTableView, QListView, QTreeView { background-color: #444444; }"
            "QWidget > QTableView, QWidget > QListView, QWidget > QTreeView { background-color: transparent; }";

        constexpr QRgb ParentColor = 0x808080;          // what nothing draws stays this grey
        constexpr int TreeRows = 3;

        //! The widgets the form built: m_leftLayout, its layout without margins or spacing, and the asset browser's tree (a QTreeView)
        //! with its entries
        struct LeftSide
        {
            LeftSide()
            {
                m_leftLayout = new QWidget(&m_root);
                m_leftLayout->setObjectName(QStringLiteral("m_leftLayout"));
                auto* layout = new QVBoxLayout(m_leftLayout);
                layout->setSpacing(0);
                layout->setContentsMargins(0, 0, 0, 0);
                m_tree = new QTreeView(m_leftLayout);
                m_tree->setObjectName(QStringLiteral("m_assetBrowserTreeViewWidget"));
                layout->addWidget(m_tree);

                for (int row = 0; row < TreeRows; ++row)
                {
                    m_model.appendRow(new QStandardItem(QStringLiteral("Entry %1").arg(row)));
                }
                m_tree->setModel(&m_model);
                m_tree->setHeaderHidden(true);
                m_root.resize(200, 90);
                m_leftLayout->setGeometry(0, 0, 200, 90);
            }

            //! The widgets the sheet reaches: the widget, the tree and its viewport
            QWidgetList reached() const
            {
                return { m_leftLayout, m_tree, m_tree->viewport() };
            }

            QStandardItemModel m_model;         // declared before the root: the tree below it is gone before its model
            QWidget m_root;
            QWidget* m_leftLayout = nullptr;
            QTreeView* m_tree = nullptr;
        };

        //! Whether the widget's style is Qt's style sheet style
        bool hasSheetStyle(const QWidget& widget)
        {
            return widget.style()->inherits("QStyleSheetStyle");
        }

        //! What a widget and everything below it paint, in the order Qt paints them, on an image of the given device ratio
        QImage paintTree(QWidget& widget, qreal ratio)
        {
            QImage image((QSizeF(widget.size()) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
            image.setDevicePixelRatio(ratio);
            image.fill(QColor::fromRgb(ParentColor));
            widget.render(&image);
            return image;
        }

        //! The first pixel two images of one size differ at, as text for a failure message ("" when they are equal)
        std::string firstDifference(const QImage& a, const QImage& b)
        {
            if (a.size() != b.size())
            {
                return "the sizes differ";
            }
            for (int y = 0; y < a.height(); ++y)
            {
                for (int x = 0; x < a.width(); ++x)
                {
                    if (a.pixel(x, y) != b.pixel(x, y))
                    {
                        const auto hex = [](QRgb rgb)
                        {
                            return QString::number(rgb & 0xFFFFFF, 16).toStdString();
                        };
                        return "at (" + std::to_string(x) + ", " + std::to_string(y) + "): with the sheet #" + hex(a.pixel(x, y))
                            + ", without #" + hex(b.pixel(x, y));
                    }
                }
            }
            return "";
        }

        //! Every role of every group, as text for a failure message ("" when the palettes are equal)
        std::string firstPaletteDifference(const QPalette& a, const QPalette& b)
        {
            for (const QPalette::ColorGroup group : { QPalette::Active, QPalette::Inactive, QPalette::Disabled })
            {
                for (int role = 0; role < QPalette::NColorRoles; ++role)
                {
                    const auto colorRole = static_cast<QPalette::ColorRole>(role);
                    if (a.brush(group, colorRole) != b.brush(group, colorRole))
                    {
                        return "group " + std::to_string(static_cast<int>(group)) + ", role " + std::to_string(role);
                    }
                }
            }
            return "";
        }

        class AtomToolsAssetBrowserFlatSheetTests
            : public LeakDetectionFixture
        {
        protected:
            void SetUp() override
            {
                LeakDetectionFixture::SetUp();
                if (!QApplication::instance())
                {
                    m_application = AZStd::make_unique<QApplication>(m_argc, m_argv);
                }

                // The test application may carry StyleManager's legacy sheets: the twins name their own
                m_applicationSheet = qApp->styleSheet();
                qApp->setStyleSheet(QString());

                m_fusion = AZStd::unique_ptr<QStyle>(QStyleFactory::create(QStringLiteral("Fusion")));
            }

            void TearDown() override
            {
                m_fusion.reset();
                AzQtComponents::GuiPerfFloor::resetForTesting();
                qApp->setStyleSheet(m_applicationSheet);
                m_application.reset();
                LeakDetectionFixture::TearDown();
            }

            //! The left side as the legacy GUI styled it: Qt's style sheet style over Fusion, with the application's rules above it as
            //! the sheet of its ancestor and, if asked, the sheet the form gave it
            static void styleLegacy(LeftSide& side, QStyle* fusion, bool ownSheet)
            {
                side.m_root.setStyle(fusion);
                side.m_leftLayout->setStyle(fusion);
                if (ownSheet)
                {
                    side.m_leftLayout->setStyleSheet(QString::fromLatin1(LeftSideSheet));
                }
                side.m_root.setStyleSheet(QString::fromLatin1(ApplicationRules));
                side.m_root.ensurePolished();
                side.m_leftLayout->ensurePolished();
                // A hidden widget is laid out when it is rendered: do it now, so that the geometry the tests read is the laid-out one
                (void)paintTree(*side.m_leftLayout, 1.0);
            }

            int m_argc = 1;                                     // QApplication keeps a reference to it
            char m_programName[5] = "test";
            char* m_argv[2] = { m_programName, nullptr };
            AZStd::unique_ptr<QApplication> m_application;
            QString m_applicationSheet;
            AZStd::unique_ptr<QStyle> m_fusion;
        };
    } // namespace AtomToolsAssetBrowserFlatSheetTestsInternal

    // A unity build joins this file with others of namespace UnitTest: the fixture is the only name brought in, the rest is named
    // through the alias
    using AtomToolsAssetBrowserFlatSheetTestsInternal::AtomToolsAssetBrowserFlatSheetTests;
    namespace LeftFlat = AtomToolsAssetBrowserFlatSheetTestsInternal;

    // ----------------------------------------------------------------------------------
    // The legacy GUI: the sheet changes nothing
    // ----------------------------------------------------------------------------------

    TEST_F(AtomToolsAssetBrowserFlatSheetTests, Pixels_TheSheetChangesNothingTheLeftSideDrawsAtEveryRatio)
    {
        for (const qreal ratio : { 1.0, 1.5, 2.0 })
        {
            LeftFlat::LeftSide withSheet;
            LeftFlat::LeftSide withoutSheet;
            styleLegacy(withSheet, m_fusion.get(), true);
            styleLegacy(withoutSheet, m_fusion.get(), false);
            ASSERT_TRUE(LeftFlat::hasSheetStyle(*withSheet.m_tree));

            EXPECT_EQ(LeftFlat::firstDifference(LeftFlat::paintTree(*withSheet.m_leftLayout, ratio),
                LeftFlat::paintTree(*withoutSheet.m_leftLayout, ratio)), std::string()) << "at ratio " << ratio;
        }
    }

    TEST_F(AtomToolsAssetBrowserFlatSheetTests, Box_TheSheetChangesNoGeometryFontOrPaletteOfTheWidgetsItReaches)
    {
        LeftFlat::LeftSide withSheet;
        LeftFlat::LeftSide withoutSheet;
        styleLegacy(withSheet, m_fusion.get(), true);
        styleLegacy(withoutSheet, m_fusion.get(), false);

        // The size hints, the box of the tree and where its viewport and rows sit
        EXPECT_EQ(withSheet.m_leftLayout->sizeHint(), withoutSheet.m_leftLayout->sizeHint());
        EXPECT_EQ(withSheet.m_leftLayout->contentsRect(), withoutSheet.m_leftLayout->contentsRect());
        EXPECT_EQ(withSheet.m_tree->sizeHint(), withoutSheet.m_tree->sizeHint());
        EXPECT_EQ(withSheet.m_tree->frameShape(), withoutSheet.m_tree->frameShape());
        EXPECT_EQ(withSheet.m_tree->frameWidth(), withoutSheet.m_tree->frameWidth());
        EXPECT_EQ(withSheet.m_tree->viewport()->geometry(), withoutSheet.m_tree->viewport()->geometry());
        EXPECT_EQ(
            withSheet.m_tree->visualRect(withSheet.m_model.index(1, 0)),
            withoutSheet.m_tree->visualRect(withoutSheet.m_model.index(1, 0)));

        // Each widget the sheet reaches: the font, the palette every child inherits, and whether the style paints its background
        const QWidgetList reachedWith = withSheet.reached();
        const QWidgetList reachedWithout = withoutSheet.reached();
        ASSERT_EQ(reachedWith.size(), reachedWithout.size());
        for (int i = 0; i < reachedWith.size(); ++i)
        {
            const QWidget* with = reachedWith[i];
            const QWidget* without = reachedWithout[i];
            const std::string name = with->objectName().toStdString() + "#" + std::to_string(i);

            EXPECT_TRUE(with->font() == without->font()) << name;
            EXPECT_EQ(QFontInfo(with->font()).pixelSize(), QFontInfo(without->font()).pixelSize()) << name;
            EXPECT_EQ(LeftFlat::firstPaletteDifference(with->palette(), without->palette()), std::string()) << name;
            EXPECT_EQ(with->testAttribute(Qt::WA_StyledBackground), without->testAttribute(Qt::WA_StyledBackground)) << name;
            EXPECT_EQ(with->testAttribute(Qt::WA_SetPalette), without->testAttribute(Qt::WA_SetPalette)) << name;
        }
    }

    // ----------------------------------------------------------------------------------
    // The conversion: the legacy GUI sets the sheet, the flattened GUI none, so the subtree is off the style sheet style
    // ----------------------------------------------------------------------------------

    TEST_F(AtomToolsAssetBrowserFlatSheetTests, LegacyGui_TheSheetPutsTheWidgetAndTheTreeBelowItUnderTheSheetStyle)
    {
        AzQtComponents::GuiPerfFloor::setRungForTesting(AzQtComponents::GuiPerfFloor::Rung::Qss);
        LeftFlat::LeftSide side;

        AzQtComponents::StyleManager::setLegacyStyleSheet(side.m_leftLayout, QString::fromLatin1(LeftFlat::LeftSideSheet));
        side.m_leftLayout->ensurePolished();

        EXPECT_EQ(side.m_leftLayout->styleSheet(), QString::fromLatin1(LeftFlat::LeftSideSheet));
        for (const QWidget* widget : side.reached())
        {
            EXPECT_TRUE(LeftFlat::hasSheetStyle(*widget)) << widget->objectName().toStdString();
        }
    }

    TEST_F(AtomToolsAssetBrowserFlatSheetTests, FlattenedGui_NoSheetStyleReachesTheWidgetTheTreeOrItsViewport)
    {
        AzQtComponents::GuiPerfFloor::setRungForTesting(AzQtComponents::GuiPerfFloor::Rung::Flat);
        LeftFlat::LeftSide side;

        AzQtComponents::StyleManager::setLegacyStyleSheet(side.m_leftLayout, QString::fromLatin1(LeftFlat::LeftSideSheet));
        side.m_leftLayout->ensurePolished();

        EXPECT_TRUE(side.m_leftLayout->styleSheet().isEmpty());
        for (const QWidget* widget : side.reached())
        {
            EXPECT_FALSE(LeftFlat::hasSheetStyle(*widget)) << widget->objectName().toStdString();
            EXPECT_FALSE(widget->testAttribute(Qt::WA_StyleSheet)) << widget->objectName().toStdString();
            EXPECT_EQ(widget->style(), QApplication::style()) << widget->objectName().toStdString();
        }
    }
} // namespace UnitTest
