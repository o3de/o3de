/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzToolsFramework/Prefab/PrefabDomTypes.h>
#include <AzToolsFramework/Prefab/PrefabIdTypes.h>
#include <AzCore/Serialization/Json/BaseJsonSerializer.h>

namespace AZ::Prefab
{
    //! A wrapper around the JSON DOM type so that the assets can read in and write out
    //! JSON directly since Prefabs are JSON serialized entity-component data
    class PrefabDomData final
    {
    public:
        AZ_RTTI(PrefabDomData, "{C73A3360-D772-4D41-9118-A039BF9340C1}");
        AZ_CLASS_ALLOCATOR(PrefabDomData, AZ::SystemAllocator);

        PrefabDomData() = default;
        ~PrefabDomData() = default;

        static void Reflect(AZ::ReflectContext* context);

        void CopyValue(const rapidjson::Value& inputValue);
        const AzToolsFramework::Prefab::PrefabDom& GetValue() const;

    private:
        AzToolsFramework::Prefab::PrefabDom m_prefabDom;
    };

    //! Registered to help read/write JSON for the PrefabDomData::m_prefabDom
    class PrefabDomDataJsonSerializer final
        : public AZ::BaseJsonSerializer
    {
    public:
        AZ_RTTI(PrefabDomDataJsonSerializer, "{9FC48652-A00B-4EFA-8FD9-345A8E625439}", BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR(PrefabDomDataJsonSerializer, AZ::SystemAllocator);

        ~PrefabDomDataJsonSerializer() override = default;

        AZ::JsonSerializationResult::Result Load(
            void* outputValue,
            const AZ::Uuid& outputValueTypeId,
            const rapidjson::Value& inputValue,
            AZ::JsonDeserializerContext& context) override;

        AZ::JsonSerializationResult::Result Store(
            rapidjson::Value& outputValue,
            const void* inputValue,
            const void* defaultValue,
            const AZ::Uuid& valueTypeId,
            AZ::JsonSerializerContext& context) override;
    };

    //! An asset type to register templates into the Prefab system so that they
    //! can instantiate like Authored Prefabs
    class ProceduralPrefabAsset
        : public AZ::Data::AssetData
    {
    public:
        AZ_CLASS_ALLOCATOR(ProceduralPrefabAsset, AZ::SystemAllocator);
        AZ_RTTI(ProceduralPrefabAsset, "{9B7C8459-471E-4EAD-A363-7990CC4065A9}", AZ::Data::AssetData);

        static bool UseProceduralPrefabs();

        ProceduralPrefabAsset(const AZ::Data::AssetId& assetId = AZ::Data::AssetId());
        ~ProceduralPrefabAsset() override = default;
        ProceduralPrefabAsset(const ProceduralPrefabAsset& rhs) = delete;
        ProceduralPrefabAsset& operator=(const ProceduralPrefabAsset& rhs) = delete;

        const AZStd::string& GetTemplateName() const;
        void SetTemplateName(AZStd::string templateName);

        AzToolsFramework::Prefab::TemplateId GetTemplateId() const;
        void SetTemplateId(AzToolsFramework::Prefab::TemplateId templateId);

        static void Reflect(AZ::ReflectContext* context);

    private:
        AZStd::string m_templateName;
        AzToolsFramework::Prefab::TemplateId m_templateId;
    };

}
