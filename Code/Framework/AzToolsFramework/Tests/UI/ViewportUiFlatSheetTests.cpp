/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <AzQtComponents/AzQtComponentsAPI.h>
#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/GuiPerfFloor.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Tests/SheetTwins.h>
#include <AzToolsFramework/ViewportUi/ButtonGroup.h>
#include <AzToolsFramework/ViewportUi/ViewportUiCluster.h>
#include <AzToolsFramework/ViewportUi/ViewportUiDisplay.h>
#include <AzToolsFramework/ViewportUi/ViewportUiSwitcher.h>

#include <QAction>
#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QFrame>
#include <QIcon>
#include <QImage>
#include <QLabel>
#include <QLayout>
#include <QMargins>
#include <QPainter>
#include <QPixmap>
#include <QStyleFactory>
#include <QStyleOption>
#include <QToolBar>
#include <QWidget>

#include <string>

namespace UnitTest
{
    // ----------------------------------------------------------------------------------
    // The viewport UI in the flattened GUI: which widgets the elements reach, that the call sites set their sheets in the legacy GUI
    // alone, and that the flattened GUI draws what the sheets drew (Qt's own style sheet style over Fusion, with the same sheet and
    // the rules of the application sheet that reach the widget) at the device ratios the GUI runs at
    //
    // Not twinned here: the label's font and the palette roles of the toolbars and the label. No sheet of the viewport UI sets a
    // font; the label's comes from the label rule (Text.qss:91-92) and, in the flattened GUI, from the generic label text element
    // (TextElement.cpp), which the test Style does not install (Style::polish(QApplication*) is not run), so the generic label tests
    // and the captured A/B carry it. The flat Style draws the toolbar panel and the label fill from its own values and reads no
    // palette role of these widgets.
    // ----------------------------------------------------------------------------------

    namespace ViewportUiFlatSheetTestsInternal
    {
        using ViewportUiDisplay = AzToolsFramework::ViewportUi::Internal::ViewportUiDisplay;
        using ViewportUiCluster = AzToolsFramework::ViewportUi::Internal::ViewportUiCluster;
        using ViewportUiSwitcher = AzToolsFramework::ViewportUi::Internal::ViewportUiSwitcher;
        using ButtonGroup = AzToolsFramework::ViewportUi::Internal::ButtonGroup;
        using Rung = AzQtComponents::GuiPerfFloor::Rung;

        //! A widget that Qt takes for an instance of a class it is not. The matchers and the toolbar's panel ask for the main window
        //! by its class name, as the selectors do, and the Editor's is a whole application.
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

        constexpr const char* BorderElementName = "Editor.Box.ViewportUiHighlightBorder";
        constexpr const char* BorderTextElementName = "Editor.Box.ViewportUiHighlightBorderText";

        // The dynamic property the flattened GUI puts on the label of the border (ViewportUiDisplay.cpp, BorderTextProperty)
        constexpr const char* BorderTextProperty = "viewportBorderText";

        // The sheets the call sites set in the legacy GUI (ViewportUiCluster.cpp, ViewportUiSwitcher.cpp, ViewportUiDisplay.cpp)
        constexpr const char* ClusterConstructorSheet = "background: black;";
        constexpr const char* SwitcherConstructorSheet =
            "QToolBar {background-color: none; border: none; spacing: 3px;}"
            "QToolButton {background-color: black; border: outset; border-color: white; border-radius: 7px; "
            "border-width: 2px; padding: 7px; color: white;}";
        constexpr const char* ToolBarSheet = "QToolBar { border: none; }";
        constexpr const char* BorderTextSheet = "background-color: #4A90E2; border: none";
        constexpr const char* BorderSheet = "#ViewportUiOverlay { border: 5px solid #4A90E2; border-top: 25px solid #4A90E2; }";
        constexpr const char* NoBorderSheet = "#ViewportUiOverlay { border: none; }";

        // The rules of the application sheet that reach the widgets, which the legacy cascade laid under the sheets above:
        // BaseStyleSheet.qss:15-21 (`*`: no fill), Text.qss:89-95 (`.Label, QLabel`: the margin above and below), ToolBar.qss:15-29
        // (`QToolBar`). The rules of ToolBar.qss:31-39 (`MainWindow QToolBar`, `AzQtComponents--DockMainWindow QToolBar`) cancel each
        // other for a toolbar of the viewport UI, which sits below both, and the sheet's `border: none` outranks them anyway.
        constexpr const char* OverlayRules = "* { background-color: transparent; }";
        constexpr const char* LabelRules = "* { background-color: transparent; } QLabel { margin-top: 1px; margin-bottom: 1px; }";
        constexpr const char* ToolBarRules =
            "QToolBar { background-color: #444444; qproperty-iconSize: 16px 16px; border: 0 none; padding: 5px; }";

        constexpr QRgb ParentColor = 0x808080;          // what neither the sheet nor the element draws stays this grey
        constexpr int BorderLabelHeight = 25;           // ViewportUiTopBorderSize, ViewportUiDisplay.cpp (setFixedHeight of the label)

        //! The option QFrame::paintEvent builds for a frame (QFrame::initStyleOption, protected)
        QStyleOptionFrame frameOption(const QFrame* frame)
        {
            QStyleOptionFrame option;
            option.initFrom(frame);
            option.frameShape = frame->frameShape();
            option.lineWidth = frame->lineWidth();
            option.midLineWidth = frame->midLineWidth();
            if (frame->frameShadow() == QFrame::Sunken)
            {
                option.state |= QStyle::State_Sunken;
            }
            else if (frame->frameShadow() == QFrame::Raised)
            {
                option.state |= QStyle::State_Raised;
            }
            return option;
        }

        //! The image a widget paints into, as a screen of that ratio is: the painter's device scales the style's drawing
        QImage blankImage(const QSize& size, qreal ratio)
        {
            QImage image((QSizeF(size) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
            image.setDevicePixelRatio(ratio);
            image.fill(QColor::fromRgb(ParentColor));
            return image;
        }

        //! What a widget paints for its box through its own style, in the order Qt paints it: the styled background (PE_Widget,
        //! QWidgetPrivate::paintBackground), then a frame's frame (CE_ShapedFrame, QFrame::drawFrame)
        QImage paintBox(const QWidget& widget, qreal ratio)
        {
            QImage image = blankImage(widget.size(), ratio);
            QPainter painter(&image);
            if (widget.testAttribute(Qt::WA_StyledBackground))
            {
                QStyleOption background;
                background.initFrom(&widget);
                widget.style()->drawPrimitive(QStyle::PE_Widget, &background, &painter, &widget);
            }
            if (const auto* frame = qobject_cast<const QFrame*>(&widget))
            {
                const QStyleOptionFrame option = frameOption(frame);
                widget.style()->drawControl(QStyle::CE_ShapedFrame, &option, &painter, &widget);
            }
            painter.end();
            return image;
        }

        //! What a toolbar paints for its panel through its own style (QToolBar::paintEvent: CE_ToolBar)
        QImage paintToolBar(QToolBar& bar, qreal ratio)
        {
            QImage image = blankImage(bar.size(), ratio);
            QPainter painter(&image);
            QStyleOptionToolBar option;
            option.initFrom(&bar);
            option.rect = bar.rect();
            option.lineWidth = bar.style()->pixelMetric(QStyle::PM_ToolBarFrameWidth, nullptr, &bar);
            bar.style()->drawControl(QStyle::CE_ToolBar, &option, &painter, &bar);
            painter.end();
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
                        return "at (" + std::to_string(x) + ", " + std::to_string(y) + "): legacy #" + hex(a.pixel(x, y)) + ", flat #"
                            + hex(b.pixel(x, y));
                    }
                }
            }
            return "";
        }

        //! A cluster as the viewport UI builds one: a vertical toolbar of checkable actions with icons of 16px, the size of
        //! ToolBar.qss:21. ViewportUiCluster adds nothing to a QToolBar but its sheet.
        QToolBar* makeCluster(QWidget* parent)
        {
            auto* bar = new QToolBar(parent);
            bar->setObjectName(QStringLiteral("cluster"));
            bar->setOrientation(Qt::Vertical);
            QPixmap icon(16, 16);
            icon.fill(Qt::white);
            for (int i = 0; i < 3; ++i)
            {
                auto* action = new QAction(bar);
                action->setCheckable(true);
                action->setIcon(QIcon(icon));
                bar->addAction(action);
            }
            bar->resize(28, 88);
            return bar;
        }

        //! A viewport UI display, as the Editor builds one, over a window that holds nothing else
        struct Display
        {
            Display()
                : m_display(&m_parent, &m_renderOverlay)
            {
                m_display.InitializeUiOverlay();
            }

            //! The widget of an element the display holds
            QWidget* element(AzToolsFramework::ViewportUi::ViewportUiElementId id)
            {
                return m_display.GetViewportUiElement(id).get();
            }

            //! The label on the border's top edge: the one label the overlay holds
            QLabel* borderLabel() const
            {
                return m_display.GetUiOverlay()->findChild<QLabel*>(QString(), Qt::FindDirectChildrenOnly);
            }

            QWidget m_parent;
            QWidget m_renderOverlay;
            ViewportUiDisplay m_display;
        };

        class ViewportUiFlatSheetTests
            : public LeakDetectionFixture
        {
        protected:
            void SetUp() override
            {
                LeakDetectionFixture::SetUp();

                // The test application carries StyleManager's legacy sheets: the twins name their own
                m_applicationSheet = qApp->styleSheet();
                qApp->setStyleSheet(QString());

                m_style = AZStd::make_unique<AzQtComponents::Style>(QStyleFactory::create(QStringLiteral("Fusion")));
                m_fusion = AZStd::unique_ptr<QStyle>(QStyleFactory::create(QStringLiteral("Fusion")));
                ViewportUiDisplay::RegisterFlatElements();
            }

            void TearDown() override
            {
                m_fusion.reset();
                m_style.reset();
                AzQtComponents::GuiPerfFloor::resetForTesting();
                qApp->setStyleSheet(m_applicationSheet);
                LeakDetectionFixture::TearDown();
            }

            //! The rung the next widgets are built in: the call sites and Style read it when they run
            static void enterRung(Rung rung)
            {
                AzQtComponents::GuiPerfFloor::setRungForTesting(rung);
            }

            //! The flat way: the style on the widget, and the element resolved as Style::polish does in the flattened GUI
            void flatten(QWidget& widget)
            {
                widget.setStyle(m_style.get());
                AzQtComponents::BoxElement::resolve(&widget);
            }

            static const char* elementOf(const QWidget& widget)
            {
                const AzQtComponents::BoxElement* element = AzQtComponents::BoxElement::forWidget(&widget);
                return element ? element->m_name : "";
            }

            //! The overlay of a viewport UI in the legacy GUI: Qt's style sheet style over Fusion, with the sheet of the call site and
            //! the rules of the application sheet above it. The callback gets the polished overlay.
            void withLegacyOverlay(const char* sheet, const AZStd::function<void(QWidget&)>& use) const
            {
                enterRung(Rung::Qss);
                QWidget window;
                window.setObjectName(QStringLiteral("ViewportUiWindow"));
                auto* overlay = new QWidget(&window);
                overlay->setObjectName(QStringLiteral("ViewportUiOverlay"));
                overlay->resize(160, 100);
                window.setStyle(m_fusion.get());
                overlay->setStyle(m_fusion.get());
                overlay->setStyleSheet(QString::fromLatin1(sheet));
                window.setStyleSheet(QString::fromLatin1(OverlayRules));
                window.ensurePolished();
                overlay->ensurePolished();
                use(*overlay);
            }

            //! The same overlay in the flattened GUI, its border shown or not, with its element
            void withFlatOverlay(bool borderShown, const AZStd::function<void(QWidget&)>& use)
            {
                enterRung(Rung::Flat);
                QWidget window;
                window.setObjectName(QStringLiteral("ViewportUiWindow"));
                auto* overlay = new QWidget(&window);
                overlay->setObjectName(QStringLiteral("ViewportUiOverlay"));
                overlay->resize(160, 100);
                overlay->setProperty("viewportBorder", borderShown);
                flatten(*overlay);
                use(*overlay);
            }

            //! The label on the border's top edge in the legacy GUI, below the overlay, with its sheet and the rules above it
            void withLegacyLabel(const AZStd::function<void(QLabel&)>& use) const
            {
                enterRung(Rung::Qss);
                QWidget window;
                window.setObjectName(QStringLiteral("ViewportUiWindow"));
                auto* overlay = new QWidget(&window);
                overlay->setObjectName(QStringLiteral("ViewportUiOverlay"));
                auto* label = new QLabel(QStringLiteral("Editing Component"), overlay);
                label->resize(200, BorderLabelHeight);
                window.setStyle(m_fusion.get());
                overlay->setStyle(m_fusion.get());
                label->setStyle(m_fusion.get());
                label->setStyleSheet(QString::fromLatin1(BorderTextSheet));
                window.setStyleSheet(QString::fromLatin1(LabelRules));
                window.ensurePolished();
                label->ensurePolished();
                use(*label);
            }

            //! The same label in the flattened GUI, with its element
            void withFlatLabel(const AZStd::function<void(QLabel&)>& use)
            {
                enterRung(Rung::Flat);
                QWidget window;
                window.setObjectName(QStringLiteral("ViewportUiWindow"));
                auto* overlay = new QWidget(&window);
                overlay->setObjectName(QStringLiteral("ViewportUiOverlay"));
                auto* label = new QLabel(QStringLiteral("Editing Component"), overlay);
                label->resize(200, BorderLabelHeight);
                label->setProperty(BorderTextProperty, true);       // as InitializeUiOverlay marks the label in the flattened GUI
                flatten(*label);
                use(*label);
            }

            //! A cluster in the legacy GUI as the viewport UI leaves it: the sheet of PrepareWidgetForViewportUi, below the rules of the
            //! application sheet, polished, and told its style changed as a repolish tells it
            void withLegacyCluster(const AZStd::function<void(QToolBar&)>& use) const
            {
                enterRung(Rung::Qss);
                QWidget root;
                QToolBar* bar = makeCluster(&root);
                root.setStyle(m_fusion.get());
                bar->setStyle(m_fusion.get());
                bar->setStyleSheet(QString::fromLatin1(ToolBarSheet));
                root.setStyleSheet(QString::fromLatin1(ToolBarRules));
                root.ensurePolished();
                bar->ensurePolished();
                QEvent styleChange(QEvent::StyleChange);
                QCoreApplication::sendEvent(bar, &styleChange);
                use(*bar);
            }

            //! The same cluster in the flattened GUI, in the chain of the Editor's viewport: the main window, the dock main window
            //! below it, then the viewport UI's window and overlay
            void withFlatCluster(const AZStd::function<void(QToolBar&)>& use)
            {
                enterRung(Rung::Flat);
                ClassNamedWidget mainWindow("MainWindow");
                auto* dockMainWindow = new ClassNamedWidget("AzQtComponents::DockMainWindow", &mainWindow);
                auto* window = new QWidget(dockMainWindow);
                window->setObjectName(QStringLiteral("ViewportUiWindow"));
                auto* overlay = new QWidget(window);
                overlay->setObjectName(QStringLiteral("ViewportUiOverlay"));
                QToolBar* bar = makeCluster(overlay);
                bar->setStyle(m_style.get());
                bar->ensurePolished();
                use(*bar);
            }

            //! The contents box of a frame (SE_ShapedFrameContents): where a label lays out its text
            static QRect frameContents(QWidget& widget)
            {
                const QStyleOptionFrame option = frameOption(qobject_cast<QFrame*>(&widget));
                return widget.style()->subElementRect(QStyle::SE_ShapedFrameContents, &option, &widget);
            }

            QString m_applicationSheet;
            AZStd::unique_ptr<AzQtComponents::Style> m_style;
            AZStd::unique_ptr<QStyle> m_fusion;
        };
    } // namespace ViewportUiFlatSheetTestsInternal

    // A unity build joins this file with others of namespace UnitTest: the fixture is the only name brought in, the rest is named
    // through the alias
    using ViewportUiFlatSheetTestsInternal::ViewportUiFlatSheetTests;
    namespace ViewportUiFlat = ViewportUiFlatSheetTestsInternal;

    // ----------------------------------------------------------------------------------
    // Wiring: the call sites set their sheets in the legacy GUI as they did, and none in the flattened GUI
    // ----------------------------------------------------------------------------------

    TEST_F(ViewportUiFlatSheetTests, Wiring_TheLegacyGuiSetsTheSheetsOfTheCallSites)
    {
        enterRung(ViewportUiFlat::Rung::Qss);
        ViewportUiFlat::Display ui;
        auto clusterGroup = AZStd::make_shared<ViewportUiFlat::ButtonGroup>();
        auto switcherGroup = AZStd::make_shared<ViewportUiFlat::ButtonGroup>();
        ui.m_display.AddCluster(clusterGroup, AzToolsFramework::ViewportUi::Alignment::TopRight);
        ui.m_display.AddSwitcher(switcherGroup, AzToolsFramework::ViewportUi::Alignment::TopLeft);

        // The toolbars, as PrepareWidgetForViewportUi leaves them (it replaces the sheets their constructors set)
        EXPECT_EQ(ui.element(clusterGroup->GetViewportUiElementId())->styleSheet(), QString::fromLatin1(ViewportUiFlat::ToolBarSheet));
        EXPECT_EQ(ui.element(switcherGroup->GetViewportUiElementId())->styleSheet(), QString::fromLatin1(ViewportUiFlat::ToolBarSheet));

        // The label on the border's top edge, and the overlay while a mode shows the border and after it
        EXPECT_EQ(ui.borderLabel()->styleSheet(), QString::fromLatin1(ViewportUiFlat::BorderTextSheet));
        ui.m_display.CreateViewportBorder("Editing Component", AZStd::nullopt);
        EXPECT_EQ(ui.m_display.GetUiOverlay()->styleSheet(), QString::fromLatin1(ViewportUiFlat::BorderSheet));
        ui.m_display.RemoveViewportBorder();
        EXPECT_EQ(ui.m_display.GetUiOverlay()->styleSheet(), QString::fromLatin1(ViewportUiFlat::NoBorderSheet));

        // The flattened GUI's state of the border and its mark on the label are the flattened GUI's alone
        EXPECT_FALSE(ui.m_display.GetUiOverlay()->property("viewportBorder").isValid());
        EXPECT_FALSE(ui.borderLabel()->property(ViewportUiFlat::BorderTextProperty).isValid());
    }

    TEST_F(ViewportUiFlatSheetTests, Wiring_TheConstructorsSetTheirSheetsInTheLegacyGuiAndNoneInTheFlattenedOne)
    {
        auto group = AZStd::make_shared<ViewportUiFlat::ButtonGroup>();

        enterRung(ViewportUiFlat::Rung::Qss);
        {
            ViewportUiFlat::ViewportUiCluster cluster(group);
            ViewportUiFlat::ViewportUiSwitcher switcher(group);
            EXPECT_EQ(cluster.styleSheet(), QString::fromLatin1(ViewportUiFlat::ClusterConstructorSheet));
            EXPECT_EQ(switcher.styleSheet(), QString::fromLatin1(ViewportUiFlat::SwitcherConstructorSheet));
        }

        enterRung(ViewportUiFlat::Rung::Flat);
        {
            ViewportUiFlat::ViewportUiCluster cluster(group);
            ViewportUiFlat::ViewportUiSwitcher switcher(group);
            EXPECT_TRUE(cluster.styleSheet().isEmpty());
            EXPECT_TRUE(switcher.styleSheet().isEmpty());
        }
    }

    TEST_F(ViewportUiFlatSheetTests, Wiring_TheFlattenedGuiSetsNoSheetOnTheToolbarsTheOverlayOrTheLabel)
    {
        enterRung(ViewportUiFlat::Rung::Flat);
        ViewportUiFlat::Display ui;
        auto clusterGroup = AZStd::make_shared<ViewportUiFlat::ButtonGroup>();
        auto switcherGroup = AZStd::make_shared<ViewportUiFlat::ButtonGroup>();
        ui.m_display.AddCluster(clusterGroup, AzToolsFramework::ViewportUi::Alignment::TopRight);
        ui.m_display.AddSwitcher(switcherGroup, AzToolsFramework::ViewportUi::Alignment::TopLeft);

        EXPECT_TRUE(ui.element(clusterGroup->GetViewportUiElementId())->styleSheet().isEmpty());
        EXPECT_TRUE(ui.element(switcherGroup->GetViewportUiElementId())->styleSheet().isEmpty());
        EXPECT_TRUE(ui.borderLabel()->styleSheet().isEmpty());
        ui.m_display.CreateViewportBorder("Editing Component", AZStd::nullopt);
        EXPECT_TRUE(ui.m_display.GetUiOverlay()->styleSheet().isEmpty());
        ui.m_display.RemoveViewportBorder();
        EXPECT_TRUE(ui.m_display.GetUiOverlay()->styleSheet().isEmpty());
    }

    // ----------------------------------------------------------------------------------
    // Resolution: the elements reach the widgets the sheets reached, and no others
    // ----------------------------------------------------------------------------------

    TEST_F(ViewportUiFlatSheetTests, Registry_TheOverlayHasTheBorderElementOnlyWhileItsBorderIsShown)
    {
        enterRung(ViewportUiFlat::Rung::Flat);
        ViewportUiFlat::Display ui;
        auto* overlay = const_cast<QWidget*>(ui.m_display.GetUiOverlay());

        // No border: the overlay draws nothing of its own, as with `border: none`
        AzQtComponents::BoxElement::resolve(overlay);
        EXPECT_STREQ(elementOf(*overlay), "");

        // The display's own border: the element, which is drawn at PE_Widget, and the label's, which the display marked
        ui.m_display.CreateViewportBorder("Editing Component", AZStd::nullopt);
        EXPECT_STREQ(elementOf(*overlay), ViewportUiFlat::BorderElementName);
        EXPECT_TRUE(overlay->testAttribute(Qt::WA_StyledBackground));
        EXPECT_TRUE(ui.borderLabel()->property(ViewportUiFlat::BorderTextProperty).toBool());
        AzQtComponents::BoxElement::resolve(ui.borderLabel());
        EXPECT_STREQ(elementOf(*ui.borderLabel()), ViewportUiFlat::BorderTextElementName);

        // The mode ends: the sheet became `border: none`, and the overlay has no element (what it then paints is the pixel tests')
        ui.m_display.RemoveViewportBorder();
        EXPECT_STREQ(elementOf(*overlay), "");
    }

    TEST_F(ViewportUiFlatSheetTests, Registry_TheSameNamesOutsideTheViewportUiHaveNoneOfTheElements)
    {
        enterRung(ViewportUiFlat::Rung::Flat);

        // An overlay of that name with the property, and a marked label, but not below the viewport UI's window
        QWidget loose;
        auto* looseOverlay = new QWidget(&loose);
        looseOverlay->setObjectName(QStringLiteral("ViewportUiOverlay"));
        looseOverlay->setProperty("viewportBorder", true);
        auto* looseLabel = new QLabel(looseOverlay);
        looseLabel->setProperty(ViewportUiFlat::BorderTextProperty, true);
        flatten(*looseOverlay);
        flatten(*looseLabel);
        EXPECT_STRNE(elementOf(*looseOverlay), ViewportUiFlat::BorderElementName);
        EXPECT_STRNE(elementOf(*looseLabel), ViewportUiFlat::BorderTextElementName);

        // Below the window: another widget with the property, and marked labels that are not children of the overlay
        QWidget window;
        window.setObjectName(QStringLiteral("ViewportUiWindow"));
        auto* overlay = new QWidget(&window);
        overlay->setObjectName(QStringLiteral("ViewportUiOverlay"));
        auto* other = new QWidget(&window);
        other->setObjectName(QStringLiteral("somethingElse"));
        other->setProperty("viewportBorder", true);
        auto* deepLabel = new QLabel(new QWidget(overlay));         // below the overlay, not on it
        auto* windowLabel = new QLabel(&window);                    // in the window, not on the overlay
        auto* overlayLabel = new QLabel(overlay);                   // the one the sheet reached
        deepLabel->setProperty(ViewportUiFlat::BorderTextProperty, true);
        windowLabel->setProperty(ViewportUiFlat::BorderTextProperty, true);
        overlayLabel->setProperty(ViewportUiFlat::BorderTextProperty, true);

        // On the overlay and not the one the sheet was set on: a label added to the overlay later does not take the fill
        auto* laterLabel = new QLabel(overlay);
        flatten(*other);
        flatten(*deepLabel);
        flatten(*windowLabel);
        flatten(*overlayLabel);
        flatten(*laterLabel);
        EXPECT_STRNE(elementOf(*other), ViewportUiFlat::BorderElementName);
        EXPECT_STRNE(elementOf(*deepLabel), ViewportUiFlat::BorderTextElementName);
        EXPECT_STRNE(elementOf(*windowLabel), ViewportUiFlat::BorderTextElementName);
        EXPECT_STREQ(elementOf(*overlayLabel), ViewportUiFlat::BorderTextElementName);
        EXPECT_STRNE(elementOf(*laterLabel), ViewportUiFlat::BorderTextElementName);
    }

    // ----------------------------------------------------------------------------------
    // Pixels: the elements against the sheets, at the device ratios of the GUI
    // ----------------------------------------------------------------------------------

    TEST_F(ViewportUiFlatSheetTests, Pixels_TheBorderIsThatOfTheSheetAtEveryRatio)
    {
        for (const qreal ratio : { 1.0, 1.5, 2.0 })
        {
            QImage legacy;
            QImage flat;
            withLegacyOverlay(
                ViewportUiFlat::BorderSheet, [&legacy, ratio](QWidget& overlay) { legacy = ViewportUiFlat::paintBox(overlay, ratio); });
            withFlatOverlay(true, [&flat, ratio](QWidget& overlay) { flat = ViewportUiFlat::paintBox(overlay, ratio); });
            EXPECT_EQ(ViewportUiFlat::firstDifference(legacy, flat), std::string()) << "at ratio " << ratio;
        }
    }

    TEST_F(ViewportUiFlatSheetTests, Pixels_TheBorderIsFiveBySideAndTwentyFiveAtTheTopInTheHighlightColour)
    {
        const QImage flat = [this]
        {
            QImage image;
            withFlatOverlay(true, [&image](QWidget& overlay) { image = ViewportUiFlat::paintBox(overlay, 1.0); });
            return image;
        }();

        const auto rgb = [&flat](int x, int y) { return flat.pixel(x, y) & 0xFFFFFF; };
        EXPECT_EQ(rgb(0, 50), 0x4A90E2u);                           // the left edge, 5px
        EXPECT_EQ(rgb(4, 50), 0x4A90E2u);
        EXPECT_EQ(rgb(5, 50), ViewportUiFlat::ParentColor);         // inside: no fill
        EXPECT_EQ(rgb(159, 50), 0x4A90E2u);                         // the right edge
        EXPECT_EQ(rgb(80, 99), 0x4A90E2u);                          // the bottom edge
        EXPECT_EQ(rgb(80, 94), ViewportUiFlat::ParentColor);
        EXPECT_EQ(rgb(80, 24), 0x4A90E2u);                          // the top edge, 25px
        EXPECT_EQ(rgb(80, 25), ViewportUiFlat::ParentColor);
    }

    TEST_F(ViewportUiFlatSheetTests, Pixels_WithoutABorderTheOverlayPaintsNothingInBothGuis)
    {
        for (const qreal ratio : { 1.0, 1.5, 2.0 })
        {
            QImage legacy;
            QImage flat;
            withLegacyOverlay(
                ViewportUiFlat::NoBorderSheet, [&legacy, ratio](QWidget& overlay) { legacy = ViewportUiFlat::paintBox(overlay, ratio); });
            withFlatOverlay(false, [&flat, ratio](QWidget& overlay) { flat = ViewportUiFlat::paintBox(overlay, ratio); });
            EXPECT_EQ(ViewportUiFlat::firstDifference(legacy, flat), std::string()) << "at ratio " << ratio;
            EXPECT_EQ(flat.pixel(0, 0) & 0xFFFFFF, ViewportUiFlat::ParentColor);
        }
    }

    TEST_F(ViewportUiFlatSheetTests, Pixels_TheOverlayPaintsNothingOnceItsBorderIsRemoved)
    {
        // The way back from a border: BoxElement::resolve only ever sets WA_StyledBackground, so the overlay keeps the attribute when its
        // element goes (as the legacy overlay keeps it from the `*` rule, BaseStyleSheet.qss:15-21), and what it paints at PE_Widget is
        // then the base style's. The legacy overlay with `border: none` paints nothing (the test above).
        enterRung(ViewportUiFlat::Rung::Flat);
        ViewportUiFlat::Display ui;
        auto* overlay = const_cast<QWidget*>(ui.m_display.GetUiOverlay());
        overlay->resize(160, 100);
        overlay->setStyle(m_style.get());

        ui.m_display.CreateViewportBorder("Editing Component", AZStd::nullopt);
        ASSERT_STREQ(elementOf(*overlay), ViewportUiFlat::BorderElementName);
        ui.m_display.RemoveViewportBorder();
        ASSERT_STREQ(elementOf(*overlay), "");
        EXPECT_TRUE(overlay->testAttribute(Qt::WA_StyledBackground));

        for (const qreal ratio : { 1.0, 1.5, 2.0 })
        {
            EXPECT_EQ(
                ViewportUiFlat::firstDifference(
                    ViewportUiFlat::blankImage(overlay->size(), ratio), ViewportUiFlat::paintBox(*overlay, ratio)),
                std::string())
                << "at ratio " << ratio;
        }
    }

    TEST_F(ViewportUiFlatSheetTests, Pixels_TheLabelOnTheBorderIsFilledAsItsSheetFilledItAtEveryRatio)
    {
        for (const qreal ratio : { 1.0, 1.5, 2.0 })
        {
            QImage legacy;
            QImage flat;
            withLegacyLabel([&legacy, ratio](QLabel& label) { legacy = ViewportUiFlat::paintBox(label, ratio); });
            withFlatLabel([&flat, ratio](QLabel& label) { flat = ViewportUiFlat::paintBox(label, ratio); });
            EXPECT_EQ(ViewportUiFlat::firstDifference(legacy, flat), std::string()) << "at ratio " << ratio;
        }
    }

    TEST_F(ViewportUiFlatSheetTests, Pixels_ThePanelOfAClusterIsThatOfItsSheetsAtEveryRatioWithoutALineAtItsBottom)
    {
        for (const qreal ratio : { 1.0, 1.5, 2.0 })
        {
            QImage legacy;
            QImage flat;
            withLegacyCluster([&legacy, ratio](QToolBar& bar) { legacy = ViewportUiFlat::paintToolBar(bar, ratio); });
            withFlatCluster([&flat, ratio](QToolBar& bar) { flat = ViewportUiFlat::paintToolBar(bar, ratio); });
            EXPECT_EQ(ViewportUiFlat::firstDifference(legacy, flat), std::string()) << "at ratio " << ratio;

            // The fill of ToolBar.qss:20 to every edge: the Editor's chain (a main window above a dock main window) has no
            // MainWindow QToolBar line (ToolBar.qss:33-34)
            EXPECT_EQ(flat.pixel(0, 0) & 0xFFFFFF, 0x444444u);
            EXPECT_EQ(flat.pixel(flat.width() - 1, flat.height() - 1) & 0xFFFFFF, 0x444444u);
        }
    }

    // ----------------------------------------------------------------------------------
    // Boxes: where the contents lie, and the spacing of the layouts
    // ----------------------------------------------------------------------------------

    TEST_F(ViewportUiFlatSheetTests, Box_TheLabelOnTheBorderLaysOutItsTextInTheContentsOfTheLabelRuleUnderItsSheet)
    {
        // The label rule's margin above and below (Text.qss:93-94), which an element that only filled the label would have dropped
        QRect legacy;
        QRect flat;
        withLegacyLabel([&legacy](QLabel& label) { legacy = frameContents(label); });
        withFlatLabel([&flat](QLabel& label) { flat = frameContents(label); });

        EXPECT_EQ(flat, legacy);
        EXPECT_EQ(flat, QRect(0, 1, 200, ViewportUiFlat::BorderLabelHeight - 2));
    }

    TEST_F(ViewportUiFlatSheetTests, Box_TheOverlaysLayoutsTakeNoMarginsFromTheStyleInEitherGui)
    {
        // A widget's own contents rect (QWidget::contentsRect) is never the style's. What a style gives a plain widget with a border of
        // its own is the margin of the layouts that set none: the sheet's border plus padding (QStyleSheetStyle, PM_Layout*Margin),
        // and 5 on every side from Style (Style.cpp, pixelMetric). The display sets the margins of its layouts itself
        // (InitializeUiOverlay, ViewportElementMargins), so neither is read: the full-screen layout of the overlay keeps its 0, which
        // Style would have made 5, and the layout of the elements keeps what the display gives it.
        QMargins legacyFullScreen;
        QMargins flatFullScreen;
        QMargins legacyElements;
        QMargins flatElements;

        enterRung(ViewportUiFlat::Rung::Qss);
        {
            ViewportUiFlat::Display ui;
            ui.m_display.CreateViewportBorder("Editing Component", AZStd::nullopt);
            legacyFullScreen = ui.m_display.GetUiOverlay()->layout()->contentsMargins();
            legacyElements = ui.m_display.GetUiOverlayLayout()->contentsMargins();
        }

        enterRung(ViewportUiFlat::Rung::Flat);
        {
            ViewportUiFlat::Display ui;
            const_cast<QWidget*>(ui.m_display.GetUiOverlay())->setStyle(m_style.get());
            ui.m_display.CreateViewportBorder("Editing Component", AZStd::nullopt);
            flatFullScreen = ui.m_display.GetUiOverlay()->layout()->contentsMargins();
            flatElements = ui.m_display.GetUiOverlayLayout()->contentsMargins();
        }

        EXPECT_EQ(flatFullScreen, legacyFullScreen);
        EXPECT_EQ(flatFullScreen, QMargins(0, 0, 0, 0));
        EXPECT_EQ(flatElements, legacyElements);
    }

    TEST_F(ViewportUiFlatSheetTests, Box_TheClustersLayoutHasTheMarginsOfItsSheets)
    {
        // ToolBar.qss:28 `padding: 5px` (QToolBarLayout takes one value for the four sides, ToolBar.qss:11-13), which Style::polish
        // gives the toolbars of the flattened GUI
        QMargins legacy;
        QMargins flat;
        withLegacyCluster([&legacy](QToolBar& bar) { legacy = bar.layout()->contentsMargins(); });
        withFlatCluster([&flat](QToolBar& bar) { flat = bar.layout()->contentsMargins(); });

        EXPECT_EQ(flat, legacy);
        EXPECT_EQ(flat, QMargins(5, 5, 5, 5));
    }

    TEST_F(ViewportUiFlatSheetTests, Box_TheSwitcherKeepsTheSpacingOfItsFirstSheetInBothGuis)
    {
        // The legacy layout read `spacing: 3px` of the constructor's sheet, and the sheet that replaced it before the first polish
        // did not tell the toolbar again (the captured switcher: layout spacing 3, a cluster's 12). The flattened GUI sets no sheet
        // and gives the layout the 3 itself.
        int legacy = 0;
        int flat = 0;

        enterRung(ViewportUiFlat::Rung::Qss);
        {
            ViewportUiFlat::Display ui;
            auto group = AZStd::make_shared<ViewportUiFlat::ButtonGroup>();
            ui.m_display.AddSwitcher(group, AzToolsFramework::ViewportUi::Alignment::TopLeft);
            legacy = qobject_cast<QToolBar*>(ui.element(group->GetViewportUiElementId()))->layout()->spacing();
        }

        enterRung(ViewportUiFlat::Rung::Flat);
        {
            ViewportUiFlat::Display ui;
            auto group = AZStd::make_shared<ViewportUiFlat::ButtonGroup>();
            ui.m_display.AddSwitcher(group, AzToolsFramework::ViewportUi::Alignment::TopLeft);
            flat = qobject_cast<QToolBar*>(ui.element(group->GetViewportUiElementId()))->layout()->spacing();
        }

        EXPECT_EQ(flat, legacy);
        EXPECT_EQ(flat, 3);
    }
} // namespace UnitTest
