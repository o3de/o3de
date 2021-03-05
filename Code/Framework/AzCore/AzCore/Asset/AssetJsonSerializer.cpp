/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetJsonSerializer.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/StackedString.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    namespace Data
    {
        AZ_CLASS_ALLOCATOR_IMPL(AssetJsonSerializer, SystemAllocator, 0);

        JsonSerializationResult::Result AssetJsonSerializer::Load(void* outputValue, const Uuid& /*outputValueTypeId*/,
            const rapidjson::Value& inputValue, JsonDeserializerContext& context)
        {
            namespace JSR = JsonSerializationResult;

            switch (inputValue.GetType())
            {
            case rapidjson::kObjectType:
                return LoadAsset(outputValue, inputValue, context);
            case rapidjson::kArrayType: // fall through
            case rapidjson::kNullType: // fall through
            case rapidjson::kStringType: // fall through
            case rapidjson::kFalseType: // fall through
            case rapidjson::kTrueType: // fall through
            case rapidjson::kNumberType:
                return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported,
                    "Unsupported type. Asset<T> can only be read from an object.");

            default:
                return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unknown, "Unknown json type encountered for Asset<T>.");
            }
        }

        JsonSerializationResult::Result AssetJsonSerializer::Store(rapidjson::Value& outputValue, const void* inputValue,
            const void* defaultValue, const Uuid& /*valueTypeId*/, JsonSerializerContext& context)
        {
            namespace JSR = JsonSerializationResult;

            const Asset<AssetData>* instance = reinterpret_cast<const Asset<AssetData>*>(inputValue);
            const Asset<AssetData>* defaultInstance = reinterpret_cast<const Asset<AssetData>*>(defaultValue);

            JSR::ResultCode result(JSR::Tasks::WriteValue);
            {
                ScopedContextPath subPathId(context, "m_assetId");
                const auto* id = &instance->GetId();
                const auto* defaultId = defaultInstance ? &defaultInstance->GetId() : nullptr;
                rapidjson::Value assetIdValue;
                result = ContinueStoring(assetIdValue, id, defaultId, azrtti_typeid<AssetId>(), context);
                if (result.GetOutcome() == JSR::Outcomes::Success || result.GetOutcome() == JSR::Outcomes::PartialDefaults)
                {
                    if (!outputValue.IsObject())
                    {
                        outputValue.SetObject();
                    }
                    outputValue.AddMember(rapidjson::StringRef("assetId"), AZStd::move(assetIdValue), context.GetJsonAllocator());
                }
            }

            {
                ScopedContextPath subPathHint(context, "m_assetHint");
                const AZStd::string* hint = &instance->GetHint();
                const AZStd::string defaultHint;
                rapidjson::Value assetHintValue;
                JSR::ResultCode resultHint = ContinueStoring(assetHintValue, hint, &defaultHint, azrtti_typeid<AZStd::string>(), context);
                if (resultHint.GetOutcome() == JSR::Outcomes::Success || resultHint.GetOutcome() == JSR::Outcomes::PartialDefaults)
                {
                    if (!outputValue.IsObject())
                    {
                        outputValue.SetObject();
                    }
                    outputValue.AddMember(rapidjson::StringRef("assetHint"), AZStd::move(assetHintValue), context.GetJsonAllocator());
                }
                result.Combine(resultHint);
            }

            return context.Report(result,
                result.GetProcessing() == JSR::Processing::Completed ? "Successfully stored Asset<T>." : "Failed to store Asset<T>.");
        }

        JsonSerializationResult::Result AssetJsonSerializer::LoadAsset(void* outputValue, const rapidjson::Value& inputValue,
            JsonDeserializerContext& context)
        {
            namespace JSR = JsonSerializationResult;

            Asset<AssetData>* instance = reinterpret_cast<Asset<AssetData>*>(outputValue);
            AssetId id;
            JSR::ResultCode result(JSR::Tasks::ReadField);

            auto it = inputValue.FindMember("assetId");
            if (it != inputValue.MemberEnd())
            {
                ScopedContextPath subPath(context, "assetId");
                result = ContinueLoading(&id, azrtti_typeid<AssetId>(), it->value, context);
                if (!id.m_guid.IsNull())
                {
                    if (instance->Create(id))
                    {
                        result.Combine(context.Report(result, "Successfully created Asset<T>."));
                    }
                    else
                    {
                        result.Combine(context.Report(JSR::Tasks::Convert, JSR::Outcomes::Unknown,
                            "The asset id was successfully read, but creating an Asset<T> instance from it failed."));
                    }
                }
                else if (result.GetProcessing() == JSR::Processing::Completed)
                {
                    result.Combine(context.Report(JSR::Tasks::ReadField, JSR::Outcomes::DefaultsUsed,
                        "Null Asset<T> created."));
                }
                else
                {
                    result.Combine(context.Report(result, "Failed to retrieve asset id for Asset<T>."));
                }
            }
            else
            {
                result.Combine(context.Report(JSR::Tasks::ReadField, JSR::Outcomes::DefaultsUsed,
                    "The asset id is missing, so there's not enough information to create an Asset<T>."));
            }

            it = inputValue.FindMember("assetHint");
            if (it != inputValue.MemberEnd())
            {
                ScopedContextPath subPath(context, "assetHint");
                AZStd::string hint;
                result.Combine(ContinueLoading(&hint, azrtti_typeid<AZStd::string>(), it->value, context));
                instance->SetHint(AZStd::move(hint));
            }
            else
            {
                result.Combine(context.Report(JSR::Tasks::ReadField, JSR::Outcomes::DefaultsUsed,
                    "The asset hint is missing for Asset<T>, so it will be left empty."));
            }

            bool success = result.GetOutcome() <= JSR::Outcomes::PartialSkip;
            bool defaulted = result.GetOutcome() == JSR::Outcomes::DefaultsUsed || result.GetOutcome() == JSR::Outcomes::PartialDefaults;
            AZStd::string_view message =
                success ? "Successfully loaded information and created instance of Asset<T>." :
                defaulted ? "A default id was provided for Asset<T>, so no instance could be created." :
                "Not enough information was available to create an instance of Asset<T> or data was corrupted.";
            return context.Report(result, message);
        }
    } // namespace Data
} // namespace AZ
