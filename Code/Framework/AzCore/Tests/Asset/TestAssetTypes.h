/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Asset/AssetManager.h>


namespace UnitTest
{
    // EmptyAsset:  bare-bones asset definition, with no data contained within.
    class EmptyAsset
        : public AZ::Data::AssetData
    {
    public:
        AZ_CLASS_ALLOCATOR(EmptyAsset, AZ::SystemAllocator);
        AZ_RTTI(EmptyAsset, "{098E3F7F-13AC-414B-9B4E-49B5AD1BD7FE}", AZ::Data::AssetData);

        EmptyAsset(
            const AZ::Data::AssetId& assetId = AZ::Data::AssetId(),
            AZ::Data::AssetData::AssetStatus status = AZ::Data::AssetData::AssetStatus::NotLoaded)
            : AZ::Data::AssetData(assetId, status)
        {
        }
    };

    // EmptyAssetWithNoHandler:  no data contained within, and no AssetHandler registered for this type
    class EmptyAssetWithNoHandler
        : public AZ::Data::AssetData
    {
    public:
        AZ_RTTI(EmptyAssetWithNoHandler, "{81123022-8D45-4B5F-BBB6-3ED5DF2EFB7A}", AZ::Data::AssetData);
        AZ_CLASS_ALLOCATOR(EmptyAssetWithNoHandler, AZ::SystemAllocator);
    };

    // EmptyAssetWithInstanceCount:  asset with no stored data, keeps track of the global number of instances currently constructed.
    class EmptyAssetWithInstanceCount
        : public AZ::Data::AssetData
    {
    public:
        AZ_CLASS_ALLOCATOR(EmptyAssetWithInstanceCount, AZ::SystemAllocator);
        AZ_RTTI(EmptyAssetWithInstanceCount, "{C0A5DE6F-590F-4DF0-86D6-9498D4C762D8}", AZ::Data::AssetData);

        EmptyAssetWithInstanceCount() { ++s_instanceCount; }
        ~EmptyAssetWithInstanceCount() override { --s_instanceCount; }

        static void Reflect(AZ::SerializeContext& context)
        {
            context.Class<EmptyAssetWithInstanceCount>()
                ;
        }

        static inline int s_instanceCount = 0;
    };

    // AssetWithCustomData:  asset with field that needs custom code for load/save (i.e. isn't serialized through ObjectStream)
    class AssetWithCustomData
        : public AZ::Data::AssetData
    {
    public:
        AZ_CLASS_ALLOCATOR(AssetWithCustomData, AZ::SystemAllocator);
        AZ_RTTI(AssetWithCustomData, "{73D60606-BDE5-44F9-9420-5649FE7BA5B8}", AZ::Data::AssetData);

        AssetWithCustomData()
            : m_data(nullptr) {}
        explicit AssetWithCustomData(const AZ::Data::AssetId& assetId,
            const AZ::Data::AssetData::AssetStatus assetStatus = AZ::Data::AssetData::AssetStatus::NotLoaded)
            : AssetData(assetId, assetStatus)
            , m_data(nullptr) {}
        ~AssetWithCustomData() override
        {
            if (m_data)
            {
                azfree(m_data);
            }
        }

        static void Reflect(AZ::SerializeContext& context)
        {
            context.Class<AssetWithCustomData>()
                ->Field("data", &AssetWithCustomData::m_data)
                ;
        }

        char* m_data;
    };

    // AssetWithSerializedData:  asset with single field that is serialized in/out
    class AssetWithSerializedData
        : public AZ::Data::AssetData
    {
    public:
        AZ_RTTI(AssetWithSerializedData, "{BC15ABCC-0150-44C4-976B-79A91F8A8608}", AZ::Data::AssetData);
        AZ_CLASS_ALLOCATOR(AssetWithSerializedData, AZ::SystemAllocator);
        AssetWithSerializedData() = default;
        static void Reflect(AZ::SerializeContext& context)
        {
            context.Class<AssetWithSerializedData>()
                ->Field("data", &AssetWithSerializedData::m_data)
                ;
        }
        float m_data = 1.f;

    private:
        AssetWithSerializedData(const AssetWithSerializedData&) = delete;
    };

    // AssetWithAssetReference:  asset with single field containing a serialized asset reference
    class AssetWithAssetReference
        : public AZ::Data::AssetData
    {
    public:
        AZ_RTTI(AssetWithAssetReference, "{97383A2D-B84B-46D6-B3FA-FB8E49A4407F}", AZ::Data::AssetData);
        AZ_CLASS_ALLOCATOR(AssetWithAssetReference, AZ::SystemAllocator);
        AssetWithAssetReference() = default;
        AssetWithAssetReference(const AssetWithAssetReference&) = delete;
        static void Reflect(AZ::SerializeContext& context)
        {
            context.Class<AssetWithAssetReference>()
                ->Field("asset", &AssetWithAssetReference::m_asset);
        }

        AZ::Data::Asset<AZ::Data::AssetData> m_asset;
    };

    // AssetWithQueueAndPreLoadReferences:  asset with two asset references, one set as PreLoad, and one set as QueueLoad
    class AssetWithQueueAndPreLoadReferences
        : public AZ::Data::AssetData
    {
    public:
        AZ_RTTI(AssetWithQueueAndPreLoadReferences, "{B86F9FA2-7953-43F9-BBD2-31070452C841}", AZ::Data::AssetData);
        AZ_CLASS_ALLOCATOR(AssetWithQueueAndPreLoadReferences, AZ::SystemAllocator);
        AssetWithQueueAndPreLoadReferences() :
            m_preLoad(AZ::Data::AssetLoadBehavior::PreLoad),
            m_queueLoad(AZ::Data::AssetLoadBehavior::QueueLoad)
        {

        }
        static void Reflect(AZ::SerializeContext& context)
        {
            context.Class<AssetWithQueueAndPreLoadReferences>()
                ->Field("preLoad", &AssetWithQueueAndPreLoadReferences::m_preLoad)
                ->Field("queueLoad", &AssetWithQueueAndPreLoadReferences::m_queueLoad)
                ;
        }
        AZ::Data::Asset<AssetWithAssetReference> m_preLoad;
        AZ::Data::Asset<AssetWithAssetReference> m_queueLoad;
    private:
        AssetWithQueueAndPreLoadReferences(const AssetWithQueueAndPreLoadReferences&) = delete;
    };
}

