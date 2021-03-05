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

#include <Window/ViewportSettingsInspector/ViewportSettingsInspector.h>
#include <Atom/Viewport/MaterialViewportRequestBus.h>

#include <AtomToolsFramework/Inspector/InspectorPropertyGroupWidget.h>

namespace MaterialEditor
{
    ViewportSettingsInspector::ViewportSettingsInspector(QWidget* parent)
        : AtomToolsFramework::InspectorWidget(parent)
    {
        MaterialViewportNotificationBus::Handler::BusConnect();
    }

    ViewportSettingsInspector::~ViewportSettingsInspector()
    {
        m_lightingPreset.reset();
        m_modelPreset.reset();
        MaterialViewportNotificationBus::Handler::BusDisconnect();
        AtomToolsFramework::InspectorRequestBus::Handler::BusDisconnect();
    }

    void ViewportSettingsInspector::Popuate()
    {
        AddGroupsBegin();

        m_modelPreset.reset();
        MaterialViewportRequestBus::BroadcastResult(m_modelPreset, &MaterialViewportRequestBus::Events::GetModelPresetSelection);

        if (m_modelPreset)
        {
            const AZStd::string groupNameId = "model";
            const AZStd::string groupDisplayName = "Model";
            const AZStd::string groupDescription = "Model";

            AddGroup(groupNameId, groupDisplayName, groupDescription,
                new AtomToolsFramework::InspectorPropertyGroupWidget(m_modelPreset.get(), m_modelPreset.get()->TYPEINFO_Uuid(), this));
        }

        m_lightingPreset.reset();
        MaterialViewportRequestBus::BroadcastResult(m_lightingPreset, &MaterialViewportRequestBus::Events::GetLightingPresetSelection);

        if (m_lightingPreset)
        {
            const AZStd::string groupNameId = "lighting";
            const AZStd::string groupDisplayName = "Lighting";
            const AZStd::string groupDescription = "Lighting";

            AddGroup(groupNameId, groupDisplayName, groupDescription,
                new AtomToolsFramework::InspectorPropertyGroupWidget(m_lightingPreset.get(), m_lightingPreset.get()->TYPEINFO_Uuid(), this));
        }

        AddGroupsEnd();
    }

    void ViewportSettingsInspector::Reset()
    {
        m_lightingPreset.reset();
        m_modelPreset.reset();
        AtomToolsFramework::InspectorRequestBus::Handler::BusDisconnect();
        AtomToolsFramework::InspectorWidget::Reset();
    }

    void ViewportSettingsInspector::OnLightingPresetSelected([[maybe_unused]] AZ::Render::LightingPresetPtr preset)
    {
        if (m_lightingPreset != preset)
        {
            Popuate();
        }
    }

    void ViewportSettingsInspector::OnModelPresetSelected([[maybe_unused]] AZ::Render::ModelPresetPtr preset)
    {
        if (m_modelPreset != preset)
        {
            Popuate();
        }
    }

    void ViewportSettingsInspector::BeforePropertyModified(AzToolsFramework::InstanceDataNode* pNode)
    {
        AZ_UNUSED(pNode);
    }

    void ViewportSettingsInspector::AfterPropertyModified(AzToolsFramework::InstanceDataNode* pNode)
    {
        AZ_UNUSED(pNode);
        MaterialViewportNotificationBus::Broadcast(&MaterialViewportNotificationBus::Events::OnLightingPresetChanged, m_lightingPreset);
        MaterialViewportNotificationBus::Broadcast(&MaterialViewportNotificationBus::Events::OnModelPresetChanged, m_modelPreset);
    }

    void ViewportSettingsInspector::SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* pNode)
    {
        AZ_UNUSED(pNode);
        MaterialViewportNotificationBus::Broadcast(&MaterialViewportNotificationBus::Events::OnLightingPresetChanged, m_lightingPreset);
        MaterialViewportNotificationBus::Broadcast(&MaterialViewportNotificationBus::Events::OnModelPresetChanged, m_modelPreset);
    }

} // namespace MaterialEditor

#include <Source/Window/ViewportSettingsInspector/moc_ViewportSettingsInspector.cpp>
