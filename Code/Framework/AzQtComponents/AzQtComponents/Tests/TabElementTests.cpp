/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzQtComponents/Components/DockTabBar.h>
#include <AzQtComponents/Components/DockTabWidget.h>
#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/TabElement.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/Widgets/TabWidget.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <QApplication>
#include <QFont>
#include <QImage>
#include <QPainter>
#include <QPalette>
#include <QStringList>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleOption>
#include <QStyleOptionTab>
#include <QStyleOptionTabWidgetFrame>
#include <QTabBar>
#include <QTabWidget>
#include <QVector>
#include <QWidget>

namespace AzQtComponents
{
    // ----------------------------------------------------------------------------------
    // Test data. The test files of this module are built as one translation unit: what the
    // tests share is a member of the fixture, or lives in this named namespace.
    //
    // The elements below stand for the ones the Project Manager will register. Their
    // numbers are the legacy rules' (ProjectManager.qss, stated again here): the tests prove
    // that the element fields reproduce those rules, not that anything reads the sheet.
    // ----------------------------------------------------------------------------------

    namespace TabElementTestsInternal
    {
        const QColor Transparent(0x00, 0x00, 0x00, 0x00);
        const QColor White(0xFF, 0xFF, 0xFF);                   // BaseStyleSheet.qss:15
        const QColor Blue(0x1E, 0x70, 0xEB);                    // PM.qss:122, :128
        const QColor PressedBlue(0x0E, 0x60, 0xEB);             // PM.qss:132
        const QColor FocusFill(0x52, 0x52, 0x52);               // PM.qss:140
        const QColor FocusBlue(0x40, 0x82, 0xEB);               // PM.qss:140
        const QColor LightBlue(0x94, 0xD2, 0xFF);               // PM.qss:1023, :1028
        const QColor PressedLightBlue(0x66, 0xBC, 0xFA);        // PM.qss:1032
        const QColor PaneGrey(0x33, 0x33, 0x33);                // PM.qss:118
        const QColor Magenta(0xFF, 0x00, 0xFF);                 // a colour no element draws: what a draw leaves shows

        //! border-bottom: 3px solid <color>
        TabElement::Edges underline(const QColor& color)
        {
            return { {}, {}, { 3, color }, {} };
        }

        //! PM.qss:61-143: the tab bar of the first tabs (Projects, Gems, Engine)
        const TabElement HeaderTab{
            .m_name = "Test.Tab.Header",
            .m_idle = { QColor(), White, underline(Transparent) },                  // :67 (border-bottom)
            .m_hover = { QColor(), Blue, underline(Transparent), 500 },             // :128
            .m_selected = { QColor(), Blue, underline(Blue), 500 },                 // :122
            .m_selectedHover = { QColor(), Blue, underline(Blue), 500 },            // :122, :128
            .m_pressed = { QColor(), PressedBlue, underline(Blue), 500 },           // :132 over :122
            .m_focus = { FocusFill, FocusBlue, underline(Blue), 500 },              // :140 over :122
            .m_margin = QMargins(0, 0, 40, 0),                                      // :67
            .m_minContentWidth = 100,                                               // :67
            .m_maxContentWidth = 200,                                               // TabWidget.qss:16
            .m_contentHeight = 82,                                                  // :67
            .m_hideFocusRect = true,                                                // :135
            .m_paneFill = PaneGrey,                                                 // :118
            .m_barLeft = 78,                                                        // :64
        };

        //! PM.qss:585-591: the same, 50px tall
        const TabElement SettingsTab{
            .m_name = "Test.Tab.Settings",
            .m_idle = { QColor(), White, underline(Transparent) },
            .m_hover = { QColor(), Blue, underline(Transparent), 500 },
            .m_selected = { QColor(), Blue, underline(Blue), 500 },
            .m_selectedHover = { QColor(), Blue, underline(Blue), 500 },
            .m_pressed = { QColor(), PressedBlue, underline(Blue), 500 },
            .m_focus = { FocusFill, FocusBlue, underline(Blue), 500 },
            .m_margin = QMargins(0, 0, 40, 0),
            .m_minContentWidth = 100,
            .m_maxContentWidth = 200,
            .m_contentHeight = 50,                                                  // :589
            .m_hideFocusRect = true,
            .m_paneFill = PaneGrey,
            .m_barLeft = 60,                                                        // :585
        };

        //! PM.qss:1011-1034: the sub-tabs of the Engine. #engineTabBar::tab:selected (:1023) outranks the focus rule
        //! (:140) for color, and #engineTabBar::tab (:1015) outranks it for the background: the element has no focus face.
        const TabElement EngineTab{
            .m_name = "Test.Tab.Engine",
            .m_idle = { QColor(), White, underline(Transparent), 400 },             // :1015
            .m_hover = { QColor(), LightBlue, underline(Transparent), 600 },        // :1028
            .m_selected = { QColor(), LightBlue, underline(LightBlue), 600 },       // :1023
            .m_selectedHover = { QColor(), LightBlue, underline(LightBlue), 600 },
            .m_pressed = { QColor(), PressedLightBlue, underline(LightBlue), 600 }, // :1032
            .m_margin = QMargins(0, 0, 40, 0),
            .m_minContentWidth = 160,                                               // :1015
            .m_maxContentWidth = 200,
            .m_contentHeight = 50,                                                  // :1015
            .m_hideFocusRect = true,
            .m_paneFill = PaneGrey,
            .m_barLeft = 60,                                                        // :1011
        };

        // ---- elements that differ in one field, for the label tests
        const TabElement RegularLabelTab{
            .m_name = "Test.Tab.RegularLabel",
            .m_idle = { QColor(), White },
            .m_selected = { QColor(), White },
        };
        const TabElement NormalLabelTab{
            .m_name = "Test.Tab.NormalLabel",
            .m_idle = { QColor(), White, {}, 400 },
            .m_selected = { QColor(), White, {}, 400 },
        };
        const TabElement BoldLabelTab{
            .m_name = "Test.Tab.BoldLabel",
            .m_idle = { QColor(), White, {}, 700 },
            .m_selected = { QColor(), White, {}, 700 },
        };
        const TabElement ShownFocusTab{
            .m_name = "Test.Tab.ShownFocus",
            .m_idle = { QColor(), White },
            .m_selected = { QColor(), White },
            .m_hideFocusRect = false,
        };
        const TabElement HiddenFocusTab{
            .m_name = "Test.Tab.HiddenFocus",
            .m_idle = { QColor(), White },
            .m_selected = { QColor(), White },
            .m_hideFocusRect = true,
        };
        //! No hover face, no minimum height
        const TabElement NoHoverTab{
            .m_name = "Test.Tab.NoHover",
            .m_idle = { QColor(), White },
            .m_selected = { QColor(), White },
        };
        //! Registered by a test of the registry; no bar matches it
        const TabElement UnmatchedTab{
            .m_name = "Test.Tab.Unmatched",
            .m_idle = { QColor(), White },
            .m_selected = { QColor(), White },
        };
        //! Registered after the bar it matches was polished
        const TabElement LateTab{
            .m_name = "Test.Tab.Late",
            .m_barMinHeight = 30,
            .m_idle = { QColor(), White },
            .m_hover = { QColor(), White },
            .m_selected = { QColor(), White },
            .m_selectedHover = { QColor(), White },
            .m_widgetHeight = 40,
        };

        // ---- matchers (plain functions: a matcher has no state), by the object name of the bar
        bool isHeaderBar(const QWidget* bar) { return bar->objectName() == QLatin1String("tabElementTestsHeader"); }
        bool isSettingsBar(const QWidget* bar) { return bar->objectName() == QLatin1String("tabElementTestsSettings"); }
        bool isEngineBar(const QWidget* bar) { return bar->objectName() == QLatin1String("tabElementTestsEngine"); }
        bool isRegularLabelBar(const QWidget* bar) { return bar->objectName() == QLatin1String("tabElementTestsRegularLabel"); }
        bool isNormalLabelBar(const QWidget* bar) { return bar->objectName() == QLatin1String("tabElementTestsNormalLabel"); }
        bool isBoldLabelBar(const QWidget* bar) { return bar->objectName() == QLatin1String("tabElementTestsBoldLabel"); }
        bool isShownFocusBar(const QWidget* bar) { return bar->objectName() == QLatin1String("tabElementTestsShownFocus"); }
        bool isHiddenFocusBar(const QWidget* bar) { return bar->objectName() == QLatin1String("tabElementTestsHiddenFocus"); }
        bool isNoHoverBar(const QWidget* bar) { return bar->objectName() == QLatin1String("tabElementTestsNoHover"); }
        bool matchesNoBar(const QWidget*) { return false; }
        //! By a property the test sets after the bar was polished, so that a registration repeated in one process starts clean
        bool isLateBar(const QWidget* bar) { return bar->property("tabElementTestsLate").toBool(); }

        //! The registry lives as long as the process: the elements above are registered by the first test that needs them
        void registerTestElements()
        {
            static bool registered = false;
            if (!registered)
            {
                registered = true;
                TabElement::registerElement(&HeaderTab, &QTabBar::staticMetaObject, &isHeaderBar);
                TabElement::registerElement(&SettingsTab, &QTabBar::staticMetaObject, &isSettingsBar);
                TabElement::registerElement(&EngineTab, &QTabBar::staticMetaObject, &isEngineBar);
                TabElement::registerElement(&RegularLabelTab, &QTabBar::staticMetaObject, &isRegularLabelBar);
                TabElement::registerElement(&NormalLabelTab, &QTabBar::staticMetaObject, &isNormalLabelBar);
                TabElement::registerElement(&BoldLabelTab, &QTabBar::staticMetaObject, &isBoldLabelBar);
                TabElement::registerElement(&ShownFocusTab, &QTabBar::staticMetaObject, &isShownFocusBar);
                TabElement::registerElement(&HiddenFocusTab, &QTabBar::staticMetaObject, &isHiddenFocusBar);
                TabElement::registerElement(&NoHoverTab, &QTabBar::staticMetaObject, &isNoHoverBar);
            }
        }

        // ---- the legacy rules, as Qt's own sheet style applies them (it does not read the flattened switch)

        //! The global rules of the legacy GUI that reach a tab bar: BaseStyleSheet.qss:15, TabWidget.qss:10 and :16
        const char* const LegacyGlobalRules =
            "* { background-color: transparent; color: white; font-family: \"Open Sans\"; font-weight: 400; }\n"
            "QTabWidget, QTabBar { background-color: #444444; }\n"
            "QTabBar::tab { max-width: 200px; }\n";

        //! PM.qss:61-77, 118-143, 585-591 and 1011-1034 (the tab rules, in the order of the file)
        const char* const LegacyPmTabRules =
            "QTabBar { background-color: transparent; }\n"
            "QTabWidget::tab-bar { left: 78px; }\n"
            "QTabBar::tab { height:82px; background-color: transparent; font-size:24px; min-width:100px; margin-right:40px;"
            " border-bottom: 3px solid transparent; }\n"
            "QTabBar::tab:text { text-align:left; }\n"
            "QTabWidget::pane { background-color: #333333; border:0 none; }\n"
            "QTabBar::tab:selected { background-color: transparent; border-bottom: 3px solid #1e70eb; color: #1e70eb; font-weight: 500; }\n"
            "QTabBar::tab:hover { color: #1e70eb; font-weight: 500; }\n"
            "QTabBar::tab:pressed { color: #0e60eb; }\n"
            "QTabBar::focus { outline: 0px; outline: none; outline-style: none; }\n"
            "QTabBar::tab:focus { background-color: #525252; color: #4082eb; }\n"
            "#projectSettingsTab::tab-bar { left: 60px; }\n"
            "#projectSettingsTabBar::tab { height:50px; }\n"
            "#engineTab::tab-bar { left: 60px; }\n"
            "#engineTabBar::tab { height: 50px; background-color: transparent; font-weight: 400; font-size: 18px; min-width: 160px; }\n"
            "#engineTabBar::tab:selected { border-bottom: 3px solid #94D2FF; color: #94D2FF; font-weight: 600; }\n"
            "#engineTabBar::tab:hover { color: #94D2FF; font-weight: 600; }\n"
            "#engineTabBar::tab:pressed { color: #66bcfa; }\n";

        // ---- what the element model replaced, restated: the tests prove that it still gives it

        //! The face of the four states that existed before the pressed and focus faces: pressed drew as hover, focus not at all
        const TabElement::TabFace& legacyFace(const TabElement& element, QStyle::State state)
        {
            const bool enabled = state.testFlag(QStyle::State_Enabled);
            const bool hover = enabled && (state.testFlag(QStyle::State_MouseOver) || state.testFlag(QStyle::State_Sunken));
            if (state.testFlag(QStyle::State_Selected))
            {
                return hover ? element.m_selectedHover : element.m_selected;
            }
            return hover ? element.m_hover : element.m_idle;
        }

        QSize legacyTabSize(const TabElement& element, const QSize& contents)
        {
            const TabElement::Edges& edges = element.m_idle.m_edges;
            int width = contents.width();
            if (element.m_maxContentWidth > 0)
            {
                width = qMin(width, element.m_maxContentWidth);
            }
            return QSize(width + element.m_padding.left() + element.m_padding.right() + edges.m_left.m_width + edges.m_right.m_width,
                contents.height() + element.m_padding.top() + element.m_padding.bottom() + edges.m_top.m_width + edges.m_bottom.m_width);
        }

        QRect legacyContentsRect(const TabElement& element, const QRect& tabRect)
        {
            const TabElement::Edges& edges = element.m_idle.m_edges;
            return tabRect.adjusted(edges.m_left.m_width + element.m_padding.left(), edges.m_top.m_width + element.m_padding.top(),
                -(edges.m_right.m_width + element.m_padding.right()), -(edges.m_bottom.m_width + element.m_padding.bottom()));
        }

        //! The states a test walks (Qt's flags of a tab): bit 0 enabled, 1 mouse over, 2 sunken, 3 selected, 4 focus
        QStyle::State stateOf(int bits)
        {
            QStyle::State state;
            if (bits & 1)
            {
                state |= QStyle::State_Enabled;
            }
            if (bits & 2)
            {
                state |= QStyle::State_MouseOver;
            }
            if (bits & 4)
            {
                state |= QStyle::State_Sunken;
            }
            if (bits & 8)
            {
                state |= QStyle::State_Selected;
            }
            if (bits & 16)
            {
                state |= QStyle::State_HasFocus;
            }
            return state;
        }

        //! The widths of the four edges of a box, as margins (left, top, right, bottom)
        QMargins edgeWidths(const TabElement::Edges& edges)
        {
            return QMargins(edges.m_left.m_width, edges.m_top.m_width, edges.m_right.m_width, edges.m_bottom.m_width);
        }

        //! How many pixels of two images differ (-1: the images are not of one size)
        int differingPixels(const QImage& first, const QImage& second)
        {
            if (first.size() != second.size())
            {
                return -1;
            }
            int count = 0;
            for (int y = 0; y < first.height(); ++y)
            {
                for (int x = 0; x < first.width(); ++x)
                {
                    count += (first.pixel(x, y) != second.pixel(x, y)) ? 1 : 0;
                }
            }
            return count;
        }

        //! One bar of each Global element, which the tests resolve without showing them. An AzQtComponents TabBar is
        //! made by its tab widget (its constructor is protected), which carries the class tags of the secondary looks.
        struct GlobalBars
        {
            GlobalBars()
                : m_dockTabs(&m_mainWindowStandIn)
            {
                TabWidget::applySecondaryStyle(&m_secondary, false);
                TabWidget::applySecondaryStyle(&m_bordered, true);
            }

            //! In the order primary, secondary, bordered, dock title, dock group, each resolved as its polish resolves it (a class
            //! tag added in a constructor polishes the bar while it is still of its base class)
            QVector<const TabElement*> elements()
            {
                QVector<const TabElement*> resolved;
                for (QTabBar* bar : { m_primary.findChild<QTabBar*>(), m_secondary.findChild<QTabBar*>(), m_bordered.findChild<QTabBar*>(),
                         static_cast<QTabBar*>(&m_dockTitle), m_dockTabs.findChild<QTabBar*>() })
                {
                    TabElement::resolve(bar);
                    resolved.append(TabElement::forWidget(bar));
                }
                return resolved;
            }

            QWidget m_mainWindowStandIn;
            TabWidget m_primary;
            TabWidget m_secondary;
            TabWidget m_bordered;
            DockTabBar m_dockTitle;
            DockTabWidget m_dockTabs;
        };
    } // namespace TabElementTestsInternal

    // ----------------------------------------------------------------------------------
    // Fixture: the flattened style chain (Style -> Fusion) under an application, and a Fusion
    // of its own for the widgets that stand for the legacy GUI
    // ----------------------------------------------------------------------------------

    class TabElementTests
        : public ::UnitTest::LeakDetectionFixture
    {
    public:
        TabElementTests()
        {
            m_application = AZStd::make_unique<QApplication>(m_argc, m_argv);

            // The application font (StyleManager.cpp:220-222), before the style: its class fonts are made at setStyle
            QFont applicationFont(QStringLiteral("Open Sans"));
            applicationFont.setPixelSize(12);
            QApplication::setFont(applicationFont);

            // As StyleManager::initialize builds it (StyleManager.cpp:244-246); the application owns it
            QApplication::setStyle(new Style(QStyleFactory::create(QStringLiteral("Fusion"))));

            // A widget under a style sheet is given this style first: Qt's sheet style then draws over Fusion
            // alone, as it did over the base of the legacy GUI, and never over the flattened style
            m_fusion.reset(QStyleFactory::create(QStringLiteral("Fusion")));
        }

        ~TabElementTests() override
        {
            m_fusion.reset();
            m_application.reset();
        }

        //! A tab bar, and a tab widget, that give the test the options Qt gives the style when it paints them
        class OpenTabBar
            : public QTabBar
        {
        public:
            using QTabBar::initStyleOption;
        };

        class OpenTabWidget
            : public QTabWidget
        {
        public:
            using QTabWidget::initStyleOption;
            using QTabWidget::setTabBar;
        };

        //! The tabs of a bar at their own widths (no stretching), in a bar that is wide enough for all of them
        static void layOut(QTabBar& bar)
        {
            bar.setExpanding(false);
            bar.resize(900, 100);
        }

        //! One tab as QTabBar paints it, in a state: the tab's own pixels (the painter starts from the bar's font)
        static QImage paintTab(OpenTabBar& bar, int index, QStyle::State state)
        {
            QStyleOptionTab option;
            bar.initStyleOption(&option, index);
            option.state = state | QStyle::State_Enabled | QStyle::State_Active;
            option.rect = bar.tabRect(index);

            QImage image(bar.size(), QImage::Format_ARGB32_Premultiplied);
            image.fill(QColor(0x13, 0x13, 0x13));
            QPainter painter(&image);
            painter.setFont(bar.font()); // a paint event starts from the widget's font
            bar.style()->drawControl(QStyle::CE_TabBarTab, &option, &painter, &bar);
            painter.end();
            return image.copy(option.rect);
        }

        //! The bar as QTabBar::paintEvent paints it: its background, the tabs, the current one last
        static QImage paintBar(OpenTabBar& bar)
        {
            QImage image(bar.size(), QImage::Format_ARGB32_Premultiplied);
            image.fill(QColor(0x13, 0x13, 0x13));
            QPainter painter(&image);
            painter.setFont(bar.font());
            if (bar.testAttribute(Qt::WA_StyledBackground))
            {
                QStyleOption background;
                background.initFrom(&bar);
                bar.style()->drawPrimitive(QStyle::PE_Widget, &background, &painter, &bar);
            }
            for (int pass = 0; pass < 2; ++pass)
            {
                for (int index = 0; index < bar.count(); ++index)
                {
                    if ((index == bar.currentIndex()) == (pass == 1))
                    {
                        QStyleOptionTab option;
                        bar.initStyleOption(&option, index);
                        bar.style()->drawControl(QStyle::CE_TabBarTab, &option, &painter, &bar);
                    }
                }
            }
            painter.end();
            return image;
        }

        //! The pane of a tab widget as QTabWidget::paintEvent paints it, on a rect of the test's choosing
        static QImage paintPane(OpenTabWidget& tabs, const QRect& rect)
        {
            QStyleOptionTabWidgetFrame option;
            tabs.initStyleOption(&option);
            option.rect = rect;

            QImage image(rect.size(), QImage::Format_ARGB32_Premultiplied);
            image.fill(QColor(0x13, 0x13, 0x13));
            QPainter painter(&image);
            tabs.style()->drawPrimitive(QStyle::PE_FrameTabWidget, &option, &painter, &tabs);
            painter.end();
            return image;
        }

        //! The label of the first (current) tab of a bar in a state, over a magenta ground; the painter starts from the bar's font
        static QImage paintLabel(OpenTabBar& bar, QStyle::State state)
        {
            QStyleOptionTab option;
            bar.initStyleOption(&option, 0);
            option.state = state | QStyle::State_Enabled | QStyle::State_Active | QStyle::State_Selected;
            option.rect = bar.tabRect(0);

            QImage image(bar.size(), QImage::Format_ARGB32_Premultiplied);
            image.fill(TabElementTestsInternal::Magenta);
            QPainter painter(&image);
            painter.setFont(bar.font());
            bar.style()->drawControl(QStyle::CE_TabBarTabLabel, &option, &painter, &bar);
            painter.end();
            return image;
        }

        //! A bar with one tab of a name that registered elements match, polished as the flattened GUI polishes it
        static void prepareLabelBar(OpenTabBar& bar, const char* name)
        {
            TabElementTestsInternal::registerTestElements();
            bar.setObjectName(QString::fromUtf8(name));
            bar.addTab(QStringLiteral("Gems"));
            bar.ensurePolished();
            layOut(bar);
        }

    protected:
        int m_argc = 1;                                     // QApplication keeps a reference to it
        char m_programName[5] = "test";
        char* m_argv[2] = { m_programName, nullptr };
        AZStd::unique_ptr<QApplication> m_application;
        AZStd::unique_ptr<QStyle> m_fusion;
    };

    // ----------------------------------------------------------------------------------
    // T1: the Global elements draw as they did. Every new field is unset on them, and each
    // method that gained a field gives the answer of the code it replaced.
    // ----------------------------------------------------------------------------------

    TEST_F(TabElementTests, GlobalElements_LeaveEveryNewFieldUnset)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled()); // the test environment sets it (AzQtComponentTests.cpp)

        TabElementTestsInternal::GlobalBars bars;
        const QVector<const TabElement*> elements = bars.elements();
        const char* const names[] = { "Global.Tab.Primary", "Global.Tab.Secondary", "Global.Tab.Secondary.Bordered",
            "Global.Tab.DockTitle", "Global.Tab.DockGroup" };
        ASSERT_EQ(elements.size(), 5);

        for (int index = 0; index < elements.size(); ++index)
        {
            const TabElement* element = elements[index];
            ASSERT_TRUE(element != nullptr);
            EXPECT_STREQ(element->m_name, names[index]);
            EXPECT_FALSE(element->isPlain());
            EXPECT_FALSE(element->m_pressed.isSet());
            EXPECT_FALSE(element->m_focus.isSet());
            EXPECT_TRUE(element->m_margin == QMargins());
            EXPECT_EQ(element->m_minContentWidth, 0);
            EXPECT_EQ(element->m_contentHeight, 0);
            EXPECT_FALSE(element->m_hideFocusRect);
            EXPECT_FALSE(element->m_paneFill.isValid());
            for (const TabElement::TabFace* face : { &element->m_idle, &element->m_hover, &element->m_selected, &element->m_selectedHover })
            {
                EXPECT_TRUE(face->isSet());
                EXPECT_EQ(face->m_weight, 0);
            }
        }
    }

    TEST_F(TabElementTests, GlobalElements_FaceIsTheOneOfTheFourStatesForEveryStateOfATab)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        // Without a pressed or a focus face, a pressed tab draws as hover and a focused one as idle or selected
        TabElementTestsInternal::GlobalBars bars;
        for (const TabElement* element : bars.elements())
        {
            ASSERT_TRUE(element != nullptr);
            for (int bits = 0; bits < 32; ++bits)
            {
                const QStyle::State state = TabElementTestsInternal::stateOf(bits);
                QStyleOptionTab option;
                option.state = state;
                EXPECT_TRUE(&element->face(&option) == &TabElementTestsInternal::legacyFace(*element, state))
                    << element->m_name << ", state bits " << bits;
            }
        }
    }

    TEST_F(TabElementTests, GlobalElements_TabSizeAndContentsRectAreTheOnesTheyWere)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        TabElementTestsInternal::GlobalBars bars;
        const QSize contents[] = { QSize(30, 14), QSize(150, 14), QSize(400, 22) };
        for (const TabElement* element : bars.elements())
        {
            ASSERT_TRUE(element != nullptr);
            for (const QSize& size : contents)
            {
                EXPECT_TRUE(element->tabSize(size) == TabElementTestsInternal::legacyTabSize(*element, size)) << element->m_name;
            }
            const QRect tabRect(10, 5, 120, 30);
            EXPECT_TRUE(element->contentsRect(tabRect) == TabElementTestsInternal::legacyContentsRect(*element, tabRect)) << element->m_name;
        }
    }

    TEST_F(TabElementTests, GlobalElements_DrawTabPaintsTheFaceFillAndEdgesInTheWholeRect)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        TabElementTestsInternal::GlobalBars bars;
        const QRect rect(2, 3, 60, 30);
        for (const TabElement* element : bars.elements())
        {
            ASSERT_TRUE(element != nullptr);
            for (int bits = 0; bits < 32; ++bits)
            {
                const QStyle::State state = TabElementTestsInternal::stateOf(bits);
                QStyleOptionTab option;
                option.state = state;
                option.rect = rect;

                QImage drawn(70, 40, QImage::Format_ARGB32_Premultiplied);
                drawn.fill(TabElementTestsInternal::Magenta);
                QPainter drawnPainter(&drawn);
                element->drawTab(&drawnPainter, &option);
                drawnPainter.end();

                // What the code drew before the margin: the face's fill in the rect, then its edges
                const TabElement::TabFace& face = TabElementTestsInternal::legacyFace(*element, state);
                QImage expected(70, 40, QImage::Format_ARGB32_Premultiplied);
                expected.fill(TabElementTestsInternal::Magenta);
                QPainter expectedPainter(&expected);
                if (face.m_fill.isValid())
                {
                    expectedPainter.fillRect(rect, face.m_fill);
                }
                BoxElement::drawEdges(&expectedPainter, rect, face.m_edges, BoxElement::Radii());
                expectedPainter.end();

                EXPECT_EQ(TabElementTestsInternal::differingPixels(drawn, expected), 0) << element->m_name << ", state bits " << bits;
            }
        }
    }

    TEST_F(TabElementTests, Elements_AllFacesOfAnElementShareOneBox)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        // tabSize and contentsRect read the box of the idle face: the stylesheets gave every state of a tab one size, and
        // a face with other edge widths would draw outside the box the layout made
        TabElementTestsInternal::GlobalBars bars;
        QVector<const TabElement*> elements = bars.elements();
        elements << &TabElementTestsInternal::HeaderTab << &TabElementTestsInternal::SettingsTab << &TabElementTestsInternal::EngineTab;
        for (const TabElement* element : elements)
        {
            ASSERT_TRUE(element != nullptr);
            for (const TabElement::TabFace* face : { &element->m_hover, &element->m_selected, &element->m_selectedHover, &element->m_pressed, &element->m_focus })
            {
                if (face->isSet())
                {
                    EXPECT_TRUE(TabElementTestsInternal::edgeWidths(face->m_edges) == TabElementTestsInternal::edgeWidths(element->m_idle.m_edges))
                        << element->m_name;
                }
            }
        }
    }

    TEST_F(TabElementTests, GlobalElements_PolishAsBefore)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        // The primary element: the bar is 28 tall at least (TabWidget.qss:46), draws no base line, follows the hover
        TabWidget tabs;
        tabs.ensurePolished();
        QTabBar* bar = tabs.findChild<QTabBar*>();
        ASSERT_TRUE(bar != nullptr);
        ASSERT_TRUE(TabElement::forTabBar(bar) != nullptr);
        EXPECT_EQ(bar->minimumHeight(), 28);
        EXPECT_FALSE(bar->drawBase());
        EXPECT_TRUE(bar->testAttribute(Qt::WA_StyledBackground));
        EXPECT_TRUE(bar->testAttribute(Qt::WA_Hover));
    }

    // ----------------------------------------------------------------------------------
    // T2: the fields the Project Manager's tab rules need. The element is tested through its own
    // methods first, then as a whole against Qt's sheet style applying the legacy rules.
    // ----------------------------------------------------------------------------------

    TEST_F(TabElementTests, TabSize_IsTheBoundedContentsPlusMarginAndEdges)
    {
        using namespace TabElementTestsInternal;

        // Native: tab rects 140x85 (header), 200x53 (sub-tabs): min-width over the label, the 40px margin and the
        // 3px border added, height 82 or 50 whatever the text is (QRenderRule::adjustSize, boxSize)
        EXPECT_TRUE(HeaderTab.tabSize(QSize(86, 33)) == QSize(140, 85));
        EXPECT_TRUE(HeaderTab.tabSize(QSize(30, 10)) == QSize(140, 85));
        EXPECT_TRUE(HeaderTab.tabSize(QSize(100, 120)) == QSize(140, 85));
        EXPECT_TRUE(HeaderTab.tabSize(QSize(250, 40)) == QSize(240, 85));       // max-width 200 first
        EXPECT_TRUE(SettingsTab.tabSize(QSize(86, 33)) == QSize(140, 53));
        EXPECT_TRUE(EngineTab.tabSize(QSize(120, 20)) == QSize(200, 53));
        EXPECT_TRUE(EngineTab.tabSize(QSize(300, 20)) == QSize(240, 53));

        // The minimum wins over the maximum, as in QRenderRule::adjustSize
        TabElement bounded;
        bounded.m_minContentWidth = 160;
        bounded.m_maxContentWidth = 100;
        EXPECT_TRUE(bounded.tabSize(QSize(120, 20)) == QSize(160, 20));
    }

    TEST_F(TabElementTests, ContentsRect_IsTheTabRectLessMarginEdgesAndPadding)
    {
        using namespace TabElementTestsInternal;

        EXPECT_TRUE(HeaderTab.contentsRect(QRect(0, 0, 140, 85)) == QRect(0, 0, 100, 82));
        EXPECT_TRUE(HeaderTab.contentsRect(QRect(140, 0, 140, 85)) == QRect(140, 0, 100, 82));
        EXPECT_TRUE(EngineTab.contentsRect(QRect(0, 0, 200, 53)) == QRect(0, 0, 160, 50));
    }

    TEST_F(TabElementTests, DrawTab_FillAndUnderlineSpanTheBoxAndNeverTheMargin)
    {
        using namespace TabElementTestsInternal;

        QStyleOptionTab option;
        option.rect = QRect(0, 0, 140, 85);

        // Selected: the 3px underline under the 100px label box, nothing else (the fill is none)
        option.state = QStyle::State_Enabled | QStyle::State_Selected;
        QImage image(140, 85, QImage::Format_ARGB32_Premultiplied);
        image.fill(Magenta);
        QPainter painter(&image);
        HeaderTab.drawTab(&painter, &option);
        painter.end();
        EXPECT_TRUE(image.pixelColor(0, 82) == Blue);
        EXPECT_TRUE(image.pixelColor(99, 84) == Blue);
        EXPECT_TRUE(image.pixelColor(50, 81) == Magenta);
        EXPECT_TRUE(image.pixelColor(50, 40) == Magenta);
        EXPECT_TRUE(image.pixelColor(100, 83) == Magenta);
        EXPECT_TRUE(image.pixelColor(139, 83) == Magenta);

        // Focused: the grey fill covers the box too (PM.qss:140), under the underline
        option.state = QStyle::State_Enabled | QStyle::State_Selected | QStyle::State_HasFocus;
        image.fill(Magenta);
        painter.begin(&image);
        HeaderTab.drawTab(&painter, &option);
        painter.end();
        EXPECT_TRUE(image.pixelColor(0, 0) == FocusFill);
        EXPECT_TRUE(image.pixelColor(99, 81) == FocusFill);
        EXPECT_TRUE(image.pixelColor(50, 83) == Blue);
        EXPECT_TRUE(image.pixelColor(100, 40) == Magenta);

        // Idle: the transparent underline draws nothing
        option.state = QStyle::State_Enabled;
        image.fill(Magenta);
        painter.begin(&image);
        HeaderTab.drawTab(&painter, &option);
        painter.end();
        EXPECT_TRUE(image.pixelColor(50, 83) == Magenta);
    }

    TEST_F(TabElementTests, Face_LaterRulesWin_FocusThenPressedThenHoverAndSelected)
    {
        using namespace TabElementTestsInternal;

        const QStyle::State enabled = QStyle::State_Enabled;
        QStyleOptionTab option;

        // The probe of the legacy PM rules through Qt's sheet style (idle white, hover blue, selected blue with the
        // underline, pressed #0e60eb, focus #4082eb on #525252, focus over pressed over hover)
        option.state = enabled;
        EXPECT_TRUE(&HeaderTab.face(&option) == &HeaderTab.m_idle);
        option.state = enabled | QStyle::State_MouseOver;
        EXPECT_TRUE(&HeaderTab.face(&option) == &HeaderTab.m_hover);
        option.state = enabled | QStyle::State_Selected;
        EXPECT_TRUE(&HeaderTab.face(&option) == &HeaderTab.m_selected);
        option.state = enabled | QStyle::State_Selected | QStyle::State_MouseOver;
        EXPECT_TRUE(&HeaderTab.face(&option) == &HeaderTab.m_selectedHover);
        option.state = enabled | QStyle::State_Selected | QStyle::State_Sunken;
        EXPECT_TRUE(&HeaderTab.face(&option) == &HeaderTab.m_pressed);
        option.state = enabled | QStyle::State_Selected | QStyle::State_Sunken | QStyle::State_MouseOver;
        EXPECT_TRUE(&HeaderTab.face(&option) == &HeaderTab.m_pressed);
        option.state = enabled | QStyle::State_Selected | QStyle::State_HasFocus;
        EXPECT_TRUE(&HeaderTab.face(&option) == &HeaderTab.m_focus);
        option.state = enabled | QStyle::State_Selected | QStyle::State_HasFocus | QStyle::State_MouseOver;
        EXPECT_TRUE(&HeaderTab.face(&option) == &HeaderTab.m_focus);
        option.state = enabled | QStyle::State_Selected | QStyle::State_HasFocus | QStyle::State_Sunken;
        EXPECT_TRUE(&HeaderTab.face(&option) == &HeaderTab.m_focus);

        // Pressed and focus belong to the current tab: another tab pressed (a style that selects on release) draws as hover
        option.state = enabled | QStyle::State_Sunken;
        EXPECT_TRUE(&HeaderTab.face(&option) == &HeaderTab.m_hover);

        // A disabled tab is never hovered, pressed or focused
        option.state = QStyle::State_MouseOver | QStyle::State_Sunken;
        EXPECT_TRUE(&HeaderTab.face(&option) == &HeaderTab.m_idle);
        option.state = QStyle::State_Selected | QStyle::State_MouseOver | QStyle::State_Sunken;
        EXPECT_TRUE(&HeaderTab.face(&option) == &HeaderTab.m_selected);
    }

    TEST_F(TabElementTests, Face_AnElementWithoutAFocusFaceDrawsFocusAsSelected)
    {
        using namespace TabElementTestsInternal;

        // The sub-tabs: #engineTabBar::tab:selected outranks the focus rule (PM.qss:1023 over :140), and has a pressed face
        QStyleOptionTab option;
        option.state = QStyle::State_Enabled | QStyle::State_Selected | QStyle::State_HasFocus;
        EXPECT_TRUE(&EngineTab.face(&option) == &EngineTab.m_selected);
        option.state = QStyle::State_Enabled | QStyle::State_Selected | QStyle::State_HasFocus | QStyle::State_Sunken;
        EXPECT_TRUE(&EngineTab.face(&option) == &EngineTab.m_pressed);
    }

    TEST_F(TabElementTests, DrawLabel_TheFaceWeightIsTheRulesFontWeightOverTheBarsFont)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        OpenTabBar regular;
        prepareLabelBar(regular, "tabElementTestsRegularLabel");
        OpenTabBar bold;
        prepareLabelBar(bold, "tabElementTestsBoldLabel");
        ASSERT_TRUE(TabElement::forTabBar(&regular) == &TabElementTestsInternal::RegularLabelTab);
        ASSERT_TRUE(TabElement::forTabBar(&bold) == &TabElementTestsInternal::BoldLabelTab);

        // No weight: the bar's own (normal) font; a weight draws the label in it, and the painter's font is put back
        const QImage unweighted = paintLabel(regular, QStyle::State());
        const QImage weighted = paintLabel(bold, QStyle::State());
        EXPECT_GT(TabElementTestsInternal::differingPixels(unweighted, weighted), 0);

        // A weight of 400 is the bar's own: it draws what no weight draws
        OpenTabBar normal;
        prepareLabelBar(normal, "tabElementTestsNormalLabel");
        ASSERT_TRUE(TabElement::forTabBar(&normal) == &TabElementTestsInternal::NormalLabelTab);
        EXPECT_EQ(TabElementTestsInternal::differingPixels(unweighted, paintLabel(normal, QStyle::State())), 0);
    }

    TEST_F(TabElementTests, DrawLabel_HideFocusRectDropsQtsFocusRectangleAndNothingElse)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        OpenTabBar shown;
        prepareLabelBar(shown, "tabElementTestsShownFocus");
        OpenTabBar hidden;
        prepareLabelBar(hidden, "tabElementTestsHiddenFocus");
        ASSERT_TRUE(TabElement::forTabBar(&shown) == &TabElementTestsInternal::ShownFocusTab);
        ASSERT_TRUE(TabElement::forTabBar(&hidden) == &TabElementTestsInternal::HiddenFocusTab);

        // Fusion draws the focus rectangle of a tab when the focus came from the keyboard (qfusionstyle.cpp:691-712)
        const QStyle::State keyboardFocus = QStyle::State_HasFocus | QStyle::State_KeyboardFocusChange;
        EXPECT_GT(TabElementTestsInternal::differingPixels(paintLabel(shown, QStyle::State()), paintLabel(shown, keyboardFocus)), 0);
        EXPECT_EQ(TabElementTestsInternal::differingPixels(paintLabel(hidden, QStyle::State()), paintLabel(hidden, keyboardFocus)), 0);
    }

    TEST_F(TabElementTests, Polish_HoverFaceSwitchesHoverOnAndTheWidgetsOwnMinimumHeightStays)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());
        TabElementTestsInternal::registerTestElements();

        // A plain QTabBar does not follow the hovered tab by itself (qtabbar.cpp:1687-1718); the legacy sheet's :hover rule
        // switched it on (qstylesheetstyle.cpp:2894-2905). The header rules set no min-height: the widget's own stays.
        OpenTabBar header;
        header.setObjectName(QStringLiteral("tabElementTestsHeader"));
        header.addTab(QStringLiteral("Gems"));
        header.setMinimumHeight(17);
        ASSERT_FALSE(header.testAttribute(Qt::WA_Hover));
        header.ensurePolished();
        EXPECT_TRUE(header.testAttribute(Qt::WA_Hover));
        EXPECT_FALSE(header.drawBase());
        EXPECT_TRUE(header.testAttribute(Qt::WA_StyledBackground));
        EXPECT_EQ(header.minimumHeight(), 17);

        // No hover face, no hover
        OpenTabBar noHover;
        noHover.setObjectName(QStringLiteral("tabElementTestsNoHover"));
        noHover.addTab(QStringLiteral("Gems"));
        noHover.ensurePolished();
        ASSERT_TRUE(TabElement::forTabBar(&noHover) == &TabElementTestsInternal::NoHoverTab);
        EXPECT_FALSE(noHover.testAttribute(Qt::WA_Hover));
    }

    TEST_F(TabElementTests, PaneFill_PaintsBeforeThePaneEdges)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());
        TabElementTestsInternal::registerTestElements();

        // QTabWidget::pane { background-color: #333333; border: 0 none } (PM.qss:118): the whole pane rect
        // The bar is named before it is handed over: QTabWidget::setTabBar shows (and so polishes) it, as the legacy
        // sheet would have matched it by name at that polish
        OpenTabWidget tabs;
        auto* header = new OpenTabBar;
        header->setObjectName(QStringLiteral("tabElementTestsHeader"));
        tabs.setTabBar(header);
        tabs.addTab(new QWidget, QStringLiteral("Gems"));
        tabs.ensurePolished();
        ASSERT_TRUE(TabElement::forTabWidget(&tabs) == &TabElementTestsInternal::HeaderTab);

        const QImage pane = paintPane(tabs, QRect(0, 0, 200, 80));
        EXPECT_TRUE(pane.pixelColor(0, 0) == TabElementTestsInternal::PaneGrey);
        EXPECT_TRUE(pane.pixelColor(100, 40) == TabElementTestsInternal::PaneGrey);
        EXPECT_TRUE(pane.pixelColor(199, 79) == TabElementTestsInternal::PaneGrey);
    }

    TEST_F(TabElementTests, PmTabRules_PaintAsTheLegacySheetDid)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());
        TabElementTestsInternal::registerTestElements();

        // Each bar twice: under the legacy rules (Qt's sheet style over Fusion), and as the element draws it. The legacy
        // sheet addresses the sub-tab bars by object name; the elements' matchers do by names of the tests' own.
        struct BarCase
        {
            const char* m_legacyName;
            const char* m_flatName;
            int m_fontPixels;                   // the TextElement row of the bar (24px, 18px: PM.qss:67, :1015)
            QStringList m_labels;
        };
        const BarCase cases[] = {
            { "headerTabBar", "tabElementTestsHeader", 24, { QStringLiteral("Projects"), QStringLiteral("Gems"), QStringLiteral("Engine") } },
            { "engineTabBar", "tabElementTestsEngine", 18, { QStringLiteral("General"), QStringLiteral("Remote Sources") } },
            { "projectSettingsTabBar", "tabElementTestsSettings", 24, { QStringLiteral("General"), QStringLiteral("Gems") } },
        };

        // Selected is the current tab (the first); the others are idle or hovered. Qt presses a tab as it selects it.
        struct StateCase
        {
            const char* m_name;
            QStyle::State m_state;
        };
        const StateCase states[] = {
            { "idle", QStyle::State() },
            { "hover", QStyle::State_MouseOver },
            { "selected", QStyle::State_Selected },
            { "selected and hover", QStyle::State_Selected | QStyle::State_MouseOver },
            { "selected and pressed", QStyle::State_Selected | QStyle::State_Sunken },
            { "selected, hover and pressed", QStyle::State_Selected | QStyle::State_MouseOver | QStyle::State_Sunken },
            { "selected and focus", QStyle::State_Selected | QStyle::State_HasFocus },
            { "selected, hover and focus", QStyle::State_Selected | QStyle::State_MouseOver | QStyle::State_HasFocus },
            { "selected, pressed and focus", QStyle::State_Selected | QStyle::State_Sunken | QStyle::State_HasFocus },
            { "selected and keyboard focus", QStyle::State_Selected | QStyle::State_HasFocus | QStyle::State_KeyboardFocusChange },
        };

        for (const BarCase& barCase : cases)
        {
            OpenTabBar legacy;
            legacy.setObjectName(QString::fromUtf8(barCase.m_legacyName));
            legacy.setStyle(m_fusion.get());
            legacy.setStyleSheet(QString::fromUtf8(TabElementTestsInternal::LegacyGlobalRules) + QString::fromUtf8(TabElementTestsInternal::LegacyPmTabRules));

            OpenTabBar flat;
            flat.setObjectName(QString::fromUtf8(barCase.m_flatName));
            QFont font(QStringLiteral("Open Sans"));
            font.setPixelSize(barCase.m_fontPixels);
            flat.setFont(font);

            for (const QString& label : barCase.m_labels)
            {
                legacy.addTab(label);
                flat.addTab(label);
            }
            legacy.ensurePolished();
            flat.ensurePolished();
            layOut(legacy);
            layOut(flat);

            // Sizes: the tab rects, and so the bar's
            ASSERT_EQ(legacy.count(), flat.count());
            for (int index = 0; index < legacy.count(); ++index)
            {
                EXPECT_TRUE(legacy.tabRect(index) == flat.tabRect(index)) << barCase.m_flatName << ", tab " << index;
            }
            EXPECT_TRUE(legacy.sizeHint() == flat.sizeHint()) << barCase.m_flatName;

            // Paint: state by state, the pixels of the tab
            for (const StateCase& stateCase : states)
            {
                const int index = stateCase.m_state.testFlag(QStyle::State_Selected) ? 0 : 1;
                const QImage legacyImage = paintTab(legacy, index, stateCase.m_state);
                const QImage flatImage = paintTab(flat, index, stateCase.m_state);
                EXPECT_EQ(TabElementTestsInternal::differingPixels(legacyImage, flatImage), 0) << barCase.m_flatName << ", " << stateCase.m_name;
            }
        }
    }

    // ----------------------------------------------------------------------------------
    // T3: a module that registers after its windows were polished
    // ----------------------------------------------------------------------------------

    TEST_F(TabElementTests, RegisterElement_ResolvesAgainTheBarsPolishedBeforeIt)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        OpenTabWidget tabs;
        tabs.setTabBar(new OpenTabBar);
        tabs.addTab(new QWidget, QStringLiteral("Gems"));
        QTabBar* bar = tabs.findChild<QTabBar*>();
        ASSERT_TRUE(bar != nullptr);
        tabs.ensurePolished();

        // Polished before: the plain element is the one kept on the bar, and no tab widget rule was applied
        EXPECT_TRUE(TabElement::forTabBar(bar) == nullptr);
        ASSERT_TRUE(TabElement::plainForTabBar(bar) != nullptr);
        EXPECT_FALSE(bar->testAttribute(Qt::WA_Hover));
        EXPECT_EQ(tabs.maximumHeight(), QWIDGETSIZE_MAX);

        // The element matches from here on (the matcher reads this property): resolved again, by the registration alone
        bar->setProperty("tabElementTestsLate", true);
        EXPECT_TRUE(TabElement::plainForTabBar(bar) != nullptr);
        TabElement::registerElement(&TabElementTestsInternal::LateTab, &QTabBar::staticMetaObject, &TabElementTestsInternal::isLateBar);

        // The bar is resolved again (the plain element kept before is gone), and both it and its tab widget polished again
        EXPECT_TRUE(TabElement::forTabBar(bar) == &TabElementTestsInternal::LateTab);
        EXPECT_TRUE(TabElement::plainForTabBar(bar) == nullptr);
        EXPECT_TRUE(TabElement::forTabWidget(&tabs) == &TabElementTestsInternal::LateTab);
        EXPECT_TRUE(bar->testAttribute(Qt::WA_Hover));
        EXPECT_EQ(bar->minimumHeight(), 30);
        EXPECT_EQ(tabs.minimumHeight(), 40);
        EXPECT_EQ(tabs.maximumHeight(), 40);
    }

    TEST_F(TabElementTests, RegisterElement_LeavesTheBarsItDoesNotMatchAlone)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        // A bar of the Global primary element keeps it, and what polish set on it, when an element no bar matches registers
        TabWidget tabs;
        tabs.ensurePolished();
        QTabBar* primary = tabs.findChild<QTabBar*>();
        ASSERT_TRUE(primary != nullptr);
        const TabElement* before = TabElement::forWidget(primary);
        ASSERT_TRUE(before != nullptr);
        primary->setMinimumHeight(50);

        TabElement::registerElement(&TabElementTestsInternal::UnmatchedTab, &QTabBar::staticMetaObject, &TabElementTestsInternal::matchesNoBar);

        EXPECT_TRUE(TabElement::forTabBar(primary) == before);
        EXPECT_EQ(primary->minimumHeight(), 50); // not polished again: the element did not change
    }

    // ----------------------------------------------------------------------------------
    // T4: plain tab widgets (Global.Tab.Plain). Qt's own tabs and pane over the palette the legacy sheet
    // gave them (TabWidget.qss:10), compared with Qt's sheet style applying that rule.
    // ----------------------------------------------------------------------------------

    TEST_F(TabElementTests, Plain_AQTabBarOfQtResolvesToPlainAndNoElementDrawsIt)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        OpenTabWidget tabs;
        tabs.setTabBar(new OpenTabBar);
        tabs.addTab(new QWidget, QStringLiteral("Patterns"));
        tabs.ensurePolished();
        QTabBar* bar = tabs.findChild<QTabBar*>();
        ASSERT_TRUE(bar != nullptr);

        const TabElement* element = TabElement::forWidget(bar);
        ASSERT_TRUE(element != nullptr);
        EXPECT_STREQ(element->m_name, "Global.Tab.Plain");
        EXPECT_TRUE(element->isPlain());
        EXPECT_TRUE(TabElement::forTabBar(bar) == nullptr);
        EXPECT_TRUE(TabElement::forTabWidget(&tabs) == nullptr);
        EXPECT_TRUE(TabElement::plainForTabBar(bar) == element);
        EXPECT_TRUE(TabElement::plainForTabWidget(&tabs) == element);

        // What polish does to a plain bar: the sheet's background (a styled background), and nothing of an element's
        EXPECT_TRUE(bar->testAttribute(Qt::WA_StyledBackground));
        EXPECT_FALSE(bar->testAttribute(Qt::WA_Hover));
        EXPECT_FALSE(tabs.testAttribute(Qt::WA_StyledBackground));

        // A widget that is no tab bar resolves to nothing, not to the plain element
        QWidget other;
        EXPECT_TRUE(TabElement::forWidget(&other) == nullptr);
    }

    TEST_F(TabElementTests, Plain_PaneAndIdleTabAreFusionsOverTheSheetsColour)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        OpenTabWidget tabs;
        OpenTabBar* bar = new OpenTabBar;
        tabs.setTabBar(bar);
        for (const char* label : { "Patterns", "Details", "Metrics" })
        {
            tabs.addTab(new QWidget, QString::fromUtf8(label));
        }
        tabs.ensurePolished();
        layOut(*bar);

        // Fusion derives them from the palette's Button #444444 (qfusionstyle.cpp:785, 1671-1686): the pane #535353, an idle
        // tab #4d4d4d (the Asset Processor captures of Builders and Assets: pane #535353, idle tab #4d4d4d)
        const QImage pane = paintPane(tabs, QRect(0, 0, 200, 80));
        EXPECT_TRUE(pane.pixelColor(100, 40) == QColor(0x53, 0x53, 0x53));

        // (sampled left of the label, which starts 12px in, and at mid height: Fusion's idle gradient is flat there)
        const QImage idleTab = paintTab(*bar, 1, QStyle::State());
        EXPECT_TRUE(idleTab.pixelColor(5, idleTab.height() / 2) == QColor(0x4D, 0x4D, 0x4D));
    }

    TEST_F(TabElementTests, Plain_PaintsAndSizesLikeTheLegacySheet)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        // The legacy sheet over Fusion, and the flattened style: one tab widget each, the last tab's label wider than 200px
        const QStringList labels = { QStringLiteral("Patterns"), QStringLiteral("Details"),
            QStringLiteral("A very long tab label that is certainly wider than two hundred pixels") };
        OpenTabWidget legacy;
        legacy.setStyle(m_fusion.get());
        legacy.setStyleSheet(QString::fromUtf8(TabElementTestsInternal::LegacyGlobalRules));
        OpenTabWidget flat;
        OpenTabBar* legacyBar = new OpenTabBar;
        OpenTabBar* flatBar = new OpenTabBar;
        legacy.setTabBar(legacyBar);
        flat.setTabBar(flatBar);
        for (const QString& label : labels)
        {
            legacy.addTab(new QWidget, label);
            flat.addTab(new QWidget, label);
        }
        legacy.ensurePolished();
        flat.ensurePolished();
        layOut(*legacyBar);
        layOut(*flatBar);

        // Sizes: the width bound of TabWidget.qss:16 and Qt's own spacing everywhere else
        for (int index = 0; index < labels.size(); ++index)
        {
            EXPECT_TRUE(legacyBar->tabRect(index) == flatBar->tabRect(index)) << "tab " << index;
        }
        EXPECT_EQ(flatBar->tabRect(2).width(), 200);

        // Paint: the bar with its tabs (the selected one and the others), and the pane
        EXPECT_EQ(TabElementTestsInternal::differingPixels(paintBar(*legacyBar), paintBar(*flatBar)), 0);
        EXPECT_EQ(TabElementTestsInternal::differingPixels(paintPane(legacy, QRect(0, 0, 200, 80)), paintPane(flat, QRect(0, 0, 200, 80))), 0);

        // The same with another tab current, and with the tabs below the pane
        legacy.setCurrentIndex(1);
        flat.setCurrentIndex(1);
        legacy.setTabPosition(QTabWidget::South);
        flat.setTabPosition(QTabWidget::South);
        EXPECT_EQ(TabElementTestsInternal::differingPixels(paintBar(*legacyBar), paintBar(*flatBar)), 0);
        EXPECT_EQ(TabElementTestsInternal::differingPixels(paintPane(legacy, QRect(0, 0, 200, 80)), paintPane(flat, QRect(0, 0, 200, 80))), 0);
    }
} // namespace AzQtComponents
