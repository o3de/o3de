/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/containers/map.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/EntityIdSerializer.h>
#include <AzCore/Component/EntitySerializer.h>
#include <AzCore/Component/EntityUtils.h>

namespace AZ
{
    AZ_CLASS_ALLOCATOR_IMPL(JsonEntitySerializer, AZ::SystemAllocator);

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

            JSR::ResultCode idLoadResult = ContinueLoadingFromJsonObjectField(
                &entityInstance->m_id, azrtti_typeid<decltype(entityInstance->m_id)>(), inputValue, "Id", context);

            // If the entity has an invalid ID, there's no point in deserializing, the entity will be unusable.
            // It's also dangerous to generate new IDs here:
            // - They need to be globally unique
            // - We don't know *why* it's invalid (maybe just a typo on the name "Id" for example), so we don't know the ramifications
            //   of changing it.  There might be many other entities that have references to this one that would become invalid as well
            //   if we try to silently fix it up.
            // - Unless we save the ID immediately, it will change every time we serialize the data in, which can happen multiple times
            //   during the serialization pipeline.  So it either needs to be saved back immediately, or we need a deterministic way
            //   to generate a globally unique ID for the entity.
            if (!entityInstance->GetId().IsValid())
            {
                // Since we're going to halt processing anyways, we just return the error here immediately.
                return context.Report(
                    JSR::ResultCode(JSR::Tasks::ReadField, JSR::Outcomes::Invalid),
                    "Invalid or missing entity ID - please add an 'Id' field to this entity with a globally unique id.  \n"
                    "Failed to load entity information.");
            }

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
            AZStd::unordered_map<AZStd::string, AZ::Component*> componentMap;
            JSR::ResultCode componentLoadResult =
                ContinueLoadingFromJsonObjectField(&componentMap,
                    azrtti_typeid<decltype(componentMap)>(),
                    inputValue, "Components", context);

            static TypeId genericComponentWrapperTypeId("{68D358CA-89B9-4730-8BA6-E181DEA28FDE}");
            for (auto& [componentKey, component] : componentMap)
            {
                // if the component didn't serialize (i.e. is null) or the underlying type is genericComponentWrapperTypeId, the
                // template is null and the component should not be added
                if (component && (component->GetUnderlyingComponentType() != genericComponentWrapperTypeId))
                {
                    entityInstance->m_components.emplace_back(component);
                    component->SetSerializedIdentifier(componentKey);
                }
            }

            result.Combine(componentLoadResult);
        }

        ContinueLoadingFromJsonObjectField(&entityInstance->m_isRuntimeActiveByDefault,
            azrtti_typeid<decltype(entityInstance->m_isRuntimeActiveByDefault)>(),
            inputValue, "IsRuntimeActive", context);
 
        AZStd::string_view message = result.GetProcessing() == JSR::Processing::Completed
            ? "Successfully loaded entity information."
            : (result.GetProcessing() != JSR::Processing::Halted ? "Partially loaded entity information."
                                                                 : "Failed to load entity information.");

        return context.Report(result, message);
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

            AZStd::unordered_map<AZStd::string, AZ::Component*> componentMap;
            AZStd::unordered_map<AZStd::string, AZ::Component*> defaultComponentMap;

            EntityUtils::ConvertComponentVectorToMap(*components, componentMap);

            if (defaultComponents)
            {
                EntityUtils::ConvertComponentVectorToMap(*defaultComponents, defaultComponentMap);
            }

            JSR::ResultCode resultComponents =
                ContinueStoringToJsonObjectField(outputValue, "Components",
                    &componentMap,
                    defaultComponents ? &defaultComponentMap : nullptr,
                    azrtti_typeid<decltype(componentMap)>(), context);

            result.Combine(resultComponents);
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
            result.GetProcessing() != JSR::Processing::Halted ? "Successfully stored Entity information." :
            "Failed to store Entity information.");
    }

    void DeprecatedComponentMetadata::SetEnableDeprecationTrackingCallback(EnableDeprecationTrackingCallback callback)
    {
        m_enableDeprecationTrackingCallback = callback;
    }

    void DeprecatedComponentMetadata::AddComponent(const TypeId& componentType)
    {
        if (!m_enableDeprecationTrackingCallback || m_enableDeprecationTrackingCallback())
        {
            m_componentTypes.insert(componentType);
        }
    }

    AZStd::vector<AZStd::string> DeprecatedComponentMetadata::GetComponentNames() const
    {
        AZStd::vector<AZStd::string> componentNames;

        componentNames.reserve(m_componentTypes.size());
        for (auto componentType : m_componentTypes)
        {
            AZ::ComponentDescriptor* descriptor = nullptr;
            AZ::ComponentDescriptorBus::EventResult(descriptor, componentType, &AZ::ComponentDescriptorBus::Events::GetDescriptor);
            if (descriptor)
            {
                componentNames.push_back(descriptor->GetName());
            }
        }

        return componentNames;
    }

} // namespace AZ
