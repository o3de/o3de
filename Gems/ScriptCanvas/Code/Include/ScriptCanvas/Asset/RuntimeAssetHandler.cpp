/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>

#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Asset/RuntimeAssetHandler.h>
#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Core/ScriptCanvasBus.h>
#include <ScriptCanvas/Execution/Interpreted/ExecutionInterpretedAPI.h>
#include <ScriptCanvas/Execution/RuntimeComponent.h>

namespace ScriptCanvas
{
    //=========================================================================
    // RuntimeAssetHandler
    //=========================================================================
    RuntimeAssetHandler::RuntimeAssetHandler(AZ::SerializeContext* context)
    {
        SetSerializeContext(context);

        AZ::AssetTypeInfoBus::MultiHandler::BusConnect(AZ::AzTypeInfo<RuntimeAsset>::Uuid());
    }

    RuntimeAssetHandler::~RuntimeAssetHandler()
    {
        AZ::AssetTypeInfoBus::MultiHandler::BusDisconnect();
    }

    AZ::Data::AssetType RuntimeAssetHandler::GetAssetType() const
    {
        return AZ::AzTypeInfo<RuntimeAsset>::Uuid();
    }

    const char* RuntimeAssetHandler::GetAssetTypeDisplayName() const
    {
        return "Script Canvas Runtime Graph";
    }

    const char* RuntimeAssetHandler::GetGroup() const
    {
        return "Script Canvas";
    }

    const char* RuntimeAssetHandler::GetBrowserIcon() const
    {
        return "Icons/ScriptCanvas/Viewport/ScriptCanvas.png";
    }

    AZ::Uuid RuntimeAssetHandler::GetComponentTypeId() const
    {
        return azrtti_typeid<RuntimeComponent>();
    }

    void RuntimeAssetHandler::GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions)
    {
        const AZ::Uuid& assetType = *AZ::AssetTypeInfoBus::GetCurrentBusId();
        if (assetType == AZ::AzTypeInfo<RuntimeAsset>::Uuid())
        {
            extensions.push_back(RuntimeAsset::GetFileExtension());
        }
    }

    bool RuntimeAssetHandler::CanCreateComponent(const AZ::Data::AssetId&) const
    {
        // This is a runtime component so we shouldn't be making components at edit time for this
        return false;
    }

    AZ::Data::AssetPtr RuntimeAssetHandler::CreateAsset(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type)
    {
        (void)type;
        AZ_Assert(type == AZ::AzTypeInfo<RuntimeAsset>::Uuid(), "This handler deals only with the Script Canvas Runtime Asset type!");

        return aznew RuntimeAsset(id);
    }

    void RuntimeAssetHandler::InitAsset(const AZ::Data::Asset<AZ::Data::AssetData>& asset, bool loadStageSucceeded, bool isReload)
    {
        AssetHandler::InitAsset(asset, loadStageSucceeded, isReload);

        if (loadStageSucceeded && !isReload)
        {
            RuntimeAsset* runtimeAsset = asset.GetAs<RuntimeAsset>();
            AZ_Assert(runtimeAsset, "RuntimeAssetHandler::InitAsset This should be a Script Canvas runtime asset, as this is the only type this handler processes!");
            Execution::Context::InitializeActivationData(runtimeAsset->GetData());
        }
    }

    AZ::Data::AssetHandler::LoadResult RuntimeAssetHandler::LoadAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, AZStd::shared_ptr<AZ::Data::AssetDataStream> stream, const AZ::Data::AssetFilterCB& assetLoadFilterCB)
    {
        RuntimeAsset* runtimeAsset = asset.GetAs<RuntimeAsset>();
        AZ_Assert(runtimeAsset, "This should be a Script Canvas runtime asset, as this is the only type we process!");
        if (runtimeAsset && m_serializeContext)
        {
            stream->Seek(0U, AZ::IO::GenericStream::ST_SEEK_BEGIN);
            bool loadSuccess = AZ::Utils::LoadObjectFromStreamInPlace(*stream, runtimeAsset->m_runtimeData, m_serializeContext, AZ::ObjectStream::FilterDescriptor(assetLoadFilterCB));
            return loadSuccess ? AZ::Data::AssetHandler::LoadResult::LoadComplete : AZ::Data::AssetHandler::LoadResult::Error;
        }
        return AZ::Data::AssetHandler::LoadResult::Error;
    }

    bool RuntimeAssetHandler::SaveAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, AZ::IO::GenericStream* stream)
    {
        RuntimeAsset* runtimeAsset = asset.GetAs<RuntimeAsset>();
        AZ_Assert(runtimeAsset, "This should be a Script Canvas runtime asset, as this is the only type we process!");
        if (runtimeAsset && m_serializeContext)
        {
            AZ::ObjectStream* binaryObjStream = AZ::ObjectStream::Create(stream, *m_serializeContext, AZ::ObjectStream::ST_XML);
            bool graphSaved = binaryObjStream->WriteClass(&runtimeAsset->m_runtimeData);
            binaryObjStream->Finalize();
            return graphSaved;
        }

        return false;
    }

    void RuntimeAssetHandler::DestroyAsset(AZ::Data::AssetPtr ptr)
    {
        RuntimeAsset* runtimeAsset = azrtti_cast<RuntimeAsset*>(ptr);
        Execution::Context::UnloadData(runtimeAsset->GetData());
        delete ptr;
    }

    void RuntimeAssetHandler::GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes)
    {
        assetTypes.push_back(AZ::AzTypeInfo<RuntimeAsset>::Uuid());
    }

    AZ::SerializeContext* RuntimeAssetHandler::GetSerializeContext() const
    {
        return m_serializeContext;
    }
    
    void RuntimeAssetHandler::SetSerializeContext(AZ::SerializeContext* context)
    {
        m_serializeContext = context;

        if (m_serializeContext == nullptr)
        {
            // use the default app serialize context
            AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            if (!m_serializeContext)
            {
                AZ_Error("Script Canvas", false, "RuntimeAssetHandler: No serialize context provided! We will not be able to process the Script Canvas Runtime Asset type");
            }
        }
    }

}
