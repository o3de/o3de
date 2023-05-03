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

        class DataConverter
            : public SerializeContext::IDataConverter
        {
        public:
            bool CanConvertFromType(
                const TypeId& convertibleTypeId,
                const SerializeContext::ClassData& classData,
                SerializeContext& /*serializeContext*/) override
            {
                return classData.m_typeId == convertibleTypeId ||
                    (convertibleTypeId == GetAssetClassId() && classData.m_typeId == azrtti_typeid<ThisType>()) ||
                    (convertibleTypeId == azrtti_typeid<Data::Asset<Data::AssetData>>() && classData.m_typeId == azrtti_typeid<ThisType>());
            }

            bool ConvertFromType(
                void*& convertibleTypePtr,
                const TypeId& convertibleTypeId,
                void* classPtr,
                const SerializeContext::ClassData& classData,
                SerializeContext& serializeContext) override
            {
                if (CanConvertFromType(convertibleTypeId, classData, serializeContext))
                {
                    convertibleTypePtr = classPtr;
                    return true;
                }

                return false;
            }
        };

        class GenericClassGenericAsset
            : public GenericClassInfo
        {
        public:
            GenericClassGenericAsset()
                : m_classData{ SerializeContext::ClassData::Create<ThisType>(AzTypeInfo<ThisType>::Name(), GetSpecializedTypeId(), &m_factory, &AssetSerializer::s_serializer) }
            {
                m_classData.m_version = 3;
                m_classData.m_dataConverter = &m_dataConverter;
            }
            // The default implementation of the special member functions would
            // not update the `m_classData.m_dataConverter` pointer, so delete
            // them.
            GenericClassGenericAsset(const GenericClassGenericAsset& rhs) = delete;
            GenericClassGenericAsset(GenericClassGenericAsset&& rhs) = delete;
            GenericClassGenericAsset& operator=(const GenericClassGenericAsset& rhs) = delete;
            GenericClassGenericAsset& operator=(GenericClassGenericAsset&& rhs) = delete;
            ~GenericClassGenericAsset() override = default;

            SerializeContext::ClassData* GetClassData() override
            {
                return &m_classData;
            }

            size_t GetNumTemplatedArguments() override
            {
                return 1;
            }

            AZ::TypeId GetTemplatedTypeId(size_t element) override
            {
                (void)element;
                return SerializeGenericTypeInfo<T>::GetClassTypeId();
            }

            AZ::TypeId GetSpecializedTypeId() const override
            {
                return azrtti_typeid<ThisType>();
            }

            AZ::TypeId GetGenericTypeId() const override
            {
                return GetAssetClassId();
            }

            void Reflect(SerializeContext* serializeContext) override
            {
                if (serializeContext)
                {
                    serializeContext->RegisterGenericClassInfo(GetSpecializedTypeId(), this, &AZ::AnyTypeInfoConcept<ThisType>::CreateAny);
                    if constexpr (AZStd::is_same_v<ThisType, Data::Asset<Data::AssetData>>)
                    {
                        serializeContext->RegisterGenericClassInfo(GetAssetClassId(), this, &AZ::AnyTypeInfoConcept<ThisType>::CreateAny);
                    }
                    else
                    {
                        if (!serializeContext->FindGenericClassInfo(GetAssetClassId()))
                        {
                            SerializeGenericTypeInfo<Data::Asset<Data::AssetData>>::GetGenericInfo()->Reflect(serializeContext);
                        }
                    }
                }
            }

            Factory m_factory;
            SerializeContext::ClassData m_classData;
        private:
            DataConverter m_dataConverter{};
        };

        using ClassInfoType = GenericClassGenericAsset;

        static ClassInfoType* GetGenericInfo()
        {
            return GetCurrentSerializeContextModule().CreateGenericClassInfo<ThisType>();
        }

        static AZ::TypeId GetClassTypeId()
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
