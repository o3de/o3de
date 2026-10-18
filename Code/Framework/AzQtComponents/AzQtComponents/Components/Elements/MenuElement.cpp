/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Elements/MenuElement.h>
#include <AzQtComponents/Components/Elements/TextElement.h>

#include <AzCore/Debug/Trace.h>

#include <QApplication>
#include <QMenu>
#include <QPainter>
#include <QPointer>
#include <QProxyStyle>
#include <QStyleFactory>
#include <QStyleOption>
#include <QVariant>
#include <QVector>
#include <QWidget>

namespace AzQtComponents
{
    //! Everything private to this file. A named namespace, not an anonymous one: a unity build joins this file
    //! with others into one translation unit, where anonymous namespaces merge (TextElement.cpp does the same).
    namespace MenuElementInternal
    {
        // ------------------------------------------------------------------------------
        // Global elements. Each value cites the declaration that grants it, as file:line of
        // the legacy stylesheets in Components/Widgets/.
        // ------------------------------------------------------------------------------

        //! QMenu (Menu.qss:10-14), QMenu::item:selected (:17-21), QMenu::item:disabled (:23-26) and QMenu::separator
        //! (:28-34: `height: 0px`, so the box is its margins and its border), over the colour of the universal rule.
        //! TOKENIZE: MenuSurfaceColor, MenuPadding, MenuSelectedColor, MenuDisabledTextColor, MenuSeparatorLine
        const MenuElement GlobalPopup{
            .m_name = "Global.Menu.Popup",
            .m_surface = QColor(0x22, 0x22, 0x22),              // Menu.qss:12
            .m_paddingH = 2,                                    // Menu.qss:13 (left padding: qstylesheetstyle.cpp:5074-5078)
            .m_paddingV = 4,                                    // Menu.qss:13 (top padding: qstylesheetstyle.cpp:5080-5084)
            .m_text = QColor(0xFF, 0xFF, 0xFF),                 // BaseStyleSheet.qss:18 `color: white`, reaching every widget
            .m_selectedFill = QColor(0x44, 0x44, 0x44),         // Menu.qss:20 (:17 item :selected, :18 hosted widget :hover). UNVERIFIED: no capture shows one
            .m_disabledText = QColor(0x55, 0x55, 0x55),         // Menu.qss:25
            .m_separatorMarginTop = 2,                          // Menu.qss:32
            .m_separatorBorder = 1,                             // Menu.qss:31 `border-bottom: 1px solid #454545`
            .m_separatorMarginBottom = 3,                       // Menu.qss:33
            .m_separatorLine = QColor(0x45, 0x45, 0x45),        // Menu.qss:31
        };

        //! QMenuBar (MenuBar.qss:9-20), QMenuBar::item (:22-26), ::item:selected (:28-31) and ::item:pressed (:33-36), over
        //! the colour of the universal rule. The bar's font is the QMenuBar class font (TextElement.cpp, MenuBar.qss:13-14).
        //! TOKENIZE: MenuBarHeight, MenuBarBorder, MenuBarItemPadding, MenuBarSelectedColor, MenuBarPressedColor
        const MenuBarElement GlobalBar{
            .m_name = "Global.Menu.Bar",
            .m_height = 28,                                     // MenuBar.qss:11-12 (min-height and height)
            .m_borderWidth = 2,                                 // MenuBar.qss:17
            .m_borderColor = QColor(0x11, 0x11, 0x11),          // MenuBar.qss:18
            .m_itemPadding = 14,                                // MenuBar.qss:24-25
            .m_text = QColor(0xFF, 0xFF, 0xFF),                 // BaseStyleSheet.qss:18
            .m_selectedFill = QColor(0x55, 0x55, 0x55),         // MenuBar.qss:30. UNVERIFIED: no capture shows an item hovered
            .m_pressedFill = QColor(0x22, 0x22, 0x22),          // MenuBar.qss:35. UNVERIFIED: no capture shows an item pressed
        };

        // ------------------------------------------------------------------------------
        // Drawing the way the legacy sheet style did
        // ------------------------------------------------------------------------------

        //! A solid edge as QRenderRule::drawBorder draws it (qstylesheetstyle.cpp:1282-1285, qcssutil.cpp:165-169): a fill
        //! with antialiasing, so that at a fractional device ratio the rows of the edge blend into their neighbours.
        void fillEdge(QPainter* painter, const QRectF& edge, const QColor& color)
        {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setPen(Qt::NoPen);
            painter->setBrush(color);
            painter->drawRect(edge);
            painter->restore();
        }

        //! What QRenderRule::configurePalette does for a rule with a fill and a colour (qstylesheetstyle.cpp:1393-1422), on the
        //! roles the base styles read: the fill reaches its role, Window and the shades Qt derives from a solid colour, the colour
        //! reaches its role, WindowText and Text. An invalid colour is a declaration the rule does not have.
        void configurePalette(QPalette& palette, const QColor& fill, const QColor& text, QPalette::ColorRole textRole, QPalette::ColorRole fillRole)
        {
            if (fill.isValid())
            {
                palette.setBrush(fillRole, fill);
                palette.setBrush(QPalette::Window, fill);
                palette.setBrush(QPalette::Light, fill.lighter(115));
                palette.setBrush(QPalette::Midlight, fill.lighter(107));
                palette.setBrush(QPalette::Dark, fill.darker(150));
                palette.setBrush(QPalette::Shadow, fill.darker(300));
            }
            if (text.isValid())
            {
                palette.setBrush(textRole, text);
                palette.setBrush(QPalette::WindowText, text);
                palette.setBrush(QPalette::Text, text);
            }
        }

        // ------------------------------------------------------------------------------
        // The style that draws an item
        // ------------------------------------------------------------------------------

        //! QWindowsStyle calls back into its proxy for the icon size, the shortcut underline, the check mark and the arrow of an
        //! item. In the legacy GUI the proxy was the sheet style, which handed each to the base style (primitives qstylesheetstyle.cpp:4914-4920,
        //! metrics :5296, hints :5788), so here it is the style of the menu being drawn: the arrow is the base style's, and a shortcut is
        //! underlined as the base style says (QWindowsStyle itself waits for the Alt key, qwindowsstyle.cpp:483-517).
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

            void drawPrimitive(PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const override
            {
                styleOf(widget)->drawPrimitive(element, option, painter, widget);
            }

        private:
            static QStyle* styleOf(const QWidget* widget)
            {
                return widget ? widget->style() : QApplication::style();
            }
        };

        //! The style that draws the items of every menu, made when first needed. It belongs to the application (a child of it),
        //! so that a later application, as the unit tests make one per test, finds none here and makes its own. Null when this
        //! Qt has no Windows style: QStyleFactory::create gives null then, and it must not reach QProxyStyle, which would take
        //! the application style as its base.
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

        // ------------------------------------------------------------------------------
        // Registry
        // ------------------------------------------------------------------------------

        struct Registration
        {
            const MenuElement* m_element;
            MenuElement::Matcher m_matcher;
        };

        QVector<Registration>& registrations()
        {
            static QVector<Registration> list;
            return list;
        }

        constexpr const char* ElementProperty = "_o3de_menuElement";
    } // namespace MenuElementInternal

    // ----------------------------------------------------------------------------------
    // Popup
    // ----------------------------------------------------------------------------------

    int MenuElement::separatorHeight() const
    {
        // QStyleSheetStyle CT_MenuItem for a separator: the rule's box (qstylesheetstyle.cpp:5430-5432)
        return m_separatorMarginTop + m_separatorBorder + m_separatorMarginBottom;
    }

    QPalette MenuElement::itemPalette(const QPalette& menuPalette, bool selected, bool enabled) const
    {
        // The menu's rule configures the palette twice, as the item's rule does (qstylesheetstyle.cpp:3865-3868), and an item
        // without a fill of its own lets the menu's show (:3981-3984). A disabled item has the :disabled colour, a selected one the fill.
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

        // Tripwire: the delegate is all the element has to lean on. Without a Windows style in this Qt the base style draws the items,
        // and they will not match the legacy GUI.
        QStyle* windows = MenuElementInternal::windowsItemStyle();
        AZ_WarningOnce("MenuElement", windows != nullptr, "This Qt has no Windows style: the items of a popup menu are drawn by the base style.");
        if (!windows)
        {
            return false;
        }

        QStyleOptionMenuItem item(option);
        item.palette = itemPalette(option.palette, option.state.testFlag(QStyle::State_Selected), option.state.testFlag(QStyle::State_Enabled));

        // The sheet style put the item's font on the painter before it handed the item over (qstylesheetstyle.cpp:3869-3873), and
        // QWindowsStyle draws the shortcut with the painter's font as it comes (qwindowsstyle.cpp:1117-1126)
        painter->save();
        painter->setFont(option.font);
        windows->drawControl(QStyle::CE_MenuItem, &item, painter, menu);
        painter->restore();
        return true;
    }

    void MenuElement::drawSeparator(QPainter* painter, const QRect& rect) const
    {
        // The rule drew its border in the box the margins leave (qstylesheetstyle.cpp:3879-3881): one edge at the bottom of it
        QRectF edge = QRectF(rect).adjusted(0, m_separatorMarginTop, 0, -m_separatorMarginBottom);
        edge.setTop(edge.bottom() - m_separatorBorder);
        MenuElementInternal::fillEdge(painter, edge, m_separatorLine);
    }

    bool MenuElement::drawHostedWidget(QPainter* painter, const QStyleOption* option, const QWidget* widget)
    {
        // `QMenu::item .WidgetAction:hover` (Menu.qss:18): a pseudo-element that is not the selector's last takes no part in
        // the match (qcssparser.cpp:2148-2191), so the rule reaches every widget tagged WidgetAction inside a menu, and :hover
        // needs it enabled (qstylesheetstyle.cpp:1760-1765). UNVERIFIED: no capture shows a hosted widget hovered.
        if (!option->state.testFlag(QStyle::State_Enabled) || !option->state.testFlag(QStyle::State_MouseOver)
            || !TextElement::tagged(widget, "WidgetAction"))
        {
            return false;
        }
        for (const QWidget* ancestor = widget->parentWidget(); ancestor; ancestor = ancestor->parentWidget())
        {
            if (qobject_cast<const QMenu*>(ancestor))
            {
                painter->fillRect(option->rect, forMenu(ancestor).m_selectedFill);
                return true;
            }
        }
        return false;
    }

    void MenuElement::registerElement(const MenuElement* element, Matcher matcher)
    {
        MenuElementInternal::registrations().append({ element, matcher });
    }

    const MenuElement& MenuElement::resolve(const QWidget* menu)
    {
        if (!qobject_cast<const QMenu*>(menu))
        {
            return popup();
        }
        const MenuElement* element = &popup();
        for (const MenuElementInternal::Registration& registration : MenuElementInternal::registrations())
        {
            if (registration.m_matcher(menu))
            {
                element = registration.m_element;
                break;
            }
        }
        // Kept on the menu: draws and metrics read it back without resolving again
        const_cast<QWidget*>(menu)->setProperty(MenuElementInternal::ElementProperty, QVariant::fromValue(reinterpret_cast<quintptr>(element)));
        return *element;
    }

    const MenuElement& MenuElement::forMenu(const QWidget* menu)
    {
        if (menu)
        {
            const QVariant stored = menu->property(MenuElementInternal::ElementProperty);
            if (stored.isValid())
            {
                return *reinterpret_cast<const MenuElement*>(stored.value<quintptr>());
            }
        }
        return resolve(menu);
    }

    const MenuElement& MenuElement::popup()
    {
        return MenuElementInternal::GlobalPopup;
    }

    // ----------------------------------------------------------------------------------
    // Bar
    // ----------------------------------------------------------------------------------

    int MenuBarElement::minHeight() const
    {
        // QStyleSheetStyle::setGeometry: the rule's min-height inside its border (qstylesheetstyle.cpp:2570-2603)
        return m_height + m_borderWidth;
    }

    QSize MenuBarElement::itemSize(const QSize& contents) const
    {
        // The bar's rule gives the height (QRenderRule::adjustSize, qstylesheetstyle.cpp:560-571) and the item's padding is added
        // to a width that exists (QRenderRule::boxSize, :1116-1121, as CT_MenuBarItem does, :5464-5469)
        return QSize(contents.width() < 0 ? -1 : contents.width() + 2 * m_itemPadding, m_height);
    }

    QColor MenuBarElement::itemFill(const QStyleOption* option) const
    {
        // :pressed is declared after :selected (MenuBar.qss:28-36), so it wins when both match; :selected is State_Selected
        // whether the item is enabled or not (qstylesheetstyle.cpp:1757-1803)
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
        // The item rule's content box (qstylesheetstyle.cpp:4002), the colour of the bar's rule (:4003), and the transparent window
        // colour an item without a fill of its own is drawn on, so that the bar shows (:4010-4013; a filled item's label does not read it)
        QStyleOptionMenuItem label(option);
        label.rect = option.rect.adjusted(m_itemPadding, 0, -m_itemPadding, 0);
        label.palette.setColor(QPalette::ButtonText, m_text);
        label.palette.setColor(QPalette::Window, Qt::transparent);
        return label;
    }

    void MenuBarElement::drawBorder(QPainter* painter, const QRect& barRect) const
    {
        // QMenuBar { border-bottom }: the rule's border, over the whole bar (QStyleSheetStyle PE_Widget, qstylesheetstyle.cpp:4718-4724)
        QRectF edge(barRect);
        edge.setTop(edge.bottom() - m_borderWidth);
        MenuElementInternal::fillEdge(painter, edge, m_borderColor);
    }

    const MenuBarElement& MenuBarElement::global()
    {
        return MenuElementInternal::GlobalBar;
    }
} // namespace AzQtComponents
