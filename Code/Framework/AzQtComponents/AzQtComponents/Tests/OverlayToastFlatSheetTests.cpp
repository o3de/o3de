/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Tests/SheetTwins.h>
#include <AzQtComponents/Tests/SiteTwins.h>

#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/ControlElements.h>
#include <AzQtComponents/Components/Elements/ElementCommon.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/ToastNotification.h>
#include <AzQtComponents/Components/ToastNotificationConfiguration.h>
#include <AzQtComponents/Components/Widgets/OverlayWidget.h>

#include <QEvent>
#include <QFrame>
#include <QHelpEvent>
#include <QLabel>
#include <QPalette>
#include <QWidget>

namespace AzQtComponents
{
    // ----------------------------------------------------------------------------------
    // The toast and the overlay layer: two widgets of AzQtComponents that set a sheet of their own, which the flattened GUI carries by
    // the Global elements of ControlElements.cpp. The legacy twin is Qt's own style sheet style over AzQtComponents::Style with the
    // real application sheet; the sheet of the widget itself is what the real class sets (StyleManager::setLegacyStyleSheet, at the
    // Qss rung). The flat twin is the flattened Style, with no sheet anywhere. Each row builds the real class.
    //   - U008, U009: ToastNotification.ui:59, :98 before the flattening, the fill of the icon frame and the label that cancelled it;
    //   - C019, C020: OverlayWidgetLayer.cpp:35, :69, :103, the sheet with no selector that dims a layer and what is below it.
    // ----------------------------------------------------------------------------------

    namespace OverlayToastFlatSheetTestsInternal
    {
        using namespace SheetTwins;

        class OverlayToastFlatSheetTests
            : public ::UnitTest::LeakDetectionFixture
        {
        protected:
            Context m_context;
        };

        // The roles of the palette a text rule decides. The fills of the legacy rules reach the palette's other roles (a sheet's
        // background is the Base, Button and Window of its widget), which the elements draw with the style and not with the palette.
        const std::vector<QPalette::ColorRole> TextRoles = { QPalette::WindowText, QPalette::Text, QPalette::ButtonText };

        // ----------------------------------------------------------------------------------
        // The toast
        // ----------------------------------------------------------------------------------

        //! A toast as the Editor shows an information: the real class, its form and its sheets
        ToastNotification* makeToast(QWidget* parent)
        {
            auto* toast = new ToastNotification(
                parent, ToastConfiguration(ToastType::Information, QStringLiteral("Title"), QStringLiteral("A description of the toast")));
            toast->setAttribute(Qt::WA_DontShowOnScreen);
            toast->show();
            return toast;
        }

        //! The whole toast: the frame of the icon (U008) and its label (U009) drawn on the toast's own background, at the three ratios.
        //! The palette of the dialog is not compared: its text colour is the QDialog rule's (BaseStyleSheet.qss:62-68), not the toast's.
        Row toastRow()
        {
            return Row{
                .m_site = "U008, U009 the toast",
                .m_make = [](QWidget* parent) -> QWidget* { return makeToast(parent); },
                .m_realApplicationSheet = true,
                .m_compare = Compare::Geometry | Compare::Pixels,
                .m_size = QSize(260, 56),
                .m_base = Base::AzQtComponentsStyle,
            };
        }

        //! The frame that holds the icon, with the label inside it (U008)
        Row iconFrameRow()
        {
            return Row{
                .m_site = "U008 the icon frame of the toast",
                .m_make = [](QWidget* parent) -> QWidget* { return makeToast(parent)->findChild<QFrame*>(QStringLiteral("icon_frame")); },
                .m_realApplicationSheet = true,
                .m_compare = Compare::Geometry | Compare::Palette | Compare::Pixels,
                .m_roles = TextRoles,
                .m_size = QSize(42, 56),
                .m_base = Base::AzQtComponentsStyle,
            };
        }

        //! The label of the icon (U009): it draws no fill in either GUI
        Row iconLabelRow()
        {
            return Row{
                .m_site = "U009 the icon label of the toast",
                .m_make = [](QWidget* parent) -> QWidget* { return makeToast(parent)->findChild<QLabel*>(QStringLiteral("iconLabel")); },
                .m_realApplicationSheet = true,
                .m_compare = Compare::Geometry | Compare::Palette | Compare::Pixels,
                .m_roles = TextRoles,
                .m_size = QSize(32, 32),
                .m_base = Base::AzQtComponentsStyle,
            };
        }

        TEST_F(OverlayToastFlatSheetTests, Toast_IsTheToastOfTheSheet)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(toastRow(), m_context));
        }

        TEST_F(OverlayToastFlatSheetTests, Toast_TheIconFrameIsTheFrameOfTheSheet)
        {
            EXPECT_TRUE(check(iconFrameRow(), m_context));
        }

        TEST_F(OverlayToastFlatSheetTests, Toast_TheIconLabelIsTheLabelOfTheSheet)
        {
            EXPECT_TRUE(check(iconLabelRow(), m_context));
        }

        TEST_F(OverlayToastFlatSheetTests, Toast_TheFillIsTheValueTheLegacySheetIsBuiltFrom)
        {
            // The legacy sheet is what the form had: ToastNotification.ui:59 and :98 before the flattening
            const Row row = iconFrameRow();
            RungScope rung(Rung::Qss);
            const Twin twin(row, m_context, true);

            QFrame* frame = qobject_cast<QFrame*>(twin.widget());
            ASSERT_NE(frame, nullptr);
            QLabel* label = frame->findChild<QLabel*>(QStringLiteral("iconLabel"));
            ASSERT_NE(label, nullptr);
            EXPECT_EQ(frame->styleSheet(), QStringLiteral("background-color: rgba(255, 255, 255, 20);"));
            EXPECT_EQ(label->styleSheet(), QStringLiteral("background-color: rgba(255, 255, 255, 0);"));
        }

        TEST_F(OverlayToastFlatSheetTests, Toast_TheCensusListsTheSheetsInTheLegacyGuiAndNoneInTheFlatOne)
        {
            const auto build = []()
            {
                QWidget root;
                makeToast(&root);
                root.ensurePolished();
            };

            const QStringList legacy = SiteTwins::sheetLinesAt(Rung::Qss, build);
            ASSERT_EQ(legacy.size(), 2) << legacy.join("\n").toStdString();
            EXPECT_TRUE(SiteTwins::namesCarrier(legacy[0], QStringLiteral("QFrame#icon_frame"))) << legacy[0].toStdString();
            EXPECT_TRUE(SiteTwins::namesCarrier(legacy[1], QStringLiteral("QLabel#iconLabel"))) << legacy[1].toStdString();

            const QStringList flat = SiteTwins::sheetLinesAt(Rung::Flat, build);
            EXPECT_TRUE(flat.isEmpty()) << flat.join("\n").toStdString();
        }

        // ----------------------------------------------------------------------------------
        // The overlay layer
        // ----------------------------------------------------------------------------------

        //! An overlay of the size of the twin's root, with one layer pushed onto it; the layer is the widget a row compares. The class
        //! is not exported, so the layer is found by its class name, as the elements find it.
        QFrame* pushLayer(QWidget* parent, QWidget* centerWidget, QWidget* breakoutWidget, const OverlayWidgetButtonList& buttons)
        {
            auto* overlay = new OverlayWidget(parent);
            overlay->setGeometry(QRect(QPoint(0, 0), QSize(300, 200)));
            overlay->PushLayer(centerWidget, breakoutWidget, "Overlay", buttons);
            for (QFrame* frame : overlay->findChildren<QFrame*>(Qt::FindDirectChildrenOnly))
            {
                if (frame->inherits("AzQtComponents::Internal::OverlayWidgetLayer"))
                {
                    return frame;
                }
            }
            return nullptr;
        }

        //! The label the importer shows in the layer while an inspector is open (AssetImporterWindow.cpp:699-701)
        QLabel* inspectorLabel()
        {
            auto* label = new QLabel(QStringLiteral("Please close the inspector to continue editing the settings."));
            label->setWordWrap(true);
            label->setAlignment(Qt::AlignCenter);
            return label;
        }

        //! The layer of a breakout window: the label in the middle, the inspector in a window of its own (AssetImporterWindow.cpp:708)
        Row breakoutLayerRow()
        {
            return Row{
                .m_site = "C019 the overlay layer with a breakout window",
                .m_make = [](QWidget* parent) -> QWidget*
                {
                    OverlayWidgetButton close;
                    close.m_text = QStringLiteral("Close");
                    close.m_triggersPop = true;
                    return pushLayer(parent, inspectorLabel(), new QWidget(), OverlayWidgetButtonList{ &close });
                },
                .m_realApplicationSheet = true,
                .m_compare = Compare::Geometry | Compare::Fonts | Compare::Palette | Compare::Pixels,
                .m_roles = TextRoles,
                .m_size = QSize(300, 200),
                .m_rootSize = QSize(300, 200),
                .m_base = Base::AzQtComponentsStyle,
            };
        }

        //! The label of that layer: dimmed again over the layer's fill, with the margin every label has
        Row breakoutLabelRow()
        {
            return Row{
                .m_site = "C019 the label in the overlay layer",
                .m_make = [](QWidget* parent) -> QWidget*
                {
                    OverlayWidgetButton close;
                    close.m_text = QStringLiteral("Close");
                    close.m_triggersPop = true;
                    QLabel* label = inspectorLabel();
                    pushLayer(parent, label, new QWidget(), OverlayWidgetButtonList{ &close });
                    return label;
                },
                .m_realApplicationSheet = true,
                .m_compare = Compare::Geometry | Compare::Fonts | Compare::Palette | Compare::Pixels,
                .m_roles = TextRoles,
                .m_size = QSize(280, 60),
                .m_rootSize = QSize(300, 200),
                .m_base = Base::AzQtComponentsStyle,
            };
        }

        //! The layer the importer pushes while a file is processed: no widget in the middle, no window (AssetImporterWindow.cpp:404)
        Row waitingLayerRow()
        {
            return Row{
                .m_site = "C020 the overlay layer with no centre widget",
                .m_make = [](QWidget* parent) -> QWidget* { return pushLayer(parent, nullptr, nullptr, OverlayWidgetButtonList()); },
                .m_realApplicationSheet = true,
                .m_compare = Compare::Geometry | Compare::Palette | Compare::Pixels,
                .m_roles = TextRoles,
                .m_size = QSize(300, 200),
                .m_rootSize = QSize(300, 200),
                .m_base = Base::AzQtComponentsStyle,
                .m_mayDecideNothing = true,         // the layer's look is the sheet the site sets: the application's adds nothing to it
            };
        }

        //! The frame of that layer's own form that holds its middle (m_center, OverlayWidgetLayer.ui)
        Row waitingFrameRow()
        {
            return Row{
                .m_site = "C020 the centre frame of the overlay layer",
                .m_make = [](QWidget* parent) -> QWidget*
                { return pushLayer(parent, nullptr, nullptr, OverlayWidgetButtonList())->findChild<QFrame*>(QStringLiteral("m_center")); },
                .m_realApplicationSheet = true,
                .m_compare = Compare::Geometry | Compare::Palette | Compare::Pixels,
                .m_roles = TextRoles,
                .m_size = QSize(300, 200),
                .m_rootSize = QSize(300, 200),
                .m_base = Base::AzQtComponentsStyle,
                .m_mayDecideNothing = true,         // the frame's look is the sheet the site sets: the application's adds nothing to it
            };
        }

        //! The frame of that layer's own form that holds its buttons (m_controls, OverlayWidgetLayer.ui). The layer shows it when it has
        //! a button (OverlayWidgetLayer::AddButtons), and no site in the tree passes one to a layer of this kind; the row shows it with
        //! no button, so that the frame alone is compared (a QPushButton below a layer that dims is not carried).
        Row waitingControlsRow()
        {
            return Row{
                .m_site = "C020 the frame of the buttons of the overlay layer",
                .m_make = [](QWidget* parent) -> QWidget*
                {
                    auto* controls = pushLayer(parent, nullptr, nullptr, OverlayWidgetButtonList())
                                         ->findChild<QFrame*>(QStringLiteral("m_controls"));
                    controls->show();
                    return controls;
                },
                .m_realApplicationSheet = true,
                .m_compare = Compare::Geometry | Compare::Palette | Compare::Pixels,
                .m_roles = TextRoles,
                .m_size = QSize(300, 30),
                .m_rootSize = QSize(300, 200),
                .m_base = Base::AzQtComponentsStyle,
                .m_mayDecideNothing = true,         // the frame's look is the sheet the site sets: the application's adds nothing to it
            };
        }

        //! A layer that does not dim: a centre widget and no window set no sheet (OverlayWidgetLayer.cpp:96-101), so nothing is drawn
        //! on it in either GUI
        Row plainLayerRow()
        {
            return Row{
                .m_site = "a layer with a centre widget and no window dims nothing",
                .m_make = [](QWidget* parent) -> QWidget*
                { return pushLayer(parent, new QLabel(QStringLiteral("Centre")), nullptr, OverlayWidgetButtonList()); },
                .m_realApplicationSheet = true,
                .m_compare = Compare::Geometry | Compare::Palette | Compare::Pixels,
                .m_roles = TextRoles,
                .m_size = QSize(300, 200),
                .m_rootSize = QSize(300, 200),
                .m_base = Base::AzQtComponentsStyle,
                .m_mayDrawNothing = true,
            };
        }

        //! The tip of the label in the layer: the sheet with no selector reached it, and its fill outranks ToolTip.qss's black
        Row layerLabelToolTipRow()
        {
            return Row{
                .m_site = "C019 the tip of a label in the overlay layer",
                .m_make = [](QWidget* parent) -> QWidget*
                {
                    OverlayWidgetButton close;
                    close.m_text = QStringLiteral("Close");
                    close.m_triggersPop = true;
                    QLabel* label = inspectorLabel();
                    label->setToolTip(QStringLiteral("Tip"));
                    pushLayer(parent, label, new QWidget(), OverlayWidgetButtonList{ &close });
                    return label;
                },
                .m_sheets = { { "BaseStyleSheet.qss:15-21, ToolTip.qss:10-20", SetOn::Application, ToolTipTwins::AppRules } },
                .m_compare = Compare::ToolTip,
                .m_size = QSize(280, 60),
                .m_rootSize = QSize(300, 200),
            };
        }

        TEST_F(OverlayToastFlatSheetTests, Overlay_TheLayerOfABreakoutWindowIsTheLayerOfTheSheet)
        {
            EXPECT_TRUE(check(breakoutLayerRow(), m_context));
        }

        TEST_F(OverlayToastFlatSheetTests, Overlay_TheLabelInTheLayerIsTheLabelOfTheSheet)
        {
            EXPECT_TRUE(check(breakoutLabelRow(), m_context));
        }

        TEST_F(OverlayToastFlatSheetTests, Overlay_TheLayerWithNoCentreWidgetIsTheLayerOfTheSheet)
        {
            EXPECT_TRUE(check(waitingLayerRow(), m_context));
        }

        TEST_F(OverlayToastFlatSheetTests, Overlay_TheCentreFrameOfTheLayerIsTheFrameOfTheSheet)
        {
            EXPECT_TRUE(check(waitingFrameRow(), m_context));
        }

        TEST_F(OverlayToastFlatSheetTests, Overlay_TheButtonsFrameOfTheLayerIsTheFrameOfTheSheet)
        {
            EXPECT_TRUE(check(waitingControlsRow(), m_context));
        }

        TEST_F(OverlayToastFlatSheetTests, Overlay_ALayerWithACentreWidgetAndNoWindowDimsNothing)
        {
            EXPECT_TRUE(check(plainLayerRow(), m_context));
        }

        TEST_F(OverlayToastFlatSheetTests, Overlay_TheTipOfALabelInTheLayerIsTheTipOfTheSheet)
        {
            EXPECT_TRUE(check(layerLabelToolTipRow(), m_context));
        }

        TEST_F(OverlayToastFlatSheetTests, Overlay_ATipQtMakesAChildOfALabelInTheLayerIsNoLabelOfTheLayer)
        {
            // Qt makes the tip a child of the widget it is shown for wherever it can (not on Windows, where this process runs): the tip
            // is a QLabel below the layer then, and the sheet's `QLabel` never reached a tip (it matches `QToolTip` alone)
            RungScope flat(Rung::Flat);
            QWidget root;
            OverlayWidgetButton close;
            close.m_text = QStringLiteral("Close");
            close.m_triggersPop = true;
            QLabel* label = inspectorLabel();
            label->setToolTip(QStringLiteral("Tip"));
            pushLayer(&root, label, new QWidget(), OverlayWidgetButtonList{ &close });
            for (QWidget* each : root.findChildren<QWidget*>())
            {
                each->setStyle(m_context.flatStyle());
            }
            root.setAttribute(Qt::WA_DontShowOnScreen);
            root.show();
            label->ensurePolished();

            // The label is a label of the layer
            const BoxElement* labelBox = BoxElement::forWidget(label);
            ASSERT_NE(labelBox, nullptr);
            EXPECT_STREQ(labelBox->m_name, "Global.Box.OverlayLayerLabel");

            // The tip, made by the help event
            const QPoint local(2, 2);
            QHelpEvent help(QEvent::ToolTip, local, label->mapToGlobal(local));
            QApplication::sendEvent(label, &help);
            QLabel* tip = ToolTipTwins::findTip();
            ASSERT_NE(tip, nullptr);
            tip->setStyle(m_context.flatStyle());
            BoxElement::resolve(tip);
            const BoxElement* standalone = BoxElement::forWidget(tip);

            // Made a child of the label, as Qt does where it can, it has the element it had
            tip->setParent(label, tip->windowFlags());
            BoxElement::resolve(tip);
            EXPECT_EQ(BoxElement::forWidget(tip), standalone);
            EXPECT_NE(BoxElement::forWidget(tip), labelBox);

            // ~QTipLabel clears QToolTip's instance, so that the next tip is a new one
            delete tip;
        }

        TEST_F(OverlayToastFlatSheetTests, Overlay_TheLegacySheetIsTheValueTheElementsReadAndOnlyTheDimmingLayersGetOne)
        {
            const auto sheetsOf = [](Rung rung, QStringList& found)
            {
                RungScope scope(rung);
                QWidget root;
                OverlayWidgetButton close;
                close.m_text = QStringLiteral("Close");
                QFrame* breakout = pushLayer(&root, inspectorLabel(), new QWidget(), OverlayWidgetButtonList{ &close });
                QFrame* waiting = pushLayer(&root, nullptr, nullptr, OverlayWidgetButtonList());
                QFrame* plain = pushLayer(&root, new QLabel(QStringLiteral("Centre")), nullptr, OverlayWidgetButtonList());
                found << breakout->styleSheet() << waiting->styleSheet() << plain->styleSheet();
                const auto dimmed = [](const QWidget* layer)
                {
                    return Selectors::tagged(layer, ControlElements::OverlayDimmedTag) ? QStringLiteral("true") : QString();
                };
                found << dimmed(breakout) << dimmed(waiting) << dimmed(plain);
            };

            // The legacy GUI sets the sheet on the two layers that dim, as OverlayWidgetLayer.cpp did, and tags nothing
            QStringList legacy;
            sheetsOf(Rung::Qss, legacy);
            const QString sheet = QStringLiteral("background-color:rgba(0, 0, 0, 179)");
            EXPECT_EQ(legacy, (QStringList{ sheet, sheet, QString(), QString(), QString(), QString() }));

            // The flattened GUI sets no sheet and tags the two layers the elements dim
            QStringList flat;
            sheetsOf(Rung::Flat, flat);
            EXPECT_EQ(flat, (QStringList{ QString(), QString(), QString(), QStringLiteral("true"), QStringLiteral("true"), QString() }));
        }

        TEST_F(OverlayToastFlatSheetTests, Overlay_TheCensusListsTheSheetsInTheLegacyGuiAndNoneInTheFlatOne)
        {
            const auto build = []()
            {
                QWidget root;
                OverlayWidgetButton close;
                close.m_text = QStringLiteral("Close");
                pushLayer(&root, inspectorLabel(), new QWidget(), OverlayWidgetButtonList{ &close });
                pushLayer(&root, nullptr, nullptr, OverlayWidgetButtonList());
                root.ensurePolished();
            };

            const QStringList legacy = SiteTwins::sheetLinesAt(Rung::Qss, build);
            // The two layers that dim, one line each (the layer that has the form of its own is named by it)
            ASSERT_EQ(legacy.size(), 2) << legacy.join(QLatin1Char('\n')).toStdString();
            for (const QString& line : legacy)
            {
                EXPECT_TRUE(SiteTwins::namesCarrier(line, QStringLiteral("AzQtComponents::Internal::OverlayWidgetLayer")))
                    << line.toStdString();
            }

            const QStringList flat = SiteTwins::sheetLinesAt(Rung::Flat, build);
            EXPECT_TRUE(flat.isEmpty()) << flat.join("\n").toStdString();
        }
    } // namespace OverlayToastFlatSheetTestsInternal
} // namespace AzQtComponents
