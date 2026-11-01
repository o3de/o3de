/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzQtComponents/Components/Elements/MenuElement.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <QAction>
#include <QApplication>
#include <QIcon>
#include <QImage>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>
#include <QPainter>
#include <QPixmap>
#include <QScopedPointer>
#include <QStyleFactory>
#include <QStyleOption>

namespace AzQtComponents
{
    // ----------------------------------------------------------------------------------
    // Fixture: an application on Fusion, the flattened Style over Fusion, and the helpers that paint a menu item or
    // a menu bar item through either a menu under the legacy rules (Qt's own sheet style) or a menu under the flat style.
    // Every helper and constant is a member: a unity build joins the test files into one translation unit, where the
    // anonymous namespaces of the others (FlatFramePaintTests.cpp has a pixelAt) would meet them.
    // ----------------------------------------------------------------------------------

    class MenuElementTests
        : public ::UnitTest::LeakDetectionFixture
    {
    public:
        // Menu.qss and MenuBar.qss as the legacy GUI applied them, with the colours of the universal rule (BaseStyleSheet.qss:15-21)
        // and without fonts (a font is no part of an item's look). Stated again here: the tests prove that the flat menus draw as
        // Qt's own sheet style draws with these rules, not that the flat code reads the sheets.
        static constexpr const char* PopupSheet =
            "* { background-color: transparent; color: white; }"
            "QMenu { background-color: #222222; padding: 4px 2px; }"
            "QMenu::item:selected, QMenu::item .WidgetAction:hover { background-color: #444444; }"
            "QMenu::item:disabled { color: #555555; }"
            "QMenu::separator { height: 0px; border-bottom: 1px solid #454545; margin-top: 2px; margin-bottom: 3px; }";
        static constexpr const char* BarSheet =
            "* { background-color: transparent; color: white; }"
            "QMenuBar { min-height: 28px; height: 28px; margin: 0px; spacing: 0px;"
            " border-bottom-width: 2px; border-bottom-color: #111111; border-bottom-style: solid; }"
            "QMenuBar::item { padding-left: 14px; padding-right: 14px; }"
            "QMenuBar::item:selected { background-color: #555555; }"
            "QMenuBar::item:pressed { background-color: #222222; }";

        static constexpr QRgb SurfaceColor = 0x222222;      // what a menu is filled with
        static constexpr QRgb SelectedColor = 0x444444;     // under the item the pointer is on
        static constexpr QRgb WindowColor = 0x444444;       // the window a menu bar is transparent over

        //! QMenu::initStyleOption and QMenuBar::initStyleOption are protected: the option a paint event builds for an action
        class TestMenu : public QMenu
        {
        public:
            using QMenu::QMenu;
            using QMenu::initStyleOption;
        };

        class TestMenuBar : public QMenuBar
        {
        public:
            using QMenuBar::QMenuBar;
            using QMenuBar::initStyleOption;
        };

        MenuElementTests()
        {
            m_application = AZStd::make_unique<QApplication>(m_argc, m_argv);
            QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion"))); // the base of the legacy sheet
            m_style = AZStd::make_unique<Style>(QStyleFactory::create(QStringLiteral("Fusion")));
        }

        ~MenuElementTests() override
        {
            m_style.reset();
            m_application.reset();
        }

        // ---- menus

        //! A menu that has every look of an item: plain, with a mnemonic, with a shortcut, disabled, checkable (checked and not),
        //! with an icon, checked with an icon, a submenu, and separators between them. The icon is a flat square.
        static void fillMenu(QMenu& menu)
        {
            QPixmap pixmap(16, 16);
            pixmap.fill(QColor(0xC8, 0x3C, 0x3C));
            const QIcon icon(pixmap);

            menu.addAction(QStringLiteral("Plain item"));
            menu.addAction(QStringLiteral("Toggle &Breakpoint"));
            menu.addAction(QStringLiteral("With shortcut"))->setShortcut(QKeySequence(QStringLiteral("Ctrl+K")));
            menu.addSeparator();
            menu.addAction(QStringLiteral("Disabled item"))->setEnabled(false);
            QAction* unchecked = menu.addAction(QStringLiteral("Unchecked item"));
            unchecked->setCheckable(true);
            QAction* checked = menu.addAction(QStringLiteral("Checked item"));
            checked->setCheckable(true);
            checked->setChecked(true);
            menu.addSeparator();
            menu.addAction(icon, QStringLiteral("Item with icon"));
            QAction* checkedIcon = menu.addAction(icon, QStringLiteral("Checked item with icon"));
            checkedIcon->setCheckable(true);
            checkedIcon->setChecked(true);
            menu.addAction(QStringLiteral("Submenu"))->setMenu(new QMenu(&menu));
        }

        //! A menu bar of three titles and one action with none (LuaIDE's menu of the application)
        static void fillBar(QMenuBar& bar)
        {
            bar.addAction(QStringLiteral("File"));
            bar.addAction(QString());
            bar.addAction(QStringLiteral("Edit"));
            bar.addAction(QStringLiteral("Help"));
            bar.resize(400, 30);
        }

        static QSize deviceSize(const QSize& size, qreal ratio)
        {
            return QSize(qRound(size.width() * ratio), qRound(size.height() * ratio));
        }

        //! One action of the menu painted over the menu's surface, as the paint event of the menu does: the clip is the item
        static QImage paintItem(const TestMenu& menu, QAction* action, bool selected, const QSize& size, qreal ratio)
        {
            QStyleOptionMenuItem option;
            menu.initStyleOption(&option, action);
            option.rect = menu.actionGeometry(action);
            if (selected)
            {
                option.state |= QStyle::State_Selected;
            }

            QImage image(deviceSize(size, ratio), QImage::Format_ARGB32_Premultiplied);
            image.setDevicePixelRatio(ratio);
            image.fill(QColor::fromRgb(SurfaceColor));
            QPainter painter(&image);
            painter.setClipRect(option.rect);
            menu.style()->drawControl(QStyle::CE_MenuItem, &option, &painter, &menu);
            painter.end();
            return image;
        }

        // ---- menu bars

        enum class BarItemState { Idle, Hovered, Pressed };

        //! One action of the bar painted over the colour of its window, as the paint event of the bar does
        static QImage paintBarItem(const TestMenuBar& bar, QAction* action, BarItemState state, qreal ratio)
        {
            QStyleOptionMenuItem option;
            bar.initStyleOption(&option, action);
            option.rect = bar.actionGeometry(action);
            if (state != BarItemState::Idle)
            {
                option.state |= QStyle::State_Selected;
            }
            if (state == BarItemState::Pressed)
            {
                option.state |= QStyle::State_Sunken;
            }

            QImage image(deviceSize(bar.size(), ratio), QImage::Format_ARGB32_Premultiplied);
            image.setDevicePixelRatio(ratio);
            image.fill(QColor::fromRgb(WindowColor));
            QPainter painter(&image);
            painter.setClipRect(option.rect);
            bar.style()->drawControl(QStyle::CE_MenuBarItem, &option, &painter, &bar);
            painter.end();
            return image;
        }

        // ---- pictures

        static QRgb pixelOf(const QImage& image, int x, int y)
        {
            return image.pixelColor(x, y).rgb() & 0xFFFFFF;
        }

        //! How many device pixels two images of one size differ in (-1: the sizes differ)
        static int differingPixels(const QImage& a, const QImage& b)
        {
            if (a.size() != b.size())
            {
                return -1;
            }
            int count = 0;
            for (int y = 0; y < a.height(); ++y)
            {
                for (int x = 0; x < a.width(); ++x)
                {
                    count += (a.pixel(x, y) != b.pixel(x, y)) ? 1 : 0;
                }
            }
            return count;
        }

        //! The brightest red channel in a part of an image
        static int brightest(const QImage& image, const QRect& area)
        {
            int value = 0;
            for (int y = area.top(); y <= area.bottom(); ++y)
            {
                for (int x = area.left(); x <= area.right(); ++x)
                {
                    value = qMax(value, qRed(image.pixel(x, y)));
                }
            }
            return value;
        }

    protected:
        int m_argc = 1;                                     // QApplication keeps a reference to it
        char m_programName[5] = "test";
        char* m_argv[2] = { m_programName, nullptr };
        AZStd::unique_ptr<QApplication> m_application;
        AZStd::unique_ptr<Style> m_style;
    };

    // ----------------------------------------------------------------------------------
    // T-MENU.1: the Windows style the items are drawn by exists in this Qt (the delegate cannot work without it)
    // ----------------------------------------------------------------------------------

    TEST_F(MenuElementTests, WindowsStyle_IsInThisQt)
    {
        bool listed = false;
        for (const QString& key : QStyleFactory::keys())
        {
            listed = listed || key.compare(QStringLiteral("Windows"), Qt::CaseInsensitive) == 0;
        }
        EXPECT_TRUE(listed);

        QScopedPointer<QStyle> windows(QStyleFactory::create(QStringLiteral("windows")));
        EXPECT_FALSE(windows.isNull());
    }

    // ----------------------------------------------------------------------------------
    // T-MENU.2: the legacy rules' arithmetic, in Qt's own sheet style (runs in every process), and the flat style's
    // ----------------------------------------------------------------------------------

    TEST_F(MenuElementTests, LegacyRule_SeparatorIsItsBoxAndTheMenuPaddingIsTheMarginMetrics)
    {
        TestMenu legacy;
        legacy.setStyleSheet(QString::fromLatin1(PopupSheet));
        legacy.ensurePolished();

        // qstylesheetstyle.cpp:5430-5432 (CT_MenuItem of a separator): the width Qt asked, the height of the box (2 + 1 + 3)
        QStyleOptionMenuItem separator;
        separator.menuItemType = QStyleOptionMenuItem::Separator;
        separator.state = QStyle::State_Enabled;
        EXPECT_TRUE(legacy.style()->sizeFromContents(QStyle::CT_MenuItem, &separator, QSize(2, 2), &legacy) == QSize(2, 6));

        // qstylesheetstyle.cpp:5074-5084: the left and the top padding of `padding: 4px 2px`
        QStyleOption option;
        option.initFrom(&legacy);
        EXPECT_EQ(legacy.style()->pixelMetric(QStyle::PM_MenuHMargin, &option, &legacy), 2);
        EXPECT_EQ(legacy.style()->pixelMetric(QStyle::PM_MenuVMargin, &option, &legacy), 4);
    }

    TEST_F(MenuElementTests, LegacyRule_AnItemOtherThanASeparatorIsTheBaseStylesSize)
    {
        TestMenu legacy;
        legacy.setStyleSheet(QString::fromLatin1(PopupSheet));
        legacy.ensurePolished();

        QStyleOptionMenuItem item;
        item.menuItemType = QStyleOptionMenuItem::Normal;
        item.state = QStyle::State_Enabled;
        item.text = QStringLiteral("Item");
        item.fontMetrics = legacy.fontMetrics();
        item.font = legacy.font();
        const QSize contents(60, 17);
        EXPECT_TRUE(legacy.style()->sizeFromContents(QStyle::CT_MenuItem, &item, contents, &legacy)
            == QApplication::style()->sizeFromContents(QStyle::CT_MenuItem, &item, contents, &legacy));
    }

    TEST_F(MenuElementTests, LegacyRule_BarItemIsTheRulesHeightAndPaddingAndAnItemWithoutContentsStaysOut)
    {
        TestMenuBar legacy;
        legacy.setStyleSheet(QString::fromLatin1(BarSheet));
        legacy.ensurePolished();

        // qstylesheetstyle.cpp:560-571 (adjustSize), :1116-1121 (boxSize) and :5464-5469 (CT_MenuBarItem)
        QStyleOptionMenuItem item;
        item.menuItemType = QStyleOptionMenuItem::Normal;
        EXPECT_TRUE(legacy.style()->sizeFromContents(QStyle::CT_MenuBarItem, &item, QSize(40, 16), &legacy) == QSize(68, 28));
        EXPECT_TRUE(legacy.style()->sizeFromContents(QStyle::CT_MenuBarItem, &item, QSize(), &legacy).isEmpty());

        // qstylesheetstyle.cpp:2570-2603: the minimum height is the rule's, inside its border
        EXPECT_EQ(legacy.minimumSize().height(), 30);
    }

    TEST_F(MenuElementTests, Flat_SeparatorAndPaddingAreTheLegacyRulesBox)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled()); // the test environment sets it (AzQtComponentTests.cpp)

        TestMenu flat;
        flat.setStyle(m_style.get());
        flat.ensurePolished();

        QStyleOptionMenuItem separator;
        separator.menuItemType = QStyleOptionMenuItem::Separator;
        separator.state = QStyle::State_Enabled;
        EXPECT_TRUE(m_style->sizeFromContents(QStyle::CT_MenuItem, &separator, QSize(2, 2), &flat) == QSize(2, 6));
        EXPECT_EQ(MenuElement::forWidget(nullptr).separatorHeight(), 6);

        QStyleOption option;
        option.initFrom(&flat);
        EXPECT_EQ(m_style->pixelMetric(QStyle::PM_MenuHMargin, &option, &flat), 2);
        EXPECT_EQ(m_style->pixelMetric(QStyle::PM_MenuVMargin, &option, &flat), 4);

        // every other item keeps the base style's size, as in the legacy GUI
        QStyleOptionMenuItem item;
        item.menuItemType = QStyleOptionMenuItem::Normal;
        item.state = QStyle::State_Enabled;
        item.text = QStringLiteral("Item");
        item.fontMetrics = flat.fontMetrics();
        item.font = flat.font();
        const QSize contents(60, 17);
        EXPECT_TRUE(m_style->sizeFromContents(QStyle::CT_MenuItem, &item, contents, &flat)
            == m_style->baseStyle()->sizeFromContents(QStyle::CT_MenuItem, &item, contents, &flat));
    }

    TEST_F(MenuElementTests, Flat_BarItemIsTheLegacyRulesHeightAndPaddingAndAnItemWithoutContentsStaysOut)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled()); // the test environment sets it (AzQtComponentTests.cpp)

        TestMenuBar flat;
        flat.setStyle(m_style.get());
        flat.ensurePolished();

        QStyleOptionMenuItem item;
        item.menuItemType = QStyleOptionMenuItem::Normal;
        EXPECT_TRUE(m_style->sizeFromContents(QStyle::CT_MenuBarItem, &item, QSize(40, 16), &flat) == QSize(68, 28));
        EXPECT_TRUE(m_style->sizeFromContents(QStyle::CT_MenuBarItem, &item, QSize(), &flat).isEmpty());
        EXPECT_EQ(flat.minimumSize().height(), 30);
    }

    // ----------------------------------------------------------------------------------
    // T-MENU.3: a flat menu and a flat menu bar lay out as the legacy ones do (the dump equality of the parity scan)
    // ----------------------------------------------------------------------------------

    TEST_F(MenuElementTests, Menu_LaysItsItemsOutLikeTheLegacySheet)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled()); // the test environment sets it (AzQtComponentTests.cpp)

        TestMenu legacy;
        legacy.setStyleSheet(QString::fromLatin1(PopupSheet));
        fillMenu(legacy);
        TestMenu flat;
        flat.setStyle(m_style.get());
        fillMenu(flat);

        EXPECT_TRUE(flat.sizeHint() == legacy.sizeHint());
        const QList<QAction*> legacyActions = legacy.actions();
        const QList<QAction*> flatActions = flat.actions();
        ASSERT_EQ(legacyActions.size(), flatActions.size());
        for (int index = 0; index < legacyActions.size(); ++index)
        {
            EXPECT_TRUE(flat.actionGeometry(flatActions[index]) == legacy.actionGeometry(legacyActions[index])) << "action " << index;
        }
    }

    TEST_F(MenuElementTests, Bar_LaysItsItemsOutLikeTheLegacySheetAndLeavesOutTheItemWithoutATitle)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled()); // the test environment sets it (AzQtComponentTests.cpp)

        TestMenuBar legacy;
        legacy.setStyleSheet(QString::fromLatin1(BarSheet));
        fillBar(legacy);
        TestMenuBar flat;
        flat.setStyle(m_style.get());
        fillBar(flat);
        legacy.ensurePolished();
        flat.ensurePolished();

        const QList<QAction*> legacyActions = legacy.actions();
        const QList<QAction*> flatActions = flat.actions();
        ASSERT_EQ(legacyActions.size(), flatActions.size());
        for (int index = 0; index < legacyActions.size(); ++index)
        {
            EXPECT_TRUE(flat.actionGeometry(flatActions[index]) == legacy.actionGeometry(legacyActions[index])) << "action " << index;
        }

        // the action without a title takes no slot, the others are 28 high and 28 wider than their text
        EXPECT_TRUE(flat.actionGeometry(flatActions[1]).isEmpty());
        EXPECT_EQ(flat.actionGeometry(flatActions[0]).height(), 28);
        EXPECT_EQ(flat.actionGeometry(flatActions[2]).left(), flat.actionGeometry(flatActions[0]).right() + 1);

        EXPECT_EQ(flat.sizeHint().height(), legacy.sizeHint().height());
        EXPECT_EQ(flat.minimumSize().height(), legacy.minimumSize().height());
        EXPECT_TRUE(flat.minimumSizeHint() == legacy.minimumSizeHint());
    }

    // ----------------------------------------------------------------------------------
    // T-MENU.4: items paint as the legacy sheet style painted them (same binary, same Qt: no value is judged, the two are compared)
    // ----------------------------------------------------------------------------------

    TEST_F(MenuElementTests, Items_PaintLikeTheLegacySheetInEveryState)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled()); // the test environment sets it (AzQtComponentTests.cpp)

        TestMenu legacy;
        legacy.setStyleSheet(QString::fromLatin1(PopupSheet));
        fillMenu(legacy);
        TestMenu flat;
        flat.setStyle(m_style.get());
        fillMenu(flat);

        const QSize size = legacy.sizeHint();
        ASSERT_TRUE(flat.sizeHint() == size);
        const QList<QAction*> legacyActions = legacy.actions();
        const QList<QAction*> flatActions = flat.actions();
        ASSERT_EQ(legacyActions.size(), flatActions.size());

        for (const qreal ratio : { 1.0, 1.5 })
        {
            for (int index = 0; index < legacyActions.size(); ++index)
            {
                // a separator and a disabled item are never the current one of a menu
                const bool canBeSelected = !legacyActions[index]->isSeparator() && legacyActions[index]->isEnabled();
                for (const bool selected : { false, true })
                {
                    if (selected && !canBeSelected)
                    {
                        continue;
                    }
                    const QImage expected = paintItem(legacy, legacyActions[index], selected, size, ratio);
                    const QImage actual = paintItem(flat, flatActions[index], selected, size, ratio);
                    EXPECT_EQ(differingPixels(expected, actual), 0)
                        << "action " << index << (selected ? " selected" : " idle") << " at ratio " << ratio;
                }
            }
        }
    }

    TEST_F(MenuElementTests, Separator_IsTheNativeLineAtTheHalfPixelOffsetOfOneAndAHalfRatio)
    {
        // LuaIDE Debug, the first separator of the legacy capture (n2 and m0 qss): item rect (2, 29, 243, 6) at ratio 1.5. Device
        // rows 46 #333333 (a half covered row) and 47 #454545, from column 3 to 367
        QImage image(QSize(371, 410), QImage::Format_ARGB32_Premultiplied);
        image.setDevicePixelRatio(1.5);
        image.fill(QColor::fromRgb(SurfaceColor));
        QPainter painter(&image);
        MenuElement::forWidget(nullptr).drawSeparator(&painter, QRect(2, 29, 243, 6));
        painter.end();

        EXPECT_EQ(pixelOf(image, 100, 45), SurfaceColor);
        EXPECT_EQ(pixelOf(image, 100, 46), 0x333333u);
        EXPECT_EQ(pixelOf(image, 100, 47), 0x454545u);
        EXPECT_EQ(pixelOf(image, 100, 48), SurfaceColor);
        EXPECT_EQ(pixelOf(image, 2, 47), SurfaceColor);
        EXPECT_EQ(pixelOf(image, 3, 47), 0x454545u);
        EXPECT_EQ(pixelOf(image, 368, 47), SurfaceColor);
    }

    TEST_F(MenuElementTests, Separator_IsOneRowTwoRowsBelowTheTopOfItsBox)
    {
        // Menu.qss:28-34: margin-top 2px, a 1px border at the bottom of the box, margin-bottom 3px
        QImage image(QSize(247, 60), QImage::Format_ARGB32_Premultiplied);
        image.fill(QColor::fromRgb(SurfaceColor));
        QPainter painter(&image);
        MenuElement::forWidget(nullptr).drawSeparator(&painter, QRect(2, 29, 243, 6));
        painter.end();

        EXPECT_EQ(pixelOf(image, 100, 30), SurfaceColor);
        EXPECT_EQ(pixelOf(image, 100, 31), 0x454545u);
        EXPECT_EQ(pixelOf(image, 100, 32), SurfaceColor);
        EXPECT_EQ(pixelOf(image, 1, 31), SurfaceColor);
        EXPECT_EQ(pixelOf(image, 2, 31), 0x454545u);
        EXPECT_EQ(pixelOf(image, 244, 31), 0x454545u);
        EXPECT_EQ(pixelOf(image, 245, 31), SurfaceColor);
    }

    TEST_F(MenuElementTests, SelectedItem_IsFilledExceptForItsLastColumn)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled()); // the test environment sets it (AzQtComponentTests.cpp)

        TestMenu flat;
        flat.setStyle(m_style.get());
        QAction* action = flat.addAction(QStringLiteral("Item"));
        const QSize size = flat.sizeHint();
        const QRect rect = flat.actionGeometry(action);
        const QImage image = paintItem(flat, action, true, size, 1.0);

        // qwindowsstyle.cpp:1042-1043: fillRect(rect.adjusted(0, 0, -1, 0), Highlight)
        EXPECT_EQ(pixelOf(image, rect.left(), rect.top()), SelectedColor);
        EXPECT_EQ(pixelOf(image, rect.right() - 1, rect.bottom()), SelectedColor);
        EXPECT_EQ(pixelOf(image, rect.right(), rect.top() + 3), SurfaceColor);
        EXPECT_EQ(pixelOf(image, rect.left(), rect.top() - 1), SurfaceColor);
        EXPECT_EQ(pixelOf(image, rect.left(), rect.bottom() + 1), SurfaceColor);
    }

    TEST_F(MenuElementTests, CheckMark_IsDrawnForACheckedItemAndForNoOther)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled()); // the test environment sets it (AzQtComponentTests.cpp)

        TestMenu flat;
        flat.setStyle(m_style.get());
        QAction* plain = flat.addAction(QStringLiteral("Item"));
        QAction* unchecked = flat.addAction(QStringLiteral("Item"));
        unchecked->setCheckable(true);
        QAction* checked = flat.addAction(QStringLiteral("Item"));
        checked->setCheckable(true);
        checked->setChecked(true);
        const QSize size = flat.sizeHint();

        // the check column of an item with no icon: 12 wide (qwindowsstyle.cpp:1040), before the label
        auto checkColumn = [&](QAction* action)
        {
            const QRect rect = flat.actionGeometry(action);
            return paintItem(flat, action, false, size, 1.0).copy(QRect(rect.left(), rect.top(), 12, rect.height()));
        };
        EXPECT_TRUE(checkColumn(unchecked) == checkColumn(plain));   // Fusion drew a check BOX here, for the unchecked and the checked alike
        EXPECT_FALSE(checkColumn(checked) == checkColumn(plain));
    }

    TEST_F(MenuElementTests, ItemText_IsWhiteWhenEnabledAndTheDisabledColourWhenNot)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled()); // the test environment sets it (AzQtComponentTests.cpp)

        TestMenu flat;
        flat.setStyle(m_style.get());
        QAction* enabled = flat.addAction(QStringLiteral("Enabled item"));
        QAction* disabled = flat.addAction(QStringLiteral("Disabled item"));
        disabled->setEnabled(false);
        const QSize size = flat.sizeHint();

        // Menu.qss:25 (#555555), BaseStyleSheet.qss:18 (white): the label is to the right of the check column
        const QRect enabledRect = flat.actionGeometry(enabled);
        const QRect disabledRect = flat.actionGeometry(disabled);
        const QImage enabledImage = paintItem(flat, enabled, false, size, 1.0);
        const QImage disabledImage = paintItem(flat, disabled, false, size, 1.0);
        EXPECT_GT(brightest(enabledImage, enabledRect.adjusted(20, 0, 0, 0)), 0xAA);
        EXPECT_LE(brightest(disabledImage, disabledRect.adjusted(20, 0, 0, 0)), 0x55);
        EXPECT_GT(brightest(disabledImage, disabledRect.adjusted(20, 0, 0, 0)), 0x22);
    }

    TEST_F(MenuElementTests, Items_DoNotTakeTheirColoursFromThePaletteOfTheMenu)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled()); // the test environment sets it (AzQtComponentTests.cpp)

        QPalette hostile;
        for (int role = 0; role < QPalette::NColorRoles; ++role)
        {
            if (role != QPalette::NoRole)
            {
                hostile.setColor(static_cast<QPalette::ColorRole>(role), QColor(0xFF, 0x00, 0xFF));
            }
        }

        TestMenu clean;
        clean.setStyle(m_style.get());
        fillMenu(clean);
        TestMenu other;
        other.setPalette(hostile);
        other.setStyle(m_style.get());
        fillMenu(other);

        const QSize size = clean.sizeHint();
        ASSERT_TRUE(other.sizeHint() == size);
        const QList<QAction*> cleanActions = clean.actions();
        const QList<QAction*> otherActions = other.actions();
        for (int index = 0; index < cleanActions.size(); ++index)
        {
            const bool canBeSelected = !cleanActions[index]->isSeparator() && cleanActions[index]->isEnabled();
            for (const bool selected : { false, true })
            {
                if (selected && !canBeSelected)
                {
                    continue;
                }
                EXPECT_EQ(differingPixels(paintItem(clean, cleanActions[index], selected, size, 1.0),
                              paintItem(other, otherActions[index], selected, size, 1.0)), 0)
                    << "action " << index << (selected ? " selected" : " idle");
            }
        }
    }

    TEST_F(MenuElementTests, MenuPanel_IsTheSurfaceOfTheElement)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled()); // the test environment sets it (AzQtComponentTests.cpp)

        TestMenu flat;
        flat.setStyle(m_style.get());
        flat.resize(60, 40);
        QStyleOptionMenuItem option;
        option.initFrom(&flat);
        QImage image(QSize(60, 40), QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::magenta);
        QPainter painter(&image);
        m_style->drawPrimitive(QStyle::PE_PanelMenu, &option, &painter, &flat);
        painter.end();

        EXPECT_EQ(pixelOf(image, 0, 0), SurfaceColor);
        EXPECT_EQ(pixelOf(image, 59, 39), SurfaceColor);
    }

    // ----------------------------------------------------------------------------------
    // T-MENU.5: the menu bar paints as the legacy sheet style painted it
    // ----------------------------------------------------------------------------------

    TEST_F(MenuElementTests, BarItems_PaintLikeTheLegacySheetInEveryState)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled()); // the test environment sets it (AzQtComponentTests.cpp)

        TestMenuBar legacy;
        legacy.setStyleSheet(QString::fromLatin1(BarSheet));
        fillBar(legacy);
        TestMenuBar flat;
        flat.setStyle(m_style.get());
        fillBar(flat);
        legacy.ensurePolished();
        flat.ensurePolished();

        const QList<QAction*> legacyActions = legacy.actions();
        const QList<QAction*> flatActions = flat.actions();
        ASSERT_EQ(legacyActions.size(), flatActions.size());

        for (const qreal ratio : { 1.0, 1.5 })
        {
            for (int index = 0; index < legacyActions.size(); ++index)
            {
                if (index == 1)
                {
                    continue; // the action without a title is not drawn
                }
                for (const BarItemState state : { BarItemState::Idle, BarItemState::Hovered, BarItemState::Pressed })
                {
                    const QImage expected = paintBarItem(legacy, legacyActions[index], state, ratio);
                    const QImage actual = paintBarItem(flat, flatActions[index], state, ratio);
                    EXPECT_EQ(differingPixels(expected, actual), 0)
                        << "action " << index << " state " << static_cast<int>(state) << " at ratio " << ratio;
                }
            }
        }
    }

    TEST_F(MenuElementTests, BarItem_IsFilledOnlyWhenHoveredOrPressed)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled()); // the test environment sets it (AzQtComponentTests.cpp)

        TestMenuBar flat;
        flat.setStyle(m_style.get());
        fillBar(flat);
        flat.ensurePolished();
        QAction* file = flat.actions().first();
        const QRect rect = flat.actionGeometry(file);

        // MenuBar.qss:28-36: #555555 hovered, #222222 pressed, nothing idle (the bar is transparent over its window)
        EXPECT_EQ(pixelOf(paintBarItem(flat, file, BarItemState::Idle, 1.0), rect.left(), rect.top()), WindowColor);
        EXPECT_EQ(pixelOf(paintBarItem(flat, file, BarItemState::Hovered, 1.0), rect.left(), rect.top()), 0x555555u);
        EXPECT_EQ(pixelOf(paintBarItem(flat, file, BarItemState::Pressed, 1.0), rect.left(), rect.top()), 0x222222u);
        // the fill is the item's 28 rows, not the bar's 30: the border is not under it
        EXPECT_EQ(pixelOf(paintBarItem(flat, file, BarItemState::Hovered, 1.0), rect.left(), rect.bottom()), 0x555555u);
        EXPECT_EQ(pixelOf(paintBarItem(flat, file, BarItemState::Hovered, 1.0), rect.left(), rect.bottom() + 1), WindowColor);
    }

    TEST_F(MenuElementTests, BarBorder_IsPaintedLikeTheLegacySheetUnderItemsAndEmptyAreaAlike)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled()); // the test environment sets it (AzQtComponentTests.cpp)

        TestMenuBar legacy;
        legacy.setStyleSheet(QString::fromLatin1(BarSheet));
        fillBar(legacy);
        TestMenuBar flat;
        flat.setStyle(m_style.get());
        fillBar(flat);
        legacy.ensurePolished();
        flat.ensurePolished();

        for (const qreal ratio : { 1.0, 1.5 })
        {
            QImage expected(deviceSize(legacy.size(), ratio), QImage::Format_ARGB32_Premultiplied);
            expected.setDevicePixelRatio(ratio);
            expected.fill(QColor::fromRgb(WindowColor));
            QImage actual = expected;
            actual.detach();

            // the legacy bar is a styled background: its rule's border is drawn over the whole bar, before the items
            QPainter legacyPainter(&expected);
            QStyleOption legacyOption;
            legacyOption.initFrom(&legacy);
            legacy.style()->drawPrimitive(QStyle::PE_Widget, &legacyOption, &legacyPainter, &legacy);
            legacyPainter.end();

            // the flat bar draws it in the empty area of its paint event, which reaches under the items (they are 28 high)
            QPainter flatPainter(&actual);
            QStyleOptionMenuItem emptyArea;
            emptyArea.menuItemType = QStyleOptionMenuItem::EmptyArea;
            emptyArea.rect = flat.rect();
            flat.style()->drawControl(QStyle::CE_MenuBarEmptyArea, &emptyArea, &flatPainter, &flat);
            flatPainter.end();

            EXPECT_EQ(differingPixels(expected, actual), 0) << "at ratio " << ratio;
        }

        // MenuBar.qss:17-19: 2px #111111 under the bar's 28 rows of content
        QImage image(deviceSize(flat.size(), 1.0), QImage::Format_ARGB32_Premultiplied);
        image.fill(QColor::fromRgb(WindowColor));
        QPainter painter(&image);
        QStyleOptionMenuItem emptyArea;
        emptyArea.rect = flat.rect();
        flat.style()->drawControl(QStyle::CE_MenuBarEmptyArea, &emptyArea, &painter, &flat);
        painter.end();
        EXPECT_EQ(pixelOf(image, 100, 27), WindowColor);
        EXPECT_EQ(pixelOf(image, 100, 28), 0x111111u);
        EXPECT_EQ(pixelOf(image, 100, 29), 0x111111u);
    }

    // ----------------------------------------------------------------------------------
    // T-MENU.6: a widget a QWidgetAction hosts in a menu is filled while it is hovered (UNVERIFIED: no capture shows one)
    // ----------------------------------------------------------------------------------

    TEST_F(MenuElementTests, HostedWidgetHover_FillsLikeTheLegacySheet)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled()); // the test environment sets it (AzQtComponentTests.cpp)

        auto host = [](QMenu& menu)
        {
            QWidget* hosted = new QWidget(&menu);
            hosted->setProperty("class", QStringLiteral("WidgetAction"));
            hosted->setAttribute(Qt::WA_StyledBackground, true);
            hosted->resize(120, 24);
            return hosted;
        };

        TestMenu legacyMenu;
        legacyMenu.setStyleSheet(QString::fromLatin1(PopupSheet));
        QWidget* legacy = host(legacyMenu);
        legacy->ensurePolished();
        TestMenu flatMenu;
        flatMenu.setStyle(m_style.get());
        QWidget* flat = host(flatMenu);
        flatMenu.ensurePolished();
        m_style->polish(flat); // in the GUI the application's style polishes it; here the application's is Fusion
        EXPECT_TRUE(flat->testAttribute(Qt::WA_Hover)); // as the sheet style gave it (qstylesheetstyle.cpp:2894-2905)

        for (const bool hovered : { false, true })
        {
            QImage expected(QSize(120, 24), QImage::Format_ARGB32_Premultiplied);
            expected.fill(QColor::fromRgb(SurfaceColor));
            QImage actual = expected;
            actual.detach();

            QStyleOption legacyOption;
            legacyOption.initFrom(legacy);
            QStyleOption flatOption;
            flatOption.initFrom(flat);
            if (hovered)
            {
                legacyOption.state |= QStyle::State_MouseOver;
                flatOption.state |= QStyle::State_MouseOver;
            }
            QPainter legacyPainter(&expected);
            legacyMenu.style()->drawPrimitive(QStyle::PE_Widget, &legacyOption, &legacyPainter, legacy);
            legacyPainter.end();
            QPainter flatPainter(&actual);
            m_style->drawPrimitive(QStyle::PE_Widget, &flatOption, &flatPainter, flat);
            flatPainter.end();

            EXPECT_EQ(differingPixels(expected, actual), 0) << (hovered ? "hovered" : "idle");
            EXPECT_EQ(pixelOf(actual, 60, 12), hovered ? SelectedColor : SurfaceColor);
        }
    }

    // ----------------------------------------------------------------------------------
    // T-MENU.7: the element's values, and the resolution of a menu's element
    // ----------------------------------------------------------------------------------

    TEST_F(MenuElementTests, ItemPalette_IsWhatTheSheetStyleHandedTheBaseStyle)
    {
        const MenuElement& popup = MenuElement::forWidget(nullptr);
        const QPalette menuPalette;

        // enabled, not selected: white text, and no fill of its own (the menu's shows)
        const QPalette idle = popup.itemPalette(menuPalette, false, true);
        EXPECT_TRUE(idle.color(QPalette::ButtonText) == QColor(0xFF, 0xFF, 0xFF));
        EXPECT_TRUE(idle.color(QPalette::Text) == QColor(0xFF, 0xFF, 0xFF));
        EXPECT_TRUE(idle.color(QPalette::WindowText) == QColor(0xFF, 0xFF, 0xFF));
        EXPECT_TRUE(idle.color(QPalette::Button) == QColor(Qt::transparent));
        EXPECT_TRUE(idle.color(QPalette::Window) == QColor(Qt::transparent));

        // selected: the fill reaches Highlight, Button and Window, and the shades Qt derives from it
        const QPalette selected = popup.itemPalette(menuPalette, true, true);
        EXPECT_TRUE(selected.color(QPalette::Highlight) == QColor(0x44, 0x44, 0x44));
        EXPECT_TRUE(selected.color(QPalette::Button) == QColor(0x44, 0x44, 0x44));
        EXPECT_TRUE(selected.color(QPalette::Light) == QColor(0x44, 0x44, 0x44).lighter(115));
        EXPECT_TRUE(selected.color(QPalette::Dark) == QColor(0x44, 0x44, 0x44).darker(150));
        EXPECT_TRUE(selected.color(QPalette::HighlightedText) == QColor(0xFF, 0xFF, 0xFF));

        // disabled: the :disabled colour reaches every text role
        const QPalette disabled = popup.itemPalette(menuPalette, false, false);
        EXPECT_TRUE(disabled.color(QPalette::ButtonText) == QColor(0x55, 0x55, 0x55));
        EXPECT_TRUE(disabled.color(QPalette::Text) == QColor(0x55, 0x55, 0x55));
        EXPECT_TRUE(disabled.color(QPalette::HighlightedText) == QColor(0x55, 0x55, 0x55));
        // and the sunken panel of a checked item with an icon is shaded from the menu's own surface when not selected
        EXPECT_TRUE(disabled.color(QPalette::Dark) == QColor(0x22, 0x22, 0x22).darker(150));
    }

    TEST_F(MenuElementTests, ModuleElement_WinsOverTheGlobalOneForTheMenusItsMatcherPicks)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled()); // the test environment sets it (AzQtComponentTests.cpp)

        // A module's element, registered once for the process and matched by the object name of its menu
        static MenuElement moduleElement = MenuElement::forWidget(nullptr);
        moduleElement.m_name = "Test.Menu.Module";
        moduleElement.m_surface = QColor(0x10, 0x20, 0x30);
        moduleElement.m_paddingH = 5;
        moduleElement.m_paddingV = 7;
        MenuElement::registerElement(&moduleElement, &QMenu::staticMetaObject,
            [](const QWidget* menu)
            {
                return menu->objectName() == QLatin1String("MenuElementTests.ModuleMenu");
            });

        TestMenu module;
        module.setObjectName(QStringLiteral("MenuElementTests.ModuleMenu"));
        module.setStyle(m_style.get());
        module.ensurePolished();
        TestMenu other;
        other.setStyle(m_style.get());
        other.ensurePolished();

        EXPECT_EQ(&MenuElement::forWidget(&module), &moduleElement);
        EXPECT_EQ(&MenuElement::forWidget(&other), &MenuElement::forWidget(nullptr));
        EXPECT_STREQ(MenuElement::forWidget(nullptr).m_name, "Global.Menu.Popup");

        // metrics and the surface follow the element of the menu
        QStyleOption option;
        EXPECT_EQ(m_style->pixelMetric(QStyle::PM_MenuHMargin, &option, &module), 5);
        EXPECT_EQ(m_style->pixelMetric(QStyle::PM_MenuVMargin, &option, &module), 7);
        EXPECT_EQ(m_style->pixelMetric(QStyle::PM_MenuHMargin, &option, &other), 2);
        EXPECT_EQ(m_style->pixelMetric(QStyle::PM_MenuVMargin, &option, &other), 4);
        // polish gave the widget's own palette the element's surface
        EXPECT_TRUE(module.palette().color(QPalette::Window) == QColor(0x10, 0x20, 0x30));
        EXPECT_TRUE(other.palette().color(QPalette::Window) == QColor(0x22, 0x22, 0x22));
    }
} // namespace AzQtComponents
