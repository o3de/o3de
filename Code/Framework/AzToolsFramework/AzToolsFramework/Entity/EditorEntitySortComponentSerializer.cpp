/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/Json/JsonSerializationResult.h>
#include <AzCore/std/sort.h>
#include <AzToolsFramework/Entity/EditorEntitySortComponent.h>
#include <AzToolsFramework/Entity/EditorEntitySortComponentSerializer.h>

namespace AzToolsFramework::Components
{
    AZ_CLASS_ALLOCATOR_IMPL(JsonEditorEntitySortComponentSerializer, AZ::SystemAllocator, 0);

    AZ::JsonSerializationResult::Result JsonEditorEntitySortComponentSerializer::Load(
        void* outputValue,
        [[maybe_unused]] const AZ::Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue,
        AZ::JsonDeserializerContext& context)
    {
        namespace JSR = AZ::JsonSerializationResult;

        AZ_Assert(
            azrtti_typeid<EditorEntitySortComponent>() == outputValueTypeId,
            "Unable to deserialize EditorEntitySortComponent from json because the provided type is %s.",
            outputValueTypeId.ToString<AZStd::string>().c_str());

        EditorEntitySortComponent* sortComponentInstance = reinterpret_cast<EditorEntitySortComponent*>(outputValue);
        AZ_Assert(sortComponentInstance, "Output value for JsonEditorEntitySortComponentSerializer can't be null.");

        JSR::ResultCode result(JSR::Tasks::ReadField);
        {
            JSR::ResultCode componentIdLoadResult = ContinueLoadingFromJsonObjectField(
                &sortComponentInstance->m_id, azrtti_typeid<decltype(sortComponentInstance->m_id)>(), inputValue,
                "Id", context);

            result.Combine(componentIdLoadResult);
        }

        {
            sortComponentInstance->m_childEntityOrderArray.clear();
            JSR::ResultCode enryLoadResult = ContinueLoadingFromJsonObjectField(
                &sortComponentInstance->m_childEntityOrderArray,
                azrtti_typeid<decltype(sortComponentInstance->m_childEntityOrderArray)>(), inputValue, "Child Entity Order",
                context);

            // Migrate ChildEntityOrderEntryArray -> ChildEntityOrderArray
            if (sortComponentInstance->m_childEntityOrderArray.empty())
            {
                enryLoadResult = ContinueLoadingFromJsonObjectField(
                    &sortComponentInstance->m_childEntityOrderEntryArray,
                    azrtti_typeid<decltype(sortComponentInstance->m_childEntityOrderEntryArray)>(), inputValue,
                    "ChildEntityOrderEntryArray", context);

                AZStd::sort(
                    sortComponentInstance->m_childEntityOrderEntryArray.begin(),
                    sortComponentInstance->m_childEntityOrderEntryArray.end(),
                    [](const EditorEntitySortComponent::EntityOrderEntry& lhs,
                       const EditorEntitySortComponent::EntityOrderEntry& rhs) -> bool
                    {
                        return lhs.m_sortIndex < rhs.m_sortIndex;
                    });

                // Sort by index and copy to the order array, any duplicates or invalid entries will be cleaned up by the sanitization pass
                sortComponentInstance->m_childEntityOrderArray.resize(sortComponentInstance->m_childEntityOrderEntryArray.size());
                for (size_t i = 0; i < sortComponentInstance->m_childEntityOrderEntryArray.size(); ++i)
                {
                    sortComponentInstance->m_childEntityOrderArray[i] = sortComponentInstance->m_childEntityOrderEntryArray[i].m_entityId;
                }
            }

            sortComponentInstance->RebuildEntityOrderCache();

            result.Combine(enryLoadResult);
        }

        return context.Report(
            result,
            result.GetProcessing() != JSR::Processing::Halted ? "Successfully loaded EditorEntitySortComponent information."
                                                              : "Failed to load EditorEntitySortComponent information.");
    }

    AZ::JsonSerializationResult::Result JsonEditorEntitySortComponentSerializer::Store(
        rapidjson::Value& outputValue,
        const void* inputValue,
        const void* defaultValue,
        [[maybe_unused]] const AZ::Uuid& valueTypeId,
        AZ::JsonSerializerContext& context)
    {
        namespace JSR = AZ::JsonSerializationResult;

        AZ_Assert(
            azrtti_typeid<EditorEntitySortComponent>() == valueTypeId,
            "Unable to Serialize EditorEntitySortComponent because the provided type is %s.",
            valueTypeId.ToString<AZStd::string>().c_str());

        const EditorEntitySortComponent* sortComponentInstance = reinterpret_cast<const EditorEntitySortComponent*>(inputValue);
        AZ_Assert(sortComponentInstance, "Input value for JsonEditorEntitySortComponentSerializer can't be null.");
        const EditorEntitySortComponent* defaultsortComponentInstance =
            reinterpret_cast<const EditorEntitySortComponent*>(defaultValue);

        JSR::ResultCode result(JSR::Tasks::WriteValue);
        {
            AZ::ScopedContextPath subPathName(context, "m_id");
            const AZ::ComponentId* componentId = &sortComponentInstance->m_id;
            const AZ::ComponentId* defaultComponentId =
                defaultsortComponentInstance ? &defaultsortComponentInstance->m_id : nullptr;

            JSR::ResultCode resultComponentId = ContinueStoringToJsonObjectField(
                outputValue, "Id", componentId, defaultComponentId, azrtti_typeid<decltype(sortComponentInstance->m_id)>(),
                context);

            result.Combine(resultComponentId);
        }

        {
            AZ::ScopedContextPath subPathName(context, "m_childEntityOrderArray");
            const EntityOrderArray* childEntityOrderArray = &sortComponentInstance->m_childEntityOrderArray;
            const EntityOrderArray* defaultChildEntityOrderArray =
                defaultsortComponentInstance ? &defaultsortComponentInstance->m_childEntityOrderArray : nullptr;

            JSR::ResultCode resultParentEntityId = ContinueStoringToJsonObjectField(
                outputValue, "Child Entity Order", childEntityOrderArray, defaultChildEntityOrderArray,
                azrtti_typeid<decltype(sortComponentInstance->m_childEntityOrderArray)>(), context);

            result.Combine(resultParentEntityId);
        }

        return context.Report(
            result,
            result.GetProcessing() != JSR::Processing::Halted ? "Successfully stored EditorEntitySortComponent information."
                                                              : "Failed to store EditorEntitySortComponent information.");
    }
} // namespace AzToolsFramework::Components
