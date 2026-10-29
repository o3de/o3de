/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/Widgets/BrowseEdit.h>
#include <AzQtComponents/Components/Widgets/Card.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFont>
#include <QHeaderView>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QPainter>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QStyleFactory>
#include <QStyleOptionFrame>
#include <QStyleOptionViewItem>
#include <QTableView>
#include <QTextEdit>
#include <QToolButton>
#include <QToolTip>
#include <QTreeView>

namespace AzQtComponents
{
    //! Named, not anonymous: a unity build joins this file with the other tests, where anonymous namespaces merge
    namespace FlatPaletteTestsInternal
    {
        // The values below are stated again here, with the source line that grants each (the tests prove what the
        // flattened style carries, not that it reads a sheet). Native dumps: UiCaptures/n2/<App>/qss.
        const QColor White(0xFF, 0xFF, 0xFF);           // BaseStyleSheet.qss:18 '*' { color: white }, in every state
        const QColor DisabledGrey(0x99, 0x99, 0x99);    // PushButton.qss:43, CheckBox.qss:18, RadioButton.qss:17, Card.qss:272, LineEdit.qss:28-35
        const QColor DisabledPrimary(0xBB, 0xBB, 0xBB); // PushButton.qss:60
        const QColor DisabledItem(0x77, 0x77, 0x77);    // TableView.qss:99-102 QTreeView::item:disabled
        const QColor ViewFill(0x44, 0x44, 0x44);        // BaseStyleSheet.qss:23-30 (a view without a parent)
        const QColor CodeFill(0x22, 0x22, 0x22);        // a fill code gave a view (ComponentPaletteWidget.cpp:88-107)
        const QColor Selection(0x65, 0x65, 0x65);       // TableView.qss:23, the application palette's Highlight
        const QColor Black(0x00, 0x00, 0x00);           // ToolTip.qss:13

        //! The brightest channel of one item's text painted through the view's style over black. The item is enabled or not
        //! (the flag of its option; a disabled view clears it too), selected or not. A bold face, so that some pixel is
        //! covered whole whatever font the machine falls back to; the selection fill (#656565) stays below the greys tested.
        int brightestItemChannel(const QAbstractItemView& view, bool itemEnabled, bool selected)
        {
            QStyleOptionViewItem option;
            option.initFrom(&view);
            option.rect = QRect(0, 0, 120, 24);
            option.state.setFlag(QStyle::State_Enabled, itemEnabled && view.isEnabled());
            option.state.setFlag(QStyle::State_Selected, selected);
            option.palette.setCurrentColorGroup(option.state.testFlag(QStyle::State_Enabled) ? QPalette::Active : QPalette::Disabled);
            option.features = QStyleOptionViewItem::HasDisplay;
            option.text = QStringLiteral("Item");
            option.displayAlignment = Qt::AlignLeft | Qt::AlignVCenter;
            option.font = view.font();
            option.font.setPixelSize(16);
            option.font.setBold(true);
            option.fontMetrics = QFontMetrics(option.font);

            QImage image(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
            image.fill(Qt::black);
            QPainter painter(&image);
            painter.setFont(option.font);
            view.style()->drawControl(QStyle::CE_ItemViewItem, &option, &painter, &view);
            painter.end();

            int brightest = 0;
            for (int y = 0; y < image.height(); ++y)
            {
                for (int x = 0; x < image.width(); ++x)
                {
                    const QColor pixel = image.pixelColor(x, y);
                    brightest = qMax(brightest, qMax(pixel.red(), qMax(pixel.green(), pixel.blue())));
                }
            }
            return brightest;
        }
    } // namespace FlatPaletteTestsInternal

    // ----------------------------------------------------------------------------------
    // Fixture: the flattened style chain (Style -> Fusion) under an application
    // ----------------------------------------------------------------------------------

    class FlatPaletteTests
        : public ::UnitTest::LeakDetectionFixture
    {
    public:
        FlatPaletteTests()
        {
            m_application = AZStd::make_unique<QApplication>(m_argc, m_argv);

            // As StyleManager::initialize builds it (StyleManager.cpp:244-246); the application owns it
            QApplication::setStyle(new Style(QStyleFactory::create(QStringLiteral("Fusion"))));
        }

        ~FlatPaletteTests() override
        {
            m_application.reset();
        }

        static QColor disabledColor(const QWidget& widget, QPalette::ColorRole role)
        {
            return widget.palette().color(QPalette::Disabled, role);
        }

        static QColor activeColor(const QWidget& widget, QPalette::ColorRole role)
        {
            return widget.palette().color(QPalette::Active, role);
        }

    protected:
        int m_argc = 1;                                     // QApplication keeps a reference to it
        char m_programName[5] = "test";
        char* m_argv[2] = { m_programName, nullptr };
        AZStd::unique_ptr<QApplication> m_application;
    };

    // ----------------------------------------------------------------------------------
    // T-DT: disabled text. The '*' rule's white holds in every state; the classes with a :disabled colour differ.
    // ----------------------------------------------------------------------------------

    TEST_F(FlatPaletteTests, DisabledText_ApplicationPaletteKeepsTheStarRulesWhite)
    {
        using namespace FlatPaletteTestsInternal;
        ASSERT_TRUE(StyleManager::stylesheetsDisabled()); // the test environment sets it (AzQtComponentTests.cpp)

        // Native dumps: disabled.WindowText / Text / ButtonText / HighlightedText #ffffffff on a plain widget, a label, a tool button
        QWidget plain;
        QLabel label;
        QToolButton toolButton;
        for (QWidget* widget : { &plain, static_cast<QWidget*>(&label), static_cast<QWidget*>(&toolButton) })
        {
            widget->ensurePolished();
            for (const QPalette::ColorRole role : { QPalette::WindowText, QPalette::Text, QPalette::ButtonText, QPalette::HighlightedText })
            {
                EXPECT_TRUE(disabledColor(*widget, role) == White);
            }
        }
    }

    TEST_F(FlatPaletteTests, DisabledText_PushButtonIsGreyAndADefaultOneIsLighter)
    {
        using namespace FlatPaletteTestsInternal;
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        QPushButton plain;
        plain.ensurePolished();
        for (const QPalette::ColorRole role : { QPalette::WindowText, QPalette::Text, QPalette::ButtonText })
        {
            EXPECT_TRUE(disabledColor(plain, role) == DisabledGrey);
        }
        EXPECT_TRUE(activeColor(plain, QPalette::ButtonText) == White);

        // QPushButton:default:disabled and QPushButton.Primary:disabled (PushButton.qss:60): PushButton::polish tags a default button Primary
        QPushButton primary;
        primary.setDefault(true);
        primary.ensurePolished();
        for (const QPalette::ColorRole role : { QPalette::WindowText, QPalette::Text, QPalette::ButtonText })
        {
            EXPECT_TRUE(disabledColor(primary, role) == DisabledPrimary);
        }
        EXPECT_TRUE(activeColor(primary, QPalette::ButtonText) == White);
    }

    TEST_F(FlatPaletteTests, DisabledText_CheckBoxAndRadioButtonAreGrey)
    {
        using namespace FlatPaletteTestsInternal;
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        QCheckBox checkBox;
        QRadioButton radioButton;
        for (QWidget* widget : { static_cast<QWidget*>(&checkBox), static_cast<QWidget*>(&radioButton) })
        {
            widget->ensurePolished();
            EXPECT_TRUE(disabledColor(*widget, QPalette::WindowText) == DisabledGrey);
            EXPECT_TRUE(activeColor(*widget, QPalette::WindowText) == White);
        }
    }

    TEST_F(FlatPaletteTests, DisabledText_LabelIsGreyOnlyInsideACard)
    {
        using namespace FlatPaletteTestsInternal;
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        // Card.qss:272 AzQtComponents--Card QLabel:disabled; the Card keeps its own header sheet (Card.cpp:101), so the label
        // is a child of the card itself, outside it
        Card card;
        auto* inside = new QLabel(&card);
        QLabel outside;
        inside->ensurePolished();
        outside.ensurePolished();

        EXPECT_TRUE(disabledColor(*inside, QPalette::WindowText) == DisabledGrey);
        EXPECT_TRUE(activeColor(*inside, QPalette::WindowText) == White);
        EXPECT_TRUE(disabledColor(outside, QPalette::WindowText) == White);
    }

    TEST_F(FlatPaletteTests, DisabledText_TreeViewItemsAreGreyerThanTheViewsOwnRoles)
    {
        using namespace FlatPaletteTestsInternal;
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        // TableView.qss:99-102 names QTreeView only: a list and a table keep the white. The colour belongs to the item
        // rule, not to the widget: the roles of the view are white in the Native dumps, disabled too.
        QTreeView tree;
        QListView list;
        QTableView table;
        for (QWidget* widget : { static_cast<QWidget*>(&tree), static_cast<QWidget*>(&list), static_cast<QWidget*>(&table) })
        {
            widget->ensurePolished();
            EXPECT_TRUE(disabledColor(*widget, QPalette::Text) == White);
            EXPECT_TRUE(disabledColor(*widget, QPalette::WindowText) == White);
        }

        EXPECT_EQ(brightestItemChannel(tree, false, false), DisabledItem.red());
        EXPECT_EQ(brightestItemChannel(list, false, false), White.red());
        EXPECT_EQ(brightestItemChannel(table, false, false), White.red());
    }

    TEST_F(FlatPaletteTests, DisabledText_TreeViewItemColourFollowsTheItemAndTheView)
    {
        using namespace FlatPaletteTestsInternal;
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        QTreeView tree;
        tree.ensurePolished();

        // An enabled item is white; a disabled item of an enabled view, and a selected one, are the item rule's grey
        EXPECT_EQ(brightestItemChannel(tree, true, false), White.red());
        EXPECT_EQ(brightestItemChannel(tree, false, false), DisabledItem.red());
        EXPECT_EQ(brightestItemChannel(tree, false, true), DisabledItem.red());

        // The items of a disabled view: the state of the option says so (QStyleOption::initFrom)
        tree.setEnabled(false);
        EXPECT_EQ(brightestItemChannel(tree, true, false), DisabledItem.red());
    }

    TEST_F(FlatPaletteTests, DisabledText_EntryFieldsAreGreyInAllThreeRoles)
    {
        using namespace FlatPaletteTestsInternal;
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        // LineEdit.qss:28-35, SpinBox.qss:69-74, ComboBox.qss:53-57: `color: #999999` sets all three roles
        QLineEdit lineEdit;
        QTextEdit textEdit;
        QComboBox comboBox;
        QSpinBox spinBox;
        QDoubleSpinBox doubleSpinBox;
        for (QWidget* widget : { static_cast<QWidget*>(&lineEdit), static_cast<QWidget*>(&textEdit), static_cast<QWidget*>(&comboBox),
                                 static_cast<QWidget*>(&spinBox), static_cast<QWidget*>(&doubleSpinBox) })
        {
            widget->ensurePolished();
            for (const QPalette::ColorRole role : { QPalette::WindowText, QPalette::Text, QPalette::ButtonText })
            {
                EXPECT_TRUE(disabledColor(*widget, role) == DisabledGrey);
            }
            EXPECT_TRUE(disabledColor(*widget, QPalette::Base) == QColor(0x66, 0x66, 0x66));
        }
    }

    TEST_F(FlatPaletteTests, DisabledText_LineEditsInsideAFieldAreGreyToo)
    {
        using namespace FlatPaletteTestsInternal;
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        // The line edit of a BrowseEdit and of a spin box is polished by its owner (the QLineEdit rule reached it all the same)
        BrowseEdit browseEdit;
        browseEdit.ensurePolished();
        QLineEdit* browseLineEdit = browseEdit.lineEdit();
        browseLineEdit->ensurePolished();

        QSpinBox spinBox;
        spinBox.ensurePolished();
        QLineEdit* spinLineEdit = spinBox.findChild<QLineEdit*>();
        ASSERT_TRUE(spinLineEdit != nullptr);
        spinLineEdit->ensurePolished();

        for (QLineEdit* lineEdit : { browseLineEdit, spinLineEdit })
        {
            for (const QPalette::ColorRole role : { QPalette::WindowText, QPalette::Text, QPalette::ButtonText })
            {
                EXPECT_TRUE(disabledColor(*lineEdit, role) == DisabledGrey);
            }
        }
    }

    TEST_F(FlatPaletteTests, DisabledText_RepolishChangesNothing)
    {
        using namespace FlatPaletteTestsInternal;
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        QPushButton button;
        button.ensurePolished();
        const QPalette before = button.palette();

        button.style()->unpolish(&button);
        button.style()->polish(&button);

        EXPECT_TRUE(button.palette() == before);
    }

    // ----------------------------------------------------------------------------------
    // T-IV: parented item views. BaseStyleSheet.qss:32-38 QWidget > QTreeView { background-color: transparent }
    // ----------------------------------------------------------------------------------

    TEST_F(FlatPaletteTests, ParentedItemView_BaseIsANullBrushAndWindowIsTransparent)
    {
        using namespace FlatPaletteTestsInternal;
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        // The Native dumps of 111 tree views: Base #ff000000 (a null brush prints black), Window #00000000
        QWidget parent;
        QTreeView tree(&parent);
        QListView list(&parent);
        QTableView table(&parent);
        for (QAbstractItemView* view : { static_cast<QAbstractItemView*>(&tree), static_cast<QAbstractItemView*>(&list),
                                         static_cast<QAbstractItemView*>(&table) })
        {
            view->ensurePolished();
            for (const auto group : { QPalette::Active, QPalette::Inactive, QPalette::Disabled })
            {
                EXPECT_EQ(view->palette().brush(group, QPalette::Base).style(), Qt::NoBrush);
                EXPECT_EQ(view->palette().color(group, QPalette::Window).alpha(), 0);
            }
            EXPECT_FALSE(view->viewport()->autoFillBackground());
            // the viewport paints from Base, which it inherits
            EXPECT_EQ(view->viewport()->palette().brush(QPalette::Active, QPalette::Base).style(), Qt::NoBrush);
        }
    }

    TEST_F(FlatPaletteTests, ParentedItemView_ViewWithoutAParentKeepsTheOpaqueFill)
    {
        using namespace FlatPaletteTestsInternal;
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        // A completer popup: BaseStyleSheet.qss:23-30 only
        QListView popup;
        popup.ensurePolished();

        EXPECT_TRUE(activeColor(popup, QPalette::Base) == ViewFill);
        EXPECT_TRUE(popup.viewport()->autoFillBackground());
    }

    TEST_F(FlatPaletteTests, ParentedItemView_FillSetByCodeIsKept)
    {
        using namespace FlatPaletteTestsInternal;
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        // ComponentPaletteWidget.cpp:88-107: the tree is given #222222 before it is polished
        QWidget parent;
        QTreeView tree(&parent);
        QPalette palette = tree.palette();
        palette.setColor(QPalette::Base, CodeFill);
        tree.setPalette(palette);
        tree.ensurePolished();

        EXPECT_TRUE(activeColor(tree, QPalette::Base) == CodeFill);
        EXPECT_TRUE(tree.viewport()->autoFillBackground());
    }

    TEST_F(FlatPaletteTests, ParentedItemView_FillInheritedFromAParentIsKept)
    {
        using namespace FlatPaletteTestsInternal;
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        // The popup list of a combo box: its container carries the menu palette, and the list inherits it
        QWidget container;
        QPalette palette = container.palette();
        palette.setColor(QPalette::Base, CodeFill);
        container.setPalette(palette);
        QListView list(&container);
        list.ensurePolished();

        EXPECT_TRUE(activeColor(list, QPalette::Base) == CodeFill);
    }

    TEST_F(FlatPaletteTests, ParentedItemView_HeaderKeepsItsOwnPalette)
    {
        using namespace FlatPaletteTestsInternal;
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        // A QHeaderView is a QAbstractItemView the rule does not name: Style::polish gives it #2d2d2d sections on a #444444 window
        QWidget parent;
        QTreeView tree(&parent);
        tree.ensurePolished();
        QHeaderView* header = tree.header();
        header->ensurePolished();

        EXPECT_TRUE(activeColor(*header, QPalette::Base) == ViewFill);
        EXPECT_TRUE(activeColor(*header, QPalette::Window) == ViewFill);
        EXPECT_TRUE(activeColor(*header, QPalette::Button) == QColor(0x2D, 0x2D, 0x2D));
        EXPECT_TRUE(disabledColor(*header, QPalette::ButtonText) == White);
    }

    // ----------------------------------------------------------------------------------
    // T-SEL: an entry field's selection is the application's. Under the sheet no field took its parent's palette
    // (QWidgetPrivate::naturalWidgetPalette): the selection roles a view's rule gives it never reached its editors.
    // ----------------------------------------------------------------------------------

    TEST_F(FlatPaletteTests, EntrySelection_EditorsInAViewKeepTheApplicationsSelection)
    {
        using namespace FlatPaletteTestsInternal;
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        // The outliner's tree as its element leaves it (EntityOutliner.qss:17 selection-background-color: transparent)
        QTreeView tree;
        tree.ensurePolished();
        QPalette treePalette = tree.palette();
        treePalette.setColor(QPalette::Highlight, QColor(0, 0, 0, 0));
        tree.setPalette(treePalette);

        // The editors a delegate opens in the viewport: a line edit, and the line edit of a spin box (it takes the spin box's)
        QLineEdit lineEditor(tree.viewport());
        QSpinBox spinEditor(tree.viewport());
        lineEditor.ensurePolished();
        spinEditor.ensurePolished();
        QLineEdit* spinLineEdit = spinEditor.findChild<QLineEdit*>();
        ASSERT_TRUE(spinLineEdit != nullptr);
        for (const QWidget* editor : { static_cast<const QWidget*>(&lineEditor), static_cast<const QWidget*>(spinLineEdit) })
        {
            for (const auto group : { QPalette::Active, QPalette::Inactive, QPalette::Disabled })
            {
                EXPECT_TRUE(editor->palette().color(group, QPalette::Highlight) == Selection);
                EXPECT_TRUE(editor->palette().color(group, QPalette::HighlightedText) == White);    // TableView.qss:24
            }
        }
        EXPECT_EQ(activeColor(tree, QPalette::Highlight).alpha(), 0); // the view keeps its own
    }

    // ----------------------------------------------------------------------------------
    // T-TIP: tooltips. ToolTip.qss:10-20 is the only rule that reached a QTipLabel: the sheet names it QToolTip, never QLabel.
    // ----------------------------------------------------------------------------------

    TEST_F(FlatPaletteTests, ToolTip_IsABlackBoxWithWhiteTextAndTheRulesMargin)
    {
        using namespace FlatPaletteTestsInternal;
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        // A tooltip takes the palette of QToolTip, which Qt fills from the platform theme (Windows: COLOR_INFOBK, COLOR_INFOTEXT).
        // The style replaces it when it polishes the application.
        QPalette platform;
        platform.setColor(QPalette::ToolTipBase, QColor(0xFF, 0xFF, 0xE1));
        platform.setColor(QPalette::ToolTipText, Qt::black);
        QToolTip::setPalette(platform);
        QApplication::setStyle(new Style(QStyleFactory::create(QStringLiteral("Fusion"))));

        // Shown at once, without an effect widget over it
        QApplication::setEffectEnabled(Qt::UI_FadeTooltip, false);
        QApplication::setEffectEnabled(Qt::UI_AnimateTooltip, false);
        QToolTip::showText(QPoint(0, 0), QStringLiteral("Tip"));
        QLabel* tip = nullptr;
        for (QWidget* window : QApplication::topLevelWidgets())
        {
            if (window->inherits("QTipLabel"))
            {
                tip = qobject_cast<QLabel*>(window);
            }
        }
        ASSERT_TRUE(tip != nullptr);

        // Tooltips paint with the Inactive group
        EXPECT_TRUE(tip->palette().color(QPalette::Inactive, QPalette::ToolTipBase) == Black);
        EXPECT_TRUE(tip->palette().color(QPalette::Inactive, QPalette::ToolTipText) == White); // ToolTip.qss:12

        // The panel is the fill alone: `border: 1px transparent black` (ToolTip.qss:14) names no border style
        QStyleOptionFrame option;
        option.initFrom(tip);
        option.rect = QRect(0, 0, 40, 20);
        QImage image(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::red);
        QPainter painter(&image);
        tip->style()->drawPrimitive(QStyle::PE_PanelTipLabel, &option, &painter, tip);
        painter.end();
        EXPECT_TRUE(image.pixelColor(0, 0) == Black);
        EXPECT_TRUE(image.pixelColor(20, 10) == Black);

        // The rule's margin (ToolTip.qss:15, :17-18), not the QLabel rule's; the label's own margin is 1 + PM_ToolTipLabelFrameWidth,
        // 3 under the sheet (border 0 + margin-left 2) and under Fusion (2)
        EXPECT_EQ(tip->contentsMargins(), QMargins(2, 4, 0, 4));
        EXPECT_EQ(tip->margin(), 3);

        // ~QTipLabel clears QToolTip's instance before the fixture ends the application
        delete tip;
    }
} // namespace AzQtComponents
