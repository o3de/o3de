/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/ControlElements.h>
#include <AzQtComponents/Components/Elements/TabElement.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/StyledDockWidget.h>
#include <AzQtComponents/Components/Titlebar.h>
#include <AzQtComponents/Components/Widgets/SegmentBar.h>
#include <AzQtComponents/Components/Widgets/SegmentControl.h>
#include <AzQtComponents/Components/Widgets/Slider.h>
#include <AzQtComponents/Components/Widgets/TabWidget.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <QApplication>
#include <QDockWidget>
#include <QFile>
#include <QIcon>
#include <QImage>
#include <QLabel>
#include <QMainWindow>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QRegularExpression>
#include <QSlider>
#include <QStyleFactory>
#include <QStyleOption>
#include <QStyleOptionButton>
#include <QStyleOptionSlider>
#include <QStyleOptionTab>
#include <QStyleOptionTabWidgetFrame>
#include <QTabBar>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QtMath>

#include <string>
#include <vector>

namespace AzQtComponents
{
    // ----------------------------------------------------------------------------------
    // The chrome of the flattened GUI against the legacy sheets: the buttons of a segment bar (S), a rule a component draws itself (F), the
    // edges of a tab (E), the frame of a floating dock (D), the bottom line of its title bar (T) and the handle of a disabled slider (L). The
    // legacy twin of a widget is Qt's own style sheet style over a Fusion of its own with the real sheets of the engine (BaseStyleSheet.qss
    // and what it imports); the flat twin is the flattened Style over the same widget. Each test builds the same tree twice, shows both, and
    // compares what Qt reads from the style (hints, limits, geometry, pixel metrics) and what it paints, through the style onto an image of
    // an explicit device ratio (a grab takes the ratio of the widget's screen, which is not the ratio the Native captures were taken at:
    // 1.5), left to right and right to left. Where a number comes from a Native capture (n2), the test says which.
    // ----------------------------------------------------------------------------------

    //! Named, not anonymous: a unity build joins the test files, where anonymous namespaces merge
    namespace ChromeTwinTestsInternal
    {
        //! The application sheet as StyleSheetCache builds it from BaseStyleSheet.qss: each @import line replaced by the file it names, from the
        //! real resources
        const QString& appSheet()
        {
            static QString expanded;
            if (expanded.isEmpty())
            {
                const auto read = [](const QString& name)
                {
                    QFile file(QStringLiteral(":/AzQtComponents/Widgets/") + name);
                    EXPECT_TRUE(file.open(QIODevice::ReadOnly)) << name.toUtf8().constData();
                    return QString::fromUtf8(file.readAll());
                };
                expanded = read(QStringLiteral("BaseStyleSheet.qss"));
                const QRegularExpression import(QStringLiteral("@import\\s+\"([^\"]+)\"\\s*;"));
                for (QRegularExpressionMatch match = import.match(expanded); match.hasMatch(); match = import.match(expanded))
                {
                    expanded.replace(match.capturedStart(), match.capturedLength(), read(match.captured(1)));
                }
            }
            return expanded;
        }

        //! The device ratios of the comparisons: whole ones and the fractional ones of the Windows scale settings. The Native captures (n2)
        //! were taken at 1.5
        const std::initializer_list<qreal> Ratios = { 1.0, 1.25, 1.5, 1.75, 2.0, 2.5 };
        const std::initializer_list<Qt::LayoutDirection> Directions = { Qt::LeftToRight, Qt::RightToLeft };

        const QColor Grey(0x80, 0x80, 0x80);        //!< what an image starts as: a pixel that is still this was not drawn

        //! What a style draws for one call, over a grey image of an explicit device ratio: the painter is the image's, so the style scales its
        //! lines by that ratio
        template<typename Draw>
        QImage drawn(const QSize& size, qreal ratio, Draw draw)
        {
            QImage image((QSizeF(size) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
            image.setDevicePixelRatio(ratio);
            image.fill(Grey);
            QPainter painter(&image);
            draw(&painter);
            painter.end();
            return image;
        }

        //! The pixels that differ
        int differing(const QImage& a, const QImage& b)
        {
            if (a.size() != b.size())
            {
                return a.width() * a.height();
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

        std::string sizeText(const QSize& size)
        {
            return QStringLiteral("%1x%2").arg(size.width()).arg(size.height()).toStdString();
        }

        std::string rectText(const QRect& rect)
        {
            return QStringLiteral("(%1, %2, %3 x %4)").arg(rect.x()).arg(rect.y()).arg(rect.width()).arg(rect.height()).toStdString();
        }

        //! The option QPushButton::initStyleOption builds (qpushbutton.cpp) for a button in a state of the test's choosing, over the whole button
        QStyleOptionButton buttonOption(const QPushButton* button, QStyle::State state, const QSize& size)
        {
            QStyleOptionButton option;
            option.initFrom(button);
            option.state = state | QStyle::State_Active;
            option.palette.setCurrentColorGroup(state.testFlag(QStyle::State_Enabled) ? QPalette::Active : QPalette::Disabled);
            option.rect = QRect(QPoint(0, 0), size);
            option.text = button->text();
            option.icon = button->icon();
            option.iconSize = button->iconSize();
            option.features = QStyleOptionButton::None;
            return option;
        }

        //! The states a segment button is painted in (the pseudo-classes of SegmentControl.qss:68-77: :checked, :hover, :pressed, and the disabled
        //! state, which has no rule of its own)
        struct NamedState
        {
            const char* m_name;
            QStyle::State m_state;
        };
        const NamedState ButtonStates[] = {
            { "idle", QStyle::State_Enabled },
            { "hover", QStyle::State_Enabled | QStyle::State_MouseOver },
            { "pressed", QStyle::State_Enabled | QStyle::State_Sunken },
            { "pressed over", QStyle::State_Enabled | QStyle::State_Sunken | QStyle::State_MouseOver },
            { "checked", QStyle::State_Enabled | QStyle::State_On },
            { "checked over", QStyle::State_Enabled | QStyle::State_On | QStyle::State_MouseOver },
            { "checked pressed", QStyle::State_Enabled | QStyle::State_On | QStyle::State_Sunken },
            { "disabled", QStyle::State_None },
            { "disabled checked", QStyle::State_On },
            { "disabled over", QStyle::State_MouseOver },
        };
    } // namespace ChromeTwinTestsInternal

    // ----------------------------------------------------------------------------------
    // Fixture: the flattened Style over Fusion, Fusion as the base of the legacy sheet, the global elements registered
    // ----------------------------------------------------------------------------------

    class ChromeTwinTests
        : public ::UnitTest::LeakDetectionFixture
    {
    public:
        ChromeTwinTests()
        {
            m_application = AZStd::make_unique<QApplication>(m_argc, m_argv);

            // The application font (StyleManager.cpp:220-222), before the style: its class fonts are made at setStyle
            QFont applicationFont(QStringLiteral("Open Sans"));
            applicationFont.setPixelSize(12);
            QApplication::setFont(applicationFont);

            // As StyleManager::initialize builds it (StyleManager.cpp:244-246); the application owns it
            QApplication::setStyle(new Style(QStyleFactory::create(QStringLiteral("Fusion"))));
            m_style = QApplication::style();

            // A widget under a style sheet is given this style first: Qt's sheet style then draws over Fusion alone, as it did over the base of
            // the legacy GUI, and never over the flattened style
            m_fusion.reset(QStyleFactory::create(QStringLiteral("Fusion")));
            ControlElements::registerGlobal();
        }

        ~ChromeTwinTests() override
        {
            m_fusion.reset();
            m_application.reset();
        }

        //! The legacy twin: the sheet on the root, over a Fusion of its own. The flat twin: the style on the root and on everything under
        //! it, which re-polishes what was polished (a widget's style is not its children's).
        void legacy(QWidget& root) const
        {
            root.setStyle(m_fusion.get());
            root.setStyleSheet(ChromeTwinTestsInternal::appSheet());
        }

        void flat(QWidget& root) const
        {
            root.setStyle(m_style);
            for (QWidget* child : root.findChildren<QWidget*>())
            {
                child->setStyle(m_style);
            }
        }

        static void show(QWidget& root, const QSize& size)
        {
            root.setAttribute(Qt::WA_DontShowOnScreen);
            root.resize(size);
            root.show();
            QApplication::processEvents();
        }

        //! One tree twice. `build(root, style)` fills a root with it: the widgets a window builds before it is put on the screen. The style is
        //! the one a widget of the tree is born with, which the legacy twin leaves to the sheet (a widget made with a parent has the style of
        //! its parent: the sheet of the application is on before any window is built, so the sheet is put on the root first) and the flat twin
        //! sets, as the flattened style is the style of the application in the real GUI. A sheet put on after the tree was built would
        //! polish all of it again, which no window of the GUI is built to. `late(root)` changes the shown tree: what a window adds to it once
        //! it is shown.
        struct Twins
        {
            template<typename Build, typename Late>
            Twins(const ChromeTwinTests& tests, Qt::LayoutDirection direction, Build build, Late late, const QSize& size)
            {
                m_legacy.setLayoutDirection(direction);
                tests.legacy(m_legacy);
                build(m_legacy, nullptr);
                show(m_legacy, size);
                late(m_legacy);
                QApplication::processEvents();
                m_flat.setLayoutDirection(direction);
                m_flat.setStyle(tests.m_style);
                build(m_flat, tests.m_style);
                show(m_flat, size);
                late(m_flat);
                QApplication::processEvents();
            }

            template<typename W>
            W* legacyChild(const char* name)
            {
                return m_legacy.template findChild<W*>(QString::fromLatin1(name));
            }

            template<typename W>
            W* flatChild(const char* name)
            {
                return m_flat.template findChild<W*>(QString::fromLatin1(name));
            }

            QWidget m_legacy;
            QWidget m_flat;
        };

    protected:
        int m_argc = 1;                                     // QApplication keeps a reference to it
        char m_programName[5] = "test";
        char* m_argv[2] = { m_programName, nullptr };
        AZStd::unique_ptr<QApplication> m_application;
        QStyle* m_style = nullptr;                          //!< the application's: the flattened Style over Fusion
        AZStd::unique_ptr<QStyle> m_fusion;                 //!< the base of the legacy twins
    };

    // ----------------------------------------------------------------------------------
    // S: the buttons of a segment bar (SegmentControl.qss:14-77). The Native captures n2 hold four bars: the Gallery's (4 tabs, 3 tabs, each
    // horizontal and vertical), the Style Sheet page's, the Asset Processor's navigation (vertical, 9 tabs, added after the window was shown)
    // and the Editor's (2 tabs).
    // ----------------------------------------------------------------------------------

    namespace ChromeTwinTestsInternal
    {
        //! A segment bar whose buttons are born with the style the test gives them (a style is not inherited by a widget the way a sheet is,
        //! so a button created later would have Fusion's)
        class StyledSegmentBar
            : public SegmentBar
        {
        public:
            StyledSegmentBar(Qt::Orientation orientation, QWidget* parent, QStyle* style)
                : SegmentBar(orientation, parent)
                , m_style(style)
            {
                if (m_style)
                {
                    setStyle(m_style);
                }
            }

        protected:
            QAbstractButton* createButton(const QIcon& icon, const QString& text) override
            {
                QAbstractButton* button = SegmentBar::createButton(icon, text);
                if (m_style)
                {
                    button->setStyle(m_style);
                }
                return button;
            }

        private:
            QStyle* m_style;
        };

        //! A bar the way the Gallery builds one (SegmentControlPage: the tabs are added to the bar while it is not shown yet)
        void buildBar(QWidget& root, QStyle* style, Qt::Orientation orientation, const QStringList& labels, bool withIcons, bool inLayout = false)
        {
            auto* bar = new StyledSegmentBar(orientation, &root, style);
            bar->setObjectName(QStringLiteral("bar"));
            if (inLayout)
            {
                // A page of the Gallery, or the window of the Asset Processor, holds its bar in a layout: it is sized from the hints the layout cached
                auto* layout = new QVBoxLayout(&root);
                layout->setContentsMargins(10, 6, 0, 0);
                layout->addWidget(bar, 0, Qt::AlignLeft | Qt::AlignTop);
            }
            else
            {
                bar->move(10, 6);
            }
            for (const QString& label : labels)
            {
                if (withIcons)
                {
                    QPixmap pixmap(17, 17);
                    pixmap.fill(QColor(0xFF, 0x00, 0xFF));
                    bar->addTab(QIcon(pixmap), QString());
                }
                else
                {
                    bar->addTab(label);
                }
            }
        }

        QList<QPushButton*> buttonsOf(QWidget& root)
        {
            return root.findChild<SegmentBar*>(QStringLiteral("bar"))->findChildren<QPushButton*>();
        }

        const QStringList FourLabels = { QStringLiteral("Segment Bar 0"), QStringLiteral("Segment Bar 1"), QStringLiteral("Red"), QStringLiteral("A longer label") };

        //! The ways a window of the GUI comes by the tabs of its bar. The sheet of the application is on while a window is built, and QWidget::setParent
        //! of a widget polished by the sheet polishes it and everything under it again (qwidget.cpp:10866, 2680-2778), which gives a button the
        //! minimum width of the rule (QStyleSheetStyle::setGeometry); SegmentBar::updateTabs polishes the buttons as soon as the bar has them
        //! (it asks them for their hint) and then makes them all as wide as the widest. The Native captures (n2) hold each flow:
        enum class Flow
        {
            Settled,            //!< the bar is built where it stays and gets its tabs there: Script Canvas logging window, all as wide as the widest hint
            AncestorMoved,      //!< a widget above the bar is put into another one once it has its tabs: the Gallery adds its pages to a stack, 102, 101, 101, 101
            BarMoved,           //!< a segment control is built without a parent and put into its window after its tabs: PhysX collision filtering, 102, 101
            TabsAfterShow,      //!< the window is shown, the tabs come after: the Asset Processor, all as wide as the widest hint
        };

        const char* flowName(Flow flow)
        {
            switch (flow)
            {
            case Flow::Settled:
                return "settled";
            case Flow::AncestorMoved:
                return "a page moved";
            case Flow::BarMoved:
                return "the bar moved";
            default:
                return "tabs after show";
            }
        }

        //! The flows the legacy twin can follow: a control built without a parent has the style of the application before it has a parent, which
        //! is the flattened one in these tests
        const std::initializer_list<Flow> TwinFlows = { Flow::Settled, Flow::AncestorMoved, Flow::TabsAfterShow };

        //! A window with a page that holds a bar (in a layout of its own or free), and a stack the page can be put into
        void buildFlow(QWidget& root, QStyle* style, Qt::Orientation orientation, Flow flow, bool inLayout, const QStringList& labels)
        {
            auto* page = new QWidget(&root);
            page->setObjectName(QStringLiteral("page"));
            page->setGeometry(0, 0, 700, 300);
            auto* stack = new QWidget(&root);
            stack->setObjectName(QStringLiteral("stack"));
            stack->setGeometry(0, 0, 700, 300);
            QVBoxLayout* layout = nullptr;
            if (inLayout)
            {
                layout = new QVBoxLayout(page);
                layout->setContentsMargins(10, 6, 0, 0);
            }

            // PhysX builds a segment control with no parent and puts it into its window after its tabs. The bar inside it has the control for a
            // parent: a bar that is a window itself is polished at its first tab, as QLayout::totalMinimumSize polishes the parent of a top level
            // layout, and no such bar is built so
            QWidget* put = nullptr;                 // what goes into the page
            SegmentBar* bar = nullptr;
            if (flow == Flow::BarMoved)
            {
                auto* control = new SegmentControl(orientation == Qt::Vertical ? SegmentControl::West : SegmentControl::North);
                bar = control->findChild<SegmentBar*>();
                for (const QString& label : labels)
                {
                    control->addTab(new QWidget(control), label);
                }
                put = control;
            }
            else
            {
                bar = new StyledSegmentBar(orientation, page, style);
                if (flow != Flow::TabsAfterShow)
                {
                    for (const QString& label : labels)
                    {
                        bar->addTab(label);
                    }
                }
                put = bar;
            }
            bar->setObjectName(QStringLiteral("bar"));
            if (layout)
            {
                layout->addWidget(put, 0, Qt::AlignLeft | Qt::AlignTop);      // a control without a parent is put into the page here
            }
            else
            {
                if (flow == Flow::BarMoved)
                {
                    put->setParent(page);       // only what has no parent yet: Qt polishes the widgets under a widget that is given a parent that
                                                // is not shown yet, whether it is the same parent or not (QWidget::setParent, newParent)
                    put->resize(600, 200);
                }
                put->move(10, 6);
            }
            if (flow == Flow::AncestorMoved)
            {
                page->setParent(stack);
            }
        }

        //! What a window does once it is shown
        void addTabsAfterShow(QWidget& root, Flow flow, const QStringList& labels)
        {
            if (flow == Flow::TabsAfterShow)
            {
                for (const QString& label : labels)
                {
                    root.findChild<SegmentBar*>(QStringLiteral("bar"))->addTab(label);
                }
            }
        }
    } // namespace ChromeTwinTestsInternal

    TEST_F(ChromeTwinTests, SegmentBar_ButtonsHaveTheSheetsHintsLimitsAndGeometryInEveryFlowOfAWindow)
    {
        using namespace ChromeTwinTestsInternal;
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());   // the test environment sets it (AzQtComponentTests.cpp)
        ASSERT_TRUE(appSheet().contains(QStringLiteral("AzQtComponents--SegmentBar QAbstractButton")));

        for (const Qt::LayoutDirection direction : Directions)
        {
            for (const Qt::Orientation orientation : { Qt::Horizontal, Qt::Vertical })
            {
                for (const Flow flow : TwinFlows)
                {
                    for (const bool inLayout : { false, true })
                    {
                        SCOPED_TRACE(QStringLiteral("%1, %2, %3, bar %4")
                                         .arg(direction == Qt::RightToLeft ? "right to left" : "left to right")
                                         .arg(orientation == Qt::Vertical ? "vertical" : "horizontal")
                                         .arg(flowName(flow))
                                         .arg(inLayout ? "in a layout" : "free").toStdString());
                        Twins twins(*this, direction,
                            [&](QWidget& root, QStyle* style) { buildFlow(root, style, orientation, flow, inLayout, FourLabels); },
                            [&](QWidget& root) { addTabsAfterShow(root, flow, FourLabels); },
                            QSize(700, 300));

                        const QList<QPushButton*> legacyButtons = buttonsOf(twins.m_legacy);
                        const QList<QPushButton*> flatButtons = buttonsOf(twins.m_flat);
                        ASSERT_EQ(legacyButtons.size(), 4);
                        ASSERT_EQ(flatButtons.size(), 4);

                        const SegmentBar* legacyBar = twins.m_legacy.findChild<SegmentBar*>();
                        const SegmentBar* flatBar = twins.m_flat.findChild<SegmentBar*>();
                        EXPECT_EQ(flatBar->sizeHint(), legacyBar->sizeHint()) << sizeText(flatBar->sizeHint()) << " " << sizeText(legacyBar->sizeHint());
                        EXPECT_EQ(flatBar->minimumSizeHint(), legacyBar->minimumSizeHint());
                        EXPECT_EQ(flatBar->geometry(), legacyBar->geometry()) << rectText(flatBar->geometry()) << " " << rectText(legacyBar->geometry());
                        EXPECT_EQ(flatBar->contentsMargins(), legacyBar->contentsMargins());

                        for (int i = 0; i < 4; ++i)
                        {
                            const QPushButton* legacyButton = legacyButtons.at(i);
                            const QPushButton* flatButton = flatButtons.at(i);
                            EXPECT_EQ(flatButton->sizeHint(), legacyButton->sizeHint()) << "button " << i << " hint " << sizeText(flatButton->sizeHint());
                            EXPECT_EQ(flatButton->minimumSizeHint(), legacyButton->minimumSizeHint()) << "button " << i;
                            EXPECT_EQ(flatButton->minimumSize(), legacyButton->minimumSize()) << "button " << i << " minimum " << sizeText(flatButton->minimumSize());
                            EXPECT_EQ(flatButton->maximumSize(), legacyButton->maximumSize()) << "button " << i;
                            EXPECT_EQ(flatButton->geometry(), legacyButton->geometry()) << "button " << i << " " << rectText(flatButton->geometry())
                                                                                          << " " << rectText(legacyButton->geometry());
                            EXPECT_TRUE(flatButton->testAttribute(Qt::WA_Hover));
                        }
                    }
                }
            }
        }
    }

    TEST_F(ChromeTwinTests, SegmentBar_TheBevelIsTheSheetsInEveryPlaceStateAndRatio)
    {
        using namespace ChromeTwinTestsInternal;
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        for (const Qt::LayoutDirection direction : Directions)
        {
            for (const Qt::Orientation orientation : { Qt::Horizontal, Qt::Vertical })
            {
                SCOPED_TRACE(QStringLiteral("%1, %2").arg(direction == Qt::RightToLeft ? "right to left" : "left to right")
                                 .arg(orientation == Qt::Vertical ? "vertical" : "horizontal").toStdString());
                Twins twins(*this, direction, [&](QWidget& root, QStyle* style) { buildBar(root, style, orientation, FourLabels, false); }, [](QWidget&) {}, QSize(700, 300));
                const QList<QPushButton*> legacyButtons = buttonsOf(twins.m_legacy);
                const QList<QPushButton*> flatButtons = buttonsOf(twins.m_flat);
                ASSERT_EQ(legacyButtons.size(), 4);
                ASSERT_EQ(flatButtons.size(), 4);

                for (int i = 0; i < 4; ++i)
                {
                    // TabFirst, TabMiddle, TabMiddle, TabLast. The size of the widget: the images of both twins are the same size
                    const QSize size = legacyButtons.at(i)->size();
                    ASSERT_EQ(flatButtons.at(i)->size(), size);
                    for (const NamedState& named : ButtonStates)
                    {
                        for (const qreal ratio : Ratios)
                        {
                            const auto paint = [&](QPushButton* button, QStyle::ControlElement element)
                            {
                                const QStyleOptionButton option = buttonOption(button, named.m_state, size);
                                return drawn(size, ratio, [&](QPainter* painter) { button->style()->drawControl(element, &option, painter, button); });
                            };
                            const QImage legacyImage = paint(legacyButtons.at(i), QStyle::CE_PushButtonBevel);
                            const QImage flatImage = paint(flatButtons.at(i), QStyle::CE_PushButtonBevel);
                            EXPECT_EQ(differing(legacyImage, flatImage), 0) << "button " << i << ", " << named.m_name << ", ratio " << ratio;
                        }
                    }
                }
            }
        }
    }

    TEST_F(ChromeTwinTests, SegmentBar_TheMinimumWidthsAreTheNativeOnesOfTheCapturesInEveryFlowOfAWindow)
    {
        using namespace ChromeTwinTestsInternal;
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        // The Native captures (n2), one bar of each flow. The rule's `min-width: 68px` with the border and the padding is 102, and 101 for a button
        // that overlaps its neighbour in a horizontal bar. SegmentBar::updateTabs made every button as wide as the widest hint, and the sheet took
        // that back only where a widget above the bar was put into another after the tabs were added.
        //   Editor 04_script_canvas__dock-Debugger (Entities, Graphs):        102, 102   the bar was never moved: the widest hint, 102
        //   Gallery 20_segment-control (Segment Bar 0..3, Red, Green, Blue):  102, 101, 101, 101 (vertical: 102 for all)   the page was put into the stack
        //   Editor PhysX Collision Filtering (Layers, Groups):                102, 101   the control was built free and put into its window
        //   Asset Processor navigation (9 tabs, vertical):                    126 for all   the tabs were added once the window was shown
        // The widest hint depends on the font of the test (the two labels of the Editor are short in the capture's font); the rule's 102 and 101 do not.
        struct Capture
        {
            const char* m_name;
            Flow m_flow;
            Qt::Orientation m_orientation;
            QStringList m_labels;
            std::vector<int> m_minimums;        //!< empty: all as wide as the widest hint
        };
        const std::vector<Capture> captures = {
            { "Script Canvas debugger", Flow::Settled, Qt::Horizontal, { QStringLiteral("Entities"), QStringLiteral("Graphs") }, {} },
            { "Gallery, horizontal", Flow::AncestorMoved, Qt::Horizontal, FourLabels, { 102, 101, 101, 101 } },
            { "Gallery, vertical", Flow::AncestorMoved, Qt::Vertical, FourLabels, { 102, 102, 102, 102 } },
            { "PhysX collision filtering", Flow::BarMoved, Qt::Horizontal, { QStringLiteral("Layers"), QStringLiteral("Groups") }, { 102, 101 } },
            { "Asset Processor", Flow::TabsAfterShow, Qt::Vertical, FourLabels, {} },
        };
        for (const bool inLayout : { false, true })
        {
            for (const Capture& capture : captures)
            {
                SCOPED_TRACE(QStringLiteral("%1, bar %2").arg(capture.m_name).arg(inLayout ? "in a layout" : "free").toStdString());
                QWidget root;
                root.setStyle(m_style);
                buildFlow(root, m_style, capture.m_orientation, capture.m_flow, inLayout, capture.m_labels);
                show(root, QSize(700, 300));
                addTabsAfterShow(root, capture.m_flow, capture.m_labels);
                QApplication::processEvents();

                const QList<QPushButton*> buttons = buttonsOf(root);
                ASSERT_EQ(buttons.size(), capture.m_labels.size());
                int widest = 0;
                for (const QPushButton* button : buttons)
                {
                    widest = qMax(widest, button->sizeHint().width());
                }
                for (int i = 0; i < buttons.size(); ++i)
                {
                    const int expected = capture.m_minimums.empty() ? widest : capture.m_minimums.at(i);
                    EXPECT_EQ(buttons.at(i)->minimumWidth(), expected) << "button " << i;
                }
            }
        }
    }

    TEST_F(ChromeTwinTests, SegmentBar_ABarOfOneAndOfTwoButtonsHasTheSheetsBevelToo)
    {
        using namespace ChromeTwinTestsInternal;
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        // TabOne has no rule of its own (the base rule alone: square, no overlap) and a bar of two is a TabFirst and a TabLast
        for (const Qt::Orientation orientation : { Qt::Horizontal, Qt::Vertical })
        {
            for (const int count : { 1, 2 })
            {
                SCOPED_TRACE(QStringLiteral("%1, %2 buttons").arg(orientation == Qt::Vertical ? "vertical" : "horizontal").arg(count).toStdString());
                const QStringList labels = FourLabels.mid(0, count);
                Twins twins(*this, Qt::LeftToRight, [&](QWidget& root, QStyle* style) { buildBar(root, style, orientation, labels, false); }, [](QWidget&) {},
                    QSize(700, 300));
                const QList<QPushButton*> legacyButtons = buttonsOf(twins.m_legacy);
                const QList<QPushButton*> flatButtons = buttonsOf(twins.m_flat);
                ASSERT_EQ(legacyButtons.size(), count);
                ASSERT_EQ(flatButtons.size(), count);
                for (int i = 0; i < count; ++i)
                {
                    EXPECT_EQ(flatButtons.at(i)->sizeHint(), legacyButtons.at(i)->sizeHint());
                    EXPECT_EQ(flatButtons.at(i)->geometry(), legacyButtons.at(i)->geometry());
                    const QSize size = legacyButtons.at(i)->size();
                    ASSERT_EQ(flatButtons.at(i)->size(), size);
                    for (const NamedState& named : ButtonStates)
                    {
                        for (const qreal ratio : Ratios)
                        {
                            const auto paint = [&](QPushButton* button)
                            {
                                const QStyleOptionButton option = buttonOption(button, named.m_state, size);
                                return drawn(size, ratio, [&](QPainter* painter) { button->style()->drawControl(QStyle::CE_PushButtonBevel, &option, painter, button); });
                            };
                            EXPECT_EQ(differing(paint(legacyButtons.at(i)), paint(flatButtons.at(i))), 0) << "button " << i << ", " << named.m_name << ", ratio " << ratio;
                        }
                    }
                }
            }
        }
    }

    TEST_F(ChromeTwinTests, SegmentBar_TheBorderIsOneAndAHalfDevicePixelsOnEverySideAtAFractionalRatio)
    {
        using namespace ChromeTwinTestsInternal;
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        // Native (n2 Gallery 20_segment-control at 150%): the border of a button is #222222 at 1.5 device pixels on every side, a whole row (or
        // column) and a blended one inside it. A line stroked inside the clip of the widget left a half pixel on two sides only: the "embossed" look
        // the user reported. The first button of a horizontal bar is round at its left corners (3px, SegmentControl.qss:46-47) and square at its right.
        Twins twins(*this, Qt::LeftToRight, [&](QWidget& root, QStyle* style) { buildBar(root, style, Qt::Horizontal, FourLabels, false); }, [](QWidget&) {},
            QSize(700, 300));
        QPushButton* first = buttonsOf(twins.m_flat).first();
        const QSize size(102, 28);          // 153 x 42 device pixels at 150%: whole, so that no edge is cut by the image
        const QStyleOptionButton option = buttonOption(first, QStyle::State_Enabled | QStyle::State_On, size);
        const QImage image = drawn(size, 1.5, [&](QPainter* painter) { first->style()->drawControl(QStyle::CE_PushButtonBevel, &option, painter, first); });
        ASSERT_EQ(image.size(), QSize(153, 42));

        const QColor line(0x22, 0x22, 0x22);
        const QColor fill(0x55, 0x55, 0x55);
        const auto isBlend = [&](const QColor& pixel)
        {
            return pixel.red() > line.red() && pixel.red() < fill.red();
        };
        const int middleX = 76;
        const int middleY = 21;
        EXPECT_TRUE(image.pixelColor(middleX, 0) == line);
        EXPECT_TRUE(image.pixelColor(middleX, 41) == line);
        EXPECT_TRUE(image.pixelColor(0, middleY) == line);
        EXPECT_TRUE(image.pixelColor(152, middleY) == line);
        EXPECT_TRUE(isBlend(image.pixelColor(middleX, 1)));
        EXPECT_TRUE(isBlend(image.pixelColor(middleX, 40)));
        EXPECT_TRUE(isBlend(image.pixelColor(1, middleY)));
        EXPECT_TRUE(isBlend(image.pixelColor(151, middleY)));
        EXPECT_TRUE(image.pixelColor(middleX, 2) == fill);
        EXPECT_TRUE(image.pixelColor(2, middleY) == fill);
        // The round corners leave the corner pixel to what is behind the button, the square ones to the border
        EXPECT_TRUE(image.pixelColor(0, 0) == Grey);
        EXPECT_TRUE(image.pixelColor(0, 41) == Grey);
        EXPECT_TRUE(image.pixelColor(152, 0) == line);
        EXPECT_TRUE(image.pixelColor(152, 41) == line);
    }

    TEST_F(ChromeTwinTests, SegmentBar_TheLabelSitsInTheContentsRectOfTheRule)
    {
        using namespace ChromeTwinTestsInternal;
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        // A button with an icon and no text, so that no font is involved: Qt centres the icon in the contents rect it is given (the rule's
        // box, which the negative margin of a following button moves: the icon of such a button sits half a pixel to the left of its middle).
        // The icon is 17px wide, so that the half pixel shows as a whole one where Qt rounds the centre down.
        for (const Qt::LayoutDirection direction : Directions)
        {
            SCOPED_TRACE(direction == Qt::RightToLeft ? "right to left" : "left to right");
            Twins twins(*this, direction, [&](QWidget& root, QStyle* style) { buildBar(root, style, Qt::Horizontal, FourLabels, true); },
                [](QWidget& root)
                {
                    for (QPushButton* button : buttonsOf(root))
                    {
                        button->setIconSize(QSize(17, 17));
                    }
                },
                QSize(700, 300));
            const QList<QPushButton*> legacyButtons = buttonsOf(twins.m_legacy);
            const QList<QPushButton*> flatButtons = buttonsOf(twins.m_flat);
            ASSERT_EQ(legacyButtons.size(), 4);
            ASSERT_EQ(flatButtons.size(), 4);

            for (int i = 0; i < 4; ++i)
            {
                const QSize size = legacyButtons.at(i)->size();
                ASSERT_EQ(flatButtons.at(i)->size(), size);
                for (const NamedState& named : ButtonStates)
                {
                    for (const qreal ratio : Ratios)
                    {
                        const auto paint = [&](QPushButton* button)
                        {
                            const QStyleOptionButton option = buttonOption(button, named.m_state, size);
                            return drawn(size, ratio, [&](QPainter* painter) { button->style()->drawControl(QStyle::CE_PushButton, &option, painter, button); });
                        };
                        EXPECT_EQ(differing(paint(legacyButtons.at(i)), paint(flatButtons.at(i))), 0) << "button " << i << ", " << named.m_name << ", ratio " << ratio;
                    }
                }
            }
        }
    }
    // ----------------------------------------------------------------------------------
    // F: a rule a component draws itself, against the sheet's drawRule (BoxElement::drawFrame)
    // ----------------------------------------------------------------------------------

    TEST_F(ChromeTwinTests, DrawFrame_IsTheSheetsFillAndBorderForEveryCombinationOfCorners)
    {
        using namespace ChromeTwinTestsInternal;

        QWidget root;
        root.setStyle(m_fusion.get());
        auto* button = new QPushButton(&root);
        button->setObjectName(QStringLiteral("framed"));
        root.setAttribute(Qt::WA_DontShowOnScreen);
        root.resize(100, 60);
        button->setGeometry(0, 0, 100, 60);

        // top left, top right, bottom right, bottom left: the order of BoxElement::Radii
        const BoxElement::Radii cornerSets[] = {
            { 0, 0, 0, 0 }, { 3, 0, 0, 3 }, { 0, 3, 3, 0 }, { 3, 3, 3, 3 }, { 4, 0, 0, 0 }, { 0, 0, 5, 0 }, { 3, 3, 0, 0 }, { 0, 0, 3, 3 }, { 2, 5, 7, 3 },
        };
        // some of them too small for their corners: a corner that does not fit is square (qNormalizeRadii)
        const QSize sizes[] = { QSize(40, 20), QSize(5, 20), QSize(30, 6), QSize(61, 29) };
        const QColor fill(0x55, 0x55, 0x55);
        const QColor line(0x22, 0x22, 0x22);
        for (const BoxElement::Radii& radii : cornerSets)
        {
            for (const int width : { 1, 2, 3 })
            {
                root.setStyleSheet(QStringLiteral("#framed { background-color: %1; border: %2px solid %3; border-top-left-radius: %4px;"
                                                  " border-top-right-radius: %5px; border-bottom-right-radius: %6px; border-bottom-left-radius: %7px; }")
                                       .arg(fill.name()).arg(width).arg(line.name()).arg(radii.m_topLeft).arg(radii.m_topRight).arg(radii.m_bottomRight)
                                       .arg(radii.m_bottomLeft));
                button->ensurePolished();
                for (const QSize& size : sizes)
                {
                    for (const qreal ratio : Ratios)
                    {
                        QStyleOptionButton option;
                        option.initFrom(button);
                        option.rect = QRect(QPoint(0, 0), size);
                        const QImage legacyImage = drawn(size, ratio, [&](QPainter* painter) { button->style()->drawControl(QStyle::CE_PushButtonBevel, &option, painter, button); });
                        const QImage flatImage = drawn(size, ratio, [&](QPainter* painter)
                            {
                                BoxElement::drawFrame(painter, QRect(QPoint(0, 0), size), QBrush(fill), BoxElement::border(width, line), radii);
                            });
                        EXPECT_EQ(differing(legacyImage, flatImage), 0)
                            << "radii " << radii.m_topLeft << " " << radii.m_topRight << " " << radii.m_bottomRight << " " << radii.m_bottomLeft << ", border "
                            << width << ", size " << sizeText(size) << ", ratio " << ratio;
                    }
                }
            }
        }
    }

    // ----------------------------------------------------------------------------------
    // E: the edges of a tab element, antialiased as the sheet drew them (TabElement::drawEdges)
    // ----------------------------------------------------------------------------------

    namespace ChromeTwinTestsInternal
    {
        struct EdgeCase
        {
            std::string m_name;
            TabElement::Edges m_edges;
        };

        //! The subset of four edges a bit mask names, each edge `width` wide and of a colour of its own
        TabElement::Edges edgesOf(int subset, int width, const QColor& top, const QColor& right, const QColor& bottom, const QColor& left)
        {
            TabElement::Edges edges;
            if (subset & 1)
            {
                edges.m_top = { width, top };
            }
            if (subset & 2)
            {
                edges.m_right = { width, right };
            }
            if (subset & 4)
            {
                edges.m_bottom = { width, bottom };
            }
            if (subset & 8)
            {
                edges.m_left = { width, left };
            }
            return edges;
        }

        //! The border declarations of a rule that has these edges
        QString borderRuleOf(const TabElement::Edges& edges)
        {
            const auto side = [](const char* name, const TabElement::Edge& edge)
            {
                return edge.m_width > 0 ? QStringLiteral("border-%1: %2px solid %3;").arg(QLatin1String(name)).arg(edge.m_width).arg(edge.m_color.name())
                                        : QStringLiteral("border-%1: none;").arg(QLatin1String(name));
            };
            return side("top", edges.m_top) + side("right", edges.m_right) + side("bottom", edges.m_bottom) + side("left", edges.m_left);
        }

        //! The sets of edges Native draws as plain rectangles in qDrawBorder's order (qDrawEdge, qcssutil.cpp:165-195: an edge of 1px, or one whose
        //! neighbours are none or of its own colour, is a rectangle; a wider edge next to an edge of another colour is a trapezoid, which no element has):
        //! every subset of four edges of 1px in colours of their own, every subset of wider edges in one colour, and the sets the elements hold
        std::vector<EdgeCase> edgeCases()
        {
            const QColor black(0x11, 0x11, 0x11);
            const QColor grey(0x44, 0x44, 0x44);
            const QColor blue(0x00, 0x8D, 0xE5);
            const QColor white(0xFF, 0xFF, 0xFF);
            std::vector<EdgeCase> cases;
            for (int subset = 0; subset < 16; ++subset)
            {
                cases.push_back({ "1px, own colours, subset " + std::to_string(subset), edgesOf(subset, 1, black, grey, blue, white) });
            }
            for (int subset = 1; subset < 16; ++subset)
            {
                cases.push_back({ "2px, one colour, subset " + std::to_string(subset), edgesOf(subset, 2, black, black, black, black) });
                cases.push_back({ "3px, one colour, subset " + std::to_string(subset), edgesOf(subset, 3, blue, blue, blue, blue) });
            }
            // TabWidget.qss:130 (the bar), :152 and :158 (a tab: a line above and an underline below), :111 (the pane line), :82 (a selected tab)
            cases.push_back({ "bar: top and left 1px", { { 1, black }, {}, {}, { 1, black } } });
            cases.push_back({ "tab: top 1px, underline 3px", { { 1, black }, {}, { 3, blue }, {} } });
            cases.push_back({ "tab: underline 3px, own colour", { {}, {}, { 3, grey }, {} } });
            cases.push_back({ "pane: top 2px", { { 2, QColor(0x2B, 0x2B, 0x2B) }, {}, {}, {} } });
            cases.push_back({ "primary selected: black sides, grey underline 1px", { { 1, black }, { 1, black }, { 1, grey }, { 1, black } } });
            return cases;
        }

        enum class TabLook
        {
            Primary,
            Secondary,
            SecondaryBordered,
        };

        //! A tab widget of one of the three looks the Gallery shows (TabWidgetPage)
        void buildTabs(QWidget& root, TabLook look)
        {
            auto* tabs = new TabWidget(&root);
            tabs->setObjectName(QStringLiteral("tabs"));
            tabs->addTab(new QWidget, QStringLiteral("Tab A"));
            tabs->addTab(new QWidget, QStringLiteral("Tab B"));
            tabs->addTab(new QWidget, QStringLiteral("Tab C"));
            if (look != TabLook::Primary)
            {
                TabWidget::applySecondaryStyle(tabs, look == TabLook::SecondaryBordered);
            }
            tabs->setGeometry(5, 5, 400, 120);
        }
    } // namespace ChromeTwinTestsInternal

    TEST_F(ChromeTwinTests, TabEdges_AreTheSheetsBorderForEverySetOfEdgesNativeDrawsAsRectangles)
    {
        using namespace ChromeTwinTestsInternal;

        QWidget root;
        root.setStyle(m_fusion.get());
        auto* button = new QPushButton(&root);
        button->setObjectName(QStringLiteral("edged"));
        root.setAttribute(Qt::WA_DontShowOnScreen);
        root.resize(100, 60);
        button->setGeometry(0, 0, 100, 60);

        const QColor fill(0x33, 0x33, 0x33);
        for (const EdgeCase& edgeCase : edgeCases())
        {
            root.setStyleSheet(QStringLiteral("#edged { background-color: %1; %2 }").arg(fill.name(), borderRuleOf(edgeCase.m_edges)));
            button->ensurePolished();
            for (const QSize& size : { QSize(40, 20), QSize(62, 31), QSize(61, 30) })
            {
                for (const qreal ratio : Ratios)
                {
                    QStyleOptionButton option;
                    option.initFrom(button);
                    option.rect = QRect(QPoint(0, 0), size);
                    const QImage legacyImage = drawn(size, ratio, [&](QPainter* painter) { button->style()->drawControl(QStyle::CE_PushButtonBevel, &option, painter, button); });
                    const QImage flatImage = drawn(size, ratio, [&](QPainter* painter)
                        {
                            painter->fillRect(QRect(QPoint(0, 0), size), fill);
                            TabElement::drawEdges(painter, QRect(QPoint(0, 0), size), edgeCase.m_edges);
                        });
                    EXPECT_EQ(differing(legacyImage, flatImage), 0) << edgeCase.m_name << ", size " << sizeText(size) << ", ratio " << ratio;
                }
            }
        }
    }

    TEST_F(ChromeTwinTests, TabEdges_AreAWholeDevicePixelAndABlendedOneAtAFractionalRatioAsNativeDrawsThem)
    {
        using namespace ChromeTwinTestsInternal;

        // The values of the Native capture (n2 Gallery 27_tab-widget at 150%): a 1px #111111 edge over #444444 is 17 then 42 across its width
        // (a whole device pixel and half of the next), where the fill with no antialiasing of the flat capture (f5) made 17 and 17. These are
        // numbers, not a twin: they hold whatever the sheet style does in a test process.
        const QColor edge(0x11, 0x11, 0x11);
        const QColor fill(0x44, 0x44, 0x44);
        TabElement::Edges edges;
        edges.m_left = { 1, edge };
        edges.m_right = { 1, edge };
        edges.m_top = { 1, edge };
        edges.m_bottom = { 1, edge };
        const QSize size(40, 20);               // 60 x 30 device pixels
        const QImage image = drawn(size, 1.5, [&](QPainter* painter)
            {
                painter->fillRect(QRect(QPoint(0, 0), size), fill);
                TabElement::drawEdges(painter, QRect(QPoint(0, 0), size), edges);
            });
        ASSERT_EQ(image.size(), QSize(60, 30));
        // Left and top: the whole pixel is outside, the blended one inside it
        EXPECT_EQ(image.pixelColor(0, 15).red(), 17);
        EXPECT_EQ(image.pixelColor(1, 15).red(), 42);
        EXPECT_EQ(image.pixelColor(2, 15).red(), 68);
        EXPECT_EQ(image.pixelColor(30, 0).red(), 17);
        EXPECT_EQ(image.pixelColor(30, 1).red(), 42);
        EXPECT_EQ(image.pixelColor(30, 2).red(), 68);
        // Right and bottom: the same from the other side
        EXPECT_EQ(image.pixelColor(59, 15).red(), 17);
        EXPECT_EQ(image.pixelColor(58, 15).red(), 42);
        EXPECT_EQ(image.pixelColor(57, 15).red(), 68);
        EXPECT_EQ(image.pixelColor(30, 29).red(), 17);
        EXPECT_EQ(image.pixelColor(30, 28).red(), 42);
        EXPECT_EQ(image.pixelColor(30, 27).red(), 68);
    }

    TEST_F(ChromeTwinTests, TabEdges_TheTabsBarAndPaneOfEveryLookHaveTheSheetsEdges)
    {
        using namespace ChromeTwinTestsInternal;
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());
        ASSERT_TRUE(appSheet().contains(QStringLiteral("AzQtComponents--TabBar.Secondary.Bordered")));

        for (const TabLook look : { TabLook::Primary, TabLook::Secondary, TabLook::SecondaryBordered })
        {
            SCOPED_TRACE(look == TabLook::Primary ? "primary" : (look == TabLook::Secondary ? "secondary" : "secondary, bordered"));
            QWidget legacyRoot;
            buildTabs(legacyRoot, look);
            legacy(legacyRoot);
            show(legacyRoot, QSize(420, 140));
            QWidget flatRoot;
            buildTabs(flatRoot, look);
            flat(flatRoot);
            show(flatRoot, QSize(420, 140));

            auto* legacyTabs = legacyRoot.findChild<TabWidget*>();
            auto* flatTabs = flatRoot.findChild<TabWidget*>();
            ASSERT_TRUE(legacyTabs != nullptr);
            ASSERT_TRUE(flatTabs != nullptr);
            QTabBar* legacyBar = legacyTabs->findChild<QTabBar*>();
            QTabBar* flatBar = flatTabs->findChild<QTabBar*>();
            ASSERT_TRUE(legacyBar != nullptr);
            ASSERT_TRUE(flatBar != nullptr);

            // The tab shapes in each state, at rects a few device pixels apart: the tabs of a bar start on whole and on half device pixels
            const NamedState tabStates[] = {
                { "idle", QStyle::State_Enabled },
                { "hover", QStyle::State_Enabled | QStyle::State_MouseOver },
                { "selected", QStyle::State_Enabled | QStyle::State_Selected },
                { "selected over", QStyle::State_Enabled | QStyle::State_Selected | QStyle::State_MouseOver },
            };
            for (const NamedState& tabState : tabStates)
            {
                for (const QRect& rect : { QRect(0, 0, 62, 31), QRect(1, 1, 62, 31), QRect(2, 0, 61, 30), QRect(3, 1, 120, 33) })
                {
                    for (const qreal ratio : Ratios)
                    {
                        const auto paint = [&](QTabBar* bar)
                        {
                            QStyleOptionTab option;
                            option.initFrom(bar);
                            option.state = tabState.m_state | QStyle::State_Active;
                            option.rect = rect;
                            option.shape = QTabBar::RoundedNorth;
                            option.position = QStyleOptionTab::Middle;
                            option.selectedPosition = QStyleOptionTab::NotAdjacent;
                            option.text = QStringLiteral("Tab");
                            return drawn(rect.size() + QSize(rect.left() + 8, rect.top() + 8), ratio,
                                [&](QPainter* painter) { bar->style()->drawControl(QStyle::CE_TabBarTabShape, &option, painter, bar); });
                        };
                        EXPECT_EQ(differing(paint(legacyBar), paint(flatBar)), 0) << tabState.m_name << ", " << rectText(rect) << ", ratio " << ratio;
                    }
                }
            }

            // The box of the bar, the box of the tab widget and its pane
            for (const qreal ratio : Ratios)
            {
                const auto paintWidget = [&](QWidget* widget, const QSize& size)
                {
                    QStyleOption option;
                    option.initFrom(widget);
                    option.rect = QRect(QPoint(0, 0), size);
                    return drawn(size, ratio, [&](QPainter* painter) { widget->style()->drawPrimitive(QStyle::PE_Widget, &option, painter, widget); });
                };
                EXPECT_EQ(differing(paintWidget(legacyBar, QSize(180, 33)), paintWidget(flatBar, QSize(180, 33))), 0) << "bar box, ratio " << ratio;
                EXPECT_EQ(differing(paintWidget(legacyTabs, QSize(200, 80)), paintWidget(flatTabs, QSize(200, 80))), 0) << "tab widget box, ratio " << ratio;

                const auto paintPane = [&](QTabWidget* tabs)
                {
                    QStyleOptionTabWidgetFrame option;
                    option.initFrom(tabs);
                    option.rect = QRect(0, 0, 200, 60);
                    option.shape = QTabBar::RoundedNorth;
                    option.lineWidth = 0;
                    option.tabBarSize = QSize(100, 30);
                    option.tabBarRect = QRect(0, 0, 100, 30);
                    return drawn(option.rect.size(), ratio,
                        [&](QPainter* painter) { tabs->style()->drawPrimitive(QStyle::PE_FrameTabWidget, &option, painter, tabs); });
                };
                EXPECT_EQ(differing(paintPane(legacyTabs), paintPane(flatTabs)), 0) << "pane, ratio " << ratio;
            }
        }
    }

    // ----------------------------------------------------------------------------------
    // D: the frame of a floating dock (StyledDockWidget.qss:9-18)
    // ----------------------------------------------------------------------------------

    namespace ChromeTwinTestsInternal
    {
        //! The ways a dock comes to float: created floating (FancyDocking::createFloatingMainWindow: setFloating before the first show), floated
        //! once shown (the Gallery floats its docks to capture them; the user undocking a tab), floated and docked again, never floated
        enum class DockOrder
        {
            FloatingBeforeShow,
            FloatedAfterShow,
            FloatedAndDockedAgain,
            Docked,
        };

        const char* dockOrderName(DockOrder order)
        {
            switch (order)
            {
            case DockOrder::FloatingBeforeShow:
                return "floating before it is shown";
            case DockOrder::FloatedAfterShow:
                return "floated after it is shown";
            case DockOrder::FloatedAndDockedAgain:
                return "floated and docked again";
            default:
                return "docked";
            }
        }

        //! A main window with a dock widget. The handler the dock connects to its own topLevelChanged calls the window API of the platform
        //! (Platform::HandleFloatingWindow), which a test has no use for.
        void buildDock(QMainWindow& window)
        {
            auto* dock = new StyledDockWidget(QStringLiteral("Left Dock Widget"), &window);
            dock->setObjectName(QStringLiteral("dock"));
            dock->setAttribute(Qt::WA_DontShowOnScreen);
            QObject::disconnect(dock, SIGNAL(topLevelChanged(bool)), dock, nullptr);
            dock->setWidget(new QLabel(QStringLiteral("StyledDockWidget")));
            window.addDockWidget(Qt::LeftDockWidgetArea, dock);
            window.setCentralWidget(new QWidget);
        }

        //! Shows the window and takes the dock through the order the test asks for
        StyledDockWidget* showDock(QMainWindow& window, DockOrder order)
        {
            auto* dock = window.findChild<StyledDockWidget*>(QStringLiteral("dock"));
            if (order == DockOrder::FloatingBeforeShow)
            {
                dock->setFloating(true);
            }
            window.setAttribute(Qt::WA_DontShowOnScreen);
            window.resize(500, 300);
            window.show();
            QApplication::processEvents();
            if (order == DockOrder::FloatingBeforeShow)
            {
                dock->show();
            }
            if (order == DockOrder::FloatedAfterShow || order == DockOrder::FloatedAndDockedAgain)
            {
                dock->setFloating(true);
                QApplication::processEvents();
            }
            if (order == DockOrder::FloatedAndDockedAgain)
            {
                dock->setFloating(false);
                QApplication::processEvents();
            }
            return dock;
        }
    } // namespace ChromeTwinTestsInternal

    TEST_F(ChromeTwinTests, DockFrame_IsTheBorderOfTheRuleTheDockWasPolishedWith)
    {
        using namespace ChromeTwinTestsInternal;
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());
        ASSERT_TRUE(appSheet().contains(QStringLiteral("AzQtComponents--StyledDockWidget[floating=\"true\"]")));

        for (const DockOrder order : { DockOrder::FloatingBeforeShow, DockOrder::FloatedAfterShow, DockOrder::FloatedAndDockedAgain, DockOrder::Docked })
        {
            SCOPED_TRACE(dockOrderName(order));
            QMainWindow legacyWindow;
            buildDock(legacyWindow);
            legacy(legacyWindow);
            StyledDockWidget* legacyDock = showDock(legacyWindow, order);

            QMainWindow flatWindow;
            buildDock(flatWindow);
            flat(flatWindow);
            StyledDockWidget* flatDock = showDock(flatWindow, order);

            ASSERT_TRUE(legacyDock != nullptr);
            ASSERT_TRUE(flatDock != nullptr);
            ASSERT_TRUE(legacyDock->isVisible());
            ASSERT_TRUE(flatDock->isVisible());
            // The floating state is what the order made it
            EXPECT_EQ(flatDock->isFloating(), legacyDock->isFloating());

            // The rule is the one the dock was polished with: [floating="true"] is read then and not again, as a dock floated once shown keeps the rule
            // it had docked (n2 Gallery 25_styled-dock-widget: the floating docks have no ring and no inset, as the Gallery floats docks it has shown;
            // the Editor creates its floating docks floating and they have the ring). The frame the layout reserves around a floating dock that has no
            // native frame (QDockWidgetLayout) is that rule's border: 1px or none.
            const int legacyWidth = legacyDock->style()->pixelMetric(QStyle::PM_DockWidgetFrameWidth, nullptr, legacyDock);
            const int flatWidth = flatDock->style()->pixelMetric(QStyle::PM_DockWidgetFrameWidth, nullptr, flatDock);
            EXPECT_EQ(flatWidth, legacyWidth);
            EXPECT_EQ(legacyWidth, order == DockOrder::FloatingBeforeShow ? 1 : 0);

            // ... and the frame the dock draws (StyledDockWidget::paintEvent: PE_FrameDockWidget over the rect of the dock) is that border, or nothing
            for (const qreal ratio : Ratios)
            {
                for (const QSize& size : { QSize(107, 80), QSize(107, 446), QSize(80, 49) })
                {
                    const auto paint = [&](QDockWidget* dock)
                    {
                        QStyleOptionFrame option;
                        option.initFrom(dock);
                        option.rect = QRect(QPoint(0, 0), size);
                        return drawn(size, ratio, [&](QPainter* painter) { dock->style()->drawPrimitive(QStyle::PE_FrameDockWidget, &option, painter, dock); });
                    };
                    const QImage legacyImage = paint(legacyDock);
                    const QImage flatImage = paint(flatDock);
                    EXPECT_EQ(differing(legacyImage, flatImage), 0) << "frame, size " << sizeText(size) << ", ratio " << ratio;
                    const int drawnPixels = differing(flatImage, drawn(size, ratio, [](QPainter*) {}));
                    if (order == DockOrder::FloatingBeforeShow)
                    {
                        EXPECT_GT(drawnPixels, 0) << "size " << sizeText(size) << ", ratio " << ratio;
                    }
                    else
                    {
                        EXPECT_EQ(drawnPixels, 0) << "size " << sizeText(size) << ", ratio " << ratio;
                    }
                }
            }
        }
    }

    // ----------------------------------------------------------------------------------
    // T: the bottom line of the title bar of a floating dock (TitleBar.qss:20-24)
    // ----------------------------------------------------------------------------------

    TEST_F(ChromeTwinTests, TitleBar_TheBottomLineOfASimpleBarIsTheSheetsBorderAtEveryRatio)
    {
        using namespace ChromeTwinTestsInternal;
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());
        ASSERT_TRUE(appSheet().contains(QStringLiteral("AzQtComponents--TitleBar[drawSimple=\"true\"]")));

        const QSize size(108, 28);      // whole device pixels at 150%
        for (const bool simple : { false, true })
        {
            QWidget legacyRoot;
            auto* legacyBar = new TitleBar(&legacyRoot);
            legacyBar->setDrawAsTabBar(true);       // what TitleBar::polish does with the configuration the flattened Style loads
            legacyBar->setDrawSimple(simple);
            legacyBar->setGeometry(QRect(QPoint(0, 0), size));
            legacy(legacyRoot);
            show(legacyRoot, QSize(120, 40));

            QWidget flatRoot;
            auto* flatBar = new TitleBar(&flatRoot);
            flatBar->setDrawSimple(simple);
            flatBar->setGeometry(QRect(QPoint(0, 0), size));
            flat(flatRoot);
            show(flatRoot, QSize(120, 40));
            ASSERT_TRUE(flatBar->drawAsTabBar());
            // TitleBar::updateTitleBar fixed the height to the style's metric (TitleBar::titleBarHeight): the test wants whole device pixels at 150%
            flatBar->setFixedHeight(size.height());
            flatBar->resize(size);
            ASSERT_EQ(flatBar->size(), size);

            for (const qreal ratio : Ratios)
            {
                // The sheet: the background and the border of the rule, drawn by PE_Widget before the frame's own paint (QWidgetPrivate::paintBackground).
                // The flat bar paints itself (TitleBar::paintEvent), here without its children.
                QStyleOption option;
                option.initFrom(legacyBar);
                option.rect = QRect(QPoint(0, 0), size);
                const QImage legacyImage = drawn(size, ratio, [&](QPainter* painter) { legacyBar->style()->drawPrimitive(QStyle::PE_Widget, &option, painter, legacyBar); });
                const QImage flatImage = drawn(size, ratio, [&](QPainter* painter) { flatBar->render(painter, QPoint(), QRegion(), QWidget::RenderFlags()); });
                EXPECT_EQ(differing(legacyImage, flatImage), 0) << (simple ? "simple" : "tab bar") << ", ratio " << ratio;
            }
        }
    }

    TEST_F(ChromeTwinTests, TitleBar_TheAccentOfASimpleBarIsFourRowsAndAHalfAsNativeDrawsIt)
    {
        using namespace ChromeTwinTestsInternal;

        // The values of the Native capture (n2 Gallery 25_styled-dock-widget, dock1 floating at 150%, a bar 27px high): rows 30-35 are the #222222
        // fill, rows 36-39 #111111, row 40 #191919: the 3px border is 4.5 device pixels, its last row half over the fill, and the fill reaches
        // the bottom of the widget (a bar of 41 rows, 40.5 rounded). The sheet drew the border antialiased; a fill of the rect left no half row.
        BoxElement::Edges edges;
        edges.m_bottom = { 3, QColor(0x11, 0x11, 0x11) };
        const QSize size(108, 27);
        const QImage image = drawn(size, 1.5, [&](QPainter* painter)
            {
                BoxElement::drawFrame(painter, QRect(QPoint(0, 0), size), QBrush(QColor(0x22, 0x22, 0x22)), edges, BoxElement::Radii());
            });
        ASSERT_EQ(image.size(), QSize(162, 41));
        for (int y = 0; y < 36; ++y)
        {
            EXPECT_EQ(image.pixelColor(80, y).red(), 0x22) << "row " << y;
        }
        for (int y = 36; y < 40; ++y)
        {
            EXPECT_EQ(image.pixelColor(80, y).red(), 0x11) << "row " << y;
        }
        EXPECT_EQ(image.pixelColor(80, 40).red(), 0x19);
    }

    // ----------------------------------------------------------------------------------
    // L: the handle of a disabled slider (Slider::drawHandle)
    // ----------------------------------------------------------------------------------

    TEST_F(ChromeTwinTests, Slider_TheDisabledHandleHasNoRingWhateverTheWindowBrushIs)
    {
        using namespace ChromeTwinTestsInternal;
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        SliderInt slider(Qt::Horizontal);
        slider.setRange(0, 100);
        slider.setValue(40);
        slider.setEnabled(false);
        slider.setAttribute(Qt::WA_DontShowOnScreen);
        slider.setStyle(m_style);
        for (QWidget* child : slider.findChildren<QWidget*>())
        {
            child->setStyle(m_style);
        }
        slider.resize(300, 19);
        slider.show();
        QApplication::processEvents();
        QSlider* inner = slider.findChild<QSlider*>();
        ASSERT_TRUE(inner != nullptr);

        // The pen of a disabled handle is the palette's Window brush. Under the legacy sheet that brush is transparent (BaseStyleSheet.qss:15-17), so
        // Native draws the disc with no ring (n2 Gallery 21_slider: 13 x 12 device pixels at 150%); the flat palette's is opaque.
        const auto optionOf = [&](const QBrush& window)
        {
            QStyleOptionSlider option;
            option.initFrom(inner);
            option.state &= ~QStyle::State_Enabled;
            option.palette.setCurrentColorGroup(QPalette::Disabled);
            option.palette.setBrush(QPalette::Disabled, QPalette::Window, window);
            option.rect = inner->rect();
            option.orientation = Qt::Horizontal;
            option.minimum = inner->minimum();
            option.maximum = inner->maximum();
            option.sliderPosition = inner->sliderPosition();
            option.sliderValue = inner->value();
            option.singleStep = inner->singleStep();
            option.pageStep = inner->pageStep();
            option.subControls = QStyle::SC_SliderGroove | QStyle::SC_SliderHandle;
            return option;
        };

        for (const qreal ratio : Ratios)
        {
            const QStyleOptionSlider opaque = optionOf(QBrush(QColor(0xFF, 0x00, 0xFF)));
            const QStyleOptionSlider transparent = optionOf(QBrush(Qt::transparent));
            const QSize size = inner->size();
            const auto paint = [&](const QStyleOptionSlider& option)
            {
                return drawn(size, ratio, [&](QPainter* painter) { inner->style()->drawComplexControl(QStyle::CC_Slider, &option, painter, inner); });
            };
            const QImage withOpaqueWindow = paint(opaque);
            const QImage withTransparentWindow = paint(transparent);
            EXPECT_GT(differing(withOpaqueWindow, drawn(size, ratio, [](QPainter*) {})), 0) << "the slider was drawn, ratio " << ratio;
            EXPECT_EQ(differing(withOpaqueWindow, withTransparentWindow), 0) << "ratio " << ratio;
        }
    }
} // namespace AzQtComponents
