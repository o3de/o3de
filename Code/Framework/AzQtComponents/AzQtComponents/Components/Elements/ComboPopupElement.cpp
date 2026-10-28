/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Elements/ComboPopupElement.h>
#include <AzQtComponents/Components/StyleHelpers.h>

#include <QAbstractItemView>
#include <QComboBox>
#include <QPainter>
#include <QStyle>
#include <QStyleOption>

namespace AzQtComponents
{
    //! Private to this file: named, not anonymous, as a unity build merges the anonymous namespaces of its files
    namespace ComboPopupElementInternal
    {
        // ------------------------------------------------------------------------------
        // Values
        // ------------------------------------------------------------------------------

        const QColor PopupSurface(0x22, 0x22, 0x22);            // ComboBox.qss:81 $fill.field.idle.BG
        const QColor ItemText(0xFF, 0xFF, 0xFF);                // ComboBox.qss:89 $text.item.idle
        const QColor SelectedItemFill(0x44, 0x44, 0x44);        // ComboBox.qss:104 $fill.item.selected.Mid
        const QColor SelectedItemText(0xFF, 0xFF, 0xFF);        // ComboBox.qss:105 $text.item.selected
        const QColor DisabledItemText(0x55, 0x55, 0x55);        // ComboBox.qss:110 $text.item.disabled.Highest
        constexpr int ItemPaddingLeft = 24;                     // ComboBox.qss:92 $space.item.idle.padding-left.Alt3
        constexpr int ItemPaddingRight = 24;                    // ComboBox.qss:90 $space.item.idle.padding-right.Alt2

        // ------------------------------------------------------------------------------
        // Elements
        // ------------------------------------------------------------------------------

        //! `QComboBox QAbstractItemView`, with its `::item` rules
        const ComboPopupElement Items{
            .m_name = "Global.ComboPopup.Items",
            .m_surface = PopupSurface,
            .m_text = ItemText,
            .m_selectedFill = SelectedItemFill,
            .m_selectedText = SelectedItemText,
            .m_disabledText = DisabledItemText,
            .m_itemPaddingLeft = ItemPaddingLeft,
            .m_itemPaddingRight = ItemPaddingRight,
        };
    } // namespace ComboPopupElementInternal

    // ----------------------------------------------------------------------------------
    // Drawing
    // ----------------------------------------------------------------------------------

    QPalette ComboPopupElement::palette(const QPalette& base) const
    {
        QPalette popup = base;
        for (const auto group : { QPalette::Active, QPalette::Inactive, QPalette::Disabled })
        {
            const bool disabled = group == QPalette::Disabled;
            const QColor text = disabled ? m_disabledText : m_text;
            popup.setColor(group, QPalette::Window, m_surface);
            popup.setColor(group, QPalette::Base, m_surface);
            popup.setColor(group, QPalette::Highlight, m_selectedFill);
            popup.setColor(group, QPalette::Text, text);
            popup.setColor(group, QPalette::WindowText, text);
            popup.setColor(group, QPalette::ButtonText, text);
            popup.setColor(group, QPalette::HighlightedText, disabled ? m_disabledText : m_selectedText);
        }
        return popup;
    }

    void ComboPopupElement::prepareItem(QStyleOptionViewItem& option, QPainter* painter) const
    {
        const bool enabled = option.state.testFlag(QStyle::State_Enabled);
        const bool selected = enabled && option.state.testFlag(QStyle::State_Selected);
        if (selected)
        {
            painter->fillRect(option.rect, m_selectedFill);
        }

        const QColor text = !enabled ? m_disabledText : (selected ? m_selectedText : m_text);
        for (const auto group : { QPalette::Active, QPalette::Inactive, QPalette::Disabled })
        {
            option.palette.setColor(group, QPalette::Text, text);
        }

        // The fill is painted above, and ComboBox.qss:83 `outline: none` turned the focus outline off; the sheet had no :hover rule
        // for an item (the current item follows the pointer)
        option.state &= ~(QStyle::State_Selected | QStyle::State_HasFocus | QStyle::State_MouseOver);
    }

    // ----------------------------------------------------------------------------------
    // Resolution
    // ----------------------------------------------------------------------------------

    const ComboPopupElement& ComboPopupElement::global()
    {
        return ComboPopupElementInternal::Items;
    }

    bool ComboPopupElement::coversView(const QWidget* widget)
    {
        // The combo box parents the view until its container takes it over, so a view is found under either
        return qobject_cast<const QAbstractItemView*>(widget) && StyleHelpers::findParent<QComboBox>(widget);
    }

    bool ComboPopupElement::covers(const QWidget* widget)
    {
        return widget && (widget->inherits("QComboBoxPrivateContainer") || coversView(widget));
    }
} // namespace AzQtComponents
