/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Image/AttachmentImageAsset.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
#include <AtomToolsFramework/DynamicNode/DynamicNodeSlotConfig.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <DynamicNode/DynamicNodeSlotConfigSerializer.h>

namespace AtomToolsFramework
{
    AZ_CLASS_ALLOCATOR_IMPL(JsonDynamicNodeSlotConfigSerializer, AZ::SystemAllocator, 0);

    AZ::JsonSerializationResult::Result JsonDynamicNodeSlotConfigSerializer::Load(
        void* outputValue,
        [[maybe_unused]] const AZ::Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue,
        AZ::JsonDeserializerContext& context)
    {
        namespace JSR = AZ::JsonSerializationResult;

        AZ_Assert(
            azrtti_typeid<DynamicNodeSlotConfig>() == outputValueTypeId,
            "Unable to deserialize DynamicNodeSlotConfig from json because the provided type is %s.",
            outputValueTypeId.ToString<AZStd::string>().c_str());

        DynamicNodeSlotConfig* dynamicNodeSlotConfig = reinterpret_cast<DynamicNodeSlotConfig*>(outputValue);
        AZ_Assert(dynamicNodeSlotConfig, "Output value for JsonDynamicNodeSlotConfigSerializer can't be null.");

        JSR::ResultCode result(JSR::Tasks::ReadField);
        result.Combine(ContinueLoadingFromJsonObjectField(
            &dynamicNodeSlotConfig->m_name, azrtti_typeid<decltype(dynamicNodeSlotConfig->m_name)>(), inputValue, "name", context));
        result.Combine(ContinueLoadingFromJsonObjectField(
            &dynamicNodeSlotConfig->m_displayName, azrtti_typeid<decltype(dynamicNodeSlotConfig->m_displayName)>(), inputValue,
            "displayName", context));
        result.Combine(ContinueLoadingFromJsonObjectField(
            &dynamicNodeSlotConfig->m_description, azrtti_typeid<decltype(dynamicNodeSlotConfig->m_description)>(), inputValue,
            "description", context));
        result.Combine(ContinueLoadingFromJsonObjectField(
            &dynamicNodeSlotConfig->m_supportedDataTypes, azrtti_typeid<decltype(dynamicNodeSlotConfig->m_supportedDataTypes)>(),
            inputValue, "supportedDataTypes", context));

        auto serializedSlotValue = inputValue.FindMember("defaultValue");
        if (serializedSlotValue != inputValue.MemberEnd())
        {
            AZStd::any slotValue;
            if (LoadAny<bool>(slotValue, serializedSlotValue->value, context, result) ||
                LoadAny<AZ::u8>(slotValue, serializedSlotValue->value, context, result) ||
                LoadAny<AZ::u16>(slotValue, serializedSlotValue->value, context, result) ||
                LoadAny<AZ::u32>(slotValue, serializedSlotValue->value, context, result) ||
                LoadAny<AZ::u64>(slotValue, serializedSlotValue->value, context, result) ||
                LoadAny<AZ::s8>(slotValue, serializedSlotValue->value, context, result) ||
                LoadAny<AZ::s16>(slotValue, serializedSlotValue->value, context, result) ||
                LoadAny<AZ::s32>(slotValue, serializedSlotValue->value, context, result) ||
                LoadAny<AZ::s64>(slotValue, serializedSlotValue->value, context, result) ||
                LoadAny<float>(slotValue, serializedSlotValue->value, context, result) ||
                LoadAny<double>(slotValue, serializedSlotValue->value, context, result) ||
                LoadAny<AZ::Vector2>(slotValue, serializedSlotValue->value, context, result) ||
                LoadAny<AZ::Vector3>(slotValue, serializedSlotValue->value, context, result) ||
                LoadAny<AZ::Vector4>(slotValue, serializedSlotValue->value, context, result) ||
                LoadAny<AZ::Color>(slotValue, serializedSlotValue->value, context, result) ||
                LoadAny<AZStd::string>(slotValue, serializedSlotValue->value, context, result) ||
                LoadAny<AZ::Data::AssetId>(slotValue, serializedSlotValue->value, context, result) ||
                LoadAny<AZ::Data::Asset<AZ::Data::AssetData>>(slotValue, serializedSlotValue->value, context, result) ||
                LoadAny<AZ::Data::Asset<AZ::RPI::ImageAsset>>(slotValue, serializedSlotValue->value, context, result) ||
                LoadAny<AZ::Data::Asset<AZ::RPI::AttachmentImageAsset>>(slotValue, serializedSlotValue->value, context, result) ||
                LoadAny<AZ::Data::Asset<AZ::RPI::StreamingImageAsset>>(slotValue, serializedSlotValue->value, context, result))
            {
                dynamicNodeSlotConfig->m_defaultValue = slotValue;
            }
        }

        result.Combine(ContinueLoadingFromJsonObjectField(
            &dynamicNodeSlotConfig->m_settings, azrtti_typeid<decltype(dynamicNodeSlotConfig->m_settings)>(), inputValue, "settings",
            context));

        return context.Report(
            result,
            result.GetProcessing() != JSR::Processing::Halted ? "Succesfully loaded DynamicNodeSlotConfig information."
                                                              : "Failed to load DynamicNodeSlotConfig information.");
    }

    AZ::JsonSerializationResult::Result JsonDynamicNodeSlotConfigSerializer::Store(
        rapidjson::Value& outputValue,
        const void* inputValue,
        const void* defaultValue,
        [[maybe_unused]] const AZ::Uuid& valueTypeId,
        AZ::JsonSerializerContext& context)
    {
        namespace JSR = AZ::JsonSerializationResult;

        AZ_Assert(
            azrtti_typeid<DynamicNodeSlotConfig>() == valueTypeId,
            "Unable to Serialize DynamicNodeSlotConfig because the provided type is %s.", valueTypeId.ToString<AZStd::string>().c_str());

        const DynamicNodeSlotConfig* dynamicNodeSlotConfig = reinterpret_cast<const DynamicNodeSlotConfig*>(inputValue);
        AZ_Assert(dynamicNodeSlotConfig, "Input value for JsonDynamicNodeSlotConfigSerializer can't be null.");
        const DynamicNodeSlotConfig* defaultDynamicNodeSlotConfigInstance =
            reinterpret_cast<const DynamicNodeSlotConfig*>(defaultValue);

        outputValue.SetObject();

        JSR::ResultCode result(JSR::Tasks::WriteValue);
        {
            AZ::ScopedContextPath subPath(context, "name");
            const AZStd::string* currentMember = &dynamicNodeSlotConfig->m_name;
            const AZStd::string* defaultMember =
                defaultDynamicNodeSlotConfigInstance ? &defaultDynamicNodeSlotConfigInstance->m_name : nullptr;

            result.Combine(ContinueStoringToJsonObjectField(
                outputValue, "name", currentMember, defaultMember, azrtti_typeid<decltype(*currentMember)>(), context));
        }
        {
            AZ::ScopedContextPath subPath(context, "displayName");
            const AZStd::string* currentMember = &dynamicNodeSlotConfig->m_displayName;
            const AZStd::string* defaultMember =
                defaultDynamicNodeSlotConfigInstance ? &defaultDynamicNodeSlotConfigInstance->m_displayName : nullptr;

            result.Combine(ContinueStoringToJsonObjectField(
                outputValue, "displayName", currentMember, defaultMember, azrtti_typeid<decltype(*currentMember)>(), context));
        }
        {
            AZ::ScopedContextPath subPath(context, "description");
            const AZStd::string* currentMember = &dynamicNodeSlotConfig->m_description;
            const AZStd::string* defaultMember =
                defaultDynamicNodeSlotConfigInstance ? &defaultDynamicNodeSlotConfigInstance->m_description : nullptr;

            result.Combine(ContinueStoringToJsonObjectField(
                outputValue, "description", currentMember, defaultMember, azrtti_typeid<decltype(*currentMember)>(), context));
        }
        {
            AZ::ScopedContextPath subPath(context, "supportedDataTypes");
            const AZStd::vector<AZStd::string>* currentMember = &dynamicNodeSlotConfig->m_supportedDataTypes;
            const AZStd::vector<AZStd::string>* defaultMember =
                defaultDynamicNodeSlotConfigInstance ? &defaultDynamicNodeSlotConfigInstance->m_supportedDataTypes : nullptr;

            result.Combine(ContinueStoringToJsonObjectField(
                outputValue, "supportedDataTypes", currentMember, defaultMember, azrtti_typeid<decltype(*currentMember)>(), context));
        }
        {
            AZ::ScopedContextPath subPath(context, "defaultValue");
            if (!dynamicNodeSlotConfig->m_defaultValue.empty())
            {
                rapidjson::Value outputPropertyValue;
                if (StoreAny<bool>(dynamicNodeSlotConfig->m_defaultValue, outputPropertyValue, context, result) ||
                    StoreAny<AZ::u8>(dynamicNodeSlotConfig->m_defaultValue, outputPropertyValue, context, result) ||
                    StoreAny<AZ::u16>(dynamicNodeSlotConfig->m_defaultValue, outputPropertyValue, context, result) ||
                    StoreAny<AZ::u32>(dynamicNodeSlotConfig->m_defaultValue, outputPropertyValue, context, result) ||
                    StoreAny<AZ::u64>(dynamicNodeSlotConfig->m_defaultValue, outputPropertyValue, context, result) ||
                    StoreAny<AZ::s8>(dynamicNodeSlotConfig->m_defaultValue, outputPropertyValue, context, result) ||
                    StoreAny<AZ::s16>(dynamicNodeSlotConfig->m_defaultValue, outputPropertyValue, context, result) ||
                    StoreAny<AZ::s32>(dynamicNodeSlotConfig->m_defaultValue, outputPropertyValue, context, result) ||
                    StoreAny<AZ::s64>(dynamicNodeSlotConfig->m_defaultValue, outputPropertyValue, context, result) ||
                    StoreAny<float>(dynamicNodeSlotConfig->m_defaultValue, outputPropertyValue, context, result) ||
                    StoreAny<double>(dynamicNodeSlotConfig->m_defaultValue, outputPropertyValue, context, result) ||
                    StoreAny<AZ::Vector2>(dynamicNodeSlotConfig->m_defaultValue, outputPropertyValue, context, result) ||
                    StoreAny<AZ::Vector3>(dynamicNodeSlotConfig->m_defaultValue, outputPropertyValue, context, result) ||
                    StoreAny<AZ::Vector4>(dynamicNodeSlotConfig->m_defaultValue, outputPropertyValue, context, result) ||
                    StoreAny<AZ::Color>(dynamicNodeSlotConfig->m_defaultValue, outputPropertyValue, context, result) ||
                    StoreAny<AZStd::string>(dynamicNodeSlotConfig->m_defaultValue, outputPropertyValue, context, result) ||
                    StoreAny<AZ::Data::AssetId>(dynamicNodeSlotConfig->m_defaultValue, outputPropertyValue, context, result) ||
                    StoreAny<AZ::Data::Asset<AZ::Data::AssetData>>(dynamicNodeSlotConfig->m_defaultValue, outputPropertyValue, context, result) ||
                    StoreAny<AZ::Data::Asset<AZ::RPI::ImageAsset>>(dynamicNodeSlotConfig->m_defaultValue, outputPropertyValue, context, result) ||
                    StoreAny<AZ::Data::Asset<AZ::RPI::AttachmentImageAsset>>(dynamicNodeSlotConfig->m_defaultValue, outputPropertyValue, context, result) ||
                    StoreAny<AZ::Data::Asset<AZ::RPI::StreamingImageAsset>>(dynamicNodeSlotConfig->m_defaultValue, outputPropertyValue, context, result))
                {
                    outputValue.AddMember("defaultValue", outputPropertyValue, context.GetJsonAllocator());
                }
            }
        }
        {
            AZ::ScopedContextPath subPath(context, "settings");
            const DynamicNodeSettingsMap* currentMember = &dynamicNodeSlotConfig->m_settings;
            const DynamicNodeSettingsMap* defaultMember =
                defaultDynamicNodeSlotConfigInstance ? &defaultDynamicNodeSlotConfigInstance->m_settings : nullptr;

            result.Combine(ContinueStoringToJsonObjectField(
                outputValue, "settings", currentMember, defaultMember, azrtti_typeid<decltype(*currentMember)>(), context));
        }

        return context.Report(
            result,
            result.GetProcessing() != JSR::Processing::Halted ? "Successfully stored DynamicNodeSlotConfig information."
                                                              : "Failed to store DynamicNodeSlotConfig information.");
    }

    template<typename T>
    bool JsonDynamicNodeSlotConfigSerializer::LoadAny(
        AZStd::any& slotValue,
        const rapidjson::Value& inputPropertyValue,
        AZ::JsonDeserializerContext& context,
        AZ::JsonSerializationResult::ResultCode& result)
    {
        if (inputPropertyValue.IsObject() && inputPropertyValue.HasMember("Value") && inputPropertyValue.HasMember("$type"))
        {
            // Requiring explicit type info to differentiate between colors versus vectors and numeric types
            const AZ::Uuid baseTypeId = azrtti_typeid<T>();
            AZ::Uuid typeId = AZ::Uuid::CreateNull();
            result.Combine(LoadTypeId(typeId, inputPropertyValue, context, &baseTypeId));

            if (typeId == azrtti_typeid<T>())
            {
                T value = {};
                result.Combine(ContinueLoadingFromJsonObjectField(&value, azrtti_typeid<T>(), inputPropertyValue, "Value", context));
                slotValue = value;
                return true;
            }
        }
        return false;
    }

    template<typename T>
    bool JsonDynamicNodeSlotConfigSerializer::StoreAny(
        const AZStd::any& slotValue,
        rapidjson::Value& outputPropertyValue,
        AZ::JsonSerializerContext& context,
        AZ::JsonSerializationResult::ResultCode& result)
    {
        if (slotValue.is<T>())
        {
            outputPropertyValue.SetObject();

            // Storing explicit type info to differentiate between colors versus vectors and numeric types
            rapidjson::Value typeValue;
            result.Combine(StoreTypeId(typeValue, azrtti_typeid<T>(), context));
            outputPropertyValue.AddMember("$type", typeValue, context.GetJsonAllocator());

            const T& value = AZStd::any_cast<T>(slotValue);
            result.Combine(ContinueStoringToJsonObjectField(outputPropertyValue, "Value", &value, nullptr, azrtti_typeid<T>(), context));
            return true;
        }
        return false;
    }
} // namespace AtomToolsFramework
