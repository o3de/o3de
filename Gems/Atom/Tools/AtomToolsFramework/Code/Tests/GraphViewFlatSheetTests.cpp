/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Graph/GraphView.h>
#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <AzQtComponents/Components/Elements/ItemViewElement.h>
#include <AzQtComponents/Components/GuiPerfFloor.h>
#include <AzQtComponents/Components/Style.h>
#include <GraphCanvas/Widgets/NodePalette/NodePaletteTreeView.h>

#include <QApplication>
#include <QImage>
#include <QPainter>
#include <QPalette>
#include <QStyleFactory>
#include <QStyleOption>
#include <QTreeView>
#include <QWidget>

#include <string>

namespace UnitTest
{
    // ----------------------------------------------------------------------------------
    // The element of GraphView.qss, the sheet the graph tools set on the node palettes of a view's menus and on their main windows:
    // which trees it reaches, and that it gives them what the sheet gave them (the role of the selection, Highlight) and changes
    // nothing else, against Qt's own style sheet style over Fusion with the same sheet and the rules of the application's sheet that
    // reach a tree, at the device ratios of the GUI. The tree is the real GraphCanvas::NodePaletteTreeView, as the selector names its
    // class. The main windows of the two graph tools are other gems': stand-ins that Qt takes for them.
    //
    // No capture can hold the role (nothing is selected or searched on any screen of the automatic captures): this is its proof.
    // ----------------------------------------------------------------------------------

    namespace GraphViewFlatSheetTestsInternal
    {
        //! A widget that Qt takes for an instance of a class it is not. The matcher asks for the view and the two windows by their
        //! class names, as the selectors of the product do, and the windows are not classes this test links.
        class ClassNamedWidget
            : public QWidget
        {
        public:
            ClassNamedWidget(const char* className, QWidget* parent = nullptr)
                : QWidget(parent)
                , m_className(className)
            {
            }

            void* qt_metacast(const char* name) override
            {
                return qstrcmp(name, m_className) == 0 ? this : QWidget::qt_metacast(name);
            }

        private:
            const char* m_className;
        };

        constexpr const char* GraphViewClass = "AtomToolsFramework::GraphView";
        constexpr const char* MaterialCanvasWindowClass = "MaterialCanvas::MaterialCanvasMainWindow";
        constexpr const char* PassCanvasWindowClass = "PassCanvas::PassCanvasMainWindow";
        constexpr const char* NodePaletteElementName = "AtomTools.ItemView.NodePaletteTree";

        //! GraphView.qss:9-12, whole: the sheet the view's palettes and the two windows get (GraphView.cpp, the windows' constructors)
        constexpr const char* GraphViewSheet = "GraphCanvas--NodePaletteTreeView { selection-background-color: rgba(255, 255, 255, 41); }";

        // The rules of the application's sheet that reach a tree, with the lines they are on
        // BaseStyleSheet.qss:15-21, the universal rule
        constexpr const char* UniversalRule =
            "* { background-color: transparent; color: white; font-family: \"Open Sans\"; font-weight: 400; }\n";
        // BaseStyleSheet.qss:23-38, the fill of a view, and its reset for a view in a widget
        constexpr const char* ViewFillRules =
            "QTableView, QListView, QTreeView { background-color: #444444; }\n"
            "QWidget > QTableView, QWidget > QListView, QWidget > QTreeView { background-color: transparent; }\n";
        // TableView.qss:14-29, the roles and the border of a view
        constexpr const char* ViewRule =
            "AzQtComponents--TableView, QTableView, QListView, QTreeView { alternate-background-color: rgb(77, 77, 77); "
            "selection-background-color: rgb(101, 101, 101); selection-color: #ffffff; show-decoration-selected: 1; border: none; }\n";
        // TableView.qss:31-60, the item rules of the Global view
        constexpr const char* ItemRules =
            "AzQtComponents--TableView::item, QTableView::item, QListView::item, QTreeView::item "
            "{ height: 24px; padding-left: 4px; border: none; }\n"
            "AzQtComponents--TableView::branch:selected, AzQtComponents--TableView::item:selected, QTableView::item:selected, "
            "QListView::item:selected, QTreeView::branch:selected, QTreeView::item:selected { background: rgb(101, 101, 101); }\n"
            "AzQtComponents--TableView::branch:hover, AzQtComponents--TableView::item:hover, QTableView::item:hover, "
            "QListView::item:hover, QTreeView::branch:hover, QTreeView::item:hover { background: rgb(90, 90, 90); }\n";
        // QDockWidget.qss:33-36, the margin every scroll area has
        constexpr const char* ScrollAreaRule = "QAbstractScrollArea { margin: 0; }\n";

        constexpr QRgb ParentColor = 0x444444;          // what the rows of the palette sit on in the legacy captures
        constexpr QSize RowSize(120, 24);

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
                            return QString::number(rgb, 16).toStdString();
                        };
                        return "at (" + std::to_string(x) + ", " + std::to_string(y) + "): legacy #" + hex(a.pixel(x, y)) + ", flat #"
                            + hex(b.pixel(x, y));
                    }
                }
            }
            return "";
        }

        //! The roles the sheet touches or leaves alone, in each colour group
        struct Roles
        {
            QRgb m_highlight[3] = {};
            QRgb m_highlightedText[3] = {};
            QRgb m_alternateBase[3] = {};
        };

        Roles rolesOf(const QWidget& widget)
        {
            Roles roles;
            const QPalette::ColorGroup groups[] = { QPalette::Active, QPalette::Inactive, QPalette::Disabled };
            for (int group = 0; group < 3; ++group)
            {
                roles.m_highlight[group] = widget.palette().color(groups[group], QPalette::Highlight).rgba();
                roles.m_highlightedText[group] = widget.palette().color(groups[group], QPalette::HighlightedText).rgba();
                roles.m_alternateBase[group] = widget.palette().color(groups[group], QPalette::AlternateBase).rgba();
            }
            return roles;
        }

        //! The panel of a row as QTreeView::drawRow asks for it (PE_PanelItemViewRow), selected or not, on a parent-coloured image of the
        //! device ratio. The image is the painter's device, so the style scales the box as a screen of that ratio does.
        QImage paintRow(QWidget& tree, bool selected, qreal ratio)
        {
            QStyleOptionViewItem option;
            option.initFrom(&tree);
            option.widget = &tree;
            option.rect = QRect(QPoint(0, 0), RowSize);
            option.state |= QStyle::State_Enabled | QStyle::State_Active;
            if (selected)
            {
                option.state |= QStyle::State_Selected;
            }
            option.viewItemPosition = QStyleOptionViewItem::OnlyOne;

            QImage image((QSizeF(RowSize) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
            image.setDevicePixelRatio(ratio);
            image.fill(QColor::fromRgb(ParentColor));
            QPainter painter(&image);
            tree.style()->drawPrimitive(QStyle::PE_PanelItemViewRow, &option, &painter, &tree);
            painter.end();
            return image;
        }

        class GraphViewFlatSheetTests
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

                // The flattened GUI's branches of Style read the rung
                AzQtComponents::GuiPerfFloor::setRungForTesting(AzQtComponents::GuiPerfFloor::Rung::Flat);

                m_style = AZStd::make_unique<AzQtComponents::Style>(QStyleFactory::create(QStringLiteral("Fusion")));
                m_fusion = AZStd::unique_ptr<QStyle>(QStyleFactory::create(QStringLiteral("Fusion")));
                AtomToolsFramework::GraphView::RegisterFlatElements();
            }

            void TearDown() override
            {
                m_fusion.reset();
                m_style.reset();
                AzQtComponents::GuiPerfFloor::resetForTesting();
                qApp->setStyleSheet(m_applicationSheet);
                m_application.reset();
                LeakDetectionFixture::TearDown();
            }

            //! A style on a widget and on each below it: a style set on a widget reaches no child
            static void useStyle(QWidget& root, QStyle* style)
            {
                root.setStyle(style);
                for (QWidget* child : root.findChildren<QWidget*>())
                {
                    child->setStyle(style);
                }
            }

            //! The flat way: the style, and the polish the flattened GUI runs when a widget is shown, which resolves the element
            void flatten(QWidget& root)
            {
                useStyle(root, m_style.get());
                root.ensurePolished();
            }

            static const char* elementOf(const QWidget& widget)
            {
                const AzQtComponents::ItemViewElement* element = AzQtComponents::ItemViewElement::forWidget(&widget);
                return element ? element->m_name : "";
            }

            //! The tree as the legacy GUI styled it: Qt's style sheet style over Fusion, with the sheet on the widget that holds it (the
            //! main window of a graph tool; the sheet reaches what is below it) and the application's rules above that, set on the
            //! application as the product's are (a widget's own sheet outranks the application's whatever the specificity). The
            //! callback gets the polished tree.
            void withLegacyTree(const AZStd::function<void(QWidget&)>& use) const
            {
                {
                    QWidget window;
                    auto* tree = new GraphCanvas::NodePaletteTreeView(&window);
                    tree->setRootIsDecorated(false);
                    useStyle(window, m_fusion.get());
                    qApp->setStyleSheet(QString::fromLatin1(UniversalRule) + QString::fromLatin1(ViewFillRules)
                        + QString::fromLatin1(ViewRule) + QString::fromLatin1(ItemRules) + QString::fromLatin1(ScrollAreaRule));
                    window.setStyleSheet(QString::fromLatin1(GraphViewSheet));
                    window.ensurePolished();
                    use(*tree);
                }
                qApp->setStyleSheet(QString());
            }

            //! The same tree in the flattened GUI: below a main window of a graph tool, with its element and no sheet
            void withFlatTree(const AZStd::function<void(QWidget&)>& use)
            {
                ClassNamedWidget window(MaterialCanvasWindowClass);
                auto* tree = new GraphCanvas::NodePaletteTreeView(&window);
                tree->setRootIsDecorated(false);
                flatten(window);
                use(*tree);
            }

            int m_argc = 1;                                     // QApplication keeps a reference to it
            char m_programName[5] = "test";
            char* m_argv[2] = { m_programName, nullptr };
            AZStd::unique_ptr<QApplication> m_application;
            QString m_applicationSheet;
            AZStd::unique_ptr<AzQtComponents::Style> m_style;
            AZStd::unique_ptr<QStyle> m_fusion;
        };
    } // namespace GraphViewFlatSheetTestsInternal

    // A unity build joins this file with others of namespace UnitTest: the fixture is the only name brought in, the rest is named
    // through the alias
    using GraphViewFlatSheetTestsInternal::GraphViewFlatSheetTests;
    namespace GraphViewFlat = GraphViewFlatSheetTestsInternal;

    // ----------------------------------------------------------------------------------
    // Resolution
    // ----------------------------------------------------------------------------------

    TEST_F(GraphViewFlatSheetTests, Registry_ANodePaletteTreeIsFoundBelowAGraphViewAndBelowTheMainWindowOfEachGraphTool)
    {
        GraphViewFlat::ClassNamedWidget view(GraphViewFlat::GraphViewClass);
        auto* viewMenu = new QWidget(&view);                                // the menu of the view holds the palette, which holds the tree
        auto* viewTree = new GraphCanvas::NodePaletteTreeView(viewMenu);

        GraphViewFlat::ClassNamedWidget materialWindow(GraphViewFlat::MaterialCanvasWindowClass);
        auto* dock = new QWidget(&materialWindow);                          // deeper in the window, as the node palette's dock is
        auto* materialTree = new GraphCanvas::NodePaletteTreeView(dock);

        GraphViewFlat::ClassNamedWidget passWindow(GraphViewFlat::PassCanvasWindowClass);
        auto* passTree = new GraphCanvas::NodePaletteTreeView(&passWindow);

        for (QWidget* root : { static_cast<QWidget*>(&view), static_cast<QWidget*>(&materialWindow), static_cast<QWidget*>(&passWindow) })
        {
            flatten(*root);
        }

        EXPECT_STREQ(elementOf(*viewTree), GraphViewFlat::NodePaletteElementName);
        EXPECT_STREQ(elementOf(*materialTree), GraphViewFlat::NodePaletteElementName);
        EXPECT_STREQ(elementOf(*passTree), GraphViewFlat::NodePaletteElementName);
    }

    TEST_F(GraphViewFlatSheetTests, Registry_ANodePaletteOutsideThoseAndAnotherTreeInsideThemHaveNoneOfIt)
    {
        // The same class of tree where no graph tool set the sheet: alone, in a plain widget, in the window of another tool
        GraphCanvas::NodePaletteTreeView alone;
        QWidget plain;
        auto* inPlain = new GraphCanvas::NodePaletteTreeView(&plain);
        GraphViewFlat::ClassNamedWidget otherWindow("MaterialEditor::MaterialEditorMainWindow");
        auto* inOtherWindow = new GraphCanvas::NodePaletteTreeView(&otherWindow);

        // A tree of another class where the sheet was set: a plain tree below the window of a graph tool, and below a view
        GraphViewFlat::ClassNamedWidget window(GraphViewFlat::MaterialCanvasWindowClass);
        auto* plainTree = new QTreeView(&window);
        GraphViewFlat::ClassNamedWidget view(GraphViewFlat::GraphViewClass);
        auto* plainTreeInView = new QTreeView(&view);

        for (QWidget* root : { static_cast<QWidget*>(&alone), static_cast<QWidget*>(&plain), static_cast<QWidget*>(&otherWindow),
                               static_cast<QWidget*>(&window), static_cast<QWidget*>(&view) })
        {
            flatten(*root);
        }

        for (const QWidget* tree : { static_cast<const QWidget*>(&alone), static_cast<const QWidget*>(inPlain),
                                     static_cast<const QWidget*>(inOtherWindow), static_cast<const QWidget*>(plainTree),
                                     static_cast<const QWidget*>(plainTreeInView) })
        {
            EXPECT_STRNE(elementOf(*tree), GraphViewFlat::NodePaletteElementName);
            EXPECT_STREQ(elementOf(*tree), "Global.ItemView.Tree");         // the Global element of a tree, as before
        }
    }

    // ----------------------------------------------------------------------------------
    // The role, and the pixels it must not change
    // ----------------------------------------------------------------------------------

    TEST_F(GraphViewFlatSheetTests, Role_TheSelectionRoleIsTheSheetsInEveryColourGroupAndTheOtherRolesAreTheGlobalOnes)
    {
        GraphViewFlat::Roles legacy;
        GraphViewFlat::Roles flat;
        withLegacyTree([&legacy](QWidget& tree) { legacy = GraphViewFlat::rolesOf(tree); });
        withFlatTree([&flat](QWidget& tree) { flat = GraphViewFlat::rolesOf(tree); });

        for (int group = 0; group < 3; ++group)
        {
            EXPECT_EQ(flat.m_highlight[group], legacy.m_highlight[group]) << "group " << group;
            EXPECT_EQ(flat.m_highlightedText[group], legacy.m_highlightedText[group]) << "group " << group;
            EXPECT_EQ(flat.m_alternateBase[group], legacy.m_alternateBase[group]) << "group " << group;

            // rgba(255, 255, 255, 41) (GraphView.qss:11); selection-color and alternate-background-color are TableView.qss:21, :24's
            EXPECT_EQ(flat.m_highlight[group], QColor(255, 255, 255, 41).rgba()) << "group " << group;
            EXPECT_EQ(flat.m_highlightedText[group], QColor(255, 255, 255).rgba()) << "group " << group;
            EXPECT_EQ(flat.m_alternateBase[group], QColor(77, 77, 77).rgba()) << "group " << group;
        }
    }

    TEST_F(GraphViewFlatSheetTests, Pixels_ARowIsTheGlobalFillWhateverTheRoleAndPaintsWhatTheSheetPaintedAtEveryRatio)
    {
        for (const qreal ratio : { 1.0, 1.5, 2.0 })
        {
            for (const bool selected : { false, true })
            {
                QImage legacy;
                QImage flat;
                withLegacyTree([&](QWidget& tree) { legacy = GraphViewFlat::paintRow(tree, selected, ratio); });
                withFlatTree([&](QWidget& tree) { flat = GraphViewFlat::paintRow(tree, selected, ratio); });
                EXPECT_EQ(GraphViewFlat::firstDifference(legacy, flat), std::string())
                    << (selected ? "a selected" : "an unselected") << " row at ratio " << ratio;
            }
        }
    }

    TEST_F(GraphViewFlatSheetTests, Pixels_ASelectedRowIsOpaqueGreyNotTheTranslucentRoleAndAnUnselectedOneIsFillless)
    {
        withFlatTree([](QWidget& tree)
            {
                const QImage selected = GraphViewFlat::paintRow(tree, true, 1.5);
                const QImage unselected = GraphViewFlat::paintRow(tree, false, 1.5);

                // rgb(101, 101, 101) of TableView.qss:49 over the base style's fill of the translucent role: opaque
                EXPECT_EQ(selected.pixel(0, 0), QColor(101, 101, 101).rgba());
                EXPECT_EQ(selected.pixel(selected.width() - 1, selected.height() - 1), QColor(101, 101, 101).rgba());
                EXPECT_EQ(unselected.pixel(0, 0) & 0xFFFFFF, GraphViewFlat::ParentColor);
            });
    }
} // namespace UnitTest
