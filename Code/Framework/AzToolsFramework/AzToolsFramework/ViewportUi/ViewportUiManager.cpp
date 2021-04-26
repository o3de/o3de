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
        auto cluster = AZStd::make_shared<Internal::Cluster>();
        m_viewportUi->AddCluster(cluster);

        return RegisterNewCluster(cluster);
    }

    const ClusterId ViewportUiManager::CreateSwitcher()
    {
        auto cluster = AZStd::make_shared<Internal::Cluster>();
        m_viewportUi->AddSwitcher(cluster);

        return RegisterNewCluster(cluster);
    }

    void ViewportUiManager::SetClusterActiveButton(const ClusterId clusterId, const ButtonId buttonId)
    {
        if (auto clusterEntry = m_clusters.find(clusterId); clusterEntry != m_clusters.end())
        {
            auto cluster = clusterEntry->second;
            cluster->SetHighlightedButton(buttonId);
            UpdateClusterUi(cluster.get());
        }
    }

    void ViewportUiManager::SetSwitcherActiveButton(const ClusterId clusterId, const ButtonId buttonId)
    {
        if (auto clusterEntry = m_clusters.find(clusterId); clusterEntry != m_clusters.end())
        {
            auto cluster = clusterEntry->second;
            cluster->SetHighlightedButton(buttonId);
            m_viewportUi->SetSwitcherActiveButton(cluster->GetViewportUiElementId(), buttonId);
            UpdateSwitcherUi(cluster.get());
        }
    }

    void ViewportUiManager::RegisterClusterEventHandler(const ClusterId clusterId, AZ::Event<ButtonId>::Handler& handler)
    {
        if (auto clusterEntry = m_clusters.find(clusterId); clusterEntry != m_clusters.end())
        {
            auto cluster = clusterEntry->second;
            cluster->ConnectEventHandler(handler);
        }
    }

    const ButtonId ViewportUiManager::CreateClusterButton(const ClusterId clusterId, const AZStd::string& icon)
    {
        if (auto clusterEntry = m_clusters.find(clusterId); clusterEntry != m_clusters.end())
        {
            auto cluster = clusterEntry->second;
            auto newId = cluster->AddButton(icon);
            m_viewportUi->AddClusterButton(cluster->GetViewportUiElementId(), cluster->GetButton(newId));

            return newId;
        }

        return ButtonId(0);
    }

    const ButtonId ViewportUiManager::CreateSwitcherButton(const ClusterId clusterId, const AZStd::string& icon, const AZStd::string& name)
    {
        if (auto clusterEntry = m_clusters.find(clusterId); clusterEntry != m_clusters.end())
        {
            auto cluster = clusterEntry->second;
            auto newId = cluster->AddButton(icon, name);
            m_viewportUi->AddSwitcherButton(cluster->GetViewportUiElementId(), cluster->GetButton(newId));

            return newId;
        }

        return ButtonId(0);
    }

    void ViewportUiManager::RemoveCluster(const ClusterId clusterId)
    {
        if (auto clusterEntry = m_clusters.find(clusterId); clusterEntry != m_clusters.end())
        {
            m_clusters.erase(clusterEntry);
            m_viewportUi->RemoveViewportUiElement(clusterEntry->second->GetViewportUiElementId());
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
        if (auto clusterEntry = m_clusters.find(clusterId); clusterEntry != m_clusters.end())
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
        if (auto clusterEntry = m_clusters.find(clusterId);
            clusterEntry != m_clusters.end())
        {
            clusterEntry->second->PressButton(buttonId);
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

        for (auto clusterEntry : m_clusters)
        {
            UpdateClusterUi(clusterEntry.second.get());
        }
        for (auto textFieldEntry : m_textFields)
        {
            UpdateTextFieldUi(textFieldEntry.second.get());
        }
    }

    ClusterId ViewportUiManager::RegisterNewCluster(AZStd::shared_ptr<Internal::Cluster>& cluster)
    {
        ClusterId newId = ClusterId(m_clusters.size() + 1);
        cluster->SetClusterId(newId);
        m_clusters.insert({ newId, cluster });

        return newId;
    }

    TextFieldId ViewportUiManager::RegisterNewTextField(AZStd::shared_ptr<Internal::TextField>& textField)
    {
        TextFieldId newId = TextFieldId(m_textFields.size() + 1);
        textField->m_textFieldId = newId;
        m_textFields.insert({ newId, textField });

        return newId;
    }

    void ViewportUiManager::UpdateClusterUi(Internal::Cluster* cluster)
    {
        m_viewportUi->UpdateCluster(cluster->GetViewportUiElementId());
    }

    void ViewportUiManager::UpdateSwitcherUi(Internal::Cluster* cluster)
    {
        m_viewportUi->UpdateSwitcher(cluster->GetViewportUiElementId());
    }

    void ViewportUiManager::UpdateTextFieldUi(Internal::TextField* textField)
    {
        m_viewportUi->UpdateTextField(textField->m_viewportId);
    }
} // namespace AzToolsFramework::ViewportUi
