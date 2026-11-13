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
#include <AzQtComponents/Components/Elements/FrameElement.h>
#include <AzQtComponents/Components/Elements/IndicatorElement.h>
#include <AzQtComponents/Components/Elements/ItemViewElement.h>
#include <AzQtComponents/Components/Elements/MenuElement.h>
#include <AzQtComponents/Components/Elements/TabElement.h>
#include <AzQtComponents/Components/Elements/TextElement.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>

#include <QCheckBox>
#include <QFrame>
#include <QGroupBox>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMenu>
#include <QPixmap>
#include <QPushButton>
#include <QTabBar>

namespace AzQtComponents
{
    // ----------------------------------------------------------------------------------
    // Foundations of the flattened GUI that the sites of the shared libraries and the gems ask for, each proven on a widget of the kind
    // of the sites that use it, against the sheet rule it stands for (Qt's own style sheet style over the sheet's real rules; the flat
    // twin is the flattened Style with an element):
    //   - F-within-tag: `.tag W`, Selectors::withinTagged;
    //   - F-reresolve: a class tag set on a widget and its elements resolved again, Style::addClass and removeClass (the colour of a
    //     line edit turned red and back, PropertyColorCtrl.cpp:338 and :342; the border of a label,
    //     BlendSpaceMotionContainerHandler.cpp:152, :157);
    //   - F-dashed-edge: `border: 2px dashed #979797` (ParameterCreateEditWidget.cpp:83), BoxElement::LineStyle::Dashed;
    //   - F-inset-radius: `border-style: inset` with `border-radius: 8px` (LUAEditorMainWindow.cpp:576-580), against the corner arcs of
    //     qDrawBorder;
    //   - F-groupbox-frame: a group box whose rule gives it a fill and a border of its own (KinematicDescriptionDialog.cpp:82-94, .ui:79),
    //     and one whose rule gives it neither, drawn through the sheet under the application's universal rule all the same.
    // The elements here are matched by the object name or the class tag their row gives the widget, so that no other test meets them.
    // ----------------------------------------------------------------------------------

    namespace BoxFoundationTwinTestsInternal
    {
        using namespace SheetTwins;

        class BoxFoundationTwinTests
            : public ::UnitTest::LeakDetectionFixture
        {
        protected:
            Context m_context;
        };

        // ----------------------------------------------------------------------------------
        // Helpers
        // ----------------------------------------------------------------------------------

        QIcon filledIcon(const QSize& size, const QColor& color)
        {
            QPixmap pixmap(size);
            pixmap.fill(color);
            return QIcon(pixmap);
        }

        template<int Variant>
        bool isVariant(const QWidget* widget)
        {
            return widget->property("foundationVariant").toInt() == Variant;
        }

        bool isNamed(const QWidget* widget, const char* name)
        {
            return Selectors::named(widget, name);
        }

        // ----------------------------------------------------------------------------------
        // The elements
        // ----------------------------------------------------------------------------------

        // ---- F-within-tag: .SceneTag QFrame { margin: 3px; padding: 2px; border: 2px solid #33AA77; background-color: #555555; }
        const BoxElement SceneTagFrame{
            .m_name = "Test.Box.SceneTagFrame",
            .m_margin = QMargins(3, 3, 3, 3),
            .m_padding = QMargins(2, 2, 2, 2),
            .m_nativeBorder = false,
            .m_idle = {
                .m_fill = QBrush(QColor(0x55, 0x55, 0x55)),
                .m_edges = BoxElement::border(2, QColor(0x33, 0xAA, 0x77)),
            },
        };

        // ---- F-reresolve. PropertyColorCtrl.cpp:338 `color: red;`
        const TextElement InvalidHexText{ .m_name = "Test.Text.InvalidHex", .m_color = QColor(Qt::red) };

        // #m_labelMotion { border: 1px solid red; } (BlendSpaceMotionContainerHandler.cpp:152) over Text.qss:93-94 (the margin of a label)
        const BoxElement MotionInvalid{
            .m_name = "Test.Box.MotionInvalid",
            .m_margin = QMargins(0, 1, 0, 1),
            .m_nativeBorder = false,
            .m_idle = { .m_edges = BoxElement::border(1, QColor(Qt::red)) },
        };
        // #m_labelMotion { border: none; } (:157) over Text.qss:93-94
        const BoxElement MotionValid{ .m_name = "Test.Box.MotionValid", .m_margin = QMargins(0, 1, 0, 1), .m_nativeBorder = false };

        // The element of each family a widget of its class can have, for the test that every family resolves again
        const BoxElement ProbeBox{ .m_name = "Test.Box.Probe", .m_minSize = QSize(41, 17) };
        const FrameElement ProbeFrame{ .m_name = "Test.Frame.Probe", .m_fill = QColor(0x12, 0x34, 0x56), .m_fillShapes = 0x3F };
        const IndicatorElement ProbeIndicator{ .m_name = "Test.Indicator.Probe", .m_imageSize = QSize(11, 11) };
        const TabElement ProbeTab{ .m_name = "Test.Tab.Probe", .m_barFill = QColor(0x65, 0x43, 0x21) };
        const MenuElement ProbeMenu{ .m_name = "Test.Menu.Probe", .m_surface = QColor(0x21, 0x43, 0x65) };
        const ItemViewElement ProbeView{ .m_name = "Test.ItemView.Probe", .m_itemHeight = 37 };
        const TextElement ProbeText{ .m_name = "Test.Text.Probe", .m_color = QColor(0x11, 0x99, 0x33) };
        //! The probe label's look without the state. The flattened Style gives no palette back when a text element's colour ends (HEAD's
        //! keep-or-forget), so a state that ends on a colour of its own gives that colour a static element too (plan D.25)
        const TextElement ProbeTextIdle{ .m_name = "Test.Text.ProbeIdle", .m_color = QColor(0x33, 0x44, 0x55) };

        // ---- F-dashed-edge. ParameterCreateEditWidget.cpp:83
        // QFrame#previewFrame { border: 2px dashed #979797; background-color: #85858580; }
        const QColor DashLine(0x97, 0x97, 0x97);
        const QColor DashFill(0x85, 0x85, 0x80, 0x85);      // `#85858580`, which Qt reads as #AARRGGBB
        const BoxElement DashedPreview{
            .m_name = "Test.Box.DashedPreview",
            .m_nativeBorder = false,
            .m_idle = { .m_fill = QBrush(DashFill), .m_edges = BoxElement::border(2, DashLine, BoxElement::LineStyle::Dashed) },
        };
        // border: 1px dashed (a dotted line in the sheet style)
        const BoxElement DashedThin{
            .m_name = "Test.Box.DashedThin",
            .m_nativeBorder = false,
            .m_idle = { .m_fill = QBrush(DashFill), .m_edges = BoxElement::border(1, DashLine, BoxElement::LineStyle::Dashed) },
        };
        // border: 3px dashed; border-radius: 6px
        const BoxElement DashedRound{
            .m_name = "Test.Box.DashedRound",
            .m_nativeBorder = false,
            .m_radius = 6,
            .m_idle = { .m_fill = QBrush(DashFill), .m_edges = BoxElement::border(3, DashLine, BoxElement::LineStyle::Dashed) },
        };
        // border: 2px dashed; border-radius: 8px
        const BoxElement DashedRoundThin{
            .m_name = "Test.Box.DashedRoundThin",
            .m_nativeBorder = false,
            .m_radius = 8,
            .m_idle = { .m_fill = QBrush(DashFill), .m_edges = BoxElement::border(2, DashLine, BoxElement::LineStyle::Dashed) },
        };
        // border-top: 2px dashed #979797; border-right: 2px dashed #E25243; border-bottom: 2px dashed #3A7BD5;
        // border-left: 2px dashed #7BD53A
        BoxElement::Edges mixedEdges()
        {
            const auto dashed = [](const QColor& color) { return BoxElement::Edge{ 2, color, BoxElement::LineStyle::Dashed }; };
            return { dashed(DashLine), dashed(QColor(0xE2, 0x52, 0x43)), dashed(QColor(0x3A, 0x7B, 0xD5)),
                dashed(QColor(0x7B, 0xD5, 0x3A)) };
        }
        // border-top: 2px dashed #979797; border-right: 2px solid #E25243; border-bottom: 2px dashed #979797;
        // border-left: 2px solid #E25243
        BoxElement::Edges dashedSolidEdges()
        {
            const BoxElement::Edge dashed{ 2, DashLine, BoxElement::LineStyle::Dashed };
            const BoxElement::Edge solid{ 2, QColor(0xE2, 0x52, 0x43) };
            return { dashed, solid, dashed, solid };
        }
        const BoxElement DashedSolid{
            .m_name = "Test.Box.DashedSolid",
            .m_nativeBorder = false,
            .m_idle = { .m_edges = dashedSolidEdges() },
        };
        // border: 3px solid rgba(226,82,67,128): one translucent colour, which the sheet style does not paint over at a corner
        const BoxElement TranslucentSolid{
            .m_name = "Test.Box.TranslucentSolid",
            .m_nativeBorder = false,
            .m_idle = { .m_edges = BoxElement::border(3, QColor(0xE2, 0x52, 0x43, 0x80)) },
        };
        const BoxElement DashedMixed{ .m_name = "Test.Box.DashedMixed", .m_nativeBorder = false, .m_idle = { .m_edges = mixedEdges() } };

        // ---- F-inset-radius. LUAEditorMainWindow.cpp:576-580 background: rgba(192,192,192,255); color: black; border-style: inset;
        // border-width: 1px; border-color: rgba(100,100,100,255); border-radius: 8px;
        const QColor InsetFill(192, 192, 192);
        const QColor InsetDark(100, 100, 100);
        const QColor OutsetBase(80, 120, 160);
        const BoxElement::LineStyle Inset = BoxElement::LineStyle::Inset;
        const BoxElement::LineStyle Outset = BoxElement::LineStyle::Outset;
        const BoxElement InsetRound{
            .m_name = "Test.Box.InsetRound",
            .m_nativeBorder = false,
            .m_radius = 8,
            .m_idle = { .m_fill = QBrush(InsetFill), .m_edges = BoxElement::border(1, InsetDark, Inset) },
        };
        const BoxElement InsetThick{
            .m_name = "Test.Box.InsetThick",
            .m_nativeBorder = false,
            .m_radius = 8,
            .m_idle = { .m_fill = QBrush(InsetFill), .m_edges = BoxElement::border(3, InsetDark, Inset) },
        };
        const BoxElement InsetSmallRadius{
            .m_name = "Test.Box.InsetSmallRadius",
            .m_nativeBorder = false,
            .m_radius = 3,
            .m_idle = { .m_fill = QBrush(InsetFill), .m_edges = BoxElement::border(2, InsetDark, Inset) },
        };
        const BoxElement OutsetRound{
            .m_name = "Test.Box.OutsetRound",
            .m_nativeBorder = false,
            .m_radius = 8,
            .m_idle = { .m_fill = QBrush(InsetFill), .m_edges = BoxElement::border(2, OutsetBase, Outset) },
        };
        // border-width: 1px; different radii per corner: border-top-left-radius: 10px; border-bottom-right-radius: 4px
        const BoxElement InsetCorners{
            .m_name = "Test.Box.InsetCorners",
            .m_nativeBorder = false,
            .m_radii = { .m_topLeft = 10, .m_topRight = 0, .m_bottomRight = 4, .m_bottomLeft = 0 },
            .m_idle = { .m_fill = QBrush(InsetFill), .m_edges = BoxElement::border(1, InsetDark, Inset) },
        };
        // The inset border of the first rule drawn as a solid one of the colour of the rule
        const BoxElement InsetFlat{
            .m_name = "Test.Box.InsetFlat",
            .m_nativeBorder = false,
            .m_radius = 8,
            .m_idle = { .m_fill = QBrush(InsetFill), .m_edges = BoxElement::border(1, InsetDark) },
        };
        // border-style: inset, no radius
        const BoxElement InsetSquare{
            .m_name = "Test.Box.InsetSquare",
            .m_nativeBorder = false,
            .m_idle = { .m_fill = QBrush(InsetFill), .m_edges = BoxElement::border(1, InsetDark, Inset) },
        };

        // border-style: inset; border-width: 2px; border-color: rgba(100,100,100,255); no radius
        const BoxElement InsetSquareThick{
            .m_name = "Test.Box.InsetSquareThick",
            .m_nativeBorder = false,
            .m_idle = { .m_fill = QBrush(InsetFill), .m_edges = BoxElement::border(2, InsetDark, Inset) },
        };
        // border-style: outset; border-width: 3px; border-color: rgb(80,120,160); no radius
        const BoxElement OutsetSquareThick{
            .m_name = "Test.Box.OutsetSquareThick",
            .m_nativeBorder = false,
            .m_idle = { .m_fill = QBrush(InsetFill), .m_edges = BoxElement::border(3, OutsetBase, Outset) },
        };
        // border-width: 2px; border-top-left-radius: 10px; border-bottom-right-radius: 4px: the other corners are square
        const BoxElement InsetCornersThick{
            .m_name = "Test.Box.InsetCornersThick",
            .m_nativeBorder = false,
            .m_radii = { .m_topLeft = 10, .m_topRight = 0, .m_bottomRight = 4, .m_bottomLeft = 0 },
            .m_idle = { .m_fill = QBrush(InsetFill), .m_edges = BoxElement::border(2, InsetDark, Inset) },
        };

        // The label of the site, whose own sheet is the rule above: under the application's label rule (Text.qss:93-94, margin-top: 1px;
        // margin-bottom: 1px), which the element restates
        const BoxElement InsetRoundSite{
            .m_name = "Test.Box.InsetRoundSite",
            .m_margin = QMargins(0, 1, 0, 1),
            .m_nativeBorder = false,
            .m_radius = 8,
            .m_idle = { .m_fill = QBrush(InsetFill), .m_edges = BoxElement::border(1, InsetDark, Inset) },
        };

        // color: black; of the same rule
        const TextElement InsetText{ .m_name = "Test.Text.InsetText", .m_color = QColor(Qt::black) };

        // ---- F-groupbox-frame. KinematicDescriptionDialog.cpp:82-94
        // `background-color: rgb(51, 51, 51); border: 1px solid rgb(30, 112, 235);`
        const QColor BoxFill(51, 51, 51);
        const QColor BoxLine(30, 112, 235);
        const BoxElement GroupBoxBordered{
            .m_name = "Test.Box.GroupBoxBordered",
            .m_nativeBorder = false,
            .m_idle = { .m_fill = QBrush(BoxFill), .m_edges = BoxElement::border(1, BoxLine) },
        };
        // background-color: rgb(51, 51, 51); border: none;
        const BoxElement GroupBoxBorderless{
            .m_name = "Test.Box.GroupBoxBorderless",
            .m_nativeBorder = false,
            .m_idle = { .m_fill = QBrush(BoxFill) },
        };
        // KinematicDescriptionDialog.ui:79 border: none
        const BoxElement GroupBoxNone{ .m_name = "Test.Box.GroupBoxNone", .m_nativeBorder = false };
        // background-color: rgb(51, 51, 51); and the frame the style draws
        const BoxElement GroupBoxFillOnly{ .m_name = "Test.Box.GroupBoxFillOnly", .m_idle = { .m_fill = QBrush(BoxFill) } };
        // background-color: rgb(51, 51, 51); border: 2px solid; border-radius: 6px; padding: 12px 6px 4px 6px
        // QGroupBox { min-width: 100px; min-height: 40px; }: a rule with no fill, no box and the border of the style
        const BoxElement GroupBoxBare{ .m_name = "Test.Box.GroupBoxBare", .m_minSize = QSize(100, 40) };
        const BoxElement GroupBoxPadded{
            .m_name = "Test.Box.GroupBoxPadded",
            .m_padding = QMargins(6, 12, 6, 4),
            .m_nativeBorder = false,
            .m_radius = 6,
            .m_idle = { .m_fill = QBrush(BoxFill), .m_edges = BoxElement::border(2, BoxLine) },
        };

        // ----------------------------------------------------------------------------------
        // The registrar of the module of these tests, run once for the process at the Flat rung
        // ----------------------------------------------------------------------------------

        void registerFoundationElements()
        {
            const QMetaObject* frame = &QFrame::staticMetaObject;
            const QMetaObject* label = &QLabel::staticMetaObject;

            // F-within-tag: the target frame has no object name of the sheet's: the selector is the ancestor's tag
            BoxElement::registerElement(&SceneTagFrame, frame,
                [](const QWidget* w) { return isNamed(w, "tagTarget") && Selectors::withinTagged(w, "SceneTag"); });

            // F-reresolve
            TextElement::registerElement(&InvalidHexText, &QLineEdit::staticMetaObject,
                [](const QWidget* w) { return Selectors::tagged(w, "InvalidHex"); });
            BoxElement::registerElement(&MotionInvalid, label,
                [](const QWidget* w) { return isNamed(w, "m_labelMotion") && Selectors::tagged(w, "MotionInvalid"); });
            BoxElement::registerElement(&MotionValid, label,
                [](const QWidget* w) { return isNamed(w, "m_labelMotion") && Selectors::tagged(w, "MotionValid"); });
            const ElementMatcher probe = [](const QWidget* w) { return Selectors::tagged(w, "ProbeOn"); };
            BoxElement::registerElement(&ProbeBox, &QPushButton::staticMetaObject, probe);
            FrameElement::registerElement(&ProbeFrame, frame, probe);
            IndicatorElement::registerElement(&ProbeIndicator, &QCheckBox::staticMetaObject, probe);
            TabElement::registerElement(&ProbeTab, &QTabBar::staticMetaObject, probe);
            MenuElement::registerElement(&ProbeMenu, &QMenu::staticMetaObject, probe);
            ItemViewElement::registerElement(&ProbeView, &QListView::staticMetaObject, probe);
            TextElement::registerElement(&ProbeText, label, probe);
            TextElement::registerElement(&ProbeTextIdle, label,
                [](const QWidget* w) { return isNamed(w, "probeLabel") && !Selectors::tagged(w, "ProbeOn"); });

            // F-dashed-edge and F-inset-radius: a frame or a label with the variant in its property
            BoxElement::registerElement(&DashedPreview, frame, isVariant<1>);
            BoxElement::registerElement(&DashedThin, frame, isVariant<2>);
            BoxElement::registerElement(&DashedRound, frame, isVariant<3>);
            BoxElement::registerElement(&DashedRoundThin, frame, isVariant<4>);
            BoxElement::registerElement(&DashedMixed, frame, isVariant<5>);
            BoxElement::registerElement(&DashedSolid, frame, isVariant<6>);
            BoxElement::registerElement(&TranslucentSolid, frame, isVariant<7>);
            BoxElement::registerElement(&InsetRound, label, isVariant<11>);
            BoxElement::registerElement(&InsetThick, label, isVariant<12>);
            BoxElement::registerElement(&InsetSmallRadius, label, isVariant<13>);
            BoxElement::registerElement(&OutsetRound, label, isVariant<14>);
            BoxElement::registerElement(&InsetCorners, label, isVariant<15>);
            BoxElement::registerElement(&InsetSquare, label, isVariant<16>);
            BoxElement::registerElement(&InsetFlat, label, isVariant<20>);
            BoxElement::registerElement(&InsetSquareThick, label, isVariant<17>);
            BoxElement::registerElement(&OutsetSquareThick, label, isVariant<18>);
            BoxElement::registerElement(&InsetCornersThick, label, isVariant<19>);
            BoxElement::registerElement(&InsetRoundSite, label, isVariant<30>);
            TextElement::registerElement(&InsetText, label,
                [](const QWidget* w)
                {
                    const int variant = w->property("foundationVariant").toInt();
                    return (variant >= 11 && variant <= 20) || variant == 30;
                });

            // F-groupbox-frame
            const QMetaObject* groupBox = &QGroupBox::staticMetaObject;
            BoxElement::registerElement(&GroupBoxBordered, groupBox, isVariant<21>);
            BoxElement::registerElement(&GroupBoxBorderless, groupBox, isVariant<22>);
            BoxElement::registerElement(&GroupBoxNone, groupBox, isVariant<23>);
            BoxElement::registerElement(&GroupBoxFillOnly, groupBox, isVariant<24>);
            BoxElement::registerElement(&GroupBoxPadded, groupBox, isVariant<25>);
            BoxElement::registerElement(&GroupBoxBare, groupBox, isVariant<26>);
        }

        // ----------------------------------------------------------------------------------
        // F-within-tag
        // ----------------------------------------------------------------------------------

        TEST_F(BoxFoundationTwinTests, Selectors_WithinTaggedLooksAtEveryAncestor)
        {
            QWidget top;
            QWidget middle(&top);
            QWidget leaf(&middle);

            EXPECT_FALSE(Selectors::withinTagged(&leaf, "SceneTag"));
            middle.setProperty("class", "Other SceneTag Last");
            EXPECT_TRUE(Selectors::withinTagged(&leaf, "SceneTag"));                 // a whole word of the class property
            EXPECT_FALSE(Selectors::withinTagged(&leaf, "Scene"));                   // not a part of one
            EXPECT_FALSE(Selectors::withinTagged(&middle, "SceneTag"));              // the widget's own tag is not an ancestor's
        }

        //! What QStyleSheetStyle::repolish does to a widget and what is in it (updateObjects): the style's polish again, and the style
        //! change that has a QFrame read its frame width. unpolish and polish alone leave the frame width as it was.
        void repolishAsTheSheetDoes(QWidget& widget)
        {
            States::repolish(widget);
            QEvent styleChange(QEvent::StyleChange);
            QList<QWidget*> widgets = widget.findChildren<QWidget*>();
            widgets.prepend(&widget);
            for (QWidget* each : widgets)
            {
                QApplication::sendEvent(each, &styleChange);
            }
        }

        //! A frame inside a row (one level below it) takes the rule of its row while the row has the class tag: the sheet is
        //! `.SceneTag QFrame { ... }` and a tag that is set after the polish is read when the row is polished again, and what is in it
        //! with it (Style::addClass polishes the row; the site polishes what is below it, States::repolish)
        Row sceneTagRow()
        {
            State tagOn{ "class SceneTag",
                [](QWidget& widget)
                {
                    QWidget* row = widget.parentWidget();
                    if (StyleManager::flatStylingActive())
                    {
                        Style::addClass(row, QStringLiteral("SceneTag"));
                        States::repolish(*row);
                    }
                    else
                    {
                        row->setProperty("class", "SceneTag");
                        repolishAsTheSheetDoes(*row);
                    }
                },
                [](QWidget& widget)
                {
                    QWidget* row = widget.parentWidget();
                    if (StyleManager::flatStylingActive())
                    {
                        Style::removeClass(row, QStringLiteral("SceneTag"));
                        States::repolish(*row);
                    }
                    else
                    {
                        row->setProperty("class", "");
                        repolishAsTheSheetDoes(*row);
                    }
                } };
            return Row{
                .m_site = "F-within-tag: .SceneTag QFrame",
                .m_make = [](QWidget* parent) -> QWidget*
                {
                    auto* target = new QFrame(parent);
                    target->setObjectName(QStringLiteral("tagTarget"));
                    return target;
                },
                .m_chain = { { .m_name = "tagRow" } },
                .m_sheets = { { "the selector `.tag W` (a class tag of an ancestor)", SetOn::Application,
                    ".SceneTag QFrame { margin: 3px; padding: 2px; border: 2px solid #33AA77; background-color: #555555; }" } },
                .m_register = &registerFoundationElements,
                .m_states = { tagOn },
                .m_compare = Compare::Geometry | Compare::Pixels,
                .m_size = QSize(80, 40),
                .m_mayDrawNothing = true,
            };
        }

        TEST_F(BoxFoundationTwinTests, WithinTagged_AFrameTakesTheRuleOfARowWithTheClassTag)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(sceneTagRow(), m_context));
        }

        TEST_F(BoxFoundationTwinTests, WithinTagged_AFrameNoElementMatchesStaysPlainWhereTheRuleReachedIt)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            // The same row with a frame that none of the elements matches: it takes the rule in the legacy twin and stays plain in the
            // flat one, which the twins show; so the twin of the row does see the rule reach the frame
            Row row = sceneTagRow();
            row.m_make = [](QWidget* parent) -> QWidget*
            {
                auto* target = new QFrame(parent);
                target->setObjectName(QStringLiteral("unmatchedTarget"));
                return target;
            };
            row.m_expectedDifference = "geometry.contentsRect";
            EXPECT_TRUE(check(row, m_context));
        }

        // ----------------------------------------------------------------------------------
        // F-reresolve
        // ----------------------------------------------------------------------------------

        //! A state the site sets through its legacy sheet in the legacy GUI and through a class tag in the flattened one (the tag is
        //! laid on as the state is entered and taken off as it is left)
        State sheetState(const char* name, const char* sheet, const char* tag)
        {
            return State{ name,
                [sheet, tag](QWidget& widget)
                {
                    if (StyleManager::flatStylingActive())
                    {
                        Style::addClass(&widget, QString::fromLatin1(tag));
                    }
                    else
                    {
                        widget.setStyleSheet(QString::fromLatin1(sheet));
                    }
                },
                [tag](QWidget& widget)
                {
                    if (StyleManager::flatStylingActive())
                    {
                        Style::removeClass(&widget, QString::fromLatin1(tag));
                    }
                    else
                    {
                        widget.setStyleSheet(QString());
                    }
                } };
        }

        //! The line edit of PropertyColorCtrl, under the application's real sheet: red text while the hex text is not valid
        //! (PropertyColorCtrl.cpp:338 `setStyleSheet("color: red;")`, :342 `setStyleSheet("")`)
        Row invalidHexRow()
        {
            return Row{
                .m_site = "F-reresolve PropertyColorCtrl.cpp:338, :342 line edit colour",
                .m_make = [](QWidget* parent) -> QWidget*
                {
                    auto* edit = new QLineEdit(QStringLiteral("#FF00F"), parent);
                    edit->setObjectName(QStringLiteral("m_colorEdit"));
                    return edit;
                },
                .m_realApplicationSheet = true,
                .m_register = &registerFoundationElements,
                .m_states = { sheetState("invalid hex", "color: red;", "InvalidHex"), States::disabled() },
                .m_compare = Compare::Geometry | Compare::Fonts | Compare::Palette | Compare::Pixels,
                .m_roles = { QPalette::Text, QPalette::PlaceholderText, QPalette::Base },
                .m_size = QSize(120, 20),
            };
        }

        TEST_F(BoxFoundationTwinTests, Reresolve_ALineEditTurnedRedAndBackIsTheLineEditOfTheSheet)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(invalidHexRow(), m_context));
        }

        TEST_F(BoxFoundationTwinTests, Reresolve_ALineEditDrawsItsOwnColoursAgainWhenTheStateEnds)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            RungScope flat(Rung::Flat);
            Context::registerOnce(&registerFoundationElements);

            QWidget root;
            auto* edit = new QLineEdit(QStringLiteral("#FF00F"), &root);
            edit->setObjectName(QStringLiteral("stateEdit"));
            root.setStyle(m_context.flatStyle());
            edit->setStyle(m_context.flatStyle());
            root.setAttribute(Qt::WA_DontShowOnScreen);
            root.show();
            edit->ensurePolished();
            const QPalette before = edit->palette();

            Style::addClass(edit, QStringLiteral("InvalidHex"));
            for (const QPalette::ColorGroup group : { QPalette::Active, QPalette::Inactive, QPalette::Disabled })
            {
                EXPECT_EQ(edit->palette().color(group, QPalette::Text), QColor(Qt::red));
            }
            Style::addClass(edit, QStringLiteral("InvalidHex"));                        // the tag it has changes nothing
            EXPECT_EQ(edit->property("class").toString(), QStringLiteral("InvalidHex"));
            Style::removeClass(edit, QStringLiteral("InvalidHex"));
            // The roles a line edit draws with: the flattened Style builds them again at every polish (applyFlattenedTextEntryPalette).
            // The other roles the text element set keep its colour (HEAD gives no palette back; a line edit does not draw with them)
            for (const QPalette::ColorGroup group : { QPalette::Active, QPalette::Inactive, QPalette::Disabled })
            {
                for (const QPalette::ColorRole role : { QPalette::Text, QPalette::PlaceholderText, QPalette::Base })
                {
                    EXPECT_EQ(edit->palette().color(group, role), before.color(group, role)) << group << " " << role;
                }
            }
        }

        TEST_F(BoxFoundationTwinTests, Reresolve_TheClassTextAloneResolvesNothing)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            RungScope flat(Rung::Flat);
            Context::registerOnce(&registerFoundationElements);

            QWidget root;
            auto* edit = new QLineEdit(QStringLiteral("#FF00F"), &root);
            edit->setObjectName(QStringLiteral("aloneEdit"));
            root.setStyle(m_context.flatStyle());
            edit->setStyle(m_context.flatStyle());
            root.setAttribute(Qt::WA_DontShowOnScreen);
            root.show();
            edit->ensurePolished();
            const QColor before = edit->palette().color(QPalette::Active, QPalette::Text);

            // setProperty reads nothing again: the element is resolved at polish
            edit->setProperty("class", "InvalidHex");
            EXPECT_EQ(edit->palette().color(QPalette::Active, QPalette::Text), before);
            edit->setProperty("class", "");

            // The tag, laid on with the polish after it, is read
            Style::addClass(edit, QStringLiteral("InvalidHex"));
            EXPECT_EQ(edit->palette().color(QPalette::Active, QPalette::Text), QColor(Qt::red));
        }

        //! The label of BlendSpaceMotionContainerHandler (#m_labelMotion): a red border while its motion is invalid
        //! (BlendSpaceMotionContainerHandler.cpp:152) and none when it is valid (:157), under the label's own rule (Text.qss:89-95)
        Row motionLabelRow()
        {
            return Row{
                .m_site = "F-reresolve BlendSpaceMotionContainerHandler.cpp:152, :157 label border",
                .m_make = [](QWidget* parent) -> QWidget*
                {
                    auto* label = new QLabel(QStringLiteral("Motion"), parent);
                    label->setObjectName(QStringLiteral("m_labelMotion"));
                    return label;
                },
                .m_realApplicationSheet = true,
                .m_register = &registerFoundationElements,
                .m_states = {
                    sheetState("invalid motion", "#m_labelMotion { border: 1px solid red; }", "MotionInvalid"),
                    sheetState("valid motion", "#m_labelMotion { border: none; }", "MotionValid"),
                },
                .m_compare = Compare::Geometry | Compare::Fonts | Compare::Palette | Compare::Pixels,
                .m_roles = { QPalette::WindowText, QPalette::Text },
                .m_size = QSize(100, 24),
                .m_mayDrawNothing = true,           // idle: a label with no fill, no border and the text of the colour of the window
            };
        }

        TEST_F(BoxFoundationTwinTests, Reresolve_ALabelBorderTurnedOnAndOffIsTheBorderOfTheSheet)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(motionLabelRow(), m_context));
        }

        //! The widget of each family that has an element, in a window of the flat style, with the property that selects the probe element
        TEST_F(BoxFoundationTwinTests, Reresolve_EveryFamilyOfElementsResolvesAgainWhenTheStateChanges)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            RungScope flat(Rung::Flat);
            Context::registerOnce(&registerFoundationElements);

            QWidget root;
            auto* button = new QPushButton(QStringLiteral("Button"), &root);
            auto* frame = new QFrame(&root);
            auto* checkBox = new QCheckBox(QStringLiteral("Check"), &root);
            auto* tabs = new QTabBar(&root);
            tabs->addTab(QStringLiteral("Tab"));
            auto* menu = new QMenu(&root);
            auto* view = new QListView(&root);
            auto* label = new QLabel(QStringLiteral("Label"), &root);
            label->setObjectName(QStringLiteral("probeLabel"));
            for (QWidget* each : { static_cast<QWidget*>(&root), static_cast<QWidget*>(button), static_cast<QWidget*>(frame),
                     static_cast<QWidget*>(checkBox), static_cast<QWidget*>(tabs), static_cast<QWidget*>(menu), static_cast<QWidget*>(view),
                     static_cast<QWidget*>(label) })
            {
                each->setStyle(m_context.flatStyle());
            }
            root.setAttribute(Qt::WA_DontShowOnScreen);
            root.show();
            QApplication::processEvents();
            for (QWidget* each : root.findChildren<QWidget*>())
            {
                each->ensurePolished();
            }

            // What each family has resolved for the widget now
            const auto resolved = [&]()
            {
                struct Seen
                {
                    bool m_box, m_frame, m_indicator, m_tab, m_menu, m_view, m_text;
                };
                return Seen{ BoxElement::forWidget(button) == &ProbeBox, FrameElement::forWidget(frame) == &ProbeFrame,
                    IndicatorElement::forWidget(checkBox) == &ProbeIndicator, TabElement::forTabBar(tabs) == &ProbeTab,
                    &MenuElement::forWidget(menu) == &ProbeMenu,
                    ItemViewElement::forWidget(view) && ItemViewElement::forWidget(view)->m_itemHeight == ProbeView.m_itemHeight,
                    label->palette().color(QPalette::Active, QPalette::WindowText) == ProbeText.m_color };
            };
            const auto expectAll = [&](bool on, const char* when)
            {
                const auto seen = resolved();
                EXPECT_EQ(seen.m_box, on) << "box, " << when;
                EXPECT_EQ(seen.m_frame, on) << "frame, " << when;
                EXPECT_EQ(seen.m_indicator, on) << "indicator, " << when;
                EXPECT_EQ(seen.m_tab, on) << "tab, " << when;
                EXPECT_EQ(seen.m_menu, on) << "menu, " << when;
                EXPECT_EQ(seen.m_view, on) << "item view, " << when;
                EXPECT_EQ(seen.m_text, on) << "text, " << when;
            };

            expectAll(false, "before the state");
            const QPalette labelBefore = label->palette();
            const QSize buttonMinimumBefore = button->minimumSize();

            // The class text alone is not read. A label aside: the flattened Style polishes a label again when its class text changes
            // (Style.cpp, QEvent::DynamicPropertyChange, as at HEAD)
            for (QWidget* each : { static_cast<QWidget*>(button), static_cast<QWidget*>(frame), static_cast<QWidget*>(checkBox),
                     static_cast<QWidget*>(tabs), static_cast<QWidget*>(menu), static_cast<QWidget*>(view) })
            {
                each->setProperty("class", "ProbeOn");
            }
            expectAll(false, "the class text set and nothing polished again");

            // The one call reaches the family of every widget
            for (QWidget* each : { static_cast<QWidget*>(button), static_cast<QWidget*>(frame), static_cast<QWidget*>(checkBox),
                     static_cast<QWidget*>(tabs), static_cast<QWidget*>(menu), static_cast<QWidget*>(view), static_cast<QWidget*>(label) })
            {
                each->setProperty("class", "");
                Style::addClass(each, QStringLiteral("ProbeOn"));
            }
            expectAll(true, "the tag laid on");
            EXPECT_NE(button->minimumSize(), buttonMinimumBefore);
            EXPECT_NE(label->palette().color(QPalette::Active, QPalette::WindowText),
                labelBefore.color(QPalette::Active, QPalette::WindowText));

            for (QWidget* each : { static_cast<QWidget*>(button), static_cast<QWidget*>(frame), static_cast<QWidget*>(checkBox),
                     static_cast<QWidget*>(tabs), static_cast<QWidget*>(menu), static_cast<QWidget*>(view), static_cast<QWidget*>(label) })
            {
                Style::removeClass(each, QStringLiteral("ProbeOn"));
            }
            expectAll(false, "the tag taken off");

            // What the state laid on the widget besides its element is undone when the widget is polished again: the limits of a box
            // element; and the label's idle element gives its colour again (every role of every colour group, not only the one the
            // element is read by), as a state that ends on a colour of its own does (D.25)
            EXPECT_EQ(button->minimumSize(), buttonMinimumBefore);
            for (const QPalette::ColorGroup group : { QPalette::Active, QPalette::Inactive, QPalette::Disabled })
            {
                for (const QPalette::ColorRole role :
                    { QPalette::Text, QPalette::WindowText, QPalette::ButtonText, QPalette::PlaceholderText, QPalette::Base })
                {
                    EXPECT_EQ(label->palette().color(group, role), labelBefore.color(group, role)) << group << " " << role;
                }
            }
        }

        // ----------------------------------------------------------------------------------
        // F-dashed-edge
        // ----------------------------------------------------------------------------------

        Row dashedRow(const char* site, int variant, const char* sheet, const QSize& size)
        {
            return Row{
                .m_site = site,
                .m_make = [variant](QWidget* parent) -> QWidget*
                {
                    auto* frame = new QFrame(parent);
                    frame->setProperty("foundationVariant", variant);
                    return frame;
                },
                .m_sheets = { { "ParameterCreateEditWidget.cpp:83 (the dash widths of the foundation)", SetOn::Widget, sheet } },
                .m_register = &registerFoundationElements,
                .m_states = { States::disabled() },
                .m_compare = Compare::Geometry | Compare::Pixels,
                .m_size = size,
            };
        }

        TEST_F(BoxFoundationTwinTests, DashedEdge_ThePreviewFrameOfTheParameterEditorIsTheFrameOfTheSheet)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(dashedRow("F-dashed-edge C123 2px dashed", 1,
                "QFrame { border: 2px dashed #979797; background-color: #85858580; }", QSize(120, 60)), m_context));
        }

        TEST_F(BoxFoundationTwinTests, DashedEdge_ThePreviewFrameOfTheSiteIsTheFrameOfTheSheetUnderTheApplicationSheet)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            // The frame as the site makes it (ParameterCreateEditWidget.cpp:80-85): a Box frame named previewFrame, the rule behind
            // its selector, under the application's rules (BaseStyleSheet.qss:41-48 gives a Box frame a fill of its own)
            Row row = dashedRow("F-dashed-edge C123 QFrame#previewFrame under the application sheet", 1,
                "QFrame#previewFrame { border: 2px dashed #979797; background-color: #85858580; }", QSize(120, 60));
            row.m_make = [](QWidget* parent) -> QWidget*
            {
                auto* frame = new QFrame(parent);
                frame->setFrameShape(QFrame::Box);
                frame->setObjectName(QStringLiteral("previewFrame"));
                frame->setProperty("foundationVariant", 1);
                return frame;
            };
            row.m_realApplicationSheet = true;
            EXPECT_TRUE(check(row, m_context));
        }

        TEST_F(BoxFoundationTwinTests, DashedEdge_TheDashesOfEveryWidthAndRadiusAreTheDashesOfTheSheet)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(dashedRow("F-dashed-edge 1px dashed (dotted)", 2,
                "QFrame { border: 1px dashed #979797; background-color: #85858580; }", QSize(70, 40)), m_context));
            EXPECT_TRUE(check(dashedRow("F-dashed-edge 3px dashed, radius 6px", 3,
                "QFrame { border: 3px dashed #979797; border-radius: 6px; background-color: #85858580; }", QSize(90, 50)), m_context));
            EXPECT_TRUE(check(dashedRow("F-dashed-edge 2px dashed, radius 8px", 4,
                "QFrame { border: 2px dashed #979797; border-radius: 8px; background-color: #85858580; }", QSize(64, 44)), m_context));
        }

        TEST_F(BoxFoundationTwinTests, DashedEdge_TheEdgesOfDifferentColoursMeetInTheOrderOfTheSheet)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(dashedRow("F-dashed-edge four dashed edges of four colours", 5,
                "QFrame { border-top: 2px dashed #979797; border-right: 2px dashed #E25243; border-bottom: 2px dashed #3A7BD5;"
                " border-left: 2px dashed #7BD53A; }", QSize(100, 50)), m_context));
        }

        TEST_F(BoxFoundationTwinTests, DashedEdge_ASolidEdgeBesideADashedOneIsMitredAsTheSheetMitresIt)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(dashedRow("F-dashed-edge dashed top and bottom, solid sides", 6,
                "QFrame { border-top: 2px dashed #979797; border-right: 2px solid #E25243; border-bottom: 2px dashed #979797;"
                " border-left: 2px solid #E25243; }", QSize(100, 50)), m_context));
            EXPECT_TRUE(check(dashedRow("F-dashed-edge a translucent solid border, which is not painted over at a corner", 7,
                "QFrame { border: 3px solid rgba(226,82,67,128); }", QSize(100, 50)), m_context));
        }

        TEST_F(BoxFoundationTwinTests, DashedEdge_ASolidEdgeIsNotADashedOne)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            // The frame of the site with solid lines in place of the dashes: the twins differ in the pixels, so the twin of a dashed
            // edge does tell a dash from a line
            Row row = dashedRow("F-dashed-edge a solid line against the dashes", 1,
                "QFrame { border: 2px solid #979797; background-color: #85858580; }", QSize(120, 60));
            row.m_expectedDifference = "pixels at ratio 1";
            EXPECT_TRUE(check(row, m_context));
        }

        // ----------------------------------------------------------------------------------
        // F-inset-radius
        // ----------------------------------------------------------------------------------

        Row insetRow(const char* site, int variant, const char* sheet, const QSize& size)
        {
            return Row{
                .m_site = site,
                .m_make = [variant](QWidget* parent) -> QWidget*
                {
                    auto* label = new QLabel(QStringLiteral("Pending Status"), parent);
                    label->setProperty("foundationVariant", variant);
                    return label;
                },
                .m_sheets = { { "LUAEditorMainWindow.cpp:576-580", SetOn::Widget, sheet } },
                .m_register = &registerFoundationElements,
                .m_states = { States::disabled() },
                .m_compare = Compare::Geometry | Compare::Fonts | Compare::Palette | Compare::Pixels,
                .m_roles = { QPalette::WindowText, QPalette::Text, QPalette::ButtonText },
                .m_size = size,
            };
        }

        TEST_F(BoxFoundationTwinTests, InsetRadius_TheStatusLabelOfTheLuaEditorIsTheLabelOfTheSheet)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(insetRow("F-inset-radius C077 inset 1px, radius 8px", 11,
                "background: rgba(192,192,192,255); color: black; border-style: inset;\nborder-width: 1px;\n"
                "border-color: rgba(100,100,100,255);\nborder-radius: 8px;", QSize(110, 24)), m_context));
        }

        TEST_F(BoxFoundationTwinTests, InsetRadius_TheStatusLabelOfTheSiteIsTheLabelOfTheSheetUnderTheApplicationSheet)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            // The label as the site makes it (LUAEditorMainWindow.cpp:573-582): contents margins of 2, an autofilled background, the
            // rule as its own sheet, under the application's rules
            Row row = insetRow("F-inset-radius C077 the label of the site under the application sheet", 30,
                "background: rgba(192,192,192,255); color: black; border-style: inset;\nborder-width: 1px;\n"
                "border-color: rgba(100,100,100,255);\nborder-radius: 8px;", QSize(110, 24));
            row.m_make = [](QWidget* parent) -> QWidget*
            {
                auto* label = new QLabel(QStringLiteral("Pending Status"), parent);
                label->setContentsMargins(2, 2, 2, 2);
                label->setAutoFillBackground(true);
                label->setProperty("foundationVariant", 30);
                return label;
            };
            row.m_realApplicationSheet = true;
            EXPECT_TRUE(check(row, m_context));
        }

        TEST_F(BoxFoundationTwinTests, InsetRadius_WiderBordersAndOtherRadiiHaveTheArcsOfTheSheet)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(insetRow("F-inset-radius inset 3px, radius 8px", 12,
                "background: rgba(192,192,192,255); color: black; border-style: inset; border-width: 3px;"
                " border-color: rgba(100,100,100,255); border-radius: 8px;", QSize(110, 30)), m_context));
            EXPECT_TRUE(check(insetRow("F-inset-radius inset 2px, radius 3px", 13,
                "background: rgba(192,192,192,255); color: black; border-style: inset; border-width: 2px;"
                " border-color: rgba(100,100,100,255); border-radius: 3px;", QSize(110, 30)), m_context));
            EXPECT_TRUE(check(insetRow("F-inset-radius outset 2px, radius 8px", 14,
                "background: rgba(192,192,192,255); color: black; border-style: outset; border-width: 2px;"
                " border-color: rgb(80,120,160); border-radius: 8px;", QSize(110, 30)), m_context));
        }

        TEST_F(BoxFoundationTwinTests, InsetRadius_CornersOfDifferentRadiiAndNoRadiusAtAllHaveTheLinesOfTheSheet)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(insetRow("F-inset-radius inset 1px, top left 10px, bottom right 4px", 15,
                "background: rgba(192,192,192,255); color: black; border-style: inset; border-width: 1px;"
                " border-color: rgba(100,100,100,255); border-top-left-radius: 10px; border-bottom-right-radius: 4px;",
                QSize(110, 30)), m_context));
            EXPECT_TRUE(check(insetRow("F-inset-radius inset 1px, no radius", 16,
                "background: rgba(192,192,192,255); color: black; border-style: inset; border-width: 1px;"
                " border-color: rgba(100,100,100,255);", QSize(110, 30)), m_context));
        }

        TEST_F(BoxFoundationTwinTests, InsetRadius_SquareCornersOfAWideBorderAreMitredAsTheSheetMitresThem)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(insetRow("F-inset-radius inset 2px, no radius", 17,
                "background: rgba(192,192,192,255); color: black; border-style: inset; border-width: 2px;"
                " border-color: rgba(100,100,100,255);", QSize(110, 30)), m_context));
            EXPECT_TRUE(check(insetRow("F-inset-radius outset 3px, no radius", 18,
                "background: rgba(192,192,192,255); color: black; border-style: outset; border-width: 3px;"
                " border-color: rgb(80,120,160);", QSize(110, 30)), m_context));
            EXPECT_TRUE(check(insetRow("F-inset-radius inset 2px, top left 10px, bottom right 4px, the other corners square", 19,
                "background: rgba(192,192,192,255); color: black; border-style: inset; border-width: 2px;"
                " border-color: rgba(100,100,100,255); border-top-left-radius: 10px; border-bottom-right-radius: 4px;",
                QSize(110, 30)), m_context));
        }

        TEST_F(BoxFoundationTwinTests, InsetRadius_AnInsetBorderOfOneColourOnAllSidesIsNotTheInsetBorder)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            // The sheet's inset border against an element that draws the colour on every side: the twins differ, so the per-side
            // colours are what the twin judges
            Row row = insetRow("F-inset-radius an inset border drawn flat", 20,
                "background: rgba(192,192,192,255); color: black; border-style: inset; border-width: 1px;"
                " border-color: rgba(100,100,100,255); border-radius: 8px;", QSize(110, 24));
            row.m_expectedDifference = "pixels at ratio 1";
            EXPECT_TRUE(check(row, m_context));
        }

        // ----------------------------------------------------------------------------------
        // F-groupbox-frame
        // ----------------------------------------------------------------------------------

        Row groupBoxRow(
            const char* site, int variant, const char* sheet, const char* title, const QSize& size, bool applicationSheet = false)
        {
            return Row{
                .m_site = site,
                .m_make = [variant, title](QWidget* parent) -> QWidget*
                {
                    auto* box = new QGroupBox(QString::fromUtf8(title), parent);
                    box->setProperty("foundationVariant", variant);
                    return box;
                },
                .m_sheets = { { "KinematicDescriptionDialog.cpp:82-94", SetOn::Widget, sheet } },
                .m_realApplicationSheet = applicationSheet,
                .m_register = &registerFoundationElements,
                .m_states = { States::hover(), States::disabled() },
                .m_compare = Compare::Geometry | Compare::Fonts | Compare::Palette | Compare::Pixels,
                .m_roles = { QPalette::WindowText, QPalette::Text, QPalette::ButtonText },
                .m_size = size,
                .m_mayDrawNothing = title[0] == 0,      // a box with no title and no border is one colour from edge to edge
            };
        }

        TEST_F(BoxFoundationTwinTests, GroupBox_ABorderlessBoxWithAFillDrawsNoFrame)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(groupBoxRow("F-groupbox-frame C180 fill, border: none", 22,
                "background-color: rgb(51, 51, 51); border: none;", "", QSize(160, 70)), m_context));
            EXPECT_TRUE(check(groupBoxRow("F-groupbox-frame C180 fill, border: none, with a title", 22,
                "background-color: rgb(51, 51, 51); border: none;", "Kinematic", QSize(160, 70)), m_context));
        }

        TEST_F(BoxFoundationTwinTests, GroupBox_ABoxWithABorderDrawsItsBorderAndNotTheFrame)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(groupBoxRow("F-groupbox-frame C181 fill, border: 1px solid", 21,
                "background-color: rgb(51, 51, 51); border: 1px solid rgb(30, 112, 235);", "", QSize(160, 70)), m_context));
            EXPECT_TRUE(check(groupBoxRow("F-groupbox-frame C181 fill, border: 1px solid, with a title", 21,
                "background-color: rgb(51, 51, 51); border: 1px solid rgb(30, 112, 235);", "Kinematic", QSize(160, 70)), m_context));
        }

        TEST_F(BoxFoundationTwinTests, GroupBox_ABoxWithNothingButBorderNoneDrawsNoFrame)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(groupBoxRow("F-groupbox-frame U093 border: none", 23, "border: none", "", QSize(160, 70)), m_context));
            EXPECT_TRUE(check(groupBoxRow("F-groupbox-frame U093 border: none, with a title", 23, "border: none", "Kinematic",
                QSize(160, 70)), m_context));
        }

        TEST_F(BoxFoundationTwinTests, GroupBox_ABoxWithAFillAndTheNativeBorderKeepsTheFrameOfTheStyle)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(groupBoxRow("F-groupbox-frame fill, native border", 24, "background-color: rgb(51, 51, 51);", "Kinematic",
                QSize(160, 70)), m_context));
        }

        TEST_F(BoxFoundationTwinTests, GroupBox_ABoxWithPaddingAndARoundBorderHasTheBoxOfTheSheet)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(groupBoxRow("F-groupbox-frame fill, 2px border, radius 6px, padding", 25,
                "background-color: rgb(51, 51, 51); border: 2px solid rgb(30, 112, 235); border-radius: 6px;"
                " padding: 12px 6px 4px 6px;", "Kinematic", QSize(160, 70)), m_context));
        }

        TEST_F(BoxFoundationTwinTests, GroupBox_UnderTheApplicationSheetARuleWithNoBoxAndNoFillStillDrawsTheGroupBoxThroughTheSheet)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            // The universal rule of the application (BaseStyleSheet.qss:15-21: a background, a colour and a font) is a drawable, a
            // palette and a font of every group box, so the sheet style never leaves one to the base style
            // (QStyleSheetStyle::drawComplexControl, CC_GroupBox): the frame is kept out of exactly the rect of the title and the
            // title is centered in it, where Fusion cuts the frame wider and aligns the title to the left. A group box under an
            // element draws as the sheet drew it, whatever the element carries.
            for (const char* title : { "", "Kinematic" })
            {
                EXPECT_TRUE(check(groupBoxRow("F-groupbox-frame application sheet, native border, no fill", 26,
                    "QGroupBox { min-width: 100px; min-height: 40px; }", title, QSize(160, 70), true), m_context))
                    << "title '" << title << "'";
            }
        }

        TEST_F(BoxFoundationTwinTests, GroupBox_TheFrameOfTheStyleIsWhatTheTwinSeesWithoutTheElement)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            // The sheet's box against a group box no element covers: the style's own frame is drawn and the box is its own, so the twins
            // differ in the pixels
            Row row = groupBoxRow("F-groupbox-frame no element", 99, "background-color: rgb(51, 51, 51); border: none;", "Kinematic",
                QSize(160, 70));
            row.m_expectedDifference = "pixels at ratio 1";
            EXPECT_TRUE(check(row, m_context));
        }
    } // namespace BoxFoundationTwinTestsInternal
} // namespace AzQtComponents
