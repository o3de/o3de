/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Elements/TabElement.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>

#include <QApplication>
#include <QHash>
#include <QIcon>
#include <QImageReader>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QStyle>
#include <QStyleOption>
#include <QTabBar>
#include <QTabWidget>
#include <QVariant>
#include <QVector>
#include <QWidget>

namespace AzQtComponents
{
    namespace
    {
        // ------------------------------------------------------------------------------
        // Values of the Global tab elements: each is what the legacy GUI finally renders.
        // The comments name the declaration that grants it, as file:line of the legacy
        // stylesheets in Components/Widgets/; a value no rule sets is Qt's own.
        // ------------------------------------------------------------------------------

        const QColor Black111(0x11, 0x11, 0x11);
        const QColor Grey333(0x33, 0x33, 0x33);
        const QColor Grey444(0x44, 0x44, 0x44);
        const QColor White(0xFF, 0xFF, 0xFF);
        const QColor Muted959(0x95, 0x95, 0x95);
        const QColor HoverVeil(0xFF, 0xFF, 0xFF, 0x26);          // rgba(255, 255, 255, 0.15)
        const QColor CloseHover(0xFF, 0xFF, 0xFF, 0x33);         // rgba(255, 255, 255, 0.2)
        const QColor CloseRed(0xB8, 0x0D, 0x1C);
        const QColor SelectedUnderline(0x00, 0x8D, 0xE5);
        const QColor PaneLine(0x2B, 0x2B, 0x2B);

        constexpr const char* CloseIcon = ":/stylesheet/img/close_small.svg";
        constexpr const char* ElementProperty = "_o3de_tabElement";

        TabElement::Edges box(int width, const QColor& color)
        {
            return { { width, color }, { width, color }, { width, color }, { width, color } };
        }

        // ------------------------------------------------------------------------------
        // Global elements
        // ------------------------------------------------------------------------------

        //! TabWidget.qss primary tabs: Gallery tab widgets, Asset Bundler, Lua IDE find.
        const TabElement Primary{
            .m_name = "Global.Tab.Primary",
            .m_barFill = Grey444,                                           // TabWidget.qss:26
            .m_barMinHeight = 28,                                           // TabWidget.qss:46
            .m_idle = { Grey333, White, box(1, Black111) },                 // TabWidget.qss:57; text BaseStyleSheet.qss:15
            .m_hover = { Grey444, White, box(1, Black111) },                // fill TabWidget.qss:87, edges :57
            // fill TabWidget.qss:87, bottom edge :82, other edges :57
            .m_selected = { Grey444, White, { { 1, Black111 }, { 1, Black111 }, { 1, Grey444 }, { 1, Black111 } } },
            .m_selectedHover = { Grey444, White, { { 1, Black111 }, { 1, Black111 }, { 1, Grey444 }, { 1, Black111 } } },
            .m_padding = QMargins(9, 6, 21, 6),                             // TabWidget.qss:57
            .m_maxContentWidth = 200,                                       // TabWidget.qss:16
            .m_closeIcon = CloseIcon,                                       // TabWidget.qss:64
            .m_closeHoverFill = CloseHover,                                 // TabWidget.qss:70
            .m_closeRadius = 2,                                             // TabWidget.qss:70
            .m_closeMarginRight = 4,                                        // TabWidget.qss:64
            .m_widgetFill = Black111,                                       // TabWidget.qss:21
            .m_paneEmptyOffset = 30,                                        // TabWidget.qss:41
            .m_cornerMinHeight = 28,                                        // TabWidgetActionToolBar.qss:1
            .m_cornerMarginBottom = 9,                                      // TabWidgetActionToolBar.qss:1
            .m_moveAnimationDuration = 0,                                   // TabWidget.qss:46
        };

        //! Docked panel groups (DockTabBar inside a DockTabWidget): primary without the left edge.
        //! A dock tab's close button is a DockBarButton (DockTabBar::tabInserted, box and margin
        //! from TitleBar.qss:79, icon from TitleBar.qss:112); the element gives its hover fill.
        const TabElement DockGroup{
            .m_name = "Global.Tab.DockGroup",
            .m_barFill = Grey444,                                           // TabWidget.qss:26
            .m_barMinHeight = 28,                                           // TabWidget.qss:46
            // fill and edges TabWidget.qss:57 (hover and selected fill :87, selected bottom edge :82).
            // No left edge on any tab: StyledDockWidget.qss:30 lists ::tab:only, and :only is not a Qt
            // pseudo-class (Qt's is :only-one), so Qt applies the rule to every tab.
            .m_idle = { Grey333, White, { { 1, Black111 }, { 1, Black111 }, { 1, Black111 }, {} } },
            .m_hover = { Grey444, White, { { 1, Black111 }, { 1, Black111 }, { 1, Black111 }, {} } },
            .m_selected = { Grey444, White, { { 1, Black111 }, { 1, Black111 }, { 1, Grey444 }, {} } },
            .m_selectedHover = { Grey444, White, { { 1, Black111 }, { 1, Black111 }, { 1, Grey444 }, {} } },
            .m_padding = QMargins(9, 6, 21, 6),                             // TabWidget.qss:57
            .m_maxContentWidth = 200,                                       // TabWidget.qss:16
            .m_closeHoverFill = CloseRed,                                   // TitleBar.qss:103
            .m_widgetFill = Black111,                                       // TabWidget.qss:21
            .m_paneEmptyOffset = 30,                                        // TabWidget.qss:41
            .m_cornerMinHeight = 28,                                        // TabWidgetActionToolBar.qss:26
            .m_cornerMarginBottom = 9,                                      // TabWidgetActionToolBar.qss:26
            .m_moveAnimationDuration = 0,                                   // TabWidget.qss:46
        };

        //! Tab strips drawn in dock title bars (DockTabBar outside a DockTabWidget): no bar fill,
        //! no top or left edge. Close button as in DockGroup.
        const TabElement DockTitle{
            .m_name = "Global.Tab.DockTitle",
            .m_barMinHeight = 28,                                           // TabWidget.qss:46
            // fill and edges TabWidget.qss:57 (hover and selected fill :87, selected bottom edge :82).
            // No top edge: TitleBar.qss:153. No left edge on any tab: TitleBar.qss:147 (::tab:only,
            // applied to every tab as in DockGroup).
            .m_idle = { Grey333, White, { {}, { 1, Black111 }, { 1, Black111 }, {} } },
            .m_hover = { Grey444, White, { {}, { 1, Black111 }, { 1, Black111 }, {} } },
            .m_selected = { Grey444, White, { {}, { 1, Black111 }, { 1, Grey444 }, {} } },
            .m_selectedHover = { Grey444, White, { {}, { 1, Black111 }, { 1, Grey444 }, {} } },
            .m_padding = QMargins(9, 6, 21, 6),                             // TabWidget.qss:57
            .m_maxContentWidth = 200,                                       // TabWidget.qss:16
            .m_closeHoverFill = CloseRed,                                   // TitleBar.qss:103
            .m_moveAnimationDuration = 0,                                   // TabWidget.qss:46
        };

        //! TabWidget::applySecondaryStyle(widget, false): Editor tab widgets, Asset Editor, Gallery.
        const TabElement Secondary{
            .m_name = "Global.Tab.Secondary",
            .m_barFill = Grey444,                                           // TabWidget.qss:26
            .m_barMinHeight = 28,                                           // TabWidget.qss:46
            .m_idle = { Grey444, Muted959, { {}, {}, { 3, Grey333 }, {} } },                     // TabWidget.qss:136
            .m_hover = { HoverVeil, White, { {}, {}, { 3, Grey444 }, {} } },                     // TabWidget.qss:145
            .m_selected = { Grey444, White, { {}, {}, { 3, SelectedUnderline }, {} } },          // fill :136, text and edge :158
            .m_selectedHover = { HoverVeil, White, { {}, {}, { 3, SelectedUnderline }, {} } },   // fill :145, text and edge :158
            .m_padding = QMargins(14, 7, 14, 5),                            // TabWidget.qss:136
            .m_maxContentWidth = 200,                                       // TabWidget.qss:16
            .m_closeIcon = CloseIcon,                                       // TabWidget.qss:64
            .m_closeHoverFill = CloseHover,                                 // TabWidget.qss:70
            .m_closeRadius = 2,                                             // TabWidget.qss:70
            .m_closeMarginRight = 4,                                        // TabWidget.qss:64
            .m_widgetFill = Grey444,                                        // TabWidget.qss:101
            .m_paneEdges = { { 2, PaneLine }, {}, {}, {} },                 // TabWidget.qss:111
            .m_panePadding = QMargins(3, 0, 3, 3),                          // TabWidget.qss:111
            .m_paneEmptyOffset = 30,                                        // TabWidget.qss:41
            .m_barLeft = 1,                                                 // TabWidget.qss:119
            .m_barAreaMinHeight = 30,                                       // TabWidget.qss:119
            .m_cornerMinHeight = 28,                                        // TabWidgetActionToolBar.qss:1
            .m_cornerMarginBottom = 9,                                      // TabWidgetActionToolBar.qss:1
            .m_moveAnimationDuration = 0,                                   // TabWidget.qss:46
        };

        //! TabWidget::applySecondaryStyle(widget, true): Gallery, Lua IDE, Material and Pass
        //! Canvas documents, every log panel.
        const TabElement SecondaryBordered{
            .m_name = "Global.Tab.Secondary.Bordered",
            .m_barFill = Grey444,                                           // TabWidget.qss:26
            .m_barEdges = { { 1, Black111 }, {}, {}, { 1, Black111 } },     // TabWidget.qss:130
            .m_barMinHeight = 28,                                           // TabWidget.qss:46
            // as Secondary (TabWidget.qss:136, :145, :158), plus the top edge of TabWidget.qss:152
            .m_idle = { Grey444, Muted959, { { 1, Black111 }, {}, { 3, Grey333 }, {} } },
            .m_hover = { HoverVeil, White, { { 1, Black111 }, {}, { 3, Grey444 }, {} } },
            .m_selected = { Grey444, White, { { 1, Black111 }, {}, { 3, SelectedUnderline }, {} } },
            .m_selectedHover = { HoverVeil, White, { { 1, Black111 }, {}, { 3, SelectedUnderline }, {} } },
            .m_padding = QMargins(14, 6, 14, 6),                            // TabWidget.qss:152
            .m_maxContentWidth = 200,                                       // TabWidget.qss:16
            .m_closeIcon = CloseIcon,                                       // TabWidget.qss:64
            .m_closeHoverFill = CloseHover,                                 // TabWidget.qss:70
            .m_closeRadius = 2,                                             // TabWidget.qss:70
            .m_closeMarginRight = 4,                                        // TabWidget.qss:64
            .m_widgetFill = Grey444,                                        // TabWidget.qss:101
            .m_widgetEdges = box(1, Black111),                              // TabWidget.qss:106
            .m_paneEdges = { { 2, PaneLine }, {}, {}, {} },                 // TabWidget.qss:111
            .m_panePadding = QMargins(3, 0, 3, 3),                          // TabWidget.qss:111
            .m_paneEmptyOffset = 30,                                        // TabWidget.qss:41
            .m_barLeft = 1,                                                 // TabWidget.qss:119
            .m_barAreaMinHeight = 30,                                       // TabWidget.qss:119
            .m_cornerMinHeight = 28,                                        // TabWidgetActionToolBar.qss:1
            .m_cornerMarginBottom = 9,                                      // TabWidgetActionToolBar.qss:1
            .m_moveAnimationDuration = 0,                                   // TabWidget.qss:46
        };

        //! Plain tab widgets and tab bars, which no other element covers (a QTabWidget of Qt's, as the Asset Processor's
        //! Builders and Assets pages): the tabs and the pane are Qt's own (Fusion's), over the palette TabWidget.qss:10
        //! gave them. That rule's background-color filled the bar and became the palette's Base, Button and Window
        //! (qstylesheetstyle.cpp:1434-1441); Fusion derives the look from them: the pane #535353, the selected tab #565656
        //! fading to #535353, an idle tab #4d4d4d (qfusionstyle.cpp:783-785, 1671-1686; the Asset Processor captures of
        //! Builders and Assets show those). They are Fusion's values and not carried: the colour the rule gave is. No rule
        //! gives a plain tab a hover, pressed or focus look, a margin, padding, edge or size but the width bound.
        const TabElement Plain{
            .m_name = "Global.Tab.Plain",
            .m_barFill = Grey444,                                           // TabWidget.qss:10
            .m_maxContentWidth = 200,                                       // TabWidget.qss:16
            .m_basePalette = Grey444,                                       // TabWidget.qss:10
        };

        // ------------------------------------------------------------------------------
        // Registry
        // ------------------------------------------------------------------------------

        struct Registration
        {
            const TabElement* m_element;
            TabElement::Matcher m_matcher;
        };

        QVector<Registration>& registrations()
        {
            static QVector<Registration> list;
            return list;
        }

        const TabElement* globalElement(const QWidget* tabBar)
        {
            if (tabBar->inherits("AzQtComponents::DockTabBar"))
            {
                const QWidget* parent = tabBar->parentWidget();
                return (parent && parent->inherits("AzQtComponents::DockTabWidget")) ? &DockGroup : &DockTitle;
            }
            if (tabBar->inherits("AzQtComponents::TabBar"))
            {
                if (Style::hasClass(tabBar, QStringLiteral("Secondary")))
                {
                    return Style::hasClass(tabBar, QStringLiteral("Bordered")) ? &SecondaryBordered : &Secondary;
                }
                return &Primary;
            }
            // Any other QTabBar is a plain one; a widget that is none is not ours
            return tabBar->inherits("QTabBar") ? &Plain : nullptr;
        }

        //! The element kept on a bar (resolved at its polish; now, if it was asked before), null for none
        const TabElement* tabElementOf(const QWidget* tabBar)
        {
            if (!tabBar)
            {
                return nullptr;
            }
            const QVariant stored = tabBar->property(ElementProperty);
            return stored.isValid() ? reinterpret_cast<const TabElement*>(stored.value<quintptr>()) : TabElement::resolve(tabBar);
        }

        //! The bar of a tab widget, null for none
        const QWidget* tabBarOf(const QWidget* tabWidget)
        {
            if (tabWidget)
            {
                for (const QObject* child : tabWidget->children())
                {
                    if (child->isWidgetType() && child->inherits("QTabBar"))
                    {
                        return static_cast<const QWidget*>(child);
                    }
                }
            }
            return nullptr;
        }

        QSize naturalSize(const QString& image)
        {
            static QHash<QString, QSize> sizes;
            auto found = sizes.find(image);
            if (found == sizes.end())
            {
                found = sizes.insert(image, QImageReader(image).size());
            }
            return found.value();
        }

        int horizontal(const TabElement::Edges& edges)
        {
            return edges.m_left.m_width + edges.m_right.m_width;
        }

        int vertical(const TabElement::Edges& edges)
        {
            return edges.m_top.m_width + edges.m_bottom.m_width;
        }
    } // namespace

    // ----------------------------------------------------------------------------------
    // Use
    // ----------------------------------------------------------------------------------

    const TabElement::TabFace& TabElement::face(const QStyleOption* option) const
    {
        const bool enabled = option->state.testFlag(QStyle::State_Enabled);
        const bool pressed = enabled && option->state.testFlag(QStyle::State_Sunken);
        const bool hover = enabled && (option->state.testFlag(QStyle::State_MouseOver) || pressed);
        const bool selected = option->state.testFlag(QStyle::State_Selected);

        // Pressed and focus are states of the current tab (m_pressed, m_focus); the later rule wins (PM.qss:122-143)
        if (selected && option->state.testFlag(QStyle::State_HasFocus) && m_focus.isSet())
        {
            return m_focus;
        }
        if (selected && pressed && m_pressed.isSet())
        {
            return m_pressed;
        }
        if (selected)
        {
            return hover ? m_selectedHover : m_selected;
        }
        return hover ? m_hover : m_idle;
    }

    QSize TabElement::tabSize(const QSize& contents) const
    {
        // QStyleSheetStyle CT_TabBarTab: the size rules bound the contents (the maximum first, the minimum over
        // it, and a height rule replaces the text's: QRenderRule::adjustSize), then margin, padding and border
        // are added around them.
        int width = contents.width();
        if (m_maxContentWidth > 0)
        {
            width = qMin(width, m_maxContentWidth);
        }
        width = qMax(width, m_minContentWidth);
        const int height = (m_contentHeight > 0) ? m_contentHeight : contents.height();
        return QSize(width + m_margin.left() + m_margin.right() + m_padding.left() + m_padding.right() + horizontal(m_idle.m_edges),
            height + m_margin.top() + m_margin.bottom() + m_padding.top() + m_padding.bottom() + vertical(m_idle.m_edges));
    }

    QRect TabElement::contentsRect(const QRect& tabRect) const
    {
        const Edges& edges = m_idle.m_edges;
        return tabRect.adjusted(m_margin.left() + edges.m_left.m_width + m_padding.left(), m_margin.top() + edges.m_top.m_width + m_padding.top(),
            -(m_margin.right() + edges.m_right.m_width + m_padding.right()), -(m_margin.bottom() + edges.m_bottom.m_width + m_padding.bottom()));
    }

    void TabElement::drawTab(QPainter* painter, const QStyleOption* option) const
    {
        // The stylesheet drew the background and the border in the tab's box: its rect inside the margin
        const TabFace& tabFace = face(option);
        const QRect tabBox = option->rect.marginsRemoved(m_margin);
        if (tabFace.m_fill.isValid())
        {
            painter->fillRect(tabBox, tabFace.m_fill);
        }
        drawEdges(painter, tabBox, tabFace.m_edges);
    }

    void TabElement::drawClose(QPainter* painter, const QStyleOption* option) const
    {
        // QTabBar::close-button { image; margin-right } with :hover { background; border-radius }
        const QRect area = option->rect.adjusted(0, 0, -m_closeMarginRight, 0);
        const bool hover = option->state.testFlag(QStyle::State_MouseOver) || option->state.testFlag(QStyle::State_Sunken);
        if (hover && m_closeHoverFill.isValid())
        {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            QPainterPath path;
            path.addRoundedRect(QRectF(area), m_closeRadius, m_closeRadius);
            painter->fillPath(path, m_closeHoverFill);
            painter->restore();
        }
        // The stylesheet draws the image at its own size, centered (no scaling).
        const QString icon = QString::fromLatin1(m_closeIcon);
        QRect imageRect(QPoint(0, 0), naturalSize(icon));
        imageRect.moveCenter(area.center());
        QIcon(icon).paint(painter, imageRect);
    }

    void TabElement::drawEdges(QPainter* painter, const QRect& rect, const Edges& edges)
    {
        // QRenderRule::drawBorder draws the edges antialiased (qstylesheetstyle.cpp:1282-1285), each a strip as long as its side, in qDrawBorder's
        // order (bottom, right, left, top: a later edge takes the corner; qcssutil.cpp:313-369, qDrawEdge :165-195). At a fractional device
        // ratio a 1px edge is 1.5 device pixels wide: a full row and a blended one, where a fill with no antialiasing covers one row or two
        // (n2 Gallery 27_tab-widget: the tab edges are #111111 over #444444 as 17, 42, not 17, 17). Qt draws an edge wider than 1px that meets
        // a visible edge of another colour as a trapezoid (qDrawEdge :165-195, paintsOver :296-311): no element has one.
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(Qt::NoPen);
        const QRectF box(rect);
        if (edges.m_bottom.m_width > 0)
        {
            painter->setBrush(edges.m_bottom.m_color);
            painter->drawRect(QRectF(box.left(), box.bottom() - edges.m_bottom.m_width, box.width(), edges.m_bottom.m_width));
        }
        if (edges.m_right.m_width > 0)
        {
            painter->setBrush(edges.m_right.m_color);
            painter->drawRect(QRectF(box.right() - edges.m_right.m_width, box.top(), edges.m_right.m_width, box.height()));
        }
        if (edges.m_left.m_width > 0)
        {
            painter->setBrush(edges.m_left.m_color);
            painter->drawRect(QRectF(box.left(), box.top(), edges.m_left.m_width, box.height()));
        }
        if (edges.m_top.m_width > 0)
        {
            painter->setBrush(edges.m_top.m_color);
            painter->drawRect(QRectF(box.left(), box.top(), box.width(), edges.m_top.m_width));
        }
        painter->restore();
    }

    void TabElement::applyPalette(QPalette& palette) const
    {
        // QRenderRule::configurePalette: a rule's background-color is the palette's Base, Button and Window, in every group
        palette.setColor(QPalette::Base, m_basePalette);
        palette.setColor(QPalette::Button, m_basePalette);
        palette.setColor(QPalette::Window, m_basePalette);
    }

    // ----------------------------------------------------------------------------------
    // Resolution
    // ----------------------------------------------------------------------------------

    void TabElement::registerElement(const TabElement* element, Matcher matcher)
    {
        registrations().append({ element, matcher });
        if (!StyleManager::stylesheetsDisabled())
        {
            return;
        }

        // A module builds some windows before it registers its elements: the bars resolved before this call are
        // resolved again (a null kept, too, or a cached element), and those polished are polished again with
        // their tab widget when the element changed. A bar not polished yet resolves at its polish.
        const QWidgetList widgets = QApplication::allWidgets();
        for (QWidget* widget : widgets)
        {
            if (!widget->inherits("QTabBar"))
            {
                continue;
            }
            const QVariant before = widget->property(ElementProperty);
            widget->setProperty(ElementProperty, QVariant());
            if (!widget->testAttribute(Qt::WA_WState_Polished))
            {
                continue;
            }
            resolve(widget);
            if (widget->property(ElementProperty) != before)
            {
                polish(widget);
                polish(widget->parentWidget());
            }
        }
    }

    const TabElement* TabElement::resolve(const QWidget* tabBar)
    {
        if (!tabBar)
        {
            return nullptr;
        }
        const TabElement* element = nullptr;
        for (const Registration& registration : registrations())
        {
            if (registration.m_matcher(tabBar))
            {
                element = registration.m_element;
                break;
            }
        }
        if (!element)
        {
            element = globalElement(tabBar);
        }
        // Kept on the widget: draws and metrics read it back without resolving again (a null
        // element is kept too, so a widget that is no tab bar is not resolved on every paint).
        const_cast<QWidget*>(tabBar)->setProperty(ElementProperty, QVariant::fromValue(reinterpret_cast<quintptr>(element)));
        return element;
    }

    const TabElement* TabElement::forTabBar(const QWidget* tabBar)
    {
        const TabElement* element = tabElementOf(tabBar);
        return (element && !element->isPlain()) ? element : nullptr;
    }

    const TabElement* TabElement::forTabWidget(const QWidget* tabWidget)
    {
        return forTabBar(tabBarOf(tabWidget));
    }

    const TabElement* TabElement::plainForTabBar(const QWidget* tabBar)
    {
        const TabElement* element = tabElementOf(tabBar);
        return (element && element->isPlain()) ? element : nullptr;
    }

    const TabElement* TabElement::plainForTabWidget(const QWidget* tabWidget)
    {
        return plainForTabBar(tabBarOf(tabWidget));
    }

    // ----------------------------------------------------------------------------------
    // Polish
    // ----------------------------------------------------------------------------------

    void TabElement::polish(QWidget* widget)
    {
        if (auto tabBar = qobject_cast<QTabBar*>(widget))
        {
            // Resolved once, here: adding or removing a class tag polishes again, and so does a module that registers
            // an element after this bar was polished (registerElement)
            const TabElement* element = resolve(tabBar);
            if (!element)
            {
                return;
            }
            // The bar and its tab widget paint their own boxes through PE_Widget (the sheet's rule gave them a
            // background; a plain bar's fill is its sheet's background-color, TabWidget.qss:10)
            tabBar->setAttribute(Qt::WA_StyledBackground, true);
            tabBar->setAutoFillBackground(false);
            if (element->isPlain())
            {
                return;
            }
            tabBar->setDrawBase(false);
            // The stylesheet set the minimum outright (replacing a code-set one, e.g. the DockTabBar's fixed 32), as
            // min-height plus the bar's own edges; an element no rule gives one leaves the widget's own
            if (element->m_barMinHeight > 0)
            {
                tabBar->setMinimumHeight(element->m_barMinHeight + element->m_barEdges.m_top.m_width + element->m_barEdges.m_bottom.m_width);
            }
            // QTabBar follows the hovered tab only while it has WA_Hover (qtabbar.cpp:1687-1718); the stylesheet engine
            // switched it on for a bar with a :hover rule (qstylesheetstyle.cpp:2894-2905)
            if (element->m_hover.isSet())
            {
                tabBar->setAttribute(Qt::WA_Hover, true);
            }
        }
        else if (auto tabWidget = qobject_cast<QTabWidget*>(widget))
        {
            const TabElement* element = forTabWidget(tabWidget);
            if (!element)
            {
                return;
            }
            tabWidget->setAttribute(Qt::WA_StyledBackground, true);
            tabWidget->setAutoFillBackground(false);
            if (element->m_widgetHeight > 0)
            {
                // height + max-height in the stylesheet: the widget is that tall
                tabWidget->setMinimumHeight(element->m_widgetHeight);
                tabWidget->setMaximumHeight(element->m_widgetHeight);
            }
            // The action toolbar container in the top-right corner: min-height plus
            // margin-bottom make its height, and its contents sit above the margin
            QWidget* corner = tabWidget->cornerWidget(Qt::TopRightCorner);
            if (corner && corner->inherits("AzQtComponents::TabWidgetActionToolBarContainer") && element->m_cornerMinHeight > 0)
            {
                corner->setMinimumHeight(element->m_cornerMinHeight + element->m_cornerMarginBottom);
                corner->setContentsMargins(0, 0, 0, element->m_cornerMarginBottom);
            }
        }
    }
} // namespace AzQtComponents
