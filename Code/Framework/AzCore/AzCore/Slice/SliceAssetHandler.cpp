/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Slice/SliceAssetHandler.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>

namespace AZ
{
    //=========================================================================
    // SliceAssetHandler
    //=========================================================================
    SliceAssetHandler::SliceAssetHandler(SerializeContext* context)
        : m_filterFlags(0)
    {
        SetSerializeContext(context);

        AZ::AssetTypeInfoBus::MultiHandler::BusConnect(AZ::AzTypeInfo<AZ::SliceAsset>::Uuid());
        AZ::AssetTypeInfoBus::MultiHandler::BusConnect(AZ::AzTypeInfo<AZ::DynamicSliceAsset>::Uuid());
    }

    SliceAssetHandler::~SliceAssetHandler()
    {
        AZ::AssetTypeInfoBus::MultiHandler::BusDisconnect();
    }

    //=========================================================================
    // CreateAsset
    //=========================================================================
    Data::AssetPtr SliceAssetHandler::CreateAsset(const Data::AssetId& id, const Data::AssetType& type)
    {
        AZ_Assert(type == AzTypeInfo<SliceAsset>::Uuid() || type == AzTypeInfo<DynamicSliceAsset>::Uuid(), "This handler deals only with SliceAsset type!");
        
        Data::AssetPtr newPtr = nullptr;
        if (type == AzTypeInfo<DynamicSliceAsset>::Uuid())
        {
            newPtr = aznew DynamicSliceAsset(id);
        }
        else if (type == AzTypeInfo<SliceAsset>::Uuid())
        {
            newPtr = aznew SliceAsset(id);
        }

        return newPtr;
    }

    //=========================================================================
    // LoadAssetData
    //=========================================================================
    Data::AssetHandler::LoadResult SliceAssetHandler::LoadAssetData(
        const Data::Asset<Data::AssetData>& asset,
        AZStd::shared_ptr<Data::AssetDataStream> stream,
        const AZ::Data::AssetFilterCB& assetLoadFilterCB)
    {
        SliceAsset* sliceAsset = asset.GetAs<SliceAsset>();
        AZ_Assert(sliceAsset, "This should be a slice asset, as this is the only type we process!");
        if (sliceAsset && m_serializeContext)
        {
            ObjectStream::FilterDescriptor filter(assetLoadFilterCB);
            filter.m_flags = m_filterFlags;

            Entity* sliceEntity = Utils::LoadObjectFromStream<Entity>(*stream, m_serializeContext, filter);
            if (sliceEntity)
            {
                sliceAsset->m_component = sliceEntity->FindComponent<SliceComponent>();
                if (sliceAsset->m_component)
                {
                    // Apply the same filter to any internal object streams generated during instantiation/data-patching.
                    sliceAsset->m_component->m_assetLoadFilterCB = assetLoadFilterCB;
                    sliceAsset->m_component->m_filterFlags = m_filterFlags;

                    sliceAsset->m_component->SetMyAsset(sliceAsset);
                    sliceAsset->m_entity = sliceEntity;
                    sliceAsset->m_component->ListenForAssetChanges();
                    return Data::AssetHandler::LoadResult::LoadComplete;
                }
                else
                {
                    AZ_Error("Slices", false, "Slice entity 0x%08x (%s) doesn't have SliceComponent!", sliceEntity->GetId(), sliceEntity->GetName().c_str());
                    delete sliceEntity;
                }
            }
        }
        return Data::AssetHandler::LoadResult::Error;
    }

    //=========================================================================
    // SaveAssetData
    //=========================================================================
    bool SliceAssetHandler::SaveAssetData(const Data::Asset<Data::AssetData>& asset, IO::GenericStream* stream)
    {
        SliceAsset* sliceAsset = asset.GetAs<SliceAsset>();
        AZ_Assert(sliceAsset, "This should be a slice asset, as this is the only type we process!");
        if (sliceAsset && sliceAsset->m_entity && m_serializeContext)
        {
            ObjectStream* binaryObjStream = ObjectStream::Create(stream, *m_serializeContext, /*ObjectStream::ST_BINARY*/ ObjectStream::ST_XML);
            bool result = binaryObjStream->WriteClass(sliceAsset->m_entity);
            binaryObjStream->Finalize();
            return result;
        }

        return false;
    }

    //=========================================================================
    // DestroyAsset
    //=========================================================================
    void SliceAssetHandler::DestroyAsset(Data::AssetPtr ptr)
    {
        delete ptr;
    }

    //=========================================================================
    // GetHandledAssetTypes
    //=========================================================================.
    void SliceAssetHandler::GetHandledAssetTypes(AZStd::vector<Data::AssetType>& assetTypes)
    {
        assetTypes.push_back(AzTypeInfo<SliceAsset>::Uuid());
        assetTypes.push_back(AzTypeInfo<DynamicSliceAsset>::Uuid());
    }

    //=========================================================================
    // GetSerializeContext
    //=========================================================================.
    SerializeContext* SliceAssetHandler::GetSerializeContext() const
    {
        return m_serializeContext;
    }

    //=========================================================================
    // SetSerializeContext
    //=========================================================================.
    void SliceAssetHandler::SetSerializeContext(SerializeContext* context)
    {
        m_serializeContext = context;

        if (m_serializeContext == nullptr)
        {
            // use the default app serialize context
            ComponentApplicationBus::BroadcastResult(m_serializeContext, &ComponentApplicationBus::Events::GetSerializeContext);
            if (!m_serializeContext)
            {
                AZ_Error("Slices", false, "SliceAssetHandler: No serialize context provided! We will not be able to process Slice Asset type");
            }
        }
    }

    void SliceAssetHandler::SetFilterFlags(u32 flags)
    {
        m_filterFlags = flags;
    }

    AZ::Data::AssetType SliceAssetHandler::GetAssetType() const
    {
        return AzTypeInfo<SliceAsset>::Uuid();
    }

    //=========================================================================
    // GetAssetTypeDisplayName
    //=========================================================================.
    const char* SliceAssetHandler::GetAssetTypeDisplayName() const
    {
        const AZ::Uuid& assetType = *AZ::AssetTypeInfoBus::GetCurrentBusId();
        if (assetType == AZ::AzTypeInfo<AZ::SliceAsset>::Uuid())
        {
            return "Slice";
        }
        else if (assetType == AZ::AzTypeInfo<AZ::DynamicSliceAsset>::Uuid())
        {
            return "Dynamic slice";
        }

        return "";
    }

    const char* SliceAssetHandler::GetGroup() const
    {
        return "Slice";
    }

    //=========================================================================
    // GetAssetTypeExtensions
    //=========================================================================.
    void SliceAssetHandler::GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions)
    {
        const AZ::Uuid& assetType = *AZ::AssetTypeInfoBus::GetCurrentBusId();
        if (assetType == AZ::AzTypeInfo<AZ::SliceAsset>::Uuid())
        {
            extensions.push_back("slice");
        }
        else if (assetType == AZ::AzTypeInfo<AZ::DynamicSliceAsset>::Uuid())
        {
            extensions.push_back("dynamicslice");
        }
    }
} // namespace AZ
