/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Tests/SheetTwins.h>

#include <AzQtComponents/Components/FilteredSearchWidget.h>
#include <AzQtComponents/Components/Widgets/BreadCrumbs.h>
#include <AzQtComponents/Components/Widgets/Card.h>
#include <AzQtComponents/Components/Widgets/CardHeader.h>

#include <QLineEdit>
#include <QMenuBar>
#include <QPaintEvent>
#include <QPainter>
#include <QPushButton>
#include <QStyleOption>

namespace AzQtComponents
{
    // ----------------------------------------------------------------------------------
    // The table-driven twin harness (SheetTwins.h), proven on three sites that wave I.3c of the flattening converted, and on rows that
    // are meant to fail:
    //   - the chip of the selected count in a search box (FilteredSearchWidget.qss:68-77), a sheet on the application, quoted;
    //   - the path edit of the breadcrumbs (BreadCrumbs.qss:26-31), under the real application sheet;
    //   - the tip of a card header (Card.cpp:167), a sheet on the widget over the application's tip rules, found through the widget.
    // The rest tests what the harness does: its states, its checks of a twin as a twin, the expected failure, and the registrar.
    // ----------------------------------------------------------------------------------

    namespace SheetTwinsTestsInternal
    {
        using namespace SheetTwins;

        class SheetTwinsTests
            : public ::UnitTest::LeakDetectionFixture
        {
        protected:
            Context m_context;
        };

        // ----------------------------------------------------------------------------------
        // The rows
        // ----------------------------------------------------------------------------------

        // BaseStyleSheet.qss:15-21, the universal rule of the application's sheet
        const char* const UniversalRule = R"qss(
            * { background-color: transparent; color: white; font-family: "Open Sans"; font-weight: 400; }
        )qss";

        // FilteredSearchWidget.qss:68-77
        const char* const SelectionCountRule = R"qss(
            AzQtComponents--SelectionCountButton { padding: 0px; border-radius: 2px; border: 1px solid #808080;
                background-color: rgb(46, 46, 46); min-height: 22px; max-height: 22px; }
        )qss";

        // Text.qss:89-95, the rule of every label: the label inside the chip has no other
        const char* const LabelRule = R"qss(
            .Label, QLabel { font-family: "Open Sans"; font-size: 12px; margin-top: 1px; margin-bottom: 1px; }
        )qss";

        //! The chip of the selected count, as the search box makes it. The legacy GUI sizes it by the rule's minimum and maximum height
        //! (22 and the 1px line on each side: 24); the flattened one fixes the height in its constructor.
        Row chipRow()
        {
            return Row{
                .m_site = "I.3c SelectionCountButton chip",
                .m_make = [](QWidget* parent) -> QWidget* { return new SelectionCountButton(parent); },
                .m_sheets = {
                    { "BaseStyleSheet.qss:15-21", SetOn::Application, UniversalRule },
                    { "FilteredSearchWidget.qss:68-77", SetOn::Application, SelectionCountRule },
                    { "Text.qss:89-95", SetOn::Application, LabelRule },
                },
                .m_states = { States::hover(), States::disabled() },
                .m_compare = Compare::Geometry | Compare::Fonts | Compare::Palette | Compare::Pixels,
                .m_roles = { QPalette::WindowText, QPalette::Text, QPalette::ButtonText },
                .m_size = QSize(120, 40),
            };
        }

        //! The line edit the breadcrumbs show in place of the path while it is edited, under the application's real sheet (the rules
        //! that reach it: BaseStyleSheet.qss:15-21 and :68-84, LineEdit.qss:11-20 and BreadCrumbs.qss:26-31)
        Row pathEditRow()
        {
            return Row{
                .m_site = "I.3c breadcrumbs path edit",
                .m_make = [](QWidget* parent) -> QWidget*
                {
                    auto* crumbs = new BreadCrumbs(parent);
                    crumbs->setEditable(true);
                    QLineEdit* edit = crumbs->findChild<QLineEdit*>();
                    edit->setText(QStringLiteral("DefaultLevel.prefab"));
                    return edit;
                },
                .m_realApplicationSheet = true,
                .m_states = { States::hover(), States::disabled(), States::focus() },
                .m_compare = Compare::Geometry | Compare::Fonts | Compare::Palette | Compare::Pixels,
                .m_size = QSize(200, 24),
            };
        }

        // Card.qss:172-175, the text of every widget in a modified header, a tip shown for one included
        const char* const ModifiedRule = R"qss(
            AzQtComponents--CardHeader[contentModified=true] * { color: #E57829; }
        )qss";

        //! The tip of a card header: the application's tip rules (BaseStyleSheet.qss:15-21 and ToolTip.qss:10-20, ToolTipTwins::AppRules),
        //! the padding the card gave its header's own sheet (Card.cpp:167), and the orange text of a modified header. A tip finds the
        //! rules of the widget it is shown for; the state is the header's property.
        Row cardHeaderTipRow()
        {
            return Row{
                .m_site = "I.3c card header tooltip padding",
                .m_make = [](QWidget* parent) -> QWidget*
                {
                    auto* card = new Card(parent);
                    card->setTitle(QStringLiteral("A card"));
                    card->header()->setToolTip(QStringLiteral("Tip"));
                    return card->header();
                },
                .m_sheets = {
                    { "BaseStyleSheet.qss:15-21, ToolTip.qss:10-20", SetOn::Application, ToolTipTwins::AppRules },
                    { "Card.qss:172-175", SetOn::Application, ModifiedRule },
                    { "Card.cpp:167", SetOn::Widget, "QToolTip { padding: 5px; }" },
                },
                .m_states = { States::property("contentModified", true) },
                .m_compare = Compare::ToolTip,
            };
        }

        //! The path edit under a sheet that is not the real one: a red field in place of the white one. Meant to fail.
        Row brokenRow()
        {
            Row row = pathEditRow();
            row.m_site = "broken: the path edit under a red field";
            row.m_realApplicationSheet = false;
            row.m_sheets = { { "BreadCrumbs.qss:26-31 (altered)", SetOn::Application,
                "QLineEdit { border: 0px; background: red; color: black; }" } };
            row.m_expectedDifference = "pixels at ratio 1";
            return row;
        }

        // ----------------------------------------------------------------------------------
        // The sites
        // ----------------------------------------------------------------------------------

        TEST_F(SheetTwinsTests, Row_TheChipOfTheSelectedCountIsTheChipOfTheSheet)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(chipRow(), m_context));
        }

        TEST_F(SheetTwinsTests, Row_ThePathEditOfTheBreadcrumbsIsThePathEditOfTheSheet)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(pathEditRow(), m_context));
        }

        TEST_F(SheetTwinsTests, Row_TheTipOfACardHeaderIsTheTipOfTheSheet)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(cardHeaderTipRow(), m_context));
        }

        TEST_F(SheetTwinsTests, Row_ARowUnderASheetThatIsNotTheRealOneFailsWithTheDifferenceItNames)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            // The row is expected to fail, and does
            EXPECT_TRUE(check(brokenRow(), m_context));

            // And the difference is the field, in every ratio: not a byproduct of the harness
            const Result result = run(brokenRow(), m_context);
            EXPECT_FALSE(result.ok());
            for (const char* ratio : { "1", "1.5", "2" })
            {
                bool found = false;
                for (const std::string& difference : result.m_differences)
                {
                    found = found || difference.find(std::string("pixels at ratio ") + ratio + ":") != std::string::npos;
                }
                EXPECT_TRUE(found) << "ratio " << ratio << "\n" << result.text();
            }
        }

        // ----------------------------------------------------------------------------------
        // What the harness checks of a row
        // ----------------------------------------------------------------------------------

        TEST_F(SheetTwinsTests, Check_ARowExpectedToFailThatPassesFails)
        {
            Row row = pathEditRow();
            row.m_expectedDifference = "pixels at ratio";
            EXPECT_FALSE(check(row, m_context));
        }

        TEST_F(SheetTwinsTests, Check_ARowExpectedToFailWithAnotherDifferenceFails)
        {
            Row row = brokenRow();
            row.m_expectedDifference = "geometry.sizeHint";
            EXPECT_FALSE(check(row, m_context));
        }

        TEST_F(SheetTwinsTests, Run_ARowWithNoSheetHasNoLegacyTwin)
        {
            Row row = pathEditRow();
            row.m_realApplicationSheet = false;

            const Result result = run(row, m_context);
            ASSERT_FALSE(result.ok());
            EXPECT_NE(result.text().find("the legacy twin is not under a style sheet style"), std::string::npos) << result.text();
        }

        TEST_F(SheetTwinsTests, Run_ASiteThatStillSetsASheetInTheFlatRungIsADifference)
        {
            Row row = pathEditRow();
            row.m_make = [](QWidget* parent) -> QWidget*
            {
                auto* edit = new QLineEdit(parent);
                edit->setStyleSheet(QStringLiteral("QLineEdit { color: black; }"));
                return edit;
            };

            const Result result = run(row, m_context);
            EXPECT_NE(result.text().find("the flat twin still has a style sheet on QLineEdit"), std::string::npos) << result.text();
        }

        TEST_F(SheetTwinsTests, Run_ARowThatComparesNothingIsADifference)
        {
            Row row = chipRow();
            row.m_compare = 0;

            const Result result = run(row, m_context);
            EXPECT_NE(result.text().find("the row compares nothing"), std::string::npos) << result.text();
        }

        TEST_F(SheetTwinsTests, Run_APictureTheLegacyTwinDrewNothingOnIsADifference)
        {
            // A sheet that draws nothing on a plain widget: the twins agree on the grey, which proves nothing
            Row row;
            row.m_site = "a widget nothing is drawn on";
            row.m_make = [](QWidget* parent) -> QWidget* { return new QWidget(parent); };
            row.m_sheets = { { "BaseStyleSheet.qss:15-21", SetOn::Application, UniversalRule } };
            row.m_compare = Compare::Pixels;
            row.m_size = QSize(40, 20);

            const Result result = run(row, m_context);
            EXPECT_NE(result.text().find("the legacy twin drew nothing"), std::string::npos) << result.text();

            row.m_mayDrawNothing = true;
            row.m_mayDecideNothing = true;
            EXPECT_TRUE(run(row, m_context).ok()) << run(row, m_context).text();
        }

        // ----------------------------------------------------------------------------------
        // The states
        // ----------------------------------------------------------------------------------

        TEST_F(SheetTwinsTests, States_EachIsEnteredAndLeft)
        {
            QWidget root;
            auto* button = new QPushButton(QStringLiteral("Button"), &root);
            root.setStyle(m_context.baseOf(Base::Fusion));
            button->setStyle(m_context.baseOf(Base::Fusion));
            root.setAttribute(Qt::WA_DontShowOnScreen);
            root.show();
            QApplication::processEvents();

            const auto drive = [&](const State& state, const std::function<bool()>& isEntered)
            {
                SCOPED_TRACE(state.m_name);
                ASSERT_FALSE(isEntered());
                state.m_enter(*button);
                QApplication::processEvents();
                EXPECT_TRUE(isEntered());
                state.m_leave(*button);
                QApplication::processEvents();
                EXPECT_FALSE(isEntered());
            };

            drive(States::hover(), [&] { return button->underMouse(); });
            drive(States::pressed(), [&] { return button->isDown(); });
            drive(States::checked(), [&] { return button->isChecked(); });
            drive(States::disabled(), [&] { return !button->isEnabled(); });
            drive(States::focus(), [&] { return button->hasFocus(); });
            drive(States::property("flat", true), [&] { return button->property("flat").toBool(); });
        }

        //! What the legacy twin of a row showed idle and in each state
        std::vector<Reading> readLegacy(const Row& row, Context& context)
        {
            RungScope rung(row.m_legacyRung);
            const Twin twin(row, context, true);
            return readStates(row, *twin.widget(), context, true, nullptr);
        }

        const Fact* findFact(const Reading& reading, const std::string& name)
        {
            for (const Fact& fact : reading.m_facts)
            {
                if (fact.m_name == name)
                {
                    return &fact;
                }
            }
            return nullptr;
        }

        TEST_F(SheetTwinsTests, States_AStateChangesWhatTheLegacyTwinShowsWhereTheSheetHasARuleForIt)
        {
            // LineEdit.qss:28-35 `QLineEdit:disabled`: the field greys, so its pixels change. Nothing in the sheet answers hover or focus.
            const std::vector<Reading> pathEdit = readLegacy(pathEditRow(), m_context);
            ASSERT_EQ(pathEdit.size(), 4u);
            ASSERT_EQ(pathEdit[2].m_state, "disabled");
            EXPECT_GT(FieldTwins::differing(pathEdit[0].m_pictures[0].m_image, pathEdit[2].m_pictures[0].m_image), 0);
            EXPECT_EQ(FieldTwins::differing(pathEdit[0].m_pictures[0].m_image, pathEdit[1].m_pictures[0].m_image), 0);

            // Card.qss:172-175: the text of the tip of a modified header is orange, the property is the header's
            ToolTipTwins::Guard tips;
            const std::vector<Reading> tip = readLegacy(cardHeaderTipRow(), m_context);
            ASSERT_EQ(tip.size(), 2u);
            ASSERT_TRUE(findFact(tip[0], "tip.text") && findFact(tip[1], "tip.text"));
            EXPECT_EQ(findFact(tip[0], "tip.text")->m_text, "#ffffffff");
            EXPECT_EQ(findFact(tip[1], "tip.text")->m_text, "#ffe57829");
        }

        TEST_F(SheetTwinsTests, States_ARowReadsTheWidgetIdleAndThenInEachStateOfItsList)
        {
            Row row = chipRow();
            row.m_states = { States::hover(), States::disabled() };

            const Twin twin(row, m_context, true);
            const std::vector<Reading> readings = readStates(row, *twin.widget(), m_context, true, nullptr);
            ASSERT_EQ(readings.size(), 3u);
            EXPECT_EQ(readings[0].m_state, "idle");
            EXPECT_EQ(readings[1].m_state, "hover");
            EXPECT_EQ(readings[2].m_state, "disabled");
            EXPECT_TRUE(twin.widget()->isEnabled()) << "the state was left";
            EXPECT_FALSE(twin.widget()->underMouse());
        }

        // ----------------------------------------------------------------------------------
        // The rest of the harness
        // ----------------------------------------------------------------------------------

        TEST_F(SheetTwinsTests, StandIn_AnswersTheClassNameItIsGivenAndNoOther)
        {
            const StandIn standIn("Ns::SomeWindow", nullptr);
            EXPECT_TRUE(standIn.inherits("Ns::SomeWindow"));
            EXPECT_TRUE(standIn.inherits("QWidget"));
            EXPECT_FALSE(standIn.inherits("Ns::OtherWindow"));
        }

        TEST_F(SheetTwinsTests, Properties_AnIconIsReadByItsPixels)
        {
            QPixmap red(16, 16);
            red.fill(Qt::red);
            QPixmap blue(16, 16);
            blue.fill(Qt::blue);

            EXPECT_EQ(variantText(QVariant::fromValue(QIcon(red))), variantText(QVariant::fromValue(QIcon(red))));
            EXPECT_NE(variantText(QVariant::fromValue(QIcon(red))), variantText(QVariant::fromValue(QIcon(blue))));
            EXPECT_EQ(variantText(QVariant()), "(invalid)");
            EXPECT_NE(variantText(QVariant(QSize(20, 11))), variantText(QVariant(QSize(22, 11))));
        }

        int g_registrations = 0;
        bool g_registeredAtFlat = false;

        void recordRegistration()
        {
            ++g_registrations;
            g_registeredAtFlat = StyleManager::stylesheetsDisabled();
        }

        TEST_F(SheetTwinsTests, Register_TheRegistrarOfARowRunsOncePerProcessAtTheFlatRung)
        {
            // The registrars of the Global kind return at once outside the flat rung (ControlElements::registerGlobal), and the
            // process latches the registrar: a registrar that ran at the rung of the caller would be lost. The caller is at Qss.
            {
                RungScope qss(Rung::Qss);
                Context::registerOnce(&recordRegistration);
                EXPECT_EQ(GuiPerfFloor::rung(), Rung::Qss) << "the rung of the caller is put back";
            }
            Context::registerOnce(&recordRegistration);
            Context::registerOnce(nullptr);
            EXPECT_EQ(g_registrations, 1);
            EXPECT_TRUE(g_registeredAtFlat);
        }

        // ----------------------------------------------------------------------------------
        // The application a twin lives in
        // ----------------------------------------------------------------------------------

        //! A window in the tree of the twin (it has a parent), which takes the palette of the application and not that of its parent
        Row windowRow()
        {
            Row row;
            row.m_site = "a window below the root";
            row.m_make = [](QWidget* parent) -> QWidget* { return new QWidget(parent, Qt::Window); };
            row.m_compare = Compare::Geometry;
            row.m_size = QSize(40, 20);
            return row;
        }

        TEST_F(SheetTwinsTests, Application_AWindowBelowTheRootHasThePaletteOfTheFlattenedGuiInBothTwins)
        {
            const QPalette process = QApplication::palette();
            QPalette flat = process;
            {
                RungScope rung(Rung::Flat);
                m_context.flatStyle()->polish(flat);
            }
            const QPalette::ColorGroup groups[] = { QPalette::Active, QPalette::Inactive, QPalette::Disabled };
            const QPalette::ColorRole roles[] = {
                QPalette::Window, QPalette::Base, QPalette::Button, QPalette::Text, QPalette::WindowText
            };
            ASSERT_NE(flat.color(QPalette::Active, QPalette::Window), process.color(QPalette::Active, QPalette::Window))
                << "the palettes differ, or the test proves nothing";

            const auto expectPalette = [&](const QWidget& widget, const QPalette& expected, const char* what)
            {
                for (const QPalette::ColorGroup group : groups)
                {
                    for (const QPalette::ColorRole role : roles)
                    {
                        EXPECT_EQ(widget.palette().color(group, role), expected.color(group, role))
                            << what << ", group " << group << ", role " << role;
                    }
                }
            };

            {
                RungScope rung(Rung::Flat);
                const Twin twin(windowRow(), m_context, false);
                ASSERT_TRUE(twin.widget()->isWindow());
                expectPalette(*twin.widget(), flat, "the window of the flat twin");
                expectPalette(*twin.widget()->parentWidget(), flat, "the root of the flat twin");
            }
            expectPalette(QWidget(), process, "a widget after the flat twin: the process has the palette it had");

            // The legacy GUI has the platform's palette, which is the machine's theme: its twin is held to the palette of the flat one
            {
                RungScope rung(Rung::Qss);
                const Twin twin(windowRow(), m_context, true);
                ASSERT_TRUE(twin.widget()->isWindow());
                expectPalette(*twin.widget(), flat, "the window of the legacy twin");
            }
            expectPalette(QWidget(), process, "a widget after the legacy twin");
        }

        TEST_F(SheetTwinsTests, Application_TheLegacyTwinHasNoClassFontsAndTheFlatTwinHasThose)
        {
            // QMenuBar has a class font in the flattened GUI (TextElement ClassRules); Qt tells a class font from the application's
            // by identity (QFont::isCopyOf), as TextElement does
            const auto hasClassFont = [] { return !QApplication::font("QMenuBar").isCopyOf(QApplication::font()); };
            Row row = windowRow();
            row.m_make = [](QWidget* parent) -> QWidget* { return new QMenuBar(parent); };

            EXPECT_TRUE(hasClassFont()) << "between twins the process has the class fonts of the flattened GUI (Context)";
            {
                RungScope rung(Rung::Qss);
                const Twin twin(row, m_context, true);
                EXPECT_FALSE(hasClassFont());
                EXPECT_TRUE(QApplication::font(twin.widget()).isCopyOf(QApplication::font()));
            }
            EXPECT_TRUE(hasClassFont()) << "put back after the legacy twin";
            {
                RungScope rung(Rung::Flat);
                const Twin twin(row, m_context, false);
                EXPECT_TRUE(hasClassFont());
                EXPECT_FALSE(QApplication::font(twin.widget()).isCopyOf(QApplication::font()));
            }
            EXPECT_TRUE(hasClassFont());
        }

        TEST_F(SheetTwinsTests, Row_AFontTheQuotedSheetsLeaveToTheApplicationIsTheApplicationsInTheLegacyTwin)
        {
            // The sheet decides nothing about fonts, so the legacy font is the application's (Open Sans, 12px); the flat menu bar
            // has the font of its class rule (MenuBar.qss:13 $type.menubar.idle.family). With the class fonts of the flattened GUI in the
            // legacy twin as well, both would agree, and the Fonts of a row would compare the flat code with itself.
            Row row = windowRow();
            row.m_site = "a menu bar under a sheet that sets no font";
            row.m_make = [](QWidget* parent) -> QWidget* { return new QMenuBar(parent); };
            row.m_sheets = { { "a rule on the colour only", SetOn::Application, "QMenuBar { background-color: #333333; }" } };
            row.m_compare = Compare::Fonts;
            row.m_mayDecideNothing = true;
            row.m_expectedDifference = "font.request";
            EXPECT_TRUE(check(row, m_context));

            const Result result = run(row, m_context);
            EXPECT_NE(result.text().find("legacy family Open Sans"), std::string::npos) << result.text();
        }

        // ----------------------------------------------------------------------------------
        // The pixels at the device ratios
        // ----------------------------------------------------------------------------------

        //! A widget that records the device ratio the painter of its paint event paints on, as the style reads it
        //! (painter->device()->devicePixelRatio())
        class RatioProbe
            : public QWidget
        {
        public:
            std::vector<qreal> m_ratios;

        protected:
            void paintEvent(QPaintEvent*) override
            {
                QPainter painter(this);
                m_ratios.push_back(painter.device()->devicePixelRatio());
            }
        };

        TEST_F(SheetTwinsTests, Pixels_TheRenderOfAWidgetHandsTheStyleTheRatioOfTheScreenAtEveryImage)
        {
            RatioProbe probe;
            probe.resize(20, 10);
            probe.setAttribute(Qt::WA_DontShowOnScreen);
            probe.show();
            QApplication::processEvents();
            probe.m_ratios.clear();

            Reading reading;
            readPixels(reading, probe, Row{});

            // The limit the header names: through QWidget::render the paint device of the style is the widget, so the images at 1.5
            // and 2 are painted by a style that sees the ratio of the screen. Were Qt to change this, the header text is to change.
            ASSERT_EQ(reading.m_pictures.size(), 3u);
            ASSERT_EQ(probe.m_ratios.size(), 3u);
            for (size_t i = 0; i < probe.m_ratios.size(); ++i)
            {
                EXPECT_EQ(reading.m_pictures[i].m_image.devicePixelRatio(), ToolTipTwins::Ratios[i]);
                EXPECT_EQ(probe.m_ratios[i], probe.devicePixelRatio()) << "image " << i;
            }
        }

        //! A site whose look reads the ratio of the painter's device: a line edit's panel drawn through its style
        Row paintedRow()
        {
            Row row = pathEditRow();
            row.m_site = "a field drawn through its style";
            row.m_states.clear();
            row.m_compare = Compare::Pixels;
            return row;
        }

        TEST_F(SheetTwinsTests, Pixels_APaintFunctionGetsAPainterOnAnImageOfEachRatioInBothTwins)
        {
            std::vector<qreal> seen;
            Row row = paintedRow();
            row.m_paint = [&seen](QPainter& painter, QWidget& widget)
            {
                seen.push_back(painter.device()->devicePixelRatio());
                QStyleOptionFrame option;
                option.initFrom(&widget);
                option.lineWidth = 1;
                widget.style()->drawPrimitive(QStyle::PE_PanelLineEdit, &option, &painter, &widget);
            };

            // Idle, the three ratios in the order of the images, for the legacy twin, the legacy twin without the sheets (the check
            // that the sheets decide something) and the flat twin
            const Result result = run(row, m_context);
            ASSERT_EQ(seen.size(), 9u) << result.text();
            for (size_t i = 0; i < seen.size(); ++i)
            {
                EXPECT_EQ(seen[i], ToolTipTwins::Ratios[i % 3]) << "call " << i;
            }

            // The pictures are the ones the harness compares, and the legacy twin drew on them
            const std::vector<Reading> legacy = readLegacy(row, m_context);
            ASSERT_EQ(legacy.size(), 1u);
            ASSERT_EQ(legacy[0].m_pictures.size(), 6u);
            for (size_t i = 0; i < 3; ++i)
            {
                const Picture& painted = legacy[0].m_pictures[3 + i];
                EXPECT_EQ(painted.m_name, std::string("painted at ratio ") + QString::number(ToolTipTwins::Ratios[i]).toStdString());
                EXPECT_EQ(painted.m_image.devicePixelRatio(), ToolTipTwins::Ratios[i]);
                EXPECT_TRUE(FieldTwins::isDrawnOn(painted.m_image)) << "ratio " << ToolTipTwins::Ratios[i];
            }
        }

        // ----------------------------------------------------------------------------------
        // What a row is judged by
        // ----------------------------------------------------------------------------------

        TEST_F(SheetTwinsTests, Check_ADifferenceIsNamedByItsFactOrPictureExactly)
        {
            const std::string atOnePointFive = "a site [idle] pixels at ratio 1.5: 12 pixels differ";
            EXPECT_TRUE(namesDifference(atOnePointFive, "pixels at ratio 1.5"));
            EXPECT_FALSE(namesDifference(atOnePointFive, "pixels at ratio 1"));
            EXPECT_FALSE(namesDifference(atOnePointFive, "pixels at ratio"));
            EXPECT_FALSE(namesDifference(atOnePointFive, "ratio 1.5"));
            EXPECT_TRUE(namesDifference("a site [hover] geometry.sizeHint: legacy 1 x 2, flat 3 x 4", "geometry.sizeHint"));
            EXPECT_FALSE(namesDifference("a site [hover] geometry.sizeHint: legacy 1 x 2, flat 3 x 4", "geometry.size"));
        }

        TEST_F(SheetTwinsTests, Check_ARowExpectedToFailWithAPictureOfAnotherRatioFails)
        {
            // The broken row differs at 1, 1.5 and 2: a ratio it does not have is not found, and "ratio 1" is not "ratio 1.5"
            Row row = brokenRow();
            row.m_expectedDifference = "pixels at ratio 1.5";
            EXPECT_TRUE(check(row, m_context));

            row.m_expectedDifference = "pixels at ratio 3";
            EXPECT_FALSE(check(row, m_context));
        }

        TEST_F(SheetTwinsTests, Check_AFlawIsNotExcusedByTheDifferenceTheRowExpects)
        {
            // The sheet is set on a link of a chain that has none: the legacy twin never had it
            Row row = brokenRow();
            row.m_sheets.push_back({ "a sheet on a link that is not there", SetOn::Ancestor, "QLineEdit { color: red; }", 2 });
            EXPECT_FALSE(check(row, m_context));

            const Result result = run(row, m_context);
            EXPECT_NE(result.text().find("is set on the link 2 of a chain of 0"), std::string::npos) << result.text();
            EXPECT_FALSE(result.m_flaws.empty());
            EXPECT_FALSE(result.m_differences.empty()) << "the difference is there, and does not excuse the flaw";
        }

        TEST_F(SheetTwinsTests, Check_AFailureNamesTheSheetsOfTheRow)
        {
            Row row = chipRow();
            row.m_expectedDifference = "geometry.size";         // the chip agrees: the row was to fail
            const ::testing::AssertionResult found = check(row, m_context);
            ASSERT_FALSE(found);
            const std::string message = found.message();
            EXPECT_NE(message.find("FilteredSearchWidget.qss:68-77 on the application"), std::string::npos) << message;
            EXPECT_NE(message.find("Text.qss:89-95 on the application"), std::string::npos) << message;
        }

        TEST_F(SheetTwinsTests, Run_AWidgetOutsideTheTreeOfTheTwinIsAFlaw)
        {
            // A window made without a parent is in the tree of neither the sheets nor the style
            std::unique_ptr<QWidget> outside;
            Row row = paintedRow();
            row.m_make = [&outside](QWidget*) -> QWidget*
            {
                outside = std::make_unique<QLineEdit>();
                return outside.get();
            };

            const Result result = run(row, m_context);
            EXPECT_NE(result.text().find("the widget is not below the root of the twin"), std::string::npos) << result.text();
        }

        TEST_F(SheetTwinsTests, Run_ASheetThatDecidesNothingTheRowComparesIsAFlaw)
        {
            // A selector that reaches nothing: the legacy twin looks the same without the sheet, and would pass for a flat twin that is
            // the base style
            Row row;
            row.m_site = "a button under a sheet for check boxes";
            row.m_make = [](QWidget* parent) -> QWidget* { return new QPushButton(QStringLiteral("Button"), parent); };
            row.m_sheets = { { "a rule that reaches no button", SetOn::Application, "QCheckBox { color: red; background-color: blue; }" } };
            row.m_compare = Compare::Geometry | Compare::Pixels;
            row.m_size = QSize(80, 24);

            const Result result = run(row, m_context);
            EXPECT_NE(result.text().find("the sheets of the row decide nothing that the row compares"), std::string::npos) << result.text();

            row.m_mayDecideNothing = true;
            EXPECT_EQ(run(row, m_context).text().find("decide nothing"), std::string::npos);
        }

        TEST_F(SheetTwinsTests, Run_ARowWhoseLegacyTwinShowsNoTipIsADifference)
        {
            // The header has no tooltip text: neither twin shows a tip, the facts agree, and nothing was compared
            Row row = cardHeaderTipRow();
            row.m_make = [](QWidget* parent) -> QWidget*
            {
                auto* card = new Card(parent);
                card->setTitle(QStringLiteral("A card"));
                return card->header();
            };

            const Result result = run(row, m_context);
            EXPECT_NE(result.text().find("tip.shown: the legacy twin showed no tip"), std::string::npos) << result.text();

            row.m_mayDrawNothing = true;
            EXPECT_EQ(run(row, m_context).text().find("tip.shown"), std::string::npos);
        }
    } // namespace SheetTwinsTestsInternal
} // namespace AzQtComponents
