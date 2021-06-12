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

#include <AzToolsFramework/Entity/EditorEntitySortComponentSerializer.h>
#include <AzToolsFramework/Entity/EditorEntitySortComponent.h>

namespace AzToolsFramework
{
    namespace Components
    {
        AZ_CLASS_ALLOCATOR_IMPL(JsonEditorEntitySortComponentSerializer, AZ::SystemAllocator, 0);

        AZ::JsonSerializationResult::Result JsonEditorEntitySortComponentSerializer::Load(
            void* outputValue, [[maybe_unused]] const AZ::Uuid& outputValueTypeId, const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context)
        {
            namespace JSR = AZ::JsonSerializationResult;

            AZ_Assert(
                azrtti_typeid<EditorEntitySortComponent>() == outputValueTypeId, "Unable to deserialize EditorEntitySortComponent from json because the provided type is %s.",
                outputValueTypeId.ToString<AZStd::string>().c_str());

            EditorEntitySortComponent* editorEntitySortComponentInstance = reinterpret_cast<EditorEntitySortComponent*>(outputValue);
            AZ_Assert(editorEntitySortComponentInstance, "Output value for JsonEditorEntitySortComponentSerializer can't be null.");

            JSR::ResultCode result(JSR::Tasks::ReadField);
            {
                JSR::ResultCode componentIdLoadResult = ContinueLoadingFromJsonObjectField(
                    &editorEntitySortComponentInstance->m_id, azrtti_typeid<decltype(editorEntitySortComponentInstance->m_id)>(),
                    inputValue, "Id", context);

                result.Combine(componentIdLoadResult);
            }

            {
                JSR::ResultCode parentEntityIdLoadResult = ContinueLoadingFromJsonObjectField(
                    &editorEntitySortComponentInstance->m_childEntityOrderArray, azrtti_typeid<decltype(editorEntitySortComponentInstance->m_childEntityOrderArray)>(),
                    inputValue, "ChildEntityOrderArray", context);

                result.Combine(parentEntityIdLoadResult);
            }

            return context.Report(
                result,
                result.GetProcessing() != JSR::Processing::Halted ? "Successfully loaded EditorEntitySortComponent information."
                                                                  : "Failed to load EditorEntitySortComponent information.");
        }

        AZ::JsonSerializationResult::Result JsonEditorEntitySortComponentSerializer::Store(
            rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue, [[maybe_unused]] const AZ::Uuid& valueTypeId,
            AZ::JsonSerializerContext& context)
        {
            namespace JSR = AZ::JsonSerializationResult;

            AZ_Assert(
                azrtti_typeid<EditorEntitySortComponent>() == valueTypeId, "Unable to Serialize EditorEntitySortComponent because the provided type is %s.",
                valueTypeId.ToString<AZStd::string>().c_str());

            const EditorEntitySortComponent* editorEntitySortComponentInstance = reinterpret_cast<const EditorEntitySortComponent*>(inputValue);
            AZ_Assert(editorEntitySortComponentInstance, "Input value for JsonEditorEntitySortComponentSerializer can't be null.");
            const EditorEntitySortComponent* defaultEditorEntitySortComponentInstance = reinterpret_cast<const EditorEntitySortComponent*>(defaultValue);

            JSR::ResultCode result(JSR::Tasks::WriteValue);
            {
                AZ::ScopedContextPath subPathName(context, "m_id");
                const AZ::ComponentId* componentId = &editorEntitySortComponentInstance->m_id;
                const AZ::ComponentId* defaultComponentId = defaultEditorEntitySortComponentInstance ? &defaultEditorEntitySortComponentInstance->m_id : nullptr;

                JSR::ResultCode resultComponentId = ContinueStoringToJsonObjectField(
                    outputValue, "Id", componentId, defaultComponentId, azrtti_typeid<decltype(editorEntitySortComponentInstance->m_id)>(), context);

                result.Combine(resultComponentId);
            }

            {
                AZ::ScopedContextPath subPathName(context, "m_childEntityOrderArray");
                const EntityOrderArray* childEntityOrderArray = &editorEntitySortComponentInstance->m_childEntityOrderArray;
                const EntityOrderArray* defaultChildEntityOrderArray = editorEntitySortComponentInstance ? &defaultEditorEntitySortComponentInstance->m_childEntityOrderArray : nullptr;

                JSR::ResultCode resultParentEntityId = ContinueStoringToJsonObjectField(
                    outputValue, "ChildEntityOrderArray", childEntityOrderArray, defaultChildEntityOrderArray, azrtti_typeid<decltype(editorEntitySortComponentInstance->m_childEntityOrderArray)>(), context);

                result.Combine(resultParentEntityId);
            }

            return context.Report(
                result,
                result.GetProcessing() != JSR::Processing::Halted ? "Successfully stored TransformComponent information."
                                                                  : "Failed to store TransformComponent information.");
        }

    } // namespace Components
} // namespace AzToolsFramework
