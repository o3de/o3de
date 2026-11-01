/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Elements/MenuElement.h>
#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/Foundations.h>

#include <AzCore/Debug/Trace.h>

#include <QApplication>
#include <QBrush>
#include <QMenu>
#include <QPainter>
#include <QPointer>
#include <QProxyStyle>
#include <QStyleFactory>
#include <QStyleOption>
#include <QWidget>

namespace AzQtComponents
{
    //! Private to this file: named, not anonymous, as a unity build merges the anonymous namespaces of its files
    namespace MenuElementInternal
    {
        // ------------------------------------------------------------------------------
        // Values
        // ------------------------------------------------------------------------------


        // ---- A popup menu
        const QColor MenuFill = Foundations::BG;                    // Menu.qss:12 $fill.menu.idle
        constexpr int MenuPaddingV = 4;                             // Menu.qss:13 $space.menu.idle (top and bottom)
        constexpr int MenuPaddingH = 2;                             // Menu.qss:13 $space.menu.idle (left and right)
        const QColor SelectedItemFill = Foundations::Mid;           // Menu.qss:20 $fill.item.selected.Mid
        const QColor DisabledItemText = Foundations::Highest;       // Menu.qss:25 $text.item.disabled.Highest
        constexpr int SeparatorLineWidth = 1;                       // Menu.qss:31 $line.separator.idle.width
        const QColor SeparatorLine(0x45, 0x45, 0x45);               // Menu.qss:31 $line.separator.idle.color
        constexpr int SeparatorMarginTop = 2;                       // Menu.qss:32 $space.separator.idle.margin-top
        constexpr int SeparatorMarginBottom = 3;                    // Menu.qss:33 $space.separator.idle.margin-bottom

        // ---- The menu bar
        // MenuBar.qss:11-12 $size.menubar.idle.min-height $size.menubar.idle.height
        constexpr int BarHeight = 28;
        constexpr int BarLineWidth = 2;                             // MenuBar.qss:17 $line.menubar.idle.width
        const QColor BarLine = Foundations::FarBG;                  // MenuBar.qss:18 $line.menubar.idle.color
        // MenuBar.qss:24-25 $space.item.idle.padding-left.Alt2 $space.item.idle.padding-right.Alt1
        constexpr int BarItemPadding = 14;
        const QColor BarSelectedItemFill = Foundations::Highest;    // MenuBar.qss:30 $fill.item.selected.Highest
        const QColor BarPressedItemFill = Foundations::BG;          // MenuBar.qss:35 $fill.item.pressed

        // ------------------------------------------------------------------------------
        // Elements
        // ------------------------------------------------------------------------------

        //! QMenu, its ::item:selected and :disabled, and ::separator (Menu.qss), over the colour of the universal rule. The
        //! separator's `height: 0px` (Menu.qss:30) leaves its box to its margins and its border.
        const MenuElement GlobalPopup{
            .m_name = "Global.Menu.Popup",
            .m_surface = MenuFill,
            .m_paddingH = MenuPaddingH,
            .m_paddingV = MenuPaddingV,
            .m_text = Foundations::Surface::Text,
            .m_selectedFill = SelectedItemFill,
            .m_disabledText = DisabledItemText,
            .m_separatorMarginTop = SeparatorMarginTop,
            .m_separatorBorder = SeparatorLineWidth,
            .m_separatorMarginBottom = SeparatorMarginBottom,
            .m_separatorLine = SeparatorLine,
        };

        //! QMenuBar, its ::item, :selected and :pressed (MenuBar.qss), over the colour of the universal rule. The bar's font is the
        //! QMenuBar class font (TextElement.cpp).
        const MenuBarElement GlobalBar{
            .m_name = "Global.Menu.Bar",
            .m_height = BarHeight,
            .m_borderWidth = BarLineWidth,
            .m_borderColor = BarLine,
            .m_itemPadding = BarItemPadding,
            .m_text = Foundations::Surface::Text,
            .m_selectedFill = BarSelectedItemFill,
            .m_pressedFill = BarPressedItemFill,
        };

        // ------------------------------------------------------------------------------
        // Helpers
        // ------------------------------------------------------------------------------

        //! A rule with a fill and a text colour, as QRenderRule::configurePalette applies it (BoxElement::configurePalette); an invalid
        //! colour is a declaration the rule does not have
        void configurePalette(
            QPalette& palette, const QColor& fill, const QColor& text, QPalette::ColorRole textRole, QPalette::ColorRole fillRole)
        {
            const BoxElement::Face rule{ .m_fill = fill.isValid() ? QBrush(fill) : QBrush(), .m_text = text };
            BoxElement::configurePalette(palette, rule, textRole, fillRole);
        }

        //! QWindowsStyle calls back into its proxy for an item's icon size, shortcut underline, check mark and arrow. The legacy proxy
        //! was the sheet style, which handed each to the base style (the fallbacks of QStyleSheetStyle::drawPrimitive, pixelMetric and
        //! styleHint), so here it is the style of the menu being drawn: the arrow is the base style's, and a shortcut is underlined
        //! as the base style says (QWindowsStyle itself waits for the Alt key, QWindowsStyle::styleHint).
        class WindowsItemStyle : public QProxyStyle
        {
        public:
            explicit WindowsItemStyle(QStyle* windows)
                : QProxyStyle(windows)
            {
            }

            int pixelMetric(PixelMetric metric, const QStyleOption* option, const QWidget* widget) const override
            {
                return styleOf(widget)->pixelMetric(metric, option, widget);
            }

            int styleHint(StyleHint hint, const QStyleOption* option, const QWidget* widget, QStyleHintReturn* returnData) const override
            {
                return styleOf(widget)->styleHint(hint, option, widget, returnData);
            }

            void drawPrimitive(
                PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const override
            {
                styleOf(widget)->drawPrimitive(element, option, painter, widget);
            }

        private:
            static QStyle* styleOf(const QWidget* widget)
            {
                return widget ? widget->style() : QApplication::style();
            }
        };

        //! The style that draws the items of every menu, made when first needed. It is a child of the application, so a later
        //! application (the unit tests make one per test) makes its own. Null when this Qt has no Windows style: QStyleFactory::create
        //! gives null then, which must not reach QProxyStyle (it would take the application style as its base).
        QStyle* windowsItemStyle()
        {
            static QPointer<QStyle> style;
            if (!style)
            {
                if (QStyle* windows = QStyleFactory::create(QStringLiteral("windows")))
                {
                    style = new WindowsItemStyle(windows);
                    style->setParent(QCoreApplication::instance());
                }
            }
            return style;
        }

        ElementRegistry<MenuElement>& registry()
        {
            static ElementRegistry<MenuElement> rows("_o3de_menuElement");
            return rows;
        }

        //! The element of a menu: a module's first, then the Global one, which a widget that is no menu has too
        const MenuElement& findElement(const QWidget* widget)
        {
            const MenuElement* element = qobject_cast<const QMenu*>(widget) ? registry().find(widget) : nullptr;
            return element ? *element : GlobalPopup;
        }
    } // namespace MenuElementInternal

    // ----------------------------------------------------------------------------------
    // Popup: geometry
    // ----------------------------------------------------------------------------------

    int MenuElement::separatorHeight() const
    {
        // QStyleSheetStyle::sizeFromContents, CT_MenuItem of a separator: the rule's box
        return m_separatorMarginTop + m_separatorBorder + m_separatorMarginBottom;
    }

    // ----------------------------------------------------------------------------------
    // Popup: drawing
    // ----------------------------------------------------------------------------------

    QPalette MenuElement::itemPalette(const QPalette& menuPalette, bool selected, bool enabled) const
    {
        // QStyleSheetStyle::drawControl (CE_MenuItem): the menu's rule, then the item's, each on the button and the highlight roles;
        // an item with no fill of its own shows the menu's. A disabled item has the :disabled colour, a selected one the fill.
        QPalette palette = menuPalette;
        const QColor none;
        MenuElementInternal::configurePalette(palette, m_surface, m_text, QPalette::ButtonText, QPalette::Button);
        MenuElementInternal::configurePalette(palette, m_surface, m_text, QPalette::HighlightedText, QPalette::Highlight);
        const QColor fill = selected ? m_selectedFill : none;
        const QColor text = enabled ? none : m_disabledText;
        MenuElementInternal::configurePalette(palette, fill, text, QPalette::ButtonText, QPalette::Button);
        MenuElementInternal::configurePalette(palette, fill, text, QPalette::HighlightedText, QPalette::Highlight);
        if (!selected)
        {
            palette.setColor(QPalette::Window, Qt::transparent);
            palette.setColor(QPalette::Button, Qt::transparent);
        }
        return palette;
    }

    bool MenuElement::drawItem(QPainter* painter, const QStyleOptionMenuItem& option, const QWidget* menu) const
    {
        if (option.menuItemType == QStyleOptionMenuItem::Separator)
        {
            drawSeparator(painter, option.rect);
            return true;
        }

        // Without a Windows style in this Qt the base style draws the items, and they will not match the legacy GUI
        QStyle* windows = MenuElementInternal::windowsItemStyle();
        AZ_WarningOnce(
            "MenuElement", windows != nullptr, "This Qt has no Windows style: the items of a popup menu are drawn by the base style.");
        if (!windows)
        {
            return false;
        }

        QStyleOptionMenuItem item(option);
        item.palette =
            itemPalette(option.palette, option.state.testFlag(QStyle::State_Selected), option.state.testFlag(QStyle::State_Enabled));

        // The sheet style put the item's font on the painter before it handed the item over (QStyleSheetStyle::drawControl,
        // CE_MenuItem), and QWindowsStyle draws the shortcut with the painter's font as it comes
        painter->save();
        painter->setFont(option.font);
        windows->drawControl(QStyle::CE_MenuItem, &item, painter, menu);
        painter->restore();
        return true;
    }

    void MenuElement::drawSeparator(QPainter* painter, const QRect& rect) const
    {
        // The rule's border in the box its margins leave (QStyleSheetStyle::drawControl, CE_MenuItem): one edge at the bottom
        const QRect box = rect.adjusted(0, m_separatorMarginTop, 0, -m_separatorMarginBottom);
        BoxElement::drawEdges(painter, box, { {}, {}, { m_separatorBorder, m_separatorLine }, {} }, BoxElement::Radii());
    }

    bool MenuElement::drawHostedWidget(QPainter* painter, const QStyleOption* option, const QWidget* widget)
    {
        // `QMenu::item .WidgetAction:hover` (Menu.qss:18): a pseudo-element that is not the selector's last takes no part in the
        // match (StyleSelector::basicSelectorMatches), so the rule reaches every widget tagged WidgetAction inside a menu, and
        // :hover needs it enabled (pseudoClass, qstylesheetstyle.cpp)
        if (!option->state.testFlag(QStyle::State_Enabled) || !option->state.testFlag(QStyle::State_MouseOver)
            || !Selectors::tagged(widget, "WidgetAction"))
        {
            return false;
        }
        for (const QWidget* ancestor = widget->parentWidget(); ancestor; ancestor = ancestor->parentWidget())
        {
            if (qobject_cast<const QMenu*>(ancestor))
            {
                painter->fillRect(option->rect, forWidget(ancestor).m_selectedFill);
                return true;
            }
        }
        return false;
    }

    // ----------------------------------------------------------------------------------
    // Popup: resolution
    // ----------------------------------------------------------------------------------

    void MenuElement::registerElement(
        const MenuElement* element, const QMetaObject* widgetClass, ElementMatcher matcher, ElementScope scope)
    {
        MenuElementInternal::registry().add(element, widgetClass, matcher, scope);
        resolvePolished(widgetClass, [](QWidget* widget)
            {
                resolve(widget);
                widget->update();
            });
    }

    void MenuElement::resolve(QWidget* widget)
    {
        if (!qobject_cast<const QMenu*>(widget))
        {
            return;
        }
        const MenuElement& element = MenuElementInternal::findElement(widget);
        MenuElementInternal::registry().keep(widget, &element);

        // The menu's colours (QMenu { background-color }, ::item:disabled { color }) reach the widgets a QWidgetAction hosts
        // through the menu's palette
        QPalette palette = widget->palette();
        palette.setColor(QPalette::Window, element.m_surface);
        palette.setColor(QPalette::Base, element.m_surface);
        palette.setColor(QPalette::Disabled, QPalette::Text, element.m_disabledText);
        palette.setColor(QPalette::Disabled, QPalette::WindowText, element.m_disabledText);
        widget->setPalette(palette);
    }

    const MenuElement& MenuElement::forWidget(const QWidget* widget)
    {
        using namespace MenuElementInternal;
        const std::optional<const MenuElement*> kept = widget ? registry().kept(widget) : std::nullopt;
        return kept ? **kept : findElement(widget);
    }

    // ----------------------------------------------------------------------------------
    // Bar
    // ----------------------------------------------------------------------------------

    int MenuBarElement::minHeight() const
    {
        // QStyleSheetStyle::setGeometry: the rule's min-height inside its border
        return m_height + m_borderWidth;
    }

    QSize MenuBarElement::itemSize(const QSize& contents) const
    {
        // The bar's rule gives the height (QRenderRule::adjustSize), and the item's padding is added to a width that exists
        // (QRenderRule::boxSize, as QStyleSheetStyle::sizeFromContents does for CT_MenuBarItem)
        return QSize(contents.width() < 0 ? -1 : contents.width() + 2 * m_itemPadding, m_height);
    }

    QColor MenuBarElement::itemFill(const QStyleOption* option) const
    {
        // :pressed is declared after :selected (MenuBar.qss:28-36), so it wins when both match; :selected is State_Selected
        // whether the item is enabled or not (pseudoClass, qstylesheetstyle.cpp)
        if (option->state.testFlag(QStyle::State_Sunken))
        {
            return m_pressedFill;
        }
        if (option->state.testFlag(QStyle::State_Selected))
        {
            return m_selectedFill;
        }
        return QColor();
    }

    QStyleOptionMenuItem MenuBarElement::labelOption(const QStyleOptionMenuItem& option) const
    {
        // QStyleSheetStyle::drawControl (CE_MenuBarItem): the item rule's content box, the bar rule's colour, and the transparent
        // window colour an item with no fill of its own is drawn on, so that the bar shows (a filled item's label does not read it)
        QStyleOptionMenuItem label(option);
        label.rect = option.rect.adjusted(m_itemPadding, 0, -m_itemPadding, 0);
        label.palette.setColor(QPalette::ButtonText, m_text);
        label.palette.setColor(QPalette::Window, Qt::transparent);
        return label;
    }

    void MenuBarElement::drawBorder(QPainter* painter, const QRect& barRect) const
    {
        // QMenuBar { border-bottom }: the rule's border over the whole bar (QStyleSheetStyle::drawPrimitive, PE_Widget)
        BoxElement::drawEdges(painter, barRect, { {}, {}, { m_borderWidth, m_borderColor }, {} }, BoxElement::Radii());
    }

    const MenuBarElement& MenuBarElement::global()
    {
        return MenuElementInternal::GlobalBar;
    }
} // namespace AzQtComponents
