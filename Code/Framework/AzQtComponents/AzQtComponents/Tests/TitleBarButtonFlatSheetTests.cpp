/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Tests/FieldTwins.h>

#include <AzQtComponents/Components/DockBarButton.h>
#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/GuiPerfFloor.h>
#include <AzQtComponents/Components/Titlebar.h>
#include <AzQtComponents/Components/Widgets/TabWidget.h>
#include <AzQtComponents/Components/Widgets/TabWidgetActionToolBar.h>

#include <QAction>
#include <QCoreApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMap>
#include <QStackedLayout>
#include <QStyleOptionToolButton>
#include <QToolButton>

#include <QtTest/qtestmouse.h>

namespace AzQtComponents
{
    // ----------------------------------------------------------------------------------
    // The window buttons of the title bars (TitleBar.qss:90-145): the hover and pressed fill of a title bar's minimize and maximize
    // buttons, the glyph of a disabled close button, and the icons of the minimize, maximize and close buttons of a tabbed floating
    // window's action toolbar.
    //
    // The legacy twin is Qt's own style sheet style over the Style (QStyleSheetStyle draws over the application's style, which is the
    // Style in the engine: it draws a tool button's label and the buttons' background in the states the sheet leaves to the base style),
    // with the rules of the sheet quoted below on a root widget. The legacy twin is built, painted and gone before the flat twin is
    // built, each at the rung of its GUI (the Style reads the rung). The flat twin is the same tree under the Style with the registered
    // elements. What each twin shows is kept as images and facts, and the two are compared.
    // ----------------------------------------------------------------------------------

    //! Named, not anonymous: a unity build joins the test files, where anonymous namespaces merge
    namespace TitleBarButtonFlatSheetTestsInternal
    {
        using namespace FieldTwins;

        // ------------------------------------------------------------------------------
        // The rules of the legacy sheets that reach the buttons
        // ------------------------------------------------------------------------------

        // BaseStyleSheet.qss:15-21 (the universal rule of the application's sheet)
        const char* const UniversalRule = R"qss(
*
{
    background-color: transparent;
    color: white;
    font-family: "Open Sans";
    font-weight: 400;
}
)qss";

        // TabWidgetActionToolBar.qss:19-24 (the buttons of a tab widget's action toolbar)
        const char* const ActionToolBarRule = R"qss(
AzQtComponents--TabWidgetActionToolBar QToolButton,
AzQtComponents--TabWidgetActionToolBar QToolButton:hover,
AzQtComponents--TabWidgetActionToolBar QToolButton:pressed
{
    background: transparent;
}
)qss";

        // TitleBar.qss:9-145 (every rule of the sheet up to the tab bar's)
        const char* const TitleBarRules = R"qss(
AzQtComponents--TitleBar
{
    background-color: #111111;
    border: 1px solid #000000;
}

AzQtComponents--TitleBar[drawAsTabBar="true"]
{
    border: none;
}

AzQtComponents--TitleBar[drawSimple="true"]
{
    background-color: #222222;
    border-bottom: 3px solid #111111;
}

AzQtComponents--TitleBar *
{
    background-color: transparent;
}

AzQtComponents--TitleBar #title
{
    font-family: "Open Sans";
    font-size: 14px;
    margin-left: 10px;
    margin-right: 16px;
}

AzQtComponents--TitleBar #buttons
{
    margin-right: 0;
}

AzQtComponents--DockBarButton
{
    border: 0 none;
    qproperty-iconSize: 16px 16px;
    min-width: 32px;
    max-width: 32px;
    min-height: 32px;
    max-height: 32px;
}

AzQtComponents--TitleBar[drawAsTabBar="true"] #tabButtons
{
    margin-bottom: 3px;
}

AzQtComponents--TitleBar[drawAsTabBar="true"] #tabButtons AzQtComponents--DockBarButton
{
    qproperty-iconSize: 14px 14px;
    border: 0 none;
    min-width: 26px;
    max-width: 26px;
    min-height: 26px;
    max-height: 26px;
}

AzQtComponents--TitleBar[drawSimple="true"] AzQtComponents--DockBarButton
{
    qproperty-iconSize: 14px 14px;
    border: 0 none;
    min-width: 24px;
    max-width: 24px;
    min-height: 24px;
    max-height: 24px;
}

AzQtComponents--TabBar AzQtComponents--DockBarButton
{
    qproperty-iconSize: 12px 12px;
    border: 0 none;
    min-width: 14px;
    max-width: 14px;
    min-height: 14px;
    max-height: 14px;
    margin-right: 4px;
}

QToolButton#minimizeButton,
QToolButton#maximizeButton,
QToolButton#closeButton
{
    background: transparent;
}

QToolButton#minimizeButton:hover,
QToolButton#maximizeButton:hover
{
    background: rgba(255,255,255,0.1);
}

QToolButton#closeButton:hover {
    background: #B80D1C;
}

AzQtComponents--TabWidget QToolButton#closeButton
{
    qproperty-icon: url(:/Application/titlebar-close.svg);
}

AzQtComponents--DockBarButton.close
{
    titlebar-close-icon: url(:/Application/titlebar-close.svg);
}

AzQtComponents--TabWidget QToolButton#maximizeButton
{
    qproperty-icon: url(:/Application/titlebar-maximize.svg);
}

AzQtComponents--DockBarButton.maximize
{
    titlebar-maximize-icon: url(:/Application/titlebar-maximize.svg);
}

AzQtComponents--DockBarButton.restore
{
    titlebar-maximize-icon: url(:/Application/titlebar-restore.svg);
}

AzQtComponents--TabWidget QToolButton#minimizeButton
{
    qproperty-icon: url(:/Application/titlebar-minimize.svg);
}

AzQtComponents--DockBarButton.minimize
{
    titlebar-minimize-icon: url(:/Application/titlebar-minimize.svg);
}

AzQtComponents--DockBarButton:disabled
{
    titlebar-close-icon: none;
}
)qss";

        //! The sheet the legacy twin's root carries
        QString legacySheet()
        {
            return QString::fromUtf8(UniversalRule) + QString::fromUtf8(ActionToolBarRule) + QString::fromUtf8(TitleBarRules);
        }

        const char* const MinimizeIcon = ":/Application/titlebar-minimize.svg";     // TitleBar.qss:134
        const char* const MaximizeIcon = ":/Application/titlebar-maximize.svg";     // TitleBar.qss:119
        const char* const CloseIcon = ":/Application/titlebar-close.svg";           // TitleBar.qss:109

        const qreal Ratios[] = { 1.0, 1.5, 2.0 };
        const QColor Backdrop(0x11, 0x11, 0x11);        // what a title bar is filled with (TitleBar.qss:11)

        // ------------------------------------------------------------------------------
        // What a twin shows
        // ------------------------------------------------------------------------------

        struct Observed
        {
            QMap<QString, QImage> m_images;
            QMap<QString, QString> m_facts;
        };

        //! The rung a twin runs at for its scope, and the rung of the process after it
        class RungScope
        {
        public:
            explicit RungScope(GuiPerfFloor::Rung rung)
                : m_before(GuiPerfFloor::rung())
            {
                GuiPerfFloor::setRungForTesting(rung);
            }

            ~RungScope()
            {
                GuiPerfFloor::setRungForTesting(m_before);
            }

        private:
            GuiPerfFloor::Rung m_before;
        };

        class TitleBarButtonFlatSheetTests
            : public Fixture
        {
        public:
            TitleBarButtonFlatSheetTests()
            {
                // The application's style is the Style, as it is in the engine: the flattened GUI draws with it, the sheet style of the
                // legacy GUI draws over it (QStyleSheetStyle::baseStyle). The Fixture's own Style is not the application's, and its
                // filter goes.
                QApplication::instance()->removeEventFilter(m_style.get());
                QApplication::setStyle(new Style(QStyleFactory::create(QStringLiteral("Fusion"))));
            }

            //! The root of a twin, shown: the legacy one carries the sheet
            static void showRoot(QWidget& root, bool legacy, const QSize& size)
            {
                if (legacy)
                {
                    root.setStyleSheet(legacySheet());
                }
                show(root, size);
                settle();
            }

            //! Posted events and the buttons the title bar deleted when it rebuilt them
            static void settle()
            {
                for (int round = 0; round < 3; ++round)
                {
                    QApplication::processEvents();
                    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
                }
            }
        };

        //! A widget as it paints itself, onto an image of a device ratio over the colour behind it
        QImage renderOf(QWidget* widget, qreal ratio)
        {
            QImage image((QSizeF(widget->size()) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
            image.setDevicePixelRatio(ratio);
            image.fill(Backdrop);
            widget->render(&image, QPoint(), QRegion(), QWidget::DrawChildren);
            return image;
        }

        QString keyOf(const QString& what, const char* state, qreal ratio)
        {
            return QStringLiteral("%1|%2|%3").arg(what, QString::fromLatin1(state)).arg(ratio);
        }

        //! The application's left mouse button, held or let go (QApplication::mouseButtons), as a press on some other window makes it. The
        //! press goes to a small window of its own, so no widget of a twin gets it: only the application's state changes.
        class LeftButtonAnchor
        {
        public:
            LeftButtonAnchor()
            {
                m_window.setWindowFlags(Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus);
                m_window.setAttribute(Qt::WA_ShowWithoutActivating);
                m_window.setGeometry(0, 0, 16, 16);
                m_window.show();
            }

            ~LeftButtonAnchor()
            {
                hold(false);
            }

            void hold(bool down)
            {
                if (down == m_down || !m_window.windowHandle())
                {
                    return;
                }
                m_down = down;
                if (down)
                {
                    QTest::mousePress(m_window.windowHandle(), Qt::LeftButton, Qt::NoModifier, QPoint(8, 8));
                }
                else
                {
                    QTest::mouseRelease(m_window.windowHandle(), Qt::LeftButton, Qt::NoModifier, QPoint(8, 8));
                }
            }

        private:
            QWidget m_window;
            bool m_down = false;
        };

        //! The button idle, with the pointer over it, pressed with the pointer over it, and with the pointer over it while the left button
        //! is held elsewhere (a press that was dragged over it), at every ratio. A disabled button is shot in the same states: only its
        //! enabled state differs. The application's left button is held in the two states that have it down, as in the engine.
        void shoot(Observed& seen, const QString& what, QWidget* widget)
        {
            struct State
            {
                const char* m_name;
                bool m_hover;
                bool m_down;
                bool m_leftHeld;
            };
            const State states[] = {
                { "idle", false, false, false },
                { "hover", true, false, false },
                { "pressed", true, true, true },
                { "dragged", true, false, true },
            };
            auto* button = qobject_cast<QAbstractButton*>(widget);
            LeftButtonAnchor anchor;
            for (const State& state : states)
            {
                anchor.hold(state.m_leftHeld);
                widget->setAttribute(Qt::WA_UnderMouse, state.m_hover);
                if (button)
                {
                    button->setDown(state.m_down);
                }
                for (const qreal ratio : Ratios)
                {
                    seen.m_images[keyOf(what, state.m_name, ratio)] = renderOf(widget, ratio);
                }
            }
            anchor.hold(false);
            widget->setAttribute(Qt::WA_UnderMouse, false);
            if (button)
            {
                button->setDown(false);
            }
        }

        //! The glyph an icon has at every ratio, as pixels
        void noteIcon(Observed& seen, const QString& what, const QIcon& icon, const QSize& size)
        {
            seen.m_facts[what + QStringLiteral("|icon null")] = icon.isNull() ? QStringLiteral("yes") : QStringLiteral("no");
            for (const qreal ratio : Ratios)
            {
                seen.m_images[keyOf(what, "icon", ratio)] = icon.pixmap(size, ratio).toImage();
            }
        }

        void expectSameImages(const Observed& legacy, const Observed& flat)
        {
            ASSERT_EQ(flat.m_images.keys(), legacy.m_images.keys());
            for (auto it = legacy.m_images.constBegin(); it != legacy.m_images.constEnd(); ++it)
            {
                EXPECT_EQ(differing(flat.m_images[it.key()], it.value()), 0)
                    << it.key().toStdString() << ": " << firstDifference(flat.m_images[it.key()], it.value());
            }
        }

        void expectSameFacts(const Observed& legacy, const Observed& flat)
        {
            ASSERT_EQ(flat.m_facts.keys(), legacy.m_facts.keys());
            for (auto it = legacy.m_facts.constBegin(); it != legacy.m_facts.constEnd(); ++it)
            {
                EXPECT_EQ(flat.m_facts[it.key()].toStdString(), it.value().toStdString()) << it.key().toStdString();
            }
        }

        //! The colour a fill of alpha `alpha` leaves over the backdrop of the images, as Qt composes it
        QRgb composedOver(const QColor& fill)
        {
            QImage one(1, 1, QImage::Format_ARGB32_Premultiplied);
            one.fill(Backdrop);
            QPainter painter(&one);
            painter.fillRect(0, 0, 1, 1, fill);
            painter.end();
            return one.pixel(0, 0);
        }

        // ------------------------------------------------------------------------------
        // The title bar's buttons
        // ------------------------------------------------------------------------------

        enum class Footprint
        {
            Main,       //!< TitleBar.qss:44-52, 32px
            Simple,     //!< TitleBar.qss:69-77, 24px (drawSimple)
            Tabs,       //!< TitleBar.qss:59-67, 26px (the buttons beside a dock's tabs)
        };

        const char* footprintName(Footprint footprint)
        {
            switch (footprint)
            {
            case Footprint::Simple:
                return "simple";
            case Footprint::Tabs:
                return "tabs";
            default:
                return "main";
            }
        }

        const char* nameOf(DockBarButton::WindowDecorationButton type)
        {
            switch (type)
            {
            case DockBarButton::MinimizeButton:
                return "minimize";
            case DockBarButton::MaximizeButton:
                return "maximize";
            default:
                return "close";
            }
        }

        //! A title bar with the three window buttons in a footprint, shown, and the visible buttons of it shot in every state. With
        //! `disabled` the three buttons are disabled before the shots.
        Observed observeTitleBar(bool legacy, Footprint footprint, bool disabled)
        {
            RungScope rung(legacy ? GuiPerfFloor::Rung::Qss : GuiPerfFloor::Rung::Flat);
            Observed seen;

            QWidget root;
            auto* bar = new TitleBar(&root);
            bar->setGeometry(0, 0, 360, 40);
            bar->setButtons({ DockBarButton::MinimizeButton, DockBarButton::MaximizeButton, DockBarButton::CloseButton });
            TitleBarButtonFlatSheetTests::showRoot(root, legacy, QSize(360, 40));

            // The title bar sets its look from the configuration when it is polished: the footprint is set after, in both twins
            bar->setDrawSimple(footprint == Footprint::Simple);
            bar->setDrawAsTabBar(footprint == Footprint::Tabs);
            bar->findChild<QStackedLayout*>()->setCurrentIndex(footprint == Footprint::Tabs ? 0 : 1);
            TitleBarButtonFlatSheetTests::settle();

            int visible = 0;
            for (DockBarButton* button : bar->findChildren<DockBarButton*>())
            {
                if (!button->isVisible())
                {
                    continue;
                }
                ++visible;
                button->setEnabled(!disabled);
                const QString what = QString::fromLatin1(nameOf(button->buttonType()));
                seen.m_facts[what + QStringLiteral("|size")] = QString::fromStdString(sizeText(button->size()));
                seen.m_facts[what + QStringLiteral("|iconSize")] = QString::fromStdString(sizeText(button->iconSize()));
                seen.m_facts[what + QStringLiteral("|enabled")] = button->isEnabled() ? QStringLiteral("yes") : QStringLiteral("no");
                shoot(seen, what, button);
            }
            seen.m_facts[QStringLiteral("visible buttons")] = QString::number(visible);
            return seen;
        }

        //! The pixel in the top left corner of a shot: the glyph of a button never reaches it, the fill of its rule does
        QRgb cornerOf(const Observed& seen, const QString& what, const char* state, qreal ratio)
        {
            return seen.m_images[keyOf(what, state, ratio)].pixel(1, 1);
        }

        // ------------------------------------------------------------------------------
        // The quotes above are the lines of the real sheets
        // ------------------------------------------------------------------------------

        TEST_F(TitleBarButtonFlatSheetTests, QuotedRules_AreTheRulesOfTheApplicationSheetVerbatim)
        {
            const QString real = appSheet().simplified();
            EXPECT_TRUE(real.contains(QString::fromUtf8(UniversalRule).simplified()));
            EXPECT_TRUE(real.contains(QString::fromUtf8(ActionToolBarRule).simplified()));
            EXPECT_TRUE(real.contains(QString::fromUtf8(TitleBarRules).simplified()));
        }

        // ------------------------------------------------------------------------------
        // T-TB.1: the hover fill of the minimize and maximize buttons (TitleBar.qss:97-101)
        // ------------------------------------------------------------------------------

        TEST_F(TitleBarButtonFlatSheetTests, HoverFill_MinimizeAndMaximizeAreWhiteAtTheAlphaTheSheetParsesInEveryFootprintAndRatio)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            for (const Footprint footprint : { Footprint::Main, Footprint::Simple, Footprint::Tabs })
            {
                SCOPED_TRACE(footprintName(footprint));
                const Observed legacy = observeTitleBar(true, footprint, false);
                const Observed flat = observeTitleBar(false, footprint, false);
                ASSERT_EQ(legacy.m_facts.value(QStringLiteral("visible buttons")), QStringLiteral("3"));

                // TitleBar.qss:100 rgba(255,255,255,0.1): Qt parses an alpha of 1 or less as a fraction and truncates it to 25 of 255
                // (QCss::parseColorValue), over the title bar's fill
                const QRgb expected = composedOver(QColor(255, 255, 255, 25));
                for (const qreal ratio : Ratios)
                {
                    for (const char* what : { "minimize", "maximize" })
                    {
                        EXPECT_EQ(cornerOf(legacy, QString::fromLatin1(what), "idle", ratio), QColor(Backdrop).rgb()) << what << " idle";
                        EXPECT_EQ(cornerOf(legacy, QString::fromLatin1(what), "hover", ratio), expected) << what << " hover";
                        EXPECT_EQ(cornerOf(flat, QString::fromLatin1(what), "hover", ratio), expected) << what << " hover (flat)";
                    }
                    // TitleBar.qss:104 #B80D1C
                    const QRgb red = composedOver(QColor(0xB8, 0x0D, 0x1C));
                    EXPECT_EQ(cornerOf(legacy, QStringLiteral("close"), "hover", ratio), red) << "close hover";
                    EXPECT_EQ(cornerOf(flat, QStringLiteral("close"), "hover", ratio), red) << "close hover (flat)";
                }
                expectSameFacts(legacy, flat);
                expectSameImages(legacy, flat);
            }
        }

        // ------------------------------------------------------------------------------
        // T-TB.2: a button under the pointer keeps its hover fill while it is pressed, and while the left button is held elsewhere
        // (TitleBar.qss:97-105 has no :pressed rule, and the sheet does not read the application's mouse buttons)
        // ------------------------------------------------------------------------------

        TEST_F(TitleBarButtonFlatSheetTests, LeftButtonAnchor_HoldsAndLetsGoOfTheApplicationsLeftButton)
        {
            LeftButtonAnchor anchor;
            ASSERT_FALSE(QApplication::mouseButtons().testFlag(Qt::LeftButton));
            anchor.hold(true);
            EXPECT_TRUE(QApplication::mouseButtons().testFlag(Qt::LeftButton));
            anchor.hold(false);
            EXPECT_FALSE(QApplication::mouseButtons().testFlag(Qt::LeftButton));
        }

        TEST_F(TitleBarButtonFlatSheetTests, PressedFill_EveryWindowButtonKeepsItsHoverFillPressedOrDraggedOverAsTheSheetHasNoPressedRule)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            for (const Footprint footprint : { Footprint::Main, Footprint::Simple, Footprint::Tabs })
            {
                SCOPED_TRACE(footprintName(footprint));
                const Observed legacy = observeTitleBar(true, footprint, false);
                const Observed flat = observeTitleBar(false, footprint, false);
                ASSERT_EQ(legacy.m_facts.value(QStringLiteral("visible buttons")), QStringLiteral("3"));

                // TitleBar.qss:100 white at alpha 25 for the minimize and maximize buttons, :104 #B80D1C for the close button
                const QRgb white = composedOver(QColor(255, 255, 255, 25));
                const QRgb red = composedOver(QColor(0xB8, 0x0D, 0x1C));
                const struct
                {
                    const char* m_name;
                    QRgb m_fill;
                } buttons[] = { { "minimize", white }, { "maximize", white }, { "close", red } };
                for (const qreal ratio : Ratios)
                {
                    for (const auto& button : buttons)
                    {
                        const QString name = QString::fromLatin1(button.m_name);
                        for (const char* state : { "hover", "pressed", "dragged" })
                        {
                            EXPECT_EQ(cornerOf(legacy, name, state, ratio), button.m_fill) << button.m_name << " " << state;
                            EXPECT_EQ(cornerOf(flat, name, state, ratio), button.m_fill) << button.m_name << " " << state << " (flat)";
                        }
                    }
                }
                expectSameFacts(legacy, flat);
                expectSameImages(legacy, flat);
            }
        }

        // ------------------------------------------------------------------------------
        // T-TB.2b: the legacy rung keeps the fills of the config where the Style draws the button (the sheet's non-native border,
        // TitleBar.qss:46, keeps QStyleSheetStyle::drawComplexControl from calling the Style for it, so the GUI never shows these)
        // ------------------------------------------------------------------------------

        //! The pixel in the top left corner of a window button the Style draws, the pointer over it
        QRgb legacyStyleFillOf(DockBarButton::WindowDecorationButton type, bool leftHeld)
        {
            RungScope rung(GuiPerfFloor::Rung::Qss);

            DockBarButton button(type);
            button.setFixedSize(32, 32);
            QStyleOptionToolButton option;
            option.initFrom(&button);
            option.rect = QRect(0, 0, 32, 32);
            option.state |= QStyle::State_MouseOver;
            option.subControls = QStyle::SC_ToolButton;
            option.toolButtonStyle = Qt::ToolButtonIconOnly;

            LeftButtonAnchor anchor;
            anchor.hold(leftHeld);
            QImage image(32, 32, QImage::Format_ARGB32_Premultiplied);
            image.fill(Backdrop);
            QPainter painter(&image);
            button.style()->drawComplexControl(QStyle::CC_ToolButton, &option, &painter, &button);
            painter.end();
            return image.pixel(1, 1);
        }

        TEST_F(TitleBarButtonFlatSheetTests, LegacyRung_TheStyleDrawsTheFillsOfTheConfigAndReadsTheApplicationsLeftButton)
        {
            // DockBarButton::defaultConfig, which DockBarButtonConfig.ini leaves as it is
            EXPECT_EQ(legacyStyleFillOf(DockBarButton::MinimizeButton, false), composedOver(QColor(QStringLiteral("#11FFFFFF"))));
            EXPECT_EQ(legacyStyleFillOf(DockBarButton::MaximizeButton, false), composedOver(QColor(QStringLiteral("#11FFFFFF"))));
            EXPECT_EQ(legacyStyleFillOf(DockBarButton::CloseButton, false), composedOver(QColor(QStringLiteral("#B80D1C"))));
            EXPECT_EQ(legacyStyleFillOf(DockBarButton::MinimizeButton, true), composedOver(QColor(QStringLiteral("#1A1A1A"))));
            EXPECT_EQ(legacyStyleFillOf(DockBarButton::MaximizeButton, true), composedOver(QColor(QStringLiteral("#1A1A1A"))));
            EXPECT_EQ(legacyStyleFillOf(DockBarButton::CloseButton, true), composedOver(QColor(QStringLiteral("#850914"))));
        }

        // ------------------------------------------------------------------------------
        // T-TB.3: a disabled close button has no glyph, and no hover fill (TitleBar.qss:142-145, :97-105 need :enabled to hover)
        // ------------------------------------------------------------------------------

        TEST_F(TitleBarButtonFlatSheetTests, DisabledClose_HasNoGlyphAndNoHoverFillWhileTheOtherButtonsKeepTheirGlyph)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            for (const Footprint footprint : { Footprint::Main, Footprint::Simple, Footprint::Tabs })
            {
                SCOPED_TRACE(footprintName(footprint));
                const Observed legacy = observeTitleBar(true, footprint, true);
                const Observed flat = observeTitleBar(false, footprint, true);
                ASSERT_EQ(legacy.m_facts.value(QStringLiteral("visible buttons")), QStringLiteral("3"));

                // Legacy: `titlebar-close-icon: none` is an empty icon, a valid style hint (QStyleSheetStyle::standardIcon); the pointer
                // over a disabled button matches no :hover. Nothing is drawn at all in any of the three states.
                for (const char* state : { "idle", "hover", "pressed" })
                {
                    for (const qreal ratio : Ratios)
                    {
                        EXPECT_FALSE(isDrawnOn(legacy.m_images[keyOf(QStringLiteral("close"), state, ratio)])) << state << " " << ratio;
                        EXPECT_FALSE(isDrawnOn(flat.m_images[keyOf(QStringLiteral("close"), state, ratio)]))
                            << state << " " << ratio << " (flat)";
                    }
                }
                // The other buttons keep a (disabled) glyph: only the close glyph is declared away
                EXPECT_TRUE(isDrawnOn(legacy.m_images[keyOf(QStringLiteral("minimize"), "idle", 1.0)]));
                EXPECT_TRUE(isDrawnOn(legacy.m_images[keyOf(QStringLiteral("maximize"), "idle", 1.0)]));
                expectSameFacts(legacy, flat);
                expectSameImages(legacy, flat);
            }
        }

        // ------------------------------------------------------------------------------
        // The action toolbar of a tab widget: the buttons FancyDocking::AddTitleBarButtons makes for a tabbed floating window
        // ------------------------------------------------------------------------------

        //! A tab widget with a page, the three actions of FancyDocking.cpp:448-475 and one more, and the other tool buttons the rules do
        //! and do not reach: a find bar's close button inside the page (the console's, ConsoleSCB.ui:113), one outside the tab widget,
        //! and a dock bar button in the tab bar
        struct ActionBarTree
        {
            explicit ActionBarTree(QWidget& root)
            {
                m_tabs = new TabWidget(&root);
                m_tabs->setGeometry(0, 0, 320, 120);
                m_page = new QWidget;
                m_find = new QToolButton(m_page);
                m_find->setObjectName(QStringLiteral("closeButton"));
                m_find->setIcon(QIcon(QStringLiteral(":/stylesheet/img/close_x.svg")));
                m_find->setGeometry(0, 0, 24, 24);
                m_tabs->addTab(m_page, QStringLiteral("Page"));

                m_tabs->setActionToolBarVisible(true);
                for (const char* name : { "minimizeButton", "maximizeButton", "closeButton", "otherButton" })
                {
                    auto* action = new QAction(QString::fromLatin1(name), m_tabs);
                    action->setObjectName(QString::fromLatin1(name));
                    m_tabs->addAction(action);
                }

                m_outside = new QToolButton(&root);
                m_outside->setObjectName(QStringLiteral("closeButton"));
                m_outside->setIcon(QIcon(QStringLiteral(":/stylesheet/img/close_x.svg")));
                m_outside->setGeometry(0, 130, 24, 24);

                // The close button of a tab, a child of the tab bar as DockTabBar::tabInserted makes it
                m_dockBarButton = new DockBarButton(DockBarButton::CloseButton, m_tabs->tabBar());
            }

            QToolButton* actionButton(const char* name) const
            {
                return m_tabs->actionToolBar()->findChild<QToolButton*>(QString::fromLatin1(name));
            }

            TabWidget* m_tabs = nullptr;
            QWidget* m_page = nullptr;
            QToolButton* m_find = nullptr;
            QToolButton* m_outside = nullptr;
            DockBarButton* m_dockBarButton = nullptr;
        };

        Observed observeActionBar(bool legacy)
        {
            RungScope rung(legacy ? GuiPerfFloor::Rung::Qss : GuiPerfFloor::Rung::Flat);
            Observed seen;

            QWidget root;
            ActionBarTree tree(root);
            TitleBarButtonFlatSheetTests::showRoot(root, legacy, QSize(320, 170));

            const QSize iconBox(16, 16);
            for (const char* name : { "minimizeButton", "maximizeButton", "closeButton", "otherButton" })
            {
                QToolButton* button = tree.actionButton(name);
                if (!button)
                {
                    ADD_FAILURE() << "no action button " << name;
                    continue;
                }
                const QString what = QString::fromLatin1(name);
                noteIcon(seen, what, button->icon(), iconBox);
                seen.m_facts[what + QStringLiteral("|iconSize")] = QString::fromStdString(sizeText(button->iconSize()));
                seen.m_facts[what + QStringLiteral("|sizeHint")] = QString::fromStdString(sizeText(button->sizeHint()));
                seen.m_facts[what + QStringLiteral("|size")] = QString::fromStdString(sizeText(button->size()));
                shoot(seen, what, button);
                button->setEnabled(false);
                shoot(seen, what + QStringLiteral(" disabled"), button);
                button->setEnabled(true);
            }
            noteIcon(seen, QStringLiteral("find bar close"), tree.m_find->icon(), iconBox);
            noteIcon(seen, QStringLiteral("outside close"), tree.m_outside->icon(), iconBox);
            seen.m_facts[QStringLiteral("element of the dock bar button")] =
                BoxElement::forWidget(tree.m_dockBarButton) ? QStringLiteral("yes") : QStringLiteral("no");
            return seen;
        }

        // ------------------------------------------------------------------------------
        // T-TB.4: the icons of the action toolbar's buttons (TitleBar.qss:107-110, :117-120, :132-135)
        // ------------------------------------------------------------------------------

        TEST_F(TitleBarButtonFlatSheetTests, ActionToolBar_MinimizeMaximizeAndCloseHaveTheIconsOfTheSheetAtEveryRatio)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            const Observed legacy = observeActionBar(true);
            const Observed flat = observeActionBar(false);

            // The sheet sets `icon` at polish over the empty icon of the action (FancyDocking.cpp:448, :458, :468)
            const struct
            {
                const char* m_name;
                const char* m_icon;
            } expected[] = { { "minimizeButton", MinimizeIcon }, { "maximizeButton", MaximizeIcon }, { "closeButton", CloseIcon } };
            for (const auto& button : expected)
            {
                SCOPED_TRACE(button.m_name);
                EXPECT_EQ(legacy.m_facts.value(QString::fromLatin1(button.m_name) + QStringLiteral("|icon null")), QStringLiteral("no"));
                for (const qreal ratio : Ratios)
                {
                    const QImage wanted = QIcon(QString::fromLatin1(button.m_icon)).pixmap(QSize(16, 16), ratio).toImage();
                    EXPECT_EQ(differing(legacy.m_images[keyOf(QString::fromLatin1(button.m_name), "icon", ratio)], wanted), 0) << ratio;
                    EXPECT_EQ(differing(flat.m_images[keyOf(QString::fromLatin1(button.m_name), "icon", ratio)], wanted), 0)
                        << ratio << " (flat)";
                }
            }
            // The fourth action, of another name, has no icon in either GUI
            EXPECT_EQ(legacy.m_facts.value(QStringLiteral("otherButton|icon null")), QStringLiteral("yes"));

            expectSameFacts(legacy, flat);
            expectSameImages(legacy, flat);
        }

        // ------------------------------------------------------------------------------
        // T-TB.5: the rules reach the buttons named so below a tab widget, and no other
        // ------------------------------------------------------------------------------

        TEST_F(TitleBarButtonFlatSheetTests, ActionToolBar_IconRulesReachTheNamedButtonsBelowATabWidgetOnly)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            const Observed legacy = observeActionBar(true);
            const Observed flat = observeActionBar(false);

            // The find bar's close button inside the page is below the tab widget: its icon is the sheet's, not the code's close_x.svg
            for (const qreal ratio : Ratios)
            {
                const QImage sheet = QIcon(QString::fromLatin1(CloseIcon)).pixmap(QSize(16, 16), ratio).toImage();
                const QImage code = QIcon(QStringLiteral(":/stylesheet/img/close_x.svg")).pixmap(QSize(16, 16), ratio).toImage();
                EXPECT_EQ(differing(legacy.m_images[keyOf(QStringLiteral("find bar close"), "icon", ratio)], sheet), 0) << ratio;
                EXPECT_EQ(differing(flat.m_images[keyOf(QStringLiteral("find bar close"), "icon", ratio)], sheet), 0) << ratio << " (flat)";
                // The one outside the tab widget keeps the code's icon in both
                EXPECT_EQ(differing(legacy.m_images[keyOf(QStringLiteral("outside close"), "icon", ratio)], code), 0) << ratio;
                EXPECT_EQ(differing(flat.m_images[keyOf(QStringLiteral("outside close"), "icon", ratio)], code), 0) << ratio << " (flat)";
            }

            // A dock bar button in the tab bar draws its glyph from the style whatever its icon property is (DockBarButton::paintEvent),
            // so the flat GUI gives it no element
            EXPECT_EQ(flat.m_facts.value(QStringLiteral("element of the dock bar button")), QStringLiteral("no"));
        }

        // ------------------------------------------------------------------------------
        // T-TB.6: the hover and pressed fills of the rules reach a plain tool button in no way (TitleBar.qss:97-105, A4)
        // ------------------------------------------------------------------------------

        //! Tool buttons with nothing to paint: the three names, a button auto-raised, and a button of another name
        Observed observePlainButtons(bool legacy)
        {
            RungScope rung(legacy ? GuiPerfFloor::Rung::Qss : GuiPerfFloor::Rung::Flat);
            Observed seen;

            QWidget root;
            auto* layout = new QHBoxLayout(&root);
            struct Plain
            {
                const char* m_what;
                const char* m_name;
                bool m_autoRaise;
            };
            const Plain plain[] = {
                { "close", "closeButton", false },
                { "close auto-raised", "closeButton", true },
                { "minimize", "minimizeButton", false },
                { "maximize auto-raised", "maximizeButton", true },
                { "other", "otherButton", false },
            };
            for (const Plain& entry : plain)
            {
                auto* button = new QToolButton(&root);
                button->setObjectName(QString::fromLatin1(entry.m_name));
                button->setAutoRaise(entry.m_autoRaise);
                button->setFixedSize(24, 24);
                layout->addWidget(button);
            }
            TitleBarButtonFlatSheetTests::showRoot(root, legacy, QSize(200, 40));

            const QList<QToolButton*> buttons = root.findChildren<QToolButton*>();
            for (int index = 0; index < buttons.size(); ++index)
            {
                shoot(seen, QString::fromLatin1(plain[index].m_what), buttons[index]);
            }
            return seen;
        }

        TEST_F(TitleBarButtonFlatSheetTests, PlainToolButtons_LegacyPaintsNoHoverOrPressedFillSoTheFlatGuiPaintsNone)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            const Observed legacy = observePlainButtons(true);
            const Observed flat = observePlainButtons(false);

            // QStyleSheetStyle::drawComplexControl (CC_ToolButton) leaves the background of a tool button with a native border to the base
            // style when the rule is a plain colour (the default rule of QToolButton: -qt-style-features: background-color), and the Style
            // draws a tool button's label and checked fill alone: no state of a button of these names shows a fill
            for (auto it = legacy.m_images.constBegin(); it != legacy.m_images.constEnd(); ++it)
            {
                EXPECT_FALSE(isDrawnOn(it.value())) << it.key().toStdString();
            }
            expectSameImages(legacy, flat);
        }

        // ------------------------------------------------------------------------------
        // T-TB.7: the hover fill of the action toolbar's buttons (TitleBar.qss:97-105)
        // ------------------------------------------------------------------------------

        TEST_F(TitleBarButtonFlatSheetTests, ActionToolBar_HoverFillIsTheRulesWhilePointerIsOverAnEnabledButtonWhetherItIsPressedOrNot)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            const Observed legacy = observeActionBar(true);
            const Observed flat = observeActionBar(false);

            // The toolbar forces the styled background of its buttons (TabWidgetActionToolBar.cpp), so the sheet paints the rule of the
            // state in the whole rect of the button at PE_Widget, before the button paints its label: rgba(255,255,255,0.1) for the
            // minimize and maximize buttons, #B80D1C for the close button. PE_Widget is given the state of the widget, which is never
            // pressed. A button of another name has the idle rule alone, and a disabled one matches no :hover.
            const QRgb idle = QColor(Backdrop).rgb();
            const QRgb white = composedOver(QColor(255, 255, 255, 25));
            const QRgb red = composedOver(QColor(0xB8, 0x0D, 0x1C));
            const struct
            {
                const char* m_name;
                QRgb m_hover;
            } buttons[] = { { "minimizeButton", white }, { "maximizeButton", white }, { "closeButton", red }, { "otherButton", idle } };
            for (const auto& button : buttons)
            {
                const QString name = QString::fromLatin1(button.m_name);
                for (const qreal ratio : Ratios)
                {
                    for (const Observed* twin : { &legacy, &flat })
                    {
                        SCOPED_TRACE(QStringLiteral("%1 at %2 %3").arg(name).arg(ratio).arg(twin == &legacy ? "legacy" : "flat")
                            .toStdString());
                        EXPECT_EQ(cornerOf(*twin, name, "idle", ratio), idle);
                        EXPECT_EQ(cornerOf(*twin, name, "hover", ratio), button.m_hover);
                        EXPECT_EQ(cornerOf(*twin, name, "pressed", ratio), button.m_hover);
                        for (const char* state : { "idle", "hover", "pressed" })
                        {
                            EXPECT_EQ(cornerOf(*twin, name + QStringLiteral(" disabled"), state, ratio), idle) << state;
                        }
                    }
                }
            }
            expectSameFacts(legacy, flat);
            expectSameImages(legacy, flat);
        }
    } // namespace TitleBarButtonFlatSheetTestsInternal
} // namespace AzQtComponents
