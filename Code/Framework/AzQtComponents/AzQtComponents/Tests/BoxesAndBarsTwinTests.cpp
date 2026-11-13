/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Tests/SheetTwins.h>

#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/Widgets/ScrollBar.h>

#include <QAbstractScrollArea>
#include <QGroupBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QScrollBar>
#include <QSlider>
#include <QVBoxLayout>

#include <functional>

namespace AzQtComponents
{
    // ----------------------------------------------------------------------------------
    // The boxes and the bars that the universal rule of the application's sheet (BaseStyleSheet.qss:15-21) and the sheets of the
    // controls decided for every widget of the kind, with no rule of a module of their own, each twinned against the legacy GUI:
    //   - A15: a group box. The universal rule gives it a background, a palette and a font, so the sheet style drew it through the
    //     sheet (QStyleSheetStyle::drawComplexControl, CC_GroupBox) however plain it was: the frame is kept out of the rect of the
    //     title, which is centered in it, and the check box is drawn over the clear palette;
    //   - A16: the scroll bars of a text edit with the DarkScrollBar class, in the colour the Editor's console gives its area
    //     (ScrollBar.qss:10-47, :76-88; ScrollBar::setBackgroundColor): the handle lies inside the border of the lane, which is
    //     thinner under the pointer, and a lane keeps the colour of the area under the pointer; the corner tile between the bars
    //     is filled while the pointer is on it, or in the colour of the area (ScrollBar.qss:90-100);
    //   - A17: a slider that no component draws (a plain QSlider): Fusion draws it from the Button and Window of its palette, which
    //     the universal rule made clear;
    //   - A19: a progress bar in a layout that has more room than the bar needs.
    // The legacy twin is Qt's own style sheet style with the application's real sheet (FieldTwins::appSheet) on the root, over the
    // Style (its metrics are the legacy GUI's); the flat twin is the flattened Style on every widget of the same tree. The Global
    // elements are registered by the Context.
    // ----------------------------------------------------------------------------------

    namespace BoxesAndBarsTwinTestsInternal
    {
        using namespace SheetTwins;

        class BoxesAndBarsTwinTests
            : public ::UnitTest::LeakDetectionFixture
        {
        protected:
            Context m_context;
        };

        // ----------------------------------------------------------------------------------
        // A15: group boxes
        // ----------------------------------------------------------------------------------

        struct GroupBoxShape
        {
            const char* m_title = "";
            bool m_checkable = false;
            bool m_checked = true;
            bool m_flat = false;
            Qt::Alignment m_alignment = Qt::AlignLeft;
        };

        //! A group box with a label in it, as the dialogs of the Editor have them (TVSequenceProps.ui, NewLevelDialog.ui): under the
        //! rules of the application's sheet alone
        Row plainGroupBoxRow(const char* site, const GroupBoxShape& shape)
        {
            return Row{
                .m_site = site,
                .m_make = [shape](QWidget* parent) -> QWidget*
                {
                    auto* box = new QGroupBox(QString::fromUtf8(shape.m_title), parent);
                    box->setCheckable(shape.m_checkable);
                    box->setChecked(shape.m_checked);
                    box->setFlat(shape.m_flat);
                    box->setAlignment(shape.m_alignment);
                    auto* layout = new QVBoxLayout(box);
                    layout->addWidget(new QLabel(QStringLiteral("Contents"), box));
                    return box;
                },
                .m_realApplicationSheet = true,
                .m_states = { States::hover(), States::disabled() },
                .m_compare = Compare::Geometry | Compare::Fonts | Compare::Palette | Compare::Pixels,
                .m_roles = { QPalette::WindowText, QPalette::Text, QPalette::ButtonText },
                .m_size = QSize(170, 80),
                .m_base = Base::AzQtComponentsStyle,
                // a group box with no title is the frame of the style whichever way it is drawn
                .m_mayDecideNothing = shape.m_title[0] == 0 && !shape.m_checkable,
            };
        }

        TEST_F(BoxesAndBarsTwinTests, GroupBox_APlainBoxWithATitleIsDrawnThroughTheSheet)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(plainGroupBoxRow("A15 a group box with a title", { .m_title = "Settings" }), m_context));
            EXPECT_TRUE(check(
                plainGroupBoxRow("A15 a group box with a title, centered", { .m_title = "Settings", .m_alignment = Qt::AlignHCenter }),
                m_context));
            EXPECT_TRUE(check(
                plainGroupBoxRow("A15 a group box with a title, at the right", { .m_title = "Settings", .m_alignment = Qt::AlignRight }),
                m_context));
        }

        TEST_F(BoxesAndBarsTwinTests, GroupBox_APlainBoxWithNoTitleIsTheFrameOfTheStyle)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(plainGroupBoxRow("A15 a group box with no title", {}), m_context));
        }

        TEST_F(BoxesAndBarsTwinTests, GroupBox_ACheckableBoxDrawsItsCheckBoxOverTheClearPalette)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(plainGroupBoxRow("A15 a checked group box", { .m_title = "Enabled", .m_checkable = true }), m_context));
            EXPECT_TRUE(check(
                plainGroupBoxRow("A15 an unchecked group box", { .m_title = "Enabled", .m_checkable = true, .m_checked = false }),
                m_context));
            EXPECT_TRUE(check(plainGroupBoxRow("A15 a group box that is only a check box", { .m_checkable = true }), m_context));
        }

        TEST_F(BoxesAndBarsTwinTests, GroupBox_AFlatBoxIsDrawnThroughTheSheetToo)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(
                plainGroupBoxRow("A15 a flat group box with a title", { .m_title = "Settings", .m_flat = true }), m_context));
            EXPECT_TRUE(check(
                plainGroupBoxRow("A15 a flat checkable group box", { .m_title = "Settings", .m_checkable = true, .m_flat = true }),
                m_context));
        }

        // ----------------------------------------------------------------------------------
        // A16: the scroll bars of a text edit with the dark class
        // ----------------------------------------------------------------------------------

        //! The text area of the Editor's console (ConsoleSCB.ui), read only, with more lines than it shows and wider than it (the lines
        //! are spaces: the bars are what the test is about). `darkBars` is ScrollBar::applyDarkStyle, which the console calls for its
        //! light background; a colour is ScrollBar::setBackgroundColor (ConsoleSCB.cpp:535), which the legacy GUI gave as the inline
        //! sheet `background: <colour>` that the row quotes (`sheet`, the same colour; none: the area has no colour of its own). A tile
        //! with the property the corner widget of a scroll area has (ShowBackground) stands for that widget, whose property the watcher
        //! of ScrollBar.cpp toggles when the pointer enters a bar: the watcher needs the StyleManager, which the tests do not have.
        Row consoleRow(const char* site, bool darkBars, const QColor& color, const char* sheet)
        {
            Row row{
                .m_site = site,
                .m_make = [darkBars, color](QWidget* parent) -> QWidget*
                {
                    auto* console = new QPlainTextEdit(parent);
                    console->setObjectName(QStringLiteral("textEdit"));
                    console->setReadOnly(true);
                    console->setLineWrapMode(QPlainTextEdit::NoWrap);
                    console->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
                    console->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
                    for (int line = 0; line < 60; ++line)
                    {
                        console->appendPlainText(QString(90, QLatin1Char(' ')));
                    }
                    if (darkBars)
                    {
                        Style::addClass(console, QStringLiteral("DarkScrollBar"));
                    }
                    if (color.isValid() && StyleManager::stylesheetsDisabled())
                    {
                        ScrollBar::setBackgroundColor(console, color);
                    }
                    auto* tile = new QWidget(console);
                    tile->setObjectName(QStringLiteral("cornerTile"));
                    tile->setProperty("ShowBackground", false);
                    tile->setGeometry(100, 20, 8, 8);
                    return console;
                },
                .m_realApplicationSheet = true,
                .m_size = QSize(240, 120),
                .m_base = Base::AzQtComponentsStyle,
            };
            if (sheet[0] != 0)
            {
                row.m_sheets.push_back({ "ScrollBar.cpp:246", SetOn::Widget, sheet });
            }
            return row;
        }

        //! A state of the console: both bars scrolled to a part of their range, and one of them under the pointer, with the tile shown
        //! as the watcher shows the corner
        struct ConsoleState
        {
            const char* m_name;
            double m_fraction;
            int m_hovered;                  //!< 0: the vertical bar, 1: the horizontal bar, -1: none
            bool m_tileHovered = false;     //!< the pointer is on the corner tile
        };

        const ConsoleState ConsoleStates[] = {
            { "idle", 0.0, -1 },
            { "scrolled to the middle", 0.5, -1 },
            { "scrolled to the end", 1.0, -1 },
            { "vertical bar under the pointer, at the start", 0.0, 0 },
            { "vertical bar under the pointer, in the middle", 0.5, 0 },
            { "vertical bar under the pointer, at the end", 1.0, 0 },
            { "horizontal bar under the pointer, at the start", 0.0, 1 },
            { "horizontal bar under the pointer, in the middle", 0.5, 1 },
            { "horizontal bar under the pointer, at the end", 1.0, 1 },
            { "corner tile under the pointer, at the start", 0.0, -1, true },
            { "corner tile under the pointer, in the middle", 0.5, -1, true },
        };

        QScrollBar* barOf(QAbstractScrollArea& area, int index)
        {
            return index == 0 ? area.verticalScrollBar() : area.horizontalScrollBar();
        }

        void setConsoleState(QAbstractScrollArea& area, const ConsoleState& state, bool entered)
        {
            QWidget* tile = area.findChild<QWidget*>(QStringLiteral("cornerTile"));
            for (int index = 0; index < 2; ++index)
            {
                QScrollBar* bar = barOf(area, index);
                bar->setValue(bar->minimum() + static_cast<int>((bar->maximum() - bar->minimum()) * (entered ? state.m_fraction : 0.0)));
            }
            if (state.m_hovered >= 0)
            {
                // The pointer on a bar: the attribute the option's State_MouseOver comes from. (A hover event would reach the watcher
                // of ScrollBar.cpp, which needs the StyleManager.)
                barOf(area, state.m_hovered)->setAttribute(Qt::WA_UnderMouse, entered);
                tile->setProperty("ShowBackground", entered);
                States::repolish(*tile);
            }
            if (state.m_tileHovered)
            {
                // The pointer on the tile: the attribute the option's State_MouseOver of the tile comes from
                tile->setAttribute(Qt::WA_UnderMouse, entered);
            }
        }

        //! The scroll bar as its widget gives it to its style (QScrollBar::initStyleOption, paintEvent)
        QStyleOptionSlider sliderOptionOf(const QScrollBar* bar)
        {
            QStyleOptionSlider option;
            option.initFrom(bar);
            option.subControls = QStyle::SC_All;
            option.orientation = bar->orientation();
            option.minimum = bar->minimum();
            option.maximum = bar->maximum();
            option.sliderPosition = bar->sliderPosition();
            option.sliderValue = bar->value();
            option.singleStep = bar->singleStep();
            option.pageStep = bar->pageStep();
            option.upsideDown = bar->invertedAppearance();
            if (bar->orientation() == Qt::Horizontal)
            {
                option.state |= QStyle::State_Horizontal;
            }
            return option;
        }

        struct BarFacts
        {
            QRect m_bar;                    //!< in the coordinates of the window
            QRect m_parts[4];               //!< the groove, the handle, the page before it and the page after it, in the bar's
            QImage m_images[3];             //!< the bar drawn through its style over grey, at the device ratios
        };

        struct ConsoleFacts
        {
            std::string m_state;
            BarFacts m_bars[2];
            QImage m_tileImages[3];         //!< the tile rendered over grey
            std::string m_tileSetup;        //!< how the tile is set up, for the report of a difference
        };

        ConsoleFacts readConsole(QAbstractScrollArea& area, const ConsoleState& state)
        {
            ConsoleFacts facts;
            facts.m_state = state.m_name;
            for (int index = 0; index < 2; ++index)
            {
                const QScrollBar* bar = barOf(area, index);
                BarFacts& read = facts.m_bars[index];
                read.m_bar = QRect(bar->mapTo(bar->window(), QPoint(0, 0)), bar->size());
                const QStyleOptionSlider option = sliderOptionOf(bar);
                int part = 0;
                for (const QStyle::SubControl subControl : { QStyle::SC_ScrollBarGroove, QStyle::SC_ScrollBarSlider,
                         QStyle::SC_ScrollBarSubPage, QStyle::SC_ScrollBarAddPage })
                {
                    read.m_parts[part++] = bar->style()->subControlRect(QStyle::CC_ScrollBar, &option, subControl, bar);
                }
                for (int i = 0; i < 3; ++i)
                {
                    read.m_images[i] = FieldTwins::paintThroughStyle(bar, ToolTipTwins::Ratios[i],
                        [&](QPainter& painter) { bar->style()->drawComplexControl(QStyle::CC_ScrollBar, &option, &painter, bar); });
                }
            }

            QWidget* tile = area.findChild<QWidget*>(QStringLiteral("cornerTile"));
            facts.m_tileSetup = std::string("styled background ") + (tile->testAttribute(Qt::WA_StyledBackground) ? "yes" : "no")
                + ", autofill " + (tile->autoFillBackground() ? "yes" : "no") + ", window "
                + colorText(tile->palette().color(QPalette::Window)) + ", style " + tile->style()->metaObject()->className()
                + ", ShowBackground " + tile->property("ShowBackground").toString().toStdString() + ", area class '"
                + area.property("class").toString().toStdString() + "', visible " + (tile->isVisible() ? "yes" : "no")
                + ", tile class '" + tile->property("class").toString().toStdString() + "', tile parent "
                + tile->parentWidget()->metaObject()->className();
            for (int i = 0; i < 3; ++i)
            {
                QImage& image = facts.m_tileImages[i];
                image = QImage((QSizeF(tile->size()) * ToolTipTwins::Ratios[i]).toSize(), QImage::Format_ARGB32_Premultiplied);
                image.setDevicePixelRatio(ToolTipTwins::Ratios[i]);
                image.fill(QColor(0x80, 0x80, 0x80));
                tile->render(&image, QPoint(), QRegion(), QWidget::DrawChildren);
            }
            return facts;
        }

        //! Gives a console that has been drawn another colour, as RefreshStyle of ConsoleSCB.cpp does (the legacy GUI: the inline sheet
        //! of ScrollBar.cpp:246 again; the flattened: ScrollBar::setBackgroundColor again)
        using Recolor = std::function<void(QAbstractScrollArea& area, bool legacy)>;

        //! The console built as a twin of the legacy GUI or of the flattened one, and read in every state (after the recolouring, if any)
        std::vector<ConsoleFacts> readConsoleTwin(
            const Row& row, const Context& context, bool legacy, std::vector<std::string>& flaws, const Recolor& recolor)
        {
            RungScope rung(legacy ? row.m_legacyRung : Rung::Flat);
            const Twin twin(row, context, legacy);
            for (const std::string& flaw : twin.flaws(legacy))
            {
                flaws.push_back(flaw);
            }

            auto* area = qobject_cast<QAbstractScrollArea*>(twin.widget());
            if (recolor)
            {
                // drawn once with the colour it was built with
                readConsole(*area, ConsoleStates[0]);
                recolor(*area, legacy);
                QApplication::processEvents();
            }
            std::vector<ConsoleFacts> facts;
            for (const ConsoleState& state : ConsoleStates)
            {
                setConsoleState(*area, state, true);
                QApplication::processEvents();
                facts.push_back(readConsole(*area, state));
                setConsoleState(*area, state, false);
                QApplication::processEvents();
            }
            return facts;
        }

        //! The two twins of a console compared in every state: where the bars lie, the parts of each, and what each draws
        void expectTheSameBars(const Row& row, const Context& context, const Recolor& recolor = {})
        {
            std::vector<std::string> flaws;
            const std::vector<ConsoleFacts> legacy = readConsoleTwin(row, context, true, flaws, recolor);
            const std::vector<ConsoleFacts> flat = readConsoleTwin(row, context, false, flaws, recolor);
            for (const std::string& flaw : flaws)
            {
                ADD_FAILURE() << row.m_site << ": " << flaw;
            }
            ASSERT_EQ(legacy.size(), flat.size());

            // The tile is filled by the inline sheet of a console with a colour, in every state. Without one it is left to the rules of
            // ScrollBar.qss:90-100, whose second selector (`[ShowBackground=true]`) the sheet style does not apply after the `:hover`
            // selector in the same list: the legacy tile is filled while the pointer is on it, and not while the watcher shows it.
            const char* partNames[] = { "groove", "handle", "page before the handle", "page after the handle" };
            const char* barNames[] = { "vertical bar", "horizontal bar" };
            for (size_t i = 0; i < legacy.size(); ++i)
            {
                const std::string where = std::string(row.m_site) + " [" + legacy[i].m_state + "] ";
                for (int index = 0; index < 2; ++index)
                {
                    const BarFacts& l = legacy[i].m_bars[index];
                    const BarFacts& f = flat[i].m_bars[index];
                    EXPECT_EQ(FieldTwins::rectText(f.m_bar), FieldTwins::rectText(l.m_bar)) << where << barNames[index] << " rect";
                    for (int part = 0; part < 4; ++part)
                    {
                        EXPECT_EQ(FieldTwins::rectText(f.m_parts[part]), FieldTwins::rectText(l.m_parts[part]))
                            << where << barNames[index] << " " << partNames[part];
                    }
                    for (int image = 0; image < 3; ++image)
                    {
                        EXPECT_EQ(FieldTwins::differing(l.m_images[image], f.m_images[image]), 0)
                            << where << barNames[index] << " drawn at ratio " << ToolTipTwins::Ratios[image] << ": "
                            << FieldTwins::firstDifference(l.m_images[image], f.m_images[image]);
                    }
                }
                for (int image = 0; image < 3; ++image)
                {
                    EXPECT_EQ(FieldTwins::differing(legacy[i].m_tileImages[image], flat[i].m_tileImages[image]), 0)
                        << where << "corner tile at ratio " << ToolTipTwins::Ratios[image] << ": "
                        << FieldTwins::firstDifference(legacy[i].m_tileImages[image], flat[i].m_tileImages[image])
                        << "; legacy tile: " << legacy[i].m_tileSetup << "; flat tile: " << flat[i].m_tileSetup;
                }

                // The legacy tile itself, so that the comparison above is not between two empty tiles: filled while the pointer is on
                // it; not filled while only the watcher's property says a bar is hovered, when the area has no colour of its own
                const QColor backdrop(0x80, 0x80, 0x80);
                const QColor legacyTile = legacy[i].m_tileImages[0].pixelColor(4, 4);
                const ConsoleState& consoleState = ConsoleStates[i];
                if (consoleState.m_tileHovered)
                {
                    EXPECT_NE(legacyTile, backdrop) << where << "the legacy tile under the pointer is filled";
                }
                if (consoleState.m_hovered >= 0 && row.m_sheets.empty())
                {
                    EXPECT_EQ(legacyTile, backdrop) << where << "the legacy tile of a bar under the pointer is not filled";
                }
            }
        }

        TEST_F(BoxesAndBarsTwinTests, DarkScrollBar_TheBarsOfALightConsoleAreTheDarkOnes)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            // ConsoleSCB.cpp:529 (the light theme): the dark bars over white
            expectTheSameBars(
                consoleRow("A16 dark bars on a white console", true, QColor(Qt::white), "background: #ffffff"), m_context);
        }

        TEST_F(BoxesAndBarsTwinTests, DarkScrollBar_TheBarsOfADarkConsoleAreTheLightOnes)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            // ConsoleSCB.cpp:523 (the dark theme): the light bars over #222222
            expectTheSameBars(
                consoleRow("A16 light bars on a dark console", false, QColor(0x22, 0x22, 0x22), "background: #222222"), m_context);
        }

        TEST_F(BoxesAndBarsTwinTests, DarkScrollBar_ADarkTextAreaWithNoColourOfItsOwnHasTheHoverFillOfTheDarkBars)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            // The Gallery's ScrollBarPage (ScrollBarPage.cpp:29): the dark class on an area that has no inline sheet
            expectTheSameBars(consoleRow("A16 dark bars on a text area", true, QColor(), ""), m_context);
        }

        TEST_F(BoxesAndBarsTwinTests, DarkScrollBar_ALightTextAreaWithNoColourOfItsOwnHasTheHoverFillOfTheLightBars)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            expectTheSameBars(consoleRow("A16 light bars on a text area", false, QColor(), ""), m_context);
        }

        TEST_F(BoxesAndBarsTwinTests, DarkScrollBar_AColourGivenToAConsoleThatHasBeenDrawnReachesTheTile)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            // ConsoleSCB.cpp:535 through RefreshStyle (OnEditorPreferencesChanged): a console drawn in the light theme is given the
            // dark theme's colour
            expectTheSameBars(consoleRow("A16 a console that changes its colour", true, QColor(Qt::white), "background: #ffffff"),
                m_context,
                [](QAbstractScrollArea& area, bool legacy)
                {
                    if (legacy)
                    {
                        area.setStyleSheet(QStringLiteral("background: #222222"));      // ScrollBar.cpp:246
                    }
                    else
                    {
                        ScrollBar::setBackgroundColor(&area, QColor(0x22, 0x22, 0x22));
                    }
                });
        }

        // ----------------------------------------------------------------------------------
        // A17: sliders no component draws
        // ----------------------------------------------------------------------------------

        Row plainSliderRow(const char* site, Qt::Orientation orientation, QSlider::TickPosition ticks, const QSize& size)
        {
            return Row{
                .m_site = site,
                .m_make = [orientation, ticks](QWidget* parent) -> QWidget*
                {
                    auto* slider = new QSlider(orientation, parent);
                    slider->setRange(0, 100);
                    slider->setValue(40);
                    slider->setTickPosition(ticks);
                    return slider;
                },
                .m_realApplicationSheet = true,
                .m_states = { States::hover(), States::focus(), States::disabled() },
                .m_compare = Compare::Geometry | Compare::Palette | Compare::Pixels,
                .m_roles = { QPalette::Window, QPalette::Base, QPalette::Button, QPalette::WindowText, QPalette::Text,
                    QPalette::ButtonText },
                .m_size = size,
                .m_base = Base::AzQtComponentsStyle,
            };
        }

        TEST_F(BoxesAndBarsTwinTests, Slider_APlainSliderIsDrawnOverTheClearPaletteOfTheSheet)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(
                plainSliderRow("A17 a plain horizontal slider", Qt::Horizontal, QSlider::NoTicks, QSize(160, 24)), m_context));
            EXPECT_TRUE(check(
                plainSliderRow("A17 a plain vertical slider", Qt::Vertical, QSlider::NoTicks, QSize(24, 100)), m_context));
            EXPECT_TRUE(check(
                plainSliderRow("A17 a plain horizontal slider with ticks", Qt::Horizontal, QSlider::TicksBelow, QSize(160, 34)),
                m_context));
        }

        // ----------------------------------------------------------------------------------
        // A19: a progress bar in a layout with room to spare
        // ----------------------------------------------------------------------------------

        //! A bar as the dialogs of the tree make it (SequenceBatchRenderDialog.ui, ProgressShield.ui, GemCatalogHeaderWidget.cpp:262: the
        //! policy of its class, no minimum height) in a layout much taller than it
        Row progressBarInLayoutRow(const char* site, bool textVisible)
        {
            return Row{
                .m_site = site,
                .m_make = [textVisible](QWidget* parent) -> QWidget*
                {
                    auto* page = new QWidget(parent);
                    auto* layout = new QVBoxLayout(page);
                    layout->setContentsMargins(0, 0, 0, 0);
                    auto* bar = new QProgressBar(page);
                    bar->setRange(0, 100);
                    bar->setValue(40);
                    bar->setTextVisible(textVisible);
                    layout->addWidget(bar);
                    return page;
                },
                .m_realApplicationSheet = true,
                .m_compare = Compare::Geometry | Compare::Pixels,
                .m_size = QSize(160, 60),
                .m_base = Base::AzQtComponentsStyle,
            };
        }

        TEST_F(BoxesAndBarsTwinTests, ProgressBar_ABarOfTheTreeIsTheHeightOfItsHintWhateverRoomTheLayoutHas)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(progressBarInLayoutRow("A19 a bar with its text hidden by the sheet", true), m_context));
            EXPECT_TRUE(check(progressBarInLayoutRow("A19 a bar with its text hidden by the code", false), m_context));
        }
    } // namespace BoxesAndBarsTwinTestsInternal
} // namespace AzQtComponents
