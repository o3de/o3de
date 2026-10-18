/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/ControlElements.h>
#include <AzQtComponents/Components/Elements/TextElement.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/StyledDockWidget.h>

#include <QBrush>
#include <QColor>
#include <QDockWidget>
#include <QFrame>
#include <QHashFunctions>
#include <QIcon>
#include <QMargins>
#include <QPainter>
#include <QPushButton>
#include <QRect>
#include <QSize>
#include <QString>
#include <QStyleOption>
#include <QStyleOptionButton>
#include <QToolBar>
#include <QToolButton>
#include <QVariant>
#include <QWidget>

namespace AzQtComponents
{
    //! Everything private to this file. A named namespace, not an anonymous one: a unity build joins this file with others
    //! into one translation unit, where anonymous namespaces merge (TextElement.cpp does the same).
    namespace ControlElementsInternal
    {
        // ------------------------------------------------------------------------------
        // The frame of the buttons of a dock's tab bar: TitleBar.qss over its own `*` rule
        // ------------------------------------------------------------------------------

        //! AzQtComponents--TitleBar[drawAsTabBar="true"] #tabButtons (TitleBar.qss:54): 3px of margin below the frame that holds
        //! the minimize, maximize and close buttons of a dock widget drawn as a tab bar (Titlebar.cpp:141-146). The frame is as
        //! tall as the title bar and the layout centres the 26px buttons in what the margin leaves, 27px: the buttons sit at the
        //! top, where centring them in the whole frame put them 2px lower. No border (a QFrame without a frame has the native
        //! width 0, qstylesheetstyle.cpp:1721-1755); the fill is TitleBar.qss:26-29 `AzQtComponents--TitleBar *`.
        //! The selector matches the 242 frames n2 records under such a title bar (the same frames recur in its screens). Native gave
        //! the margin to all 56 that hold buttons (the 18 buttons of the parity rows sit at y 0 there) and to 86 of the 186 empty
        //! ones. The other 100 are empty, 0 wide, in the title bars of docked dock widgets: they were polished before their title
        //! bar set drawAsTabBar (TitleBar::polish) and kept a rule without the margin. That order is no data and every frame gets
        //! the margin here: an empty frame shows nothing, and only its hint differs (3 high, Native's 0). OPEN.
        //! The frame is polished before its title bar sets the property (the stacked layout shows its first page in the title bar's
        //! constructor), so TitleBar::setDrawAsTabBar resolves the frame's element again when the property changes (Titlebar.cpp).
        //! TOKENIZE: DockTabButtonsMargin
        const BoxElement TabButtons{
            .m_name = "Global.Box.TabButtons",
            .m_margin = QMargins(0, 0, 0, 3),                       // TitleBar.qss:56
            .m_idle = {
                .m_fill = QBrush(Qt::transparent),                  // TitleBar.qss:28
            },
        };

        // ------------------------------------------------------------------------------
        // The close button of a filter tag: FilteredSearchWidget.qss over PushButton.qss and its own QPushButton rule
        // ------------------------------------------------------------------------------

        //! AzQtComponents--FilterCriteriaButton QPushButton#closeTag (FilteredSearchWidget.qss:37) and the same rule of
        //! FilterTextButton (:90): a flat button with the close glyph, 16px wide and at most 16 high, no margin and no padding
        //! (:43-44), and no border: FilteredSearchWidget.qss:15-19 gave every QPushButton of the search widget `border: none`
        //! (a border style of its own, so the limits do not count the native border of a push button). Native min height is
        //! none: only the width is declared at least (:47). n2 holds 28 of these buttons, all of a FilterCriteriaButton in a
        //! FilteredSearchWidget (Gallery, Material Editor, Pass Canvas); the FilterTextButton rule (:90) repeats the rule and no
        //! capture has such a button: UNVERIFIED.
        //! TOKENIZE: TagCloseButtonSize, TagCloseIcon
        const BoxElement CloseTag{
            .m_name = "Global.Box.CloseTag",
            .m_hasBox = true,                                       // margin and padding of 0: FilteredSearchWidget.qss:43-44
            .m_nativeBorder = false,                                // FilteredSearchWidget.qss:18
            .m_minSize = QSize(16, -1),                             // FilteredSearchWidget.qss:47
            .m_maxSize = QSize(16, 16),                             // FilteredSearchWidget.qss:46, :45
            .m_idle = {
                .m_fill = QBrush(Qt::transparent),                  // FilteredSearchWidget.qss:42
            },
            .m_properties = {
                { "flat", true },                                   // FilteredSearchWidget.qss:41
                { "icon", QString(":/stylesheet/img/close_x.svg") },// FilteredSearchWidget.qss:39
                { "iconSize", QSize(10, 10) },                      // FilteredSearchWidget.qss:40
            },
        };

        // ------------------------------------------------------------------------------
        // The panel of a toolbar: ToolBar.qss
        // ------------------------------------------------------------------------------

        const QColor ToolBarFill(0x44, 0x44, 0x44);                 // ToolBar.qss:20. TOKENIZE: ToolBarFill
        const QColor ToolBarBottomLine(0x11, 0x11, 0x11);           // ToolBar.qss:33. TOKENIZE: ToolBarBottomLine

        //! QToolBar { background-color: #444444; border: 0 none } (ToolBar.qss:15-22): a border of its own, none, so no bevel of the
        //! base style (CE_ToolBar: qstylesheetstyle.cpp:3815-3830 drew the background and the border and nothing else).
        const BoxElement ToolBarPanel{
            .m_name = "Global.Box.ToolBar",
            .m_nativeBorder = false,                                // ToolBar.qss:22
            .m_idle = {
                .m_fill = QBrush(ToolBarFill),                      // ToolBar.qss:20
            },
        };

        //! MainWindow QToolBar { border-bottom: 2px solid #111111 } (ToolBar.qss:31-34), the edge over the rule above; a toolbar
        //! of a DockMainWindow has no border at all (:36-39, later in the sheet, as specific). n2 shows both: the 22 horizontal
        //! toolbars of the Editor's main window with the line (device rows #2a #11 #11 #2a at a half pixel), the others without. A
        //! vertical toolbar under a MainWindow has the line at its bottom by the rule, UNVERIFIED.
        const BoxElement ToolBarPanelUnderMainWindow{
            .m_name = "Global.Box.ToolBar.MainWindow",
            .m_nativeBorder = false,                                // ToolBar.qss:22, :33
            .m_idle = {
                .m_fill = QBrush(ToolBarFill),                      // ToolBar.qss:20
                .m_edges = { {}, {}, { 2, ToolBarBottomLine }, {} },// ToolBar.qss:33 (top, right, bottom, left)
            },
        };

        // ------------------------------------------------------------------------------
        // The button of a toolbar that shows what does not fit it: ToolBar.qss:139-142
        // ------------------------------------------------------------------------------

        //! QToolBarExtension#qt_toolbar_ext_button { qproperty-icon: url(:/stylesheet/img/hidden-icons.svg) } (ToolBar.qss:139-142): the
        //! double caret of the button at the end of a toolbar that is too narrow for its actions. The sheet set the icon when it polished the
        //! button, as it sets the properties of any widget (qstylesheetstyle.cpp:2618-2678), over the small caret QToolBarExtension takes from
        //! the base style (QToolBarExtension::setOrientation, standardIcon SP_ToolBarHorizontalExtensionButton). The button is a QToolButton
        //! with the style's own look otherwise, and 12px wide (Style::pixelMetric PM_ToolBarExtensionExtent), so the icon is fitted into the
        //! 10px of its contents (the 16px icon size bounded by the rect, qcommonstyle.cpp:1701). The element has no box. n2 holds 12 of
        //! these buttons, all 12x22 (Material Canvas 9, Pass Canvas 3): the ink of Native's caret is 15 x 16 device px at 150%, where the base
        //! style's is 12 x 9 (Material Canvas viewport toolbar, n2 and f5).
        const BoxElement ToolBarExtension{
            .m_name = "Global.Box.ToolBarExtension",
            .m_properties = {
                { "icon", QString(":/stylesheet/img/hidden-icons.svg") },   // ToolBar.qss:141
            },
        };

        // ------------------------------------------------------------------------------
        // The handle of a movable toolbar: ToolBar.qss:81-137, the image and the padding of the most specific rule
        // ------------------------------------------------------------------------------

        //! The images of the handle (ToolBar.qss:83, :88), and their size: the sheet read it from the file (qcssparser.cpp:1461-1467,
        //! tear.svg:2 `width="2px" height="14px"`, tear-vertical.svg:2 `width="24px" height="6px"`).
        //! TOKENIZE: ToolBarHandleImage
        constexpr const char* HandleImageHorizontal = ":/stylesheet/img/UI20/tear.svg";
        constexpr const char* HandleImageVertical = ":/stylesheet/img/UI20/tear-vertical.svg";
        const QSize HandleImageSizeHorizontal(2, 14);
        const QSize HandleImageSizeVertical(24, 6);

        struct HandleBox
        {
            const char* m_image;
            QSize m_imageSize;
            QMargins m_padding;                                     //!< left, top, right, bottom
        };

        //! The handle rule that wins for a toolbar of the look its class tags give it: of the rules whose selectors hold, the most
        //! specific, and of equally specific ones the later. Horizontal: MainToolBar (+ IconNormal or IconLarge) 1px 4px
        //! (:92-95, :103-107), IconLarge 4px 3px (:116-119), else 2px 4px (:127-131, IconNormal is the same rule). Vertical: a
        //! MainToolBar that is IconLarge 2px 1px (:97-100), all the others 2px -2px (:109-113, :121-124, :133-136).
        //! The 34 handles of n2 (LuaIDE, a Gallery dock window, Graph Canvas) are all of a plain horizontal toolbar: the other looks and
        //! the vertical handle are carried from the rules, UNVERIFIED (equal to the sheet style's in the tests).
        HandleBox handleBox(const QWidget* toolbar, bool horizontal)
        {
            const bool main = TextElement::tagged(toolbar, "MainToolBar");
            const bool large = TextElement::tagged(toolbar, "IconLarge");
            if (horizontal)
            {
                if (main)
                {
                    return { HandleImageHorizontal, HandleImageSizeHorizontal, QMargins(4, 1, 4, 1) };
                }
                return { HandleImageHorizontal, HandleImageSizeHorizontal, large ? QMargins(3, 4, 3, 4) : QMargins(4, 2, 4, 2) };
            }
            return { HandleImageVertical, HandleImageSizeVertical, (main && large) ? QMargins(1, 2, 1, 2) : QMargins(-2, 2, -2, 2) };
        }

        // ------------------------------------------------------------------------------
        // The buttons of a segment bar: SegmentControl.qss:14-77 over the '*' rule of BaseStyleSheet.qss:15-21
        // ------------------------------------------------------------------------------

        // TOKENIZE: SegmentFill, SegmentHoverFill, SegmentCheckedFill, SegmentLine, SegmentBorderWidth, SegmentPadding, SegmentContentHeight,
        // SegmentMinContentWidth, SegmentOverlap, SegmentRadius
        const QColor SegmentFill(0x33, 0x33, 0x33);                 // SegmentControl.qss:16
        const QColor SegmentHoverFill(0x44, 0x44, 0x44);            // SegmentControl.qss:76 (:hover:!checked, :pressed:!checked)
        const QColor SegmentCheckedFill(0x55, 0x55, 0x55);          // SegmentControl.qss:70 (:checked)
        const QColor SegmentLine(0x22, 0x22, 0x22);                 // SegmentControl.qss:17 (border-color, every state)
        constexpr int SegmentBorderWidth = 1;                       // SegmentControl.qss:19
        constexpr int SegmentPadding = 16;                          // SegmentControl.qss:31-32 (left and right)
        constexpr int SegmentContentHeight = 28;                    // SegmentControl.qss:23
        constexpr int SegmentMinContentWidth = 68;                  // SegmentControl.qss:29
        constexpr int SegmentOverlap = 1;                           // SegmentControl.qss:37, :54 (margin-left -1px), :65 (margin-top -1px)
        constexpr int SegmentRadius = 3;                            // SegmentControl.qss:40-41, :46-47

        //! Where a button sits in its bar. SegmentBar::updateTabs tags every button TabOne, TabFirst, TabMiddle or TabLast (SegmentBar.cpp:326-348)
        //! and the bar parents them to an inner widget (SegmentBar.cpp:283-295): the bar, which owns the orientation, is an ancestor of
        //! the button, not its parent (the selector `AzQtComponents--SegmentBar QAbstractButton` is a descendant selector too).
        struct SegmentPlace
        {
            bool m_isButton = false;
            QWidget* m_bar = nullptr;   //!< the SegmentBar the button is in (null for a widget that is no button of a bar)
            bool m_vertical = false;    //!< the bar's orientation is Qt::Vertical ([orientation="2"], SegmentControl.qss:57-66)
            bool m_first = false;       //!< TabFirst
            bool m_last = false;        //!< TabLast
            bool m_one = false;         //!< TabOne: the base rule alone

            //! TabMiddle and TabLast have margin-left -1px in a horizontal bar (SegmentControl.qss:37, :54), and margin-top -1px in a vertical
            //! one (:65): their border box starts on the border of the button before them. The widget's own rect is not part of it, so the
            //! border strip that falls outside the widget is not drawn and the neighbour's line is the one shared.
            bool overlapsPrevious() const
            {
                return m_isButton && !m_first && !m_one;
            }

            //! The margin of the rule: left, top, right, bottom
            QMargins margin() const
            {
                const int overlap = overlapsPrevious() ? SegmentOverlap : 0;
                return m_vertical ? QMargins(0, -overlap, 0, 0) : QMargins(-overlap, 0, 0, 0);
            }

            //! SegmentControl.qss:35-50: a horizontal bar rounds the outer corners of its first button (the left ones) and of its last (the
            //! right ones), whatever the layout direction: the rule says left and right. A vertical bar rounds none.
            BoxElement::Radii radii() const
            {
                BoxElement::Radii radii;
                if (!m_vertical)
                {
                    if (m_first)
                    {
                        radii.m_topLeft = radii.m_bottomLeft = SegmentRadius;
                    }
                    if (m_last)
                    {
                        radii.m_topRight = radii.m_bottomRight = SegmentRadius;
                    }
                }
                return radii;
            }

            //! The box around the contents (QRenderRule::boxSize, qstylesheetstyle.cpp:1096-1122): the border and the padding, the margin
            //! of the place in the direction of the bar
            QMargins around() const
            {
                const QMargins margin = this->margin();
                const int inset = SegmentBorderWidth + SegmentPadding;
                return QMargins(margin.left() + inset, margin.top() + SegmentBorderWidth, margin.right() + inset, margin.bottom() + SegmentBorderWidth);
            }
        };

        SegmentPlace segmentPlaceOf(const QWidget* button)
        {
            SegmentPlace place;
            if (!button)
            {
                return place;
            }
            place.m_first = TextElement::tagged(button, "TabFirst");
            place.m_last = TextElement::tagged(button, "TabLast");
            place.m_one = TextElement::tagged(button, "TabOne");
            const bool tagged = place.m_first || place.m_last || place.m_one || TextElement::tagged(button, "TabMiddle");
            for (QWidget* ancestor = tagged ? button->parentWidget() : nullptr; ancestor; ancestor = ancestor->parentWidget())
            {
                if (ancestor->inherits("AzQtComponents::SegmentBar"))
                {
                    place.m_isButton = true;
                    place.m_bar = ancestor;
                    place.m_vertical = ancestor->property("orientation").toInt() == static_cast<int>(Qt::Vertical);
                    break;
                }
            }
            return place;
        }

        //! The widgets above a widget, its parent and the parent of that and so on, as one number: it changes when QWidget::setParent moves the
        //! widget or any widget above it, and in no other way (a hash of the pointers, which are not reused while the widgets live)
        quint64 ancestryOf(const QWidget* widget)
        {
            size_t hash = 0;
            for (const QWidget* ancestor = widget->parentWidget(); ancestor; ancestor = ancestor->parentWidget())
            {
                hash = qHash(ancestor, hash);
            }
            return static_cast<quint64>(hash);
        }

        //! The ancestry a bar had when the style last polished one of its buttons (polishSegmentBar), kept on the bar
        constexpr const char* SegmentAncestryProperty = "_o3de_segmentBarAncestry";

        //! The fill of the state: :checked #555555 (the later rule, whatever else holds), else #444444 under the pointer (:hover needs the button
        //! enabled, qstylesheetstyle.cpp:1757-1764) or while it is pressed, else #333333. SegmentControl.qss:16, :68-77. UNVERIFIED by capture:
        //! the Native captures hold buttons idle and checked; hover, pressed and disabled follow the rules and are proven against the legacy sheet
        //! by ChromeTwinTests.SegmentBar_TheBevelIsTheSheetsInEveryPlaceStateAndRatio.
        const QColor& segmentFill(const QStyleOption* option)
        {
            if (option->state.testFlag(QStyle::State_On))
            {
                return SegmentCheckedFill;
            }
            const bool hovered = option->state.testFlag(QStyle::State_Enabled) && option->state.testFlag(QStyle::State_MouseOver);
            return (hovered || option->state.testFlag(QStyle::State_Sunken)) ? SegmentHoverFill : SegmentFill;
        }

        //! The `min-width: 68px` of the rule with the box around it, which QStyleSheetStyle::setGeometry made the button's minimum width
        //! (qstylesheetstyle.cpp:2597-2600; no min-height is declared): 102px, 101px for a button that overlaps its neighbour in a horizontal bar
        int segmentMinimumWidth(const SegmentPlace& place)
        {
            const QMargins around = place.around();
            return SegmentMinContentWidth + around.left() + around.right();
        }

        // ------------------------------------------------------------------------------
        // The frame of a floating dock: StyledDockWidget.qss:15-18 (the rule of a dock polished while it was floating)
        // ------------------------------------------------------------------------------

        //! AzQtComponents--StyledDockWidget[floating="true"] { border: 1px solid black } (StyledDockWidget.qss:15-18). A rule of its own style
        //! (not the native one), so no frame of the base style, and the rule has no fill: the dock's background is its palette's (:12).
        //! The element is resolved once, when the dock is polished (BoxElement::apply), and so holds the rule the sheet matched then: a dock
        //! that was docked has none (StyledDockWidget.qss:9-13: `border: none`), whatever it becomes afterwards. TOKENIZE: FloatingDockLine
        const BoxElement FloatingDockFrame{
            .m_name = "Global.Box.StyledDockWidget.Floating",
            .m_nativeBorder = false,                                // StyledDockWidget.qss:17
            .m_idle = {
                .m_edges = BoxElement::border(1, QColor(Qt::black)),// StyledDockWidget.qss:17
            },
        };

        // ------------------------------------------------------------------------------
        // The frame of the breadcrumbs whose path can be typed: BreadCrumbs.qss:33-37
        // ------------------------------------------------------------------------------

        //! AzQtComponents--BreadCrumbs[editable="true"] { border: 1px solid #2f2f2f; border-radius: 2px } (BreadCrumbs.qss:33-37): the frame of the
        //! breadcrumbs whose path the user can edit (the Asset Browser's, the Gallery's page). A BreadCrumbs is a QFrame with no frame shape and its
        //! layout lies inside the frame, so the 1px border is what a frame of 1px is: n2 holds the Gallery's 995 x 21 with contents margins of 1px,
        //! its label stack 19 high at [574, 94] and the Asset Browser's 688 x 21, where f5 had no border (margins 0) and 25 high. The rule has no
        //! fill (the sheet's `*` is transparent). The property is read when the breadcrumbs are polished, as the sheet matched its rule then.
        //! TOKENIZE: BreadCrumbsEditableBorder
        const BoxElement BreadCrumbsEditable{
            .m_name = "Global.Box.BreadCrumbsEditable",
            .m_nativeBorder = false,                                // border: BreadCrumbs.qss:35
            .m_radius = 2,                                          // border-radius: BreadCrumbs.qss:36
            .m_idle = {
                .m_edges = BoxElement::border(1, QColor(0x2F, 0x2F, 0x2F)),     // border: BreadCrumbs.qss:35
            },
        };

        // ------------------------------------------------------------------------------
        // Selectors: what each element covers is what its rule's selector covered
        // ------------------------------------------------------------------------------

        //! `AzQtComponents--BreadCrumbs[editable="true"]`: the property holds the whole word true when it is a bool
        bool isEditableBreadCrumbs(const QWidget* widget)
        {
            return widget->inherits("AzQtComponents::BreadCrumbs") && widget->property("editable").toString() == QLatin1String("true");
        }

        //! `AzQtComponents--TitleBar[drawAsTabBar="true"] #tabButtons`: the property holds the whole word true when it is a bool
        bool inTitleBarDrawnAsTabBar(const QWidget* widget)
        {
            for (const QWidget* ancestor = widget->parentWidget(); ancestor; ancestor = ancestor->parentWidget())
            {
                if (ancestor->inherits("AzQtComponents::TitleBar") && ancestor->property("drawAsTabBar").toString() == QLatin1String("true"))
                {
                    return true;
                }
            }
            return false;
        }

        //! `AzQtComponents--FilterCriteriaButton QPushButton#closeTag`, and the rule of FilterTextButton that repeats it
        bool inFilterTag(const QWidget* widget)
        {
            return TextElement::withinClass(widget, "AzQtComponents::FilterCriteriaButton")
                || TextElement::withinClass(widget, "AzQtComponents::FilterTextButton");
        }

        //! `MainWindow QToolBar` less the toolbars the later `AzQtComponents--DockMainWindow QToolBar` rule left without a border
        bool inMainWindowOutsideDockMainWindow(const QWidget* widget)
        {
            return TextElement::withinClass(widget, "MainWindow") && !TextElement::withinClass(widget, "AzQtComponents::DockMainWindow");
        }
    } // namespace ControlElementsInternal

    // ----------------------------------------------------------------------------------
    // Registration
    // ----------------------------------------------------------------------------------

    void ControlElements::registerGlobal()
    {
        using namespace ControlElementsInternal;

        static bool registered = false;
        if (registered || !StyleManager::stylesheetsDisabled())
        {
            return;
        }
        registered = true;

        constexpr BoxElement::Scope global = BoxElement::Scope::Global;

        BoxElement::registerElement(&TabButtons, &QFrame::staticMetaObject,
            [](const QWidget* w) { return TextElement::named(w, "tabButtons") && inTitleBarDrawnAsTabBar(w); }, global);
        BoxElement::registerElement(&CloseTag, &QPushButton::staticMetaObject,
            [](const QWidget* w) { return TextElement::named(w, "closeTag") && inFilterTag(w); }, global);
        BoxElement::registerElement(&FloatingDockFrame, &StyledDockWidget::staticMetaObject,
            [](const QWidget* w) { const auto* dock = qobject_cast<const QDockWidget*>(w); return dock && dock->isFloating(); }, global);
        BoxElement::registerElement(&ToolBarExtension, &QToolButton::staticMetaObject,
            [](const QWidget* w) { return TextElement::named(w, "qt_toolbar_ext_button") && w->inherits("QToolBarExtension"); }, global);
        BoxElement::registerElement(&BreadCrumbsEditable, &QFrame::staticMetaObject, &isEditableBreadCrumbs, global);
    }

    // ----------------------------------------------------------------------------------
    // Toolbars
    // ----------------------------------------------------------------------------------

    bool ControlElements::drawToolBarPanel(const QStyleOption* option, QPainter* painter, const QWidget* widget)
    {
        using namespace ControlElementsInternal;

        if (!StyleManager::stylesheetsDisabled() || !qobject_cast<const QToolBar*>(widget))
        {
            return false;
        }
        const BoxElement& panel = inMainWindowOutsideDockMainWindow(widget) ? ToolBarPanelUnderMainWindow : ToolBarPanel;
        return panel.draw(painter, option, widget);
    }

    int ControlElements::toolBarHandleExtent(const QStyleOption* option, const QWidget* widget)
    {
        using namespace ControlElementsInternal;

        if (!StyleManager::stylesheetsDisabled() || !qobject_cast<const QToolBar*>(widget))
        {
            return -1;
        }
        // The sheet read the box of the handle rule in the state of the option, a null one a vertical handle
        // (qstylesheetstyle.cpp:5110-5121): the image and the padding along the toolbar
        const bool horizontal = option && option->state.testFlag(QStyle::State_Horizontal);
        const HandleBox box = handleBox(widget, horizontal);
        return horizontal ? box.m_imageSize.width() + box.m_padding.left() + box.m_padding.right()
                          : box.m_imageSize.height() + box.m_padding.top() + box.m_padding.bottom();
    }

    bool ControlElements::drawToolBarHandle(const QStyleOption* option, QPainter* painter, const QWidget* widget)
    {
        using namespace ControlElementsInternal;

        if (!StyleManager::stylesheetsDisabled() || !qobject_cast<const QToolBar*>(widget))
        {
            return false;
        }
        // PE_IndicatorToolBarHandle (qstylesheetstyle.cpp:4741-4748): the rule draws its image, centred, in the contents rect
        // of the handle rect (QRenderRule::drawRule, :1386-1390)
        const HandleBox box = handleBox(widget, option->state.testFlag(QStyle::State_Horizontal));
        const QRect contents = option->rect.marginsRemoved(box.m_padding);
        QIcon(QString::fromLatin1(box.m_image)).paint(painter, contents, Qt::AlignCenter);
        return true;
    }

    // ----------------------------------------------------------------------------------
    // Segment bars
    // ----------------------------------------------------------------------------------

    QSize ControlElements::segmentButtonSize(const QSize& contents, const QWidget* widget)
    {
        using namespace ControlElementsInternal;

        if (!StyleManager::stylesheetsDisabled() || !qobject_cast<const QPushButton*>(widget))
        {
            return QSize();
        }
        const SegmentPlace place = segmentPlaceOf(widget);
        if (!place.m_isButton)
        {
            return QSize();
        }
        // QRenderRule::adjustSize (qstylesheetstyle.cpp:560-571): 28px high, at least 68px wide over the label; then the box around it
        // (boxSize, :1096-1122). Native: 30px high and 102 or 101 wide for a short label; a following vertical button 29 high.
        const QMargins around = place.around();
        return QSize(qMax(contents.width(), SegmentMinContentWidth) + around.left() + around.right(),
            SegmentContentHeight + around.top() + around.bottom());
    }

    QRect ControlElements::segmentButtonContents(const QStyleOption* option, const QWidget* widget)
    {
        using namespace ControlElementsInternal;

        if (!StyleManager::stylesheetsDisabled() || !qobject_cast<const QPushButton*>(widget))
        {
            return QRect();
        }
        const SegmentPlace place = segmentPlaceOf(widget);
        if (!place.m_isButton)
        {
            return QRect();
        }
        // QStyleSheetStyle::subElementRect (qstylesheetstyle.cpp:6155-6168): the contents rect of the rule, mirrored for a right to left option
        return QStyle::visualRect(option->direction, option->rect, option->rect.marginsRemoved(place.around()));
    }

    bool ControlElements::drawSegmentButton(const QStyle* style, QStyle::ControlElement element, const QStyleOption* option, QPainter* painter,
        const QWidget* widget)
    {
        using namespace ControlElementsInternal;

        // Asked for every control element of every widget: the cheap tests first
        const bool pushButton = element == QStyle::CE_PushButton || element == QStyle::CE_PushButtonBevel || element == QStyle::CE_PushButtonLabel;
        if (!StyleManager::stylesheetsDisabled() || !pushButton || !qobject_cast<const QPushButton*>(widget))
        {
            return false;
        }
        const SegmentPlace place = segmentPlaceOf(widget);
        if (!place.m_isButton)
        {
            return false;
        }
        switch (element)
        {
        case QStyle::CE_PushButtonBevel:
            // QStyleSheetStyle::drawControl (qstylesheetstyle.cpp:3651-3672): a rule with a border of its own is drawn whole, over the rect of
            // the widget (QRenderRule::drawRule: the fill and the border in the border box, which the negative margin pulls out of the widget)
            BoxElement::drawFrame(painter, option->rect.marginsRemoved(place.margin()), QBrush(segmentFill(option)),
                BoxElement::border(SegmentBorderWidth, SegmentLine), place.radii());
            return true;

        case QStyle::CE_PushButton:
            if (const auto* button = qstyleoption_cast<const QStyleOptionButton*>(option))
            {
                // QCommonStyle::drawControl (qcommonstyle.cpp:1294-1308): the bevel, then the label in the contents of the rule. Fusion would ask
                // its own subElementRect for them (qfusionstyle.cpp:1598-1604), and its contents rect is not the rule's. No focus rectangle:
                // the buttons of a bar take no focus (SegmentBar::createButton).
                style->drawControl(QStyle::CE_PushButtonBevel, button, painter, widget);
                QStyleOptionButton label(*button);
                label.rect = segmentButtonContents(button, widget);
                style->drawControl(QStyle::CE_PushButtonLabel, &label, painter, widget);
                return true;
            }
            return false;

        case QStyle::CE_PushButtonLabel:
            if (const auto* button = qstyleoption_cast<const QStyleOptionButton*>(option); button && place.m_vertical && button->icon.isNull())
            {
                // SegmentControl.qss:57-60 `text-align: left`, in the contents of the rule (the widget's rect less margin, border and padding), as
                // QStyleSheetStyle draws a label with a text alignment (qstylesheetstyle.cpp:3688-3787): vertically centred, the mnemonic
                // shown unless the style hides it. A button with an icon keeps the base style's centred group: no vertical bar has one (UNVERIFIED:
                // no capture of one).
                const QRect contents = widget->rect().marginsRemoved(place.around());
                int textFlags = Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic;
                if (!style->styleHint(QStyle::SH_UnderlineShortcut, button, widget, nullptr))
                {
                    textFlags |= Qt::TextHideMnemonic;
                }
                style->drawItemText(painter, contents, textFlags, button->palette, button->state.testFlag(QStyle::State_Enabled), button->text,
                    QPalette::ButtonText);
                return true;
            }
            return false;

        default:
            return false;
        }
    }

    void ControlElements::polishSegmentBar(QWidget* widget)
    {
        using namespace ControlElementsInternal;

        if (!StyleManager::stylesheetsDisabled() || !widget)
        {
            return;
        }
        // The sheet gave a button its minimum width when it polished it (QStyleSheetStyle::polish: setGeometry, qstylesheetstyle.cpp:2889),
        // and WA_Hover for the :hover of its fill (:2894-2905)
        const auto polishButton = [](QWidget* button, const SegmentPlace& place)
        {
            button->setMinimumWidth(segmentMinimumWidth(place));
            button->setAttribute(Qt::WA_Hover, true);
        };
        if (qobject_cast<QPushButton*>(widget))
        {
            if (const SegmentPlace place = segmentPlaceOf(widget); place.m_isButton)
            {
                polishButton(widget, place);
                // Where the bar is now (see below)
                place.m_bar->setProperty(SegmentAncestryProperty, QVariant::fromValue(ancestryOf(place.m_bar)));
            }
            return;
        }

        // SegmentBar::updateTabs sizes every button to the widest (setMinimumWidth, SegmentBar.cpp:352-355) after QPushButton::sizeHint
        // polished them, so the minimum width the sheet set is gone. In Native it came back wherever the bar, or a widget above it, was put
        // into another widget after the tabs were added: QWidget::setParent of a polished widget polishes it and its children again
        // (qwidget.cpp:10866, QWidgetPrivate::inheritStyle and setStyle_helper, :2680-2778), which the sheet answered with setGeometry. n2: the
        // Gallery adds a page to its stack after the page has built its bar (its buttons: 102, 101, 101, 101 wide, not 116), and so does
        // PhysX's collision filtering widget (102, 101); the Script Canvas logging window builds its bar in place and nobody moves it (102,
        // 102: the widest hint), and the Asset Processor adds its tabs once its window is shown (126 for all nine). A flattened style has no
        // style to change at the reparent, so the bar, polished at its first show, does it where a widget above it is not where it was
        // when its buttons were last polished (the ancestry that polishSegmentBar kept). UNVERIFIED, and not followed: a bar moved after it
        // was polished keeps what it has here and the sheet reset it, and so with a bar that is a window itself while it gets its tabs (it is
        // polished at its first tab, QLayout::totalMinimumSize polishes the parent of a top level layout; no bar of the engine is built so).
        if (qobject_cast<QFrame*>(widget) && widget->inherits("AzQtComponents::SegmentBar"))
        {
            const QVariant polishedAt = widget->property(SegmentAncestryProperty);
            const quint64 ancestry = ancestryOf(widget);
            if (!polishedAt.isValid() || polishedAt.toULongLong() == ancestry)
            {
                return;                 // no button was polished yet, or nothing above the bar was moved since
            }
            widget->setProperty(SegmentAncestryProperty, QVariant::fromValue(ancestry));
            const QList<QPushButton*> buttons = widget->findChildren<QPushButton*>();
            bool changed = false;
            for (QPushButton* button : buttons)
            {
                if (const SegmentPlace place = segmentPlaceOf(button); place.m_isButton)
                {
                    polishButton(button, place);
                    changed = true;
                }
            }
            if (changed)
            {
                // The style change of the repolish told every widget to forget its size hint and its layout to lay out again (QWidget::changeEvent,
                // StyleChange: updateGeometry and invalidate, qwidget.cpp:9475-9483): the layout of the bar caches the hints of the widgets it
                // holds, the buttons' container among them, until it is told, and the bar is sized from them as it is shown.
                for (QPushButton* button : buttons)
                {
                    if (QWidget* container = button->parentWidget())
                    {
                        container->updateGeometry();
                    }
                }
            }
        }
    }

    // ----------------------------------------------------------------------------------
    // Floating docks
    // ----------------------------------------------------------------------------------

    int ControlElements::dockFrameWidth(const QWidget* widget)
    {
        if (!StyleManager::stylesheetsDisabled() || !qobject_cast<const StyledDockWidget*>(widget))
        {
            return -1;
        }
        // QStyleSheetStyle::pixelMetric (qstylesheetstyle.cpp:5052-5059): the left border of the rule, plus the margin and the padding of a rule
        // that has a box. The rule of a dock polished docked has `border: none`: 0 (StyledDockWidget.qss:11).
        const BoxElement* frame = BoxElement::forWidget(widget);
        return frame ? frame->m_idle.m_edges.m_left.m_width + frame->m_margin.left() + frame->m_padding.left() : 0;
    }

    bool ControlElements::drawDockFrame(const QStyleOption* option, QPainter* painter, const QWidget* widget)
    {
        if (!StyleManager::stylesheetsDisabled() || !qobject_cast<const StyledDockWidget*>(widget))
        {
            return false;
        }
        // QStyleSheetStyle::drawPrimitive (qstylesheetstyle.cpp:4638-4650): a rule with a border of its own draws it in the border rect of the
        // option; `border: none` draws nothing
        if (const BoxElement* frame = BoxElement::forWidget(widget))
        {
            frame->draw(painter, option, widget);
        }
        return true;
    }
} // namespace AzQtComponents
