/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/Json/JsonSerializationResult.h>
#include <Material/EditorMaterialComponent.h>
#include <Material/EditorMaterialComponentSerializer.h>

namespace AZ
{
    namespace Render
    {
        AZ_CLASS_ALLOCATOR_IMPL(JsonEditorMaterialComponentSerializer, AZ::SystemAllocator);

        AZ::JsonSerializationResult::Result JsonEditorMaterialComponentSerializer::Load(
            void* outputValue,
            [[maybe_unused]] const AZ::Uuid& outputValueTypeId,
            const rapidjson::Value& inputValue,
            AZ::JsonDeserializerContext& context)
        {
            namespace JSR = AZ::JsonSerializationResult;

            AZ_Assert(
                azrtti_typeid<EditorMaterialComponent>() == outputValueTypeId,
                "Unable to deserialize EditorMaterialComponent from json because the provided type is %s.",
                outputValueTypeId.ToString<AZStd::string>().c_str());

            auto componentInstance = reinterpret_cast<EditorMaterialComponent*>(outputValue);
            AZ_Assert(componentInstance, "Output value for JsonEditorMaterialComponentSerializer can't be null.");

            JSR::ResultCode result(JSR::Tasks::ReadField);

            result.Combine(ContinueLoadingFromJsonObjectField(
                &componentInstance->m_id, azrtti_typeid<decltype(componentInstance->m_id)>(), inputValue, "Id", context));

            result.Combine(ContinueLoadingFromJsonObjectField(
                &componentInstance->m_controller, azrtti_typeid<decltype(componentInstance->m_controller)>(), inputValue, "Controller",
                context));

            result.Combine(ContinueLoadingFromJsonObjectField(
                &componentInstance->m_materialSlotsByLodEnabled, azrtti_typeid<decltype(componentInstance->m_materialSlotsByLodEnabled)>(),
                inputValue, "materialSlotsByLodEnabled", context));

            return context.Report(
                result,
                result.GetProcessing() != JSR::Processing::Halted ? "Successfully loaded EditorMaterialComponent information."
                                                                  : "Failed to load EditorMaterialComponent information.");
        }

        AZ::JsonSerializationResult::Result JsonEditorMaterialComponentSerializer::Store(
            rapidjson::Value& outputValue,
            const void* inputValue,
            const void* defaultValue,
            [[maybe_unused]] const AZ::Uuid& valueTypeId,
            AZ::JsonSerializerContext& context)
        {
            namespace JSR = AZ::JsonSerializationResult;

            AZ_Assert(
                azrtti_typeid<EditorMaterialComponent>() == valueTypeId,
                "Unable to Serialize EditorMaterialComponent because the provided type is %s.",
                valueTypeId.ToString<AZStd::string>().c_str());

            auto componentInstance = reinterpret_cast<const EditorMaterialComponent*>(inputValue);
            AZ_Assert(componentInstance, "Input value for JsonEditorMaterialComponentSerializer can't be null.");
            auto defaultComponentInstance = reinterpret_cast<const EditorMaterialComponent*>(defaultValue);

            JSR::ResultCode result(JSR::Tasks::WriteValue);
            {
                AZ::ScopedContextPath subPathName(context, "m_id");
                const auto componentId = &componentInstance->m_id;
                const auto defaultComponentId = defaultComponentInstance ? &defaultComponentInstance->m_id : nullptr;

                result.Combine(ContinueStoringToJsonObjectField(
                    outputValue, "Id", componentId, defaultComponentId, azrtti_typeid<decltype(componentInstance->m_id)>(), context));
            }

            {
                AZ::ScopedContextPath subPathName(context, "Controller");
                const auto controller = &componentInstance->m_controller;
                const auto defaultController = defaultComponentInstance ? &defaultComponentInstance->m_controller : nullptr;

                result.Combine(ContinueStoringToJsonObjectField(
                    outputValue, "Controller", controller, defaultController, azrtti_typeid<decltype(componentInstance->m_controller)>(),
                    context));
            }

            {
                AZ::ScopedContextPath subPathName(context, "materialSlotsByLodEnabled");
                const auto enabled = &componentInstance->m_materialSlotsByLodEnabled;
                const auto defaultEnabled = defaultComponentInstance ? &defaultComponentInstance->m_materialSlotsByLodEnabled : nullptr;

                result.Combine(ContinueStoringToJsonObjectField(
                    outputValue, "materialSlotsByLodEnabled", enabled, defaultEnabled,
                    azrtti_typeid<decltype(componentInstance->m_materialSlotsByLodEnabled)>(), context));
            }

            return context.Report(
                result,
                result.GetProcessing() != JSR::Processing::Halted ? "Successfully stored EditorMaterialComponent information."
                                                                  : "Failed to store EditorMaterialComponent information.");
        }

    } // namespace Render
} // namespace AZ
