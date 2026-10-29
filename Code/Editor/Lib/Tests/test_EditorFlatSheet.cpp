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

#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/ItemViewElement.h>
#include <AzQtComponents/Components/Style.h>
#include <EditorFlatSheet.h>

#include <QFrame>
#include <QImage>
#include <QListWidget>
#include <QMargins>
#include <QPainter>
#include <QStyleFactory>
#include <QStyleOption>
#include <QTreeView>
#include <QWidget>

namespace UnitTest
{
    // ----------------------------------------------------------------------------------
    // The elements of Editor.qss: which widgets of the Editor they reach, and that they paint what the sheet painted.
    // The expected pixels are those of the legacy capture (UiCaptures n2, Editor, ratio 1.5), as the sheet's own style
    // paints them in the twin, with the declarations of Editor.qss as the sheet had them.
    // ----------------------------------------------------------------------------------

    namespace EditorFlatSheetTestsInternal
    {
        //! A widget that Qt takes for an instance of a class it is not. The matchers ask for a window by its class name, as
        //! the selectors do, and the classes of the Editor's windows are not ones this test links.
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

        constexpr QRgb ParentColor = 0x444444;          // what the twin's widgets sit on (the Editor's window colour)

        //! A Global element that would take the status bar, were it not for the module's: registered first, as AzQtComponents
        //! registers its own before a module has set up its styles
        const AzQtComponents::BoxElement GlobalProbe{ .m_name = "Test.Box.GlobalProbe" };

        class EditorFlatSheetTests
            : public LeakDetectionFixture
        {
        protected:
            void SetUp() override
            {
                LeakDetectionFixture::SetUp();
                m_style = AZStd::make_unique<AzQtComponents::Style>(QStyleFactory::create(QStringLiteral("Fusion")));
                m_fusion = AZStd::unique_ptr<QStyle>(QStyleFactory::create(QStringLiteral("Fusion")));

                // Once for the process, as the flattened GUI runs the registrar of a sheet once: a Global element first, then the sheet's
                static const bool registered = (AzQtComponents::BoxElement::registerElement(&GlobalProbe, &QWidget::staticMetaObject,
                    [](const QWidget* widget) { return widget->objectName() == QLatin1String("AssetEditorStatusBar"); },
                    AzQtComponents::ElementScope::Global), EditorFlatSheet::RegisterElements(), true);
                ASSERT_TRUE(registered);
            }

            void TearDown() override
            {
                m_fusion.reset();
                m_style.reset();
                LeakDetectionFixture::TearDown();
            }

            //! The flat way: the style on the widget, and the element resolved as Style::polish does in the flattened GUI
            void flatten(QWidget& widget)
            {
                widget.setStyle(m_style.get());
                AzQtComponents::BoxElement::resolve(&widget);
            }

            const char* elementOf(const QWidget& widget) const
            {
                const AzQtComponents::BoxElement* element = AzQtComponents::BoxElement::forWidget(&widget);
                return element ? element->m_name : "";
            }

            //! The element of this sheet a widget resolved to, or "": a Global element of AzQtComponents may cover a widget of the same
            //! class (the flattened GUI registers them from Style::polish(QApplication*)), and is not what the tests below ask about
            const char* editorElementOf(const QWidget& widget) const
            {
                const char* name = elementOf(widget);
                return qstrncmp(name, "Editor.Box.", 11) == 0 ? name : "";
            }

            //! The widgets under a sheet, as the legacy GUI had them: Qt's own sheet style over Fusion (set on each, so that
            //! the style of the screen the test runs on is not their base)
            void legacy(QWidget& root, const char* rules)
            {
                root.setStyle(m_fusion.get());
                for (QWidget* child : root.findChildren<QWidget*>())
                {
                    child->setStyle(m_fusion.get());
                }
                root.setStyleSheet(QString::fromUtf8(rules));
                root.ensurePolished();
                for (QWidget* child : root.findChildren<QWidget*>())
                {
                    child->ensurePolished();
                }
            }

            //! What a widget paints as its background and border (PE_Widget) at a logical offset on a parent-coloured image of the
            //! given device ratio. The image is the painter's device, so the style scales the box as the screen did
            //! (QWidget::render would hand the style the ratio of the screen the test runs on).
            static QImage paint(const QWidget& widget, const AzQtComponents::BoxElement* element, const QPoint& offset, qreal ratio, const QSize& deviceSize)
            {
                QImage image(deviceSize, QImage::Format_ARGB32_Premultiplied);
                image.setDevicePixelRatio(ratio);
                image.fill(QColor::fromRgb(ParentColor));

                QPainter painter(&image);
                painter.translate(offset);
                painter.setClipRect(widget.rect());
                QStyleOption option;
                option.initFrom(&widget);
                if (element)
                {
                    element->draw(&painter, &option, &widget);
                }
                else
                {
                    widget.style()->drawPrimitive(QStyle::PE_Widget, &option, &painter, &widget);
                }
                painter.end();
                return image;
            }

            static QRgb pixelAt(const QImage& image, int x, int y)
            {
                return image.pixel(x, y) & 0xFFFFFF;
            }

            AZStd::unique_ptr<AzQtComponents::Style> m_style;
            AZStd::unique_ptr<QStyle> m_fusion;
        };
    } // namespace EditorFlatSheetTestsInternal

    using EditorFlatSheetTestsInternal::ClassNamedWidget;
    using EditorFlatSheetTestsInternal::EditorFlatSheetTests;
    using EditorFlatSheetTestsInternal::ParentColor;

    // ----------------------------------------------------------------------------------
    // Resolution
    // ----------------------------------------------------------------------------------

    TEST_F(EditorFlatSheetTests, Registry_TheStatusBarOfTheAssetEditorIsFoundByItsNameWhereverItSits)
    {
        QWidget root;
        QWidget* statusBar = new QWidget(&root);
        statusBar->setObjectName("AssetEditorStatusBar");
        QWidget* other = new QWidget(&root);
        other->setObjectName("AssetEditorStatus");
        QFrame* frame = new QFrame(&root);                  // `#AssetEditorStatusBar` names no type
        frame->setObjectName("AssetEditorStatusBar");

        flatten(*statusBar);
        flatten(*other);
        flatten(*frame);

        // The module's element is before the Global one that matches the same widget, whichever was registered first
        EXPECT_STREQ(elementOf(*statusBar), "Editor.Box.AssetEditorStatusBar");
        EXPECT_STREQ(elementOf(*frame), "Editor.Box.AssetEditorStatusBar");
        EXPECT_STREQ(editorElementOf(*other), "");
    }

    TEST_F(EditorFlatSheetTests, Registry_TheAssetBrowserRowAndSeparatorAreFoundOnlyBelowTheAssetBrowserWindow)
    {
        ClassNamedWidget browser("AzAssetBrowserWindow");
        QFrame* row = new QFrame(&browser);
        row->setObjectName("searchButtonContainer");
        QWidget* separator = new QWidget(&browser);
        separator->setObjectName("m_separator");

        // The same names in a window that is not the asset browser's, and two other names in it
        QWidget elsewhere;
        QFrame* strayRow = new QFrame(&elsewhere);
        strayRow->setObjectName("searchButtonContainer");
        QWidget* straySeparator = new QWidget(&elsewhere);
        straySeparator->setObjectName("m_separator");
        QWidget* otherName = new QWidget(&browser);
        otherName->setObjectName("m_separator2");

        for (QWidget* widget : { static_cast<QWidget*>(row), separator, static_cast<QWidget*>(strayRow), straySeparator, otherName })
        {
            flatten(*widget);
        }

        EXPECT_STREQ(elementOf(*row), "Editor.Box.AssetBrowserSearchButtons");
        EXPECT_STREQ(elementOf(*separator), "Editor.Box.AssetBrowserSeparator");
        EXPECT_STREQ(editorElementOf(*strayRow), "");
        EXPECT_STREQ(editorElementOf(*straySeparator), "");
        EXPECT_STREQ(editorElementOf(*otherName), "");
    }

    TEST_F(EditorFlatSheetTests, Registry_AWidgetWithNoElementKeepsNothing_AndTheElementsStyleTheBackground)
    {
        ClassNamedWidget browser("AzAssetBrowserWindow");
        QWidget* separator = new QWidget(&browser);
        separator->setObjectName("m_separator");
        QWidget* plain = new QWidget(&browser);

        flatten(*separator);
        flatten(*plain);

        EXPECT_TRUE(separator->testAttribute(Qt::WA_StyledBackground));       // the fill is drawn at PE_Widget
        EXPECT_FALSE(plain->testAttribute(Qt::WA_StyledBackground));
        EXPECT_FALSE(plain->dynamicPropertyNames().contains(QByteArrayLiteral("_o3de_boxElement")));
    }

    // ----------------------------------------------------------------------------------
    // Pixels: the element paints the box the sheet painted
    // ----------------------------------------------------------------------------------

    TEST_F(EditorFlatSheetTests, StatusBar_PaintsTheLineOnItsTopEdgeLikeTheSheet)
    {
        // Editor.qss:180-183 over BaseStyleSheet.qss:17. Legacy capture, Editor 06_asset_editor: the bar sits at logical y 877, device
        // row 1315.5, and its columns read, from device row 1315: #444444, #111111, #111111, #2a2a2a, #444444 (the flat capture:
        // #444444 on all of them). The same half-pixel phase here is logical y 1: the rows shift by 1314.
        QWidget root;
        QWidget* statusBar = new QWidget(&root);
        statusBar->setObjectName("AssetEditorStatusBar");
        statusBar->resize(40, 29);

        legacy(root, "* { background-color: transparent; } #AssetEditorStatusBar { border-top: 2px solid #111111; }");
        const QImage sheet = paint(*statusBar, nullptr, QPoint(0, 1), 1.5, QSize(60, 54));

        QWidget flatRoot;
        QWidget* flatBar = new QWidget(&flatRoot);
        flatBar->setObjectName("AssetEditorStatusBar");
        flatBar->resize(40, 29);
        flatten(*flatBar);
        const QImage flat = paint(*flatBar, AzQtComponents::BoxElement::forWidget(flatBar), QPoint(0, 1), 1.5, QSize(60, 54));

        EXPECT_EQ(pixelAt(flat, 10, 1), ParentColor);
        EXPECT_EQ(pixelAt(flat, 10, 2), 0x111111u);
        EXPECT_EQ(pixelAt(flat, 10, 3), 0x111111u);
        EXPECT_EQ(pixelAt(flat, 10, 4), 0x2A2A2Au);
        EXPECT_EQ(pixelAt(flat, 10, 5), ParentColor);
        EXPECT_EQ(pixelAt(flat, 10, 40), ParentColor);
        EXPECT_TRUE(flat == sheet);

        // at a whole device pixel and at ratio 1
        const AzQtComponents::BoxElement* element = AzQtComponents::BoxElement::forWidget(flatBar);
        EXPECT_TRUE(paint(*flatBar, element, QPoint(0, 2), 1.5, QSize(60, 54)) == paint(*statusBar, nullptr, QPoint(0, 2), 1.5, QSize(60, 54)));
        EXPECT_TRUE(paint(*flatBar, element, QPoint(0, 3), 1.0, QSize(40, 40)) == paint(*statusBar, nullptr, QPoint(0, 3), 1.0, QSize(40, 40)));
    }

    TEST_F(EditorFlatSheetTests, AssetBrowserRow_FillsWithTheDarkFillLikeTheSheet)
    {
        // Editor.qss:20-24. Legacy capture, Editor 07_main_window_final (Asset Browser tab): the row sits at logical y 1062, device row
        // 1593 (whole), and its column reads #444444, #444444 above it, then #141414 from its first row (flat: #444444 throughout).
        QWidget root;
        QFrame* row = new QFrame(&root);
        row->setObjectName("searchButtonContainer");
        row->resize(40, 35);

        legacy(root, "* { background-color: transparent; } #searchButtonContainer { background-color: #141414; border: 0 none; }");
        const QImage sheet = paint(*row, nullptr, QPoint(0, 2), 1.5, QSize(60, 80));

        QWidget flatRoot;
        ClassNamedWidget browser("AzAssetBrowserWindow", &flatRoot);
        QFrame* flatRow = new QFrame(&browser);
        flatRow->setObjectName("searchButtonContainer");
        flatRow->resize(40, 35);
        flatten(*flatRow);
        const QImage flat = paint(*flatRow, AzQtComponents::BoxElement::forWidget(flatRow), QPoint(0, 2), 1.5, QSize(60, 80));

        EXPECT_EQ(pixelAt(flat, 20, 2), ParentColor);
        EXPECT_EQ(pixelAt(flat, 20, 3), 0x141414u);
        EXPECT_EQ(pixelAt(flat, 20, 20), 0x141414u);
        EXPECT_EQ(pixelAt(flat, 20, 55), 0x141414u);
        EXPECT_TRUE(flat == sheet);
    }

    TEST_F(EditorFlatSheetTests, AssetBrowserSeparator_IsOneDeviceRowOfTheDarkFillLikeTheSheet)
    {
        // Editor.qss:26-29. Legacy capture, Editor 07_main_window_final (Asset Browser tab): the 1px line sits at logical y 1097, device
        // row 1645.5; its column reads #141414 (the row above), #141414, #222222 on device row 1646, then #333333 below (flat: #444444
        // down to the row below). The same phase here is logical y 1, and the line is device row 2.
        QWidget root;
        QWidget* separator = new QWidget(&root);
        separator->setObjectName("m_separator");
        separator->resize(40, 1);

        legacy(root, "* { background-color: transparent; } #m_separator { background-color: #222222; }");
        const QImage sheet = paint(*separator, nullptr, QPoint(0, 1), 1.5, QSize(60, 12));

        QWidget flatRoot;
        ClassNamedWidget browser("AzAssetBrowserWindow", &flatRoot);
        QWidget* flatSeparator = new QWidget(&browser);
        flatSeparator->setObjectName("m_separator");
        flatSeparator->resize(40, 1);
        flatten(*flatSeparator);
        const QImage flat = paint(*flatSeparator, AzQtComponents::BoxElement::forWidget(flatSeparator), QPoint(0, 1), 1.5, QSize(60, 12));

        EXPECT_EQ(pixelAt(flat, 10, 1), ParentColor);
        EXPECT_EQ(pixelAt(flat, 10, 2), 0x222222u);
        EXPECT_EQ(pixelAt(flat, 10, 3), ParentColor);
        EXPECT_TRUE(flat == sheet);
    }

    // ----------------------------------------------------------------------------------
    // The tree of the Add Component popup (Editor.qss:38-41) and the node palettes (Editor.qss:109-113)
    // ----------------------------------------------------------------------------------

    TEST_F(EditorFlatSheetTests, Registry_TheComponentPaletteTreeIsTheTreeWhoseParentIsThePaletteWidget)
    {
        // n2 03_add_component_palette: AzToolsFramework::ComponentPaletteWidget > QTreeView#Tree (contents [0, 0, 0, 0])
        ClassNamedWidget palette("AzToolsFramework::ComponentPaletteWidget");
        QTreeView* tree = new QTreeView(&palette);
        tree->setObjectName("Tree");
        QWidget* wrapper = new QWidget(&palette);                   // a child combinator: a tree deeper in the palette is not the rule's
        QTreeView* deepTree = new QTreeView(wrapper);
        QWidget elsewhere;
        QTreeView* strayTree = new QTreeView(&elsewhere);
        strayTree->setObjectName("Tree");
        QFrame* notATree = new QFrame(&palette);

        for (QWidget* widget : { static_cast<QWidget*>(tree), static_cast<QWidget*>(deepTree), static_cast<QWidget*>(strayTree),
                                 static_cast<QWidget*>(notATree) })
        {
            flatten(*widget);
        }

        EXPECT_STREQ(elementOf(*tree), "Editor.Box.ComponentPaletteTree");
        EXPECT_STREQ(editorElementOf(*deepTree), "");
        EXPECT_STREQ(editorElementOf(*strayTree), "");
        EXPECT_STREQ(editorElementOf(*notATree), "");
        const AzQtComponents::BoxElement* element = AzQtComponents::BoxElement::forWidget(tree);
        ASSERT_TRUE(element != nullptr);
        EXPECT_TRUE(element->m_idle.m_fill.color() == QColor(0x22, 0x22, 0x22));
        EXPECT_TRUE(tree->testAttribute(Qt::WA_StyledBackground));
    }

    TEST_F(EditorFlatSheetTests, ComponentPaletteTree_FillsTheWholeAreaLikeTheSheetSoTheLaneOfItsScrollBarHasTheFill)
    {
        // Editor.qss:38-41 over BaseStyleSheet.qss:17. n2 03_add_component_palette: the tree is at [6, 39, 336, 1037] with its scroll bar lane
        // (8px) at its right edge, and the lane reads #222222 where f5 reads the popup's #383a3b: the rule paints the area of the tree, not
        // its viewport alone. The twin paints the area of a tree (PE_Widget, which is where the sheet painted a scroll area's rule)
        QWidget root;
        QTreeView* tree = new QTreeView(&root);
        tree->setObjectName("Tree");
        tree->setGeometry(0, 0, 80, 60);

        legacy(root, "* { background-color: transparent; } #Tree { background-color: #222222; border: none; margin: 0; }");
        const QImage sheet = paint(*tree, nullptr, QPoint(0, 0), 1.5, QSize(120, 90));

        QWidget flatRoot;
        ClassNamedWidget palette("AzToolsFramework::ComponentPaletteWidget", &flatRoot);
        QTreeView* flatTree = new QTreeView(&palette);
        flatTree->setObjectName("Tree");
        flatTree->setGeometry(0, 0, 80, 60);
        flatten(*flatTree);
        const QImage flat = paint(*flatTree, AzQtComponents::BoxElement::forWidget(flatTree), QPoint(0, 0), 1.5, QSize(120, 90));

        // The corners and the middle, and the lane at the right edge (8 logical px: device columns 108 to 119)
        EXPECT_EQ(pixelAt(flat, 0, 0), 0x222222u);
        EXPECT_EQ(pixelAt(flat, 119, 0), 0x222222u);
        EXPECT_EQ(pixelAt(flat, 60, 45), 0x222222u);
        EXPECT_EQ(pixelAt(flat, 110, 80), 0x222222u);
        EXPECT_TRUE(flat == sheet);
    }

    TEST_F(EditorFlatSheetTests, NodePalette_HasTheSearchHighlightOfTheSheetAsItsSelectionRole)
    {
        using AzQtComponents::ItemViewElement;

        // The Script Canvas node palette and the variable palette are GraphCanvas::NodePaletteTreeView (n2 04_script_canvas, a QTreeView
        // named treeView): any tree of that class, whose rule sets the role of the selection and nothing else
        class NodePaletteTreeView
            : public QTreeView
        {
        public:
            using QTreeView::QTreeView;

            void* qt_metacast(const char* name) override
            {
                return qstrcmp(name, "GraphCanvas::NodePaletteTreeView") == 0 ? this : QTreeView::qt_metacast(name);
            }
        };

        NodePaletteTreeView palette;
        palette.setObjectName("treeView");
        QTreeView other;
        other.setObjectName("treeView");

        ItemViewElement::resolve(&palette);
        ItemViewElement::resolve(&other);

        const ItemViewElement* element = ItemViewElement::forWidget(&palette);
        ASSERT_TRUE(element != nullptr);
        EXPECT_STREQ(element->m_name, "Editor.ItemView.NodePaletteTree");
        EXPECT_TRUE(element->m_selection == QColor(255, 255, 255, 41));
        EXPECT_TRUE(palette.palette().color(QPalette::Highlight) == QColor(255, 255, 255, 41));

        // The fill of a selected item and its hover are the Global element's (rgb(101,101,101), rgb(90,90,90)): the rule names no item
        EXPECT_TRUE(element->fillOf(ItemViewElement::Selected, ItemViewElement::OnItem) == QColor(101, 101, 101));
        EXPECT_TRUE(element->fillOf(ItemViewElement::Hover, ItemViewElement::OnItem) == QColor(90, 90, 90));

        // Another tree keeps the Global element's role
        const ItemViewElement* globalElement = ItemViewElement::forWidget(&other);
        ASSERT_TRUE(globalElement != nullptr);
        EXPECT_STREQ(globalElement->m_name, "Global.ItemView.Tree");
        EXPECT_TRUE(globalElement->m_selection == QColor(101, 101, 101));
    }

    // ----------------------------------------------------------------------------------
    // The New Level dialog sets a rule on its own template list (NewLevelDialog.cpp:79)
    // ----------------------------------------------------------------------------------

    TEST_F(EditorFlatSheetTests, NewLevelDialog_ItsTemplateListHasTheItemsOfItsInlineRuleAndNoOtherListHasThem)
    {
        using AzQtComponents::ItemViewElement;

        // `QListWidget::item { height: 148px; padding-left: 0px; padding-right: 0px; background-color: transparent }` is set on the dialog (a QDialog
        // named CNewLevelDialog, whose list is listTemplates: NewLevelDialog.ui:4, :15). No capture holds the dialog.
        ClassNamedWidget dialog("CNewLevelDialog");
        QListWidget* templates = new QListWidget(&dialog);
        templates->setObjectName("listTemplates");
        QWidget elsewhere;
        QListWidget* other = new QListWidget(&elsewhere);
        other->setObjectName("listTemplates");

        ItemViewElement::resolve(templates);
        ItemViewElement::resolve(other);

        const ItemViewElement* element = ItemViewElement::forWidget(templates);
        ASSERT_TRUE(element != nullptr);
        EXPECT_STREQ(element->m_name, "Editor.ItemView.NewLevelTemplateList");
        EXPECT_EQ(element->m_itemHeight, 148);
        EXPECT_EQ(element->m_itemPadding, QMargins(0, 0, 0, 0));

        // The items are 148px high and as wide as the base style makes them: no 4px of padding (the Global list's), whatever the item's width
        EXPECT_EQ(element->itemSize(QSize(100, 20)), QSize(100, 148));
        EXPECT_EQ(element->itemContentsRect(QRect(0, 0, 100, 148)), QRect(0, 0, 100, 148));

        // No fill in any state: the transparent background of the dialog's rule beats the application's `::item:selected` and `::item:hover`
        // (TableView.qss:42-60), which the Global list has as rgb(101,101,101) and rgb(90,90,90)
        for (const unsigned states : { unsigned(ItemViewElement::Selected), unsigned(ItemViewElement::Hover),
                                       unsigned(ItemViewElement::Selected | ItemViewElement::Active), unsigned(ItemViewElement::Selected | ItemViewElement::Hover) })
        {
            const QColor fill = element->fillOf(states, ItemViewElement::OnItem);
            EXPECT_TRUE(fill.isValid() && fill.alpha() == 0) << "states " << states;
        }

        // A list of the same name anywhere else has the Global list's items
        const ItemViewElement* globalElement = ItemViewElement::forWidget(other);
        ASSERT_TRUE(globalElement != nullptr);
        EXPECT_STREQ(globalElement->m_name, "Global.ItemView.List");
        EXPECT_EQ(globalElement->m_itemPadding, QMargins(4, 0, 0, 0));
        EXPECT_EQ(globalElement->itemSize(QSize(100, 20)), QSize(104, 24));
        EXPECT_TRUE(globalElement->fillOf(ItemViewElement::Selected, ItemViewElement::OnItem) == QColor(101, 101, 101));
    }
} // namespace UnitTest
