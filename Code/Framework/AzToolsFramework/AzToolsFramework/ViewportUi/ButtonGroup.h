/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzToolsFramework/ViewportUi/ViewportUiRequestBus.h>

namespace AzToolsFramework::ViewportUi::Internal
{
    class Button;

    //! Data class for a button group on the Viewport UI. A button group is defined as a group of buttons with icons
    //! each of which can be clicked to trigger an event e.g. toggling between modes.
    //! @note This can be used with either a Cluster or a Switcher with slightly different visuals for each.
    class ButtonGroup
    {
    public:
        ButtonGroup();
        ~ButtonGroup() = default;

        void SetHighlightedButton(ButtonId buttonId);
        void SetDisabledButton(ButtonId buttonId, bool disabled);
        void ClearHighlightedButton();

        void SetViewportUiElementId(ViewportUiElementId id);
        ViewportUiElementId GetViewportUiElementId() const;

        ButtonId AddButton(const AZStd::string& icon, const AZStd::string& name = AZStd::string());
        bool RemoveButton(ButtonId buttonId);
        Button* GetButton(ButtonId buttonId);
        AZStd::vector<Button*> GetButtons();

        void ConnectEventHandler(AZ::Event<ButtonId>::Handler& handler);
        void PressButton(ButtonId buttonId);

    private:
        AZ::Event<ButtonId> m_buttonTriggeredEvent;
        ViewportUiElementId m_viewportUiId;
        AZStd::unordered_map<ButtonId, AZStd::unique_ptr<Button>> m_buttons;
        ButtonId m_highlightedButtonId = InvalidButtonId;
    };
} // namespace AzToolsFramework::ViewportUi::Internal
