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

#include "AzToolsFramework_precompiled.h"

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzToolsFramework/ViewportUi/Button.h>
#include <AzToolsFramework/ViewportUi/Cluster.h>
#include <AzToolsFramework/ViewportUi/ViewportUiManager.h>
#include <AzToolsFramework/ViewportUi/ViewportUiDisplay.h>

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

    const ClusterId ViewportUiManager::CreateCluster()
    {
        auto buttonGroup = AZStd::make_shared<Internal::ButtonGroup>();
        m_viewportUi->AddCluster(buttonGroup);

        return RegisterNewCluster(buttonGroup);
    }

    const SwitcherId ViewportUiManager::CreateSwitcher(ButtonId currMode)
    {
        auto buttonGroup = AZStd::make_shared<Internal::ButtonGroup>();
        m_viewportUi->AddSwitcher(buttonGroup, currMode);

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

    void ViewportUiManager::SetSwitcherActiveButton(const SwitcherId switcherId, const ButtonId buttonId)
    {
        if (auto switcherIt = m_switcherButtonGroups.find(switcherId); switcherIt != m_switcherButtonGroups.end())
        {
            auto switcher = switcherIt->second;
            switcher->SetHighlightedButton(buttonId);
            m_viewportUi->SetSwitcherActiveMode(switcher->GetViewportUiElementId(), buttonId);
            UpdateButtonGroupUi(switcher.get());
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

    void ViewportUiManager::RegisterSwitcherEventHandler(SwitcherId switcherId, AZ::Event<ButtonId>::Handler& handler)
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

    const ButtonId ViewportUiManager::CreateSwitcherButton(const SwitcherId switcherId, const AZStd::string& icon, const AZStd::string& name)
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
            m_clusterButtonGroups.erase(clusterIt);
            m_viewportUi->RemoveViewportUiElement(clusterIt->second->GetViewportUiElementId());
        }
    }

    void ViewportUiManager::RemoveSwitcher(SwitcherId switcherId)
    {
        if (auto switcherIt = m_switcherButtonGroups.find(switcherId); switcherIt != m_switcherButtonGroups.end())
        {
            m_switcherButtonGroups.erase(switcherIt);
            m_viewportUi->RemoveViewportUiElement(switcherIt->second->GetViewportUiElementId());
        }
    }

    static void SetViewportUiElementVisible(
        Internal::ViewportUiDisplay* ui, ViewportUiElementId id, bool visible)
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

    void ViewportUiManager::SetClusterVisible(ClusterId clusterId, bool visible)
    {
        if (auto clusterEntry = m_clusterButtonGroups.find(clusterId); clusterEntry != m_clusterButtonGroups.end())
        {
            auto cluster = clusterEntry->second;
            SetViewportUiElementVisible(m_viewportUi.get(), cluster->GetViewportUiElementId(), visible);
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
        const AZStd::string& labelText, const AZStd::string& textFieldDefaultText,
        TextFieldValidationType validationType) 
    {
        auto textField = AZStd::make_shared<Internal::TextField>(
            labelText, textFieldDefaultText, validationType);
        m_viewportUi->AddTextField(textField);

        return RegisterNewTextField(textField);
    }

    void ViewportUiManager::SetTextFieldText(TextFieldId textFieldId, const AZStd::string& text)
    {
        if (auto textFieldEntry = m_textFields.find(textFieldId); textFieldEntry != m_textFields.end())
        {
            auto textField = textFieldEntry->second;
            textField->m_fieldText = text;
            UpdateTextFieldUi(textField.get());
        }
    }

    void ViewportUiManager::RegisterTextFieldCallback(
        TextFieldId textFieldId, AZ::Event<AZStd::string>::Handler& handler) 
    {
        if (auto textFieldEntry = m_textFields.find(textFieldId); textFieldEntry != m_textFields.end())
        {
            auto textField = textFieldEntry->second;
            textField->ConnectEventHandler(handler);
        }
    }

    void ViewportUiManager::RemoveTextField(TextFieldId textFieldId)
    {
        if (auto textFieldEntry = m_textFields.find(textFieldId); textFieldEntry != m_textFields.end())
        {
            m_textFields.erase(textFieldEntry);
            m_viewportUi->RemoveViewportUiElement(textFieldEntry->second->m_viewportId);
        }
    }

    void ViewportUiManager::SetTextFieldVisible(TextFieldId textFieldId, bool visible)
    {
        if (auto textFieldEntry = m_textFields.find(textFieldId); textFieldEntry != m_textFields.end())
        {
            auto textField = textFieldEntry->second;
            SetViewportUiElementVisible(m_viewportUi.get(), textField->m_viewportId, visible);
        }
    }


    void ViewportUiManager::CreateComponentModeBorder(const AZStd::string& borderTitle)
    {
        m_viewportUi->CreateComponentModeBorder(borderTitle);
    }

    void ViewportUiManager::RemoveComponentModeBorder()
    {
        m_viewportUi->RemoveComponentModeBorder();
    }

    void ViewportUiManager::PressButton(ClusterId clusterId, ButtonId buttonId)
    {
        // Find cluster using ID and cluster map
        if (auto clusterEntry = m_clusterButtonGroups.find(clusterId);
            clusterEntry != m_clusterButtonGroups.end())
        {
            clusterEntry->second->PressButton(buttonId);
        }
    }

    void ViewportUiManager::PressButton(SwitcherId switcherId, ButtonId buttonId)
    {
        // Find cluster using ID and cluster map
        if (auto switcherEntry = m_switcherButtonGroups.find(switcherId); switcherEntry != m_switcherButtonGroups.end())
        {
            switcherEntry->second->PressButton(buttonId);
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
        for (auto textFieldEntry : m_textFields)
        {
            UpdateTextFieldUi(textFieldEntry.second.get());
        }
    }

    ClusterId ViewportUiManager::RegisterNewCluster(AZStd::shared_ptr<Internal::ButtonGroup>& buttonGroup)
    {
        ClusterId newId = ClusterId(m_clusterButtonGroups.size() + 1);
        m_clusterButtonGroups.insert({ newId, buttonGroup });

        return newId;
    }

    SwitcherId ViewportUiManager::RegisterNewSwitcher(AZStd::shared_ptr<Internal::ButtonGroup>& buttonGroup)
    {
        SwitcherId newId = SwitcherId(m_switcherButtonGroups.size() + 1);
        m_switcherButtonGroups.insert({newId, buttonGroup});

        return newId;
    }

    TextFieldId ViewportUiManager::RegisterNewTextField(AZStd::shared_ptr<Internal::TextField>& textField)
    {
        TextFieldId newId = TextFieldId(m_textFields.size() + 1);
        textField->m_textFieldId = newId;
        m_textFields.insert({ newId, textField });

        return newId;
    }

    void ViewportUiManager::UpdateButtonGroupUi(Internal::ButtonGroup* buttonGroup)
    {
        m_viewportUi->UpdateCluster(buttonGroup->GetViewportUiElementId());
    }

    void ViewportUiManager::UpdateTextFieldUi(Internal::TextField* textField)
    {
        m_viewportUi->UpdateTextField(textField->m_viewportId);
    }
} // namespace AzToolsFramework::ViewportUi
