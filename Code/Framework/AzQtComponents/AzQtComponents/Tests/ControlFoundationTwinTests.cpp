/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Tests/SheetTwins.h>

#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/ElementCommon.h>
#include <AzQtComponents/Components/Elements/Foundations.h>
#include <AzQtComponents/Components/Elements/IndicatorElement.h>
#include <AzQtComponents/Components/Elements/MenuElement.h>
#include <AzQtComponents/Components/Elements/SplitterElement.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>

#include <QAbstractItemView>
#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QIcon>
#include <QLineEdit>
#include <QListView>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QProgressBar>
#include <QSplitter>

namespace AzQtComponents
{
    // ----------------------------------------------------------------------------------
    // The foundations of the flattened GUI that wave W2a added for the control lanes after it, each proven on a widget of the kind of
    // the sites that ask for it, against the sheet rules it stands for (Qt's own style sheet style over the sheets' real rules, the
    // application's real sheet below them; the flat twin is the flattened Style with an element):
    //   - F-lineedit-box: the box of a line edit that its own sheet changed (EMStudioManager.cpp:262 `border: 1px solid red;`,
    //     TrackHeaderWidget.cpp:237 `background-color: rgb(70, 70, 70);`), and gap A12, a line edit with no frame outside an item view,
    //     which LineEdit.qss still gave its box;
    //   - F-splitter: `QSplitter::handle { height: 4px; background: transparent; }` (DialogStack.cpp:35, :146), SplitterElement;
    //   - F-indicator-images: `QCheckBox::indicator:checked { image }` (MotionExtractionWindow.cpp:167, PropertyEditorWidget.cpp:55),
    //     IndicatorElement::m_stateImages;
    //   - F-progressbar-box: a progress bar with a padding (CommandBarPlugin.cpp:112 `padding-right: 2px;`), and the bar's height
    //     when the box is above and below the track too (the maximum height Style::polish gives a flat bar follows its hint);
    //   - F-menu-icon: `QMenu::icon { left: 0px; right: 6px; }` (ATLControlsPanel.cpp:41), MenuElement::m_iconOffsets.
    // The elements here are matched by the object name their row gives the widget, so that no other test meets them.
    // ----------------------------------------------------------------------------------

    namespace ControlFoundationTwinTestsInternal
    {
        using namespace SheetTwins;

        class ControlFoundationTwinTests
            : public ::UnitTest::LeakDetectionFixture
        {
        protected:
            Context m_context;
        };

        // ----------------------------------------------------------------------------------
        // Helpers
        // ----------------------------------------------------------------------------------

        bool isNamed(const QWidget* widget, const char* name)
        {
            return Selectors::named(widget, name);
        }

        //! A widget that fills itself with one colour: a pane of a splitter, which neither twin may leave to a sheet
        class Swatch
            : public QWidget
        {
        public:
            Swatch(const QColor& color, QWidget* parent)
                : QWidget(parent)
                , m_color(color)
            {
            }

        protected:
            void paintEvent(QPaintEvent*) override
            {
                QPainter painter(this);
                painter.fillRect(rect(), m_color);
            }

        private:
            QColor m_color;
        };

        //! A class tag laid on a widget, or taken off it, in the flattened GUI (Style::addClass and removeClass polish the widget again)
        void setTag(QWidget& widget, const char* tag, bool on)
        {
            if (on)
            {
                Style::addClass(&widget, QString::fromLatin1(tag));
            }
            else
            {
                Style::removeClass(&widget, QString::fromLatin1(tag));
            }
        }

        //! A state the site sets through its legacy sheet in the legacy GUI and through a class tag in the flattened one: while the
        //! state holds the tag is laid on (`tagOnInState`) or taken off, and the other way round as the state is left
        State sheetState(const char* name, const char* sheet, const char* tag, bool tagOnInState)
        {
            return State{ name,
                [sheet, tag, tagOnInState](QWidget& widget)
                {
                    if (StyleManager::flatStylingActive())
                    {
                        setTag(widget, tag, tagOnInState);
                    }
                    else
                    {
                        widget.setStyleSheet(QString::fromLatin1(sheet));
                    }
                },
                [tag, tagOnInState](QWidget& widget)
                {
                    if (StyleManager::flatStylingActive())
                    {
                        setTag(widget, tag, !tagOnInState);
                    }
                    else
                    {
                        widget.setStyleSheet(QString());
                    }
                } };
        }

        // ----------------------------------------------------------------------------------
        // The elements
        // ----------------------------------------------------------------------------------

        // ---- F-lineedit-box. The Global box of a line edit (BaseStyleSheet.qss:71-84, LineEdit.qss:11-20, :28-35) as the element of a
        // site whose own rule changed the border: the box (padding, height, radius), the fill, the disabled fill, and the border of
        // the site. EMStudioManager.cpp:262 `border: 1px solid red;` (LineEditValidatable.cpp:46 is the same rule) changes the width
        // and the colour of the border only, so the fill of each state stays the Global one.
        constexpr int RingWidth = 1;
        const QColor RingColor(Qt::red);
        const BoxElement InvalidInput{
            .m_name = "Test.Box.InvalidInput",
            .m_padding = QMargins(Foundations::Field::PaddingLeft, Foundations::Field::PaddingTop, 0, Foundations::Field::PaddingBottom),
            .m_hasBox = true,
            .m_nativeBorder = false,
            .m_radius = Foundations::Field::Radius,
            .m_size = QSize(BoxElement::Unset, Foundations::Field::Height),
            .m_idle = { .m_fill = QBrush(Foundations::Field::Fill), .m_edges = BoxElement::border(RingWidth, RingColor) },
            .m_stateFaces = {
                { BoxElement::Disabled, {
                    .m_fill = QBrush(Foundations::Field::DisabledFill),
                    .m_edges = BoxElement::border(RingWidth, RingColor),
                } },
            },
        };

        // TrackHeaderWidget.cpp:237 `background-color: rgb(70, 70, 70);`: the fill of every state, over the Global ring
        // (LineEdit.qss:16-17)
        const QColor TrackNameFill(70, 70, 70);
        const BoxElement DisabledTrackName{
            .m_name = "Test.Box.DisabledTrackName",
            .m_padding = QMargins(Foundations::Field::PaddingLeft, Foundations::Field::PaddingTop, 0, Foundations::Field::PaddingBottom),
            .m_hasBox = true,
            .m_nativeBorder = false,
            .m_radius = Foundations::Field::Radius,
            .m_size = QSize(BoxElement::Unset, Foundations::Field::Height),
            .m_idle = { .m_fill = QBrush(TrackNameFill), .m_edges = BoxElement::border(2, Foundations::Field::Line) },
        };

        // ---- F-splitter. DialogStack.cpp:35, :383 `QSplitter::handle{ height: 4px; background: transparent; }`, :146, :469 the same with
        // `height: 1px`
        const SplitterElement TransparentHandle4{
            .m_name = "Test.Splitter.TransparentHandle4", .m_thickness = 4, .m_fill = QColor(Qt::transparent)
        };
        const SplitterElement TransparentHandle1{
            .m_name = "Test.Splitter.TransparentHandle1", .m_thickness = 1, .m_fill = QColor(Qt::transparent)
        };
        // `QSplitter::handle { background-color: #336699; }`: a fill of its own for every state, the width of Splitter.qss:18
        const QColor HandleBlue(0x33, 0x66, 0x99);
        const SplitterElement BlueHandle{ .m_name = "Test.Splitter.BlueHandle", .m_thickness = 1, .m_fill = HandleBlue };
        // `QSplitter::handle { height: 3px; }`: a width and no background of its own, which the application's rule (Splitter.qss:9-11)
        // gives a dark fill, and its :hover rule (Splitter.qss:21-24) the blue one
        const SplitterElement ThickHandle{
            .m_name = "Test.Splitter.ThickHandle",
            .m_thickness = 3,
            .m_fill = Foundations::BG,
            .m_hoverFill = QColor(0x1E, 0x70, 0xEB),
        };

        // ---- F-indicator-images. MotionExtractionWindow.cpp:167 the box of the indicator (`width: 16px; height: 16px; border: none;
        // margin: 0px`) and the images of :checked and :unchecked, the same for :disabled and :focus
        constexpr const char* CaretDown = ":/Cards/img/UI20/Cards/caret-down.svg";
        constexpr const char* CaretRight = ":/Cards/img/UI20/Cards/caret-right.svg";
        const IndicatorElement CaretCheck{
            .m_name = "Test.Indicator.CaretCheck",
            .m_imageSize = IndicatorElement::ImageSize,
            .m_margin = 0,
            .m_focusImageSize = IndicatorElement::ImageSize,
            .m_focusMargin = 0,
            .m_stateImages = {
                { IndicatorElement::Checked, CaretDown },
                { IndicatorElement::Unchecked, CaretRight },
            },
        };

        // PropertyEditorWidget.cpp:55 `QCheckBox::indicator:unchecked{image: url(:/stylesheet/img/UI20/checkbox/off.svg);}` and
        // `:checked{image: ...on.svg}`: the images of CheckBox.qss:50-73 for every state of the two, over the box of CheckBox.qss:23-28
        constexpr const char* CheckOn = ":/stylesheet/img/UI20/checkbox/on.svg";
        constexpr const char* CheckOff = ":/stylesheet/img/UI20/checkbox/off.svg";
        const IndicatorElement RestingCheck{
            .m_name = "Test.Indicator.RestingCheck",
            .m_imageSize = IndicatorElement::ImageSize,
            .m_margin = 2,
            .m_focusImageSize = IndicatorElement::ImageSize,
            .m_focusMargin = 2,
            .m_stateImages = {
                { IndicatorElement::Unchecked, CheckOff },
                { IndicatorElement::Checked, CheckOn },
            },
        };

        // Rules in the order of a sheet, the compound before the plain rule (`:checked:disabled` then `:checked`): the compound rule has
        // the more pseudo-classes and is the more specific, whatever the order of the rules
        const IndicatorElement RankedCheck{
            .m_name = "Test.Indicator.RankedCheck",
            .m_imageSize = IndicatorElement::ImageSize,
            .m_margin = 2,
            .m_focusImageSize = IndicatorElement::ImageSize,
            .m_focusMargin = 2,
            .m_stateImages = {
                { IndicatorElement::Checked | IndicatorElement::Disabled, CaretDown },
                { IndicatorElement::Checked, CheckOn },
            },
        };

        // The same box with the images of the style only: what the site's element would be without the state images
        const IndicatorElement StyleImagesCheck{
            .m_name = "Test.Indicator.StyleImagesCheck",
            .m_imageSize = IndicatorElement::ImageSize,
            .m_margin = 2,
            .m_focusImageSize = IndicatorElement::ImageSize,
            .m_focusMargin = 2,
        };

        // A hovered and focused box that name their own images, and a compound state that names a third: the last entry whose states
        // all hold is the image (CheckBox.qss:50-73 restated with the states a rule can name)
        const IndicatorElement CompoundStatesCheck{
            .m_name = "Test.Indicator.CompoundStatesCheck",
            .m_imageSize = IndicatorElement::ImageSize,
            .m_margin = 2,
            .m_focusImageSize = IndicatorElement::ImageSize,
            .m_focusMargin = 2,
            .m_image = CheckOff,
            .m_stateImages = {
                { IndicatorElement::Checked, CheckOn },
                { IndicatorElement::Checked | IndicatorElement::Disabled, CaretDown },
                { IndicatorElement::Hover, CaretRight },
                { IndicatorElement::Indeterminate | IndicatorElement::Focus, CaretDown },
            },
        };

        // ---- F-progressbar-box. The Global bar (ProgressBar.qss:11-19) with CommandBarPlugin.cpp:112 `padding-right: 2px;`: the
        // track fill and the hidden text of ProgressBar.qss:14 and :18, a square borderless box, and the padding
        const QColor ProgressTrack(0x9A, 0x9A, 0x9A);
        const BoxElement PaddedProgress{
            .m_name = "Test.Box.PaddedProgress",
            .m_padding = QMargins(0, 0, 2, 0),
            .m_nativeBorder = false,
            .m_idle = { .m_fill = QBrush(ProgressTrack) },
            .m_properties = { { "textVisible", false } },
        };
        // The same bar with a padding above and below the track too: a box of 5px in height, which the bar is as tall as
        const BoxElement TallPaddedProgress{
            .m_name = "Test.Box.TallPaddedProgress",
            .m_padding = QMargins(0, 2, 2, 3),
            .m_nativeBorder = false,
            .m_idle = { .m_fill = QBrush(ProgressTrack) },
            .m_properties = { { "textVisible", false } },
        };
        // The same bar with a height of its own (`height: 9px`): the contents of the hint are 9px high, not the track's 5px
        const BoxElement NinePixelProgress{
            .m_name = "Test.Box.NinePixelProgress",
            .m_padding = QMargins(0, 2, 2, 3),
            .m_nativeBorder = false,
            .m_size = QSize(-1, 9),
            .m_idle = { .m_fill = QBrush(ProgressTrack) },
            .m_properties = { { "textVisible", false } },
        };
        // The same bar with a maximum height (`max-height: 3px`), below the 5px of ProgressBar.qss:15 which the element restates in its
        // size: the limit of the element is the bar's, the lesser of the two grown by the box
        const BoxElement CappedProgress{
            .m_name = "Test.Box.CappedProgress",
            .m_padding = QMargins(0, 2, 2, 3),
            .m_nativeBorder = false,
            .m_size = QSize(-1, 5),                                     // ProgressBar.qss:15
            .m_maxSize = QSize(-1, 3),
            .m_idle = { .m_fill = QBrush(ProgressTrack) },
            .m_properties = { { "textVisible", false } },
        };

        // ---- F-menu-icon. ATLControlsPanel.cpp:41 `QMenu::icon { left: 0px; right: 6px; }` over the popup of Menu.qss:10-34
        const MenuElement IconMenu{
            .m_name = "Test.Menu.IconMenu",
            .m_surface = Foundations::BG,                               // Menu.qss:12
            .m_paddingH = 2,                                            // Menu.qss:13
            .m_paddingV = 4,                                            // Menu.qss:13
            .m_text = Foundations::Surface::Text,                       // BaseStyleSheet.qss:18
            .m_selectedFill = Foundations::Mid,                         // Menu.qss:20
            .m_disabledText = Foundations::Highest,                     // Menu.qss:25
            .m_iconOffsets = MenuElement::IconOffsets{ .m_left = 0, .m_top = 0, .m_right = 6, .m_bottom = 0 },
            .m_separatorMarginTop = 2,                                  // Menu.qss:32
            .m_separatorBorder = 1,                                     // Menu.qss:31
            .m_separatorMarginBottom = 3,                               // Menu.qss:33
            .m_separatorLine = QColor(0x45, 0x45, 0x45),                // Menu.qss:31
        };
        // `QMenu::icon { left: 4px; top: 2px; }`
        const MenuElement ShiftedIconMenu{
            .m_name = "Test.Menu.ShiftedIconMenu",
            .m_surface = Foundations::BG,
            .m_paddingH = 2,
            .m_paddingV = 4,
            .m_text = Foundations::Surface::Text,
            .m_selectedFill = Foundations::Mid,
            .m_disabledText = Foundations::Highest,
            .m_iconOffsets = MenuElement::IconOffsets{ .m_left = 4, .m_top = 2, .m_right = 0, .m_bottom = 0 },
            .m_separatorMarginTop = 2,
            .m_separatorBorder = 1,
            .m_separatorMarginBottom = 3,
            .m_separatorLine = QColor(0x45, 0x45, 0x45),
        };
        // The popup with no rule for the icon
        const MenuElement PlainIconMenu{
            .m_name = "Test.Menu.PlainIconMenu",
            .m_surface = Foundations::BG,
            .m_paddingH = 2,
            .m_paddingV = 4,
            .m_text = Foundations::Surface::Text,
            .m_selectedFill = Foundations::Mid,
            .m_disabledText = Foundations::Highest,
            .m_separatorMarginTop = 2,
            .m_separatorBorder = 1,
            .m_separatorMarginBottom = 3,
            .m_separatorLine = QColor(0x45, 0x45, 0x45),
        };

        // ----------------------------------------------------------------------------------
        // The registrar of the module of these tests, run once for the process at the Flat rung
        // ----------------------------------------------------------------------------------

        void registerControlElements()
        {
            const QMetaObject* lineEdit = &QLineEdit::staticMetaObject;
            BoxElement::registerElement(&InvalidInput, lineEdit,
                [](const QWidget* w) { return isNamed(w, "invalidInput") && !Selectors::tagged(w, "ValidInput"); });
            BoxElement::registerElement(&DisabledTrackName, lineEdit, [](const QWidget* w) { return isNamed(w, "trackName"); });

            const QMetaObject* splitter = &QSplitter::staticMetaObject;
            SplitterElement::registerElement(&TransparentHandle4, splitter,
                [](const QWidget* w) { return isNamed(w, "dialogStack") && Selectors::tagged(w, "HandleHeight4"); });
            SplitterElement::registerElement(&TransparentHandle1, splitter,
                [](const QWidget* w) { return isNamed(w, "dialogStack") && Selectors::tagged(w, "HandleHeight1"); });
            SplitterElement::registerElement(&BlueHandle, splitter, [](const QWidget* w) { return isNamed(w, "blueSplitter"); });
            SplitterElement::registerElement(&ThickHandle, splitter, [](const QWidget* w) { return isNamed(w, "thickSplitter"); });

            const QMetaObject* checkBox = &QCheckBox::staticMetaObject;
            IndicatorElement::registerElement(&CaretCheck, checkBox, [](const QWidget* w) { return isNamed(w, "caretCheck"); });
            IndicatorElement::registerElement(&RestingCheck, checkBox, [](const QWidget* w) { return isNamed(w, "restingCheck"); });
            IndicatorElement::registerElement(&StyleImagesCheck, checkBox, [](const QWidget* w) { return isNamed(w, "styleImagesCheck"); });
            IndicatorElement::registerElement(&CompoundStatesCheck, checkBox, [](const QWidget* w) { return isNamed(w, "compoundCheck"); });
            IndicatorElement::registerElement(&RankedCheck, checkBox, [](const QWidget* w) { return isNamed(w, "rankedCheck"); });

            BoxElement::registerElement(&PaddedProgress, &QProgressBar::staticMetaObject,
                [](const QWidget* w) { return isNamed(w, "paddedProgress"); });
            BoxElement::registerElement(&TallPaddedProgress, &QProgressBar::staticMetaObject,
                [](const QWidget* w) { return isNamed(w, "tallProgress"); });
            BoxElement::registerElement(&NinePixelProgress, &QProgressBar::staticMetaObject,
                [](const QWidget* w) { return isNamed(w, "ninePixelProgress"); });
            BoxElement::registerElement(&CappedProgress, &QProgressBar::staticMetaObject,
                [](const QWidget* w) { return isNamed(w, "cappedProgress"); });

            const QMetaObject* menu = &QMenu::staticMetaObject;
            MenuElement::registerElement(&IconMenu, menu, [](const QWidget* w) { return isNamed(w, "iconMenu"); });
            MenuElement::registerElement(&ShiftedIconMenu, menu, [](const QWidget* w) { return isNamed(w, "shiftedIconMenu"); });
            MenuElement::registerElement(&PlainIconMenu, menu, [](const QWidget* w) { return isNamed(w, "plainIconMenu"); });
        }

        // ----------------------------------------------------------------------------------
        // F-lineedit-box
        // ----------------------------------------------------------------------------------

        //! A line edit under the application's real sheet, as the site makes it: with or without a frame, its own sheet if it has one.
        //! The legacy twin is the real engine's: Qt's sheet style over AzQtComponents::Style over Fusion.
        Row lineEditRow(const char* site, const char* name, bool frame, const char* ownSheet, std::vector<State> states)
        {
            Row row{
                .m_site = site,
                .m_make = [name, frame](QWidget* parent) -> QWidget*
                {
                    auto* edit = new QLineEdit(QStringLiteral("Track 1"), parent);
                    edit->setObjectName(QString::fromLatin1(name));
                    edit->setFrame(frame);
                    return edit;
                },
                .m_realApplicationSheet = true,
                .m_register = &registerControlElements,
                .m_states = states,
                .m_compare = Compare::Geometry | Compare::Fonts | Compare::Palette | Compare::Pixels,
                // The text and the placeholder. Not Base: the field is filled by the element in its border box, and no line edit paints
                // anything from its palette's Base (Style.cpp applyFlattenedTextEntryPalette gives it the class's)
                .m_roles = { QPalette::Text, QPalette::PlaceholderText },
                .m_base = Base::AzQtComponentsStyle,
            };
            if (ownSheet)
            {
                row.m_sheets.push_back({ site, SetOn::Widget, ownSheet });
            }
            return row;
        }

        const std::vector<State> fieldStates() { return { States::hover(), States::focus(), States::disabled() }; }

        TEST_F(ControlFoundationTwinTests, LineEditBox_AnElementWithTheRingOfTheSiteIsTheFieldOfTheSheet)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(lineEditRow("F-lineedit-box C103 EMStudioManager.cpp:262", "invalidInput", true,
                "border: 1px solid red;", fieldStates()), m_context));
        }

        TEST_F(ControlFoundationTwinTests, LineEditBox_ARingedFieldWithNoFrameHasTheSameBox)
        {
            // TrackHeaderWidget.cpp:226 m_nameEdit->setFrame(false), and :335 SetWidgetAsInvalidInput(m_nameEdit)
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(lineEditRow("F-lineedit-box C103 frameless TrackHeaderWidget.cpp:335", "invalidInput", false,
                "border: 1px solid red;", fieldStates()), m_context));
        }

        TEST_F(ControlFoundationTwinTests, LineEditBox_TheRingComesAndGoesWithTheStateOfTheField)
        {
            // LineEditValidatable.cpp:35-48: the sheet is set while the text is not valid and cleared when it is
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            Row row = lineEditRow("F-lineedit-box C148 LineEditValidatable.cpp:46", "invalidInput", true, nullptr,
                { sheetState("invalid text", "border: 1px solid red;", "ValidInput", false), States::disabled() });
            // idle: the field is valid (the tag says so), and the Global look is the field's
            row.m_make = [](QWidget* parent) -> QWidget*
            {
                auto* edit = new QLineEdit(QStringLiteral("Track 1"), parent);
                edit->setObjectName(QStringLiteral("invalidInput"));
                if (StyleManager::flatStylingActive())
                {
                    setTag(*edit, "ValidInput", true);
                }
                return edit;
            };
            EXPECT_TRUE(check(row, m_context));
        }

        TEST_F(ControlFoundationTwinTests, LineEditBox_AFieldWithNoFrameOutsideAnItemViewDrawsTheBoxOfTheSheet)
        {
            // Gap A12: LineEdit.qss:11-20 reached a line edit whatever its frame, and the sheet drew the rule of the field
            // (QStyleSheetStyle::drawPrimitive, PE_PanelLineEdit)
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(lineEditRow("A12 a frameless line edit", "plainField", false, nullptr, fieldStates()), m_context));
        }

        TEST_F(ControlFoundationTwinTests, LineEditBox_AFieldWithNoFrameAndAFillOfItsOwnKeepsTheRestOfTheBox)
        {
            // TrackHeaderWidget.cpp:226, :237
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(lineEditRow("F-lineedit-box C140 TrackHeaderWidget.cpp:237", "trackName", false,
                "background-color: rgb(70, 70, 70);", fieldStates()), m_context));
        }

        TEST_F(ControlFoundationTwinTests, LineEditBox_AFieldWithNoFrameInAnItemViewIsFramedAsTheSheetFramedIt)
        {
            // LineEdit.qss:37-40 `QAbstractItemView QLineEdit { qproperty-frame: true }`: the editor of a delegate has the box whatever
            // the delegate said about its frame
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            Row row = lineEditRow("A12 a frameless line edit in an item view", "viewEditor", false, nullptr, fieldStates());
            // QAbstractItemView is abstract, so a QListView stands for the class; its selector is the same
            row.m_chain = { { .m_name = "view", .m_make = [](QWidget* parent) -> QWidget* { return new QListView(parent); } } };
            EXPECT_TRUE(check(row, m_context));
        }

        TEST_F(ControlFoundationTwinTests, LineEditBox_AFieldInAComboBoxHasNoBoxOfItsOwnWithOrWithoutFrame)
        {
            // The line edit of an editable combo box is the combo box's (ComboBox.qss), with no panel of its own
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            Row row = lineEditRow("A12 the line edit of a combo box", "comboField", false, nullptr, { States::disabled() });
            row.m_make = [](QWidget* parent) -> QWidget*
            {
                auto* combo = new QComboBox(parent);
                combo->setEditable(true);
                combo->addItem(QStringLiteral("Track 1"));
                combo->setObjectName(QStringLiteral("comboHost"));
                return combo;
            };
            row.m_size = QSize(120, 20);
            row.m_compare = Compare::Geometry | Compare::Pixels;
            EXPECT_TRUE(check(row, m_context));
        }

        // ----------------------------------------------------------------------------------
        // F-splitter
        // ----------------------------------------------------------------------------------

        //! A splitter of two panes the way DialogStack makes it (MysticQt::DialogStackSplitter), under the application's real sheet
        Row splitterRow(const char* site, const char* name, Qt::Orientation orientation, const char* ownSheet, std::vector<State> states)
        {
            Row row{
                .m_site = site,
                .m_make = [name, orientation](QWidget* parent) -> QWidget*
                {
                    auto* splitter = new QSplitter(orientation, parent);
                    splitter->setObjectName(QString::fromLatin1(name));
                    if (StyleManager::flatStylingActive())
                    {
                        setTag(*splitter, "HandleHeight4", true);
                    }
                    splitter->addWidget(new Swatch(QColor(0x80, 0x20, 0x20), splitter));
                    splitter->addWidget(new Swatch(QColor(0x20, 0x80, 0x20), splitter));
                    splitter->addWidget(new Swatch(QColor(0x20, 0x20, 0x80), splitter));
                    return splitter;
                },
                .m_realApplicationSheet = true,
                .m_register = &registerControlElements,
                .m_states = states,
                .m_compare = Compare::Geometry | Compare::Pixels,
                .m_size = QSize(160, 120),
                .m_base = Base::AzQtComponentsStyle,
            };
            if (ownSheet)
            {
                row.m_sheets.push_back({ site, SetOn::Widget, ownSheet });
            }
            return row;
        }

        //! The pointer on the handle of a splitter (the second one: the first belongs to the first pane and is hidden)
        State handleHover()
        {
            const auto set = [](QWidget& widget, bool inside)
            {
                auto* splitter = qobject_cast<QSplitter*>(&widget);
                if (splitter && splitter->count() > 1)
                {
                    States::movePointer(*splitter->handle(1), inside);
                }
            };
            return { "pointer on a handle", [set](QWidget& widget) { set(widget, true); }, [set](QWidget& widget) { set(widget, false); } };
        }

        //! DialogStack's handle height, 4 or 1: the tag of the one height replaces the tag of the other
        void setHandleHeight(QWidget& widget, int height)
        {
            setTag(widget, height == 4 ? "HandleHeight1" : "HandleHeight4", false);
            setTag(widget, height == 4 ? "HandleHeight4" : "HandleHeight1", true);
        }

        //! DialogStack's handle height, 4 and 1, set by the sheet in the legacy GUI (DialogStack.cpp:383, :469) and by the class tag
        //! of the splitter in the flattened one
        State handleHeight(int height, int back)
        {
            const auto sheet = [](int px)
            {
                return QStringLiteral("QSplitter::handle{ height: %1px; background: transparent; }").arg(px);
            };
            return State{ QStringLiteral("handle height %1").arg(height).toStdString(),
                [height, sheet](QWidget& widget)
                {
                    if (StyleManager::flatStylingActive())
                    {
                        setHandleHeight(widget, height);
                    }
                    else
                    {
                        widget.setStyleSheet(sheet(height));
                    }
                },
                [back, sheet](QWidget& widget)
                {
                    if (StyleManager::flatStylingActive())
                    {
                        setHandleHeight(widget, back);
                    }
                    else
                    {
                        widget.setStyleSheet(sheet(back));
                    }
                } };
        }

        //! No change of its own: the reading of the splitter after the state before it was left (the stack closed, DialogStack.cpp:469)
        State afterTheStateBeforeWasLeft()
        {
            return State{ "after the state before was left", [](QWidget&) {}, [](QWidget&) {} };
        }

        constexpr const char* DialogStackSheet = "QSplitter::handle{ height: 4px; background: transparent; }";

        TEST_F(ControlFoundationTwinTests, Splitter_AHandleOfFourPixelsThatIsTransparentIsTheHandleOfTheDialogStack)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(splitterRow("F-splitter C142 DialogStack.cpp:35", "dialogStack", Qt::Vertical, DialogStackSheet,
                { handleHover(), States::disabled() }), m_context));
        }

        TEST_F(ControlFoundationTwinTests, Splitter_TheHandleChangesItsHeightWhenTheStackOpensAndCloses)
        {
            // DialogStack.cpp:383 (Open) and :469 (Close) set the sheet again with a height of 4 and 1. Each state is read once it is
            // entered, so the way back up is read by the state after it:
            //   open (4 to 1), left (1 to 4) / read after the stack closed / open again (4 to 1), left at 1 / closed (1 to 4)
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(splitterRow("F-splitter C145 DialogStack.cpp:383, C146 :469", "dialogStack", Qt::Vertical, DialogStackSheet,
                { handleHeight(1, 4), afterTheStateBeforeWasLeft(), handleHeight(1, 1), handleHeight(4, 4) }), m_context));
        }

        TEST_F(ControlFoundationTwinTests, Splitter_AHandleOfAnotherOrientationHasTheSameWidth)
        {
            // PM_SplitterWidth is asked with no option, so a horizontal splitter's handle is as wide as the rule's height
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(splitterRow("F-splitter horizontal splitter", "dialogStack", Qt::Horizontal, DialogStackSheet,
                { handleHover() }), m_context));
        }

        TEST_F(ControlFoundationTwinTests, Splitter_AFillOfItsOwnIsTheFillOfEveryStateOfTheHandle)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(splitterRow("F-splitter fill only", "blueSplitter", Qt::Horizontal,
                "QSplitter::handle { background-color: #336699; }", { handleHover(), States::disabled() }), m_context));
        }

        TEST_F(ControlFoundationTwinTests, Splitter_AWidthOfItsOwnKeepsTheFillsOfTheApplication)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(splitterRow("F-splitter width only", "thickSplitter", Qt::Vertical,
                "QSplitter::handle { height: 3px; }", { handleHover(), States::disabled() }), m_context));
        }

        TEST_F(ControlFoundationTwinTests, Splitter_ASplitterWithNoElementOfItsOwnIsTheHandleOfSplitterQss)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(splitterRow("F-splitter Global", "plainSplitter", Qt::Horizontal, nullptr,
                { handleHover(), States::disabled() }), m_context));
            EXPECT_TRUE(check(splitterRow("F-splitter Global vertical", "plainSplitter", Qt::Vertical, nullptr,
                { handleHover() }), m_context));
        }

        TEST_F(ControlFoundationTwinTests, Splitter_TheHandleUnderThePointerHasTheFillOfItsHoverRuleAndTheHandleAtRestTheOneAtRest)
        {
            // The rows above compare the twins in the hover state; this one reads the flat handle, so that a pointer that never reached
            // the handle would not pass them unseen (Splitter.qss:10 and :23)
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            RungScope flat(Rung::Flat);
            Context::registerOnce(&registerControlElements);

            QWidget root;
            auto* splitter = new QSplitter(Qt::Horizontal, &root);
            splitter->setObjectName(QStringLiteral("thickSplitter"));
            splitter->addWidget(new Swatch(QColor(0x80, 0x20, 0x20), splitter));
            splitter->addWidget(new Swatch(QColor(0x20, 0x80, 0x20), splitter));
            splitter->setGeometry(0, 0, 120, 60);
            for (QWidget* each : root.findChildren<QWidget*>())
            {
                each->setStyle(m_context.flatStyle());
            }
            root.setStyle(m_context.flatStyle());
            root.setAttribute(Qt::WA_DontShowOnScreen);
            root.resize(120, 60);
            root.show();
            QApplication::processEvents();

            QWidget* handle = splitter->handle(1);
            ASSERT_EQ(splitter->handleWidth(), 3);
            const QPoint middle = handle->mapTo(&root, handle->contentsRect().center());
            const auto handlePixel = [&root, &middle]()
            {
                QImage image(root.size(), QImage::Format_ARGB32_Premultiplied);
                image.fill(Qt::black);
                root.render(&image);
                return image.pixelColor(middle).rgb() & 0xFFFFFF;
            };
            EXPECT_EQ(handlePixel(), 0x222222u);
            States::movePointer(*handle, true);
            QApplication::processEvents();
            EXPECT_EQ(handlePixel(), 0x1E70EBu);
            States::movePointer(*handle, false);
            QApplication::processEvents();
            EXPECT_EQ(handlePixel(), 0x222222u);
        }

        TEST_F(ControlFoundationTwinTests, Splitter_ATwinThatIgnoresTheElementDiffersFromTheSheet)
        {
            // The negative control: the same handle with the widths of the Global element (what the style gave every splitter before
            // the element) differs from the sheet's four pixels
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            Row row = splitterRow("F-splitter control", "unmatchedSplitter", Qt::Vertical, DialogStackSheet, {});
            row.m_expectedDifference = "geometry.minimumSizeHint";
            EXPECT_TRUE(check(row, m_context));
        }

        // ----------------------------------------------------------------------------------
        // F-indicator-images
        // ----------------------------------------------------------------------------------

        //! A check box under the application's real sheet, with its own sheet and the state the site gives it
        Row checkBoxRow(const char* site, const char* name, const char* ownSheet, bool checked, std::vector<State> states,
            unsigned compare = Compare::Geometry | Compare::Fonts | Compare::Palette | Compare::Pixels)
        {
            return Row{
                .m_site = site,
                .m_make = [name, checked](QWidget* parent) -> QWidget*
                {
                    auto* checkBox = new QCheckBox(QStringLiteral("Motion Extraction"), parent);
                    checkBox->setObjectName(QString::fromLatin1(name));
                    checkBox->setChecked(checked);
                    return checkBox;
                },
                .m_sheets = { { site, SetOn::Widget, ownSheet } },
                .m_realApplicationSheet = true,
                .m_register = &registerControlElements,
                .m_states = states,
                .m_compare = compare,
                .m_roles = { QPalette::WindowText, QPalette::ButtonText, QPalette::Text },
                .m_size = QSize(160, 20),
                .m_base = Base::AzQtComponentsStyle,
            };
        }

        State uncheckedBox()
        {
            return { "unchecked",
                [](QWidget& widget) { qobject_cast<QCheckBox*>(&widget)->setChecked(false); },
                [](QWidget& widget) { qobject_cast<QCheckBox*>(&widget)->setChecked(true); } };
        }

        State partiallyCheckedBox()
        {
            return { "partially checked",
                [](QWidget& widget)
                {
                    auto* checkBox = qobject_cast<QCheckBox*>(&widget);
                    checkBox->setTristate(true);
                    checkBox->setCheckState(Qt::PartiallyChecked);
                },
                [](QWidget& widget)
                {
                    auto* checkBox = qobject_cast<QCheckBox*>(&widget);
                    checkBox->setCheckState(Qt::Checked);
                    checkBox->setTristate(false);
                } };
        }

        constexpr const char* CaretCheckSheet =
            "QCheckBox::indicator { width: 16px; height: 16px; border: none; margin: 0px; }\n"
            "QCheckBox::indicator:checked, QCheckBox::indicator:checked:disabled, QCheckBox::indicator:checked:focus\n"
            "{ image: url(:/Cards/img/UI20/Cards/caret-down.svg); }\n"
            "QCheckBox::indicator:unchecked, QCheckBox::indicator:unchecked:disabled, QCheckBox::indicator:unchecked:focus\n"
            "{ image: url(:/Cards/img/UI20/Cards/caret-right.svg); }";

        constexpr const char* RestingCheckSheet =
            "QCheckBox::indicator:unchecked{image: url(:/stylesheet/img/UI20/checkbox/off.svg);}\n"
            "QCheckBox::indicator:checked{image: url(:/stylesheet/img/UI20/checkbox/on.svg);}";

        constexpr const char* RankedCheckSheet =
            "QCheckBox::indicator:checked:disabled { image: url(:/Cards/img/UI20/Cards/caret-down.svg); }\n"
            "QCheckBox::indicator:checked { image: url(:/stylesheet/img/UI20/checkbox/on.svg); }";

        TEST_F(ControlFoundationTwinTests, IndicatorImages_ACompoundRuleBeforeThePlainOneStillDecidesItsStates)
        {
            // The more specific rule wins whatever the order of the rules: the disabled checked box draws the caret
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(checkBoxRow("F-indicator-images ranked entries", "rankedCheck", RankedCheckSheet, true,
                { States::disabled(), States::focus(), States::hover() }), m_context));
        }

        TEST_F(ControlFoundationTwinTests, IndicatorImages_ACaretForCheckedAndUncheckedIsTheIndicatorOfTheMotionExtractionBox)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(checkBoxRow("F-indicator-images C139 MotionExtractionWindow.cpp:167", "caretCheck", CaretCheckSheet, true,
                { uncheckedBox(), States::disabled(), States::focus(), States::hover(), partiallyCheckedBox() }), m_context));
        }

        TEST_F(ControlFoundationTwinTests, IndicatorImages_TheSameImageAtEveryStateIsTheIndicatorOfTheParticleBoxes)
        {
            // The widget sheet outranks the application's :focus and :disabled rules (CheckBox.qss:50-73): the box keeps off.svg and on.svg
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(checkBoxRow("F-indicator-images C177 PropertyEditorWidget.cpp:55", "restingCheck", RestingCheckSheet, true,
                { uncheckedBox(), States::disabled(), States::focus(), States::hover(), partiallyCheckedBox() }), m_context));
        }

        TEST_F(ControlFoundationTwinTests, IndicatorImages_AnElementWithoutTheImagesDrawsTheImagesOfTheStyleWhichDifferFromTheSheet)
        {
            // The negative control: the box of the site without its state images draws on-disabled.svg and on-focus.svg where the sheet
            // drew on.svg
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            Row row = checkBoxRow("F-indicator-images control", "styleImagesCheck", RestingCheckSheet, true,
                { States::disabled() }, Compare::Geometry | Compare::Pixels);
            row.m_expectedDifference = "pixels at ratio 1";
            EXPECT_TRUE(check(row, m_context));
        }

        TEST_F(ControlFoundationTwinTests, IndicatorImages_TheEntryNamingTheMostStatesDecidesAndTheImageForAllStatesIsTheFallback)
        {
            using Indicator = IndicatorElement;
            const auto state = [](QStyle::State flags) { return flags; };
            constexpr QStyle::State enabled = QStyle::State_Enabled;

            // the compound entry before the plain one: the entry naming more states decides, not the later one
            EXPECT_STREQ(RankedCheck.imageOf(state(QStyle::State_On)), CaretDown);                              // Checked | Disabled
            EXPECT_STREQ(RankedCheck.imageOf(state(enabled | QStyle::State_On)), CheckOn);                      // Checked
            EXPECT_STREQ(RankedCheck.imageOf(state(enabled | QStyle::State_Off)), "");                          // no entry: the style's

            EXPECT_STREQ(CompoundStatesCheck.imageOf(state(enabled | QStyle::State_Off)), CheckOff);            // m_image
            EXPECT_STREQ(CompoundStatesCheck.imageOf(state(enabled | QStyle::State_On)), CheckOn);              // Checked
            EXPECT_STREQ(CompoundStatesCheck.imageOf(state(QStyle::State_On)), CaretDown);                      // Checked | Disabled
            EXPECT_STREQ(CompoundStatesCheck.imageOf(state(enabled | QStyle::State_On | QStyle::State_MouseOver)), CaretRight);  // Hover
            // the pointer over a disabled box is no hover (QStyleSheetStyle::pseudoClass)
            EXPECT_STREQ(CompoundStatesCheck.imageOf(state(QStyle::State_On | QStyle::State_MouseOver)), CaretDown);
            EXPECT_STREQ(CompoundStatesCheck.imageOf(state(enabled | QStyle::State_NoChange)), CheckOff);       // neither
            EXPECT_STREQ(CompoundStatesCheck.imageOf(state(enabled | QStyle::State_NoChange | QStyle::State_HasFocus)), CaretDown);
            // an element with no entry for the state and no image of its own: the style's
            EXPECT_STREQ(CaretCheck.imageOf(state(enabled | QStyle::State_NoChange)), "");
            EXPECT_STREQ(Indicator().imageOf(state(enabled)), "");
        }

        // ----------------------------------------------------------------------------------
        // F-progressbar-box
        // ----------------------------------------------------------------------------------

        //! A progress bar the way CommandBarPlugin makes it (CommandBarPlugin.cpp:108-112), under the application's real sheet. The
        //! maximum height that Style::polish gives a flat bar, and no legacy bar has, is what the box of the bar is high: the legacy twin
        //! gets the same number from the row, and the flat twin from the polish (the height of the bars with a box above and below the
        //! track is what the rows after the first prove). QWIDGETSIZE_MAX: the legacy twin keeps the maximum its sheet gives it.
        Row progressRow(const char* site, const char* name, std::vector<State> states, unsigned compare, const QSize& size,
            const char* ownSheet = "padding-right: 2px;", int maximumHeight = 5)
        {
            return Row{
                .m_site = site,
                .m_make = [name, maximumHeight](QWidget* parent) -> QWidget*
                {
                    auto* bar = new QProgressBar(parent);
                    bar->setObjectName(QString::fromLatin1(name));
                    bar->setRange(0, 100);
                    bar->setValue(0);
                    bar->setMaximumHeight(maximumHeight);
                    return bar;
                },
                .m_sheets = { { "CommandBarPlugin.cpp:112", SetOn::Widget, ownSheet } },
                .m_realApplicationSheet = true,
                .m_register = &registerControlElements,
                .m_states = states,
                .m_compare = compare,
                .m_roles = { QPalette::Base, QPalette::Highlight, QPalette::Text, QPalette::WindowText },
                .m_size = size,
                .m_base = Base::AzQtComponentsStyle,
                .m_mayDrawNothing = true,           // a bar at its minimum is the track alone, of one colour
            };
        }

        State progressValue(int value)
        {
            return { QStringLiteral("value %1").arg(value).toStdString(),
                [value](QWidget& widget) { qobject_cast<QProgressBar*>(&widget)->setValue(value); },
                [](QWidget& widget) { qobject_cast<QProgressBar*>(&widget)->setValue(0); } };
        }

        TEST_F(ControlFoundationTwinTests, ProgressBarBox_APaddingOnTheRightLeavesTheChunkItsRectAsTheSheetDid)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(progressRow("F-progressbar-box C130 pixels", "paddedProgress",
                { progressValue(1), progressValue(50), progressValue(99), progressValue(100), States::disabled() },
                Compare::Geometry | Compare::Fonts | Compare::Pixels, QSize(200, 5)), m_context));
        }

        TEST_F(ControlFoundationTwinTests, ProgressBarBox_TheHintOfABarWithAPaddingIsTheHintOfTheSheet)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(progressRow("F-progressbar-box C130 hint", "paddedProgress", { progressValue(50) },
                Compare::Geometry | Compare::Fonts, QSize()), m_context));
        }

        TEST_F(ControlFoundationTwinTests, ProgressBarBox_ABarWithNoPaddingElementIsTheBarOfTheSheetWithoutIt)
        {
            // The negative control: the bar of the site matched by no element is the Global bar, which has no padding at its right
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            Row row = progressRow("F-progressbar-box control", "unmatchedProgress", { progressValue(50) }, Compare::Geometry, QSize());
            row.m_expectedDifference = "geometry.sizeHint";
            EXPECT_TRUE(check(row, m_context));
        }

        //! The bar of CommandBarPlugin.cpp:112 with a box above and below the track as well (`padding: 2px 2px 3px 0px;`, 5px in
        //! height): the legacy bar is as tall as its hint, which is the box around the track, and has no maximum height of its own
        Row verticalBoxProgressRow(const char* site, const char* name, const char* ownSheet, int maximumHeight, unsigned compare,
            const QSize& size, std::vector<State> states)
        {
            Row row = progressRow(site, name, states, compare, size, ownSheet, maximumHeight);
            row.m_sheets[0].m_cite = "CommandBarPlugin.cpp:112 with a padding above and below the track";
            return row;
        }

        TEST_F(ControlFoundationTwinTests, ProgressBarBox_APaddingAboveAndBelowMakesTheBarAsTallAsItsHint)
        {
            // The height of the bar in a layout is its hint (5px of track and 5px of padding), not the 5px of the track
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(verticalBoxProgressRow("F-progressbar-box vertical padding hint", "tallProgress",
                "padding: 2px 2px 3px 0px;", 10, Compare::Geometry | Compare::Fonts, QSize(), { progressValue(50) }), m_context));
        }

        TEST_F(ControlFoundationTwinTests, ProgressBarBox_APaddingAboveAndBelowKeepsTheTrackInTheBorderRectAndTheChunkInTheContents)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(verticalBoxProgressRow("F-progressbar-box vertical padding pixels", "tallProgress",
                "padding: 2px 2px 3px 0px;", 10, Compare::Geometry | Compare::Fonts | Compare::Pixels, QSize(200, 10),
                { progressValue(1), progressValue(50), progressValue(100), States::disabled() }), m_context));
        }

        TEST_F(ControlFoundationTwinTests, ProgressBarBox_AHeightOfItsOwnIsTheContentsOfTheHintInsteadOfTheTrack)
        {
            // `height: 9px` in the bar's own rule: 9px of contents and 5px of padding
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(verticalBoxProgressRow("F-progressbar-box height of its own", "ninePixelProgress",
                "padding: 2px 2px 3px 0px; height: 9px;", 14, Compare::Geometry | Compare::Fonts, QSize(), { progressValue(50) }),
                m_context));
        }

        TEST_F(ControlFoundationTwinTests, ProgressBarBox_AMaximumHeightOfItsOwnIsTheLimitOfTheBarGrownByTheBox)
        {
            // `max-height: 3px` limits the contents, which are the lesser of it and the 5px of the track (QStyleSheetStyle::setGeometry):
            // 3px and 5px of padding. The legacy twin keeps the maximum its sheet gave it, and the flat bar the one its element did,
            // which is below the hint of 10px.
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(verticalBoxProgressRow("F-progressbar-box max-height of its own", "cappedProgress",
                "padding: 2px 2px 3px 0px; max-height: 3px;", QWIDGETSIZE_MAX, Compare::Geometry | Compare::Fonts, QSize(),
                { progressValue(50) }), m_context));
        }

        TEST_F(ControlFoundationTwinTests, ProgressBarBox_ATwinWhoseBarIsCappedAtTheTrackDiffersFromTheTallBarOfTheSheet)
        {
            // The negative control: the same bar with the maximum of the track alone (what Style::polish gave every flat bar before the
            // maximum followed the box) is 5px high where the sheet's hint is 10px
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            Row row = verticalBoxProgressRow("F-progressbar-box control", "tallProgress", "padding: 2px 2px 3px 0px;", 5,
                Compare::Geometry, QSize(), { progressValue(50) });
            row.m_expectedDifference = "geometry.maximumSize";
            EXPECT_TRUE(check(row, m_context));
        }

        // ----------------------------------------------------------------------------------
        // F-menu-icon
        // ----------------------------------------------------------------------------------

        QIcon swatchIcon(const QColor& color)
        {
            QPixmap pixmap(16, 16);
            pixmap.fill(color);
            return QIcon(pixmap);
        }

        //! The Add menu of the audio controls panel (ATLControlsPanel.cpp:122-129): icon items, a separator, a folder
        Row iconMenuRow(const char* site, const char* name, const char* ownSheet, std::vector<State> states)
        {
            Row row{
                .m_site = site,
                .m_make = [name](QWidget* parent) -> QWidget*
                {
                    auto* menu = new QMenu(parent);
                    menu->setObjectName(QString::fromLatin1(name));
                    menu->addAction(swatchIcon(QColor(0xC8, 0x3C, 0x3C)), QStringLiteral("Trigger"));
                    menu->addAction(swatchIcon(QColor(0x3C, 0xC8, 0x3C)), QStringLiteral("RTPC"));
                    menu->addAction(QStringLiteral("Without icon"));
                    menu->addSeparator();
                    menu->addAction(swatchIcon(QColor(0x3C, 0x3C, 0xC8)), QStringLiteral("Folder"))->setEnabled(false);
                    QAction* checked = menu->addAction(swatchIcon(QColor(0xC8, 0xC8, 0x3C)), QStringLiteral("Checked with icon"));
                    checked->setCheckable(true);
                    checked->setChecked(true);
                    menu->addAction(QStringLiteral("Checked without icon"))->setCheckable(true);
                    return menu;
                },
                .m_realApplicationSheet = true,
                .m_register = &registerControlElements,
                .m_states = states,
                .m_compare = Compare::Geometry | Compare::Pixels,
                .m_base = Base::AzQtComponentsStyle,
            };
            if (ownSheet)
            {
                row.m_sheets.push_back({ site, SetOn::Widget, ownSheet });
            }
            return row;
        }

        State activeAction(int index)
        {
            return { QStringLiteral("action %1 active").arg(index).toStdString(),
                [index](QWidget& widget)
                {
                    auto* menu = qobject_cast<QMenu*>(&widget);
                    menu->setActiveAction(menu->actions().value(index));
                },
                [](QWidget& widget) { qobject_cast<QMenu*>(&widget)->setActiveAction(nullptr); } };
        }

        TEST_F(ControlFoundationTwinTests, MenuIcon_AnIconMovedByTheRuleOfTheAudioMenuIsWhereTheSheetDrewIt)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(iconMenuRow("F-menu-icon C101 ATLControlsPanel.cpp:41", "iconMenu", "QMenu::icon { left: 0px; right: 6px; }",
                { activeAction(0), activeAction(1), activeAction(4), activeAction(5) }), m_context));
        }

        TEST_F(ControlFoundationTwinTests, MenuIcon_ALeftAndATopOffsetMoveTheIconTheOtherWay)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(iconMenuRow("F-menu-icon left and top", "shiftedIconMenu", "QMenu::icon { left: 4px; top: 2px; }",
                { activeAction(1), activeAction(5) }), m_context));
        }

        TEST_F(ControlFoundationTwinTests, MenuIcon_AMenuWithNoRuleForTheIconHasTheIconWhereTheBaseStyleDrawsIt)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(iconMenuRow("F-menu-icon no icon rule", "plainIconMenu", nullptr, { activeAction(1) }), m_context));
        }

        TEST_F(ControlFoundationTwinTests, MenuIcon_ARightToLeftMenuMirrorsTheOffsets)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            Row row = iconMenuRow("F-menu-icon right to left", "iconMenu", "QMenu::icon { left: 0px; right: 6px; }", { activeAction(1) });
            const auto make = row.m_make;
            row.m_make = [make](QWidget* parent) -> QWidget*
            {
                QWidget* menu = make(parent);
                menu->setLayoutDirection(Qt::RightToLeft);
                return menu;
            };
            EXPECT_TRUE(check(row, m_context));
        }

        TEST_F(ControlFoundationTwinTests, MenuIcon_AnElementWithoutTheOffsetsDrawsTheIconInTheCheckColumnWhichDiffersFromTheSheet)
        {
            // The negative control: the popup of the site without the rule for the icon
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            Row row = iconMenuRow("F-menu-icon control", "plainIconMenu", "QMenu::icon { left: 0px; right: 6px; }", {});
            row.m_expectedDifference = "pixels at ratio 1";
            EXPECT_TRUE(check(row, m_context));
        }

        TEST_F(ControlFoundationTwinTests, MenuIcon_TheOffsetsAreTheOppositeSidesWhereTheNearOneIsZero)
        {
            const MenuElement element{ .m_iconOffsets = MenuElement::IconOffsets{ .m_left = 0, .m_top = 0, .m_right = 6, .m_bottom = 3 } };
            const QRect item(10, 20, 100, 24);
            const QSize icon(16, 16);
            EXPECT_EQ(element.iconRect(item, icon, Qt::LeftToRight), QRect(4, 20 + 4 - 3, 16, 16));
            EXPECT_EQ(element.iconRect(item, icon, Qt::RightToLeft), QRect(110 - 16 + 6, 20 + 4 - 3, 16, 16));
            const MenuElement closer{ .m_iconOffsets = MenuElement::IconOffsets{ .m_left = 5, .m_top = 2, .m_right = 9, .m_bottom = 9 } };
            EXPECT_EQ(closer.iconRect(item, icon, Qt::LeftToRight), QRect(15, 26, 16, 16));
        }
    } // namespace ControlFoundationTwinTestsInternal
} // namespace AzQtComponents
