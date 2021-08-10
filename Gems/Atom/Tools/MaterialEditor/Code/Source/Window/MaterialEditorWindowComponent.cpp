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

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<AtomToolsFramework::AtomToolsMainWindowFactoryRequestBus>("MaterialEditorWindowAtomRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "materialeditor")
                ->Event("CreateMaterialEditorWindow", &AtomToolsFramework::AtomToolsMainWindowFactoryRequestBus::Events::CreateMainWindow)
                ->Event("DestroyMaterialEditorWindow", &AtomToolsFramework::AtomToolsMainWindowFactoryRequestBus::Events::DestroyMainWindow)
                ;

            behaviorContext->EBus<AtomToolsFramework::AtomToolsMainWindowRequestBus>("MaterialEditorWindowRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "materialeditor")
                ->Event("ActivateWindow", &AtomToolsFramework::AtomToolsMainWindowRequestBus::Events::ActivateWindow)
                ->Event("SetDockWidgetVisible", &AtomToolsFramework::AtomToolsMainWindowRequestBus::Events::SetDockWidgetVisible)
                ->Event("IsDockWidgetVisible", &AtomToolsFramework::AtomToolsMainWindowRequestBus::Events::IsDockWidgetVisible)
                ->Event("GetDockWidgetNames", &AtomToolsFramework::AtomToolsMainWindowRequestBus::Events::GetDockWidgetNames)
                ->Event("ResizeViewportRenderTarget", &AtomToolsFramework::AtomToolsMainWindowRequestBus::Events::ResizeViewportRenderTarget)
                ->Event("LockViewportRenderTargetSize", &AtomToolsFramework::AtomToolsMainWindowRequestBus::Events::LockViewportRenderTargetSize)
                ->Event("UnlockViewportRenderTargetSize", &AtomToolsFramework::AtomToolsMainWindowRequestBus::Events::UnlockViewportRenderTargetSize)
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
