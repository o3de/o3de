/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Tests/FieldTwins.h>

#include <AzQtComponents/Components/DockTabBar.h>
#include <AzQtComponents/Components/DockTabWidget.h>
#include <AzQtComponents/Components/Elements/MenuElement.h>
#include <AzQtComponents/Components/Elements/TabElement.h>
#include <AzQtComponents/Components/Widgets/CheckBox.h>
#include <AzQtComponents/Components/Widgets/TabWidget.h>

#include <QAction>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeySequence>
#include <QMenu>
#include <QPixmap>
#include <QStackedWidget>
#include <QStyleOptionMenuItem>
#include <QStyleOptionTab>
#include <QTabBar>
#include <QTabWidget>
#include <QToolBar>
#include <QToolButton>
#include <QWidgetAction>

#include <iterator>
#include <vector>

namespace AzQtComponents
{
    // ----------------------------------------------------------------------------------
    // The bars of the chrome that the flattened GUI carried from these rules of the sheets:
    //
    //   TabWidget.qss:93-97    AzQtComponents--TabBar:focus { outline: none; border: none }
    //   ToolBar.qss:53-57      QToolBar.MainToolBar.IconNormal, QToolBar.MainToolBar.IconNormal QToolButton
    //                          { qproperty-iconSize: 20px 20px }
    //   ToolBar.qss:59-63      QToolBar.IconLarge, QToolBar.IconLarge QToolButton { qproperty-iconSize: 20px 20px }
    //   ToolBar.qss:65-69      QToolBar.MainToolBar.IconLarge, QToolBar.MainToolBar.IconLarge QToolButton
    //                          { qproperty-iconSize: 32px 32px }
    //   Menu.qss:36-40         QMenu.SourceControlMenu { margin-left: 8px; margin-right: 8px }
    //   Menu.qss:42-45         QMenu.SourceControlMenu::item { margin-left: 41px }
    //   and a Secondary DockTabWidget (the graph editors, GraphCanvasEditorCentralWidget.cpp:192), which reaches the rules of
    //   TabWidget.qss:101-162 and TabWidgetActionToolBar.qss:9-12 through the classes of AzQtComponents--TabWidget and --TabBar.
    //
    // The legacy twin of a widget is Qt's own style sheet style over Fusion with the application's real sheet (FieldTwins::appSheet:
    // BaseStyleSheet.qss and the files it imports, the rules above among them, and BaseStyleSheet.qss:15-21 `* { background-color:
    // transparent; color: white }`), set on a root widget the tree is built under, as the application's sheet is on every widget of the
    // real GUI. The flat twin is the flattened Style over Fusion on every widget of the same tree. What the rules decide is compared:
    // the properties (an icon size), the geometry (hints, rects, metrics) and the pixels through the style, onto images of the device
    // ratios 1, 1.5 and 2.
    // ----------------------------------------------------------------------------------

    namespace ChromeBarsFlatSheetTestsInternal
    {
        using namespace FieldTwins;

        const qreal Ratios[] = { 1.0, 1.5, 2.0 };

        class ChromeBarsFlatSheetTests
            : public Fixture
        {
        };

        // ------------------------------------------------------------------------------
        // Reading what a twin drew
        // ------------------------------------------------------------------------------

        //! A grey image of an explicit device ratio the size of a widget, drawn on through the style (QWidget::render would hand the style
        //! the ratio of the screen the test runs on)
        template<typename Draw>
        QImage drawnOn(const QSize& size, qreal ratio, Draw draw)
        {
            QImage image((QSizeF(size) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
            image.setDevicePixelRatio(ratio);
            image.fill(QColor(0x80, 0x80, 0x80));
            QPainter painter(&image);
            draw(painter);
            painter.end();
            return image;
        }

        // ------------------------------------------------------------------------------
        // TabWidget.qss:93-97: the focus of a tab bar
        // ------------------------------------------------------------------------------

        enum class TabLook
        {
            Primary,
            Secondary,
            SecondaryBordered,
            DockTitle,
            DockGroup,
            DockGroupSecondary,
            DockGroupSecondaryBordered,
        };

        struct NamedLook
        {
            const char* m_name;
            TabLook m_look;
            const char* m_element;          //!< the Global element the flat twin has
            bool m_sameInLegacy;            //!< whether the legacy twin of this tree draws the look of the element
        };

        // A bar of a title bar (DockTitle) has the rules of TitleBar.qss only below a TitleBar, which the tree of the test does not build:
        // its legacy twin has the Primary look, so its pixels are not compared, and only the focus rectangle is
        const NamedLook TabLooks[] = {
            { "primary", TabLook::Primary, "Global.Tab.Primary", true },
            { "secondary", TabLook::Secondary, "Global.Tab.Secondary", true },
            { "secondary, bordered", TabLook::SecondaryBordered, "Global.Tab.Secondary.Bordered", true },
            { "dock title", TabLook::DockTitle, "Global.Tab.DockTitle", false },
            { "dock group", TabLook::DockGroup, "Global.Tab.DockGroup", true },
            { "dock group, secondary", TabLook::DockGroupSecondary, "Global.Tab.DockGroup.Secondary", true },
            { "dock group, secondary, bordered", TabLook::DockGroupSecondaryBordered, "Global.Tab.DockGroup.Secondary.Bordered", true },
        };

        bool isDock(TabLook look)
        {
            return look == TabLook::DockGroup || look == TabLook::DockGroupSecondary || look == TabLook::DockGroupSecondaryBordered;
        }

        bool isSecondary(TabLook look)
        {
            return look == TabLook::Secondary || look == TabLook::SecondaryBordered || look == TabLook::DockGroupSecondary
                || look == TabLook::DockGroupSecondaryBordered;
        }

        bool isBordered(TabLook look)
        {
            return look == TabLook::SecondaryBordered || look == TabLook::DockGroupSecondaryBordered;
        }

        const QSize TabsRootSize(420, 140);

        //! A tab widget of one look, as the windows build them (TabWidget::applySecondaryStyle after the tabs); a bar of a title bar alone
        void buildTabs(QWidget& root, TabLook look)
        {
            const char* const titles[] = { "Tab A", "Tab B", "Tab C" };
            if (look == TabLook::DockTitle)
            {
                auto* bar = new DockTabBar(&root);
                bar->setObjectName(QStringLiteral("bar"));
                for (const char* title : titles)
                {
                    bar->addTab(QString::fromLatin1(title));
                }
                bar->setGeometry(5, 5, 400, 32);
                return;
            }

            // The pages are plain widgets: DockTabWidget::addTab(QDockWidget*) hides the base class's
            TabWidget* tabs = isDock(look) ? new DockTabWidget(&root, &root) : new TabWidget(&root);
            tabs->setObjectName(QStringLiteral("tabs"));
            for (const char* title : titles)
            {
                static_cast<QTabWidget*>(tabs)->addTab(new QWidget, QString::fromLatin1(title));
            }
            if (isSecondary(look))
            {
                TabWidget::applySecondaryStyle(tabs, isBordered(look));
            }
            tabs->setGeometry(5, 5, 400, 120);
        }

        QTabBar* barOf(QWidget& root)
        {
            return root.findChild<QTabBar*>();
        }

        //! One tab as QTabBar::paintEvent paints it: the option it builds (QTabBar::initStyleOption), the shape and the label
        QStyleOptionTab tabOption(QTabBar* bar, int index, QStyle::State state)
        {
            QStyleOptionTab option;
            option.initFrom(bar);
            option.state = state | QStyle::State_Enabled | QStyle::State_Active;
            option.rect = bar->tabRect(index);
            option.shape = bar->shape();
            option.text = bar->tabText(index);
            option.row = 0;
            option.position = QStyleOptionTab::Beginning;
            option.selectedPosition = QStyleOptionTab::NotAdjacent;
            option.tabIndex = index;
            return option;
        }

        QImage paintTab(QTabBar* bar, int index, QStyle::State state, qreal ratio)
        {
            return drawnOn(bar->size(), ratio, [&](QPainter& painter)
                {
                    painter.setFont(bar->font());
                    const QStyleOptionTab option = tabOption(bar, index, state);
                    bar->style()->drawControl(QStyle::CE_TabBarTab, &option, &painter, bar);
                });
        }

        // ------------------------------------------------------------------------------
        // ToolBar.qss:53-69: the icon size of the tool buttons below a toolbar
        // ------------------------------------------------------------------------------

        struct ToolBarTier
        {
            const char* m_name;
            const char* m_tags;
            int m_extent;           //!< the rule that names the tool buttons below the toolbar, 0: none does
        };

        const ToolBarTier ToolBarTiers[] = {
            { "no tags", "", 0 },
            { "MainToolBar", "MainToolBar", 0 },
            { "IconNormal", "IconNormal", 16 },                                 // ToolBar.qss:50
            { "MainToolBar.IconNormal", "MainToolBar IconNormal", 20 },         // ToolBar.qss:56
            { "IconLarge", "IconLarge", 20 },                                   // ToolBar.qss:62
            { "MainToolBar.IconLarge", "MainToolBar IconLarge", 32 },           // ToolBar.qss:68
        };

        void tagToolBar(QToolBar* bar, const char* tags)
        {
            for (const QString& tag : QString::fromLatin1(tags).split(QLatin1Char(' '), Qt::SkipEmptyParts))
            {
                Style::addClass(bar, tag);
            }
        }

        QIcon testIcon()
        {
            QPixmap pixmap(48, 48);
            pixmap.fill(QColor(0xC8, 0x3C, 0x3C));
            return QIcon(pixmap);
        }

        //! A toolbar as a window builds one: tagged, given its actions, a tool button added with addWidget and a widget that holds
        //! another, and narrow enough for the last of them to be in the extension at its end (QToolBarLayout)
        void buildToolBar(QWidget& root, const char* tags)
        {
            auto* bar = new QToolBar(&root);
            bar->setObjectName(QStringLiteral("bar"));
            tagToolBar(bar, tags);
            for (int index = 0; index < 6; ++index)
            {
                bar->addAction(testIcon(), QStringLiteral("Action %1").arg(index));
            }
            auto* hosted = new QToolButton;
            hosted->setObjectName(QStringLiteral("hosted"));
            hosted->setIcon(testIcon());
            bar->addWidget(hosted);
            auto* holder = new QWidget;
            holder->setObjectName(QStringLiteral("holder"));
            auto* layout = new QHBoxLayout(holder);
            layout->setContentsMargins(0, 0, 0, 0);
            auto* nested = new QToolButton(holder);
            nested->setObjectName(QStringLiteral("nested"));
            nested->setIcon(testIcon());
            layout->addWidget(nested);
            bar->addWidget(holder);
            bar->setGeometry(0, 0, 120, 48);
        }

        QToolButton* extensionOf(QToolBar* bar)
        {
            return bar->findChild<QToolButton*>(QStringLiteral("qt_toolbar_ext_button"));
        }

        std::string iconSizeText(const QToolButton* button)
        {
            return button ? sizeText(button->iconSize()) : std::string("none");
        }

        // ------------------------------------------------------------------------------
        // Menu.qss:36-45: the popup of the status bar's source control item
        // ------------------------------------------------------------------------------

        //! QMenu::initStyleOption is protected: the option a paint event builds for an action
        class TestMenu
            : public QMenu
        {
        public:
            using QMenu::QMenu;
            using QMenu::initStyleOption;
        };

        //! The menu SourceControlItem::InitMenu builds (MainStatusBar.cpp:344-354), less the connections: a settings action, an Enable
        //! check box in a widget action (checkable) and the tag of the rules, given last
        void buildSourceControlMenu(QMenu& menu)
        {
            menu.addAction(QStringLiteral("Settings"));
            auto* checkBox = new QCheckBox(&menu);
            checkBox->setObjectName(QStringLiteral("enable"));
            checkBox->setText(QStringLiteral("Enable"));
            CheckBox::applyToggleSwitchStyle(checkBox);
            auto* enableAction = new QWidgetAction(&menu);
            enableAction->setDefaultWidget(checkBox);
            menu.addAction(enableAction);
            Style::addClass(&menu, QStringLiteral("SourceControlMenu"));
            enableAction->setCheckable(true);
        }

        //! The same menu with text items only: what the element carries of the items (text, a shortcut, disabled, selected)
        void buildTextMenu(QMenu& menu)
        {
            menu.addAction(QStringLiteral("Settings"));
            menu.addAction(QStringLiteral("Open &Project"))->setShortcut(QKeySequence(QStringLiteral("Ctrl+O")));
            menu.addAction(QStringLiteral("Disabled item"))->setEnabled(false);
            menu.addSeparator();
            menu.addAction(QStringLiteral("A longer item with more words"));
            Style::addClass(&menu, QStringLiteral("SourceControlMenu"));
        }

        //! The same with a checkable action of text among them: the menu has a check column then (QMenu::initStyleOption,
        //! menuHasCheckableItems), which the widget action of the real menu does not make. That action is the base style's to draw.
        void buildCheckableTextMenu(QMenu& menu)
        {
            menu.addAction(QStringLiteral("Settings"));
            menu.addAction(QStringLiteral("Open &Project"))->setShortcut(QKeySequence(QStringLiteral("Ctrl+O")));
            menu.addAction(QStringLiteral("Auto refresh"))->setCheckable(true);
            Style::addClass(&menu, QStringLiteral("SourceControlMenu"));
        }

        //! The menu of the legacy GUI: below a root that carries the application's sheet
        struct LegacyMenu
        {
            explicit LegacyMenu(void (*build)(QMenu&))
            {
                m_menu = new TestMenu(&m_root);
                build(*m_menu);
                Fixture::legacy(m_root);
                m_menu->ensurePolished();
            }

            QWidget m_root;
            TestMenu* m_menu = nullptr;
        };

        //! The menu of the flattened GUI: the style on it and on everything below it
        struct FlatMenu
        {
            FlatMenu(const Fixture& fixture, void (*build)(QMenu&))
            {
                m_menu = new TestMenu(&m_root);
                build(*m_menu);
                fixture.flat(m_root);
                m_menu->ensurePolished();
            }

            QWidget m_root;
            TestMenu* m_menu = nullptr;
        };

        //! The menu as its paint event draws it, onto an image of an explicit device ratio: its window background, the panel and each item
        //! (the action the pointer is on selected). A widget action's widget is not shown while the menu is not.
        QImage paintMenu(TestMenu& menu, QAction* selected, qreal ratio)
        {
            menu.setActiveAction(selected);
            menu.resize(menu.sizeHint());
            QImage image((QSizeF(menu.size()) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
            image.setDevicePixelRatio(ratio);
            image.fill(QColor(0x80, 0x80, 0x80));
            menu.render(&image, QPoint(), QRegion(), QWidget::DrawWindowBackground | QWidget::DrawChildren);
            return image;
        }

        //! The option of an action as the paint event builds it: after the menu laid its actions out, which is what makes it know whether
        //! it has a checkable one (QMenuPrivate::updateActionRects)
        QStyleOptionMenuItem itemOption(TestMenu& menu, QAction* action)
        {
            const QRect rect = menu.actionGeometry(action);
            QStyleOptionMenuItem option;
            menu.initStyleOption(&option, action);
            option.rect = rect;
            return option;
        }

        // ==============================================================================
        // TabWidget.qss:93-97
        // ==============================================================================

        TEST_F(ChromeBarsFlatSheetTests, TabFocus_EveryLookOfAnAzQtComponentsBarHidesTheFocusRectangle)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            for (const NamedLook& look : TabLooks)
            {
                QWidget root;
                buildTabs(root, look.m_look);
                flat(root);
                show(root, TabsRootSize);

                QTabBar* bar = barOf(root);
                ASSERT_TRUE(bar != nullptr) << look.m_name;
                const TabElement* element = TabElement::forTabBar(bar);
                ASSERT_TRUE(element != nullptr) << look.m_name;
                EXPECT_STREQ(element->m_name, look.m_element) << look.m_name;
                EXPECT_TRUE(element->m_hideFocusRect) << look.m_name;
            }
        }

        TEST_F(ChromeBarsFlatSheetTests, TabFocus_ABarWithTheKeyboardFocusDrawsItsCurrentTabAsAnUnfocusedBarDoes)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            // The focus of a bar by the keyboard: Tab key navigation sets the keyboard focus change attribute of the window, which
            // QStyleOption::initFrom reads (State_KeyboardFocusChange), and the current tab of the bar that has the focus carries
            // State_HasFocus (QTabBarPrivate::initBasicStyleOption). Fusion draws its focus rectangle for both.
            const QStyle::State selected = QStyle::State_Selected;
            const QStyle::State focused = QStyle::State_Selected | QStyle::State_HasFocus | QStyle::State_KeyboardFocusChange;

            for (const NamedLook& look : TabLooks)
            {
                SCOPED_TRACE(look.m_name);

                QWidget legacyRoot;
                buildTabs(legacyRoot, look.m_look);
                legacy(legacyRoot);
                show(legacyRoot, TabsRootSize);

                // The same tree without TabWidget.qss:95: the sheet style over Fusion draws Fusion's rectangle there, which proves that
                // the tab below is one that would show it
                QWidget controlRoot;
                buildTabs(controlRoot, look.m_look);
                legacy(controlRoot, { { "outline: none;", "" } });
                show(controlRoot, TabsRootSize);

                QWidget flatRoot;
                buildTabs(flatRoot, look.m_look);
                flat(flatRoot);
                show(flatRoot, TabsRootSize);

                QTabBar* legacyBar = barOf(legacyRoot);
                QTabBar* controlBar = barOf(controlRoot);
                QTabBar* flatBar = barOf(flatRoot);
                ASSERT_TRUE(legacyBar != nullptr && controlBar != nullptr && flatBar != nullptr);
                if (look.m_sameInLegacy)
                {
                    ASSERT_EQ(legacyBar->tabRect(0), flatBar->tabRect(0));
                }

                for (const qreal ratio : Ratios)
                {
                    const QImage legacyIdle = paintTab(legacyBar, 0, selected, ratio);
                    const QImage legacyFocused = paintTab(legacyBar, 0, focused, ratio);
                    const QImage controlIdle = paintTab(controlBar, 0, selected, ratio);
                    const QImage controlFocused = paintTab(controlBar, 0, focused, ratio);
                    const QImage flatIdle = paintTab(flatBar, 0, selected, ratio);
                    const QImage flatFocused = paintTab(flatBar, 0, focused, ratio);

                    EXPECT_TRUE(isDrawnOn(legacyIdle)) << "ratio " << ratio;
                    EXPECT_EQ(differing(legacyFocused, legacyIdle), 0) << "the sheet draws no focus rectangle, ratio " << ratio;
                    EXPECT_GT(differing(controlFocused, controlIdle), 0) << "without the rule Fusion draws one, ratio " << ratio;
                    EXPECT_EQ(differing(flatFocused, flatIdle), 0) << "ratio " << ratio << ": " << firstDifference(flatFocused, flatIdle);
                    if (look.m_sameInLegacy)
                    {
                        EXPECT_EQ(differing(flatFocused, legacyFocused), 0)
                            << "ratio " << ratio << ": " << firstDifference(flatFocused, legacyFocused);
                    }
                }
            }
        }

        //! The bars of a tab widget the module rows of the focus test match by the name of the tab widget, as the graph tabs of Script
        //! Canvas are matched (Selectors::childOf, the name given before the style classes re-polish the bar)
        bool isHidingFocusTabBar(const QWidget* bar)
        {
            return Selectors::childOf(bar, "ChromeBarsFocusHidingTabs");
        }

        bool isShowingFocusTabBar(const QWidget* bar)
        {
            return Selectors::childOf(bar, "ChromeBarsFocusShowingTabs");
        }

        TEST_F(ChromeBarsFlatSheetTests, TabFocus_ARowAModuleRegistersHidesTheFocusRectangleWhereItSaysSo)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            // Rows of a module, as Script Canvas registers its graph tabs (MainWindow.cpp GraphTabs, a copy of the secondary look held at
            // another height): the look of the Global secondary row, with the focus rectangle hidden by one of them and not by the other
            static TabElement hidingRow;
            static TabElement showingRow;
            static bool registered = false;
            if (!registered)
            {
                QWidget probeRoot;
                buildTabs(probeRoot, TabLook::Secondary);
                flat(probeRoot);
                show(probeRoot, TabsRootSize);
                const TabElement* global = TabElement::forTabBar(barOf(probeRoot));
                ASSERT_TRUE(global != nullptr);
                hidingRow = *global;
                hidingRow.m_name = "ChromeBars.Tab.FocusHiding";
                hidingRow.m_hideFocusRect = true;
                showingRow = *global;
                showingRow.m_name = "ChromeBars.Tab.FocusShowing";
                showingRow.m_hideFocusRect = false;
                TabElement::registerElement(&hidingRow, &QTabBar::staticMetaObject, isHidingFocusTabBar);
                TabElement::registerElement(&showingRow, &QTabBar::staticMetaObject, isShowingFocusTabBar);
                registered = true;
            }

            const QStyle::State selected = QStyle::State_Selected;
            const QStyle::State focused = QStyle::State_Selected | QStyle::State_HasFocus | QStyle::State_KeyboardFocusChange;
            const struct
            {
                const char* m_name;
                const char* m_row;
                bool m_hidden;
            } cases[] = {
                { "ChromeBarsFocusHidingTabs", "ChromeBars.Tab.FocusHiding", true },
                { "ChromeBarsFocusShowingTabs", "ChromeBars.Tab.FocusShowing", false },
            };
            for (const auto& testCase : cases)
            {
                SCOPED_TRACE(testCase.m_name);
                QWidget root;
                auto* tabs = new TabWidget(&root);
                tabs->setObjectName(QString::fromLatin1(testCase.m_name));
                for (const char* title : { "Tab A", "Tab B", "Tab C" })
                {
                    static_cast<QTabWidget*>(tabs)->addTab(new QWidget, QString::fromLatin1(title));
                }
                TabWidget::applySecondaryStyle(tabs, false);
                tabs->setGeometry(5, 5, 400, 120);
                flat(root);
                show(root, TabsRootSize);

                QTabBar* bar = barOf(root);
                ASSERT_TRUE(bar != nullptr);
                const TabElement* element = TabElement::forTabBar(bar);
                ASSERT_TRUE(element != nullptr);
                EXPECT_STREQ(element->m_name, testCase.m_row);
                for (const qreal ratio : Ratios)
                {
                    const QImage idle = paintTab(bar, 0, selected, ratio);
                    const QImage withFocus = paintTab(bar, 0, focused, ratio);
                    EXPECT_TRUE(isDrawnOn(idle)) << "ratio " << ratio;
                    if (testCase.m_hidden)
                    {
                        EXPECT_EQ(differing(withFocus, idle), 0) << "ratio " << ratio << ": " << firstDifference(withFocus, idle);
                    }
                    else
                    {
                        EXPECT_GT(differing(withFocus, idle), 0) << "the base style draws its rectangle, ratio " << ratio;
                    }
                }
            }
        }

        //! The bar as its paint event draws it (its own paint event, the tabs through the style) onto an image of an explicit ratio
        QImage paintBar(QTabBar* bar, qreal ratio)
        {
            QImage image((QSizeF(bar->size()) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
            image.setDevicePixelRatio(ratio);
            image.fill(QColor(0x80, 0x80, 0x80));
            bar->render(&image, QPoint(), QRegion(), QWidget::DrawChildren);
            return image;
        }

        //! The bar as the Tab key focuses it: the window active (a window that is not on the screen is made so by the application), the
        //! keyboard focus change attribute of the window that Tab key navigation sets, the bar given the focus. Its image at each ratio,
        //! empty when the bar did not get the focus (only the active window has one, so each bar is painted while it has it).
        std::vector<QImage> paintFocusedBar(QWidget& root, QTabBar* bar)
        {
            // QApplication::setActiveWindow is deprecated for QWidget::activateWindow, which a window that is not on the screen ignores
            QT_WARNING_PUSH
            QT_WARNING_DISABLE_DEPRECATED
            QApplication::setActiveWindow(&root);
            QT_WARNING_POP
            bar->setFocus(Qt::TabFocusReason);
            root.setAttribute(Qt::WA_KeyboardFocusChange, true);     // QWidget::focusNextPrevChild sets it as the Tab key moves the focus
            QApplication::processEvents();
            std::vector<QImage> images;
            if (bar->hasFocus())
            {
                for (const qreal ratio : Ratios)
                {
                    images.push_back(paintBar(bar, ratio));
                }
            }
            return images;
        }

        TEST_F(ChromeBarsFlatSheetTests, TabFocus_ABarThatTheTabKeyFocusedPaintsAsTheLegacyBarDid)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            // The rule as a whole, and the rule less its `border: none`
            const Replacements withoutRule = { { "AzQtComponents--TabBar:focus\n{\n    outline: none;\n    border: none;\n}", "" } };
            const Replacements withoutBorder = { { "    outline: none;\n    border: none;\n}", "    outline: none;\n}" } };

            for (const NamedLook& look : TabLooks)
            {
                if (!look.m_sameInLegacy)
                {
                    continue;
                }
                SCOPED_TRACE(look.m_name);

                QWidget legacyRoot;
                buildTabs(legacyRoot, look.m_look);
                legacy(legacyRoot);
                show(legacyRoot, TabsRootSize);
                QWidget noRuleRoot;
                buildTabs(noRuleRoot, look.m_look);
                legacy(noRuleRoot, withoutRule);
                show(noRuleRoot, TabsRootSize);
                QWidget noBorderRoot;
                buildTabs(noBorderRoot, look.m_look);
                legacy(noBorderRoot, withoutBorder);
                show(noBorderRoot, TabsRootSize);
                QWidget flatRoot;
                buildTabs(flatRoot, look.m_look);
                flat(flatRoot);
                show(flatRoot, TabsRootSize);

                const std::vector<QImage> legacyFocused = paintFocusedBar(legacyRoot, barOf(legacyRoot));
                const std::vector<QImage> noRuleFocused = paintFocusedBar(noRuleRoot, barOf(noRuleRoot));
                const std::vector<QImage> noBorderFocused = paintFocusedBar(noBorderRoot, barOf(noBorderRoot));
                const std::vector<QImage> flatFocused = paintFocusedBar(flatRoot, barOf(flatRoot));
                ASSERT_EQ(legacyFocused.size(), std::size(Ratios)) << "the window could not be made active, offscreen";
                ASSERT_EQ(noRuleFocused.size(), std::size(Ratios));
                ASSERT_EQ(noBorderFocused.size(), std::size(Ratios));
                ASSERT_EQ(flatFocused.size(), std::size(Ratios));

                for (size_t index = 0; index < std::size(Ratios); ++index)
                {
                    const qreal ratio = Ratios[index];
                    // Without the rule the bar draws Fusion's rectangle on its current tab
                    EXPECT_GT(differing(noRuleFocused[index], legacyFocused[index]), 0) << "ratio " << ratio;
                    // The border of the rule changes nothing that is drawn: with the outline gone, so is the rectangle it moved
                    EXPECT_EQ(differing(noBorderFocused[index], legacyFocused[index]), 0)
                        << "ratio " << ratio << ": " << firstDifference(noBorderFocused[index], legacyFocused[index]);
                    // With the rule the bar draws what the flat one does
                    EXPECT_EQ(differing(flatFocused[index], legacyFocused[index]), 0)
                        << "ratio " << ratio << ": " << firstDifference(flatFocused[index], legacyFocused[index]);
                }
            }
        }

        // ==============================================================================
        // A secondary DockTabWidget
        // ==============================================================================

        //! What the layout of a tab widget decided, for the comparison of two twins
        struct TabsFacts
        {
            QRect m_bar;
            QSize m_barHint;
            QSize m_barMinimumHint;
            QList<QRect> m_tabs;
            QRect m_stack;
            QRect m_corner;
            QSize m_cornerMinimumHint;
        };

        TabsFacts factsOf(QWidget& root)
        {
            TabsFacts facts;
            auto* tabs = root.findChild<QTabWidget*>();
            QTabBar* bar = barOf(root);
            facts.m_bar = bar->geometry();
            facts.m_barHint = bar->sizeHint();
            facts.m_barMinimumHint = bar->minimumSizeHint();
            for (int index = 0; index < bar->count(); ++index)
            {
                facts.m_tabs.append(bar->tabRect(index));
            }
            facts.m_stack = tabs->findChild<QStackedWidget*>()->geometry();
            if (QWidget* corner = tabs->cornerWidget(Qt::TopRightCorner))
            {
                facts.m_corner = corner->geometry();
                facts.m_cornerMinimumHint = corner->minimumSizeHint();
            }
            return facts;
        }

        void expectSameFacts(const TabsFacts& actual, const TabsFacts& expected, const std::string& what, bool compareStack)
        {
            EXPECT_EQ(actual.m_bar, expected.m_bar)
                << what << ", bar " << rectText(actual.m_bar) << " against " << rectText(expected.m_bar);
            EXPECT_EQ(actual.m_barHint, expected.m_barHint) << what << ", bar hint";
            EXPECT_EQ(actual.m_barMinimumHint, expected.m_barMinimumHint) << what << ", bar minimum hint";
            ASSERT_EQ(actual.m_tabs.size(), expected.m_tabs.size()) << what;
            for (int index = 0; index < actual.m_tabs.size(); ++index)
            {
                EXPECT_EQ(actual.m_tabs[index], expected.m_tabs[index]) << what << ", tab " << index << " "
                    << rectText(actual.m_tabs[index]) << " against " << rectText(expected.m_tabs[index]);
            }
            if (compareStack)
            {
                EXPECT_EQ(actual.m_stack, expected.m_stack)
                    << what << ", pages " << rectText(actual.m_stack) << " against " << rectText(expected.m_stack);
            }
            EXPECT_EQ(actual.m_corner, expected.m_corner)
                << what << ", corner " << rectText(actual.m_corner) << " against " << rectText(expected.m_corner);
            EXPECT_EQ(actual.m_cornerMinimumHint, expected.m_cornerMinimumHint) << what << ", corner minimum hint";
        }

        //! The tabs widget as its parent's paint draws it (its own paint event and its children's), onto an image of an explicit ratio
        QImage paintTabs(QWidget& root, qreal ratio)
        {
            auto* tabs = root.findChild<QTabWidget*>();
            QImage image((QSizeF(tabs->size()) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
            image.setDevicePixelRatio(ratio);
            image.fill(QColor(0x80, 0x80, 0x80));
            tabs->render(&image, QPoint(), QRegion(), QWidget::DrawChildren);
            return image;
        }

        TEST_F(ChromeBarsFlatSheetTests, DockSecondary_TheBarOfASecondaryDockTabWidgetHasTheSecondaryLookWithTheDockClose)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            const auto elementOf = [this](TabLook look)
            {
                QWidget root;
                buildTabs(root, look);
                flat(root);
                show(root, TabsRootSize);
                return TabElement::forTabBar(barOf(root));
            };

            // Not a copy of the TabBar looks' close button: the close button of a dock tab is a DockBarButton (DockTabBar::tabInserted)
            // that only the hover fill of the element reaches, as DockGroup has it
            const TabElement* plainDock = elementOf(TabLook::DockGroup);
            const struct
            {
                TabLook m_dock;
                TabLook m_bar;
            } pairs[] = { { TabLook::DockGroupSecondary, TabLook::Secondary },
                { TabLook::DockGroupSecondaryBordered, TabLook::SecondaryBordered } };
            for (const auto& pair : pairs)
            {
                const TabElement* dock = elementOf(pair.m_dock);
                const TabElement* bar = elementOf(pair.m_bar);
                ASSERT_TRUE(dock != nullptr && bar != nullptr);

                EXPECT_TRUE(dock->m_barFill == bar->m_barFill);
                EXPECT_TRUE(dock->m_barEdges.m_top.m_width == bar->m_barEdges.m_top.m_width
                    && dock->m_barEdges.m_left.m_width == bar->m_barEdges.m_left.m_width
                    && dock->m_barEdges.m_right.m_width == bar->m_barEdges.m_right.m_width
                    && dock->m_barEdges.m_bottom.m_width == bar->m_barEdges.m_bottom.m_width);
                EXPECT_EQ(dock->m_barMinHeight, bar->m_barMinHeight);
                EXPECT_TRUE(dock->m_idle.m_fill == bar->m_idle.m_fill && dock->m_idle.m_text == bar->m_idle.m_text);
                EXPECT_TRUE(dock->m_hover.m_fill == bar->m_hover.m_fill && dock->m_hover.m_text == bar->m_hover.m_text);
                EXPECT_TRUE(dock->m_selected.m_fill == bar->m_selected.m_fill && dock->m_selected.m_text == bar->m_selected.m_text);
                EXPECT_TRUE(dock->m_padding == bar->m_padding);
                EXPECT_EQ(dock->m_maxContentWidth, bar->m_maxContentWidth);
                EXPECT_TRUE(dock->m_widgetFill == bar->m_widgetFill);
                EXPECT_TRUE(dock->m_widgetEdges.m_top.m_width == bar->m_widgetEdges.m_top.m_width);
                EXPECT_TRUE(dock->m_paneEdges.m_top.m_width == bar->m_paneEdges.m_top.m_width);
                EXPECT_TRUE(dock->m_panePadding == bar->m_panePadding);
                EXPECT_EQ(dock->m_barLeft, bar->m_barLeft);
                EXPECT_EQ(dock->m_barAreaMinHeight, bar->m_barAreaMinHeight);
                EXPECT_EQ(dock->m_cornerMinHeight, bar->m_cornerMinHeight);
                EXPECT_EQ(dock->m_cornerMarginBottom, bar->m_cornerMarginBottom);

                EXPECT_STREQ(dock->m_closeIcon, "");
                EXPECT_TRUE(dock->m_closeHoverFill == plainDock->m_closeHoverFill);
                EXPECT_EQ(dock->m_closeMarginRight, 0);
            }
        }

        TEST_F(ChromeBarsFlatSheetTests, DockSecondary_ThePlainDockTabWidgetKeepsTheDockGroupLook)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            // Only the tags of TabWidget::applySecondaryStyle select the secondary looks: a dock bar with none, and the bar of a title
            // bar with the same tags (no tab widget around it), keep theirs
            QWidget plainRoot;
            buildTabs(plainRoot, TabLook::DockGroup);
            flat(plainRoot);
            show(plainRoot, TabsRootSize);
            EXPECT_STREQ(TabElement::forTabBar(barOf(plainRoot))->m_name, "Global.Tab.DockGroup");

            QWidget titleRoot;
            buildTabs(titleRoot, TabLook::DockTitle);
            flat(titleRoot);
            show(titleRoot, TabsRootSize);
            auto* titleBar = barOf(titleRoot);
            Style::addClass(titleBar, QStringLiteral("Secondary"));
            Style::addClass(titleBar, QStringLiteral("Bordered"));
            EXPECT_STREQ(TabElement::forTabBar(titleBar)->m_name, "Global.Tab.DockTitle");
        }

        TEST_F(ChromeBarsFlatSheetTests, DockSecondary_TheLayoutOfTheTabWidgetIsTheLegacyOne)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            for (const NamedLook& look : TabLooks)
            {
                if (!isDock(look.m_look))
                {
                    continue;
                }
                SCOPED_TRACE(look.m_name);

                QWidget legacyRoot;
                buildTabs(legacyRoot, look.m_look);
                legacy(legacyRoot);
                show(legacyRoot, TabsRootSize);
                QWidget flatRoot;
                buildTabs(flatRoot, look.m_look);
                flat(flatRoot);
                show(flatRoot, TabsRootSize);

                // Where the pages start under a secondary look is not compared: the sheet style over Fusion puts the pane under the
                // bar's own height there, for a plain TabWidget as for a DockTabWidget, and the element under the bar's slot
                expectSameFacts(factsOf(flatRoot), factsOf(legacyRoot), look.m_name, !isSecondary(look.m_look));
            }
        }

        TEST_F(ChromeBarsFlatSheetTests, DockSecondary_TheTabsAndThePaneAreDrawnAsTheLegacyOnesAre)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            for (const NamedLook& look : TabLooks)
            {
                if (!isDock(look.m_look))
                {
                    continue;
                }
                SCOPED_TRACE(look.m_name);

                QWidget legacyRoot;
                buildTabs(legacyRoot, look.m_look);
                legacy(legacyRoot);
                show(legacyRoot, TabsRootSize);
                QWidget flatRoot;
                buildTabs(flatRoot, look.m_look);
                flat(flatRoot);
                show(flatRoot, TabsRootSize);

                QTabBar* legacyBar = barOf(legacyRoot);
                QTabBar* flatBar = barOf(flatRoot);
                const std::pair<const char*, QStyle::State> states[] = {
                    { "idle", QStyle::State() },
                    { "hover", QStyle::State(QStyle::State_MouseOver) },
                    { "selected", QStyle::State(QStyle::State_Selected) },
                    { "selected over", QStyle::State(QStyle::State_Selected | QStyle::State_MouseOver) },
                };
                for (const auto& state : states)
                {
                    for (const qreal ratio : Ratios)
                    {
                        // The shape of every tab, at the rect it has in the bar
                        for (int index = 0; index < legacyBar->count(); ++index)
                        {
                            const QImage legacyImage = drawnOn(legacyBar->size(), ratio, [&](QPainter& painter)
                                {
                                    const QStyleOptionTab option = tabOption(legacyBar, index, state.second);
                                    legacyBar->style()->drawControl(QStyle::CE_TabBarTabShape, &option, &painter, legacyBar);
                                });
                            const QImage flatImage = drawnOn(flatBar->size(), ratio, [&](QPainter& painter)
                                {
                                    const QStyleOptionTab option = tabOption(flatBar, index, state.second);
                                    flatBar->style()->drawControl(QStyle::CE_TabBarTabShape, &option, &painter, flatBar);
                                });
                            EXPECT_EQ(differing(flatImage, legacyImage), 0) << state.first << ", tab " << index << ", ratio " << ratio
                                                                            << ": " << firstDifference(flatImage, legacyImage);
                        }
                    }
                }

                // The whole widget: the bar with its edges and its tabs, the pane, the widget's border and the corner. Of a secondary look
                // not the pane's top (see the layout test): the strip of the tabs, and the border down both sides and along the bottom
                // below where either twin puts the pane's top line
                const QRect widgetRect(QPoint(0, 0), legacyRoot.findChild<QTabWidget*>()->size());
                // the strip of the tabs, down to a row clear of the pane's top line that the twin draws under the bar's own height
                const int legacyPaneTop = legacyRoot.findChild<QTabWidget*>()->findChild<QStackedWidget*>()->geometry().top() - 2;
                const int stripHeight = qMin(legacyBar->tabRect(0).height(), legacyPaneTop - 1);
                const int below = 40;
                const QRect regions[] = { QRect(0, 0, widgetRect.width(), stripHeight), QRect(0, below, 1, widgetRect.height() - below),
                    QRect(widgetRect.width() - 1, below, 1, widgetRect.height() - below),
                    QRect(0, widgetRect.height() - 1, widgetRect.width(), 1) };
                for (const qreal ratio : Ratios)
                {
                    const QImage legacyImage = paintTabs(legacyRoot, ratio);
                    const QImage flatImage = paintTabs(flatRoot, ratio);
                    EXPECT_TRUE(isDrawnOn(legacyImage));
                    if (!isSecondary(look.m_look))
                    {
                        EXPECT_EQ(differing(flatImage, legacyImage), 0)
                            << "ratio " << ratio << ": " << firstDifference(flatImage, legacyImage);
                        continue;
                    }
                    for (const QRect& region : regions)
                    {
                        const QRect device(QPointF(region.topLeft() * ratio).toPoint(), (QSizeF(region.size()) * ratio).toSize());
                        const QImage legacyPart = legacyImage.copy(device);
                        const QImage flatPart = flatImage.copy(device);
                        EXPECT_EQ(differing(flatPart, legacyPart), 0)
                            << "ratio " << ratio << ", region " << rectText(region) << ": " << firstDifference(flatPart, legacyPart);
                    }
                }
            }
        }

        // ==============================================================================
        // ToolBar.qss:53-69
        // ==============================================================================

        TEST_F(ChromeBarsFlatSheetTests, ToolBarButtons_TheTierOfAToolBarIsTheIconSizeOfTheToolButtonsBelowIt)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            for (const ToolBarTier& tier : ToolBarTiers)
            {
                SCOPED_TRACE(tier.m_name);
                Twins twins(
                    *this, [&](QWidget& root, bool) { buildToolBar(root, tier.m_tags); }, QSize(130, 60));

                auto* legacyBar = twins.legacyChild<QToolBar>("bar");
                auto* flatBar = twins.flatChild<QToolBar>("bar");
                ASSERT_TRUE(legacyBar != nullptr && flatBar != nullptr);
                ASSERT_TRUE(extensionOf(legacyBar) != nullptr && extensionOf(flatBar) != nullptr);

                const struct
                {
                    const char* m_what;
                    QToolButton* m_legacy;
                    QToolButton* m_flat;
                } buttons[] = {
                    { "added with addWidget", twins.legacyChild<QToolButton>("hosted"), twins.flatChild<QToolButton>("hosted") },
                    { "in a widget added with addWidget", twins.legacyChild<QToolButton>("nested"),
                        twins.flatChild<QToolButton>("nested") },
                    { "the extension", extensionOf(legacyBar), extensionOf(flatBar) },
                };
                for (const auto& button : buttons)
                {
                    ASSERT_TRUE(button.m_legacy != nullptr && button.m_flat != nullptr) << button.m_what;
                    button.m_legacy->ensurePolished();
                    button.m_flat->ensurePolished();
                    // The rule's size where one names the button, else the button's own (the metric the base style gives a tool button)
                    if (tier.m_extent > 0)
                    {
                        EXPECT_EQ(button.m_legacy->iconSize(), QSize(tier.m_extent, tier.m_extent)) << button.m_what;
                    }
                    EXPECT_EQ(button.m_flat->iconSize(), button.m_legacy->iconSize())
                        << button.m_what << ": " << iconSizeText(button.m_flat) << " against " << iconSizeText(button.m_legacy);
                }

                // The toolbar and the buttons it made for its actions have the size of the tier already
                EXPECT_EQ(flatBar->iconSize(), legacyBar->iconSize());
                const QList<QToolButton*> legacyActions = legacyBar->findChildren<QToolButton*>(QString(), Qt::FindDirectChildrenOnly);
                const QList<QToolButton*> flatActions = flatBar->findChildren<QToolButton*>(QString(), Qt::FindDirectChildrenOnly);
                ASSERT_EQ(legacyActions.size(), flatActions.size());
                for (int index = 0; index < legacyActions.size(); ++index)
                {
                    legacyActions[index]->ensurePolished();
                    flatActions[index]->ensurePolished();
                    EXPECT_EQ(flatActions[index]->iconSize(), legacyActions[index]->iconSize()) << "action button " << index;
                }
            }
        }

        TEST_F(ChromeBarsFlatSheetTests, ToolBarButtons_AButtonPolishedBeforeTheTagHoldsItsSizeAndOneAfterIsGivenTheTiers)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            // The sheet style sets qproperty-iconSize as a widget is polished, and a class tag polishes the toolbar again, not the
            // widgets below it: a button polished before the toolbar was tagged (the toolbars of the main window are tagged after
            // they are built, MainWindow::AdjustToolBarIconSize) keeps the size it had, and one polished after has the tier's
            Twins twins(
                *this, [&](QWidget& root, bool) { buildToolBar(root, ""); }, QSize(130, 60));
            auto* legacyBar = twins.legacyChild<QToolBar>("bar");
            auto* flatBar = twins.flatChild<QToolBar>("bar");
            auto* legacyEarly = twins.legacyChild<QToolButton>("hosted");
            auto* flatEarly = twins.flatChild<QToolButton>("hosted");
            ASSERT_TRUE(legacyBar != nullptr && flatBar != nullptr && legacyEarly != nullptr && flatEarly != nullptr);
            legacyEarly->ensurePolished();
            flatEarly->ensurePolished();
            const QSize before = legacyEarly->iconSize();
            EXPECT_EQ(flatEarly->iconSize(), before);

            Style::addClass(legacyBar, QStringLiteral("MainToolBar"));
            Style::addClass(flatBar, QStringLiteral("MainToolBar"));
            Style::addClass(legacyBar, QStringLiteral("IconLarge"));
            Style::addClass(flatBar, QStringLiteral("IconLarge"));
            QApplication::processEvents();

            EXPECT_EQ(legacyEarly->iconSize(), before);
            EXPECT_EQ(flatEarly->iconSize(), before);

            auto* legacyLate = new QToolButton;
            legacyLate->setIcon(testIcon());
            legacyBar->addWidget(legacyLate);
            auto* flatLate = new QToolButton;
            flatLate->setIcon(testIcon());
            flatLate->setStyle(flatBar->style());
            flatBar->addWidget(flatLate);
            legacyLate->ensurePolished();
            flatLate->ensurePolished();
            EXPECT_EQ(legacyLate->iconSize(), QSize(32, 32));                       // ToolBar.qss:68
            EXPECT_EQ(flatLate->iconSize(), legacyLate->iconSize());
        }

        // ==============================================================================
        // Menu.qss:36-45
        // ==============================================================================

        TEST_F(ChromeBarsFlatSheetTests, SourceControlMenu_OnlyAMenuWithTheTagHasTheElement)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            FlatMenu tagged(*this, buildSourceControlMenu);
            EXPECT_STREQ(MenuElement::forWidget(tagged.m_menu).m_name, "Global.Menu.SourceControlPopup");
            EXPECT_EQ(MenuElement::forWidget(tagged.m_menu).panelWidth(), 8);                   // Menu.qss:38-39
            EXPECT_EQ(MenuElement::forWidget(tagged.m_menu).m_itemMargin.left(), 41);           // Menu.qss:44

            TestMenu plain;
            plain.addAction(QStringLiteral("Plain"));
            flat(plain);
            plain.ensurePolished();
            EXPECT_STREQ(MenuElement::forWidget(&plain).m_name, "Global.Menu.Popup");
            EXPECT_EQ(MenuElement::forWidget(&plain).panelWidth(), 0);
            EXPECT_TRUE(MenuElement::forWidget(&plain).m_itemMargin.isNull());
        }

        TEST_F(ChromeBarsFlatSheetTests, SourceControlMenu_TheMenuIsLaidOutAsTheLegacyMenuIs)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            for (void (*build)(QMenu&) : { &buildSourceControlMenu, &buildTextMenu })
            {
                LegacyMenu legacyMenu(build);
                FlatMenu flatMenu(*this, build);
                TestMenu& expected = *legacyMenu.m_menu;
                TestMenu& actual = *flatMenu.m_menu;

                EXPECT_EQ(actual.sizeHint(), expected.sizeHint())
                    << sizeText(actual.sizeHint()) << " against " << sizeText(expected.sizeHint());
                EXPECT_EQ(actual.minimumSizeHint(), expected.minimumSizeHint());
                const QList<QAction*> expectedActions = expected.actions();
                const QList<QAction*> actualActions = actual.actions();
                ASSERT_EQ(expectedActions.size(), actualActions.size());
                for (int index = 0; index < expectedActions.size(); ++index)
                {
                    EXPECT_EQ(actual.actionGeometry(actualActions[index]), expected.actionGeometry(expectedActions[index]))
                        << "action " << index << ": " << rectText(actual.actionGeometry(actualActions[index])) << " against "
                        << rectText(expected.actionGeometry(expectedActions[index]));
                }

                expected.resize(expected.sizeHint());
                actual.resize(actual.sizeHint());
                EXPECT_EQ(actual.contentsMargins(), expected.contentsMargins());
                QStyleOption expectedOption;
                expectedOption.initFrom(&expected);
                QStyleOption actualOption;
                actualOption.initFrom(&actual);
                for (const QStyle::PixelMetric metric : { QStyle::PM_MenuPanelWidth, QStyle::PM_MenuHMargin, QStyle::PM_MenuVMargin })
                {
                    EXPECT_EQ(actual.style()->pixelMetric(metric, &actualOption, &actual),
                        expected.style()->pixelMetric(metric, &expectedOption, &expected))
                        << "metric " << static_cast<int>(metric);
                }
                EXPECT_EQ(expected.style()->pixelMetric(QStyle::PM_MenuPanelWidth, &expectedOption, &expected), 8);     // Menu.qss:38-39
            }
        }

        TEST_F(ChromeBarsFlatSheetTests, SourceControlMenu_TheSizeOfAnItemIsTheSheetsOwn)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            // The "Settings" action, whatever the size of its text: the text, the check column of a menu that has a checkable action
            // (the widget action of the real menu does not make one) or none, and the 41px of the item rule
            for (void (*build)(QMenu&) : { &buildSourceControlMenu, &buildTextMenu, &buildCheckableTextMenu })
            {
                LegacyMenu legacyMenu(build);
                FlatMenu flatMenu(*this, build);
                TestMenu& expected = *legacyMenu.m_menu;
                TestMenu& actual = *flatMenu.m_menu;
                const bool hasCheckable = (build == &buildCheckableTextMenu);

                for (const int width : { 20, 47, 120 })
                {
                    QStyleOptionMenuItem expectedOption = itemOption(expected, expected.actions().first());
                    QStyleOptionMenuItem actualOption = itemOption(actual, actual.actions().first());
                    EXPECT_EQ(expectedOption.menuHasCheckableItems, hasCheckable);
                    EXPECT_EQ(actualOption.menuHasCheckableItems, hasCheckable);
                    const QSize contents(width, 14);
                    const QSize expectedSize =
                        expected.style()->sizeFromContents(QStyle::CT_MenuItem, &expectedOption, contents, &expected);
                    const QSize actualSize = actual.style()->sizeFromContents(QStyle::CT_MenuItem, &actualOption, contents, &actual);
                    EXPECT_EQ(actualSize, expectedSize)
                        << "text width " << width << ": " << sizeText(actualSize) << " against " << sizeText(expectedSize);
                    EXPECT_GE(expectedSize.width(), width + 41) << "text width " << width;
                }
            }
        }

        //! Whether the twins of this menu are compared at an action: not a separator or a widget action (the pointer is never on one),
        //! and, in the menu that has a checkable action of text, which is the base style's, only the rows the element carries
        bool isComparedRow(const QList<QAction*>& actions, int index, bool onlyCarriedRows)
        {
            if (index < 0)
            {
                return true;
            }
            const int carriedRows = 2;
            return !actions[index]->isSeparator() && !qobject_cast<QWidgetAction*>(actions[index])
                && !(onlyCarriedRows && index >= carriedRows);
        }

        TEST_F(ChromeBarsFlatSheetTests, SourceControlMenu_TheItemsAreDrawnAsTheLegacyMenuDrawsThem)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            for (void (*build)(QMenu&) : { &buildSourceControlMenu, &buildTextMenu, &buildCheckableTextMenu })
            {
                LegacyMenu legacyMenu(build);
                FlatMenu flatMenu(*this, build);
                TestMenu& expected = *legacyMenu.m_menu;
                TestMenu& actual = *flatMenu.m_menu;
                const QList<QAction*> expectedActions = expected.actions();
                const QList<QAction*> actualActions = actual.actions();
                ASSERT_EQ(expectedActions.size(), actualActions.size());
                const bool onlyCarriedRows = (build == &buildCheckableTextMenu);

                // Idle, and with the pointer on each of the actions that can be, left to right and right to left
                for (const Qt::LayoutDirection direction : { Qt::LeftToRight, Qt::RightToLeft })
                {
                    expected.setLayoutDirection(direction);
                    actual.setLayoutDirection(direction);
                    for (int selected = -1; selected < expectedActions.size(); ++selected)
                    {
                        if (!isComparedRow(expectedActions, selected, onlyCarriedRows))
                        {
                            continue;
                        }
                        for (const qreal ratio : Ratios)
                        {
                            QImage expectedImage = paintMenu(expected, selected < 0 ? nullptr : expectedActions[selected], ratio);
                            QImage actualImage = paintMenu(actual, selected < 0 ? nullptr : actualActions[selected], ratio);
                            EXPECT_TRUE(isDrawnOn(expectedImage));
                            if (onlyCarriedRows)
                            {
                                const int rowsBottom = expected.actionGeometry(expectedActions[1]).bottom() + 1;
                                const QRect device(QPoint(0, 0), (QSizeF(expected.width(), rowsBottom) * ratio).toSize());
                                expectedImage = expectedImage.copy(device);
                                actualImage = actualImage.copy(device);
                            }
                            EXPECT_EQ(differing(actualImage, expectedImage), 0)
                                << "selected " << selected << ", ratio " << ratio << ", direction " << static_cast<int>(direction)
                                << ": " << firstDifference(actualImage, expectedImage);
                        }
                    }
                }
            }
        }

        TEST_F(ChromeBarsFlatSheetTests, SourceControlMenu_TheSheetDrawsTheSelectedFillFromTheItemsMarginAndNotBefore)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            // Numbers of the legacy menu, not a twin: the highlight of the item the pointer is on starts at the 41px margin of its rule
            // (QRenderRule::drawRule fills the border rect), and the fill of the menu stops 8px in from the sides (Menu.qss:38-39)
            LegacyMenu legacyMenu(buildTextMenu);
            FlatMenu flatMenu(*this, buildTextMenu);
            for (TestMenu* menu : { legacyMenu.m_menu, flatMenu.m_menu })
            {
                QAction* settings = menu->actions().first();
                const QImage image = paintMenu(*menu, settings, 1.0);
                const QRect row = menu->actionGeometry(settings);
                const int y = row.center().y();
                const int bottom = row.bottom();                // below the glyphs of the text
                const QRgb selectedFill = 0xFF444444;
                const QRgb menuFill = 0xFF222222;
                EXPECT_EQ(image.pixel(row.left() + 40, bottom), menuFill) << "the margin of the item";
                EXPECT_EQ(image.pixel(row.left() + 41, bottom), selectedFill) << "the item's box";
                EXPECT_EQ(image.pixel(row.right(), bottom), selectedFill) << "the item's right edge";
                EXPECT_EQ(image.pixel(2, y), menuFill) << "inside the margin of the menu";
                EXPECT_EQ(image.pixel(menu->width() - 3, y), menuFill) << "inside the margin of the menu";
            }
        }
    } // namespace ChromeBarsFlatSheetTestsInternal
} // namespace AzQtComponents
