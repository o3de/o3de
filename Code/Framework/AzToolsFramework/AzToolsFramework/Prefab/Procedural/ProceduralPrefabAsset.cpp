/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Prefab/Procedural/ProceduralPrefabAsset.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzFramework/FileFunc/FileFunc.h>

namespace AZ::Prefab
{
    static constexpr const char s_useProceduralPrefabsKey[] = "/O3DE/Preferences/Prefabs/UseProceduralPrefabs";

    // ProceduralPrefabAsset

    ProceduralPrefabAsset::ProceduralPrefabAsset(const AZ::Data::AssetId& assetId)
        : AZ::Data::AssetData(assetId)
        , m_templateId(AzToolsFramework::Prefab::InvalidTemplateId)
    {
    }

    void ProceduralPrefabAsset::Reflect(AZ::ReflectContext* context)
    {
        PrefabDomData::Reflect(context);

        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context); serializeContext != nullptr)
        {
            serializeContext->Class<ProceduralPrefabAsset, AZ::Data::AssetData>()
                ->Version(1)
                ->Field("Template Name", &ProceduralPrefabAsset::m_templateName)
                ->Field("Template ID", &ProceduralPrefabAsset::m_templateId);
        }
    }

    const AZStd::string& ProceduralPrefabAsset::GetTemplateName() const
    {
        return m_templateName;
    }

    void ProceduralPrefabAsset::SetTemplateName(AZStd::string templateName)
    {
        m_templateName = AZStd::move(templateName);
    }

    AzToolsFramework::Prefab::TemplateId ProceduralPrefabAsset::GetTemplateId() const
    {
        return m_templateId;
    }

    void ProceduralPrefabAsset::SetTemplateId(AzToolsFramework::Prefab::TemplateId templateId)
    {
        m_templateId = templateId;
    }

    bool ProceduralPrefabAsset::UseProceduralPrefabs()
    {
        bool useProceduralPrefabs = false;
        bool result = AZ::SettingsRegistry::Get()->GetObject(useProceduralPrefabs, s_useProceduralPrefabsKey);
        return result && useProceduralPrefabs;
    }

    // PrefabDomData

    void PrefabDomData::Reflect(AZ::ReflectContext* context)
    {
        if (auto* jsonContext = azrtti_cast<AZ::JsonRegistrationContext*>(context))
        {
            jsonContext->Serializer<PrefabDomDataJsonSerializer>()->HandlesType<PrefabDomData>();
        }

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<PrefabDomData>()
                ->Version(1);
        }
    }

    void PrefabDomData::CopyValue(const rapidjson::Value& inputValue)
    {
        m_prefabDom.CopyFrom(inputValue, m_prefabDom.GetAllocator());
    }

    const AzToolsFramework::Prefab::PrefabDom& PrefabDomData::GetValue() const
    {
        return m_prefabDom;
    }

    // PrefabDomDataJsonSerializer

    AZ::JsonSerializationResult::Result PrefabDomDataJsonSerializer::Load(
        void* outputValue,
        [[maybe_unused]] const AZ::Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue,
        AZ::JsonDeserializerContext& context)
    {
        AZ_Assert(outputValueTypeId == azrtti_typeid<PrefabDomData>(),
            "PrefabDomDataJsonSerializer Load against output typeID that was not PrefabDomData");
        AZ_Assert(outputValue, "PrefabDomDataJsonSerializer Load against null output");

        namespace JSR = AZ::JsonSerializationResult;
        JSR::ResultCode result(JSR::Tasks::ReadField);

        if (inputValue.IsObject() == false)
        {
            result.Combine(context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Missing, "Missing object"));
            return context.Report(result, "Prefab should be an object.");
        }

        if (inputValue.MemberCount() < 1)
        {
            result.Combine(context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Missing, "Missing members"));
            return context.Report(result, "Prefab should have multiple members.");
        }

        auto* outputVariable = reinterpret_cast<PrefabDomData*>(outputValue);
        outputVariable->CopyValue(inputValue);
        return context.Report(result, "Loaded procedural prefab");
    }

    AZ::JsonSerializationResult::Result PrefabDomDataJsonSerializer::Store(
        rapidjson::Value& outputValue,
        const void* inputValue,
        [[maybe_unused]] const void* defaultValue,
        [[maybe_unused]] const AZ::Uuid& valueTypeId,
        AZ::JsonSerializerContext& context)
    {
        AZ_Assert(inputValue, "Input value for PrefabDomDataJsonSerializer can't be null.");
        AZ_Assert(azrtti_typeid<PrefabDomData>() == valueTypeId,
            "Unable to Serialize because the provided type is not PrefabGroup::PrefabDomData.");

        const PrefabDomData* prefabDomData = reinterpret_cast<const PrefabDomData*>(inputValue);

        namespace JSR = AZ::JsonSerializationResult;
        JSR::ResultCode result(JSR::Tasks::WriteValue);
        outputValue.SetObject();
        outputValue.CopyFrom(prefabDomData->GetValue(), context.GetJsonAllocator());
        return context.Report(result, "Stored procedural prefab");
    }
}
