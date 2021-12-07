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
#include <ScriptCanvas/Assets/ScriptCanvasAsset.h>
#include <ScriptCanvas/Assets/ScriptCanvasAssetHandler.h>
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
        provided.push_back(AZ_CRC("ScriptCanvasEditorAssetService", 0x4a1c043d));
    }

    void EditorAssetSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("AssetDatabaseService", 0x3abf5601));
        required.push_back(AZ_CRC("AssetCatalogService", 0xc68ffc57));
        required.push_back(AZ_CRC("ScriptCanvasService", 0x41fd58f3));
    }

    void EditorAssetSystemComponent::Init()
    {
    }

    void EditorAssetSystemComponent::Activate()
    {
        m_editorAssetRegistry.Register<ScriptCanvas::SubgraphInterfaceAsset, ScriptCanvas::SubgraphInterfaceAssetHandler, ScriptCanvas::SubgraphInterfaceAssetDescription>();

        AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusConnect();
        EditorAssetConversionBus::Handler::BusConnect();
    }

    void EditorAssetSystemComponent::Deactivate()
    {
        EditorAssetConversionBus::Handler::BusDisconnect();
        AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusDisconnect();
        m_editorAssetRegistry.Unregister();
    }

    ScriptCanvas::AssetRegistry& EditorAssetSystemComponent::GetAssetRegistry()
    {
        return m_editorAssetRegistry;
    }

    static bool HandlesSource(const AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry* entry)
    {
        AZStd::string_view targetExtension = entry->GetExtension();

        ScriptCanvasAsset::Description description;
        AZStd::string_view scriptCanvasFileFilter = description.GetFileFilterImpl();

        if (AZStd::wildcard_match(scriptCanvasFileFilter.data(), targetExtension.data()))
        {
            return true;
        }

        return false;
    }

    AZ::Outcome<AZ::Data::Asset<ScriptCanvas::RuntimeAsset>, AZStd::string> EditorAssetSystemComponent::CreateRuntimeAsset(const SourceHandle& editAsset)
    {
        return ScriptCanvasBuilder::CreateRuntimeAsset(editAsset);
    }

    AZ::Outcome<ScriptCanvas::Translation::LuaAssetResult, AZStd::string> EditorAssetSystemComponent::CreateLuaAsset(const SourceHandle& editAsset, AZStd::string_view graphPathForRawLuaFile)
    {
        return ScriptCanvasBuilder::CreateLuaAsset(editAsset, graphPathForRawLuaFile);
    }

    void EditorAssetSystemComponent::AddSourceFileOpeners
        ( [[maybe_unused]] const char* fullSourceFileName
        , const AZ::Uuid& sourceUuid
        , AzToolsFramework::AssetBrowser::SourceFileOpenerList& openers)
    {
        using namespace AzToolsFramework;
        using namespace AzToolsFramework::AssetBrowser;
        // get the full details of the source file based on its UUID.
        if (const SourceAssetBrowserEntry* source = SourceAssetBrowserEntry::GetSourceByUuid(sourceUuid))  
        {
            if (!HandlesSource(source))
            {
                return;
            }
        }
        else
        {
            // has no UUID / Not a source file.
            return;
        }

        // You can push back any number of "Openers" - choose a unique identifier, and icon,
        // and then a lambda which will be activated if the user chooses to open it with your opener:

        openers.push_back({ "ScriptCanvas_Editor_Asset_Edit", "Script Canvas Editor..."
            , QIcon(ScriptCanvasAssetDescription().GetIconPathImpl()),
            [](const char*, const AZ::Uuid& scSourceUuid)
            {
                AzToolsFramework::OpenViewPane(LyViewPane::ScriptCanvas);

                if (auto sourceHandle = CompleteDescription(SourceHandle(nullptr, scSourceUuid, {})))
                {
                    AZ::Outcome<int, AZStd::string> openOutcome = AZ::Failure(AZStd::string());
                    GeneralRequestBus::BroadcastResult(openOutcome, &GeneralRequests::OpenScriptCanvasAsset
                        , *sourceHandle, Tracker::ScriptCanvasFileState::UNMODIFIED, -1);
                    if (!openOutcome)
                    {
                        AZ_Warning("ScriptCanvas", openOutcome, "%s", openOutcome.GetError().data());
                    }
                }
                else
                {
                    AZ_Warning("ScriptCanvas", false
                        , "Unabled to find full path for Source UUid %s", scSourceUuid.ToString<AZStd::string>().c_str());
                }            
            }});
    }
}
