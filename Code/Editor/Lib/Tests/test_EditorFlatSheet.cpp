/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <AzQtComponents/AzQtComponentsAPI.h>
#include <AzQtComponents/Components/DockMainWindow.h>
#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/ItemViewElement.h>
#include <AzQtComponents/Components/Elements/TextElement.h>
#include <AzQtComponents/Components/Elements/ToolTipElement.h>
#include <AzQtComponents/Components/GuiPerfFloor.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/Widgets/ElidingLabel.h>
#include <AzQtComponents/Components/Widgets/ScrollBar.h>
#include <AzQtComponents/Tests/ToolTipTwins.h>
#include <AzToolsFramework/AssetEditor/AssetEditorHeader.h>
#include <AzToolsFramework/UI/DocumentPropertyEditor/DocumentPropertyEditor.h>
#include <AzToolsFramework/UI/PropertyEditor/InstanceDataHierarchy.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyRowWidget.hxx>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <AzToolsFramework/UI/UICore/ConsoleTextEdit.hxx>
#include <EditorFlatSheet.h>

#include <QApplication>
#include <QColor>
#include <QComboBox>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QImage>
#include <QLabel>
#include <QListWidget>
#include <QMainWindow>
#include <QMargins>
#include <QMenuBar>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QRect>
#include <QRegion>
#include <QScrollBar>
#include <QSize>
#include <QStyleFactory>
#include <QStyleOption>
#include <QToolBar>
#include <QTreeView>
#include <QVBoxLayout>
#include <QWidget>

#include <utility>

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

        //! Once for the process, as the flattened GUI runs the registrar of a sheet once: a Global element first, then the sheet's.
        //! Every fixture of this file asks for it.
        bool registerOnce()
        {
            static const bool registered = (AzQtComponents::BoxElement::registerElement(&GlobalProbe, &QWidget::staticMetaObject,
                [](const QWidget* widget) { return widget->objectName() == QLatin1String("AssetEditorStatusBar"); },
                AzQtComponents::ElementScope::Global), EditorFlatSheet::RegisterElements(), true);
            return registered;
        }

        class EditorFlatSheetTests
            : public LeakDetectionFixture
        {
        protected:
            void SetUp() override
            {
                LeakDetectionFixture::SetUp();
                m_style = AZStd::make_unique<AzQtComponents::Style>(QStyleFactory::create(QStringLiteral("Fusion")));
                m_fusion = AZStd::unique_ptr<QStyle>(QStyleFactory::create(QStringLiteral("Fusion")));
                ASSERT_TRUE(registerOnce());
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
    // The New Level dialog sets a rule on its own template list (NewLevelDialog.cpp:84)
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

    // ----------------------------------------------------------------------------------
    // The Asset Editor's header (Editor.qss:56-91): twins of the real header (Ui::AssetEditorHeader), set up as a tab of the Asset
    // Editor sets it up. The legacy twin is Qt's own style sheet style over Fusion, with Editor.qss on a window standing for the main
    // window and the application's rules above it; the flat twin is the flattened style with the elements. No capture holds the
    // header: the Asset Editor of the captures has no tab open.
    // ----------------------------------------------------------------------------------

    namespace EditorFlatSheetTestsInternal
    {
        // Editor.qss:56-91, whole: the rules of the header, in the main window's sheet (CryEdit.cpp:1571)
        constexpr const char* HeaderRules = R"(
Ui--AssetEditorHeader
{
    margin: 0px;
    padding: 0px;
}

Ui--AssetEditorHeader *
{
    height: 16px;
    background-color: #272727;
}

Ui--AssetEditorHeader #Background
{
    margin: 0px;
    padding: 2px 7px 3px 7px;
    border: 1px solid transparent;
    border-bottom: none;
    background-image: none;
    background-color: #272727;
}

Ui--AssetEditorHeader #Name
{
    height: 16px;
    font: italic;
    background-color: transparent;
}

Ui--AssetEditorHeader #Location
{
    height: 16px;
    font: italic;
    background-color: transparent;
}
)";

        // Text.qss:89-95, every label. With the universal rule and the tooltip rule (ToolTipTwins::AppRules), the rules of the
        // application's sheet that reach the header's widgets and their tips.
        constexpr const char* HeaderLabelRule =
            ".Label, QLabel { font-family: \"Open Sans\"; font-size: 12px; margin-top: 1px; margin-bottom: 1px; }\n";

        //! The name a tab gives its header (AssetEditorTab::SetupHeader): the label in bold, then the folder of the asset
        constexpr const char* HeaderName = "<b>Asset location: </b>Assets/Materials/Examples";

        constexpr int HeaderWidth = 371;                // a tab of the Asset Editor (m5 06_asset_editor: its stack is 371 px wide)
        constexpr int WindowHeight = 200;
        constexpr QRgb HeaderParentColor = 0x808080;    // what neither the sheet nor an element draws stays this grey
        constexpr QRgb HeaderFill = 0x272727;           // Editor.qss:66, :76

        //! The widgets of a header, by the names it gives them (AssetEditorHeader.cpp:17-19), and a label beside it
        struct HeaderParts
        {
            Ui::AssetEditorHeader* m_header = nullptr;
            QFrame* m_background = nullptr;
            QLabel* m_icon = nullptr;
            AzQtComponents::ElidingLabel* m_name = nullptr;
            QLabel* m_measure = nullptr;                //!< the hidden label the name measures its hint with (ElidingLabel.cpp:25)
            QLabel* m_beside = nullptr;                 //!< a label beside the header, below the same window
        };

        //! The tree of a twin: the root (the application's level), the window standing for the main window, and in it a header in
        //! the layout a tab puts it in (AssetEditorTab.cpp:231-241: no margins, no spacing, the property editor below it, here a
        //! label), with the name and the icon a tab gives it
        struct HeaderTree
        {
            QWidget m_root;
            QWidget* m_window = nullptr;
            HeaderParts m_parts;

            HeaderTree()
            {
                m_window = new QWidget(&m_root);
                m_window->setGeometry(0, 0, HeaderWidth, WindowHeight);
                auto* layout = new QVBoxLayout(m_window);
                layout->setContentsMargins(0, 0, 0, 0);
                layout->setSpacing(0);

                m_parts.m_header = new Ui::AssetEditorHeader(m_window);
                m_parts.m_header->setName(QString::fromLatin1(HeaderName));
                QPixmap icon(16, 16);
                icon.fill(QColor(0x80, 0xC0, 0xFF));
                m_parts.m_header->setIcon(QIcon(icon));
                layout->addWidget(m_parts.m_header);
                m_parts.m_beside = new QLabel(QStringLiteral("Beside"), m_window);
                layout->addWidget(m_parts.m_beside);
                layout->addStretch();

                m_parts.m_background = m_parts.m_header->findChild<QFrame*>(QStringLiteral("Background"), Qt::FindDirectChildrenOnly);
                m_parts.m_icon = m_parts.m_header->findChild<QLabel*>(QStringLiteral("Icon"));
                m_parts.m_name = m_parts.m_header->findChild<AzQtComponents::ElidingLabel*>(QStringLiteral("Name"));
                if (m_parts.m_name)
                {
                    m_parts.m_measure = m_parts.m_name->findChild<QLabel*>(QStringLiteral("Name"), Qt::FindDirectChildrenOnly);
                }
            }

            bool complete() const
            {
                return m_parts.m_background && m_parts.m_icon && m_parts.m_name && m_parts.m_measure;
            }

            //! Shown off screen, as an open tab is: the layouts place the bar, the icon and the name, and the name elides its text to
            //! its width
            void layOut()
            {
                m_root.setAttribute(Qt::WA_DontShowOnScreen);
                m_root.resize(HeaderWidth, WindowHeight);
                m_root.show();
                QApplication::processEvents();
            }
        };

        //! What a twin's header is: its size and the places of its parts, the frame boxes of the bar and the name, the name's text
        //! and fonts, and its pixels
        struct HeaderFacts
        {
            QSize m_headerHint;
            QRect m_header;                             //!< in the window
            QRect m_background;                         //!< in the header
            QMargins m_backgroundBox;                   //!< the bar's frame box (a QFrame's contents margins)
            QRect m_icon;                               //!< in the bar
            QRect m_name;                               //!< in the bar
            QMargins m_nameBox;
            QSize m_nameHint;
            QString m_nameText;                         //!< what the name shows: its text elided to its width
            QFont m_nameFont;
            QFont m_measureFont;
            bool m_backgroundStyled = false;            //!< the bar's fill is drawn at PE_Widget
            bool m_backgroundAutoFill = false;
            QImage m_images[3];                         //!< the header at the ratios of ToolTipTwins::Ratios, over HeaderParentColor
        };

        HeaderFacts takeFacts(const HeaderParts& parts)
        {
            HeaderFacts facts;
            facts.m_headerHint = parts.m_header->sizeHint();
            facts.m_header = parts.m_header->geometry();
            facts.m_background = parts.m_background->geometry();
            facts.m_backgroundBox = parts.m_background->contentsMargins();
            facts.m_icon = parts.m_icon->geometry();
            facts.m_name = parts.m_name->geometry();
            facts.m_nameBox = parts.m_name->contentsMargins();
            facts.m_nameHint = parts.m_name->sizeHint();
            facts.m_nameText = parts.m_name->elidedText();         // text() is the whole text the label was given
            facts.m_nameFont = parts.m_name->font();
            facts.m_measureFont = parts.m_measure->font();
            facts.m_backgroundStyled = parts.m_background->testAttribute(Qt::WA_StyledBackground);
            facts.m_backgroundAutoFill = parts.m_background->autoFillBackground();
            for (int i = 0; i < 3; ++i)
            {
                const qreal ratio = AzQtComponents::ToolTipTwins::Ratios[i];
                QImage& image = facts.m_images[i];
                image = QImage((QSizeF(parts.m_header->size()) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
                image.setDevicePixelRatio(ratio);
                image.fill(QColor::fromRgb(HeaderParentColor));

                // What the header and the widgets in it paint over the window, without the window's background
                QPainter painter(&image);
                parts.m_header->render(&painter, QPoint(), QRegion(), QWidget::DrawChildren);
                painter.end();
            }
            return facts;
        }

        QRgb rgbAt(const QImage& image, int x, int y)
        {
            return image.pixel(x, y) & 0xFFFFFF;
        }

        class AssetEditorHeaderFlatSheetTests
            : public LeakDetectionFixture
        {
        protected:
            void SetUp() override
            {
                LeakDetectionFixture::SetUp();

                // The twins name their own sheets. The flattened branches of Style and the elements read the rung.
                m_applicationSheet = qApp->styleSheet();
                qApp->setStyleSheet(QString());
                m_applicationFont = QApplication::font();
                AzQtComponents::GuiPerfFloor::setRungForTesting(AzQtComponents::GuiPerfFloor::Rung::Flat);

                m_style = AZStd::make_unique<AzQtComponents::Style>(QStyleFactory::create(QStringLiteral("Fusion")));
                m_fusion = AZStd::unique_ptr<QStyle>(QStyleFactory::create(QStringLiteral("Fusion")));
                ASSERT_TRUE(registerOnce());
            }

            void TearDown() override
            {
                m_fusion.reset();
                m_style.reset();
                AzQtComponents::GuiPerfFloor::resetForTesting();
                QApplication::setFont(m_applicationFont);       // without a class name: the class fonts of a flat twin are cleared
                qApp->setStyleSheet(m_applicationSheet);
                LeakDetectionFixture::TearDown();
            }

            //! A style on a widget and on each below it: a style set on a widget reaches no child without a sheet
            static void useStyle(QWidget& root, QStyle* style)
            {
                root.setStyle(style);
                for (QWidget* child : root.findChildren<QWidget*>())
                {
                    child->setStyle(style);
                }
            }

            //! The legacy GUI: Qt's style sheet style over Fusion, the application's rules on the root and Editor.qss on the window
            //! below it, the deeper sheet, as on the main window
            void toLegacy(QWidget& root, QWidget& window)
            {
                useStyle(root, m_fusion.get());
                root.setStyleSheet(QString::fromLatin1(AzQtComponents::ToolTipTwins::AppRules) + QString::fromLatin1(HeaderLabelRule));
                window.setStyleSheet(QString::fromLatin1(HeaderRules));
                root.ensurePolished();
            }

            //! The flattened GUI: the class fonts and the palette of its application, the flattened style on every widget, and the
            //! polish that resolves the elements
            void toFlat(QWidget& root)
            {
                AzQtComponents::TextElement::installClassFonts();
                useStyle(root, m_style.get());
                QPalette palette = root.palette();
                static_cast<QStyle*>(m_style.get())->polish(palette);
                root.setPalette(palette);
                root.ensurePolished();
            }

            //! The header built, styled, shown and measured in one GUI; the tree is gone when the facts are taken. A header that did not
            //! build the parts the twins read gives no facts.
            HeaderFacts legacyFacts()
            {
                HeaderTree tree;
                if (!tree.complete())
                {
                    ADD_FAILURE() << "the header has not the parts it names";
                    return HeaderFacts();
                }
                toLegacy(tree.m_root, *tree.m_window);
                tree.layOut();
                return takeFacts(tree.m_parts);
            }

            HeaderFacts flatFacts()
            {
                HeaderTree tree;
                if (!tree.complete())
                {
                    ADD_FAILURE() << "the header has not the parts it names";
                    return HeaderFacts();
                }
                toFlat(tree.m_root);
                tree.layOut();
                return takeFacts(tree.m_parts);
            }

            //! The element of this sheet a widget resolved to, or "" (a Global element of another module may cover a widget)
            static const char* editorBoxOf(const QWidget& widget)
            {
                const AzQtComponents::BoxElement* element = AzQtComponents::BoxElement::forWidget(&widget);
                return (element && qstrncmp(element->m_name, "Editor.Box.", 11) == 0) ? element->m_name : "";
            }

            QString m_applicationSheet;
            QFont m_applicationFont;
            AZStd::unique_ptr<AzQtComponents::Style> m_style;
            AZStd::unique_ptr<QStyle> m_fusion;
        };
    } // namespace EditorFlatSheetTestsInternal

    using EditorFlatSheetTestsInternal::AssetEditorHeaderFlatSheetTests;
    namespace HeaderFlat = EditorFlatSheetTestsInternal;

    TEST_F(AssetEditorHeaderFlatSheetTests, Registry_TheHeaderItsBarItsIconAndItsNameHaveTheElementsAndTheirTipsTheFill)
    {
        HeaderFlat::HeaderTree tree;
        ASSERT_TRUE(tree.complete());
        toFlat(tree.m_root);
        const HeaderFlat::HeaderParts& parts = tree.m_parts;

        EXPECT_STREQ(editorBoxOf(*parts.m_header), "Editor.Box.AssetEditorHeader");
        EXPECT_STREQ(editorBoxOf(*parts.m_background), "Editor.Box.AssetEditorHeaderBackground");
        EXPECT_STREQ(editorBoxOf(*parts.m_icon), "Editor.Box.AssetEditorHeaderLabel");
        EXPECT_STREQ(editorBoxOf(*parts.m_name), "Editor.Box.AssetEditorHeaderName");
        EXPECT_STREQ(editorBoxOf(*parts.m_measure), "Editor.Box.AssetEditorHeaderName");
        EXPECT_STREQ(editorBoxOf(*parts.m_beside), "");
        EXPECT_STREQ(editorBoxOf(*tree.m_window), "");

        // The tip of the header and of each widget below it has the header's fill; the tip of a widget beside it has none
        const QColor fill = QColor::fromRgb(HeaderFlat::HeaderFill);
        for (const QWidget* owner : { static_cast<const QWidget*>(parts.m_header), static_cast<const QWidget*>(parts.m_background),
                                      static_cast<const QWidget*>(parts.m_icon), static_cast<const QWidget*>(parts.m_name) })
        {
            EXPECT_EQ(AzQtComponents::ToolTipElement::lookFor(owner).m_fill, fill) << owner->objectName().toStdString();
        }
        EXPECT_FALSE(AzQtComponents::ToolTipElement::lookFor(parts.m_beside).m_fill.isValid());
        EXPECT_FALSE(AzQtComponents::ToolTipElement::lookFor(tree.m_window).m_fill.isValid());
    }

    TEST_F(AssetEditorHeaderFlatSheetTests, Registry_TheNamesOfTheHeadersPartsOutsideAHeaderHaveNoneOfItsElements)
    {
        // A frame and labels with the names the header gives its parts (AssetEditorHeader.cpp:17-19), in no header
        QWidget root;
        auto* background = new QFrame(&root);
        background->setObjectName(QStringLiteral("Background"));
        auto* icon = new QLabel(&root);
        icon->setObjectName(QStringLiteral("Icon"));
        auto* name = new QLabel(QStringLiteral("Name"), &root);
        name->setObjectName(QStringLiteral("Name"));
        toFlat(root);

        EXPECT_STREQ(editorBoxOf(*background), "");
        EXPECT_STREQ(editorBoxOf(*icon), "");
        EXPECT_STREQ(editorBoxOf(*name), "");
        EXPECT_FALSE(name->font().italic());
        EXPECT_FALSE(AzQtComponents::ToolTipElement::lookFor(name).m_fill.isValid());
    }

    TEST_F(AssetEditorHeaderFlatSheetTests, Box_TheHeaderAndItsPartsHaveThePlacesAndTheBoxesTheSheetGaveThem)
    {
        const HeaderFlat::HeaderFacts legacy = legacyFacts();
        const HeaderFlat::HeaderFacts flat = flatFacts();

        // The sheet's header: the bar's border (1px at the top and the sides) and its padding put the icon and the name 8px in from
        // the sides and 3px from the top and the bottom (Editor.qss:72-74), the bar is the whole header, the header is as high as
        // the bar asks, and the name has every label's box (Text.qss:93-94)
        EXPECT_EQ(legacy.m_backgroundBox, QMargins(8, 3, 8, 3));
        EXPECT_EQ(legacy.m_background, QRect(QPoint(0, 0), legacy.m_header.size()));
        EXPECT_EQ(legacy.m_header.height(), legacy.m_headerHint.height());
        EXPECT_EQ(legacy.m_header.height(), 3 + qMax(16, legacy.m_nameHint.height()) + 3);
        EXPECT_EQ(legacy.m_icon.left(), 8);
        EXPECT_EQ(legacy.m_name.left(), 8 + 16);
        EXPECT_EQ(legacy.m_nameBox, QMargins(0, 1, 0, 1));
        EXPECT_TRUE(legacy.m_backgroundStyled);
        EXPECT_FALSE(legacy.m_backgroundAutoFill);      // the sheet's fill replaces the one the frame asks for

        EXPECT_EQ(flat.m_headerHint, legacy.m_headerHint);
        EXPECT_EQ(flat.m_header, legacy.m_header);
        EXPECT_EQ(flat.m_background, legacy.m_background);
        EXPECT_EQ(flat.m_backgroundBox, legacy.m_backgroundBox);
        EXPECT_EQ(flat.m_icon, legacy.m_icon);
        EXPECT_EQ(flat.m_name, legacy.m_name);
        EXPECT_EQ(flat.m_nameBox, legacy.m_nameBox);
        EXPECT_EQ(flat.m_nameHint, legacy.m_nameHint);
        EXPECT_EQ(flat.m_nameText.toStdString(), legacy.m_nameText.toStdString());
        EXPECT_EQ(flat.m_backgroundStyled, legacy.m_backgroundStyled);
        EXPECT_EQ(flat.m_backgroundAutoFill, legacy.m_backgroundAutoFill);
    }

    TEST_F(AssetEditorHeaderFlatSheetTests, Text_TheNameAndTheLabelItMeasuresWithAreItalicAsUnderTheSheet)
    {
        const HeaderFlat::HeaderFacts legacy = legacyFacts();
        const HeaderFlat::HeaderFacts flat = flatFacts();

        // `font: italic` (Editor.qss:82) over the label rule's family and size (Text.qss:91-92), with the weight the shorthand sets.
        // The font is read as the widget asks for it: this process has no Open Sans to match.
        for (const QFont* font : { &legacy.m_nameFont, &legacy.m_measureFont })
        {
            EXPECT_TRUE(font->italic());
            EXPECT_EQ(font->pixelSize(), 12);
            EXPECT_EQ(font->weight(), QFont::Normal);
            EXPECT_EQ(font->family().toStdString(), "Open Sans");
        }

        const auto expectSame = [](const QFont& flatFont, const QFont& legacyFont, const char* what)
        {
            EXPECT_EQ(flatFont.italic(), legacyFont.italic()) << what;
            EXPECT_EQ(flatFont.pixelSize(), legacyFont.pixelSize()) << what;
            EXPECT_EQ(flatFont.weight(), legacyFont.weight()) << what;
            EXPECT_EQ(flatFont.family().toStdString(), legacyFont.family().toStdString()) << what;
        };
        expectSame(flat.m_nameFont, legacy.m_nameFont, "the name");
        expectSame(flat.m_measureFont, legacy.m_measureFont, "the label the name measures with");
    }

    TEST_F(AssetEditorHeaderFlatSheetTests, Pixels_TheHeaderPaintsWhatTheSheetPaintedAtEveryRatio)
    {
        namespace Twins = AzQtComponents::ToolTipTwins;
        const HeaderFlat::HeaderFacts legacy = legacyFacts();
        const HeaderFlat::HeaderFacts flat = flatFacts();

        // The sheet's bar fills the header to its edges, its transparent border too (Editor.qss:73-76)
        const QImage& sheet = legacy.m_images[0];
        ASSERT_FALSE(sheet.isNull());
        EXPECT_EQ(HeaderFlat::rgbAt(sheet, 0, 0), HeaderFlat::HeaderFill);
        EXPECT_EQ(HeaderFlat::rgbAt(sheet, sheet.width() - 1, 0), HeaderFlat::HeaderFill);
        EXPECT_EQ(HeaderFlat::rgbAt(sheet, 0, sheet.height() - 1), HeaderFlat::HeaderFill);
        EXPECT_EQ(HeaderFlat::rgbAt(sheet, sheet.width() - 1, sheet.height() - 1), HeaderFlat::HeaderFill);

        for (int i = 0; i < 3; ++i)
        {
            EXPECT_EQ(Twins::differingPixels(legacy.m_images[i], flat.m_images[i]), 0) << "at ratio " << Twins::Ratios[i];
        }
    }

    TEST_F(AssetEditorHeaderFlatSheetTests, ToolTip_TheNameShowsItsTipInTheHeadersFillAndALabelBesideTheHeaderDoesNot)
    {
        AzQtComponents::ToolTipTwins::Guard tips;
        AzQtComponents::ToolTipTwins::StyleFilter filter(m_style.get());
        namespace Twins = AzQtComponents::ToolTipTwins;

        for (const bool inHeader : { true, false })
        {
            SCOPED_TRACE(inHeader ? "the name of the header" : "a label beside the header");

            // The legacy tip: the sheet style resolves it through the widget it is shown for, so the main window's sheet reaches it
            Twins::TipFacts legacy;
            {
                HeaderFlat::HeaderTree tree;
                ASSERT_TRUE(tree.complete());
                toLegacy(tree.m_root, *tree.m_window);
                QLabel* owner = inHeader ? static_cast<QLabel*>(tree.m_parts.m_name) : tree.m_parts.m_beside;
                owner->setToolTip(QStringLiteral("Assets/Materials/Examples"));
                legacy = Twins::showAndReadTip(owner, nullptr);
            }

            // The flat tip. The font and the margin of its label are the legacy tip's: this process has the platform's fonts and
            // style, and what a fill proves is not a font
            Twins::TipFacts flat;
            {
                HeaderFlat::HeaderTree tree;
                ASSERT_TRUE(tree.complete());
                QLabel* owner = inHeader ? static_cast<QLabel*>(tree.m_parts.m_name) : tree.m_parts.m_beside;
                owner->setToolTip(QStringLiteral("Assets/Materials/Examples"));
                flat = Twins::showAndReadTip(owner, m_style.get(), &legacy.m_font, legacy.m_margin);
            }

            ASSERT_TRUE(legacy.m_found && flat.m_found);
            EXPECT_EQ(legacy.m_base.name(), inHeader ? QStringLiteral("#272727") : QStringLiteral("#000000"));
            EXPECT_EQ(flat.m_base.name(), legacy.m_base.name());
            EXPECT_EQ(flat.m_text.name(), legacy.m_text.name());
            EXPECT_EQ(Twins::marginsText(flat.m_contents), Twins::marginsText(legacy.m_contents));
            EXPECT_EQ(flat.m_hint, legacy.m_hint);
            EXPECT_EQ(flat.m_contentsRect, legacy.m_contentsRect);
            for (int i = 0; i < 3; ++i)
            {
                EXPECT_EQ(Twins::differingPixels(legacy.m_images[i], flat.m_images[i]), 0) << "at ratio " << Twins::Ratios[i];
            }
        }
    }

    // ----------------------------------------------------------------------------------
    // The rows of the Asset Editor's property editor (Editor.qss:93-107) and the output of the Python Console (Editor.qss:187-201).
    // The twins are built from the real classes the selectors name (AzToolsFramework::PropertyRowWidget, DPERowWidget and
    // ConsoleTextEdit), with the container rows initialized on the nodes of a real hierarchy of a reflected class, as a property
    // editor initializes them. The legacy twin is Qt's own style sheet style over Fusion, with the rules of Editor.qss on a window
    // standing for the main window (CryEdit.cpp:1571) and the application's rules above it; the flat twin is the flattened style with
    // the elements.
    // ----------------------------------------------------------------------------------

    namespace EditorFlatSheetTestsInternal
    {
        // Editor.qss:93-107, whole: the rules of the rows of the Asset Editor's property editor
        constexpr const char* RowRules = R"sheet(
#AssetEditorWidgetPropertyEditor AzToolsFramework--PropertyRowWidget[isTopLevel="true"]
{
    qproperty-appendDefaultLabelToName:1;
}

#AssetEditorWidgetPropertyEditor AzToolsFramework--PropertyRowWidget[isTopLevel="false"]
{
    qproperty-appendDefaultLabelToName:0;
}

#AssetEditorWidgetPropertyEditor AzToolsFramework--PropertyRowWidget[getLevel="1"][hasChildRows="true"]
, #AssetEditorWidgetPropertyEditor AzToolsFramework--DPERowWidget[getLevel="2"][hasChildRows="true"]
{
    border-top: 1px solid rgba(255, 255, 255, 0.2);
}
)sheet";

        // Editor.qss:187-201, whole: the rule of the Python Console (and, by its three first selectors, of nothing)
        constexpr const char* ConsoleRules = R"sheet(
ConsoleTextEdit,
ConsoleTextEdit:hover,
ConsoleTextEdit:focus,
#SCRIPT_OUTPUT,
#SCRIPT_OUTPUT:hover,
#SCRIPT_OUTPUT:focus
{
    margin: 0;
    margin-top: 6px;
    background-color: #222222;
    border-radius: 0px;
    border-width: 0px;
    border-color: #e9e9e9;
    border-style: solid;
}
)sheet";

        // BaseStyleSheet.qss:15-21, the universal rule: the application's rule that reaches every row
        constexpr const char* UniversalRule =
            "* { background-color: transparent; color: white; font-family: \"Open Sans\"; font-weight: 400; }\n";

        // The rules of the application's sheet that reach a text edit, in the order of the sheet: BaseStyleSheet.qss:68-84 (the entry
        // fields), :86-95 (the text edits), :97-105 (hovered), :107-115 (focused), and QDockWidget.qss:33-36 (every scroll area)
        constexpr const char* TextEditAppRules = R"sheet(
QTextEdit, QPlainText, QPlainTextEdit, QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox
{
    background-color: #CCCCCC;
    color: black;
    font-family: "Open Sans";
    border-radius: 2px;
    border-width: 0px;
    border-color: #e9e9e9;
    font-size: 12px;
    line-height: 16px;
}

QTextEdit, QPlainText, QPlainTextEdit
{
    background-color: #E9E9E9;
    color: #545454;
    margin: 2px;
    border-width: 0px;
    border-style: solid;
}

QTextEdit:hover, QPlainText:hover, QPlainTextEdit:hover
{
    margin: 1px;
    border-width: 1px;
    border-style: solid;
    border-color: #222222;
}

QTextEdit:focus, QPlainText:focus, QPlainTextEdit:focus
{
    margin: 1px;
    border-width: 1px;
    border-color: #94D2FF;
    background-color: #E9E9E9;
}

QAbstractScrollArea
{
    margin: 0;
}
)sheet";

        constexpr QSize RowBoxSize(150, 40);            // the box a row is given in the twins
        constexpr QSize RowNameSize(80, 16);            // the box of the name of a row
        constexpr QRgb RowsParentColor = 0x808080;      // what neither the sheet nor an element draws stays this grey

        //! The elements of this sheet for the rows, by name
        constexpr const char* TopLevelGroupRowElement = "Editor.Box.AssetEditorTopLevelGroupRow";
        constexpr const char* TopLevelRowElement = "Editor.Box.AssetEditorTopLevelRow";
        constexpr const char* GroupRowElement = "Editor.Box.AssetEditorGroupRow";
        constexpr const char* ConsoleElement = "Editor.Box.PythonConsoleOutput";

        // ------------------------------------------------------------------------------
        // The rows: a tree of them below the property editor, and the same tree below a widget of another name
        // ------------------------------------------------------------------------------

        //! A class with two containers, as the objects the Asset Editor shows have them: the container rows are built on its nodes
        struct ContainerHolder
        {
            AZ_TYPE_INFO(ContainerHolder, "{6D0C3E5A-8E0F-4E7B-9C53-2F4B7A1D9E21}");

            static void Reflect(AZ::SerializeContext& context)
            {
                context.Class<ContainerHolder>()
                    ->Field("Values", &ContainerHolder::m_values)
                    ->Field("Others", &ContainerHolder::m_others)
                    ;
            }

            AZStd::vector<int> m_values{ 1, 2 };
            AZStd::vector<int> m_others{ 3, 4, 5 };
        };

        //! The rows of one tree. A row is top level when no row above it has a name, and has a level, the number of rows above it.
        enum RowName
        {
            UnnamedRoot,    //!< level 0, no name: not top level (a row needs a name for that), has child rows
            Values,         //!< the container "Values": level 1, top level (the root has no name), has a child row: both rules reach it
            ValuesChild,    //!< level 2, not top level
            Plain,          //!< level 1, top level, no child rows: the first rule alone
            NamedRoot,      //!< level 0, top level, has child rows: the first rule alone
            Others,         //!< the container "Others": level 1, not top level (the root has a name), has a child row: the second alone
            OthersChild,    //!< level 2, not top level
            RowCount,
        };

        constexpr const char* RowNames[RowCount] = {
            "the unnamed root", "the container Values", "the child of Values", "the plain row", "the named root", "the container Others",
            "the child of Others",
        };

        struct RowSet
        {
            AzToolsFramework::PropertyRowWidget* m_rows[RowCount] = {};
        };

        AzToolsFramework::PropertyRowWidget* newRow(QWidget& parent)
        {
            auto* row = new AzToolsFramework::PropertyRowWidget(&parent);
            // A row that is not overridden has the property a property editor gives it; PropertyRowWidget::SetOverridden would
            // otherwise repolish the row through the StyleManager, which these tests have not
            row->setProperty("IsOverridden", false);
            row->setGeometry(0, 0, RowBoxSize.width(), RowBoxSize.height());
            return row;
        }

        //! A row with a name (none: a row that shows no name), below a parent row
        AzToolsFramework::PropertyRowWidget* newNamedRow(QWidget& parent, AzToolsFramework::PropertyRowWidget* parentRow, const char* name)
        {
            AzToolsFramework::PropertyRowWidget* row = newRow(parent);
            row->SetNameLabel(name);
            row->GetNameLabel()->setGeometry(0, 0, RowNameSize.width(), RowNameSize.height());
            if (parentRow)
            {
                row->SetParentRow(parentRow);
                parentRow->AddedChild(row);
            }
            return row;
        }

        //! A container row, initialized on the node of its container below a parent row, as a property editor does
        AzToolsFramework::PropertyRowWidget* newContainerRow(
            QWidget& parent, AzToolsFramework::PropertyRowWidget* parentRow, AzToolsFramework::InstanceDataNode* node)
        {
            AzToolsFramework::PropertyRowWidget* row = newRow(parent);
            row->Initialize(parentRow, node, parentRow ? 1 : 0);
            row->GetNameLabel()->setGeometry(0, 0, RowNameSize.width(), RowNameSize.height());
            if (parentRow)
            {
                parentRow->AddedChild(row);
            }
            return row;
        }

        AzToolsFramework::InstanceDataNode* containerNode(AzToolsFramework::InstanceDataHierarchy& hierarchy, const char* field)
        {
            for (AzToolsFramework::InstanceDataNode& child : hierarchy.GetChildren())
            {
                if (child.GetElementMetadata() && AZStd::string_view(child.GetElementMetadata()->m_name) == field)
                {
                    return &child;
                }
            }
            return nullptr;
        }

        //! The rows of the tree below a parent widget, in the order a property editor builds them
        RowSet buildRows(QWidget& parent, AzToolsFramework::InstanceDataHierarchy& hierarchy)
        {
            RowSet set;
            set.m_rows[UnnamedRoot] = newNamedRow(parent, nullptr, "");
            set.m_rows[Values] = newContainerRow(parent, set.m_rows[UnnamedRoot], containerNode(hierarchy, "Values"));
            set.m_rows[ValuesChild] = newNamedRow(parent, set.m_rows[Values], "0");
            set.m_rows[Plain] = newNamedRow(parent, set.m_rows[UnnamedRoot], "Plain");
            set.m_rows[NamedRoot] = newNamedRow(parent, nullptr, "Settings");
            set.m_rows[Others] = newContainerRow(parent, set.m_rows[NamedRoot], containerNode(hierarchy, "Others"));
            set.m_rows[OthersChild] = newNamedRow(parent, set.m_rows[Others], "0");
            return set;
        }

        //! A row of the document property editor with the level and the child rows a test gives it. The members it sets are the
        //! row's own (DPERowWidget::GetLevel and HasChildRows read them); the child is a row below it, which Qt parents.
        class DpeRowProbe
            : public AzToolsFramework::DPERowWidget
        {
        public:
            explicit DpeRowProbe(QWidget* parent, int level)
            {
                setParent(parent);
                m_depth = level;
                setGeometry(0, 0, RowBoxSize.width(), RowBoxSize.height());
            }

            ~DpeRowProbe() override
            {
                m_domOrderedChildren.clear();       // the children are Qt's, below this row: the row has no pool to give them back to
            }

            void addChildRow(DpeRowProbe* child)
            {
                m_domOrderedChildren.push_back(child);
            }
        };

        //! The rows of the document property editor: at the level the rule names with a child, with none, a level above and below
        enum DpeName
        {
            DpeGroup,       //!< level 2, has a child row
            DpeGroupChild,  //!< level 3, no child rows
            DpeLeaf,        //!< level 2, no child rows
            DpeShallow,     //!< level 1, has a child row
            DpeDeep,        //!< level 3, has a child row
            DpeCount,
        };

        struct DpeSet
        {
            DpeRowProbe* m_rows[DpeCount] = {};
        };

        DpeSet buildDpeRows(QWidget& parent)
        {
            DpeSet set;
            set.m_rows[DpeGroup] = new DpeRowProbe(&parent, 2);
            set.m_rows[DpeGroupChild] = new DpeRowProbe(set.m_rows[DpeGroup], 3);
            set.m_rows[DpeGroup]->addChildRow(set.m_rows[DpeGroupChild]);
            set.m_rows[DpeLeaf] = new DpeRowProbe(&parent, 2);
            set.m_rows[DpeShallow] = new DpeRowProbe(&parent, 1);
            auto* shallowChild = new DpeRowProbe(set.m_rows[DpeShallow], 2);
            set.m_rows[DpeShallow]->addChildRow(shallowChild);
            set.m_rows[DpeDeep] = new DpeRowProbe(&parent, 3);
            auto* deepChild = new DpeRowProbe(set.m_rows[DpeDeep], 4);
            set.m_rows[DpeDeep]->addChildRow(deepChild);
            return set;
        }

        //! The window standing for the main window, with the property editor in it, the same rows below a widget of another name, and
        //! the rows of the document property editor below a widget of the editor's name. The hierarchy and its context outlive the rows.
        struct RowTree
        {
            RowTree()
            {
                ContainerHolder::Reflect(m_context);
                m_hierarchy.AddRootInstance(&m_holder, AZ::AzTypeInfo<ContainerHolder>::Uuid());
                m_hierarchy.Build(&m_context, AZ::SerializeContext::ENUM_ACCESS_FOR_READ);

                m_window = new QWidget(&m_root);
                m_window->setGeometry(0, 0, 400, 400);
                m_editor = new AzToolsFramework::ReflectedPropertyEditor(m_window);
                m_editor->setObjectName(QStringLiteral("AssetEditorWidgetPropertyEditor"));
                m_elsewhere = new QWidget(m_window);
                m_elsewhere->setObjectName(QStringLiteral("SomeOtherEditor"));
                m_dpeEditor = new QWidget(m_window);
                m_dpeEditor->setObjectName(QStringLiteral("AssetEditorWidgetPropertyEditor"));

                m_inEditor = buildRows(*m_editor, m_hierarchy);
                m_outside = buildRows(*m_elsewhere, m_hierarchy);
                m_dpe = buildDpeRows(*m_dpeEditor);
            }

            AZ::SerializeContext m_context;
            ContainerHolder m_holder;
            AzToolsFramework::InstanceDataHierarchy m_hierarchy;
            QWidget m_root;
            QWidget* m_window = nullptr;
            AzToolsFramework::ReflectedPropertyEditor* m_editor = nullptr;      //!< named AssetEditorWidgetPropertyEditor
            QWidget* m_elsewhere = nullptr;
            QWidget* m_dpeEditor = nullptr;                                     //!< named AssetEditorWidgetPropertyEditor as well
            RowSet m_inEditor;
            RowSet m_outside;
            DpeSet m_dpe;
        };

        // ------------------------------------------------------------------------------
        // What a twin's row is
        // ------------------------------------------------------------------------------

        //! The box of a frame, and what it paints as its styled background (the rule is drawn once, at PE_Widget: the row's own paint
        //! draws no frame)
        struct BoxFacts
        {
            QRect m_contents;                   //!< QWidget::contentsRect: where the contents sit inside the box
            QMargins m_margins;
            bool m_styled = false;
            QImage m_images[3];                 //!< the box at the ratios of ToolTipTwins::Ratios, over RowsParentColor
        };

        struct RowFacts : BoxFacts
        {
            QString m_label;                    //!< the name the row shows
            bool m_defaultLabelHidden = false;
            QSize m_hint;
        };

        void takeBoxFacts(const QFrame& frame, BoxFacts& facts)
        {
            facts.m_contents = frame.contentsRect();
            facts.m_margins = frame.contentsMargins();
            facts.m_styled = frame.testAttribute(Qt::WA_StyledBackground);
            for (int i = 0; i < 3; ++i)
            {
                const qreal ratio = AzQtComponents::ToolTipTwins::Ratios[i];
                QImage& image = facts.m_images[i];
                image = QImage((QSizeF(frame.size()) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
                image.setDevicePixelRatio(ratio);
                image.fill(QColor::fromRgb(RowsParentColor));
                QPainter painter(&image);
                if (facts.m_styled)
                {
                    QStyleOption option;
                    option.initFrom(&frame);
                    frame.style()->drawPrimitive(QStyle::PE_Widget, &option, &painter, &frame);
                }
                painter.end();
            }
        }

        RowFacts takeRowFacts(AzToolsFramework::PropertyRowWidget& row)
        {
            RowFacts facts;
            takeBoxFacts(row, facts);
            facts.m_label = row.label();
            const QLabel* defaultLabel = row.findChild<QLabel*>(QStringLiteral("DefaultLabel"));
            facts.m_defaultLabelHidden = defaultLabel && defaultLabel->isHidden();
            facts.m_hint = row.sizeHint();
            return facts;
        }

        struct RowTreeFacts
        {
            RowFacts m_inEditor[RowCount];
            RowFacts m_outside[RowCount];
            BoxFacts m_dpe[DpeCount];
        };

        RowTreeFacts takeTreeFacts(RowTree& tree)
        {
            RowTreeFacts facts;
            for (int row = 0; row < RowCount; ++row)
            {
                facts.m_inEditor[row] = takeRowFacts(*tree.m_inEditor.m_rows[row]);
                facts.m_outside[row] = takeRowFacts(*tree.m_outside.m_rows[row]);
            }
            for (int row = 0; row < DpeCount; ++row)
            {
                takeBoxFacts(*tree.m_dpe.m_rows[row], facts.m_dpe[row]);
            }
            return facts;
        }

        // ------------------------------------------------------------------------------
        // The Python Console: the output and, beside it, the Editor's Console
        // ------------------------------------------------------------------------------

        constexpr QSize ConsoleBoxSize(240, 120);

        //! The states the rule is declared for (:hover, :focus), as the option of a paint carries them
        enum ConsoleState
        {
            Idle,
            Hovered,
            Focused,
            HoveredAndFocused,
            StateCount,
        };

        constexpr const char* ConsoleStateNames[StateCount] = { "idle", "hovered", "focused", "hovered and focused" };

        QStyle::State consoleStateFlags(int state)
        {
            QStyle::State flags;
            if (state == Hovered || state == HoveredAndFocused)
            {
                flags |= QStyle::State_MouseOver;
            }
            if (state == Focused || state == HoveredAndFocused)
            {
                flags |= QStyle::State_HasFocus;
            }
            return flags;
        }

        //! A window standing for the main window, a pane in it (CScriptTermDialog), and in the pane the output of the Python Console
        //! (ScriptTermDialog.ui: a ConsoleTextEdit named SCRIPT_OUTPUT, read only) and the Editor's Console (ConsoleSCB.ui: a
        //! ConsoleTextEdit named textEdit). Each has the colour of its console theme as ScrollBar::setBackgroundColor gives it
        //! (ScriptTermDialog.cpp, ConsoleSCB.cpp:471); a legacy twin sets the inline sheet that call sets in the legacy GUI
        //! (ScrollBar.cpp, setBackgroundColor).
        struct ConsoleTree
        {
            ConsoleTree(const QColor& color, bool flat, bool verticalBar = false)
            {
                m_window = new QWidget(&m_root);
                m_window->setGeometry(0, 0, 300, 300);
                m_pane = new QWidget(m_window);
                m_pane->setGeometry(0, 0, 300, 300);

                m_output = new AzToolsFramework::ConsoleTextEdit(m_pane);
                m_output->setObjectName(QStringLiteral("SCRIPT_OUTPUT"));
                m_console = new AzToolsFramework::ConsoleTextEdit(m_pane);
                m_console->setObjectName(QStringLiteral("textEdit"));
                int top = 0;
                for (AzToolsFramework::ConsoleTextEdit* edit : { m_output, m_console })
                {
                    edit->setReadOnly(true);
                    // Only the field is compared, unless a test asks for the bar: the look of the bars is the bars' (the bar is
                    // ItemViewElementTests' Console_ tests')
                    edit->setVerticalScrollBarPolicy(verticalBar ? Qt::ScrollBarAlwaysOn : Qt::ScrollBarAlwaysOff);
                    edit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                    edit->setGeometry(10, top, ConsoleBoxSize.width(), ConsoleBoxSize.height());
                    top += ConsoleBoxSize.height() + 10;
                    if (flat)
                    {
                        AzQtComponents::ScrollBar::setBackgroundColor(edit, color);
                    }
                    else
                    {
                        edit->setStyleSheet(QStringLiteral("background: %1").arg(color.name(QColor::HexRgb)));
                    }
                }
            }

            QWidget m_root;
            QWidget* m_window = nullptr;
            QWidget* m_pane = nullptr;
            AzToolsFramework::ConsoleTextEdit* m_output = nullptr;      //!< #SCRIPT_OUTPUT
            AzToolsFramework::ConsoleTextEdit* m_console = nullptr;     //!< the Editor's Console, which no rule of Editor.qss reaches
        };

        //! What a console's field is: the box in each state, its pixels in each state at each ratio, and, rendered whole in its idle
        //! state, the field with its viewport
        struct ConsoleFacts
        {
            QRect m_contents[StateCount];       //!< SE_ShapedFrameContents for the state of the option
            QRect m_widgetContents;
            QMargins m_margins;
            QRect m_viewport;
            QRect m_verticalBar;                //!< where the lane of the vertical scroll bar sits (a test that shows it)
            bool m_styled = false;
            bool m_autoFill = false;
            bool m_viewportAutoFill = false;
            QImage m_boxes[StateCount][3];      //!< PE_Widget of the console in the state, at the ratios, over RowsParentColor
            QImage m_rendered[3];               //!< the console rendered with its viewport, over RowsParentColor
        };

        ConsoleFacts takeConsoleFacts(QPlainTextEdit& console)
        {
            ConsoleFacts facts;
            facts.m_widgetContents = console.contentsRect();
            facts.m_margins = console.contentsMargins();
            facts.m_viewport = console.viewport()->geometry();
            facts.m_verticalBar = console.verticalScrollBar()->parentWidget()->geometry();     // the lane: the bar's container
            facts.m_styled = console.testAttribute(Qt::WA_StyledBackground);
            facts.m_autoFill = console.autoFillBackground();
            facts.m_viewportAutoFill = console.viewport()->autoFillBackground();

            for (int state = 0; state < StateCount; ++state)
            {
                QStyleOptionFrame frameOption;
                frameOption.initFrom(&console);
                frameOption.frameShape = console.frameShape();
                frameOption.state |= consoleStateFlags(state);
                facts.m_contents[state] = console.style()->subElementRect(QStyle::SE_ShapedFrameContents, &frameOption, &console);

                for (int i = 0; i < 3; ++i)
                {
                    const qreal ratio = AzQtComponents::ToolTipTwins::Ratios[i];
                    QImage& image = facts.m_boxes[state][i];
                    image = QImage((QSizeF(console.size()) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
                    image.setDevicePixelRatio(ratio);
                    image.fill(QColor::fromRgb(RowsParentColor));
                    QPainter painter(&image);
                    if (facts.m_styled)
                    {
                        QStyleOption option;
                        option.initFrom(&console);
                        option.state |= consoleStateFlags(state);
                        console.style()->drawPrimitive(QStyle::PE_Widget, &option, &painter, &console);
                    }
                    painter.end();
                }
            }

            for (int i = 0; i < 3; ++i)
            {
                const qreal ratio = AzQtComponents::ToolTipTwins::Ratios[i];
                QImage& image = facts.m_rendered[i];
                image = QImage((QSizeF(console.size()) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
                image.setDevicePixelRatio(ratio);
                image.fill(QColor::fromRgb(RowsParentColor));
                QPainter painter(&image);
                console.render(&painter, QPoint(), QRegion(), QWidget::DrawChildren);
                painter.end();
            }
            return facts;
        }

        struct ConsoleTreeFacts
        {
            ConsoleFacts m_output;
            ConsoleFacts m_console;
        };

        //! The fills the consoles of the two themes have (ScriptTermDialog.cpp, RefreshStyle)
        const QColor DarkConsoleColor(0x22, 0x22, 0x22);
        const QColor LightConsoleColor(Qt::white);

        // ------------------------------------------------------------------------------
        // The fixture: the header's, with the legacy twin taking the rules a test names
        // ------------------------------------------------------------------------------

        class AssetEditorBodyFlatSheetTests
            : public AssetEditorHeaderFlatSheetTests
        {
        protected:
            //! The legacy GUI: Qt's own style sheet style over Fusion, the application's rules on the root and the main window's rules on
            //! the window below it, the deeper sheet, as on the main window
            void toLegacyWith(QWidget& root, QWidget& window, const QString& applicationRules, const char* windowRules)
            {
                useStyle(root, m_fusion.get());
                root.setStyleSheet(applicationRules);
                window.setStyleSheet(QString::fromLatin1(windowRules));
                root.ensurePolished();
            }

            //! The tree shown off screen, as the panes of the Editor are: the layouts and the scroll areas place what they hold
            static void layOut(QWidget& root)
            {
                root.setAttribute(Qt::WA_DontShowOnScreen);
                root.resize(300, 300);
                root.show();
                QApplication::processEvents();
            }

            static QString rowApplicationRules()
            {
                return QString::fromLatin1(UniversalRule) + QString::fromLatin1(HeaderLabelRule);
            }

            RowTreeFacts legacyRows(const char* windowRules = RowRules)
            {
                RowTree tree;
                toLegacyWith(tree.m_root, *tree.m_window, rowApplicationRules(), windowRules);
                return takeTreeFacts(tree);
            }

            RowTreeFacts flatRows()
            {
                RowTree tree;
                toFlat(tree.m_root);
                return takeTreeFacts(tree);
            }

            ConsoleTreeFacts legacyConsoles(const QColor& color, const char* windowRules = ConsoleRules, bool verticalBar = false)
            {
                ConsoleTree tree(color, false, verticalBar);
                toLegacyWith(
                    tree.m_root, *tree.m_window, QString::fromLatin1(UniversalRule) + QString::fromLatin1(TextEditAppRules), windowRules);
                layOut(tree.m_root);
                return { takeConsoleFacts(*tree.m_output), takeConsoleFacts(*tree.m_console) };
            }

            ConsoleTreeFacts flatConsoles(const QColor& color, bool verticalBar = false)
            {
                ConsoleTree tree(color, true, verticalBar);
                toFlat(tree.m_root);
                layOut(tree.m_root);
                return { takeConsoleFacts(*tree.m_output), takeConsoleFacts(*tree.m_console) };
            }
        };
    } // namespace EditorFlatSheetTestsInternal

    using EditorFlatSheetTestsInternal::AssetEditorBodyFlatSheetTests;
    namespace BodyFlat = EditorFlatSheetTestsInternal;

    // ----------------------------------------------------------------------------------
    // The rows of the Asset Editor's property editor
    // ----------------------------------------------------------------------------------

    TEST_F(AssetEditorBodyFlatSheetTests, Registry_TheRowsBelowTheAssetEditorsEditorHaveTheElementOfTheRulesThatReachThem)
    {
        BodyFlat::RowTree tree;
        toFlat(tree.m_root);
        const BodyFlat::RowSet& rows = tree.m_inEditor;

        // The properties the selectors read, as the rows have them when they are polished
        EXPECT_FALSE(rows.m_rows[BodyFlat::UnnamedRoot]->IsTopLevel());
        EXPECT_TRUE(rows.m_rows[BodyFlat::Values]->IsTopLevel());
        EXPECT_EQ(rows.m_rows[BodyFlat::Values]->GetLevel(), 1);
        EXPECT_TRUE(rows.m_rows[BodyFlat::Values]->HasChildRows());
        EXPECT_FALSE(rows.m_rows[BodyFlat::ValuesChild]->IsTopLevel());
        EXPECT_TRUE(rows.m_rows[BodyFlat::Plain]->IsTopLevel());
        EXPECT_FALSE(rows.m_rows[BodyFlat::Plain]->HasChildRows());
        EXPECT_TRUE(rows.m_rows[BodyFlat::NamedRoot]->IsTopLevel());
        EXPECT_EQ(rows.m_rows[BodyFlat::NamedRoot]->GetLevel(), 0);
        EXPECT_FALSE(rows.m_rows[BodyFlat::Others]->IsTopLevel());
        EXPECT_EQ(rows.m_rows[BodyFlat::Others]->GetLevel(), 1);
        EXPECT_TRUE(rows.m_rows[BodyFlat::Others]->HasChildRows());

        // A row reached by both rules has the element of both; a top level row the property alone; a group the line alone
        const char* const expected[BodyFlat::RowCount] = {
            "",                                         // the unnamed root
            BodyFlat::TopLevelGroupRowElement,          // Values
            "",                                         // the child of Values
            BodyFlat::TopLevelRowElement,               // the plain row
            BodyFlat::TopLevelRowElement,               // the named root
            BodyFlat::GroupRowElement,                  // Others
            "",                                         // the child of Others
        };
        for (int row = 0; row < BodyFlat::RowCount; ++row)
        {
            EXPECT_STREQ(editorBoxOf(*rows.m_rows[row]), expected[row]) << BodyFlat::RowNames[row];
            // The same rows below a widget of another name are reached by no rule
            EXPECT_STREQ(editorBoxOf(*tree.m_outside.m_rows[row]), "") << BodyFlat::RowNames[row] << " elsewhere";
        }
    }

    TEST_F(AssetEditorBodyFlatSheetTests, Registry_TheRowsOfTheDocumentPropertyEditorHaveTheLineOnlyAtTheLevelTheRuleNamesWithAChild)
    {
        BodyFlat::RowTree tree;
        toFlat(tree.m_root);
        const BodyFlat::DpeSet& rows = tree.m_dpe;

        EXPECT_EQ(rows.m_rows[BodyFlat::DpeGroup]->GetLevel(), 2);
        EXPECT_TRUE(rows.m_rows[BodyFlat::DpeGroup]->HasChildRows());
        EXPECT_FALSE(rows.m_rows[BodyFlat::DpeLeaf]->HasChildRows());

        const char* const expected[BodyFlat::DpeCount] = { BodyFlat::GroupRowElement, "", "", "", "" };
        for (int row = 0; row < BodyFlat::DpeCount; ++row)
        {
            EXPECT_STREQ(editorBoxOf(*rows.m_rows[row]), expected[row]) << "row " << row;
        }
    }

    TEST_F(AssetEditorBodyFlatSheetTests, Properties_ATopLevelContainerRowMovesItsNumberOfElementsIntoItsNameAsTheSheetDoes)
    {
        const BodyFlat::RowTreeFacts legacy = legacyRows();
        const BodyFlat::RowTreeFacts flat = flatRows();

        // Legacy: the sheet sets the property of a top level row at its polish (QStyleSheetStyle::setProperties), and its setter puts the
        // number of elements of the container in the name and hides the label that showed it. The rule of a row that is not top level
        // sets 0, which the setter ignores.
        EXPECT_EQ(legacy.m_inEditor[BodyFlat::Values].m_label.toStdString(), "Values (2 elements)");
        EXPECT_TRUE(legacy.m_inEditor[BodyFlat::Values].m_defaultLabelHidden);
        EXPECT_EQ(legacy.m_inEditor[BodyFlat::Others].m_label.toStdString(), "Others");
        EXPECT_FALSE(legacy.m_inEditor[BodyFlat::Others].m_defaultLabelHidden);

        // The same container rows beside the editor are not reached
        EXPECT_EQ(legacy.m_outside[BodyFlat::Values].m_label.toStdString(), "Values");
        EXPECT_FALSE(legacy.m_outside[BodyFlat::Values].m_defaultLabelHidden);
        EXPECT_EQ(legacy.m_outside[BodyFlat::Others].m_label.toStdString(), "Others");

        for (int row = 0; row < BodyFlat::RowCount; ++row)
        {
            EXPECT_EQ(flat.m_inEditor[row].m_label.toStdString(), legacy.m_inEditor[row].m_label.toStdString())
                << BodyFlat::RowNames[row];
            EXPECT_EQ(flat.m_inEditor[row].m_defaultLabelHidden, legacy.m_inEditor[row].m_defaultLabelHidden) << BodyFlat::RowNames[row];
            EXPECT_EQ(flat.m_outside[row].m_label.toStdString(), legacy.m_outside[row].m_label.toStdString())
                << BodyFlat::RowNames[row] << " elsewhere";
            EXPECT_EQ(flat.m_outside[row].m_defaultLabelHidden, legacy.m_outside[row].m_defaultLabelHidden)
                << BodyFlat::RowNames[row] << " elsewhere";
        }
    }

    TEST_F(AssetEditorBodyFlatSheetTests, Properties_ARowIsReadAgainWhenItIsPolishedAgain)
    {
        // A row that is polished again (PropertyRowWidget::RefreshStyle unpolishes and polishes it) is matched again: a container row
        // below a row with a name is not top level, and is when that name is gone
        const auto polishedAfterNameIsGone = [this](bool flat)
        {
            BodyFlat::RowTree tree;
            if (flat)
            {
                toFlat(tree.m_root);
            }
            else
            {
                toLegacyWith(tree.m_root, *tree.m_window, rowApplicationRules(), BodyFlat::RowRules);
            }
            AzToolsFramework::PropertyRowWidget* others = tree.m_inEditor.m_rows[BodyFlat::Others];
            const QString before = others->label();

            tree.m_inEditor.m_rows[BodyFlat::NamedRoot]->SetNameLabel("");
            EXPECT_TRUE(others->IsTopLevel());
            others->style()->unpolish(others);
            others->style()->polish(others);
            return std::make_pair(before, others->label());
        };

        const auto legacy = polishedAfterNameIsGone(false);
        const auto flat = polishedAfterNameIsGone(true);
        EXPECT_EQ(legacy.first.toStdString(), "Others");
        EXPECT_EQ(legacy.second.toStdString(), "Others (3 elements)");
        EXPECT_EQ(flat.first.toStdString(), legacy.first.toStdString());
        EXPECT_EQ(flat.second.toStdString(), legacy.second.toStdString());
    }

    TEST_F(AssetEditorBodyFlatSheetTests, Box_TheGroupRowsHaveTheLineOfTheSheetAboveTheirContents)
    {
        namespace Twins = AzQtComponents::ToolTipTwins;
        const BodyFlat::RowTreeFacts legacy = legacyRows();
        const BodyFlat::RowTreeFacts flat = flatRows();

        // Legacy: the border of the rule is the row's own, so its contents sit 1px lower, in the rows of level 1 that have child rows
        // (a top level one too); no other row has a box
        const QRect whole(QPoint(0, 0), BodyFlat::RowBoxSize);
        const QRect lowered(0, 1, BodyFlat::RowBoxSize.width(), BodyFlat::RowBoxSize.height() - 1);
        for (int row = 0; row < BodyFlat::RowCount; ++row)
        {
            const bool group = row == BodyFlat::Values || row == BodyFlat::Others;
            EXPECT_EQ(legacy.m_inEditor[row].m_contents, group ? lowered : whole) << BodyFlat::RowNames[row];
            EXPECT_EQ(legacy.m_outside[row].m_contents, whole) << BodyFlat::RowNames[row] << " elsewhere";
        }

        for (int row = 0; row < BodyFlat::RowCount; ++row)
        {
            const bool group = row == BodyFlat::Values || row == BodyFlat::Others;
            EXPECT_EQ(flat.m_inEditor[row].m_contents, legacy.m_inEditor[row].m_contents) << BodyFlat::RowNames[row];
            EXPECT_EQ(Twins::marginsText(flat.m_inEditor[row].m_margins), Twins::marginsText(legacy.m_inEditor[row].m_margins))
                << BodyFlat::RowNames[row];
            EXPECT_EQ(flat.m_outside[row].m_contents, legacy.m_outside[row].m_contents) << BodyFlat::RowNames[row] << " elsewhere";

            // The row with the line has the styled background that draws it. (The universal rule gives every row of the legacy GUI
            // one, for a fill that is transparent: it draws nothing, and the rows the flattened GUI leaves without an element have none.)
            if (group)
            {
                EXPECT_TRUE(legacy.m_inEditor[row].m_styled) << BodyFlat::RowNames[row];
                EXPECT_TRUE(flat.m_inEditor[row].m_styled) << BodyFlat::RowNames[row];
            }

            // The line adds its pixel to the height a row asks for, in each GUI. The sizes of the labels and buttons of a container row
            // come from rules of the application's sheet that these twins do not carry (the Global elements of the rows), so the
            // heights of the two GUIs are not compared with each other: each is compared with the same row beside the editor.
            const int line = group ? 1 : 0;
            EXPECT_EQ(legacy.m_inEditor[row].m_hint.height(), legacy.m_outside[row].m_hint.height() + line) << BodyFlat::RowNames[row];
            EXPECT_EQ(flat.m_inEditor[row].m_hint.height(), flat.m_outside[row].m_hint.height() + line) << BodyFlat::RowNames[row];
        }
    }

    TEST_F(AssetEditorBodyFlatSheetTests, Pixels_TheGroupRowsPaintTheLineOfTheSheetAtEveryRatio)
    {
        namespace Twins = AzQtComponents::ToolTipTwins;
        const BodyFlat::RowTreeFacts legacy = legacyRows();
        const BodyFlat::RowTreeFacts flat = flatRows();

        // Legacy: a line of white at a fifth of its alpha over the parent's grey: 255 * 0.2 + 128 * 0.8 = 153 on the first row; the rest
        // of the row, a transparent fill, shows the parent
        const QImage& line = legacy.m_inEditor[BodyFlat::Values].m_images[0];
        ASSERT_FALSE(line.isNull());
        const QRgb lineColor = line.pixel(75, 0) & 0xFFFFFF;
        EXPECT_NE(lineColor, BodyFlat::RowsParentColor);
        EXPECT_EQ(lineColor & 0xFF, 153u);
        EXPECT_EQ(line.pixel(75, 1) & 0xFFFFFF, BodyFlat::RowsParentColor);
        EXPECT_EQ(line.pixel(0, 0) & 0xFFFFFF, lineColor);
        EXPECT_EQ(line.pixel(line.width() - 1, 0) & 0xFFFFFF, lineColor);
        EXPECT_EQ(legacy.m_inEditor[BodyFlat::Plain].m_images[0].pixel(75, 0) & 0xFFFFFF, BodyFlat::RowsParentColor);

        for (int row = 0; row < BodyFlat::RowCount; ++row)
        {
            for (int i = 0; i < 3; ++i)
            {
                EXPECT_EQ(Twins::differingPixels(legacy.m_inEditor[row].m_images[i], flat.m_inEditor[row].m_images[i]), 0)
                    << BodyFlat::RowNames[row] << " at ratio " << Twins::Ratios[i];
                EXPECT_EQ(Twins::differingPixels(legacy.m_outside[row].m_images[i], flat.m_outside[row].m_images[i]), 0)
                    << BodyFlat::RowNames[row] << " elsewhere at ratio " << Twins::Ratios[i];
            }
        }
    }

    TEST_F(AssetEditorBodyFlatSheetTests, Box_TheRowsOfTheDocumentPropertyEditorHaveTheSameLineAtTheirOwnLevel)
    {
        namespace Twins = AzQtComponents::ToolTipTwins;
        const BodyFlat::RowTreeFacts legacy = legacyRows();
        const BodyFlat::RowTreeFacts flat = flatRows();

        const QRect whole(QPoint(0, 0), BodyFlat::RowBoxSize);
        const QRect lowered(0, 1, BodyFlat::RowBoxSize.width(), BodyFlat::RowBoxSize.height() - 1);
        for (int row = 0; row < BodyFlat::DpeCount; ++row)
        {
            // Legacy: level 2 with a child row has the line (the document's rule); the rows a level above and below, and the one with
            // no child, have none
            EXPECT_EQ(legacy.m_dpe[row].m_contents, row == BodyFlat::DpeGroup ? lowered : whole) << "row " << row;
            EXPECT_EQ(flat.m_dpe[row].m_contents, legacy.m_dpe[row].m_contents) << "row " << row;
            EXPECT_EQ(Twins::marginsText(flat.m_dpe[row].m_margins), Twins::marginsText(legacy.m_dpe[row].m_margins)) << "row " << row;
            if (row == BodyFlat::DpeGroup)
            {
                EXPECT_TRUE(legacy.m_dpe[row].m_styled);
                EXPECT_TRUE(flat.m_dpe[row].m_styled);
            }
            for (int i = 0; i < 3; ++i)
            {
                EXPECT_EQ(Twins::differingPixels(legacy.m_dpe[row].m_images[i], flat.m_dpe[row].m_images[i]), 0)
                    << "row " << row << " at ratio " << Twins::Ratios[i];
            }
        }
        EXPECT_NE(legacy.m_dpe[BodyFlat::DpeGroup].m_images[0].pixel(75, 0) & 0xFFFFFF, BodyFlat::RowsParentColor);
    }

    // ----------------------------------------------------------------------------------
    // The Python Console
    // ----------------------------------------------------------------------------------

    TEST_F(AssetEditorBodyFlatSheetTests, Registry_TheOutputOfThePythonConsoleIsFoundByItsNameAndNoOtherTextEdit)
    {
        BodyFlat::ConsoleTree tree(BodyFlat::DarkConsoleColor, true);
        auto* other = new QPlainTextEdit(tree.m_pane);          // a text edit of another name
        auto* plainNamed = new QPlainTextEdit(tree.m_pane);     // a plain text edit of the name: reached by the rule's `#SCRIPT_OUTPUT`
        plainNamed->setObjectName(QStringLiteral("SCRIPT_OUTPUT"));
        toFlat(tree.m_root);

        EXPECT_STREQ(editorBoxOf(*tree.m_output), BodyFlat::ConsoleElement);
        EXPECT_STREQ(editorBoxOf(*plainNamed), BodyFlat::ConsoleElement);
        EXPECT_STREQ(editorBoxOf(*tree.m_console), "");
        EXPECT_STREQ(editorBoxOf(*other), "");
    }

    TEST_F(AssetEditorBodyFlatSheetTests, Console_TheTypeSelectorsOfTheRuleReachNoTextEdit_SoTheConsolesKeepTheApplicationsRules)
    {
        // The Editor's Console is a ConsoleTextEdit as well: the class AzToolsFramework::ConsoleTextEdit is not named `ConsoleTextEdit`
        // to a type selector (QStyleSheetStyleSelector::nodeNameEquals compares the whole name). The legacy console beside the output
        // looks the same with the rule on the window and without it.
        EXPECT_STREQ(AzToolsFramework::ConsoleTextEdit::staticMetaObject.className(), "AzToolsFramework::ConsoleTextEdit");

        namespace Twins = AzQtComponents::ToolTipTwins;
        const BodyFlat::ConsoleTreeFacts withRule = legacyConsoles(BodyFlat::DarkConsoleColor);
        const BodyFlat::ConsoleTreeFacts withoutRule = legacyConsoles(BodyFlat::DarkConsoleColor, "");

        for (int state = 0; state < BodyFlat::StateCount; ++state)
        {
            SCOPED_TRACE(BodyFlat::ConsoleStateNames[state]);
            EXPECT_EQ(withRule.m_console.m_contents[state], withoutRule.m_console.m_contents[state]);
            for (int i = 0; i < 3; ++i)
            {
                EXPECT_EQ(Twins::differingPixels(withRule.m_console.m_boxes[state][i], withoutRule.m_console.m_boxes[state][i]), 0)
                    << "at ratio " << Twins::Ratios[i];
            }
        }
        // ... while the rule does reach the output: its box is not the application's (a margin of 2px, 1px under the pointer)
        EXPECT_NE(withRule.m_output.m_contents[BodyFlat::Idle], withoutRule.m_output.m_contents[BodyFlat::Idle]);
        EXPECT_NE(withRule.m_output.m_contents[BodyFlat::Hovered], withoutRule.m_output.m_contents[BodyFlat::Hovered]);
    }

    TEST_F(AssetEditorBodyFlatSheetTests, Console_TheOutputHasTheSameBoxInEveryStateWithTheMarginAboveItAsTheSheetGaveIt)
    {
        for (const QColor& color : { BodyFlat::DarkConsoleColor, BodyFlat::LightConsoleColor })
        {
            SCOPED_TRACE(color.name().toStdString());
            const BodyFlat::ConsoleTreeFacts legacy = legacyConsoles(color);
            const BodyFlat::ConsoleTreeFacts flat = flatConsoles(color);

            // Legacy: margin 0 and margin-top 6px in the idle state, hovered and focused alike, over the application's 2px and 1px;
            // no line, so the field is the box less its margin, and so are the contents, the viewport and the scroll bars' room
            const QRect box(0, 6, BodyFlat::ConsoleBoxSize.width(), BodyFlat::ConsoleBoxSize.height() - 6);
            for (int state = 0; state < BodyFlat::StateCount; ++state)
            {
                EXPECT_EQ(legacy.m_output.m_contents[state], box) << BodyFlat::ConsoleStateNames[state];
                EXPECT_EQ(flat.m_output.m_contents[state], legacy.m_output.m_contents[state]) << BodyFlat::ConsoleStateNames[state];
            }
            EXPECT_EQ(legacy.m_output.m_viewport, box);
            EXPECT_EQ(flat.m_output.m_viewport, legacy.m_output.m_viewport);
            EXPECT_EQ(flat.m_output.m_widgetContents, legacy.m_output.m_widgetContents);
            EXPECT_EQ(flat.m_output.m_margins, legacy.m_output.m_margins);

            // The field is drawn by the style at PE_Widget, over no autofill of the area or of its viewport: the strip above the field
            // shows what is behind the console
            EXPECT_TRUE(legacy.m_output.m_styled);
            EXPECT_FALSE(legacy.m_output.m_autoFill);
            EXPECT_FALSE(legacy.m_output.m_viewportAutoFill);
            EXPECT_EQ(flat.m_output.m_styled, legacy.m_output.m_styled);
            EXPECT_EQ(flat.m_output.m_autoFill, legacy.m_output.m_autoFill);
            EXPECT_EQ(flat.m_output.m_viewportAutoFill, legacy.m_output.m_viewportAutoFill);
        }
    }

    TEST_F(AssetEditorBodyFlatSheetTests, Console_TheScrollBarOfTheOutputSitsInsideTheMarginAboveIt)
    {
        // The margin is the box of the area: the lane of the vertical bar starts below it and ends at the right edge of the field, and
        // so does the viewport. (The width of the bar and the room the flattened style keeps at its foot for the corner widget it gives
        // every scroll area are the bar's: the twins carry no rule of ScrollBar.qss and no corner widget in the legacy GUI.)
        const BodyFlat::ConsoleTreeFacts legacy = legacyConsoles(BodyFlat::DarkConsoleColor, BodyFlat::ConsoleRules, true);
        const BodyFlat::ConsoleTreeFacts flat = flatConsoles(BodyFlat::DarkConsoleColor, true);

        EXPECT_EQ(legacy.m_output.m_verticalBar.top(), 6);
        EXPECT_EQ(legacy.m_output.m_verticalBar.right(), BodyFlat::ConsoleBoxSize.width() - 1);
        EXPECT_EQ(legacy.m_output.m_viewport.top(), 6);
        EXPECT_EQ(legacy.m_output.m_viewport.bottom(), BodyFlat::ConsoleBoxSize.height() - 1);

        EXPECT_EQ(flat.m_output.m_verticalBar.top(), legacy.m_output.m_verticalBar.top());
        EXPECT_EQ(flat.m_output.m_verticalBar.right(), legacy.m_output.m_verticalBar.right());
        EXPECT_EQ(flat.m_output.m_viewport.top(), legacy.m_output.m_viewport.top());
        EXPECT_EQ(flat.m_output.m_viewport.bottom(), legacy.m_output.m_viewport.bottom());
    }

    TEST_F(AssetEditorBodyFlatSheetTests, Console_TheOutputPaintsWhatTheSheetPaintedInEveryStateAtEveryRatio)
    {
        namespace Twins = AzQtComponents::ToolTipTwins;
        for (const QColor& color : { BodyFlat::DarkConsoleColor, BodyFlat::LightConsoleColor })
        {
            SCOPED_TRACE(color.name().toStdString());
            const BodyFlat::ConsoleTreeFacts legacy = legacyConsoles(color);
            const BodyFlat::ConsoleTreeFacts flat = flatConsoles(color);

            // Legacy: the colour of the console from the inline sheet fills the field below the 6px strip in every state, with no ring;
            // the strip is the parent's
            const QImage& idle = legacy.m_output.m_boxes[BodyFlat::Idle][0];
            ASSERT_FALSE(idle.isNull());
            for (int state = 0; state < BodyFlat::StateCount; ++state)
            {
                const QImage& image = legacy.m_output.m_boxes[state][0];
                EXPECT_EQ(image.pixel(100, 3) & 0xFFFFFF, BodyFlat::RowsParentColor) << BodyFlat::ConsoleStateNames[state];
                EXPECT_EQ(image.pixel(100, 6) & 0xFFFFFF, color.rgb() & 0xFFFFFF) << BodyFlat::ConsoleStateNames[state];
                EXPECT_EQ(image.pixel(0, 6) & 0xFFFFFF, color.rgb() & 0xFFFFFF) << BodyFlat::ConsoleStateNames[state];
                EXPECT_EQ(image.pixel(image.width() - 1, image.height() - 1) & 0xFFFFFF, color.rgb() & 0xFFFFFF)
                    << BodyFlat::ConsoleStateNames[state];
            }

            for (int state = 0; state < BodyFlat::StateCount; ++state)
            {
                for (int i = 0; i < 3; ++i)
                {
                    EXPECT_EQ(Twins::differingPixels(legacy.m_output.m_boxes[state][i], flat.m_output.m_boxes[state][i]), 0)
                        << BodyFlat::ConsoleStateNames[state] << " at ratio " << Twins::Ratios[i];
                }
            }
            for (int i = 0; i < 3; ++i)
            {
                EXPECT_EQ(Twins::differingPixels(legacy.m_output.m_rendered[i], flat.m_output.m_rendered[i]), 0)
                    << "rendered at ratio " << Twins::Ratios[i];
            }
        }
    }

    // ----------------------------------------------------------------------------------
    // The bars of the UI Editor and of the Animation Editor (Editor.qss:145-148, :160-171): the line under the toolbars of the UI
    // Editor's window, and the line, the margins and the combo box of the Animation Editor's menu widget.
    //
    // The legacy twin is Qt's own style sheet style over Fusion: the rules of the application's sheet that reach the widgets on the
    // root, and Editor.qss's rules on the window below it (the main window's sheet, deeper than the application's). The classes of the
    // Editor's windows (MainWindow, EditorWindow) are not ones this test links: the flat twin's windows answer to their names
    // (ClassNamedMainWindow), and the legacy twin's sheet names them by their object names, as a type selector only matches a class the
    // meta object knows. The flat twin is the flattened style on every widget, with the elements of this sheet.
    // ----------------------------------------------------------------------------------

    namespace EditorFlatSheetTestsInternal
    {
        // ToolBar.qss:15-29, the rule of every toolbar (the comments left out)
        constexpr const char* ToolBarRule = R"sheet(
QToolBar
{
    background-color: #444444;
    qproperty-iconSize: 16px 16px;
    border: 0 none;
    padding: 5px;
}
)sheet";

        // ToolBar.qss:31-34 `MainWindow QToolBar { border-bottom: 2px solid #111111 }` and :36-39 `AzQtComponents--DockMainWindow QToolBar
        // { border: 0 none }`, for the stand-in of the main window: its object name for the class, and the id on the second rule too, so
        // that it is as specific as the first and later, as :36 is of :31
        constexpr const char* ToolBarWindowRules = R"sheet(
#mainWindow QToolBar { border-bottom: 2px solid #111111; }
#mainWindow AzQtComponents--DockMainWindow QToolBar { border: 0 none; }
)sheet";

        // The spacing of a toolbar's items is the style's (ToolBar::itemSpacing, ToolBarConfig.ini: 12) under the application's sheet; the
        // legacy twin has Fusion under its sheet, so the sheet states it
        constexpr const char* ToolBarSpacingRule = "QToolBar { spacing: 12px; }\n";

        // Editor.qss:145-148 `EditorWindow QToolBar { border-bottom: 2px solid #111111 }`, for the stand-in of the UI Editor's window
        constexpr const char* UiEditorToolBarRule = R"sheet(
#editorWindow QToolBar
{
    border-bottom: 2px solid #111111;
}
)sheet";

        // Editor.qss:160-171, whole: the rules of the Animation Editor's menu widget and of the combo box in it
        constexpr const char* AnimationEditorMenuRules = R"sheet(
#EMFX_Menu
{
    border-bottom-width: 2px;
    border-bottom-color: #111111;
    border-bottom-style: solid;
}

#EMFX_Menu QComboBox
{
    margin-right: 4px;
    margin-bottom: 2px;
}
)sheet";

        // MenuBar.qss:9-20, the rule of the menu bar (the items' rules play no part: the bar holds no menu)
        constexpr const char* MenuBarRule = R"sheet(
QMenuBar
{
    min-height: 28px;
    height: 28px;
    font-family: "Amazon Ember";
    font-size: 12px;
    margin: 0px;
    spacing: 0px;
    border-bottom-width: 2px;
    border-bottom-color: #111111;
    border-bottom-style: solid;
}
)sheet";

        // BaseStyleSheet.qss:68-84, the rule of the entry controls, and ComboBox.qss:13-75, the rules of the combo box (the popup's
        // rules play no part: no popup is shown)
        constexpr const char* ComboBoxRules = R"sheet(
QTextEdit,
QPlainText,
QPlainTextEdit,
QLineEdit,
QSpinBox,
QDoubleSpinBox,
QComboBox
{
    background-color: #CCCCCC;
    color: black;
    font-family: "Open Sans";
    border-radius: 2px;
    border-width: 0px;
    border-color: #e9e9e9;
    font-size: 12px;
    line-height: 16px;
}

QComboBox
{
    height: 16px;
    padding-left: 4px;
    margin: 1px;
    font-size: 12px;
    background-color: #CCCCCC;
}

QComboBox:hover
{
    margin: 0px;
    border-width: 1px;
    border-style: solid;
    border-radius: 3px;
    border-color: rgba(0, 0, 0, 50);
}

QComboBox:focus,
QComboBox[HasPopupOpen=true],
QComboBox[HasPopupOpen=true][HasPopupOpen=true]
{
    margin: 0px;
    border-width: 1px;
    border-style: solid;
    border-radius: 3px;
    border-color: #94D2FF;

    background-color: #FFFFFF;
}

QComboBox[HasError=true]
{
    margin: 0px;
    border-width: 2px;
    border-style: solid;
    border-radius: 4px;
    border-color: #E25243;
}

QComboBox:disabled
{
    background-color: #666666;
    color: #999999;
}

QComboBox::drop-down
{
    border-width: 0;
}

QComboBox::down-arrow
{
    width: 16px;
    height: 16px;
    margin-right: 4px;
    image: url(":/stylesheet/img/UI20/combobox-arrow.svg");
}

QComboBox::down-arrow:disabled
{
    image: url(":/stylesheet/img/UI20/combobox-arrow-disabled.svg");
}
)sheet";

        constexpr const char* UiEditorToolBarElement = "Editor.Box.UiEditorToolBar";
        constexpr const char* AnimationEditorMenuElement = "Editor.Box.AnimationEditorMenu";
        constexpr const char* AnimationEditorModeComboElement = "Editor.Box.AnimationEditorModeCombo";

        constexpr QRgb BarsParentColor = 0x204060;      // what neither the sheet nor an element draws stays this colour
        constexpr QRgb ToolBarFillColor = 0x444444;     // ToolBar.qss:20
        constexpr QRgb BarLineColor = 0x111111;         // Editor.qss:147, :163

        //! Whether anything was drawn on the image: it is not all of one colour
        bool isDrawnOn(const QImage& image)
        {
            for (int y = 0; y < image.height(); ++y)
            {
                for (int x = 0; x < image.width(); ++x)
                {
                    if (image.pixel(x, y) != image.pixel(0, 0))
                    {
                        return true;
                    }
                }
            }
            return false;
        }

        //! A QMainWindow that Qt takes for an instance of a class it is not (ClassNamedWidget, for a window)
        class ClassNamedMainWindow
            : public QMainWindow
        {
        public:
            ClassNamedMainWindow(const char* className, QWidget* parent = nullptr)
                : QMainWindow(parent)
                , m_className(className)
            {
            }

            void* qt_metacast(const char* name) override
            {
                return qstrcmp(name, m_className) == 0 ? this : QMainWindow::qt_metacast(name);
            }

        private:
            const char* m_className;
        };

        // ------------------------------------------------------------------------------
        // The toolbars: the Editor's main window, its DockMainWindow, and below it a window with a toolbar of its own, which holds a
        // window with another (as the UI Editor holds the animation window: both are reached by `EditorWindow QToolBar`)
        // ------------------------------------------------------------------------------

        constexpr int BarItemWidth = 18;
        constexpr int BarItemHeight = 19;               // + the 5px of padding above and below: the 29 of the Editor's main toolbar
        constexpr QPoint BarOffsets[2] = { QPoint(0, 0), QPoint(1, 1) };    // on a whole device pixel and on a half one at the ratio 1.5

        QToolBar* addBar(QMainWindow& window, const char* name)
        {
            QToolBar* bar = new QToolBar(&window);
            bar->setObjectName(QString::fromLatin1(name));
            bar->setMovable(false);
            for (int index = 0; index < 3; ++index)
            {
                QWidget* item = new QWidget(bar);
                item->setFixedSize(BarItemWidth, BarItemHeight);
                bar->addWidget(item);
            }
            window.addToolBar(Qt::TopToolBarArea, bar);
            return bar;
        }

        struct ToolBarTree
        {
            ToolBarTree(const char* windowClass, const char* windowName)
            {
                // A QMainWindow is a window (Qt::Window) until a layout takes it in: each is made without a parent and set as the
                // central widget of the one above, as the Editor's windows are
                QVBoxLayout* rootLayout = new QVBoxLayout(&m_root);
                rootLayout->setContentsMargins(0, 0, 0, 0);
                m_window = new ClassNamedMainWindow("MainWindow");
                m_window->setObjectName(QStringLiteral("mainWindow"));
                rootLayout->addWidget(m_window);
                m_dock = new AzQtComponents::DockMainWindow();
                m_window->setCentralWidget(m_dock);
                m_pane = new ClassNamedMainWindow(windowClass);
                m_pane->setObjectName(QString::fromLatin1(windowName));
                m_dock->setCentralWidget(m_pane);
                m_bar = addBar(*m_pane, "toolBar");

                // the window the pane holds
                m_inner = new QMainWindow();
                m_innerBar = addBar(*m_inner, "innerToolBar");
                m_inner->setCentralWidget(new QWidget());
                m_pane->setCentralWidget(m_inner);
            }

            QWidget m_root;
            ClassNamedMainWindow* m_window = nullptr;          //!< MainWindow, on which Editor.qss is set
            AzQtComponents::DockMainWindow* m_dock = nullptr;  //!< the DockMainWindow that hosts the view panes
            ClassNamedMainWindow* m_pane = nullptr;            //!< the view pane's window (EditorWindow, or another)
            QToolBar* m_bar = nullptr;
            QMainWindow* m_inner = nullptr;
            QToolBar* m_innerBar = nullptr;
        };

        //! What a toolbar is: its box, and its panel at the ratios and offsets
        struct BarFacts
        {
            QSize m_size;
            QSize m_hint;
            QMargins m_layoutMargins;
            QImage m_panels[3][2];
        };

        //! A toolbar's panel as QToolBar::paintEvent asks its style for it (CE_ToolBar): onto an image of the device ratio, which the
        //! style scales by, the toolbar at a logical offset. A toolbar has no styled background.
        QImage drawBarPanel(QToolBar& bar, qreal ratio, const QPoint& offset)
        {
            const QSize size = bar.size();
            const QSize logical = size + QSize(2 * offset.x() + 2, 2 * offset.y() + 2);
            QImage image((QSizeF(logical) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
            image.setDevicePixelRatio(ratio);
            image.fill(QColor::fromRgb(BarsParentColor));

            QPainter painter(&image);
            painter.translate(offset);
            painter.setClipRect(QRect(QPoint(0, 0), size));
            QStyleOptionToolBar option;
            option.initFrom(&bar);
            option.state = QStyle::State_Enabled | QStyle::State_Horizontal;
            option.rect = QRect(QPoint(0, 0), size);
            option.features = QStyleOptionToolBar::None;
            bar.style()->drawControl(QStyle::CE_ToolBar, &option, &painter, &bar);
            painter.end();
            return image;
        }

        BarFacts takeBarFacts(QToolBar& bar)
        {
            BarFacts facts;
            facts.m_size = bar.size();
            facts.m_hint = bar.sizeHint();
            facts.m_layoutMargins = bar.layout()->contentsMargins();
            for (int i = 0; i < 3; ++i)
            {
                for (int offset = 0; offset < 2; ++offset)
                {
                    facts.m_panels[i][offset] = drawBarPanel(bar, AzQtComponents::ToolTipTwins::Ratios[i], BarOffsets[offset]);
                }
            }
            return facts;
        }

        struct ToolBarTreeFacts
        {
            BarFacts m_bar;
            BarFacts m_innerBar;
        };

        // ------------------------------------------------------------------------------
        // The Animation Editor's menu widget (EMStudio MainWindow.cpp, :289-307): a plain widget with a layout of a menu bar and the
        // application mode combo box, the menu widget of the Animation Editor's window
        // ------------------------------------------------------------------------------

        enum ModeState
        {
            ModeIdle,
            ModeHover,
            ModeFocus,
            ModePopup,
            ModeStateCount,
        };
        constexpr const char* ModeStateNames[ModeStateCount] = { "idle", "hovered", "focused", "popup open" };

        QStyle::State modeStateFlags(int state)
        {
            switch (state)
            {
            case ModeHover:
                return QStyle::State_MouseOver;
            case ModeFocus:
                return QStyle::State_HasFocus;
            case ModePopup:
                return QStyle::State_On;        // what QComboBox::initStyleOption gives a combo box that shows its popup
            default:
                return QStyle::State_None;
            }
        }

        struct MenuTree
        {
            MenuTree()
            {
                // A QMainWindow is a window (Qt::Window) until a layout takes it in: each is made without a parent and set as the
                // central widget of the one above, as the Editor's windows are
                QVBoxLayout* rootLayout = new QVBoxLayout(&m_root);
                rootLayout->setContentsMargins(0, 0, 0, 0);
                m_window = new ClassNamedMainWindow("MainWindow");
                m_window->setObjectName(QStringLiteral("mainWindow"));
                rootLayout->addWidget(m_window);
                m_animationEditor = new QMainWindow();
                m_window->setCentralWidget(m_animationEditor);

                m_menu = new QWidget();
                m_menu->setObjectName(QStringLiteral("EMFX_Menu"));
                // the height of the window's default menu bar, a widget that was not laid out (a child widget's default size is 100 x 30)
                m_menu->setMinimumHeight(30);
                m_menu->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
                QHBoxLayout* layout = new QHBoxLayout(m_menu);
                layout->setContentsMargins(0, 0, 0, 0);
                layout->setSpacing(0);
                m_menuBar = new QMenuBar(m_menu);
                layout->addWidget(m_menuBar);
                m_mode = new QComboBox();
                m_mode->setObjectName(QStringLiteral("applicationMode"));
                m_mode->addItems({ QStringLiteral("AnimGraph"), QStringLiteral("Animation"), QStringLiteral("Character") });
                layout->addWidget(m_mode);
                m_animationEditor->setMenuWidget(m_menu);

                // a combo box of the window, outside the menu widget
                QWidget* page = new QWidget();
                QVBoxLayout* pageLayout = new QVBoxLayout(page);
                m_other = new QComboBox();
                m_other->setObjectName(QStringLiteral("other"));
                m_other->addItem(QStringLiteral("Other"));
                pageLayout->addWidget(m_other);
                m_animationEditor->setCentralWidget(page);
            }

            QWidget m_root;
            ClassNamedMainWindow* m_window = nullptr;       //!< MainWindow, on which Editor.qss is set
            QMainWindow* m_animationEditor = nullptr;       //!< the Animation Editor's window
            QWidget* m_menu = nullptr;                      //!< #EMFX_Menu
            QMenuBar* m_menuBar = nullptr;
            QComboBox* m_mode = nullptr;                    //!< the application mode combo box, in the menu widget
            QComboBox* m_other = nullptr;                   //!< a combo box of the window outside the menu widget
        };

        //! What the menu widget and its combo box are: the boxes, and the pixels at the ratios
        struct MenuFacts
        {
            QRect m_menu;                       //!< in the Animation Editor's window
            QRect m_menuBar;                    //!< in the menu widget
            QRect m_mode;                       //!< in the menu widget
            QSize m_modeHint;
            QRect m_frames[ModeStateCount];     //!< SC_ComboBoxFrame in the state, in the combo box
            QRect m_edits[ModeStateCount];      //!< SC_ComboBoxEditField
            QRect m_arrows[ModeStateCount];     //!< SC_ComboBoxArrow
            bool m_styled = false;              //!< the menu widget has a styled background
            QImage m_line[3];                   //!< the menu widget's own background and border (PE_Widget)
            QImage m_modes[ModeStateCount][3];  //!< the combo box in the state (CC_ComboBox and the label)
            QImage m_rendered[3];               //!< the menu widget rendered with its menu bar and combo box
        };

        QStyleOptionComboBox modeOption(const QComboBox& combo, QStyle::State extraState)
        {
            QStyleOptionComboBox option;
            option.initFrom(&combo);
            option.state |= extraState;
            option.rect = combo.rect();
            option.editable = combo.isEditable();
            option.frame = combo.hasFrame();
            option.currentText = combo.currentText();
            option.iconSize = combo.iconSize();
            option.subControls = QStyle::SC_All;
            option.activeSubControls = QStyle::SC_None;
            return option;
        }

        //! A combo box as QComboBox::paintEvent draws it, onto an image of the device ratio: the box and its arrow, then the label
        QImage drawMode(const QComboBox& combo, qreal ratio, QStyle::State extraState)
        {
            QImage image((QSizeF(combo.size()) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
            image.setDevicePixelRatio(ratio);
            image.fill(QColor::fromRgb(BarsParentColor));
            QPainter painter(&image);
            const QStyleOptionComboBox option = modeOption(combo, extraState);
            combo.style()->drawComplexControl(QStyle::CC_ComboBox, &option, &painter, &combo);
            combo.style()->drawControl(QStyle::CE_ComboBoxLabel, &option, &painter, &combo);
            painter.end();
            return image;
        }

        //! The facts of the menu in the tree as it was laid out. The popup state is the last: the legacy sheet reads [HasPopupOpen=true]
        //! from a property the combo box keeps while it shows its popup (ComboBoxWatcher), polishing the combo box again, with no layout
        //! after it, so the box keeps the size it had when the popup opened; the flattened style sees the state (QStyle::State_On)
        void takeMenuFacts(MenuTree& tree, MenuFacts& facts, bool legacy)
        {
            facts.m_menu = tree.m_menu->geometry();
            facts.m_menuBar = tree.m_menuBar->geometry();
            facts.m_mode = tree.m_mode->geometry();
            facts.m_modeHint = tree.m_mode->sizeHint();
            facts.m_styled = tree.m_menu->testAttribute(Qt::WA_StyledBackground);
            for (int i = 0; i < 3; ++i)
            {
                const qreal ratio = AzQtComponents::ToolTipTwins::Ratios[i];
                QImage& line = facts.m_line[i];
                line = QImage((QSizeF(tree.m_menu->size()) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
                line.setDevicePixelRatio(ratio);
                line.fill(QColor::fromRgb(BarsParentColor));
                {
                    QPainter painter(&line);
                    if (facts.m_styled)
                    {
                        QStyleOption option;
                        option.initFrom(tree.m_menu);
                        tree.m_menu->style()->drawPrimitive(QStyle::PE_Widget, &option, &painter, tree.m_menu);
                    }
                }

                QImage& rendered = facts.m_rendered[i];
                rendered = QImage((QSizeF(tree.m_menu->size()) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
                rendered.setDevicePixelRatio(ratio);
                rendered.fill(QColor::fromRgb(BarsParentColor));
                QPainter painter(&rendered);
                tree.m_menu->render(&painter, QPoint(), QRegion(), QWidget::DrawChildren);
            }

            QStyle* style = tree.m_mode->style();
            for (int state = 0; state < ModeStateCount; ++state)
            {
                if (state == ModePopup && legacy)
                {
                    tree.m_mode->setProperty(AzQtComponents::HasPopupOpen, true);
                    style->unpolish(tree.m_mode);
                    style->polish(tree.m_mode);
                }
                const QStyleOptionComboBox option = modeOption(*tree.m_mode, modeStateFlags(state));
                facts.m_frames[state] = style->subControlRect(QStyle::CC_ComboBox, &option, QStyle::SC_ComboBoxFrame, tree.m_mode);
                facts.m_edits[state] = style->subControlRect(QStyle::CC_ComboBox, &option, QStyle::SC_ComboBoxEditField, tree.m_mode);
                facts.m_arrows[state] = style->subControlRect(QStyle::CC_ComboBox, &option, QStyle::SC_ComboBoxArrow, tree.m_mode);
                for (int i = 0; i < 3; ++i)
                {
                    facts.m_modes[state][i] = drawMode(*tree.m_mode, AzQtComponents::ToolTipTwins::Ratios[i], modeStateFlags(state));
                }
            }
        }

        // ------------------------------------------------------------------------------
        // The fixture: the header's, with the legacy twin taking the rules of the bars
        // ------------------------------------------------------------------------------

        class EditorBarsFlatSheetTests
            : public AssetEditorHeaderFlatSheetTests
        {
        protected:
            //! The legacy GUI: the application's rules on the root, Editor.qss's on the window below it, the deeper sheet
            void toLegacyBars(QWidget& root, QWidget& window, const QString& applicationRules, const QString& windowRules)
            {
                useStyle(root, m_fusion.get());
                root.setStyleSheet(applicationRules);
                window.setStyleSheet(windowRules);
                root.ensurePolished();
            }

            static void layOutBars(QWidget& root)
            {
                root.setAttribute(Qt::WA_DontShowOnScreen);
                root.resize(400, 300);
                root.show();
                QApplication::processEvents();
            }

            static QString toolBarApplicationRules()
            {
                return QString::fromLatin1(UniversalRule) + QString::fromLatin1(ToolBarRule) + QString::fromLatin1(ToolBarSpacingRule)
                    + QString::fromLatin1(ToolBarWindowRules);
            }

            ToolBarTreeFacts legacyToolBars(const char* windowClass, const char* windowName, const char* editorRules = UiEditorToolBarRule)
            {
                ToolBarTree tree(windowClass, windowName);
                toLegacyBars(tree.m_root, *tree.m_window, toolBarApplicationRules(), QString::fromLatin1(editorRules));
                layOutBars(tree.m_root);
                return { takeBarFacts(*tree.m_bar), takeBarFacts(*tree.m_innerBar) };
            }

            ToolBarTreeFacts flatToolBars(const char* windowClass, const char* windowName)
            {
                ToolBarTree tree(windowClass, windowName);
                toFlat(tree.m_root);
                layOutBars(tree.m_root);
                return { takeBarFacts(*tree.m_bar), takeBarFacts(*tree.m_innerBar) };
            }

            static QString menuApplicationRules()
            {
                return QString::fromLatin1(UniversalRule) + QString::fromLatin1(MenuBarRule) + QString::fromLatin1(ComboBoxRules);
            }

            MenuFacts legacyMenu(const char* editorRules = AnimationEditorMenuRules)
            {
                MenuFacts facts;
                MenuTree tree;
                toLegacyBars(tree.m_root, *tree.m_window, menuApplicationRules(), QString::fromLatin1(editorRules));
                layOutBars(tree.m_root);
                takeMenuFacts(tree, facts, true);
                return facts;
            }

            MenuFacts flatMenu()
            {
                MenuFacts facts;
                MenuTree tree;
                toFlat(tree.m_root);
                layOutBars(tree.m_root);
                takeMenuFacts(tree, facts, false);
                return facts;
            }
        };
    } // namespace EditorFlatSheetTestsInternal

    using EditorFlatSheetTestsInternal::EditorBarsFlatSheetTests;
    namespace BarsFlat = EditorFlatSheetTestsInternal;

    // ----------------------------------------------------------------------------------
    // The UI Editor's toolbars (Editor.qss:145-148)
    // ----------------------------------------------------------------------------------

    TEST_F(EditorBarsFlatSheetTests, Registry_TheToolBarsBelowTheUiEditorsWindowHaveTheElementAtAnyDepth)
    {
        BarsFlat::ToolBarTree tree("EditorWindow", "editorWindow");
        toFlat(tree.m_root);

        // `EditorWindow QToolBar`: the toolbar of the window and the one of the window it holds
        EXPECT_STREQ(editorBoxOf(*tree.m_bar), BarsFlat::UiEditorToolBarElement);
        EXPECT_STREQ(editorBoxOf(*tree.m_innerBar), BarsFlat::UiEditorToolBarElement);

        // A type selector names the whole class: the window of another class, PhysX's among them, is not reached
        for (const char* otherClass : { "PhysX::Editor::EditorWindow", "OtherWindow" })
        {
            BarsFlat::ToolBarTree other(otherClass, "otherWindow");
            toFlat(other.m_root);
            EXPECT_STREQ(editorBoxOf(*other.m_bar), "") << otherClass;
            EXPECT_STREQ(editorBoxOf(*other.m_innerBar), "") << otherClass;
        }

        // and the window itself, and what is no toolbar, have none
        EXPECT_STREQ(editorBoxOf(*tree.m_pane), "");
        EXPECT_STREQ(editorBoxOf(*tree.m_dock), "");
    }

    TEST_F(EditorBarsFlatSheetTests, ToolBar_TheLineTakesNoRoom_TheBoxIsTheSheetsAndTheApplicationsToolBarsHaveNone)
    {
        const BarsFlat::ToolBarTreeFacts legacy = legacyToolBars("EditorWindow", "editorWindow");
        const BarsFlat::ToolBarTreeFacts flat = flatToolBars("EditorWindow", "editorWindow");

        // Legacy: the items keep the 5px of padding on every side (QStyleSheetStyle::pixelMetric, PM_ToolBarFrameWidth: the left border
        // and the left padding), and the line is painted over the padding below them: a toolbar is as high as without the rule
        const int height = BarsFlat::BarItemHeight + 2 * 5;
        for (const auto* bars : { &legacy, &flat })
        {
            for (const BarsFlat::BarFacts* bar : { &bars->m_bar, &bars->m_innerBar })
            {
                EXPECT_EQ(bar->m_size.height(), height);
                EXPECT_EQ(bar->m_hint.height(), height);
                EXPECT_EQ(bar->m_layoutMargins, QMargins(5, 5, 5, 5));
            }
        }
        EXPECT_EQ(flat.m_bar.m_size, legacy.m_bar.m_size);
        EXPECT_EQ(flat.m_bar.m_hint, legacy.m_bar.m_hint);
        EXPECT_EQ(flat.m_bar.m_layoutMargins, legacy.m_bar.m_layoutMargins);
        EXPECT_EQ(flat.m_innerBar.m_size, legacy.m_innerBar.m_size);
        EXPECT_EQ(flat.m_innerBar.m_hint, legacy.m_innerBar.m_hint);
        EXPECT_EQ(flat.m_innerBar.m_layoutMargins, legacy.m_innerBar.m_layoutMargins);

        // ... and in that height the last two rows are the line, in both GUIs
        for (const auto* bars : { &legacy, &flat })
        {
            for (const BarsFlat::BarFacts* bar : { &bars->m_bar, &bars->m_innerBar })
            {
                const QImage& image = bar->m_panels[0][0];
                const int x = image.width() - 20;
                EXPECT_EQ(image.pixel(x, height - 3) & 0xFFFFFF, BarsFlat::ToolBarFillColor);
                EXPECT_EQ(image.pixel(x, height - 2) & 0xFFFFFF, BarsFlat::BarLineColor);
                EXPECT_EQ(image.pixel(x, height - 1) & 0xFFFFFF, BarsFlat::BarLineColor);
            }
        }

        // The toolbars of the window of another class keep the box of the DockMainWindow's rule: the same height, no line
        const BarsFlat::ToolBarTreeFacts legacyOther = legacyToolBars("OtherWindow", "otherWindow");
        const BarsFlat::ToolBarTreeFacts flatOther = flatToolBars("OtherWindow", "otherWindow");
        EXPECT_EQ(legacyOther.m_bar.m_size.height(), height);
        EXPECT_EQ(flatOther.m_bar.m_size, legacyOther.m_bar.m_size);
        EXPECT_EQ(flatOther.m_innerBar.m_size, legacyOther.m_innerBar.m_size);
    }

    TEST_F(EditorBarsFlatSheetTests, ToolBar_PaintsTheFillAndTheLineAlongItsBottomEdgeLikeTheSheetAtEveryRatio)
    {
        namespace Twins = AzQtComponents::ToolTipTwins;
        const BarsFlat::ToolBarTreeFacts legacy = legacyToolBars("EditorWindow", "editorWindow");
        const BarsFlat::ToolBarTreeFacts flat = flatToolBars("EditorWindow", "editorWindow");

        // Legacy, at the ratio 1: the two last rows of the toolbar are the line, over the fill of the rows above them
        {
            const QImage& image = legacy.m_bar.m_panels[0][0];
            const int x = image.width() - 20;                       // right of the items, where only the panel is
            const int last = legacy.m_bar.m_size.height() - 1;
            EXPECT_EQ(image.pixel(x, 10) & 0xFFFFFF, BarsFlat::ToolBarFillColor);
            EXPECT_EQ(image.pixel(x, last - 2) & 0xFFFFFF, BarsFlat::ToolBarFillColor);
            EXPECT_EQ(image.pixel(x, last - 1) & 0xFFFFFF, BarsFlat::BarLineColor);
            EXPECT_EQ(image.pixel(x, last) & 0xFFFFFF, BarsFlat::BarLineColor);
        }

        for (int i = 0; i < 3; ++i)
        {
            for (int offset = 0; offset < 2; ++offset)
            {
                EXPECT_EQ(Twins::differingPixels(legacy.m_bar.m_panels[i][offset], flat.m_bar.m_panels[i][offset]), 0)
                    << "toolbar of the window at ratio " << Twins::Ratios[i] << ", offset " << BarsFlat::BarOffsets[offset].x();
                EXPECT_EQ(Twins::differingPixels(legacy.m_innerBar.m_panels[i][offset], flat.m_innerBar.m_panels[i][offset]), 0)
                    << "toolbar of the window it holds at ratio " << Twins::Ratios[i] << ", offset " << BarsFlat::BarOffsets[offset].x();
            }
        }
    }

    TEST_F(EditorBarsFlatSheetTests, ToolBar_OfAnotherWindowHasNoLineInEitherGui)
    {
        namespace Twins = AzQtComponents::ToolTipTwins;
        for (const char* otherClass : { "PhysX::Editor::EditorWindow", "OtherWindow" })
        {
            // the legacy sheet names the window by its object name, which no other window has
            const BarsFlat::ToolBarTreeFacts legacy = legacyToolBars(otherClass, "otherWindow");
            const BarsFlat::ToolBarTreeFacts flat = flatToolBars(otherClass, "otherWindow");

            const QImage& image = legacy.m_bar.m_panels[0][0];
            const int x = image.width() - 20;
            const int last = legacy.m_bar.m_size.height() - 1;
            EXPECT_EQ(image.pixel(x, last) & 0xFFFFFF, BarsFlat::ToolBarFillColor) << otherClass;
            EXPECT_EQ(image.pixel(x, last - 1) & 0xFFFFFF, BarsFlat::ToolBarFillColor) << otherClass;
            for (int i = 0; i < 3; ++i)
            {
                for (int offset = 0; offset < 2; ++offset)
                {
                    EXPECT_EQ(Twins::differingPixels(legacy.m_bar.m_panels[i][offset], flat.m_bar.m_panels[i][offset]), 0)
                        << otherClass << " at ratio " << Twins::Ratios[i] << ", offset " << BarsFlat::BarOffsets[offset].x();
                }
            }

            // while the toolbar of the UI Editor's window, in the same GUI, has the line the other has not
            const BarsFlat::ToolBarTreeFacts flatUiEditor = flatToolBars("EditorWindow", "editorWindow");
            EXPECT_TRUE(flatUiEditor.m_bar.m_panels[0][0] != flat.m_bar.m_panels[0][0]) << otherClass;
        }
    }

    // ----------------------------------------------------------------------------------
    // The Animation Editor's menu widget and its combo box (Editor.qss:160-171)
    // ----------------------------------------------------------------------------------

    TEST_F(EditorBarsFlatSheetTests, Registry_TheMenuWidgetAndTheComboBoxInItHaveTheirElementsAndNoOtherWidgetHasThem)
    {
        BarsFlat::MenuTree tree;
        toFlat(tree.m_root);

        EXPECT_STREQ(editorBoxOf(*tree.m_menu), BarsFlat::AnimationEditorMenuElement);
        EXPECT_STREQ(editorBoxOf(*tree.m_mode), BarsFlat::AnimationEditorModeComboElement);
        EXPECT_STREQ(editorBoxOf(*tree.m_other), "");               // a combo box of the window, outside the menu widget
        EXPECT_STREQ(editorBoxOf(*tree.m_menuBar), "");
        EXPECT_STREQ(editorBoxOf(*tree.m_animationEditor), "");

        // `#EMFX_Menu` names no type, and a widget of another name is not reached
        QWidget root;
        QWidget* sameName = new QWidget(&root);
        sameName->setObjectName(QStringLiteral("EMFX_Menu"));
        QWidget* otherName = new QWidget(&root);
        otherName->setObjectName(QStringLiteral("EMFX_Menu2"));
        QComboBox* inSameName = new QComboBox(sameName);
        QComboBox* inOtherName = new QComboBox(otherName);
        for (QWidget* widget : { sameName, otherName, static_cast<QWidget*>(inSameName), static_cast<QWidget*>(inOtherName) })
        {
            widget->setStyle(m_style.get());
            AzQtComponents::BoxElement::resolve(widget);
        }
        EXPECT_STREQ(editorBoxOf(*sameName), BarsFlat::AnimationEditorMenuElement);
        EXPECT_STREQ(editorBoxOf(*otherName), "");
        EXPECT_STREQ(editorBoxOf(*inSameName), BarsFlat::AnimationEditorModeComboElement);
        EXPECT_STREQ(editorBoxOf(*inOtherName), "");

        // The plain widget takes the line through its styled background
        EXPECT_TRUE(tree.m_menu->testAttribute(Qt::WA_StyledBackground));
    }

    TEST_F(EditorBarsFlatSheetTests, Menu_TheLineTakesNoRoom_AndTheComboBoxHasTheMarginsOfTheSheet)
    {
        const BarsFlat::MenuFacts legacy = legacyMenu();
        const BarsFlat::MenuFacts flat = flatMenu();

        // Legacy: the menu widget is the 30px of its minimum height (a menu bar of 28px and its 2px line is as high), the line is painted
        // over its bottom edge and takes no room. The combo box is its 16px and the margins around it: 1px above, 2px below, 4px at
        // the right, in every state (hovered or focused, the sheet takes the margins of ComboBox.qss away on the left and above only)
        EXPECT_EQ(legacy.m_menu.height(), 30);
        EXPECT_EQ(legacy.m_modeHint.height(), 1 + 16 + 2);
        const int width = legacy.m_mode.width();
        const int height = legacy.m_mode.height();
        EXPECT_EQ(height, 1 + 16 + 2);
        EXPECT_EQ(legacy.m_frames[BarsFlat::ModeIdle], QRect(1, 1, width - 1 - 4, 16));
        EXPECT_EQ(legacy.m_frames[BarsFlat::ModeHover], QRect(0, 0, width - 4, height - 2));
        EXPECT_EQ(legacy.m_frames[BarsFlat::ModeFocus], legacy.m_frames[BarsFlat::ModeHover]);
        EXPECT_EQ(legacy.m_frames[BarsFlat::ModePopup], legacy.m_frames[BarsFlat::ModeHover]);

        EXPECT_EQ(flat.m_menu, legacy.m_menu);
        EXPECT_EQ(flat.m_menuBar.height(), legacy.m_menuBar.height());
        EXPECT_EQ(flat.m_mode, legacy.m_mode);
        EXPECT_EQ(flat.m_modeHint, legacy.m_modeHint);
        EXPECT_EQ(flat.m_styled, legacy.m_styled);
        for (int state = 0; state < BarsFlat::ModeStateCount; ++state)
        {
            EXPECT_EQ(flat.m_frames[state], legacy.m_frames[state]) << BarsFlat::ModeStateNames[state];
            EXPECT_EQ(flat.m_edits[state], legacy.m_edits[state]) << BarsFlat::ModeStateNames[state];
            EXPECT_EQ(flat.m_arrows[state], legacy.m_arrows[state]) << BarsFlat::ModeStateNames[state];
        }
    }

    TEST_F(EditorBarsFlatSheetTests, Menu_PaintsTheLineAlongItsBottomEdgeAndTheComboBoxAboveItLikeTheSheetAtEveryRatio)
    {
        namespace Twins = AzQtComponents::ToolTipTwins;
        const BarsFlat::MenuFacts legacy = legacyMenu();
        const BarsFlat::MenuFacts flat = flatMenu();

        // Legacy, at the ratio 1: the two last rows of the menu widget are the line; the rows above are the window's (nothing is drawn)
        {
            const QImage& image = legacy.m_line[0];
            ASSERT_FALSE(image.isNull());
            const int x = 5;
            EXPECT_EQ(image.pixel(x, 10) & 0xFFFFFF, BarsFlat::BarsParentColor);
            EXPECT_EQ(image.pixel(x, image.height() - 3) & 0xFFFFFF, BarsFlat::BarsParentColor);
            EXPECT_EQ(image.pixel(x, image.height() - 2) & 0xFFFFFF, BarsFlat::BarLineColor);
            EXPECT_EQ(image.pixel(x, image.height() - 1) & 0xFFFFFF, BarsFlat::BarLineColor);
            EXPECT_EQ(image.pixel(image.width() - 1, image.height() - 1) & 0xFFFFFF, BarsFlat::BarLineColor);
        }

        for (int i = 0; i < 3; ++i)
        {
            EXPECT_EQ(Twins::differingPixels(legacy.m_line[i], flat.m_line[i]), 0) << "line at ratio " << Twins::Ratios[i];
            EXPECT_EQ(Twins::differingPixels(legacy.m_rendered[i], flat.m_rendered[i]), 0) << "menu rendered at ratio " << Twins::Ratios[i];
            for (int state = 0; state < BarsFlat::ModeStateCount; ++state)
            {
                EXPECT_EQ(Twins::differingPixels(legacy.m_modes[state][i], flat.m_modes[state][i]), 0)
                    << "combo box " << BarsFlat::ModeStateNames[state] << " at ratio " << Twins::Ratios[i];
            }
        }
    }

    TEST_F(EditorBarsFlatSheetTests, Menu_TheRulesAreWhatGiveTheLineAndTheMargins_ThePlainBoxIsNotTheFlatOne)
    {
        // The legacy twin without the rules of Editor.qss has the box of ComboBox.qss alone (a margin of 1px on every side) and no line:
        // the flat twin is not that box. Both are judged against it, and the flat twin differs from it where the rules do
        const BarsFlat::MenuFacts withRules = legacyMenu();
        const BarsFlat::MenuFacts withoutRules = legacyMenu("");
        const BarsFlat::MenuFacts flat = flatMenu();

        EXPECT_EQ(withoutRules.m_modeHint.height(), 1 + 16 + 1);
        EXPECT_NE(withRules.m_modeHint, withoutRules.m_modeHint);
        EXPECT_NE(flat.m_modeHint, withoutRules.m_modeHint);
        EXPECT_NE(flat.m_mode.size(), withoutRules.m_mode.size());
        EXPECT_FALSE(BarsFlat::isDrawnOn(withoutRules.m_line[0]));
        EXPECT_TRUE(BarsFlat::isDrawnOn(withRules.m_line[0]));
        EXPECT_TRUE(BarsFlat::isDrawnOn(flat.m_line[0]));
    }
} // namespace UnitTest
