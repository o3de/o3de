#pragma once

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

#include <AzToolsFramework/ViewportUi/ViewportUiRequestBus.h>

namespace AzToolsFramework::ViewportUi::Internal
{
    class Button;

    //! Data class for a cluster on the Viewport UI. A cluster is defined as a group of buttons with icons
    //! each of which can be clicked to trigger an event e.g. toggling between modes.
    class ButtonGroup
    {
    public:
        ButtonGroup();
        ~ButtonGroup() = default;

        void SetHighlightedButton(ButtonId buttonId);

        void SetViewportUiElementId(ViewportUiElementId id);
        ViewportUiElementId GetViewportUiElementId() const;

        ButtonId AddButton(const AZStd::string& icon, const AZStd::string& name = AZStd::string());
        Button* GetButton(ButtonId buttonId);
        AZStd::vector<Button*> GetButtons();

        void ConnectEventHandler(AZ::Event<ButtonId>::Handler& handler);
        void PressButton(ButtonId buttonId);

    private:
        AZ::Event<ButtonId> m_buttonTriggeredEvent;
        ViewportUiElementId m_viewportUiId;
        AZStd::unordered_map<ButtonId, AZStd::unique_ptr<Button>> m_buttons;
    };
} // namespace AzToolsFramework::ViewportUi::Internal
