/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ViewportUi/Button.h>
#include <AzToolsFramework/ViewportUi/TextField.h>
#include <AzToolsFramework/ViewportUi/ViewportUiDisplay.h>

namespace AzToolsFramework::ViewportUi
{
    namespace Internal
    {
        class ButtonGroup;
        class ViewportUiDisplay;
    } // namespace Internal

    class ViewportUiManager : public ViewportUiRequestBus::Handler
    {
    public:
        ViewportUiManager() = default;
        ~ViewportUiManager() = default;

        // ViewportUiRequestBus ...
        const ClusterId CreateCluster(Alignment align) override;
        const SwitcherId CreateSwitcher(Alignment align) override;
        void SetClusterActiveButton(ClusterId clusterId, ButtonId buttonId) override;
        void ClearClusterActiveButton(ClusterId clusterId) override;
        void SetSwitcherActiveButton(SwitcherId switcherId, ButtonId buttonId) override;
        void SetClusterButtonLocked(ClusterId clusterId, ButtonId buttonId, bool isLocked) override;
        void SetClusterButtonTooltip(ClusterId clusterId, ButtonId buttonId, const AZStd::string& tooltip) override;
        const ButtonId CreateClusterButton(ClusterId clusterId, const AZStd::string& icon) override;
        const ButtonId CreateSwitcherButton(
            SwitcherId switcherId, const AZStd::string& icon, const AZStd::string& name = AZStd::string()) override;
        void RegisterClusterEventHandler(ClusterId clusterId, AZ::Event<ButtonId>::Handler& handler) override;
        void RegisterSwitcherEventHandler(SwitcherId switcherId, AZ::Event<ButtonId>::Handler& handler) override;
        void RemoveCluster(ClusterId clusterId) override;
        void RemoveSwitcher(SwitcherId switcherId) override;
        void SetClusterVisible(ClusterId clusterId, bool visible) override;
        void SetSwitcherVisible(SwitcherId switcherId, bool visible);
        void SetClusterGroupVisible(const AZStd::vector<ClusterId>& clusterGroup, bool visible) override;
        const TextFieldId CreateTextField(
            const AZStd::string& labelText, const AZStd::string& textFieldDefaultText, TextFieldValidationType validationType) override;
        void SetTextFieldText(TextFieldId textFieldId, const AZStd::string& text) override;
        void RegisterTextFieldCallback(TextFieldId textFieldId, AZ::Event<AZStd::string>::Handler& handler) override;
        void RemoveTextField(TextFieldId textFieldId) override;
        void SetTextFieldVisible(TextFieldId textFieldId, bool visible) override;
        void CreateViewportBorder(
            const AZStd::string& borderTitle, AZStd::optional<ViewportUiBackButtonCallback> backButtonCallback) override;
        void RemoveViewportBorder() override;
        void PressButton(ClusterId clusterId, ButtonId buttonId) override;
        void PressButton(SwitcherId switcherId, ButtonId buttonId) override;

        //! Connects to the correct viewportId bus address.
        void ConnectViewportUiBus(const int viewportId);
        //! Disconnects from the viewport request bus.
        void DisconnectViewportUiBus();
        //! Initializes the Viewport UI by attaching it to the given parent and render overlay.
        void InitializeViewportUi(QWidget* parent, QWidget* renderOverlay);
        //! Updates all registered elements to display up to date.
        void Update();

    protected:
        AZStd::unordered_map<ClusterId, AZStd::shared_ptr<Internal::ButtonGroup>>
            m_clusterButtonGroups; //!< A map of all registered Clusters.
        AZStd::unordered_map<SwitcherId, AZStd::shared_ptr<Internal::ButtonGroup>>
            m_switcherButtonGroups; //!< A map of all registered Switchers.
        AZStd::unordered_map<TextFieldId, AZStd::shared_ptr<Internal::TextField>> m_textFields; //!< A map of all registered TextFields.

        AZStd::unique_ptr<Internal::ViewportUiDisplay> m_viewportUi; //!< The lower level graphical API for Viewport UI.

    private:
        //! Register a new Cluster and return its id.
        ClusterId RegisterNewCluster(AZStd::shared_ptr<Internal::ButtonGroup>& buttonGroup);
        //! Register a new Switcher and return its id.
        SwitcherId RegisterNewSwitcher(AZStd::shared_ptr<Internal::ButtonGroup>& buttonGroup);
        //! Register a new text field and return its id.
        TextFieldId RegisterNewTextField(AZStd::shared_ptr<Internal::TextField>& textField);
        //! Update the corresponding ui element for the given button group.
        void UpdateButtonGroupUi(Internal::ButtonGroup* buttonGroup);
        //! Update the corresponding ui element for the given text field.
        void UpdateTextFieldUi(Internal::TextField* textField);
    };
} // namespace AzToolsFramework::ViewportUi
