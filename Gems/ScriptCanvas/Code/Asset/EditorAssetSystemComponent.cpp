/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Asset/EditorAssetSystemComponent.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/string/wildcard.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/ToolsComponents/ToolsAssetCatalogBus.h>
#include <Builder/ScriptCanvasBuilderWorker.h>
#include <LyViewPaneNames.h>

 // Undo this
AZ_PUSH_DISABLE_WARNING(4251 4800 4244, "-Wunknown-warning-option")
#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Asset/SubgraphInterfaceAsset.h>
#include <ScriptCanvas/Components/EditorGraph.h>
#include <ScriptCanvas/Components/EditorGraphVariableManagerComponent.h>
AZ_POP_DISABLE_WARNING

#include <ScriptCanvas/Asset/SubgraphInterfaceAssetHandler.h>

namespace ScriptCanvasEditor
{
    EditorAssetSystemComponent::~EditorAssetSystemComponent()
    {
    }

    void EditorAssetSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorAssetSystemComponent, AZ::Component>()
                ->Version(0)
                ;
        }
    }

    void EditorAssetSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("ScriptCanvasEditorAssetService"));
    }

    void EditorAssetSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("AssetDatabaseService"));
        required.push_back(AZ_CRC_CE("AssetCatalogService"));
        required.push_back(AZ_CRC_CE("ScriptCanvasService"));
    }

    void EditorAssetSystemComponent::Init()
    {
    }

    void EditorAssetSystemComponent::Activate()
    {
        m_editorAssetRegistry.Register<ScriptCanvas::SubgraphInterfaceAsset, ScriptCanvas::SubgraphInterfaceAssetHandler, ScriptCanvas::SubgraphInterfaceAssetDescription>();

        EditorAssetConversionBus::Handler::BusConnect();
    }

    void EditorAssetSystemComponent::Deactivate()
    {
        EditorAssetConversionBus::Handler::BusDisconnect();
        m_editorAssetRegistry.Unregister();
    }

    ScriptCanvas::AssetRegistry& EditorAssetSystemComponent::GetAssetRegistry()
    {
        return m_editorAssetRegistry;
    }

    AZ::Outcome<AZ::Data::Asset<ScriptCanvas::RuntimeAsset>, AZStd::string> EditorAssetSystemComponent::CreateRuntimeAsset(const SourceHandle& editAsset)
    {
        return ScriptCanvasBuilder::CreateRuntimeAsset(editAsset);
    }

    AZ::Outcome<ScriptCanvas::Translation::LuaAssetResult, AZStd::string> EditorAssetSystemComponent::CreateLuaAsset(const SourceHandle& editAsset, AZStd::string_view graphPathForRawLuaFile)
    {
        return ScriptCanvasBuilder::CreateLuaAsset(editAsset, graphPathForRawLuaFile);
    }
}
