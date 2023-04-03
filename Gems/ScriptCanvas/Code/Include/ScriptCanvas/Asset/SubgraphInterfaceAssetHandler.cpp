/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>

#include <ScriptCanvas/Asset/SubgraphInterfaceAsset.h>
#include <ScriptCanvas/Asset/SubgraphInterfaceAssetHandler.h>
#include <ScriptCanvas/Execution/RuntimeComponent.h>

namespace ScriptCanvas
{
    //=========================================================================
    // SubgraphInterfaceAssetHandler
    //=========================================================================
    SubgraphInterfaceAssetHandler::SubgraphInterfaceAssetHandler(AZ::SerializeContext* context)
    {
        SetSerializeContext(context);

        AZ::AssetTypeInfoBus::MultiHandler::BusConnect(AZ::AzTypeInfo<ScriptCanvas::SubgraphInterfaceAsset>::Uuid());
    }

    SubgraphInterfaceAssetHandler::~SubgraphInterfaceAssetHandler()
    {
        AZ::AssetTypeInfoBus::MultiHandler::BusDisconnect();
    }

    AZ::Data::AssetType SubgraphInterfaceAssetHandler::GetAssetType() const
    {
        return AZ::AzTypeInfo<SubgraphInterfaceAsset>::Uuid();
    }

    const char* SubgraphInterfaceAssetHandler::GetAssetTypeDisplayName() const
    {
        return "Script Canvas Runtime Function Graph";
    }

    const char* SubgraphInterfaceAssetHandler::GetGroup() const
    {
        return "Script Canvas";
    }

    const char* SubgraphInterfaceAssetHandler::GetBrowserIcon() const
    {
        return "Icons/ScriptCanvas/Viewport/ScriptCanvas_Function.png";
    }

    AZ::Uuid SubgraphInterfaceAssetHandler::GetComponentTypeId() const
    {
        return azrtti_typeid<RuntimeComponent>();
    }

    void SubgraphInterfaceAssetHandler::GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions)
    {
        const AZ::Uuid& assetType = *AZ::AssetTypeInfoBus::GetCurrentBusId();
        if (assetType == AZ::AzTypeInfo<ScriptCanvas::SubgraphInterfaceAsset>::Uuid())
        {
            extensions.push_back(ScriptCanvas::SubgraphInterfaceAsset::GetFileExtension());
        }
    }

    bool SubgraphInterfaceAssetHandler::CanCreateComponent(const AZ::Data::AssetId&) const
    {
        // This is a runtime component so we shouldn't be making components at edit time for this
        return false;
    }

    AZ::Data::AssetPtr SubgraphInterfaceAssetHandler::CreateAsset(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type)
    {
        (void)type;
        AZ_Assert(type == AZ::AzTypeInfo<ScriptCanvas::SubgraphInterfaceAsset>::Uuid(), "This handler deals only with the Script Canvas Runtime Asset type!");

        return aznew ScriptCanvas::SubgraphInterfaceAsset(id);
    }

    AZ::Data::AssetHandler::LoadResult SubgraphInterfaceAssetHandler::LoadAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, AZStd::shared_ptr<AZ::Data::AssetDataStream> stream, const AZ::Data::AssetFilterCB& assetLoadFilterCB)
    {
        ScriptCanvas::SubgraphInterfaceAsset* runtimeFunctionAsset = asset.GetAs<ScriptCanvas::SubgraphInterfaceAsset>();
        AZ_Assert(runtimeFunctionAsset, "This should be a Script Canvas runtime asset, as this is the only type we process!");
        if (runtimeFunctionAsset && m_serializeContext)
        {
            stream->Seek(0U, AZ::IO::GenericStream::ST_SEEK_BEGIN);
            bool loadSuccess = AZ::Utils::LoadObjectFromStreamInPlace(*stream, runtimeFunctionAsset->m_interfaceData, m_serializeContext, AZ::ObjectStream::FilterDescriptor(assetLoadFilterCB));
            return loadSuccess ? AZ::Data::AssetHandler::LoadResult::LoadComplete : AZ::Data::AssetHandler::LoadResult::Error;
        }
        return AZ::Data::AssetHandler::LoadResult::Error;
    }

    bool SubgraphInterfaceAssetHandler::SaveAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, AZ::IO::GenericStream* stream)
    {
        ScriptCanvas::SubgraphInterfaceAsset* runtimeFunctionAsset = asset.GetAs<ScriptCanvas::SubgraphInterfaceAsset>();
        AZ_Assert(runtimeFunctionAsset, "This should be a Script Canvas runtime asset, as this is the only type we process!");
        if (runtimeFunctionAsset && m_serializeContext)
        {
            AZ::ObjectStream* binaryObjStream = AZ::ObjectStream::Create(stream, *m_serializeContext
                , ScriptCanvas::g_saveEditorAssetsAsPlainTextForDebug
                ? AZ::ObjectStream::ST_JSON
                : AZ::ObjectStream::ST_BINARY);
            bool graphSaved = binaryObjStream->WriteClass(&runtimeFunctionAsset->m_interfaceData);
            binaryObjStream->Finalize();
            return graphSaved;
        }

        return false;
    }

    void SubgraphInterfaceAssetHandler::DestroyAsset(AZ::Data::AssetPtr ptr)
    {
        delete ptr;
    }

    void SubgraphInterfaceAssetHandler::GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes)
    {
        assetTypes.push_back(AZ::AzTypeInfo<ScriptCanvas::SubgraphInterfaceAsset>::Uuid());
    }

    AZ::SerializeContext* SubgraphInterfaceAssetHandler::GetSerializeContext() const
    {
        return m_serializeContext;
    }
    
    void SubgraphInterfaceAssetHandler::SetSerializeContext(AZ::SerializeContext* context)
    {
        m_serializeContext = context;

        if (m_serializeContext == nullptr)
        {
            // use the default app serialize context
            AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            if (!m_serializeContext)
            {
                AZ_Error("Script Canvas", false, "SubgraphInterfaceAssetHandler: No serialize context provided! We will not be able to process the Script Canvas Runtime Asset type");
            }
        }
    }
}
