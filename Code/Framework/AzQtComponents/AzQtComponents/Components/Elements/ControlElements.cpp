/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/ControlElements.h>
#include <AzQtComponents/Components/Elements/ElementCommon.h>
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
    //! Private to this file: named, not anonymous, as a unity build merges the anonymous namespaces of its files
    namespace ControlElementsInternal
    {
        // ------------------------------------------------------------------------------
        // Values
        // ------------------------------------------------------------------------------

        // ---- The buttons beside a dock's tabs
        constexpr QMargins TabButtonsMargin(0, 0, 0, 3);            // TitleBar.qss:56 $space.surface.idle.margin-bottom.Alt1
        const QColor TitleBarFill(0x00, 0x00, 0x00, 0x00);          // TitleBar.qss:28 $fill.surface.idle.Clear

        // ---- The close button of a filter tag
        const QColor CloseTagFill(0x00, 0x00, 0x00, 0x00);          // FilteredSearchWidget.qss:42 $fill.button.idle.fill.Clear
        constexpr int CloseTagMinWidth = 16;                        // FilteredSearchWidget.qss:47 $size.button.idle.min-width.Alt4
        // FilteredSearchWidget.qss:46, :45 $size.button.idle.max-width.Alt3 $size.button.idle.max-height.Alt3
        constexpr QSize CloseTagMaxSize(16, 16);
        constexpr bool CloseTagFlat = true;                         // FilteredSearchWidget.qss:41 $other.button.idle.flat
        constexpr const char* CloseTagIcon = ":/stylesheet/img/close_x.svg";    // FilteredSearchWidget.qss:39 $asset.button.idle.icon.Alt3
        constexpr QSize CloseTagIconSize(10, 10);                   // FilteredSearchWidget.qss:40 $asset.button.idle.iconsize.Alt5

        // ---- The panel of a toolbar, and the button for the actions that do not fit it
        const QColor ToolBarFill(0x44, 0x44, 0x44);                 // ToolBar.qss:20 $fill.bar.idle.Mid
        // ToolBar.qss:33 $line.bar.idle.width.Alt2 $line.bar.idle.color
        const BoxElement::Edge ToolBarBottomLine{ 2, QColor(0x11, 0x11, 0x11) };
        constexpr const char* ToolBarExtensionIcon = ":/stylesheet/img/hidden-icons.svg";   // ToolBar.qss:141 $asset.bar.idle.icon

        // ---- The handle of a movable toolbar: the image, its own size in the file, and the padding of each rule
        // ToolBar.qss:83 $asset.scroll-handle.idle.Alt1
        constexpr const char* HandleImageHorizontal = ":/stylesheet/img/UI20/tear.svg";
        // ToolBar.qss:88 $asset.scroll-handle.idle.Alt2
        constexpr const char* HandleImageVertical = ":/stylesheet/img/UI20/tear-vertical.svg";
        constexpr QSize HandleImageSizeHorizontal(2, 14);           // tear.svg:2, the size of $asset.scroll-handle.idle.Alt1
        constexpr QSize HandleImageSizeVertical(24, 6);             // tear-vertical.svg:2, the size of $asset.scroll-handle.idle.Alt2
        constexpr QMargins MainHandlePadding(4, 1, 4, 1);           // ToolBar.qss:94, :106 $space.scroll-handle.idle.Alt3
        constexpr QMargins LargeHandlePadding(3, 4, 3, 4);          // ToolBar.qss:118 $space.scroll-handle.idle.Alt4
        constexpr QMargins HandlePadding(4, 2, 4, 2);               // ToolBar.qss:130 $space.scroll-handle.idle.Alt2
        constexpr QMargins MainLargeVerticalHandlePadding(1, 2, 1, 2);  // ToolBar.qss:99 $space.scroll-handle.idle.Alt5
        constexpr QMargins VerticalHandlePadding(-2, 2, -2, 2);     // ToolBar.qss:112, :123, :136 $space.scroll-handle.idle.Alt1

        // ---- The buttons of a segment bar
        const QColor SegmentFill(0x33, 0x33, 0x33);                 // SegmentControl.qss:16 $fill.button.idle.fill.Low
        const QColor SegmentHoverFill(0x44, 0x44, 0x44);            // SegmentControl.qss:76 $fill.button.hover+unchecked
        const QColor SegmentPressedFill(0x44, 0x44, 0x44);          // SegmentControl.qss:76 $fill.button.pressed+unchecked
        const QColor SegmentCheckedFill(0x55, 0x55, 0x55);          // SegmentControl.qss:70 $fill.button.checked
        const QColor SegmentLine(0x22, 0x22, 0x22);                 // SegmentControl.qss:17 $line.button.idle.color.BG
        constexpr int SegmentLineWidth = 1;                         // SegmentControl.qss:19 $line.button.idle.width.Alt2
        // SegmentControl.qss:31-32 $space.button.idle.padding-left $space.button.idle.padding-right
        constexpr QMargins SegmentPadding(16, 0, 16, 0);
        constexpr int SegmentContentHeight = 28;                    // SegmentControl.qss:23 $size.button.idle.height
        constexpr int SegmentMinContentWidth = 68;                  // SegmentControl.qss:29 $size.button.idle.min-width.Alt3
        constexpr int SegmentMarginLeft = -1;                       // SegmentControl.qss:37, :54 $space.button.idle.margin-left.Alt2
        constexpr int SegmentMarginTop = -1;                        // SegmentControl.qss:65 $space.button.idle.margin-top.Alt1
        // SegmentControl.qss:46-47 $radius.button.idle.top-left.Alt2 $radius.button.idle.bottom-left.Alt2
        constexpr BoxElement::Radii FirstSegmentRadii{ .m_topLeft = 3, .m_bottomLeft = 3 };
        // SegmentControl.qss:40-41 $radius.button.idle.top-right.Alt3 $radius.button.idle.bottom-right.Alt3
        constexpr BoxElement::Radii LastSegmentRadii{ .m_topRight = 3, .m_bottomRight = 3 };
        constexpr Qt::Alignment VerticalSegmentTextAlign = Qt::AlignLeft;   // SegmentControl.qss:58 $other.button.idle.text-align.Alt1

        // ---- The frame of a floating dock, and of breadcrumbs whose path can be typed
        // StyledDockWidget.qss:17 $line.surface.floating.width $line.surface.floating.color
        const BoxElement::Edge FloatingDockLine{ 1, QColor(0x00, 0x00, 0x00) };
        // BreadCrumbs.qss:35 $line.bar.editable.width $line.bar.editable.color
        const BoxElement::Edge EditableBreadCrumbsLine{ 1, QColor(0x2F, 0x2F, 0x2F) };
        constexpr int EditableBreadCrumbsRadius = 2;                // BreadCrumbs.qss:36 $radius.bar.editable

        // ------------------------------------------------------------------------------
        // Elements
        // ------------------------------------------------------------------------------

        //! `AzQtComponents--TitleBar[drawAsTabBar="true"] #tabButtons`: the frame of a dock's minimize, maximize and close buttons while
        //! the dock draws as a tab bar. Its margin keeps the 26px buttons at the top of the title bar. The frame is polished before its
        //! title bar sets the property, so TitleBar::setDrawAsTabBar resolves it again.
        const BoxElement TabButtons{
            .m_name = "Global.Box.TabButtons",
            .m_margin = TabButtonsMargin,
            .m_idle = {
                .m_fill = QBrush(TitleBarFill),
            },
        };

        //! `AzQtComponents--FilterCriteriaButton QPushButton#closeTag`, and the same rule of FilterTextButton: a flat button with the close
        //! glyph, no margin or padding (FilteredSearchWidget.qss:43-44 $space.button.idle.margin.Alt1 $space.button.idle.padding.Alt1),
        //! and no border: every push button of the search widget has `border: none` (FilteredSearchWidget.qss:18
        //! $line.button.idle.style.Alt1)
        const BoxElement CloseTag{
            .m_name = "Global.Box.CloseTag",
            .m_hasBox = true,
            .m_nativeBorder = false,
            .m_minSize = QSize(CloseTagMinWidth, -1),
            .m_maxSize = CloseTagMaxSize,
            .m_idle = {
                .m_fill = QBrush(CloseTagFill),
            },
            .m_properties = {
                { "flat", CloseTagFlat },
                { "icon", QString(CloseTagIcon) },
                { "iconSize", CloseTagIconSize },
            },
        };

        //! `QToolBar { background-color; border: 0 none }`: a border of its own, none (ToolBar.qss:22 $line.bar.idle.style.Alt1), so the
        //! sheet draws no bevel of the base style (QStyleSheetStyle::drawControl, CE_ToolBar)
        const BoxElement ToolBarPanel{
            .m_name = "Global.Box.ToolBar",
            .m_nativeBorder = false,
            .m_idle = {
                .m_fill = QBrush(ToolBarFill),
            },
        };

        //! `MainWindow QToolBar { border-bottom }` over the rule above; a toolbar of a DockMainWindow has none (the later rule
        //! `AzQtComponents--DockMainWindow QToolBar`)
        const BoxElement ToolBarPanelUnderMainWindow{
            .m_name = "Global.Box.ToolBar.MainWindow",
            .m_nativeBorder = false,
            .m_idle = {
                .m_fill = QBrush(ToolBarFill),
                .m_edges = { {}, {}, ToolBarBottomLine, {} },               // top, right, bottom, left
            },
        };

        //! `QToolBarExtension#qt_toolbar_ext_button { qproperty-icon }`: the double caret of the button at the end of a toolbar too narrow
        //! for its actions, set at polish over the base style's caret (QStyleSheetStyle::setProperties)
        const BoxElement ToolBarExtension{
            .m_name = "Global.Box.ToolBarExtension",
            .m_properties = {
                { "icon", QString(ToolBarExtensionIcon) },
            },
        };

        //! `AzQtComponents--StyledDockWidget[floating="true"] { border }`: a border of its own, so no frame of the base style. The element
        //! is resolved at the dock's polish and so holds the rule the sheet matched then: a dock polished docked has none
        //! (StyledDockWidget.qss:11 `border: none`), whatever it becomes afterwards.
        const BoxElement FloatingDockFrame{
            .m_name = "Global.Box.StyledDockWidget.Floating",
            .m_nativeBorder = false,
            .m_idle = {
                .m_edges = { FloatingDockLine, FloatingDockLine, FloatingDockLine, FloatingDockLine },
            },
        };

        //! `AzQtComponents--BreadCrumbs[editable="true"] { border; border-radius }`: the frame of breadcrumbs whose path can be edited (the
        //! Asset Browser's, the Gallery's). Its layout lies inside the frame, so the border is a frame of 1px; the rule has no fill.
        const BoxElement BreadCrumbsEditable{
            .m_name = "Global.Box.BreadCrumbsEditable",
            .m_nativeBorder = false,
            .m_radius = EditableBreadCrumbsRadius,
            .m_idle = {
                .m_edges = { EditableBreadCrumbsLine, EditableBreadCrumbsLine, EditableBreadCrumbsLine, EditableBreadCrumbsLine },
            },
        };

        // ------------------------------------------------------------------------------
        // Helpers: the handle of a toolbar
        // ------------------------------------------------------------------------------

        struct HandleBox
        {
            const char* m_image;
            QSize m_imageSize;
            QMargins m_padding;                                     //!< left, top, right, bottom
        };

        //! The handle rule that wins for a toolbar's tags: of the rules that hold, the most specific, of equal ones the later
        HandleBox handleBox(const QWidget* toolbar, bool horizontal)
        {
            const bool main = Selectors::tagged(toolbar, "MainToolBar");
            const bool large = Selectors::tagged(toolbar, "IconLarge");
            if (horizontal)
            {
                if (main)
                {
                    return { HandleImageHorizontal, HandleImageSizeHorizontal, MainHandlePadding };
                }
                return { HandleImageHorizontal, HandleImageSizeHorizontal, large ? LargeHandlePadding : HandlePadding };
            }
            return { HandleImageVertical, HandleImageSizeVertical,
                (main && large) ? MainLargeVerticalHandlePadding : VerticalHandlePadding };
        }

        // ------------------------------------------------------------------------------
        // Helpers: the buttons of a segment bar
        // ------------------------------------------------------------------------------

        //! Where a button sits in its bar. SegmentBar::updateTabs tags every button TabOne, TabFirst, TabMiddle or TabLast, and the bar
        //! that owns the orientation is an ancestor of the button, not its parent (`AzQtComponents--SegmentBar QAbstractButton`).
        struct SegmentPlace
        {
            bool m_isButton = false;
            QWidget* m_bar = nullptr;   //!< the SegmentBar the button is in (null for a widget that is no button of a bar)
            bool m_vertical = false;    //!< the bar's orientation is Qt::Vertical
            bool m_first = false;       //!< TabFirst
            bool m_last = false;        //!< TabLast
            bool m_one = false;         //!< TabOne: the base rule alone

            //! TabMiddle and TabLast start on the border of the button before them: the border strip outside the widget is not drawn,
            //! and the neighbour's line is the one shared
            bool overlapsPrevious() const
            {
                return m_isButton && !m_first && !m_one;
            }

            //! The margin of the rule: left, top, right, bottom
            QMargins margin() const
            {
                if (!overlapsPrevious())
                {
                    return QMargins();
                }
                return m_vertical ? QMargins(0, SegmentMarginTop, 0, 0) : QMargins(SegmentMarginLeft, 0, 0, 0);
            }

            //! A horizontal bar rounds the outer corners of its first button and of its last, whatever the layout direction; a vertical
            //! bar rounds none
            BoxElement::Radii radii() const
            {
                BoxElement::Radii radii;
                if (!m_vertical && m_first)
                {
                    radii.m_topLeft = FirstSegmentRadii.m_topLeft;
                    radii.m_bottomLeft = FirstSegmentRadii.m_bottomLeft;
                }
                if (!m_vertical && m_last)
                {
                    radii.m_topRight = LastSegmentRadii.m_topRight;
                    radii.m_bottomRight = LastSegmentRadii.m_bottomRight;
                }
                return radii;
            }

            //! The box around the contents (QRenderRule::boxSize): the margin of the place, the border and the padding
            QMargins around() const
            {
                const QMargins line(SegmentLineWidth, SegmentLineWidth, SegmentLineWidth, SegmentLineWidth);
                return margin() + line + SegmentPadding;
            }
        };

        SegmentPlace segmentPlaceOf(const QWidget* button)
        {
            SegmentPlace place;
            if (!button)
            {
                return place;
            }
            place.m_first = Selectors::tagged(button, "TabFirst");
            place.m_last = Selectors::tagged(button, "TabLast");
            place.m_one = Selectors::tagged(button, "TabOne");
            const bool tagged = place.m_first || place.m_last || place.m_one || Selectors::tagged(button, "TabMiddle");
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

        //! The widgets above a widget as one number: it changes when QWidget::setParent moves the widget or any widget above it, and in
        //! no other way (a hash of the pointers, which are not reused while the widgets live)
        quint64 ancestryOf(const QWidget* widget)
        {
            size_t hash = 0;
            for (const QWidget* ancestor = widget->parentWidget(); ancestor; ancestor = ancestor->parentWidget())
            {
                hash = qHash(ancestor, hash);
            }
            return static_cast<quint64>(hash);
        }

        //! The dynamic property that keeps the ancestry a bar had when the style last polished one of its buttons
        constexpr const char* SegmentAncestryProperty = "_o3de_segmentBarAncestry";

        //! The fill of the state: :checked, the later rule, whatever else holds; then pressed, or hovered (:hover needs :enabled)
        const QColor& segmentFill(const QStyleOption* option)
        {
            if (option->state.testFlag(QStyle::State_On))
            {
                return SegmentCheckedFill;
            }
            if (option->state.testFlag(QStyle::State_Sunken))
            {
                return SegmentPressedFill;
            }
            const bool hovered = option->state.testFlag(QStyle::State_Enabled) && option->state.testFlag(QStyle::State_MouseOver);
            return hovered ? SegmentHoverFill : SegmentFill;
        }

        //! The rule's min-width with the box around it, which QStyleSheetStyle::setGeometry made the button's minimum width
        int segmentMinimumWidth(const SegmentPlace& place)
        {
            const QMargins around = place.around();
            return SegmentMinContentWidth + around.left() + around.right();
        }

        // ------------------------------------------------------------------------------
        // Helpers: selectors
        // ------------------------------------------------------------------------------

        //! `AzQtComponents--BreadCrumbs[editable="true"]`
        bool isEditableBreadCrumbs(const QWidget* widget)
        {
            return widget->inherits("AzQtComponents::BreadCrumbs") && Selectors::hasProperty(widget, "editable", "true");
        }

        //! `AzQtComponents--TitleBar[drawAsTabBar="true"] W`
        bool inTitleBarDrawnAsTabBar(const QWidget* widget)
        {
            for (const QWidget* ancestor = widget->parentWidget(); ancestor; ancestor = ancestor->parentWidget())
            {
                if (ancestor->inherits("AzQtComponents::TitleBar") && Selectors::hasProperty(ancestor, "drawAsTabBar", "true"))
                {
                    return true;
                }
            }
            return false;
        }

        //! `AzQtComponents--FilterCriteriaButton QPushButton#closeTag`, and the rule of FilterTextButton that repeats it
        bool inFilterTag(const QWidget* widget)
        {
            return Selectors::withinClass(widget, "AzQtComponents::FilterCriteriaButton")
                || Selectors::withinClass(widget, "AzQtComponents::FilterTextButton");
        }

        //! `MainWindow QToolBar` less the toolbars the later `AzQtComponents--DockMainWindow QToolBar` rule left without a border
        bool inMainWindowOutsideDockMainWindow(const QWidget* widget)
        {
            return Selectors::withinClass(widget, "MainWindow") && !Selectors::withinClass(widget, "AzQtComponents::DockMainWindow");
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

        constexpr ElementScope global = ElementScope::Global;

        BoxElement::registerElement(&TabButtons, &QFrame::staticMetaObject,
            [](const QWidget* w) { return Selectors::named(w, "tabButtons") && inTitleBarDrawnAsTabBar(w); }, global);
        BoxElement::registerElement(&CloseTag, &QPushButton::staticMetaObject,
            [](const QWidget* w) { return Selectors::named(w, "closeTag") && inFilterTag(w); }, global);
        BoxElement::registerElement(&FloatingDockFrame, &StyledDockWidget::staticMetaObject,
            [](const QWidget* w) { const auto* dock = qobject_cast<const QDockWidget*>(w); return dock && dock->isFloating(); }, global);
        BoxElement::registerElement(&ToolBarExtension, &QToolButton::staticMetaObject,
            [](const QWidget* w) { return Selectors::named(w, "qt_toolbar_ext_button") && w->inherits("QToolBarExtension"); }, global);
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
        // The sheet read the handle rule's box in the option's state, a vertical one with no option (QStyleSheetStyle::pixelMetric):
        // the image and the padding along the toolbar
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
        // The rule draws its image centered in the handle's contents rect (QStyleSheetStyle::drawPrimitive,
        // PE_IndicatorToolBarHandle; QRenderRule::drawRule)
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
        // QRenderRule::adjustSize, then the box around it (QRenderRule::boxSize)
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
        // The rule's contents rect, mirrored for a right to left option (QStyleSheetStyle::subElementRect)
        return QStyle::visualRect(option->direction, option->rect, option->rect.marginsRemoved(place.around()));
    }

    bool ControlElements::drawSegmentButton(
        const QStyle* style, QStyle::ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget)
    {
        using namespace ControlElementsInternal;

        // Asked for every control element of every widget: the cheap tests first
        const bool pushButton =
            element == QStyle::CE_PushButton || element == QStyle::CE_PushButtonBevel || element == QStyle::CE_PushButtonLabel;
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
            // A rule with a border of its own is drawn whole over the widget's rect (QStyleSheetStyle::drawControl,
            // CE_PushButtonBevel): the fill and the border in the border box, which the negative margin pulls out of the widget
            BoxElement::drawFrame(painter, option->rect.marginsRemoved(place.margin()), QBrush(segmentFill(option)),
                BoxElement::border(SegmentLineWidth, SegmentLine), place.radii());
            return true;

        case QStyle::CE_PushButton:
            if (const auto* button = qstyleoption_cast<const QStyleOptionButton*>(option))
            {
                // QCommonStyle::drawControl: the bevel, then the label in the rule's contents. Fusion would ask its own subElementRect,
                // which is not the rule's. No focus rectangle: the buttons of a bar take no focus.
                style->drawControl(QStyle::CE_PushButtonBevel, button, painter, widget);
                QStyleOptionButton label(*button);
                label.rect = segmentButtonContents(button, widget);
                style->drawControl(QStyle::CE_PushButtonLabel, &label, painter, widget);
                return true;
            }
            return false;

        case QStyle::CE_PushButtonLabel:
            if (const auto* button = qstyleoption_cast<const QStyleOptionButton*>(option);
                button && place.m_vertical && button->icon.isNull())
            {
                // The rule's text-align in its contents, vertically centered, the mnemonic shown unless the style hides it
                // (QStyleSheetStyle::drawControl, CE_PushButtonLabel). A button with an icon keeps the base style's centered group.
                const QRect contents = widget->rect().marginsRemoved(place.around());
                int textFlags = int(VerticalSegmentTextAlign) | Qt::AlignVCenter | Qt::TextShowMnemonic;
                if (!style->styleHint(QStyle::SH_UnderlineShortcut, button, widget, nullptr))
                {
                    textFlags |= Qt::TextHideMnemonic;
                }
                style->drawItemText(painter, contents, textFlags, button->palette, button->state.testFlag(QStyle::State_Enabled),
                    button->text, QPalette::ButtonText);
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
        // The sheet gave a button its minimum width when it polished it (QStyleSheetStyle::setGeometry), and WA_Hover for the
        // :hover of its fill
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
                place.m_bar->setProperty(SegmentAncestryProperty, QVariant::fromValue(ancestryOf(place.m_bar)));
            }
            return;
        }

        // SegmentBar::updateTabs sizes every button to the widest after QPushButton::sizeHint polished them, so the minimum width the
        // sheet set is gone. It came back wherever the bar, or a widget above it, was moved into another widget after the tabs were
        // added: the sheet polished a reparented widget again (QWidgetPrivate::setStyle_helper) and set its geometry. The bar, polished
        // at its first show, does the same where a widget above it moved since its buttons were last polished.
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
                // The repolish told every widget its style changed, so that it forgot its size hint and its layout laid out again
                // (QWidget::changeEvent): the bar's layout caches the hints of the buttons' container until it is told
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
        // The rule's left border, plus the margin and the padding of a rule that has a box (QStyleSheetStyle::pixelMetric,
        // PM_DockWidgetFrameWidth); a dock polished docked has `border: none`: 0
        const BoxElement* frame = BoxElement::forWidget(widget);
        return frame ? frame->m_idle.m_edges.m_left.m_width + frame->m_margin.left() + frame->m_padding.left() : 0;
    }

    bool ControlElements::drawDockFrame(const QStyleOption* option, QPainter* painter, const QWidget* widget)
    {
        if (!StyleManager::stylesheetsDisabled() || !qobject_cast<const StyledDockWidget*>(widget))
        {
            return false;
        }
        // A rule with a border of its own draws it in the option's border rect; `border: none` draws nothing
        // (QStyleSheetStyle::drawPrimitive, PE_FrameDockWidget)
        if (const BoxElement* frame = BoxElement::forWidget(widget))
        {
            frame->draw(painter, option, widget);
        }
        return true;
    }
} // namespace AzQtComponents
