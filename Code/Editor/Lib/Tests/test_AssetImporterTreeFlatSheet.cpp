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

#include <AzQtComponents/Components/Elements/ItemViewElement.h>
#include <AzQtComponents/Components/GuiPerfFloor.h>
#include <AzQtComponents/Components/Style.h>
#include <AssetImporter/AssetImporterFlatSheet.h>

#include <QApplication>
#include <QColor>
#include <QDialog>
#include <QFile>
#include <QFont>
#include <QFontMetrics>
#include <QImage>
#include <QListView>
#include <QMargins>
#include <QPainter>
#include <QStyleFactory>
#include <QStyleOptionViewItem>
#include <QTreeView>
#include <QWidget>

#include <string>

namespace UnitTest
{
    // ----------------------------------------------------------------------------------
    // The element of the sheet the asset importer's window sets on itself (AssetImporterWindow.qss): which trees it reaches, and that it
    // sizes, places and paints the check boxes of their items as Qt's own style sheet style did, in each state of an item and at the
    // device ratios the GUI runs at. The legacy twin is a tree under the sheet of the window (on a widget the root's sheet is not on: a
    // sheet's weight is the depth of its widget before it is the specificity of a rule, so the window's outranks the application's, as
    // it does in the Editor) and the rules of the application's sheet that reach the check boxes of the items, quoted with their
    // lines. The flat twin is the same tree under the style, with the element the window registers.
    // ----------------------------------------------------------------------------------

    namespace AssetImporterTreeFlatSheetTestsInternal
    {
        //! A widget that Qt takes for an instance of a class it is not. The matcher asks for the window by its class name, as the selector
        //! of a sheet does, and building the real one needs the Editor's whole scene settings system.
        class ImporterClassWidget
            : public QWidget
        {
        public:
            ImporterClassWidget(const char* className, QWidget* parent = nullptr)
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

        constexpr const char* WindowClass = "AssetImporterWindow";
        constexpr const char* ElementName = "Editor.ItemView.ImporterTree";

        // The sheet of the window: AssetImporterWindow.qss:6-40. Its first rule (:1-4, QPushButton#m_deleteButton) reaches no widget: the
        // importer's only m_deleteButton is a QToolButton (HeaderWidget.ui:86). The three PNGs it names are in no .qrc of the engine.
        constexpr const char* WindowSheet =
            "QTreeView::indicator {"                                                // :6
            "    width: 12px;"                                                      // :8
            "    height: 12px;"                                                     // :9
            "    border: 1px solid rgb(204, 204, 204);"                             // :10
            "    border-radius: 2px;"                                               // :11
            "}"
            "QTreeView::indicator:checked {"                                        // :14
            "    background-color: rgb(66, 133, 244);"                              // :16
            "    image: url(:Icons/CheckMark_Checked.png);"                         // :17
            "    border-radius: 2px;"                                               // :18
            "    border: 0px;"                                                      // :19
            "}"
            "QTreeView::indicator:hover {"                                          // :22
            "    border: 1px solid white;"                                          // :24
            "}"
            "QTreeView::indicator:checked:hover {"                                  // :27
            "    image: url(:Icons/CheckMark_Hover.png);"                           // :29
            "}"
            "QTreeView::indicator:!checked:hover {"                                 // :32
            "    image: url(:Icons/CheckMark_Unchecked_Hover.png);"                 // :34
            "}"
            "QTreeView::indicator:!enabled {"                                       // :37
            "    border: 1px solid rgb(102, 102, 102);"                             // :39
            "}";

        // The rules of the application's sheet that reach the check box of an item of a tree (the selectors of their lists that name other
        // classes are left out), in the order the sheet has them: CheckBox.qss is imported before TableView.qss
        constexpr const char* ApplicationRules =
            // CheckBox.qss:23-28
            "QAbstractItemView::indicator { width: 16px; height: 16px; margin: 2px; }"
            // :30-33
            "QAbstractItemView::indicator:indeterminate { image: url(:/stylesheet/img/UI20/checkbox/partial-selected.svg); }"
            // :35-38
            "QAbstractItemView::indicator:indeterminate:focus"
            " { image: url(:/stylesheet/img/UI20/checkbox/partial-selected-focus.svg); }"
            // :40-43
            "QAbstractItemView::indicator:indeterminate:disabled"
            " { image: url(:/stylesheet/img/UI20/checkbox/partial-selected-disabled.svg); }"
            // :45-48
            "QAbstractItemView::indicator:checked { image: url(:/stylesheet/img/UI20/checkbox/on.svg); }"
            // :50-53
            "QAbstractItemView::indicator:checked:focus { image: url(:/stylesheet/img/UI20/checkbox/on-focus.svg); }"
            // :55-58
            "QAbstractItemView::indicator:checked:disabled { image: url(:/stylesheet/img/UI20/checkbox/on-disabled.svg); }"
            // :60-63
            "QAbstractItemView::indicator:unchecked { image: url(:/stylesheet/img/UI20/checkbox/off.svg); }"
            // :65-68
            "QAbstractItemView::indicator:unchecked:focus { image: url(:/stylesheet/img/UI20/checkbox/off-focus.svg); }"
            // :70-73
            "QAbstractItemView::indicator:unchecked:disabled { image: url(:/stylesheet/img/UI20/checkbox/off-disabled.svg); }"
            // TableView.qss:31-40
            "QTreeView::item { height: 24px; padding-left: 4px; border: none; }"
            // TableView.qss:79-85
            "QTreeView::indicator { margin: 0px; width: 16px; height: 16px; margin-left: -3px; }";

        //! One state of an item's check box: its option's state flags and check state
        struct CheckCase
        {
            const char* m_name;
            QStyle::State m_state;
            Qt::CheckState m_check;
            bool m_hovered;
            bool m_disabled;
        };

        // The state QStyledItemDelegate::initStyleOption gives an item (State_On, State_NoChange or State_Off for its check state)
        const QStyle::State Enabled = QStyle::State_Enabled | QStyle::State_Active;
        const QStyle::State Disabled = QStyle::State_Active;
        const CheckCase CheckCases[] = {
            { "unchecked", Enabled | QStyle::State_Off, Qt::Unchecked, false, false },
            { "checked", Enabled | QStyle::State_On, Qt::Checked, false, false },
            { "indeterminate", Enabled | QStyle::State_NoChange, Qt::PartiallyChecked, false, false },
            { "unchecked and hovered", Enabled | QStyle::State_Off | QStyle::State_MouseOver, Qt::Unchecked, true, false },
            { "checked and hovered", Enabled | QStyle::State_On | QStyle::State_MouseOver, Qt::Checked, true, false },
            { "indeterminate and hovered", Enabled | QStyle::State_NoChange | QStyle::State_MouseOver, Qt::PartiallyChecked, true, false },
            { "unchecked and disabled", Disabled | QStyle::State_Off, Qt::Unchecked, false, true },
            { "checked and disabled", Disabled | QStyle::State_On, Qt::Checked, false, true },
            { "indeterminate and disabled", Disabled | QStyle::State_NoChange, Qt::PartiallyChecked, false, true },
        };

        //! The option of an item with a text and a check box, as the delegate makes it; the font is one for both twins (the flat style's
        //! text element is not what is proved here)
        QStyleOptionViewItem itemOption(const QWidget* view, const CheckCase& test)
        {
            const QFont font;
            QStyleOptionViewItem option;
            option.initFrom(view);
            option.widget = view;
            option.styleObject = const_cast<QWidget*>(view);
            option.state = test.m_state;
            option.rect = QRect(7, 8, 140, 24);
            option.font = font;
            option.fontMetrics = QFontMetrics(font);
            option.decorationSize = QSize(16, 16);
            option.text = QStringLiteral("Apple");
            option.features = QStyleOptionViewItem::HasDisplay | QStyleOptionViewItem::HasCheckIndicator;
            option.checkState = test.m_check;
            option.palette.setCurrentColorGroup(test.m_disabled ? QPalette::Disabled : QPalette::Active);
            return option;
        }

        //! What a style paints for the check box of an item, in the slot the item gives it, onto an image of an explicit device ratio (the
        //! painter is the image's, so the style scales what it draws by that ratio)
        QImage paintCheck(const QWidget* view, const CheckCase& test, qreal ratio)
        {
            QImage image((QSizeF(160, 40) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
            image.setDevicePixelRatio(ratio);
            image.fill(QColor(0x80, 0x80, 0x80));
            QPainter painter(&image);
            const QStyleOptionViewItem item = itemOption(view, test);
            QStyleOptionViewItem box(item);
            box.rect = view->style()->subElementRect(QStyle::SE_ItemViewItemCheckIndicator, &item, view);
            view->style()->drawPrimitive(QStyle::PE_IndicatorItemViewItemCheck, &box, &painter, view);
            painter.end();
            return image;
        }

        int countOf(const QImage& image, const QColor& color)
        {
            int count = 0;
            for (int y = 0; y < image.height(); ++y)
            {
                for (int x = 0; x < image.width(); ++x)
                {
                    count += (QColor::fromRgba(image.pixel(x, y)) == color) ? 1 : 0;
                }
            }
            return count;
        }

        //! The pixels of a rect of an image (at ratio 1) that are not the backdrop paintCheck fills the image with
        int countNotBackdrop(const QImage& image, const QRect& rect)
        {
            int count = 0;
            for (int y = rect.top(); y <= rect.bottom(); ++y)
            {
                for (int x = rect.left(); x <= rect.right(); ++x)
                {
                    count += (QColor::fromRgba(image.pixel(x, y)) != QColor(0x80, 0x80, 0x80)) ? 1 : 0;
                }
            }
            return count;
        }

        //! The first pixel two images of one size differ at, as text for a failure message ("" when they are equal)
        std::string firstDifference(const QImage& legacy, const QImage& flat)
        {
            if (legacy.size() != flat.size())
            {
                return "the sizes differ";
            }
            for (int y = 0; y < legacy.height(); ++y)
            {
                for (int x = 0; x < legacy.width(); ++x)
                {
                    if (legacy.pixel(x, y) != flat.pixel(x, y))
                    {
                        return "at (" + std::to_string(x) + ", " + std::to_string(y) + "): legacy #"
                            + QString::number(legacy.pixel(x, y) & 0xFFFFFFu, 16).toStdString() + ", flat #"
                            + QString::number(flat.pixel(x, y) & 0xFFFFFFu, 16).toStdString();
                    }
                }
            }
            return "";
        }

        //! A tree below a window of the importer, in a root of its own: legacy under the sheets, or flat under the style
        struct TreeTwin
        {
            TreeTwin(QStyle* base, bool legacy)
                : m_window(new ImporterClassWidget(WindowClass, &m_root))
                , m_tree(new QTreeView(m_window))
            {
                m_root.setStyle(base);
                m_window->setStyle(base);
                m_tree->setStyle(base);
                if (legacy)
                {
                    m_window->setStyleSheet(QString::fromLatin1(WindowSheet));
                    m_root.setStyleSheet(QString::fromLatin1(ApplicationRules));
                    m_root.ensurePolished();
                    m_window->ensurePolished();
                    m_tree->ensurePolished();
                }
                else
                {
                    AzQtComponents::ItemViewElement::resolve(m_tree);
                }
            }

            QWidget m_root;
            ImporterClassWidget* m_window;
            QTreeView* m_tree;
        };

        class AssetImporterTreeFlatSheetTests
            : public LeakDetectionFixture
        {
        protected:
            void SetUp() override
            {
                LeakDetectionFixture::SetUp();

                // The test application carries StyleManager's legacy sheets: the twins name their own
                m_applicationSheet = qApp->styleSheet();
                qApp->setStyleSheet(QString());

                // The flattened GUI's branches of Style read the rung
                AzQtComponents::GuiPerfFloor::setRungForTesting(AzQtComponents::GuiPerfFloor::Rung::Flat);

                m_style = AZStd::make_unique<AzQtComponents::Style>(QStyleFactory::create(QStringLiteral("Fusion")));
                m_fusion = AZStd::unique_ptr<QStyle>(QStyleFactory::create(QStringLiteral("Fusion")));

                // Once for the process, as the window registers the element once
                static const bool registered = (AssetImporterFlatSheet::RegisterElements(), true);
                ASSERT_TRUE(registered);
            }

            void TearDown() override
            {
                m_fusion.reset();
                m_style.reset();
                AzQtComponents::GuiPerfFloor::resetForTesting();
                qApp->setStyleSheet(m_applicationSheet);
                LeakDetectionFixture::TearDown();
            }

            static const AzQtComponents::ItemViewElement* elementOf(const QWidget* tree)
            {
                return AzQtComponents::ItemViewElement::forWidget(tree);
            }

            //! The flat way: the style on the widget, and the element resolved as Style::polish does in the flattened GUI
            void flatten(QAbstractItemView& view)
            {
                view.setStyle(m_style.get());
                AzQtComponents::ItemViewElement::resolve(&view);
            }

            QString m_applicationSheet;
            AZStd::unique_ptr<AzQtComponents::Style> m_style;
            AZStd::unique_ptr<QStyle> m_fusion;
        };
    } // namespace AssetImporterTreeFlatSheetTestsInternal

    // A unity build joins this file with others of namespace UnitTest: the fixture is the only name brought in, the rest is named
    // through the alias
    using AssetImporterTreeFlatSheetTestsInternal::AssetImporterTreeFlatSheetTests;
    namespace TreeFlat = AssetImporterTreeFlatSheetTestsInternal;

    // ----------------------------------------------------------------------------------
    // Resolution
    // ----------------------------------------------------------------------------------

    TEST_F(AssetImporterTreeFlatSheetTests, Registry_EveryTreeBelowTheWindowHasTheElementAndNoWidgetOutsideItDoes)
    {
        TreeFlat::ImporterClassWidget window(TreeFlat::WindowClass);
        QTreeView* tree = new QTreeView(&window);
        QWidget* wrapper = new QWidget(&window);
        QTreeView* deepTree = new QTreeView(wrapper);                     // `QTreeView::indicator` names no parent: a tree may sit deeper
        QDialog* dialog = new QDialog(&window);                           // a child dialog is below the window the sheet is on
        QTreeView* dialogTree = new QTreeView(dialog);

        // Outside the window: the same class with no window above it, and another view below the window
        QWidget elsewhere;
        QTreeView* strayTree = new QTreeView(&elsewhere);
        QListView* list = new QListView(&window);

        for (QAbstractItemView* view : { static_cast<QAbstractItemView*>(tree), static_cast<QAbstractItemView*>(deepTree),
                                         static_cast<QAbstractItemView*>(dialogTree), static_cast<QAbstractItemView*>(strayTree),
                                         static_cast<QAbstractItemView*>(list) })
        {
            flatten(*view);
        }

        for (const QTreeView* below : { tree, deepTree, dialogTree })
        {
            ASSERT_TRUE(elementOf(below) != nullptr);
            EXPECT_STREQ(elementOf(below)->m_name, TreeFlat::ElementName);
        }
        ASSERT_TRUE(elementOf(strayTree) != nullptr);
        EXPECT_STRNE(elementOf(strayTree)->m_name, TreeFlat::ElementName);
        EXPECT_FALSE(elementOf(strayTree)->m_check.declared());
        ASSERT_TRUE(elementOf(list) != nullptr);
        EXPECT_STRNE(elementOf(list)->m_name, TreeFlat::ElementName);
        EXPECT_FALSE(elementOf(list)->m_check.declared());
    }

    TEST_F(AssetImporterTreeFlatSheetTests, Registry_ATreeBuiltBeforeItIsBelowTheWindowHasTheElementOfThePlaceItIsPolishedAt)
    {
        // The "Select nodes" tree is built with no parent and handed to OverlayWidget::PushLayerToContainingOverlay afterwards
        // (NodeTreeSelectionWidget.cpp: ResetNewTreeWidget, then the push), which puts it into a dialog and shows that
        // (OverlayWidgetLayer.cpp:83-91). The matcher reads the places of a tree when the style polishes it, so the element is the
        // window's if nothing polished the tree before the push, and a polish after the push gives it. The order of the real push is
        // not built here (it needs the scene settings system), nor is the dialog's parent (see open item 8 of the I.3n2 fix report).
        TreeFlat::ImporterClassWidget window(TreeFlat::WindowClass);
        QWidget graph;                                                    // the SceneGraphWidget, parentless
        QTreeView* tree = new QTreeView(&graph);                          // its m_selectionTree

        flatten(*tree);
        ASSERT_TRUE(elementOf(tree) != nullptr);
        EXPECT_STRNE(elementOf(tree)->m_name, TreeFlat::ElementName);     // polished first: not below the window yet

        graph.setParent(&window);                                         // put below the window class
        AzQtComponents::ItemViewElement::resolve(tree);                   // the style's polish, as a style change gives it
        ASSERT_TRUE(elementOf(tree) != nullptr);
        EXPECT_STREQ(elementOf(tree)->m_name, TreeFlat::ElementName);
    }

    TEST_F(AssetImporterTreeFlatSheetTests, Element_ItHasTheValuesOfTheSheetOverTheTreesOfTheApplicationSheet)
    {
        TreeFlat::TreeTwin flat(m_style.get(), false);
        const AzQtComponents::ItemViewElement* element = elementOf(flat.m_tree);
        ASSERT_TRUE(element != nullptr);
        EXPECT_STREQ(element->m_name, TreeFlat::ElementName);
        EXPECT_EQ(element->m_indicatorSize, QSize(12, 12));                         // width, height: AssetImporterWindow.qss:8-9
        EXPECT_EQ(element->m_indicatorMargin, QMargins(-3, 0, 0, 0));               // the Global tree element's: TableView.qss:81, :84
        EXPECT_EQ(element->m_check.m_radius, 2);                                    // border-radius: :11, :18
        EXPECT_EQ(element->m_indentation, 20);                                      // the Global tree element's

        // The look of the box in each state, from the sheet over the application's: the line, the fill and the image (nullptr: the stock
        // check box's image of the state, "": the declared url that loads nothing, so no image)
        using Item = AzQtComponents::ItemViewElement;
        struct Expected
        {
            unsigned m_states;
            int m_line;
            QColor m_lineColor;
            QColor m_fill;
            const char* m_image;
        };
        const unsigned h = Item::Hover;
        const unsigned c = Item::Checked;
        const unsigned i = Item::Indeterminate;
        const unsigned d = Item::Disabled;
        const Expected table[] = {
            { 0, 1, QColor(204, 204, 204), QColor(), nullptr },                     // unchecked: :10, the stock off.svg (CheckBox:60-63)
            { i, 1, QColor(204, 204, 204), QColor(), nullptr },                     // :10, the stock partial-selected.svg (:30-33)
            { c, 0, QColor(), QColor(66, 133, 244), "" },                           // :16-19
            { h, 1, QColor(255, 255, 255), QColor(), "" },                          // :24, :34
            { h | i, 1, QColor(255, 255, 255), QColor(), "" },                      // :24, :34 (:!checked)
            { h | c, 1, QColor(255, 255, 255), QColor(66, 133, 244), "" },          // :24 after :19, :16, :29
            { d, 1, QColor(102, 102, 102), QColor(), nullptr },                     // :39, the stock off-disabled.svg (CheckBox.qss:70-73)
            { d | i, 1, QColor(102, 102, 102), QColor(), nullptr },                 // :39, the stock partial-selected-disabled.svg (:40-43)
            { d | c, 1, QColor(102, 102, 102), QColor(66, 133, 244), "" },          // :39 after :19, :16, :17 over CheckBox.qss:55-58
        };
        for (const Expected& expected : table)
        {
            SCOPED_TRACE(expected.m_states);
            const Item::CheckLook look = element->checkLookOf(expected.m_states);
            EXPECT_EQ(look.m_borderWidth, expected.m_line);
            EXPECT_EQ(look.m_borderColor, expected.m_lineColor);
            EXPECT_EQ(look.m_fill, expected.m_fill);
            ASSERT_EQ(look.m_image == nullptr, expected.m_image == nullptr);
            if (expected.m_image)
            {
                EXPECT_STREQ(look.m_image, expected.m_image);
            }
        }
    }

    // ----------------------------------------------------------------------------------
    // Geometry: the room of the box, and the rects of an item that has one
    // ----------------------------------------------------------------------------------

    TEST_F(AssetImporterTreeFlatSheetTests, Geometry_TheBoxTheItemAndItsRectsAreThoseOfTheRuleOfEachState)
    {
        TreeFlat::TreeTwin legacy(m_fusion.get(), true);
        TreeFlat::TreeTwin flat(m_style.get(), false);
        const QWidget* legacyView = legacy.m_tree;
        const QWidget* flatView = flat.m_tree;
        ASSERT_TRUE(elementOf(flatView) != nullptr);
        ASSERT_TRUE(elementOf(legacyView) == nullptr);

        for (const TreeFlat::CheckCase& test : TreeFlat::CheckCases)
        {
            SCOPED_TRACE(test.m_name);
            const QStyleOptionViewItem legacyOption = TreeFlat::itemOption(legacyView, test);
            const QStyleOptionViewItem flatOption = TreeFlat::itemOption(flatView, test);

            // The room the layout makes for the box is the rule's of the state: 12 by 12, the 3px of margin-left of the application's
            // rule for a tree (TableView.qss:84) and the line of the state around it. A checked box has none, unless it is hovered (the
            // window's :hover is after its :checked) or disabled (the same of :!enabled), so its box is the narrower and lower one.
            const int width = legacyView->style()->pixelMetric(QStyle::PM_IndicatorWidth, &legacyOption, legacyView);
            const int height = legacyView->style()->pixelMetric(QStyle::PM_IndicatorHeight, &legacyOption, legacyView);
            const bool bare = test.m_check == Qt::Checked && !test.m_hovered && !test.m_disabled;
            EXPECT_EQ(width, bare ? 9 : 11);
            EXPECT_EQ(height, bare ? 12 : 14);
            EXPECT_EQ(flatView->style()->pixelMetric(QStyle::PM_IndicatorWidth, &flatOption, flatView), width);
            EXPECT_EQ(flatView->style()->pixelMetric(QStyle::PM_IndicatorHeight, &flatOption, flatView), height);

            // The size of the item, and the rects of its check box, text and focus
            EXPECT_EQ(flatView->style()->sizeFromContents(QStyle::CT_ItemViewItem, &flatOption, QSize(), flatView),
                legacyView->style()->sizeFromContents(QStyle::CT_ItemViewItem, &legacyOption, QSize(), legacyView));
            for (const auto element :
                { QStyle::SE_ItemViewItemCheckIndicator, QStyle::SE_ItemViewItemText, QStyle::SE_ItemViewItemFocusRect })
            {
                EXPECT_EQ(flatView->style()->subElementRect(element, &flatOption, flatView),
                    legacyView->style()->subElementRect(element, &legacyOption, legacyView)) << "element " << int(element);
            }
            EXPECT_EQ(flatView->style()->subElementRect(QStyle::SE_ItemViewItemCheckIndicator, &flatOption, flatView).size(),
                QSize(width, height));
        }
    }

    // ----------------------------------------------------------------------------------
    // Pixels: the element against the sheet, in every state, at the device ratios of the GUI
    // ----------------------------------------------------------------------------------

    TEST_F(AssetImporterTreeFlatSheetTests, Pixels_TheBoxIsWhatTheSheetPaintedInEveryStateAtEveryRatio)
    {
        // The stock images of the application sheet are in the library: a process that did not register them would draw none, and the
        // twins would be two boxes with nothing inside the line
        for (const char* stock : { "off", "off-disabled", "partial-selected", "partial-selected-disabled" })
        {
            ASSERT_TRUE(QFile::exists(QStringLiteral(":/stylesheet/img/UI20/checkbox/%1.svg").arg(QLatin1String(stock)))) << stock;
        }

        TreeFlat::TreeTwin legacy(m_fusion.get(), true);
        TreeFlat::TreeTwin flat(m_style.get(), false);

        for (const qreal ratio : { 1.0, 1.5, 2.0 })
        {
            for (const TreeFlat::CheckCase& test : TreeFlat::CheckCases)
            {
                const QImage legacyImage = TreeFlat::paintCheck(legacy.m_tree, test, ratio);
                const QImage flatImage = TreeFlat::paintCheck(flat.m_tree, test, ratio);
                EXPECT_EQ(TreeFlat::firstDifference(legacyImage, flatImage), std::string()) << test.m_name << " at ratio " << ratio;

                // The twins are not two images of nothing: the colours of the sheet's rules are in the legacy one (at ratio 1, where a line
                // is a pixel)
                if (ratio == 1.0)
                {
                    const bool checked = test.m_check == Qt::Checked;
                    if (checked)
                    {
                        EXPECT_GT(TreeFlat::countOf(legacyImage, QColor(66, 133, 244)), 0) << test.m_name;       // :16
                    }
                    if (test.m_hovered)
                    {
                        EXPECT_GT(TreeFlat::countOf(legacyImage, QColor(255, 255, 255)), 0) << test.m_name;      // :24
                    }
                    else if (test.m_disabled)
                    {
                        EXPECT_GT(TreeFlat::countOf(legacyImage, QColor(102, 102, 102)), 0) << test.m_name;      // :39
                    }
                    else if (!checked)
                    {
                        EXPECT_GT(TreeFlat::countOf(legacyImage, QColor(204, 204, 204)), 0) << test.m_name;      // :10
                    }

                    // What is in the box: it begins 3px to the left of its slot (the margin-left of -3, TableView.qss:84), and the image is
                    // inside its line. A state with the stock image has it (a white outline, a blue square, grey ones); a state with a
                    // declared image that loads nothing has the rule's fill, or the backdrop, and nothing else (:17, :29, :34).
                    const QStyleOptionViewItem item = TreeFlat::itemOption(legacy.m_tree, test);
                    const QRect slot = legacy.m_tree->style()->subElementRect(
                        QStyle::SE_ItemViewItemCheckIndicator, &item, legacy.m_tree);
                    const QRect box = slot.adjusted(-3, 0, 0, 0);
                    const QRect inside = box.adjusted(1, 1, -1, -1);
                    if (!checked && !test.m_hovered)
                    {
                        EXPECT_GT(TreeFlat::countNotBackdrop(legacyImage, inside), 20) << test.m_name;
                    }
                    else
                    {
                        EXPECT_EQ(QColor::fromRgba(legacyImage.pixel(box.center().x(), box.center().y())),
                            checked ? QColor(66, 133, 244) : QColor(0x80, 0x80, 0x80)) << test.m_name;
                        if (!checked)
                        {
                            // (inside the line less its rounded corner: the radius of 2 and the line of 1)
                            EXPECT_EQ(TreeFlat::countNotBackdrop(legacyImage, box.adjusted(3, 3, -3, -3)), 0) << test.m_name;
                        }
                    }
                }
            }
        }
    }

    TEST_F(AssetImporterTreeFlatSheetTests, Pixels_TheBoxIsWhatTheSheetPaintedRightToLeftToo)
    {
        // The box sticks out to the physical left of its slot (the margin of the sheet is not mirrored)
        TreeFlat::TreeTwin legacy(m_fusion.get(), true);
        TreeFlat::TreeTwin flat(m_style.get(), false);
        legacy.m_root.setLayoutDirection(Qt::RightToLeft);
        flat.m_root.setLayoutDirection(Qt::RightToLeft);

        for (const TreeFlat::CheckCase& test : TreeFlat::CheckCases)
        {
            const QImage legacyImage = TreeFlat::paintCheck(legacy.m_tree, test, 1.5);
            const QImage flatImage = TreeFlat::paintCheck(flat.m_tree, test, 1.5);
            EXPECT_EQ(TreeFlat::firstDifference(legacyImage, flatImage), std::string()) << test.m_name << " right to left";
        }
    }
} // namespace UnitTest
