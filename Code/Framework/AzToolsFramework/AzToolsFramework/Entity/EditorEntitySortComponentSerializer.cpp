/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/EntityIdSerializer.h>
#include <AzCore/Serialization/Json/JsonSerializationResult.h>
#include <AzCore/std/sort.h>
#include <AzToolsFramework/Entity/EditorEntitySortComponent.h>
#include <AzToolsFramework/Entity/EditorEntitySortComponentSerializer.h>

namespace AzToolsFramework::Components
{
    AZ_CLASS_ALLOCATOR_IMPL(JsonEditorEntitySortComponentSerializer, AZ::SystemAllocator);

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


            // Since sort order component on an entity stores a vector of child entityIds in it,
            // it's possible for the vector to contain stale entityIds if a discrepancy occurs
            // between existing children and the child entityId vector. This is acceptable, so we
            // skip warning on unknown entityIds when serializing.
            // An example of having a stale entityId can be seen when using prefab overrides.
            // When an entity is added/removed as a prefab override, two separate overrides are
            // generated, one referencing the child and the other referencing the parent.
            // Reverting prefab overrides only affects one entity at a time, so reverting overrides on
            // a parent whose child was deleted as an override would cause the deleted child's entityId
            // to be added back to the parent's child order array
            AZ::JsonEntityIdSerializer::JsonEntityIdMapper** idMapper =
                context.GetMetadata().Find<AZ::JsonEntityIdSerializer::JsonEntityIdMapper*>();
            bool prevAcceptUnregisteredEntity = false;
            if (idMapper && *idMapper)
            {
                prevAcceptUnregisteredEntity = (*idMapper)->GetAcceptUnregisteredEntity();
                (*idMapper)->SetAcceptUnregisteredEntity(true);
            }

            JSR::ResultCode resultParentEntityId = ContinueStoringToJsonObjectField(
                outputValue, "Child Entity Order", childEntityOrderArray, defaultChildEntityOrderArray,
                azrtti_typeid<decltype(sortComponentInstance->m_childEntityOrderArray)>(), context);

            if (idMapper && *idMapper)
            {
                (*idMapper)->SetAcceptUnregisteredEntity(prevAcceptUnregisteredEntity);
            }

            result.Combine(resultParentEntityId);
        }

        return context.Report(
            result,
            result.GetProcessing() != JSR::Processing::Halted ? "Successfully stored EditorEntitySortComponent information."
                                                              : "Failed to store EditorEntitySortComponent information.");
    }
} // namespace AzToolsFramework::Components
