/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "precompiled.h"

#include <ScriptCanvas/Assets/ScriptCanvasAssetHandler.h>
#include <ScriptCanvas/Assets/ScriptCanvasAsset.h>

#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Bus/ScriptCanvasBus.h>
#include <Core/ScriptCanvasBus.h>
#include <ScriptCanvas/Components/EditorGraph.h>
#include <ScriptCanvas/Components/EditorScriptCanvasComponent.h>
#include <ScriptCanvas/Components/EditorGraphVariableManagerComponent.h>

#include <GraphCanvas/GraphCanvasBus.h>

#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/std/string/string_view.h>

#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

namespace ScriptCanvasEditor
{
    ScriptCanvasAssetHandler::ScriptCanvasAssetHandler(AZ::SerializeContext* context)
    {
        SetSerializeContext(context);

        AZ::AssetTypeInfoBus::MultiHandler::BusConnect(GetAssetType());
    }

    ScriptCanvasAssetHandler::~ScriptCanvasAssetHandler()
    {
        AZ::AssetTypeInfoBus::MultiHandler::BusDisconnect();
    }

    AZ::Data::AssetPtr ScriptCanvasAssetHandler::CreateAsset(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type)
    {
        (void)type;
        auto assetData = aznew ScriptCanvasAsset(id);

        AZ::Entity* scriptCanvasEntity = aznew AZ::Entity("Script Canvas Graph");
        SystemRequestBus::Broadcast(&SystemRequests::CreateEditorComponentsOnEntity, scriptCanvasEntity, azrtti_typeid<ScriptCanvas::RuntimeAsset>());

        assetData->SetScriptCanvasEntity(scriptCanvasEntity);

        return assetData;
    }

    // Override the stream info to force source assets to load into the Editor instead of cached, processed assets.
    void ScriptCanvasAssetHandler::GetCustomAssetStreamInfoForLoad(AZ::Data::AssetStreamInfo& streamInfo)
    {
        //ScriptCanvas files are source assets and should be placed in a source asset directory
        const char* assetPath = streamInfo.m_streamName.c_str();
        if (AzFramework::StringFunc::Path::IsRelative(assetPath))
        {
            AZStd::string watchFolder;
            bool sourceInfoFound{};
            AZ::Data::AssetInfo assetInfo;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(sourceInfoFound, &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath, assetPath, assetInfo, watchFolder);
            if (sourceInfoFound)
            {
                AzFramework::StringFunc::Path::Join(watchFolder.data(), assetInfo.m_relativePath.data(), streamInfo.m_streamName);
            }
        }
    }


    AZ::Data::AssetHandler::LoadResult ScriptCanvasAssetHandler::LoadAssetData(
        const AZ::Data::Asset<AZ::Data::AssetData>& asset,
        AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
        const AZ::Data::AssetFilterCB& assetLoadFilterCB)
    {
        auto* scriptCanvasAsset = asset.GetAs<ScriptCanvasAsset>();
        AZ_Assert(scriptCanvasAsset, "This should be an scene slice asset, as this is the only type we process!");
        if (scriptCanvasAsset && m_serializeContext)
        {
            stream->Seek(0U, AZ::IO::GenericStream::ST_SEEK_BEGIN);
            // tolerate unknown classes in the editor.  Let the asset processor warn about bad nodes...
            bool loadSuccess = AZ::Utils::LoadObjectFromStreamInPlace(*stream, scriptCanvasAsset->GetScriptCanvasData(), m_serializeContext, AZ::ObjectStream::FilterDescriptor(assetLoadFilterCB, AZ::ObjectStream::FILTERFLAG_IGNORE_UNKNOWN_CLASSES));
            return loadSuccess ? AZ::Data::AssetHandler::LoadResult::LoadComplete : AZ::Data::AssetHandler::LoadResult::Error;
        }
        return AZ::Data::AssetHandler::LoadResult::Error;

    }


    bool ScriptCanvasAssetHandler::SaveAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, AZ::IO::GenericStream* stream)
    {
        return SaveAssetData(asset.GetAs<ScriptCanvasAsset>(), stream);
    }

    bool ScriptCanvasAssetHandler::SaveAssetData(const ScriptCanvasAsset* assetData, AZ::IO::GenericStream* stream)
    {
        return SaveAssetData(assetData, stream, AZ::DataStream::ST_XML);
    }

    bool ScriptCanvasAssetHandler::SaveAssetData(const ScriptCanvasAsset* assetData, AZ::IO::GenericStream* stream, AZ::DataStream::StreamType streamType)
    {
        if (assetData && m_serializeContext)
        {
            AZStd::vector<char> byteBuffer;
            AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);
            AZ::ObjectStream* objStream = AZ::ObjectStream::Create(&byteStream, *m_serializeContext, streamType);
            bool scriptCanvasAssetSaved = objStream->WriteClass(&assetData->GetScriptCanvasData());
            objStream->Finalize();
            scriptCanvasAssetSaved = stream->Write(byteBuffer.size(), byteBuffer.data()) == byteBuffer.size() && scriptCanvasAssetSaved;
            return scriptCanvasAssetSaved;
        }

        return false;
    }

    void ScriptCanvasAssetHandler::DestroyAsset(AZ::Data::AssetPtr ptr)
    {
        delete ptr;
    }

    //=========================================================================
    // GetSerializeContext
    //=========================================================================.
    AZ::SerializeContext* ScriptCanvasAssetHandler::GetSerializeContext() const
    {
        return m_serializeContext;
    }

    //=========================================================================
    // SetSerializeContext
    //=========================================================================.
    void ScriptCanvasAssetHandler::SetSerializeContext(AZ::SerializeContext* context)
    {
        m_serializeContext = context;

        if (m_serializeContext == nullptr)
        {
            // use the default app serialize context
            EBUS_EVENT_RESULT(m_serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
            if (!m_serializeContext)
            {
                AZ_Error("Script Canvas", false, "ScriptCanvasAssetHandler: No serialize context provided! We will not be able to process Graph Asset type");
            }
        }
    }

    //=========================================================================
    // GetHandledAssetTypes
    //=========================================================================.
    void ScriptCanvasAssetHandler::GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes)
    {
        assetTypes.push_back(GetAssetType());
    }

    //=========================================================================
    // GetAssetType
    //=========================================================================.
    AZ::Data::AssetType ScriptCanvasAssetHandler::GetAssetType() const
    {
        return ScriptCanvasAssetHandler::GetAssetTypeStatic();
    }

    const char* ScriptCanvasAssetHandler::GetAssetTypeDisplayName() const
    {
        return "Script Canvas";
    }

    AZ::Data::AssetType ScriptCanvasAssetHandler::GetAssetTypeStatic()
    {
        return azrtti_typeid<ScriptCanvasAsset>();
    }

    //=========================================================================
    // GetAssetTypeExtensions
    //=========================================================================.
    void ScriptCanvasAssetHandler::GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions)
    {
        ScriptCanvasAsset::Description description;
        extensions.push_back(description.GetExtensionImpl());
    }

    //=========================================================================
    // GetComponentTypeId
    //=========================================================================.
    AZ::Uuid ScriptCanvasAssetHandler::GetComponentTypeId() const
    {
        return azrtti_typeid<EditorScriptCanvasComponent>();
    }

    //=========================================================================
    // GetGroup
    //=========================================================================.
    const char* ScriptCanvasAssetHandler::GetGroup() const
    {
        return ScriptCanvas::AssetDescription::GetGroup<ScriptCanvasEditor::ScriptCanvasAsset>();
    }

    //=========================================================================
    // GetBrowserIcon
    //=========================================================================.
    const char* ScriptCanvasAssetHandler::GetBrowserIcon() const
    {
        return ScriptCanvas::AssetDescription::GetIconPath<ScriptCanvasEditor::ScriptCanvasAsset>();
    }


}
