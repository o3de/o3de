/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/Json/BaseJsonSerializer.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/JsonSerializationResult.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/Json/StackedString.h>
#include <Serializer/ParticleSourceDataSerializer.h>

#include <OpenParticleSystem/ParticleEditDataConfig.h>

namespace OpenParticle
{
    AZ_CLASS_ALLOCATOR_IMPL(ParticleSourceDataSerializer, AZ::SystemAllocator, 0);

    namespace JSR = AZ::JsonSerializationResult;

    AZ::JsonSerializationResult::Result ParticleSourceDataSerializer::Load(
        void* outputValue, const AZ::Uuid& outputValueTypeId, const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context)
    {
        AZ_Assert(
            azrtti_typeid<ParticleSourceData>() == outputValueTypeId,
            "Unable to deserialize particle to json because the provided type is %s", outputValueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(outputValueTypeId);
        ParticleSourceData* particleSourceData = reinterpret_cast<ParticleSourceData*>(outputValue);
        AZ_Assert(particleSourceData, "Output value for ParticleSourceDataSerializer can't be null.");

        if (!inputValue.IsObject())
        {
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported, "Particle data must be a JSON object");
        }

        JSR::ResultCode result(JSR::Tasks::ReadField);
        result.Combine(
            ContinueLoadingFromJsonObjectField(&particleSourceData->m_name, azrtti_typeid<AZStd::string>(), inputValue, "name", context));

        particleSourceData->m_config = context.GetSerializeContext()->CreateAny(azrtti_typeid<OpenParticle::SystemConfig>());
        result.Combine(ContinueLoadingFromJsonObjectField(
            AZStd::any_cast<void>(&particleSourceData->m_config), azrtti_typeid<OpenParticle::SystemConfig>(), inputValue, "systemConfig",
            context));

        particleSourceData->m_preWarm.clear();
        auto iterator = inputValue.FindMember(rapidjson::StringRef("preWarm"));
        if (iterator != inputValue.MemberEnd())
        {
            particleSourceData->m_preWarm = context.GetSerializeContext()->CreateAny(azrtti_typeid<OpenParticle::PreWarm>());
            result.Combine(ContinueLoadingFromJsonObjectField(
                AZStd::any_cast<void>(&particleSourceData->m_preWarm),
                azrtti_typeid<OpenParticle::PreWarm>(), inputValue, "preWarm", context));
        }

        iterator = inputValue.FindMember(rapidjson::StringRef("emitters"));
        if (iterator == inputValue.MemberEnd())
        {
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported, "fail to load particle.");
        }
        const rapidjson::Value& emitNode = iterator->value;
        if (emitNode.IsArray())
        {
            for (rapidjson_ly::SizeType i = 0; i < emitNode.Size(); i++)
            {
                ParticleSourceData::EmitterInfo* emitInfo = aznew ParticleSourceData::EmitterInfo();
                auto res = ContinueLoading(emitInfo, azrtti_typeid<ParticleSourceData::EmitterInfo*>(), emitNode[i], context);
                if (emitInfo != nullptr)
                {
                    particleSourceData->m_emitters.emplace_back(emitInfo);
                }
                result.Combine(res);
            }
        }

        iterator = inputValue.FindMember(rapidjson::StringRef("LODs"));
        if (iterator != inputValue.MemberEnd())
        {
            const rapidjson::Value& lodsNode = iterator->value;
            if (lodsNode.IsArray())
            {
                for (rapidjson_ly::SizeType i = 0; i < lodsNode.Size(); i++)
                {
                    ParticleSourceData::Lod lodInfo;
                    result.Combine(ContinueLoading(&lodInfo, azrtti_typeid<ParticleSourceData::Lod*>(), lodsNode[i], context));
                    particleSourceData->m_lods.emplace_back(lodInfo);
                }
            }
        }

        iterator = inputValue.FindMember(rapidjson::StringRef("distribution"));
        if (iterator == inputValue.MemberEnd())
        {
            return context.Report(result, "fail to load particle distribution.");
        }
        const rapidjson::Value& distributionNode = iterator->value;
        if (distributionNode.IsObject())
        {
            auto res = ContinueLoading(&particleSourceData->m_distribution, azrtti_typeid<Distribution*>(), distributionNode, context);
            result.Combine(res);
        }

        if (result.GetProcessing() == AZ::JsonSerializationResult::Processing::Completed)
        {
            return context.Report(result, "Successfully loaded particle.");
        }
        return context.Report(result, "Partially loaded particle.");
    }

    AZ::JsonSerializationResult::Result ParticleSourceDataSerializer::Store(
        [[maybe_unused]] rapidjson::Value& outputValue,
        const void* inputValue,
        [[maybe_unused]] const void* defaultValue,
        const AZ::Uuid& valueTypeId,
        AZ::JsonSerializerContext& context)
    {
        AZ_Assert(
            azrtti_typeid<ParticleSourceData>() == valueTypeId, "Unable to serialize particle to json because the provided type is %s",
            valueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(valueTypeId);

        const ParticleSourceData* particleSourceData = reinterpret_cast<const ParticleSourceData*>(inputValue);
        AZ_Assert(particleSourceData, "Input value for ParticleSourceDataSerializer can't be null.");

        JSR::ResultCode resultCode(JSR::Tasks::ReadField);
        resultCode.Combine(ContinueStoringToJsonObjectField(
            outputValue, "name", &particleSourceData->m_name, nullptr, azrtti_typeid<AZStd::string>(), context));
        resultCode.Combine(ContinueStoringToJsonObjectField(
            outputValue, "systemConfig", AZStd::any_cast<void>(&particleSourceData->m_config), nullptr,
            azrtti_typeid<OpenParticle::SystemConfig>(), context));
        if (!particleSourceData->m_preWarm.empty()) {
            resultCode.Combine(ContinueStoringToJsonObjectField(
                outputValue, "preWarm", AZStd::any_cast<void>(&particleSourceData->m_preWarm), nullptr,
                azrtti_typeid<OpenParticle::PreWarm>(), context));
        }

        rapidjson::Value emittersNode;
        emittersNode.SetArray();
        JSR::ResultCode emtRst(JSR::Tasks::ReadField);
        for (auto& emitter : particleSourceData->m_emitters)
        {
            rapidjson::Value emitterNode;
            auto rst = ContinueStoring(emitterNode, emitter, nullptr, azrtti_typeid<ParticleSourceData::EmitterInfo>(), context);
            if (rst.GetProcessing() != JSR::Processing::Halted &&
                (context.ShouldKeepDefaults() || rst.GetOutcome() != JSR::Outcomes::DefaultsUsed))
            {
                emittersNode.PushBack(emitterNode, context.GetJsonAllocator());
            }
            emtRst.Combine(rst);
        }
        outputValue.AddMember("emitters", emittersNode, context.GetJsonAllocator());

        rapidjson::Value lodsNode;
        lodsNode.SetArray();
        JSR::ResultCode lodsRst(JSR::Tasks::ReadField);
        for (auto& lod : particleSourceData->m_lods)
        {
            rapidjson::Value lodNode;
            auto rst = ContinueStoring(lodNode, &lod, nullptr, azrtti_typeid<ParticleSourceData::Lod*>(), context);
            if (rst.GetProcessing() != JSR::Processing::Halted &&
                (context.ShouldKeepDefaults() || rst.GetOutcome() != JSR::Outcomes::DefaultsUsed))
            {
                lodsNode.PushBack(lodNode, context.GetJsonAllocator());
            }
            lodsRst.Combine(rst);
        }
        outputValue.AddMember("LODs", lodsNode, context.GetJsonAllocator());

        rapidjson::Value distributionNode;
        distributionNode.SetObject();
        resultCode.Combine(ContinueStoring(distributionNode, &particleSourceData->m_distribution,
            nullptr, azrtti_typeid<Distribution*>(), context));

        outputValue.AddMember("distribution", distributionNode, context.GetJsonAllocator());

        return context.Report(resultCode, "Processed particle.");
    }

    AZ::BaseJsonSerializer::OperationFlags ParticleSourceDataSerializer::GetOperationsFlags() const
    {
        return OperationFlags::ManualDefault;
    }
} // namespace OpenParticle
