/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzToolsFramework/ViewportUi/Button.h>
#include <AzToolsFramework/ViewportUi/ButtonGroup.h>
#include <AzToolsFramework/ViewportUi/ViewportUiDisplay.h>
#include <AzToolsFramework/ViewportUi/ViewportUiManager.h>

namespace AzToolsFramework::ViewportUi
{
    void ViewportUiManager::ConnectViewportUiBus(const int viewportId)
    {
        ViewportUiRequestBus::Handler::BusConnect(viewportId);
    }

    void ViewportUiManager::DisconnectViewportUiBus()
    {
        ViewportUiRequestBus::Handler::BusDisconnect();
    }

    const ClusterId ViewportUiManager::CreateCluster(const Alignment align)
    {
        auto buttonGroup = AZStd::make_shared<Internal::ButtonGroup>();
        m_viewportUi->AddCluster(buttonGroup, align);

        return RegisterNewCluster(buttonGroup);
    }

    const SwitcherId ViewportUiManager::CreateSwitcher(const Alignment align)
    {
        auto buttonGroup = AZStd::make_shared<Internal::ButtonGroup>();
        m_viewportUi->AddSwitcher(buttonGroup, align);

        return RegisterNewSwitcher(buttonGroup);
    }

    void ViewportUiManager::SetClusterActiveButton(const ClusterId clusterId, const ButtonId buttonId)
    {
        if (auto clusterIt = m_clusterButtonGroups.find(clusterId); clusterIt != m_clusterButtonGroups.end())
        {
            auto cluster = clusterIt->second;
            cluster->SetHighlightedButton(buttonId);
            UpdateButtonGroupUi(cluster.get());
        }
    }

    void ViewportUiManager::SetClusterDisableButton(ClusterId clusterId, ButtonId buttonId, bool disabled)
    {
        if (auto clusterIt = m_clusterButtonGroups.find(clusterId); clusterIt != m_clusterButtonGroups.end())
        {
            auto& cluster = clusterIt->second;
            cluster->SetDisabledButton(buttonId, disabled);
            UpdateButtonGroupUi(cluster.get());
        }
    }

    void ViewportUiManager::ClearClusterActiveButton(ClusterId clusterId)
    {
        if (auto clusterIt = m_clusterButtonGroups.find(clusterId); clusterIt != m_clusterButtonGroups.end())
        {
            auto cluster = clusterIt->second;
            cluster->ClearHighlightedButton();
            UpdateButtonGroupUi(cluster.get());
        }
    }

    void ViewportUiManager::SetSwitcherActiveButton(const SwitcherId switcherId, const ButtonId buttonId)
    {
        if (auto switcherIt = m_switcherButtonGroups.find(switcherId); switcherIt != m_switcherButtonGroups.end())
        {
            auto switcher = switcherIt->second;
            switcher->SetHighlightedButton(buttonId);
            m_viewportUi->SetSwitcherActiveButton(switcher->GetViewportUiElementId(), buttonId);
            UpdateSwitcherButtonGroupUi(switcher.get());
        }
    }

    void ViewportUiManager::SetSwitcherDisableButton(SwitcherId switcherId, ButtonId buttonId, bool disabled)
    {
        if (auto switcherIt = m_switcherButtonGroups.find(switcherId); switcherIt != m_switcherButtonGroups.end())
        {
            auto switcher = switcherIt->second;
            switcher->SetDisabledButton(buttonId, disabled);
            UpdateSwitcherButtonGroupUi(switcher.get());
        }
    }

    void ViewportUiManager::SetClusterButtonLocked(const ClusterId clusterId, const ButtonId buttonId, const bool isLocked)
    {
        if (auto clusterIt = m_clusterButtonGroups.find(clusterId); clusterIt != m_clusterButtonGroups.end())
        {
            auto cluster = clusterIt->second;
            m_viewportUi->SetClusterButtonLocked(cluster->GetViewportUiElementId(), buttonId, isLocked);
            UpdateButtonGroupUi(cluster.get());
        }
    }

    void ViewportUiManager::SetClusterButtonTooltip(const ClusterId clusterId, const ButtonId buttonId, const AZStd::string& tooltip)
    {
        if (auto clusterIt = m_clusterButtonGroups.find(clusterId); clusterIt != m_clusterButtonGroups.end())
        {
            auto cluster = clusterIt->second;
            m_viewportUi->SetClusterButtonTooltip(cluster->GetViewportUiElementId(), buttonId, tooltip);
            UpdateButtonGroupUi(cluster.get());
        }
    }

    void ViewportUiManager::SetSwitcherButtonTooltip(const SwitcherId switcherId, const ButtonId buttonId, const AZStd::string& tooltip)
    {
        if (auto switcherIt = m_switcherButtonGroups.find(switcherId); switcherIt != m_switcherButtonGroups.end())
        {
            auto switcher = switcherIt->second;
            m_viewportUi->SetSwitcherButtonTooltip(switcher->GetViewportUiElementId(), buttonId, tooltip);
            UpdateSwitcherButtonGroupUi(switcher.get());
        }
    }

    void ViewportUiManager::RegisterClusterEventHandler(const ClusterId clusterId, AZ::Event<ButtonId>::Handler& handler)
    {
        if (auto clusterIt = m_clusterButtonGroups.find(clusterId); clusterIt != m_clusterButtonGroups.end())
        {
            auto cluster = clusterIt->second;
            cluster->ConnectEventHandler(handler);
        }
    }

    void ViewportUiManager::RegisterSwitcherEventHandler(const SwitcherId switcherId, AZ::Event<ButtonId>::Handler& handler)
    {
        if (auto switcherIt = m_switcherButtonGroups.find(switcherId); switcherIt != m_switcherButtonGroups.end())
        {
            auto switcher = switcherIt->second;
            switcher->ConnectEventHandler(handler);
        }
    }

    const ButtonId ViewportUiManager::CreateClusterButton(const ClusterId clusterId, const AZStd::string& icon)
    {
        if (auto clusterIt = m_clusterButtonGroups.find(clusterId); clusterIt != m_clusterButtonGroups.end())
        {
            auto cluster = clusterIt->second;
            auto newId = cluster->AddButton(icon);
            m_viewportUi->AddClusterButton(cluster->GetViewportUiElementId(), cluster->GetButton(newId));

            return newId;
        }

        return ButtonId(0);
    }

    const ButtonId ViewportUiManager::CreateSwitcherButton(
        const SwitcherId switcherId, const AZStd::string& icon, const AZStd::string& name)
    {
        if (auto switcherIt = m_switcherButtonGroups.find(switcherId); switcherIt != m_switcherButtonGroups.end())
        {
            auto switcher = switcherIt->second;
            auto newId = switcher->AddButton(icon, name);
            m_viewportUi->AddSwitcherButton(switcher->GetViewportUiElementId(), switcher->GetButton(newId));

            return newId;
        }

        return ButtonId(0);
    }

    void ViewportUiManager::RemoveCluster(const ClusterId clusterId)
    {
        if (auto clusterIt = m_clusterButtonGroups.find(clusterId); clusterIt != m_clusterButtonGroups.end())
        {
            m_viewportUi->RemoveViewportUiElement(clusterIt->second->GetViewportUiElementId());
            m_clusterButtonGroups.erase(clusterIt);
        }
    }

    void ViewportUiManager::RemoveSwitcher(SwitcherId switcherId)
    {
        if (auto switcherIt = m_switcherButtonGroups.find(switcherId); switcherIt != m_switcherButtonGroups.end())
        {
            m_viewportUi->RemoveViewportUiElement(switcherIt->second->GetViewportUiElementId());
            m_switcherButtonGroups.erase(switcherIt);
        }
    }

    void ViewportUiManager::RemoveSwitcherButton(SwitcherId switcherId, ButtonId buttonId)
    {
        if (auto switcherIt = m_switcherButtonGroups.find(switcherId); switcherIt != m_switcherButtonGroups.end())
        {
            auto switcher = switcherIt->second;
            m_viewportUi->RemoveSwitcherButton(switcher->GetViewportUiElementId(), buttonId);
            m_viewportUi->UpdateSwitcher(switcher->GetViewportUiElementId());
        }
    }

    static void SetViewportUiElementVisible(Internal::ViewportUiDisplay* ui, ViewportUiElementId id, bool visible)
    {
        if (visible)
        {
            ui->ShowViewportUiElement(id);
        }
        else
        {
            ui->HideViewportUiElement(id);
        }
    }

    void ViewportUiManager::SetClusterVisible(const ClusterId clusterId, bool visible)
    {
        if (auto clusterIt = m_clusterButtonGroups.find(clusterId); clusterIt != m_clusterButtonGroups.end())
        {
            auto cluster = clusterIt->second;
            SetViewportUiElementVisible(m_viewportUi.get(), cluster->GetViewportUiElementId(), visible);
        }
    }

    void ViewportUiManager::SetSwitcherVisible(const SwitcherId switcherId, bool visible)
    {
        if (auto switcherIt = m_switcherButtonGroups.find(switcherId); switcherIt != m_switcherButtonGroups.end())
        {
            auto switcher = switcherIt->second;
            SetViewportUiElementVisible(m_viewportUi.get(), switcher->GetViewportUiElementId(), visible);
        }
    }

    void ViewportUiManager::SetClusterGroupVisible(const AZStd::vector<ClusterId>& clusterGroup, bool visible)
    {
        for (auto clusterId : clusterGroup)
        {
            SetClusterVisible(clusterId, visible);
        }
    }

    const TextFieldId ViewportUiManager::CreateTextField(
        const AZStd::string& labelText, const AZStd::string& textFieldDefaultText, TextFieldValidationType validationType)
    {
        auto textField = AZStd::make_shared<Internal::TextField>(labelText, textFieldDefaultText, validationType);
        m_viewportUi->AddTextField(textField);

        return RegisterNewTextField(textField);
    }

    void ViewportUiManager::SetTextFieldText(TextFieldId textFieldId, const AZStd::string& text)
    {
        if (auto textFieldIt = m_textFields.find(textFieldId); textFieldIt != m_textFields.end())
        {
            auto textField = textFieldIt->second;
            textField->m_fieldText = text;
            UpdateTextFieldUi(textField.get());
        }
    }

    void ViewportUiManager::RegisterTextFieldCallback(TextFieldId textFieldId, AZ::Event<AZStd::string>::Handler& handler)
    {
        if (auto textFieldIt = m_textFields.find(textFieldId); textFieldIt != m_textFields.end())
        {
            auto textField = textFieldIt->second;
            textField->ConnectEventHandler(handler);
        }
    }

    void ViewportUiManager::RemoveTextField(TextFieldId textFieldId)
    {
        if (auto textFieldIt = m_textFields.find(textFieldId); textFieldIt != m_textFields.end())
        {
            m_viewportUi->RemoveViewportUiElement(textFieldIt->second->m_viewportId);
            m_textFields.erase(textFieldIt);
        }
    }

    void ViewportUiManager::SetTextFieldVisible(TextFieldId textFieldId, bool visible)
    {
        if (auto textFieldIt = m_textFields.find(textFieldId); textFieldIt != m_textFields.end())
        {
            auto textField = textFieldIt->second;
            SetViewportUiElementVisible(m_viewportUi.get(), textField->m_viewportId, visible);
        }
    }

    void ViewportUiManager::CreateViewportBorder(
        const AZStd::string& borderTitle, AZStd::optional<ViewportUiBackButtonCallback> backButtonCallback)
    {
        m_viewportUi->CreateViewportBorder(borderTitle, backButtonCallback);
    }

    bool ViewportUiManager::GetViewportBorderVisible() const
    {
        return m_viewportUi->GetViewportBorderVisible();
    }

    void ViewportUiManager::ChangeViewportBorderText(const AZStd::string& borderTitle)
    {
        m_viewportUi->ChangeViewportBorderText(borderTitle.c_str());
    }

    void ViewportUiManager::RemoveViewportBorder()
    {
        m_viewportUi->RemoveViewportBorder();
    }

    void ViewportUiManager::PressButton(ClusterId clusterId, ButtonId buttonId)
    {
        // Find cluster using ID and cluster map
        if (auto clusterIt = m_clusterButtonGroups.find(clusterId); clusterIt != m_clusterButtonGroups.end())
        {
            clusterIt->second->PressButton(buttonId);
        }
    }

    void ViewportUiManager::PressButton(SwitcherId switcherId, ButtonId buttonId)
    {
        // Find cluster using ID and cluster map
        if (auto switcherIt = m_switcherButtonGroups.find(switcherId); switcherIt != m_switcherButtonGroups.end())
        {
            switcherIt->second->PressButton(buttonId);
        }
    }

    void ViewportUiManager::InitializeViewportUi(QWidget* parent, QWidget* renderOverlay)
    {
        if (m_viewportUi)
        {
            AZ_Warning("ViewportUi", false, "Viewport UI already initialized. Removing previous ViewportUiDisplay.");
            m_viewportUi.reset();
        }

        m_viewportUi = AZStd::make_unique<Internal::ViewportUiDisplay>(parent, renderOverlay);
        m_viewportUi->InitializeUiOverlay();
    }

    void ViewportUiManager::Update()
    {
        m_viewportUi->Update();

        for (auto buttonGroup : m_clusterButtonGroups)
        {
            UpdateButtonGroupUi(buttonGroup.second.get());
        }
        for (auto buttonGroup : m_switcherButtonGroups)
        {
            UpdateButtonGroupUi(buttonGroup.second.get());
        }
        for (auto textField : m_textFields)
        {
            UpdateTextFieldUi(textField.second.get());
        }
    }

    ClusterId ViewportUiManager::RegisterNewCluster(AZStd::shared_ptr<Internal::ButtonGroup>& buttonGroup)
    {
        ClusterId newId = ClusterId(++m_nextViewportUiElementId);
        m_clusterButtonGroups.insert({ newId, buttonGroup });

        return newId;
    }

    SwitcherId ViewportUiManager::RegisterNewSwitcher(AZStd::shared_ptr<Internal::ButtonGroup>& buttonGroup)
    {
        SwitcherId newId = SwitcherId(++m_nextViewportUiElementId);
        m_switcherButtonGroups.insert({ newId, buttonGroup });

        return newId;
    }

    TextFieldId ViewportUiManager::RegisterNewTextField(AZStd::shared_ptr<Internal::TextField>& textField)
    {
        TextFieldId newId = TextFieldId(++m_nextViewportUiElementId);
        textField->m_textFieldId = newId;
        m_textFields.insert({ newId, textField });

        return newId;
    }

    void ViewportUiManager::UpdateButtonGroupUi(Internal::ButtonGroup* buttonGroup)
    {
        m_viewportUi->UpdateCluster(buttonGroup->GetViewportUiElementId());
    }

    void ViewportUiManager::UpdateSwitcherButtonGroupUi(Internal::ButtonGroup* buttonGroup)
    {
        m_viewportUi->UpdateSwitcher(buttonGroup->GetViewportUiElementId());
    }

    void ViewportUiManager::UpdateTextFieldUi(Internal::TextField* textField)
    {
        m_viewportUi->UpdateTextField(textField->m_viewportId);
    }
} // namespace AzToolsFramework::ViewportUi
