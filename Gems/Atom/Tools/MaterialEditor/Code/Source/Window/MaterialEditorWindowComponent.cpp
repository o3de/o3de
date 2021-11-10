/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Window/AtomToolsMainWindowFactoryRequestBus.h>
#include <AtomToolsFramework/Window/AtomToolsMainWindowRequestBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/ViewPaneOptions.h>
#include <AzToolsFramework/UI/UICore/QWidgetSavedState.h>
#include <Window/MaterialEditorWindow.h>
#include <Window/MaterialEditorWindowComponent.h>
#include <Window/ViewportSettingsInspector/ViewportSettingsInspector.h>

namespace MaterialEditor
{
    void MaterialEditorWindowComponent::Reflect(AZ::ReflectContext* context)
    {
        MaterialEditorWindowSettings::Reflect(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<MaterialEditorWindowComponent, AZ::Component>()
                ->Version(0);
        }
    }

    void MaterialEditorWindowComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("AssetBrowserService"));
        required.push_back(AZ_CRC_CE("PropertyManagerService"));
        required.push_back(AZ_CRC_CE("SourceControlService"));
        required.push_back(AZ_CRC_CE("AtomToolsMainWindowSystemService"));
    }

    void MaterialEditorWindowComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("MaterialEditorWindowService"));
    }

    void MaterialEditorWindowComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("MaterialEditorWindowService"));
    }

    void MaterialEditorWindowComponent::Init()
    {
    }

    void MaterialEditorWindowComponent::Activate()
    {
        AzToolsFramework::EditorWindowRequestBus::Handler::BusConnect();
        AtomToolsFramework::AtomToolsMainWindowFactoryRequestBus::Handler::BusConnect();
        AzToolsFramework::SourceControlConnectionRequestBus::Broadcast(&AzToolsFramework::SourceControlConnectionRequests::EnableSourceControl, true);
    }

    void MaterialEditorWindowComponent::Deactivate()
    {
        AtomToolsFramework::AtomToolsMainWindowFactoryRequestBus::Handler::BusDisconnect();
        AzToolsFramework::EditorWindowRequestBus::Handler::BusDisconnect();

        m_window.reset();
    }

    void MaterialEditorWindowComponent::CreateMainWindow()
    {
        m_materialEditorBrowserInteractions.reset(aznew MaterialEditorBrowserInteractions);

        m_window.reset(aznew MaterialEditorWindow);
    }

    void MaterialEditorWindowComponent::DestroyMainWindow()
    {
        m_window.reset();
    }

    QWidget* MaterialEditorWindowComponent::GetAppMainWindow()
    {
        return m_window.get();
    }

}
