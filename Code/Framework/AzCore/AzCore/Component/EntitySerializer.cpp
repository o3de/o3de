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

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/EntityIdSerializer.h>
#include <AzCore/Component/EntitySerializer.h>

namespace AZ
{
    AZ_CLASS_ALLOCATOR_IMPL(JsonEntitySerializer, AZ::SystemAllocator, 0);

    JsonSerializationResult::Result JsonEntitySerializer::Load(void* outputValue, [[maybe_unused]] const Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue, JsonDeserializerContext& context)
    {
        namespace JSR = JsonSerializationResult;

        AZ_Assert(azrtti_typeid<AZ::Entity>() == outputValueTypeId, "Unable to deserialize Entity from json because the provided type is %s.",
            outputValueTypeId.ToString<AZStd::string>().c_str());

        AZ::Entity* entityInstance = reinterpret_cast<AZ::Entity*>(outputValue);
        AZ_Assert(entityInstance, "Output value for JsonEntitySerializer can't be null.");

        JsonEntityIdSerializer::JsonEntityIdMapper** idMapper = context.GetMetadata().Find<JsonEntityIdSerializer::JsonEntityIdMapper*>();
        bool hasValidIdMapper = idMapper && *idMapper;

        JSR::ResultCode result(JSR::Tasks::ReadField);
        {
            if (hasValidIdMapper)
            {
                (*idMapper)->SetIsEntityReference(false);
            }

            JSR::ResultCode idLoadResult =
                ContinueLoadingFromJsonObjectField(&entityInstance->m_id,
                    azrtti_typeid<decltype(entityInstance->m_id)>(),
                    inputValue, "Id", context);

            if (hasValidIdMapper)
            {
                (*idMapper)->SetIsEntityReference(true);
            }

            result.Combine(idLoadResult);
        }

        {
            JSR::ResultCode nameLoadResult =
                ContinueLoadingFromJsonObjectField(&entityInstance->m_name,
                    azrtti_typeid<decltype(entityInstance->m_name)>(),
                    inputValue, "Name", context);

            result.Combine(nameLoadResult);
        }

        {
            JSR::ResultCode componentLoadResult =
                ContinueLoadingFromJsonObjectField(&entityInstance->m_components,
                    azrtti_typeid<decltype(entityInstance->m_components)>(),
                    inputValue, "Components", context);

            result.Combine(componentLoadResult);
        }

        {
            JSR::ResultCode dependencyReadyLoadResult =
                ContinueLoadingFromJsonObjectField(&entityInstance->m_isDependencyReady,
                    azrtti_typeid<decltype(entityInstance->m_isDependencyReady)>(),
                    inputValue, "IsDependencyReady", context);

            result.Combine(dependencyReadyLoadResult);
        }

        {
            JSR::ResultCode runtimeActiveLoadResult =
                ContinueLoadingFromJsonObjectField(&entityInstance->m_isRuntimeActiveByDefault,
                    azrtti_typeid<decltype(entityInstance->m_isRuntimeActiveByDefault)>(),
                    inputValue, "IsRuntimeActive", context);
        }

        return context.Report(result,
            result.GetProcessing() == JSR::Processing::Halted ? "Succesfully loaded entity information." :
            "Failed to load entity information.");
    }

    JsonSerializationResult::Result JsonEntitySerializer::Store(rapidjson::Value& outputValue,
        const void* inputValue, const void* defaultValue, [[maybe_unused]] const Uuid& valueTypeId, JsonSerializerContext& context)
    {
        namespace JSR = AZ::JsonSerializationResult;

        AZ_Assert(azrtti_typeid<AZ::Entity>() == valueTypeId, "Unable to Serialize Entity because the provided type is %s.",
            valueTypeId.ToString<AZStd::string>().c_str());

        const AZ::Entity* entityInstance = reinterpret_cast<const AZ::Entity*>(inputValue);
        AZ_Assert(entityInstance, "Input value for JsonEntitySerializer can't be null.");
        const AZ::Entity* defaultEntityInstance = reinterpret_cast<const AZ::Entity*>(defaultValue);

        JsonEntityIdSerializer::JsonEntityIdMapper** idMapper = context.GetMetadata().Find<JsonEntityIdSerializer::JsonEntityIdMapper*>();
        bool hasValidIdMapper = idMapper && *idMapper;

        JSR::ResultCode result(JSR::Tasks::WriteValue);
        {
            AZ::ScopedContextPath subPathId(context, "m_id");
            const AZ::EntityId* id = &entityInstance->m_id;
            const AZ::EntityId* defaultId = defaultEntityInstance ? &defaultEntityInstance->m_id : nullptr;

            if (hasValidIdMapper)
            {
                (*idMapper)->SetIsEntityReference(false);
            }

            result = ContinueStoringToJsonObjectField(outputValue, "Id",
                id, defaultId, azrtti_typeid<decltype(entityInstance->m_id)>(), context);

            if (hasValidIdMapper)
            {
                (*idMapper)->SetIsEntityReference(true);
            }
        }

        {
            AZ::ScopedContextPath subPathName(context, "m_name");
            const AZStd::string* name = &entityInstance->m_name;
            const AZStd::string* defaultName =
                defaultEntityInstance ? &defaultEntityInstance->m_name : nullptr;

            JSR::ResultCode resultName =
                ContinueStoringToJsonObjectField(outputValue, "Name",
                    name, defaultName, azrtti_typeid<decltype(entityInstance->m_name)>(), context);

            result.Combine(resultName);
        }

        {
            AZ::ScopedContextPath subPathComponents(context, "m_components");
            const AZ::Entity::ComponentArrayType* components = &entityInstance->m_components;
            const AZ::Entity::ComponentArrayType* defaultComponents =
                defaultEntityInstance ? &defaultEntityInstance->m_components : nullptr;

            JSR::ResultCode resultComponents =
                ContinueStoringToJsonObjectField(outputValue, "Components",
                    components, defaultComponents, azrtti_typeid<decltype(entityInstance->m_components)>(), context);

            result.Combine(resultComponents);
        }

        {
            AZ::ScopedContextPath subPathDependencyReady(context, "m_isDependencyReady");
            const bool* dependencyReady = &entityInstance->m_isDependencyReady;
            const bool* dependencyReadyDefault =
                defaultEntityInstance ? &defaultEntityInstance->m_isDependencyReady : nullptr;

            JSR::ResultCode resultDependencyReady =
                ContinueStoringToJsonObjectField(outputValue, "IsDependencyReady",
                    dependencyReady, dependencyReadyDefault,
                    azrtti_typeid<decltype(entityInstance->m_isDependencyReady)>(), context);

            result.Combine(resultDependencyReady);
        }

        {
            AZ::ScopedContextPath subPathRuntimeActive(context, "m_isRuntimeActiveByDefault");
            const bool* runtimeActive = &entityInstance->m_isRuntimeActiveByDefault;
            const bool* runtimeActiveDefault =
                defaultEntityInstance ? &defaultEntityInstance->m_isRuntimeActiveByDefault : nullptr;

            JSR::ResultCode resultRuntimeActive =
                ContinueStoringToJsonObjectField(outputValue, "IsRuntimeActive",
                    runtimeActive, runtimeActiveDefault, azrtti_typeid<decltype(entityInstance->m_isRuntimeActiveByDefault)>(), context);

            result.Combine(resultRuntimeActive);
        }

        return context.Report(result,
            result.GetProcessing() == JSR::Processing::Halted ? "Successfully stored Entity information." :
            "Failed to store Entity information.");
    }
}
