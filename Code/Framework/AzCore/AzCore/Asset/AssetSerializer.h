/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ {
    struct Uuid;

    namespace Data
    {
        template<typename T>
        class Asset;
        class AssetData;
        using AssetFilterCB = AZStd::function<bool(const AssetFilterInfo& filterInfo)>;
    }   // namespace Data

    /*
     * Returns the serialization UUID for Asset class
     */
    const Uuid& GetAssetClassId();

    /// Generic IDataSerializer specialization for Asset<T>
    /// This is used internally by the object stream because assets need
    /// special handling during serialization
    class AssetSerializer
        : public SerializeContext::IDataSerializer
    {
    public:
        // Store the class data into a stream.
        size_t Save(const void* classPtr, IO::GenericStream& stream, bool isDataBigEndian = false) override;

        // Convert binary data to text
        size_t DataToText(IO::GenericStream& in, IO::GenericStream& out, bool isDataBigEndian /*= false*/) override;

        // Convert text data to binary, to support loading old version formats. We must respect text version if the text->binary format has changed!
        size_t TextToData(const char* text, unsigned int textVersion, IO::GenericStream& stream, bool isDataBigEndian /*= false*/) override;

        // Load the class data from a stream.
        bool Load(void* classPtr, IO::GenericStream& stream, unsigned int version, bool isDataBigEndian /*= false*/) override;

        // Extended load function that enables asset filtering behavior.
        bool LoadWithFilter(void* classPtr, IO::GenericStream& stream, unsigned int version, const Data::AssetFilterCB& assetFilterCallback, bool isDataBigEndian = false);

        // Optimized clone operation for asset references that bypasses asset lookup if source is already populated.
        void Clone(const void* sourcePtr, void* destPtr);

        bool CompareValueData(const void* lhs, const void* rhs) override;

        // Even though Asset<T> is a template class, we don't actually care about its underlying asset type
        // during serialization, so all types will share the same instance of the serializer.
        static AssetSerializer  s_serializer;

    private:

        /// Called after we are done writing to the instance pointed by classPtr.
        bool PostSerializeAssetReference(AZ::Data::Asset<AZ::Data::AssetData>& asset, const Data::AssetFilterCB& assetFilterCallback);

        // Upgrade legacy Ids.
        void RemapLegacyIds(AZ::Data::Asset<AZ::Data::AssetData>& asset);
    };
    /*
     * Generic serialization descriptor for all Assets of all types.
     */
    template<typename T>
    struct SerializeGenericTypeInfo< Data::Asset<T> >
    {
        typedef typename Data::Asset<T> ThisType;

        class Factory
            : public SerializeContext::IObjectFactory
        {
        public:
            void* Create(const char* name) override
            {
                (void)name;
                AZ_Assert(false, "Asset<T> %s should be stored by value!", name);
                return nullptr;
            }

            void Destroy(void*) override
            {
                // do nothing
            }
        };

        class GenericClassGenericAsset
            : public GenericClassInfo
        {
        public:
            GenericClassGenericAsset()
                : m_classData{ SerializeContext::ClassData::Create<ThisType>("Asset", GetAssetClassId(), &m_factory, &AssetSerializer::s_serializer) }
            {
                m_classData.m_version = 2;
            }

            SerializeContext::ClassData* GetClassData() override
            {
                return &m_classData;
            }

            size_t GetNumTemplatedArguments() override
            {
                return 1;
            }

            const Uuid& GetTemplatedTypeId(size_t element) override
            {
                (void)element;
                return SerializeGenericTypeInfo<T>::GetClassTypeId();
            }

            const Uuid& GetSpecializedTypeId() const override
            {
                return GetAssetClassId();
            }

            const Uuid& GetGenericTypeId() const override
            {
                return GetAssetClassId();
            }

            void Reflect(SerializeContext* serializeContext) override
            {
                if (serializeContext)
                {
                    serializeContext->RegisterGenericClassInfo(GetSpecializedTypeId(), this, &AZ::AnyTypeInfoConcept<Data::Asset<Data::AssetData>>::CreateAny);
                    serializeContext->RegisterGenericClassInfo(azrtti_typeid<ThisType>(), this, &AZ::AnyTypeInfoConcept<ThisType>::CreateAny);
                }
            }

            Factory m_factory;
            SerializeContext::ClassData m_classData;
        };

        using ClassInfoType = GenericClassGenericAsset;

        static ClassInfoType* GetGenericInfo()
        {
            return GetCurrentSerializeContextModule().CreateGenericClassInfo<ThisType>();
        }

        static const Uuid& GetClassTypeId()
        {
            return GetGenericInfo()->m_classData.m_typeId;
        }
    };

    //! OnDemandReflection for any generic Data::Asset<T>
    template<typename T>
    struct OnDemandReflection<Data::Asset<T>>
    {
        using DataAssetType = Data::Asset<T>;
        static void Reflect(ReflectContext* context)
        {
            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<DataAssetType>()
                    ->Attribute(Script::Attributes::Scope, Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(Script::Attributes::Module, "asset")
                    ->Method("IsReady", &DataAssetType::IsReady)
                    ->Attribute(AZ::Script::Attributes::Alias, "is_ready")
                    ->Method("IsError", &DataAssetType::IsError)
                    ->Attribute(AZ::Script::Attributes::Alias, "is_error")
                    ->Method("IsLoading", &DataAssetType::IsLoading)
                    ->Attribute(AZ::Script::Attributes::Alias, "is_loading")
                    ->Method("GetStatus", &DataAssetType::GetStatus)
                    ->Attribute(AZ::Script::Attributes::Alias, "get_status")
                    ->Method("GetId", &DataAssetType::GetId)
                    ->Attribute(AZ::Script::Attributes::Alias, "get_id")
                    ->Method("GetType", &DataAssetType::GetType)
                    ->Attribute(AZ::Script::Attributes::Alias, "get_type")
                    ->Method("GetHint", &DataAssetType::GetHint)
                    ->Attribute(AZ::Script::Attributes::Alias, "get_hint")
                    ->Method("GetData", &DataAssetType::GetData)
                    ->Attribute(AZ::Script::Attributes::Alias, "get_data")
                    ;
            }
        }
    };

}   // namespace AZ
