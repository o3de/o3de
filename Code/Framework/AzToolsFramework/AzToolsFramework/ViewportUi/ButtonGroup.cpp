/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ViewportUi/Button.h>
#include <AzToolsFramework/ViewportUi/ButtonGroup.h>

namespace AzToolsFramework::ViewportUi::Internal
{
    ButtonGroup::ButtonGroup()
        : m_buttons()
        , m_buttonTriggeredEvent()
    {
    }

    void ButtonGroup::SetViewportUiElementId(const ViewportUiElementId id)
    {
        m_viewportUiId = id;
    }

    ViewportUiElementId ButtonGroup::GetViewportUiElementId() const
    {
        return m_viewportUiId;
    }

    void ButtonGroup::SetHighlightedButton(ButtonId buttonId)
    {
        if (buttonId == m_highlightedButtonId) // the requested button is highlighted, so do nothing.
        {
            return;
        }

        if (auto buttonEntry = m_buttons.find(buttonId); buttonEntry != m_buttons.end())
        {
            ClearHighlightedButton();
            buttonEntry->second->m_state = Button::State::Selected;
            m_highlightedButtonId = buttonId;
        }
    }

    void ButtonGroup::ClearHighlightedButton()
    {
        if (m_highlightedButtonId == InvalidButtonId)
        {
            return;
        }
        if (auto buttonEntry = m_buttons.find(m_highlightedButtonId); buttonEntry != m_buttons.end())
        {
            buttonEntry->second->m_state = Button::State::Deselected;
            m_highlightedButtonId = InvalidButtonId;
        }
    }

    ButtonId ButtonGroup::AddButton(const AZStd::string& icon, const AZStd::string& name)
    {
        auto buttonId = ButtonId(m_buttons.size() + 1);

        if (name.empty())
        {
            m_buttons.insert({ buttonId, AZStd::make_unique<Button>(icon, buttonId) });
        }
        else
        {
            m_buttons.insert({ buttonId, AZStd::make_unique<Button>(icon, name, buttonId) });
        }
        return buttonId;
    }

    Button* ButtonGroup::GetButton(ButtonId buttonId)
    {
        if (auto buttonEntry = m_buttons.find(buttonId); buttonEntry != m_buttons.end())
        {
            return buttonEntry->second.get();
        }
        return nullptr;
    }

    AZStd::vector<Button*> ButtonGroup::GetButtons()
    {
        auto buttons = AZStd::vector<Button*>();
        for (const auto& button : m_buttons)
        {
            buttons.push_back(button.second.get());
        }
        return buttons;
    }

    void ButtonGroup::ConnectEventHandler(AZ::Event<ButtonId>::Handler& handler)
    {
        handler.Connect(m_buttonTriggeredEvent);
    }

    void ButtonGroup::PressButton(ButtonId buttonId)
    {
        m_buttonTriggeredEvent.Signal(buttonId);
    }
} // namespace AzToolsFramework::ViewportUi::Internal
