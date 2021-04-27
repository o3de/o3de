/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzToolsFramework/ViewportUi/Button.h>
#include <AzToolsFramework/ViewportUi/ButtonGroup.h>

#pragma optimize("", off)
#pragma inline_depth(0)

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
        if (auto buttonEntry = m_buttons.find(buttonId); buttonEntry != m_buttons.end())
        {
            for (auto& button : m_buttons)
            {
                button.second->m_state = Button::State::Deselected;
            }
            buttonEntry->second->m_state = Button::State::Selected;
        }
    }

    ButtonId ButtonGroup::AddButton(const AZStd::string& icon, const AZStd::string& name)
    {
        auto buttonId = ButtonId(m_buttons.size() + 1);

        if (name.empty())
        {
            m_buttons.insert({buttonId, AZStd::make_unique<Button>(icon, buttonId)});
        }
        else
        {
            m_buttons.insert({buttonId, AZStd::make_unique<Button>(icon, name, buttonId)});
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

    void ButtonGroup::ConnectEventHandler(AZ::Event<ButtonId>::Handler& handler) {
        handler.Connect(m_buttonTriggeredEvent);
    }

    void ButtonGroup::PressButton(ButtonId buttonId)
    {
        m_buttonTriggeredEvent.Signal(buttonId);
    }
} // namespace AzToolsFramework::ViewportUi::Internal
