/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"

#include "LuaIDEApplication.h"

#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzFramework/Script/ScriptRemoteDebugging.h>
#include <AzFramework/Asset/AssetCatalogComponent.h>
#include <AzToolsFramework/SourceControl/PerforceComponent.h>
#include <AzToolsFramework/Asset/AssetSystemComponent.h>
#include <AzFramework/Asset/AssetSystemComponent.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorSelectionAccentSystemComponent.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerNullComponent.h>

#include <Source/AssetDatabaseLocationListener.h>
#include <Source/LUA/LUAEditorContext.h>
#include <Source/LUA/LUADebuggerComponent.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyManagerComponent.h>


namespace LUAEditor
{

    Application::Application(int &argc, char **argv) : BaseApplication(argc, argv)
    {
        AzToolsFramework::SourceControlNotificationBus::Handler::BusConnect();
    }

    Application::~Application()
    {
        AzToolsFramework::SourceControlNotificationBus::Handler::BusDisconnect();
    }

    void Application::RegisterCoreComponents()
    {
        StandaloneTools::BaseApplication::RegisterCoreComponents();

        RegisterComponentDescriptor(LUAEditor::Context::CreateDescriptor());
        RegisterComponentDescriptor(LUADebugger::Component::CreateDescriptor());
        RegisterComponentDescriptor(AzFramework::CreateScriptDebugAgentFactory());
        RegisterComponentDescriptor(AzToolsFramework::PerforceComponent::CreateDescriptor());
        RegisterComponentDescriptor(AzFramework::AssetCatalogComponent::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::Components::PropertyManagerComponent::CreateDescriptor());
        RegisterComponentDescriptor(AzFramework::AssetSystem::AssetSystemComponent::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::AssetSystem::AssetSystemComponent::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::Thumbnailer::ThumbnailerNullComponent::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::AssetBrowser::AssetBrowserComponent::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::Components::EditorSelectionAccentSystemComponent::CreateDescriptor());

        RegisterComponentDescriptor(AzFramework::CreateScriptDebugAgentFactory());
    }

    void Application::CreateApplicationComponents()
    {
        StandaloneTools::BaseApplication::CreateApplicationComponents();

        EnsureComponentCreated(LUAEditor::Context::RTTI_Type());
        EnsureComponentCreated(LUADebugger::Component::RTTI_Type());
        EnsureComponentCreated(AZ::Uuid("{624a7be2-3c7e-4119-aee2-1db2bdb6cc89}"));
        EnsureComponentCreated(AzToolsFramework::PerforceComponent::RTTI_Type());
        EnsureComponentCreated(AzFramework::AssetCatalogComponent::RTTI_Type());
        EnsureComponentCreated(AZ::AssetManagerComponent::RTTI_Type());
        EnsureComponentCreated(AzToolsFramework::Components::PropertyManagerComponent::RTTI_Type());
        EnsureComponentCreated(AzFramework::AssetSystem::AssetSystemComponent::RTTI_Type());
        EnsureComponentCreated(AzToolsFramework::AssetSystem::AssetSystemComponent::RTTI_Type());
        EnsureComponentCreated(AzToolsFramework::Thumbnailer::ThumbnailerNullComponent::RTTI_Type());
        EnsureComponentCreated(AzToolsFramework::AssetBrowser::AssetBrowserComponent::RTTI_Type());
        EnsureComponentCreated(AzToolsFramework::Components::EditorSelectionAccentSystemComponent::RTTI_Type());
    }

    void Application::ConnectivityStateChanged(const AzToolsFramework::SourceControlState state)
    {
        using SCConnectionBus = AzToolsFramework::SourceControlConnectionRequestBus;
        using AzToolsFramework::SourceControlState;

        // If status invalid, just disconnect from source control
        if (state == SourceControlState::ConfigurationInvalid)
        {
            SCConnectionBus::Broadcast(&SCConnectionBus::Events::EnableSourceControl, false);
        }
    }

}
