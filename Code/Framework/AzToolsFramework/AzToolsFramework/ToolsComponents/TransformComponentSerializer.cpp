/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ToolsComponents/TransformComponentSerializer.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>

namespace AzToolsFramework
{
    namespace Components
    {
        AZ_CLASS_ALLOCATOR_IMPL(JsonTransformComponentSerializer, AZ::SystemAllocator);

        AZ::JsonSerializationResult::Result JsonTransformComponentSerializer::Load(
            void* outputValue, [[maybe_unused]] const AZ::Uuid& outputValueTypeId, const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context)
        {
            namespace JSR = AZ::JsonSerializationResult;

            AZ_Assert(
                azrtti_typeid<TransformComponent>() == outputValueTypeId, "Unable to deserialize TransformComponent from json because the provided type is %s.",
                outputValueTypeId.ToString<AZStd::string>().c_str());

            TransformComponent* transformComponentInstance = reinterpret_cast<TransformComponent*>(outputValue);
            AZ_Assert(transformComponentInstance, "Output value for JsonTransformComponentSerializer can't be null.");

            JSR::ResultCode result(JSR::Tasks::ReadField);
            {
                JSR::ResultCode componentIdLoadResult = ContinueLoadingFromJsonObjectField(
                    &transformComponentInstance->m_id, azrtti_typeid<decltype(transformComponentInstance->m_id)>(),
                    inputValue, "Id", context);

                result.Combine(componentIdLoadResult);
            }

            {
                JSR::ResultCode parentEntityIdLoadResult = ContinueLoadingFromJsonObjectField(
                    &transformComponentInstance->m_parentEntityId, azrtti_typeid<decltype(transformComponentInstance->m_parentEntityId)>(),
                    inputValue, "Parent Entity", context);

                result.Combine(parentEntityIdLoadResult);
            }

            {
                JSR::ResultCode transformDataLoadResult = ContinueLoadingFromJsonObjectField(
                    &transformComponentInstance->m_editorTransform, azrtti_typeid<decltype(transformComponentInstance->m_editorTransform)>(),
                    inputValue, "Transform Data", context);

                result.Combine(transformDataLoadResult);
            }

            {
                JSR::ResultCode parentActivationTransformModeLoadResult = ContinueLoadingFromJsonObjectField(
                    &transformComponentInstance->m_parentActivationTransformMode, azrtti_typeid<decltype(transformComponentInstance->m_parentActivationTransformMode)>(),
                    inputValue, "Parent Activation Transform Mode", context);

                result.Combine(parentActivationTransformModeLoadResult);
            }

            {
                JSR::ResultCode isStaticLoadResult = ContinueLoadingFromJsonObjectField(
                    &transformComponentInstance->m_isStatic, azrtti_typeid<decltype(transformComponentInstance->m_isStatic)>(),
                    inputValue, "IsStatic", context);

                result.Combine(isStaticLoadResult);
            }

            {
                JSR::ResultCode interpolatePositionLoadResult = ContinueLoadingFromJsonObjectField(
                    &transformComponentInstance->m_interpolatePosition, azrtti_typeid<decltype(transformComponentInstance->m_interpolatePosition)>(),
                    inputValue, "InterpolatePosition", context);

                result.Combine(interpolatePositionLoadResult);
            }

            {
                JSR::ResultCode interpolateRotationLoadResult = ContinueLoadingFromJsonObjectField(
                    &transformComponentInstance->m_interpolateRotation, azrtti_typeid<decltype(transformComponentInstance->m_interpolateRotation)>(),
                    inputValue, "InterpolateRotation", context);

                result.Combine(interpolateRotationLoadResult);
            }

            return context.Report(
                result,
                result.GetProcessing() != JSR::Processing::Halted ? "Successfully loaded TransformComponent information."
                                                                  : "Failed to load TransformComponent information.");
        }

        AZ::JsonSerializationResult::Result JsonTransformComponentSerializer::Store(
            rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue, [[maybe_unused]] const AZ::Uuid& valueTypeId,
            AZ::JsonSerializerContext& context)
        {
            namespace JSR = AZ::JsonSerializationResult;

            AZ_Assert(
                azrtti_typeid<TransformComponent>() == valueTypeId, "Unable to Serialize TransformComponent because the provided type is %s.",
                valueTypeId.ToString<AZStd::string>().c_str());

            const TransformComponent* transformComponentInstance = reinterpret_cast<const TransformComponent*>(inputValue);
            AZ_Assert(transformComponentInstance, "Input value for JsonTransformComponentSerializer can't be null.");
            const TransformComponent* defaultTransformComponentInstance = reinterpret_cast<const TransformComponent*>(defaultValue);

            JSR::ResultCode result(JSR::Tasks::WriteValue);
            {
                AZ::ScopedContextPath subPathName(context, "m_id");
                const AZ::ComponentId* componentId = &transformComponentInstance->m_id;
                const AZ::ComponentId* defaultComponentId = defaultTransformComponentInstance ? &defaultTransformComponentInstance->m_id : nullptr;

                JSR::ResultCode resultComponentId = ContinueStoringToJsonObjectField(
                    outputValue, "Id", componentId, defaultComponentId, azrtti_typeid<decltype(transformComponentInstance->m_id)>(), context);

                result.Combine(resultComponentId);
            }

            {
                AZ::ScopedContextPath subPathName(context, "m_parentEntityId");
                const AZ::EntityId* parentEntityId = &transformComponentInstance->m_parentEntityId;
                const AZ::EntityId* defaultParentEntityId = defaultTransformComponentInstance ? &defaultTransformComponentInstance->m_parentEntityId : nullptr;

                JSR::ResultCode resultParentEntityId = ContinueStoringToJsonObjectField(
                    outputValue, "Parent Entity", parentEntityId, defaultParentEntityId, azrtti_typeid<decltype(transformComponentInstance->m_parentEntityId)>(), context);

                result.Combine(resultParentEntityId);
            }

            {
                AZ::ScopedContextPath subPathName(context, "m_editorTransform");
                const EditorTransform* editorTransform = &transformComponentInstance->m_editorTransform;
                const EditorTransform* defaultEditorTransform =
                    defaultTransformComponentInstance ? &defaultTransformComponentInstance->m_editorTransform : nullptr;

                JSR::ResultCode resultEditorTransform = ContinueStoringToJsonObjectField(
                    outputValue, "Transform Data", editorTransform, defaultEditorTransform,
                    azrtti_typeid<decltype(transformComponentInstance->m_editorTransform)>(), context);

                result.Combine(resultEditorTransform);
            }

            {
                AZ::ScopedContextPath subPathName(context, "m_parentActivationTransformMode");
                const AZ::TransformConfig::ParentActivationTransformMode* parentActivationTransformMode = &transformComponentInstance->m_parentActivationTransformMode;
                const AZ::TransformConfig::ParentActivationTransformMode* defaultParentActivationTransformMode =
                    defaultTransformComponentInstance ? &defaultTransformComponentInstance->m_parentActivationTransformMode : nullptr;

                JSR::ResultCode resultParentActivationTransformMode = ContinueStoringToJsonObjectField(
                    outputValue, "Parent Activation Transform Mode", parentActivationTransformMode, defaultParentActivationTransformMode,
                    azrtti_typeid<decltype(transformComponentInstance->m_parentActivationTransformMode)>(), context);

                result.Combine(resultParentActivationTransformMode);
            }

            {
                AZ::ScopedContextPath subPathName(context, "m_isStatic");
                const bool* isStatic = &transformComponentInstance->m_isStatic;
                const bool* defaultIsStatic = defaultTransformComponentInstance ? &defaultTransformComponentInstance->m_isStatic : nullptr;

                JSR::ResultCode resultIsStatic = ContinueStoringToJsonObjectField(
                    outputValue, "IsStatic", isStatic, defaultIsStatic,
                    azrtti_typeid<decltype(transformComponentInstance->m_isStatic)>(), context);

                result.Combine(resultIsStatic);
            }

            {
                AZ::ScopedContextPath subPathName(context, "m_interpolatePosition");
                const AZ::InterpolationMode* interpolatePosition = &transformComponentInstance->m_interpolatePosition;
                const AZ::InterpolationMode* defaultInterpolatePosition = defaultTransformComponentInstance ? &defaultTransformComponentInstance->m_interpolatePosition : nullptr;

                JSR::ResultCode resultInterpolatePosition = ContinueStoringToJsonObjectField(
                    outputValue, "InterpolatePosition", interpolatePosition, defaultInterpolatePosition, azrtti_typeid<decltype(transformComponentInstance->m_interpolatePosition)>(),
                    context);

                result.Combine(resultInterpolatePosition);
            }

            {
                AZ::ScopedContextPath subPathName(context, "m_interpolateRotation");
                const AZ::InterpolationMode* interpolateRotation = &transformComponentInstance->m_interpolateRotation;
                const AZ::InterpolationMode* defaultInterpolateRotation = defaultTransformComponentInstance ? &defaultTransformComponentInstance->m_interpolateRotation : nullptr;

                JSR::ResultCode resultInterpolateRotation = ContinueStoringToJsonObjectField(
                    outputValue, "InterpolateRotation", interpolateRotation, defaultInterpolateRotation, azrtti_typeid<decltype(transformComponentInstance->m_interpolateRotation)>(),
                    context);

                result.Combine(resultInterpolateRotation);
            }

            return context.Report(
                result,
                result.GetProcessing() != JSR::Processing::Halted ? "Successfully stored TransformComponent information."
                                                                  : "Failed to store TransformComponent information.");
        }

    } // namespace Components
} // namespace AzToolsFramework
