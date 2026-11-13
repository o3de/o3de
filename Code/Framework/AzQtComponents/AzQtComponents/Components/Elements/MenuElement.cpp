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
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>

#include <AzCore/Debug/Trace.h>

#include <QApplication>
#include <QBrush>
#include <QIcon>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
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
        // A menu below a TabWidget has the same fill: TabWidget.qss:31-34 `QMenu { background-color: #222222 }`
        const QColor MenuFill = Foundations::BG;                    // Menu.qss:12 $fill.menu.idle
        constexpr int MenuPaddingV = 4;                             // Menu.qss:13 $space.menu.idle (top and bottom)
        constexpr int MenuPaddingH = 2;                             // Menu.qss:13 $space.menu.idle (left and right)
        const QColor SelectedItemFill = Foundations::Mid;           // Menu.qss:20 $fill.item.selected.Mid
        const QColor DisabledItemText = Foundations::Highest;       // Menu.qss:25 $text.item.disabled.Highest
        constexpr int SeparatorLineWidth = 1;                       // Menu.qss:31 $line.separator.idle.width
        const QColor SeparatorLine(0x45, 0x45, 0x45);               // Menu.qss:31 $line.separator.idle.color
        constexpr int SeparatorMarginTop = 2;                       // Menu.qss:32 $space.separator.idle.margin-top
        constexpr int SeparatorMarginBottom = 3;                    // Menu.qss:33 $space.separator.idle.margin-bottom

        // ---- The popup that fills itself
        // Four menus set a sheet with no selector on themselves, which reached the menu and every widget below it. The forms before the
        // flattening, each sheet on one line as it was set, with its line:
        //   EntityPropertyEditor.cpp:753 (OnSearchContextMenu), FilteredSearchWidget.cpp:1412 (OnSearchContextMenu) and
        //   AssetSelectionGrid.cpp:266 (ShowSearchMenu):
        //     menu->setStyleSheet("background-color: #333333")
        //   GraphTypeSelector.cpp:31 (CreateGUI):
        //     menu->setStyleSheet("border: none; background-color: #333333;")
        // SearchAssetTypeSelectorWidget.cpp:87 sets the second sheet too, but nothing constructs that class (abandoned): no part here.
        // The fill of the sites above, the first EntityPropertyEditor.cpp:753 (EntityPropertyEditor::OnSearchContextMenu)
        // $fill.menu.FilledPopup.Low
        const QColor FilledMenuFill = Foundations::Low;
        constexpr const char* FilledPopupTag = "FilledPopup";       // the class tag markFilledPopup gives a menu, `.FilledPopup`

        // ---- The popup of the status bar's source control item
        // MainStatusBar.cpp (SourceControlItem::InitMenu) tags its menu SourceControlMenu, the `QMenu.SourceControlMenu` of Menu.qss:36
        constexpr const char* SourceControlTag = "SourceControlMenu";
        // Menu.qss:38-39 `margin-left: 8px; margin-right: 8px`. TOKENIZE: SourceControlMenuMargin
        constexpr int SourceControlMarginH = 8;
        // Menu.qss:44 `QMenu.SourceControlMenu::item { margin-left: 41px }`. TOKENIZE: SourceControlMenuItemIndent
        constexpr int SourceControlItemMarginLeft = 41;
        // QCommonStyle's room for a shortcut after the text, which the sheet's own item size added (QStyleSheetStyle::sizeFromContents)
        constexpr int BoxedItemTabWidth = 12;
        // The gap the sheet's own item size left after the check column (QStyleSheetStyle::sizeFromContents, CT_MenuItem)
        constexpr int BoxedItemCheckGap = 4;

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

        //! The popup of the status bar's source control item: the popup in a margin of 8px on the left and right (Menu.qss:36-40), its
        //! text items 41px from the left (Menu.qss:42-45). The sheet style laid out the items of a menu whose item rule has a box itself
        //! (QStyleSheetStyle::sizeFromContents and drawControl, CT_MenuItem and CE_MenuItem), not the base style.
        const MenuElement SourceControlPopup{
            .m_name = "Global.Menu.SourceControlPopup",
            .m_surface = MenuFill,
            .m_margin = QMargins(SourceControlMarginH, 0, SourceControlMarginH, 0),
            .m_paddingH = MenuPaddingH,
            .m_paddingV = MenuPaddingV,
            .m_text = Foundations::Surface::Text,
            .m_selectedFill = SelectedItemFill,
            .m_disabledText = DisabledItemText,
            .m_itemMargin = QMargins(SourceControlItemMarginLeft, 0, 0, 0),
            .m_separatorMarginTop = SeparatorMarginTop,
            .m_separatorBorder = SeparatorLineWidth,
            .m_separatorMarginBottom = SeparatorMarginBottom,
            .m_separatorLine = SeparatorLine,
        };

        //! The popup on the fill of the four inline sheets: their `* { background-color }`, a menu's own sheet, outranks
        //! `QMenu { background-color }` (Menu.qss:12), whatever the specificity. Everything else is the popup's own: a rule with no
        //! pseudo-element does not reach the menu's ::item or ::separator (QStyleSheetStyle declarations: rules with pseudo elements
        //! do not cascade), and the menu rule has no border for `border: none` to replace.
        const MenuElement FilledPopup{
            .m_name = "Global.Menu.FilledPopup",
            .m_surface = FilledMenuFill,
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

        //! The icon of an item, drawn apart from its text (QStyleSheetStyle::renderMenuItemIcon): the icon at the size of a small icon, in
        //! the mode and state of the item, centered in the rect the element places it in
        void drawItemIcon(const MenuElement& element, QPainter* painter, const QStyleOptionMenuItem& option, const QWidget* menu)
        {
            const QIcon::Mode mode = option.state.testFlag(QStyle::State_Enabled)
                ? (option.state.testFlag(QStyle::State_Selected) ? QIcon::Active : QIcon::Normal)
                : QIcon::Disabled;
            const bool checked = option.checkType != QStyleOptionMenuItem::NotCheckable && option.checked;
            const int extent = menu->style()->pixelMetric(QStyle::PM_SmallIconSize, &option, menu);
            const QPixmap pixmap = option.icon.pixmap(
                QSize(extent, extent), painter->device()->devicePixelRatio(), mode, checked ? QIcon::On : QIcon::Off);
            const int width = pixmap.width() / pixmap.devicePixelRatio();
            const int height = pixmap.height() / pixmap.devicePixelRatio();

            const QRect rect = element.iconRect(option.rect, QSize(width, height), option.direction);
            QRect pixmapRect(0, 0, width, height);
            pixmapRect.moveCenter(rect.center());
            painter->drawPixmap(pixmapRect.topLeft(), pixmap);
        }

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

        //! A menu that markFilledPopup marked, or a menu below one: the sheet had no selector, so it reached the submenus a menu keeps
        //! (the sheet style reads the sheets of a widget's parents, and a submenu is a child of its menu) as it reached the menu
        bool isFilledPopup(const QWidget* menu)
        {
            for (const QWidget* widget = menu; widget; widget = widget->parentWidget())
            {
                if (Selectors::tagged(widget, FilledPopupTag))
                {
                    return true;
                }
            }
            return false;
        }

        //! `QMenu.SourceControlMenu`
        bool isSourceControlPopup(const QWidget* menu)
        {
            return Selectors::tagged(menu, SourceControlTag);
        }

        //! The registry begins with the Global rows; a module's rows are checked before them
        ElementRegistry<MenuElement> makeRegistry()
        {
            ElementRegistry<MenuElement> rows("_o3de_menuElement");
            rows.add(&FilledPopup, &QMenu::staticMetaObject, &isFilledPopup, ElementScope::Global);
            rows.add(&SourceControlPopup, &QMenu::staticMetaObject, &isSourceControlPopup, ElementScope::Global);
            return rows;
        }

        ElementRegistry<MenuElement>& registry()
        {
            static ElementRegistry<MenuElement> rows = makeRegistry();
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

    int MenuElement::panelWidth() const
    {
        // QStyleSheetStyle::pixelMetric, PM_MenuPanelWidth: the rule's border plus its left margin, for all four sides
        return m_margin.left();
    }

    QRect MenuElement::panelRect(const QRect& menuRect) const
    {
        // QStyleSheetStyle::drawPrimitive, PE_PanelMenu: QRenderRule::drawRule fills the rule's border rect, the rect inside its margin
        return menuRect.marginsRemoved(m_margin);
    }

    bool MenuElement::boxesItem(const QStyleOptionMenuItem& option) const
    {
        // QStyleSheetStyle::sizeFromContents and drawControl take their own item code for an item rule with a box. The element carries
        // that code for the plain text item of the menu; an icon, a check mark and a submenu arrow are the base style's
        return !m_itemMargin.isNull() && option.menuItemType == QStyleOptionMenuItem::Normal && option.icon.isNull()
            && option.checkType == QStyleOptionMenuItem::NotCheckable;
    }

    QSize MenuElement::boxedItemSize(const QStyleOptionMenuItem& option, const QSize& contents, int checkWidth) const
    {
        // QStyleSheetStyle::sizeFromContents, CT_MenuItem of an item whose rule has a box: the text's size, room for a shortcut, the
        // check column (the menu has one while any of its actions is checkable) or the icon column, and the rule's margin around it
        QSize size(contents);
        if (option.text.contains(QLatin1Char('\t')))
        {
            size.rwidth() += MenuElementInternal::BoxedItemTabWidth;
        }
        if (option.menuHasCheckableItems)
        {
            size.rwidth() += qMax(option.maxIconWidth, checkWidth) + MenuElementInternal::BoxedItemCheckGap;
        }
        else
        {
            size.rwidth() += option.maxIconWidth;
        }
        return QSize(size.width() + m_itemMargin.left() + m_itemMargin.right(), size.height() + m_itemMargin.top() + m_itemMargin.bottom());
    }

    // ----------------------------------------------------------------------------------
    // Popup: drawing
    // ----------------------------------------------------------------------------------

    QPalette MenuElement::itemPalette(const QPalette& menuPalette, bool selected, bool enabled, bool clearIdle) const
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
        if (!selected && clearIdle)
        {
            palette.setColor(QPalette::Window, Qt::transparent);
            palette.setColor(QPalette::Button, Qt::transparent);
        }
        return palette;
    }

    QRect MenuElement::iconRect(const QRect& itemRect, const QSize& iconSize, Qt::LayoutDirection direction) const
    {
        // QStyleSheetStyle::positionRect: the icon aligned left and centered vertically in the item's rect, then moved by the offsets
        // of the rule. A left or top of 0 takes the opposite side's offset in its place, its sign turned.
        const IconOffsets offsets = m_iconOffsets.value_or(IconOffsets());
        const int left = offsets.m_left ? offsets.m_left : -offsets.m_right;
        const int top = offsets.m_top ? offsets.m_top : -offsets.m_bottom;
        QRect rect = QStyle::alignedRect(direction, Qt::AlignLeft | Qt::AlignVCenter, iconSize, itemRect);
        rect.translate(direction == Qt::LeftToRight ? left : -left, top);
        return rect;
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

        // A menu with a rule for the icon has QWindowsStyle draw the text of an item with an icon, with no icon and no check mark, and
        // draws the icon itself (QStyleSheetStyle::drawControl, CE_MenuItem)
        const bool iconApart = m_iconOffsets.has_value() && !option.icon.isNull();
        QStyleOptionMenuItem item(option);
        item.palette = itemPalette(
            option.palette, option.state.testFlag(QStyle::State_Selected), option.state.testFlag(QStyle::State_Enabled), !iconApart);
        if (iconApart)
        {
            item.icon = QIcon();
            item.checkType = QStyleOptionMenuItem::NotCheckable;
        }

        // The sheet style put the item's font on the painter before it handed the item over (QStyleSheetStyle::drawControl,
        // CE_MenuItem), and QWindowsStyle draws the shortcut with the painter's font as it comes
        painter->save();
        painter->setFont(option.font);
        windows->drawControl(QStyle::CE_MenuItem, &item, painter, menu);
        painter->restore();

        if (iconApart)
        {
            MenuElementInternal::drawItemIcon(*this, painter, option, menu);
        }
        return true;
    }

    void MenuElement::drawBoxedItem(QPainter* painter, const QStyleOptionMenuItem& option, const QWidget* menu, int checkWidth) const
    {
        // QStyleSheetStyle::drawControl, CE_MenuItem of an item whose rule has a box: the rule's fill (the selected one) inside the
        // item's margin, then the text drawn by the sheet style itself, from the left of the contents rect after the check column
        painter->save();
        painter->setFont(option.font);
        if (option.state.testFlag(QStyle::State_Selected))
        {
            painter->fillRect(option.rect.marginsRemoved(m_itemMargin), m_selectedFill);
        }

        const QRect contents = option.rect.marginsRemoved(m_itemMargin);
        QRect textRect = contents;
        textRect.setLeft(textRect.left() + (option.menuHasCheckableItems ? qMax(option.maxIconWidth, checkWidth) : option.maxIconWidth));
        textRect.setWidth(textRect.width() - option.reservedShortcutWidth);
        const QRect visualText = QStyle::visualRect(option.direction, option.rect, textRect);

        int textFlags = Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic | Qt::TextDontClip | Qt::TextSingleLine;
        if (!menu->style()->styleHint(QStyle::SH_UnderlineShortcut, &option, menu))
        {
            textFlags |= Qt::TextHideMnemonic;
        }
        QString text = option.text;
        painter->setPen(option.state.testFlag(QStyle::State_Enabled) ? m_text : m_disabledText);
        const qsizetype tab = text.indexOf(QLatin1Char('\t'));
        if (tab >= 0)
        {
            // The shortcut starts where the text's rect ends and runs to the contents' right edge, mirrored in the contents (the text's
            // rect is mirrored in the whole item, QStyleSheetStyle::drawControl)
            const QRect shortcut = QStyle::visualRect(
                option.direction, contents, QRect(textRect.topRight(), QPoint(contents.right(), textRect.bottom())));
            painter->drawText(shortcut, textFlags, text.mid(tab + 1));
            text = text.left(tab);
        }
        painter->drawText(visualText, textFlags, text);
        painter->restore();
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
    // Popup: the variant that fills itself
    // ----------------------------------------------------------------------------------

    const MenuElement& MenuElement::filledPopup()
    {
        return MenuElementInternal::FilledPopup;
    }

    void MenuElement::setFilledPopupSheet(QMenu* menu, const QString& legacySheet)
    {
        if (!menu)
        {
            return;
        }
        StyleManager::setLegacyStyleSheet(menu, legacySheet);
        markFilledPopup(menu);
    }

    void MenuElement::markFilledPopup(QMenu* menu)
    {
        // Only the flattened GUI reads the tag: the legacy GUI keeps the menu as it was. The menu is not shown yet at any call site, so
        // the menus below it are polished after the tag is there and find their element then.
        if (menu && StyleManager::stylesheetsDisabled())
        {
            Style::addClass(menu, QString::fromLatin1(MenuElementInternal::FilledPopupTag));
        }
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
