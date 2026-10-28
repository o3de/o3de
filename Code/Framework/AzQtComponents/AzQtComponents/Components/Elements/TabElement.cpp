/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Elements/TabElement.h>
#include <AzQtComponents/Components/Style.h>

#include <QBrush>
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
#include <QWidget>

namespace AzQtComponents
{
    //! Private to this file: named, not anonymous, as a unity build merges the anonymous namespaces of its files
    namespace TabElementInternal
    {
        // ------------------------------------------------------------------------------
        // Values
        // ------------------------------------------------------------------------------

        // ---- Every tab bar
        constexpr int TabMaxWidth = 200;                            // TabWidget.qss:18 $size.tab.idle.max-width
        const QColor PlainFill(0x44, 0x44, 0x44);                   // TabWidget.qss:13 $fill.tab.idle.Mid

        // ---- AzQtComponents' TabBar: the bar, and a tab of the Primary look
        const QColor BarFill(0x44, 0x44, 0x44);                     // TabWidget.qss:28 $fill.surface.idle.Mid
        constexpr int BarMinHeight = 28;                            // TabWidget.qss:51 $size.tab.idle.min-height.Alt1
        constexpr int MoveAnimationDuration = 0;                    // TabWidget.qss:54 $other.tab.idle.widget-animation-duration
        const QColor TabFill(0x33, 0x33, 0x33);                     // TabWidget.qss:60 $fill.tab.idle.Low
        // TabWidget.qss:59 $line.tab.idle.width.Alt1 $line.tab.idle.color.FarBG
        const TabElement::Edge TabLine{ 1, QColor(0x11, 0x11, 0x11) };
        constexpr QMargins TabPadding(9, 6, 21, 6);                 // TabWidget.qss:61 $space.tab.idle.padding.Alt1
        const QColor TabText(0xFF, 0xFF, 0xFF);                     // BaseStyleSheet.qss:18 $text.surface.idle.Alt1
        const QColor HoverFill(0x44, 0x44, 0x44);                   // TabWidget.qss:90 $fill.tab.hover.Mid
        const QColor SelectedFill(0x44, 0x44, 0x44);                // TabWidget.qss:90 $fill.tab.selected.Mid
        // TabWidget.qss:84 $line.tab.selected.width.Alt1 $line.tab.selected.color.Mid
        const TabElement::Edge SelectedLine{ 1, QColor(0x44, 0x44, 0x44) };

        // ---- A tab of the Secondary look, and of the bordered one
        const QColor SecondaryFill(0x44, 0x44, 0x44);               // TabWidget.qss:138 $fill.tab.idle.Mid
        // TabWidget.qss:140 $line.tab.idle.width.Alt3 $line.tab.idle.color.Low
        const TabElement::Edge SecondaryLine{ 3, QColor(0x33, 0x33, 0x33) };
        constexpr QMargins SecondaryPadding(14, 7, 14, 5);          // TabWidget.qss:141 $space.tab.idle.padding.Alt3
        const QColor SecondaryText(0x95, 0x95, 0x95);               // TabWidget.qss:142 $text.tab.idle
        const QColor SecondaryHoverFill(0xFF, 0xFF, 0xFF, 0x26);    // TabWidget.qss:147 $fill.tab.hover.Alt1
        // TabWidget.qss:148 $line.tab.hover.width $line.tab.hover.color
        const TabElement::Edge SecondaryHoverLine{ 3, QColor(0x44, 0x44, 0x44) };
        const QColor SecondaryHoverText(0xFF, 0xFF, 0xFF);          // TabWidget.qss:149 $text.tab.hover.Alt1
        // TabWidget.qss:160 $line.tab.selected.width.Alt2 $line.tab.selected.color.Alt1
        const TabElement::Edge SecondarySelectedLine{ 3, QColor(0x00, 0x8D, 0xE5) };
        const QColor SecondarySelectedText(0xFF, 0xFF, 0xFF);       // TabWidget.qss:161 $text.tab.selected.Alt1
        // TabWidget.qss:154 $line.tab.idle.width.Alt1 $line.tab.idle.color.FarBG
        const TabElement::Edge BorderedTopLine{ 1, QColor(0x11, 0x11, 0x11) };
        constexpr QMargins BorderedPadding(14, 6, 14, 6);           // TabWidget.qss:155 $space.tab.idle.padding.Alt2
        // TabWidget.qss:132-133 $line.tab.idle.width.Alt1 $line.tab.idle.color.FarBG (one rule for the left and top edges)
        const TabElement::Edge BorderedBarLine{ 1, QColor(0x11, 0x11, 0x11) };

        // ---- The close button of a tab, and the hover fill of a dock tab's (a DockBarButton)
        constexpr const char* CloseIcon = ":/stylesheet/img/close_small.svg";   // TabWidget.qss:66 $asset.button.idle.image
        constexpr int CloseMarginRight = 4;                         // TabWidget.qss:67 $space.button.idle.margin-right.Alt1
        const QColor CloseHoverFill(0xFF, 0xFF, 0xFF, 0x33);        // TabWidget.qss:72 $fill.button.hover.fill.Alt1
        constexpr int CloseHoverRadius = 2;                         // TabWidget.qss:73 $radius.button.hover
        const QColor DockCloseHoverFill(0xB8, 0x0D, 0x1C);          // TitleBar.qss:104 $fill.button.hover.fill.Alt3

        // ---- The tab widget, its pane and the action toolbar container in its corner
        const QColor WidgetFill(0x11, 0x11, 0x11);                  // TabWidget.qss:23 $fill.tab.idle.FarBG
        constexpr int EmptyPaneOffset = 30;                         // TabWidget.qss:43 $space.tab-pane.idle.margin-top
        constexpr int CornerMinHeight = 28;                 // TabWidgetActionToolBar.qss:4, :29 $size.tab.idle.min-height.Alt1
        constexpr int CornerMarginBottom = 9;               // TabWidgetActionToolBar.qss:6, :31 $space.tab.idle.margin-bottom.Alt2
        const QColor SecondaryWidgetFill(0x44, 0x44, 0x44);         // TabWidget.qss:103 $fill.tab.idle.Mid
        // TabWidget.qss:108 $line.tab.idle.width.Alt1 $line.tab.idle.color.FarBG
        const TabElement::Edge BorderedWidgetLine{ 1, QColor(0x11, 0x11, 0x11) };
        // TabWidget.qss:114 $line.tab-pane.idle.width.Alt2 $line.tab-pane.idle.color
        const TabElement::Edge SecondaryPaneLine{ 2, QColor(0x2B, 0x2B, 0x2B) };
        // TabWidget.qss:115, :116 $space.tab-pane.idle.padding $space.tab-pane.idle.padding-top
        constexpr QMargins SecondaryPanePadding(3, 0, 3, 3);
        constexpr int SecondaryBarLeft = 1;                         // TabWidget.qss:121 $other.tab.idle.left.Alt1
        constexpr int SecondaryBarAreaMinHeight = 30;               // TabWidget.qss:122 $size.tab.idle.min-height.Alt2

        // ------------------------------------------------------------------------------
        // Elements
        // ------------------------------------------------------------------------------

        //! AzQtComponents' TabBar (Gallery tab widgets, the Asset Bundler, the Lua IDE's find). A selected tab under the pointer has
        //! the hover fill (TabWidget.qss:90 names both states) and the selected bottom edge.
        const TabElement Primary{
            .m_name = "Global.Tab.Primary",
            .m_barFill = BarFill,
            .m_barMinHeight = BarMinHeight,
            .m_idle = { .m_fill = TabFill, .m_text = TabText, .m_edges = { TabLine, TabLine, TabLine, TabLine } },
            .m_hover = { .m_fill = HoverFill, .m_text = TabText, .m_edges = { TabLine, TabLine, TabLine, TabLine } },
            .m_selected = { .m_fill = SelectedFill, .m_text = TabText, .m_edges = { TabLine, TabLine, SelectedLine, TabLine } },
            .m_selectedHover = { .m_fill = HoverFill, .m_text = TabText, .m_edges = { TabLine, TabLine, SelectedLine, TabLine } },
            .m_padding = TabPadding,
            .m_maxContentWidth = TabMaxWidth,
            .m_closeIcon = CloseIcon,
            .m_closeHoverFill = CloseHoverFill,
            .m_closeRadius = CloseHoverRadius,
            .m_closeMarginRight = CloseMarginRight,
            .m_widgetFill = WidgetFill,
            .m_paneEmptyOffset = EmptyPaneOffset,
            .m_cornerMinHeight = CornerMinHeight,
            .m_cornerMarginBottom = CornerMarginBottom,
            .m_moveAnimationDuration = MoveAnimationDuration,
        };

        //! A DockTabBar in a DockTabWidget (docked panel groups): the Primary look with no left edge on any tab. StyledDockWidget.qss:33
        //! names `::tab:only`, which is no Qt pseudo-class (Qt's is :only-one), so Qt applies the rule to every tab. The close button
        //! is a DockBarButton (DockTabBar::tabInserted); the element gives only its hover fill.
        const TabElement DockGroup{
            .m_name = "Global.Tab.DockGroup",
            .m_barFill = BarFill,
            .m_barMinHeight = BarMinHeight,
            .m_idle = { .m_fill = TabFill, .m_text = TabText, .m_edges = { TabLine, TabLine, TabLine, {} } },
            .m_hover = { .m_fill = HoverFill, .m_text = TabText, .m_edges = { TabLine, TabLine, TabLine, {} } },
            .m_selected = { .m_fill = SelectedFill, .m_text = TabText, .m_edges = { TabLine, TabLine, SelectedLine, {} } },
            .m_selectedHover = { .m_fill = HoverFill, .m_text = TabText, .m_edges = { TabLine, TabLine, SelectedLine, {} } },
            .m_padding = TabPadding,
            .m_maxContentWidth = TabMaxWidth,
            .m_closeHoverFill = DockCloseHoverFill,
            .m_widgetFill = WidgetFill,
            .m_paneEmptyOffset = EmptyPaneOffset,
            .m_cornerMinHeight = CornerMinHeight,
            .m_cornerMarginBottom = CornerMarginBottom,
            .m_moveAnimationDuration = MoveAnimationDuration,
        };

        //! A DockTabBar in a dock's title bar (outside a DockTabWidget): the Primary look with no bar fill (the title bar shows), no
        //! top edge (TitleBar.qss:156) and no left edge on any tab (TitleBar.qss:150, `::tab:only` as in DockGroup). Its close button
        //! is DockGroup's.
        const TabElement DockTitle{
            .m_name = "Global.Tab.DockTitle",
            .m_barMinHeight = BarMinHeight,
            .m_idle = { .m_fill = TabFill, .m_text = TabText, .m_edges = { {}, TabLine, TabLine, {} } },
            .m_hover = { .m_fill = HoverFill, .m_text = TabText, .m_edges = { {}, TabLine, TabLine, {} } },
            .m_selected = { .m_fill = SelectedFill, .m_text = TabText, .m_edges = { {}, TabLine, SelectedLine, {} } },
            .m_selectedHover = { .m_fill = HoverFill, .m_text = TabText, .m_edges = { {}, TabLine, SelectedLine, {} } },
            .m_padding = TabPadding,
            .m_maxContentWidth = TabMaxWidth,
            .m_closeHoverFill = DockCloseHoverFill,
            .m_moveAnimationDuration = MoveAnimationDuration,
        };

        //! TabWidget::applySecondaryStyle(widget, false): Editor tab widgets, the Asset Editor, the Gallery. A selected tab under the
        //! pointer has the hover fill and the selected text and bottom edge (TabWidget.qss:158 follows :145).
        const TabElement Secondary{
            .m_name = "Global.Tab.Secondary",
            .m_barFill = BarFill,
            .m_barMinHeight = BarMinHeight,
            .m_idle = { .m_fill = SecondaryFill, .m_text = SecondaryText, .m_edges = { {}, {}, SecondaryLine, {} } },
            .m_hover = { .m_fill = SecondaryHoverFill, .m_text = SecondaryHoverText, .m_edges = { {}, {}, SecondaryHoverLine, {} } },
            .m_selected = { .m_fill = SecondaryFill, .m_text = SecondarySelectedText, .m_edges = { {}, {}, SecondarySelectedLine, {} } },
            .m_selectedHover = { .m_fill = SecondaryHoverFill, .m_text = SecondarySelectedText,
                .m_edges = { {}, {}, SecondarySelectedLine, {} } },
            .m_padding = SecondaryPadding,
            .m_maxContentWidth = TabMaxWidth,
            .m_closeIcon = CloseIcon,
            .m_closeHoverFill = CloseHoverFill,
            .m_closeRadius = CloseHoverRadius,
            .m_closeMarginRight = CloseMarginRight,
            .m_widgetFill = SecondaryWidgetFill,
            .m_paneEdges = { SecondaryPaneLine, {}, {}, {} },
            .m_panePadding = SecondaryPanePadding,
            .m_paneEmptyOffset = EmptyPaneOffset,
            .m_barLeft = SecondaryBarLeft,
            .m_barAreaMinHeight = SecondaryBarAreaMinHeight,
            .m_cornerMinHeight = CornerMinHeight,
            .m_cornerMarginBottom = CornerMarginBottom,
            .m_moveAnimationDuration = MoveAnimationDuration,
        };

        //! TabWidget::applySecondaryStyle(widget, true): the Gallery, the Lua IDE, the Material and Pass Canvas documents, every log
        //! panel. The Secondary look with a top edge on the bar and on every tab, and a border around the tab widget.
        const TabElement SecondaryBordered{
            .m_name = "Global.Tab.Secondary.Bordered",
            .m_barFill = BarFill,
            .m_barEdges = { BorderedBarLine, {}, {}, BorderedBarLine },
            .m_barMinHeight = BarMinHeight,
            .m_idle = { .m_fill = SecondaryFill, .m_text = SecondaryText, .m_edges = { BorderedTopLine, {}, SecondaryLine, {} } },
            .m_hover = { .m_fill = SecondaryHoverFill, .m_text = SecondaryHoverText,
                .m_edges = { BorderedTopLine, {}, SecondaryHoverLine, {} } },
            .m_selected = { .m_fill = SecondaryFill, .m_text = SecondarySelectedText,
                .m_edges = { BorderedTopLine, {}, SecondarySelectedLine, {} } },
            .m_selectedHover = { .m_fill = SecondaryHoverFill, .m_text = SecondarySelectedText,
                .m_edges = { BorderedTopLine, {}, SecondarySelectedLine, {} } },
            .m_padding = BorderedPadding,
            .m_maxContentWidth = TabMaxWidth,
            .m_closeIcon = CloseIcon,
            .m_closeHoverFill = CloseHoverFill,
            .m_closeRadius = CloseHoverRadius,
            .m_closeMarginRight = CloseMarginRight,
            .m_widgetFill = SecondaryWidgetFill,
            .m_widgetEdges = { BorderedWidgetLine, BorderedWidgetLine, BorderedWidgetLine, BorderedWidgetLine },
            .m_paneEdges = { SecondaryPaneLine, {}, {}, {} },
            .m_panePadding = SecondaryPanePadding,
            .m_paneEmptyOffset = EmptyPaneOffset,
            .m_barLeft = SecondaryBarLeft,
            .m_barAreaMinHeight = SecondaryBarAreaMinHeight,
            .m_cornerMinHeight = CornerMinHeight,
            .m_cornerMarginBottom = CornerMarginBottom,
            .m_moveAnimationDuration = MoveAnimationDuration,
        };

        //! Plain tab widgets and bars, which no other element covers (a QTabWidget of Qt's, as the Asset Processor's Builders and
        //! Assets pages). The rule of TabWidget.qss:13 filled the bar and its background-color became the palette's Base, Button and
        //! Window (QRenderRule::configurePalette); the tabs and the pane are the base style's over that palette. No rule gives a plain
        //! tab a state, margin, padding, edge or size but the width bound.
        const TabElement Plain{
            .m_name = "Global.Tab.Plain",
            .m_barFill = PlainFill,
            .m_maxContentWidth = TabMaxWidth,
            .m_basePalette = PlainFill,
        };

        // ------------------------------------------------------------------------------
        // Helpers
        // ------------------------------------------------------------------------------

        ElementRegistry<TabElement>& registry()
        {
            static ElementRegistry<TabElement> rows("_o3de_tabElement");
            return rows;
        }

        //! The Global element of a tab bar; none for a widget that is no tab bar
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
            return tabBar->inherits("QTabBar") ? &Plain : nullptr;
        }

        //! The element of a tab bar: a module's first, then the Global one
        const TabElement* findElement(const QWidget* tabBar)
        {
            const TabElement* element = registry().find(tabBar);
            return element ? element : globalElement(tabBar);
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

        //! The size of an image file, read once
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

        //! Margin, edges and padding of a tab in one: its contents sit inside them. The edge widths are m_idle's, those of every face.
        QMargins boxOf(const TabElement& element)
        {
            const TabElement::Edges& edges = element.m_idle.m_edges;
            const QMargins widths(edges.m_left.m_width, edges.m_top.m_width, edges.m_right.m_width, edges.m_bottom.m_width);
            return element.m_margin + widths + element.m_padding;
        }
    } // namespace TabElementInternal

    // ----------------------------------------------------------------------------------
    // Geometry
    // ----------------------------------------------------------------------------------

    QSize TabElement::tabSize(const QSize& contents) const
    {
        // QStyleSheetStyle::sizeFromContents, CT_TabBarTab: the size rules bound the contents (the maximum first, the minimum over it,
        // and a height rule replaces the text's: QRenderRule::adjustSize), then margin, edges and padding are added around them
        int width = contents.width();
        if (m_maxContentWidth > 0)
        {
            width = qMin(width, m_maxContentWidth);
        }
        width = qMax(width, m_minContentWidth);
        const int height = (m_contentHeight > 0) ? m_contentHeight : contents.height();
        return QSize(width, height).grownBy(TabElementInternal::boxOf(*this));
    }

    QRect TabElement::contentsRect(const QRect& tabRect) const
    {
        return tabRect.marginsRemoved(TabElementInternal::boxOf(*this));
    }

    // ----------------------------------------------------------------------------------
    // Drawing
    // ----------------------------------------------------------------------------------

    const TabElement::TabFace& TabElement::face(const QStyleOption* option) const
    {
        const bool enabled = option->state.testFlag(QStyle::State_Enabled);
        const bool pressed = enabled && option->state.testFlag(QStyle::State_Sunken);
        const bool hover = enabled && (option->state.testFlag(QStyle::State_MouseOver) || pressed);
        const bool selected = option->state.testFlag(QStyle::State_Selected);

        // Focus, then pressed: states of the current tab only
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

    void TabElement::drawTab(QPainter* painter, const QStyleOption* option) const
    {
        // The rule drew the tab's box: its rect inside the margin
        const TabFace& tabFace = face(option);
        drawFrame(painter, option->rect.marginsRemoved(m_margin), tabFace.m_fill, tabFace.m_edges);
    }

    void TabElement::drawClose(QPainter* painter, const QStyleOption* option) const
    {
        // QTabBar::close-button { image; margin-right } with :hover { background; border-radius }
        const QRect area = option->rect.marginsRemoved(QMargins(0, 0, m_closeMarginRight, 0));
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
        // The image at its own size, centered
        const QString icon = QString::fromLatin1(m_closeIcon);
        QRect imageRect(QPoint(0, 0), TabElementInternal::naturalSize(icon));
        imageRect.moveCenter(area.center());
        QIcon(icon).paint(painter, imageRect);
    }

    void TabElement::drawFrame(QPainter* painter, const QRect& borderBox, const QColor& fill, const Edges& edges)
    {
        BoxElement::drawFrame(painter, borderBox, fill.isValid() ? QBrush(fill) : QBrush(), edges, BoxElement::Radii());
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

    void TabElement::registerElement(
        const TabElement* element, const QMetaObject* widgetClass, ElementMatcher matcher, ElementScope scope)
    {
        TabElementInternal::registry().add(element, widgetClass, matcher, scope);
        resolvePolished(widgetClass, [](QWidget* tabBar)
            {
                // A bar whose element changed is resolved again, with its tab widget
                if (TabElementInternal::findElement(tabBar) != forWidget(tabBar))
                {
                    resolve(tabBar);
                    resolve(tabBar->parentWidget());
                }
            });
    }

    void TabElement::resolve(QWidget* widget)
    {
        if (auto tabBar = qobject_cast<QTabBar*>(widget))
        {
            // Resolved once, here: adding or removing a class tag polishes again, and so does a module that registers an element
            // after this bar was polished (registerElement)
            const TabElement* element = TabElementInternal::findElement(tabBar);
            TabElementInternal::registry().keep(tabBar, element);
            if (!element)
            {
                return;
            }
            // The bar and its tab widget paint their own boxes through PE_Widget (a plain bar's fill is TabWidget.qss:13's)
            tabBar->setAttribute(Qt::WA_StyledBackground, true);
            tabBar->setAutoFillBackground(false);
            if (element->isPlain())
            {
                return;
            }
            // No base line under the tabs (qproperty-drawBase of AzQtComponents' TabBar, TabWidget.qss:48)
            tabBar->setDrawBase(false);
            // The sheet's minimum replaced one the code set (the DockTabBar's fixed 32), as min-height plus the bar's own edges
            if (element->m_barMinHeight > 0)
            {
                const Edges& edges = element->m_barEdges;
                tabBar->setMinimumHeight(element->m_barMinHeight + edges.m_top.m_width + edges.m_bottom.m_width);
            }
            // QTabBar follows the hovered tab only with WA_Hover (QTabBar::event); QStyleSheetStyle::polish set it for a bar with a
            // :hover rule
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
                // height + max-height: the widget is that tall
                tabWidget->setMinimumHeight(element->m_widgetHeight);
                tabWidget->setMaximumHeight(element->m_widgetHeight);
            }
            // The action toolbar container in the top-right corner: min-height plus margin-bottom make its height, and its contents
            // sit above the margin
            QWidget* corner = tabWidget->cornerWidget(Qt::TopRightCorner);
            if (corner && corner->inherits("AzQtComponents::TabWidgetActionToolBarContainer") && element->m_cornerMinHeight > 0)
            {
                corner->setMinimumHeight(element->m_cornerMinHeight + element->m_cornerMarginBottom);
                corner->setContentsMargins(0, 0, 0, element->m_cornerMarginBottom);
            }
        }
    }

    const TabElement* TabElement::forWidget(const QWidget* tabBar)
    {
        using namespace TabElementInternal;
        if (!tabBar)
        {
            return nullptr;
        }
        const std::optional<const TabElement*> kept = registry().kept(tabBar);
        return kept ? *kept : findElement(tabBar);
    }

    const TabElement* TabElement::forTabBar(const QWidget* tabBar)
    {
        const TabElement* element = forWidget(tabBar);
        return (element && !element->isPlain()) ? element : nullptr;
    }

    const TabElement* TabElement::forTabWidget(const QWidget* tabWidget)
    {
        return forTabBar(TabElementInternal::tabBarOf(tabWidget));
    }

    const TabElement* TabElement::plainForTabBar(const QWidget* tabBar)
    {
        const TabElement* element = forWidget(tabBar);
        return (element && element->isPlain()) ? element : nullptr;
    }

    const TabElement* TabElement::plainForTabWidget(const QWidget* tabWidget)
    {
        return plainForTabBar(TabElementInternal::tabBarOf(tabWidget));
    }
} // namespace AzQtComponents
