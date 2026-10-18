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
    //! Everything private to this file. A named namespace, not an anonymous one: a unity build joins this file
    //! with others into one translation unit, where anonymous namespaces merge.
    namespace ComboPopupElementInternal
    {
        // ------------------------------------------------------------------------------
        // The Global element. Each value cites the declaration that grants it, as file:line of
        // Components/Widgets/ComboBox.qss.
        // ------------------------------------------------------------------------------

        //! QComboBox QAbstractItemView, with its ::item rules. The disabled text is UNVERIFIED: no capture holds a
        //! disabled item. TOKENIZE: ComboPopupSurface, ComboPopupItemText, ComboPopupSelectedFill, ComboPopupDisabledText
        const ComboPopupElement Items{
            .m_name = "Global.ComboPopup.Items",
            .m_surface = QColor(0x22, 0x22, 0x22),              // :81 QComboBox QAbstractItemView { background-color }
            .m_text = QColor(0xFF, 0xFF, 0xFF),                 // :89 ::item { color }
            .m_selectedFill = QColor(0x44, 0x44, 0x44),         // :104 ::item:selected { background-color }
            .m_selectedText = QColor(0xFF, 0xFF, 0xFF),         // :105 ::item:selected { color }
            .m_disabledText = QColor(0x55, 0x55, 0x55),         // :110 ::item:disabled { color }
            .m_itemPaddingLeft = 24,                            // :92 ::item { padding-left }
            .m_itemPaddingRight = 24,                           // :90 ::item { padding-right }
        };
    } // namespace ComboPopupElementInternal

    // ----------------------------------------------------------------------------------
    // Element
    // ----------------------------------------------------------------------------------

    const ComboPopupElement& ComboPopupElement::global()
    {
        return ComboPopupElementInternal::Items;
    }

    // ----------------------------------------------------------------------------------
    // Selectors
    // ----------------------------------------------------------------------------------

    bool ComboPopupElement::coversView(const QWidget* widget)
    {
        // The combo box parents the view until its container takes it over, so a view is found under either
        return qobject_cast<const QAbstractItemView*>(widget) && StyleHelpers::findParent<QComboBox>(widget);
    }

    bool ComboPopupElement::covers(const QWidget* widget)
    {
        return widget && (widget->inherits("QComboBoxPrivateContainer") || coversView(widget));
    }

    // ----------------------------------------------------------------------------------
    // Use
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

        // The fill is painted above, and ComboBox.qss:83 `outline: none` turned the focus outline off; the sheet
        // had no :hover rule for an item (the current item follows the mouse).
        option.state &= ~(QStyle::State_Selected | QStyle::State_HasFocus | QStyle::State_MouseOver);
    }
} // namespace AzQtComponents
