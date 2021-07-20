/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Window/MaterialEditorWindowFactoryRequestBus.h>
#include <Atom/Window/MaterialEditorWindowSettings.h>
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

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<MaterialEditorWindowFactoryRequestBus>("MaterialEditorWindowFactoryRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "materialeditor")
                ->Event("CreateMaterialEditorWindow", &MaterialEditorWindowFactoryRequestBus::Events::CreateMaterialEditorWindow)
                ->Event("DestroyMaterialEditorWindow", &MaterialEditorWindowFactoryRequestBus::Events::DestroyMaterialEditorWindow)
                ;

            behaviorContext->EBus<MaterialEditorWindowRequestBus>("MaterialEditorWindowRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "materialeditor")
                ->Event("ActivateWindow", &MaterialEditorWindowRequestBus::Events::ActivateWindow)
                ->Event("SetDockWidgetVisible", &MaterialEditorWindowRequestBus::Events::SetDockWidgetVisible)
                ->Event("IsDockWidgetVisible", &MaterialEditorWindowRequestBus::Events::IsDockWidgetVisible)
                ->Event("GetDockWidgetNames", &MaterialEditorWindowRequestBus::Events::GetDockWidgetNames)
                ->Event("ResizeViewportRenderTarget", &MaterialEditorWindowRequestBus::Events::ResizeViewportRenderTarget)
                ->Event("LockViewportRenderTargetSize", &MaterialEditorWindowRequestBus::Events::LockViewportRenderTargetSize)
                ->Event("UnlockViewportRenderTargetSize", &MaterialEditorWindowRequestBus::Events::UnlockViewportRenderTargetSize)
                ;
        }
    }

    void MaterialEditorWindowComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("AssetBrowserService", 0x1e54fffb));
        required.push_back(AZ_CRC("PropertyManagerService", 0x63a3d7ad));
        required.push_back(AZ_CRC("SourceControlService", 0x67f338fd));
    }

    void MaterialEditorWindowComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("MaterialEditorWindowService", 0xb6e7d922));
    }

    void MaterialEditorWindowComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("MaterialEditorWindowService", 0xb6e7d922));
    }

    void MaterialEditorWindowComponent::Init()
    {
    }

    void MaterialEditorWindowComponent::Activate()
    {
        AzToolsFramework::EditorWindowRequestBus::Handler::BusConnect();
        MaterialEditorWindowFactoryRequestBus::Handler::BusConnect();
        AzToolsFramework::SourceControlConnectionRequestBus::Broadcast(&AzToolsFramework::SourceControlConnectionRequests::EnableSourceControl, true);
    }

    void MaterialEditorWindowComponent::Deactivate()
    {
        MaterialEditorWindowFactoryRequestBus::Handler::BusDisconnect();
        AzToolsFramework::EditorWindowRequestBus::Handler::BusDisconnect();

        m_window.reset();
    }

    void MaterialEditorWindowComponent::CreateMaterialEditorWindow()
    {
        m_materialEditorBrowserInteractions.reset(aznew MaterialEditorBrowserInteractions);

        m_window.reset(aznew MaterialEditorWindow);
    }

    void MaterialEditorWindowComponent::DestroyMaterialEditorWindow()
    {
        m_window.reset();
    }

    QWidget* MaterialEditorWindowComponent::GetAppMainWindow()
    {
        return m_window.get();
    }

}
