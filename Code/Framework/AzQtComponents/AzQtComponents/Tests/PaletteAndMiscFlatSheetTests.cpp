/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Tests/SheetTwins.h>

#include <AzQtComponents/Components/Widgets/OverlayWidget.h>

#include <QComboBox>
#include <QDialog>
#include <QDockWidget>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPushButton>
#include <QStyleOption>

namespace AzQtComponents
{
    // ----------------------------------------------------------------------------------
    // Three gaps of the shared library that the flattening left to the end, each judged by what the legacy GUI drew. The legacy twin is
    // Qt's own style sheet style over AzQtComponents::Style with the application's real sheet; the flat twin is the flattened Style with
    // the Global elements; the harness is SheetTwins.h.
    //   - A18: the colour of a main window, a dialog and a dock widget (BaseStyleSheet.qss:60-66, `color: #cccccc`) as the window's own
    //     WindowText. The tests establish what paints with it (the title of a dock widget alone) and why no palette is carried;
    //   - A20: the placeholder of the line edit an editable combo box makes (QRenderRule::configurePalette: the text colour of the
    //     combo box's rule at half its alpha);
    //   - A21: the push buttons of an overlay layer that dims and has no centre widget (OverlayWidgetLayer.cpp, `AddButtons`).
    // ----------------------------------------------------------------------------------

    namespace PaletteAndMiscFlatSheetTestsInternal
    {
        using namespace SheetTwins;

        class PaletteAndMiscFlatSheetTests
            : public ::UnitTest::LeakDetectionFixture
        {
        protected:
            Context m_context;
        };

        // The roles of the palette a text rule decides
        const std::vector<QPalette::ColorRole> TextRoles = { QPalette::WindowText, QPalette::Text, QPalette::ButtonText };

        // ----------------------------------------------------------------------------------
        // A18: the title of a plain dock widget is the one thing that paints with a window's own WindowText
        // ----------------------------------------------------------------------------------

        //! A dock widget with a title and no buttons, below a plain widget: Qt draws its title strip in CE_DockWidgetTitle with the
        //! dock's own WindowText
        Row dockTitleRow()
        {
            return Row{
                .m_site = "A18 the title of a plain dock widget",
                .m_make = [](QWidget* parent) -> QWidget*
                {
                    auto* dock = new QDockWidget(QStringLiteral("Log"), parent);
                    dock->setFeatures(QDockWidget::NoDockWidgetFeatures);
                    dock->setWidget(new QWidget());
                    return dock;
                },
                .m_realApplicationSheet = true,
                .m_compare = Compare::Geometry | Compare::Fonts | Compare::Pixels,
                .m_size = QSize(160, 80),
                .m_base = Base::AzQtComponentsStyle,
            };
        }

        TEST_F(PaletteAndMiscFlatSheetTests, A18_TheTitleOfADockWidgetIsDrawnInTheDocksColourOfTheSheet)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(dockTitleRow(), m_context));
        }

        //! A window to draw: `top` is what is shown and drawn, `subject` the window whose own palette the test changes (the top itself,
        //! or a dock widget below a plain widget)
        struct Subject
        {
            QWidget* m_top = nullptr;
            QWidget* m_subject = nullptr;
        };

        //! The window as the flattened GUI draws it, with the text roles of the subject's own palette set to a colour (an invalid colour
        //! leaves the palette as it is): the window and everything it holds, onto an image of the device ratio
        template<typename Build>
        QImage drawnWithWindowText(const Context& context, const QColor& windowText, qreal ratio, Build build)
        {
            const Subject window = build();
            window.m_top->setStyle(context.flatStyle());
            for (QWidget* child : window.m_top->findChildren<QWidget*>())
            {
                child->setStyle(context.flatStyle());
            }
            if (windowText.isValid())
            {
                QPalette palette = window.m_subject->palette();
                for (const QPalette::ColorRole role : TextRoles)
                {
                    palette.setColor(role, windowText);
                }
                window.m_subject->setPalette(palette);
            }
            window.m_top->setAttribute(Qt::WA_DontShowOnScreen);
            window.m_top->resize(200, 120);
            window.m_top->show();
            QApplication::processEvents();
            QImage image((QSizeF(window.m_top->size()) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
            image.setDevicePixelRatio(ratio);
            image.fill(QColor(0x80, 0x80, 0x80));
            window.m_top->render(&image, QPoint(), QRegion(), QWidget::DrawWindowBackground | QWidget::DrawChildren);
            delete window.m_top;
            return image;
        }

        //! A dialog that holds nothing that draws text; a main window with two docks (their titles, their separator); a dock widget
        //! below a plain widget (its title, drawn in the dock's own palette): the three classes of the rule, with every part of them that
        //! Qt draws by itself
        Subject buildDialog()
        {
            auto* dialog = new QDialog();
            return { dialog, dialog };
        }

        Subject buildMainWindow()
        {
            auto* window = new QMainWindow();
            window->setCentralWidget(new QWidget());
            for (const char* title : { "Log", "Console" })
            {
                auto* dock = new QDockWidget(QString::fromLatin1(title), window);
                dock->setWidget(new QWidget());
                window->addDockWidget(Qt::LeftDockWidgetArea, dock);
            }
            return { window, window };
        }

        Subject buildDock()
        {
            auto* root = new QWidget();
            auto* dock = new QDockWidget(QStringLiteral("Log"), root);
            dock->setFeatures(QDockWidget::NoDockWidgetFeatures);
            dock->setWidget(new QWidget());
            dock->setGeometry(0, 0, 160, 80);
            return { root, dock };
        }

        TEST_F(PaletteAndMiscFlatSheetTests, A18_NothingAWindowDrawsDependsOnItsOwnTextRoles)
        {
            // The flat dialog, main window and dock widget drawn in the palette of the application, and again with WindowText, Text and
            // ButtonText of the window made magenta: the pixels are the same, at the three ratios. The title of a dock widget is the
            // one thing Qt draws with the dock's WindowText (QStyle::CE_DockWidgetTitle), and Style draws it in the colour of the
            // rule (g_dockTitleText); the dialog and the main window draw no text of their own.
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            const ApplicationScope application(false, m_context);

            const struct
            {
                const char* m_name;
                Subject (*m_build)();
                bool m_hasParts;        //!< more than the window's fill is drawn: a dock's title, a separator
            } windows[] = {
                { "QDialog", &buildDialog, false },
                { "QMainWindow", &buildMainWindow, true },
                { "QDockWidget", &buildDock, true },
            };
            for (const auto& window : windows)
            {
                for (const qreal ratio : ToolTipTwins::Ratios)
                {
                    const QImage plain = drawnWithWindowText(m_context, QColor(), ratio, window.m_build);
                    const QImage magenta = drawnWithWindowText(m_context, QColor(Qt::magenta), ratio, window.m_build);
                    EXPECT_NE(plain.pixel(0, 0), QColor(0x80, 0x80, 0x80).rgb()) << window.m_name << " at ratio " << ratio;     // drawn on
                    EXPECT_EQ(FieldTwins::isDrawnOn(plain), window.m_hasParts) << window.m_name << " at ratio " << ratio;
                    EXPECT_EQ(FieldTwins::differing(plain, magenta), 0)
                        << window.m_name << " at ratio " << ratio << ": " << FieldTwins::firstDifference(plain, magenta);
                }
            }

            // The probe sees a dependence where there is one: a label inside the dialog takes the roles of the dialog (the propagation
            // the last test of A18 reads), and its text turns magenta
            const auto dialogWithLabel = []()
            {
                auto* dialog = new QDialog();
                auto* label = new QLabel(QStringLiteral("Text"), dialog);
                label->move(10, 10);
                return Subject{ dialog, dialog };
            };
            const QImage plain = drawnWithWindowText(m_context, QColor(), 1.0, dialogWithLabel);
            const QImage magenta = drawnWithWindowText(m_context, QColor(Qt::magenta), 1.0, dialogWithLabel);
            EXPECT_GT(FieldTwins::differing(plain, magenta), 0);
        }

        TEST_F(PaletteAndMiscFlatSheetTests, A18_TheLegacyWindowsHaveTheColourAndTheirChildrenTheUniversalRule)
        {
            // What the sheet decided: the window's own roles are #cccccc, the roles of everything inside it are the white of the
            // universal rule (BaseStyleSheet.qss:15-21), which the colour of the window never reached
            RungScope legacyGui(Rung::Qss);
            const ApplicationScope application(true, m_context);

            QDialog dialog;
            auto* label = new QLabel(QStringLiteral("Text"), &dialog);
            QMainWindow window;
            auto* dock = new QDockWidget(QStringLiteral("Log"), &window);
            dock->setWidget(new QWidget());
            for (QWidget* each : { static_cast<QWidget*>(&dialog), static_cast<QWidget*>(&window) })
            {
                each->setStyle(m_context.baseOf(Base::AzQtComponentsStyle));
                for (QWidget* child : each->findChildren<QWidget*>())
                {
                    child->setStyle(m_context.baseOf(Base::AzQtComponentsStyle));
                }
                each->setStyleSheet(FieldTwins::appSheet());
                each->ensurePolished();
                for (QWidget* child : each->findChildren<QWidget*>())
                {
                    child->ensurePolished();
                }
            }

            const QColor cccccc(0xCC, 0xCC, 0xCC);
            const QWidget* const windows[] = { &dialog, &window, dock };
            for (const QWidget* each : windows)
            {
                for (const QPalette::ColorGroup group : { QPalette::Active, QPalette::Inactive, QPalette::Disabled })
                {
                    EXPECT_EQ(each->palette().color(group, QPalette::WindowText), cccccc) << each->metaObject()->className();
                    EXPECT_EQ(each->palette().color(group, QPalette::Text), cccccc) << each->metaObject()->className();
                    EXPECT_EQ(each->palette().color(group, QPalette::ButtonText), cccccc) << each->metaObject()->className();
                }
            }
            for (const QPalette::ColorGroup group : { QPalette::Active, QPalette::Inactive, QPalette::Disabled })
            {
                EXPECT_EQ(label->palette().color(group, QPalette::WindowText), QColor(Qt::white));
                EXPECT_EQ(dock->widget()->palette().color(group, QPalette::WindowText), QColor(Qt::white));
            }
        }

        TEST_F(PaletteAndMiscFlatSheetTests, A18_AColourOnAFlatWindowReachesEveryChildThatHasNoRuleOfItsOwn)
        {
            // Why the flattened GUI carries the colour where it is read (the dock's title) and not as the window's palette: a child
            // takes the roles its parent has set (QWidget::palette), and the flat children have no rule of their own that holds them
            // to white as the universal rule did
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            const ApplicationScope application(false, m_context);

            QDialog dialog;
            auto* label = new QLabel(QStringLiteral("Text"), &dialog);
            dialog.setStyle(m_context.flatStyle());
            label->setStyle(m_context.flatStyle());
            label->ensurePolished();
            ASSERT_EQ(label->palette().color(QPalette::Active, QPalette::WindowText), QColor(Qt::white));

            QPalette palette = dialog.palette();
            palette.setColor(QPalette::WindowText, QColor(0xCC, 0xCC, 0xCC));
            dialog.setPalette(palette);

            EXPECT_EQ(label->palette().color(QPalette::Active, QPalette::WindowText), QColor(0xCC, 0xCC, 0xCC));
        }

        // ----------------------------------------------------------------------------------
        // A20: the placeholder of the line edit of an editable combo box
        // ----------------------------------------------------------------------------------

        QComboBox* makeEditableCombo(QWidget* parent)
        {
            auto* combo = new QComboBox(parent);
            combo->addItems({ QStringLiteral("Small"), QStringLiteral("Medium") });
            combo->setEditable(true);
            combo->setCurrentIndex(-1);
            combo->lineEdit()->setPlaceholderText(QStringLiteral("Hint text"));
            return combo;
        }

        //! The combo box of the widget a state is entered on: the combo box itself, or the line edit it made
        QComboBox* comboOf(QWidget& widget)
        {
            auto* combo = qobject_cast<QComboBox*>(&widget);
            return combo ? combo : qobject_cast<QComboBox*>(widget.parentWidget());
        }

        //! The combo box polished again, as its watcher does when the state of its popup or its error changes
        State comboPolishedAgain()
        {
            return { "the combo box polished again",
                [](QWidget& widget)
                {
                    QComboBox* combo = comboOf(widget);
                    combo->style()->unpolish(combo);
                    combo->style()->polish(combo);
                },
                [](QWidget&) {} };
        }

        //! Both polished again, the line edit first: the order in which a line edit that was asked for its hint before the combo box was
        //! polished gets its style
        State lineEditBeforeComboBox()
        {
            return { "the line edit polished before the combo box",
                [](QWidget& widget)
                {
                    QComboBox* combo = comboOf(widget);
                    QLineEdit* edit = combo->lineEdit();
                    combo->style()->unpolish(combo);
                    edit->style()->unpolish(edit);
                    edit->style()->polish(edit);
                    combo->style()->polish(combo);
                },
                [](QWidget&) {} };
        }

        //! Both polished again, the combo box first: the order of a widget polished by its parent
        State comboBoxBeforeLineEdit()
        {
            return { "the combo box polished before the line edit",
                [](QWidget& widget)
                {
                    QComboBox* combo = comboOf(widget);
                    QLineEdit* edit = combo->lineEdit();
                    combo->style()->unpolish(combo);
                    edit->style()->unpolish(edit);
                    combo->style()->polish(combo);
                    edit->style()->polish(edit);
                },
                [](QWidget&) {} };
        }

        Row placeholderRow()
        {
            return Row{
                .m_site = "A20 the line edit of an editable combo box",
                .m_make = [](QWidget* parent) -> QWidget* { return makeEditableCombo(parent)->lineEdit(); },
                .m_realApplicationSheet = true,
                .m_states = { comboPolishedAgain(), lineEditBeforeComboBox(), comboBoxBeforeLineEdit() },
                .m_compare = Compare::Palette,
                .m_roles = { QPalette::PlaceholderText, QPalette::Text },
                .m_base = Base::AzQtComponentsStyle,
            };
        }

        Row placeholderPixelsRow()
        {
            return Row{
                .m_site = "A20 an editable combo box with a placeholder",
                .m_make = [](QWidget* parent) -> QWidget* { return makeEditableCombo(parent); },
                .m_realApplicationSheet = true,
                .m_states = { comboPolishedAgain(), lineEditBeforeComboBox(), comboBoxBeforeLineEdit() },
                .m_compare = Compare::Geometry | Compare::Pixels,
                .m_size = QSize(200, 18),
                .m_base = Base::AzQtComponentsStyle,
            };
        }

        TEST_F(PaletteAndMiscFlatSheetTests, A20_ThePlaceholderOfTheLineEditIsTheTextOfTheComboBoxAtHalfItsAlpha)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(placeholderRow(), m_context));
        }

        TEST_F(PaletteAndMiscFlatSheetTests, A20_TheComboBoxWithAPlaceholderIsDrawnAsTheSheetDrawsIt)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(placeholderPixelsRow(), m_context));
        }

        // ----------------------------------------------------------------------------------
        // A21: the buttons of an overlay layer that dims and has no centre widget
        // ----------------------------------------------------------------------------------

        //! A layer pushed onto an overlay of the size of the twin's root, with no centre widget and no window, and the buttons given
        QFrame* pushWaitingLayer(QWidget* parent, const OverlayWidgetButtonList& buttons)
        {
            auto* overlay = new OverlayWidget(parent);
            overlay->setGeometry(QRect(QPoint(0, 0), QSize(300, 200)));
            overlay->PushLayer(nullptr, nullptr, "Overlay", buttons);
            for (QFrame* frame : overlay->findChildren<QFrame*>(Qt::FindDirectChildrenOnly))
            {
                if (frame->inherits("AzQtComponents::Internal::OverlayWidgetLayer"))
                {
                    return frame;
                }
            }
            return nullptr;
        }

        //! The button AddButtons makes in the controls of the layer. With `withSheet` false the layer's own sheet is taken off (the
        //! legacy GUI sets it; the flattened GUI sets none, so there is nothing to take off): the button the layer would have had
        //! without it.
        QPushButton* makeLayerButton(QWidget* parent, bool withSheet = true)
        {
            OverlayWidgetButton cancel;
            cancel.m_text = QStringLiteral("Cancel");
            cancel.m_triggersPop = true;
            QFrame* layer = pushWaitingLayer(parent, OverlayWidgetButtonList{ &cancel });
            if (!withSheet)
            {
                layer->setStyleSheet(QString());
            }
            return layer->findChild<QPushButton*>();
        }

        const QSize ButtonSize(84, 32);         //!< the size a "Cancel" button has in either GUI

        //! The button of the layer, in the states of a button but the hover: the hover is read within the legacy GUI, with the sheet of
        //! the layer and without it, by A21_TheSheetOfTheLayerChangesNothingOnTheButton.
        Row layerButtonRow()
        {
            return Row{
                .m_site = "A21 a button of the overlay layer",
                .m_make = [](QWidget* parent) -> QWidget* { return makeLayerButton(parent); },
                .m_realApplicationSheet = true,
                .m_states = { States::pressed(), States::disabled(), States::focus() },
                .m_compare = Compare::Geometry | Compare::Fonts | Compare::Palette | Compare::Pixels,
                .m_roles = TextRoles,
                .m_rootSize = QSize(300, 200),
                .m_base = Base::AzQtComponentsStyle,
            };
        }

        //! The same button at a size of its own, in every state, with or without the sheet of its layer; or a plain button of that size
        Row sizedButtonRow(const char* site, bool inLayer, bool withSheet)
        {
            return Row{
                .m_site = site,
                .m_make = [inLayer, withSheet](QWidget* parent) -> QWidget*
                {
                    QPushButton* button = inLayer ? makeLayerButton(parent, withSheet) : new QPushButton(QStringLiteral("Cancel"), parent);
                    button->setFixedSize(ButtonSize);
                    return button;
                },
                .m_realApplicationSheet = true,
                .m_states = { States::hover(), States::pressed(), States::disabled(), States::focus() },
                .m_compare = Compare::Geometry | Compare::Fonts | Compare::Palette | Compare::Pixels,
                .m_roles = TextRoles,
                .m_rootSize = QSize(300, 200),
                .m_base = Base::AzQtComponentsStyle,
            };
        }

        //! The widget of a row read in one GUI, idle and in each state: the twin is built, read and gone
        std::vector<Reading> readingsIn(bool legacy, const Row& row, const Context& context)
        {
            RungScope rung(legacy ? row.m_legacyRung : Rung::Flat);
            const Twin twin(row, context, legacy);
            return readStates(row, *twin.widget(), context, legacy, nullptr);
        }

        //! What differs between two readings of a widget, for the failure message of a comparison
        std::string differencesOf(const Reading& a, const Reading& b)
        {
            std::string text;
            for (size_t i = 0; i < a.m_facts.size() && i < b.m_facts.size(); ++i)
            {
                if (a.m_facts[i].m_text != b.m_facts[i].m_text)
                {
                    text += a.m_facts[i].m_name + ": " + a.m_facts[i].m_text + " against " + b.m_facts[i].m_text + "; ";
                }
            }
            for (size_t i = 0; i < a.m_pictures.size() && i < b.m_pictures.size(); ++i)
            {
                if (FieldTwins::differing(a.m_pictures[i].m_image, b.m_pictures[i].m_image) != 0)
                {
                    text += a.m_pictures[i].m_name + ": "
                        + FieldTwins::firstDifference(a.m_pictures[i].m_image, b.m_pictures[i].m_image) + "; ";
                }
            }
            return text;
        }

        Row layerWithButtonsRow()
        {
            return Row{
                .m_site = "A21 the overlay layer with a button",
                .m_make = [](QWidget* parent) -> QWidget*
                {
                    OverlayWidgetButton cancel;
                    cancel.m_text = QStringLiteral("Cancel");
                    cancel.m_triggersPop = true;
                    return pushWaitingLayer(parent, OverlayWidgetButtonList{ &cancel });
                },
                .m_realApplicationSheet = true,
                .m_compare = Compare::Geometry | Compare::Pixels,
                .m_size = QSize(300, 200),
                .m_rootSize = QSize(300, 200),
                .m_base = Base::AzQtComponentsStyle,
                .m_mayDecideNothing = true,         // the layer's look is the sheet the site sets: the application's adds nothing to it
            };
        }

        TEST_F(PaletteAndMiscFlatSheetTests, A21_TheButtonOfTheLayerIsTheButtonOfTheSheet)
        {
            EXPECT_TRUE(check(layerButtonRow(), m_context));
        }

        TEST_F(PaletteAndMiscFlatSheetTests, A21_TheLayerWithAButtonIsTheLayerOfTheSheet)
        {
            EXPECT_TRUE(check(layerWithButtonsRow(), m_context));
        }

        TEST_F(PaletteAndMiscFlatSheetTests, A21_TheSheetOfTheLayerChangesNothingOnTheButton)
        {
            // The fill of the layer (`background-color:rgba(0, 0, 0, 179)`, a sheet with no selector) reached the button, and a rule with a
            // background does not paint a push button: its bevel is the base style's (QStyleSheetStyle::drawControl, CE_PushButtonBevel
            // draws the rule's background image only, the colour reaches the palette), and a push button has no styled background
            // (QStyleSheetStyle::polish). The legacy button with the sheet of its layer is the legacy button without it, in every state
            // (hover included), and the flattened GUI has the second.
            const Row withSheet = sizedButtonRow("A21 a button of the layer, with its sheet", true, true);
            const Row withoutSheet = sizedButtonRow("A21 a button of the layer, without its sheet", true, false);
            const std::vector<Reading> with = readingsIn(true, withSheet, m_context);
            const std::vector<Reading> without = readingsIn(true, withoutSheet, m_context);
            ASSERT_EQ(with.size(), 5u);
            ASSERT_EQ(without.size(), with.size());
            for (size_t i = 0; i < with.size(); ++i)
            {
                EXPECT_FALSE(readingsDiffer(with[i], without[i])) << with[i].m_state << ": " << differencesOf(with[i], without[i]);
                ASSERT_FALSE(with[i].m_pictures.empty());
                EXPECT_TRUE(FieldTwins::isDrawnOn(with[i].m_pictures[0].m_image)) << with[i].m_state;
            }

            // The comparison sees a sheet that does paint a button: a border of its own is drawn by the rule (CE_PushButtonBevel draws
            // the rule where its border is not native)
            Row bordered = sizedButtonRow("A21 a button of the layer, with a border of its own", true, false);
            const WidgetFactory make = bordered.m_make;
            bordered.m_make = [make](QWidget* parent) -> QWidget*
            {
                QWidget* button = make(parent);
                button->setStyleSheet(QStringLiteral("border: 1px solid red;"));
                return button;
            };
            const std::vector<Reading> drawn = readingsIn(true, bordered, m_context);
            ASSERT_EQ(drawn.size(), without.size());
            EXPECT_TRUE(readingsDiffer(drawn[0], without[0]));
        }

        TEST_F(PaletteAndMiscFlatSheetTests, A21_TheDimmedFillIsTheBackgroundRolesOfTheLegacyButtonAndNothingPaintsWithThem)
        {
            // What the sheet did to the button: its Button, Window and Base roles are the fill of the layer, in every group
            {
                RungScope legacyGui(Rung::Qss);
                const Row row = sizedButtonRow("A21 a button of the layer", true, true);
                const Twin twin(row, m_context, true);
                for (const QPalette::ColorGroup group : { QPalette::Active, QPalette::Inactive, QPalette::Disabled })
                {
                    for (const QPalette::ColorRole role : { QPalette::Button, QPalette::Window, QPalette::Base })
                    {
                        EXPECT_EQ(twin.widget()->palette().color(group, role), QColor(0, 0, 0, 179)) << group << " " << role;
                    }
                }
            }

            // Nothing the flat button paints reads them: the same roles on the flat button change no pixel in any state
            Row dimmed = sizedButtonRow("A21 a flat button with the roles of the legacy one", false, false);
            dimmed.m_make = [](QWidget* parent) -> QWidget*
            {
                auto* button = new QPushButton(QStringLiteral("Cancel"), parent);
                button->setFixedSize(ButtonSize);
                QPalette palette = button->palette();
                for (const QPalette::ColorGroup group : { QPalette::Active, QPalette::Inactive, QPalette::Disabled })
                {
                    for (const QPalette::ColorRole role : { QPalette::Button, QPalette::Window, QPalette::Base })
                    {
                        palette.setColor(group, role, QColor(0, 0, 0, 179));
                    }
                }
                button->setPalette(palette);
                return button;
            };
            const std::vector<Reading> withRoles = readingsIn(false, dimmed, m_context);
            const std::vector<Reading> plain = readingsIn(false, sizedButtonRow("A21 a flat button", false, false), m_context);
            ASSERT_EQ(withRoles.size(), plain.size());
            for (size_t i = 0; i < plain.size(); ++i)
            {
                ASSERT_EQ(withRoles[i].m_pictures.size(), plain[i].m_pictures.size());
                for (size_t picture = 0; picture < plain[i].m_pictures.size(); ++picture)
                {
                    EXPECT_EQ(FieldTwins::differing(withRoles[i].m_pictures[picture].m_image, plain[i].m_pictures[picture].m_image), 0)
                        << plain[i].m_state << ", " << plain[i].m_pictures[picture].m_name;
                }
            }
        }
    } // namespace PaletteAndMiscFlatSheetTestsInternal
} // namespace AzQtComponents
