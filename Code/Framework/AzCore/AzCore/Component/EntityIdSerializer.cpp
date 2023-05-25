/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/EntityId.h>
#include <AzCore/Component/EntityIdSerializer.h>

namespace AZ
{
    AZ_CLASS_ALLOCATOR_IMPL(JsonEntityIdSerializer, SystemAllocator);

    JsonSerializationResult::Result JsonEntityIdSerializer::Load(void* outputValue, [[maybe_unused]] const Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue, JsonDeserializerContext& context)
    {
        namespace JSR = JsonSerializationResult;

        AZ_Assert(azrtti_typeid<AZ::EntityId>() == outputValueTypeId,
            "Unable to deserialize EntityId from json because the provided type is %s",
            outputValueTypeId.ToString<AZStd::string>().c_str());

        AZ::EntityId* entityIdInstance = reinterpret_cast<AZ::EntityId*>(outputValue);
        AZ_Assert(entityIdInstance, "Output value for JsonEntityIdSerializer can't be null");

        JSR::ResultCode result(JSR::Tasks::ReadField);
        JsonEntityIdMapper** idMapper = context.GetMetadata().Find<JsonEntityIdMapper*>();

        // Load the id via a mapper if provided
        if (idMapper && *idMapper)
        {
            result.Combine((*idMapper)->MapJsonToId(*entityIdInstance, inputValue, context));
        }
        else if(inputValue.IsObject())
        {
            // Otherwise attempt to acquire the id member
            auto idMember = inputValue.FindMember("id");
            if (idMember != inputValue.MemberEnd())
            {
                AZ::ScopedContextPath subPathId(context, "id");
                result.Combine(ContinueLoading(&entityIdInstance->m_id, azrtti_typeid<AZ::u64>(), idMember->value, context));
            }
            else
            {
                result.Combine(JSR::ResultCode(JSR::Tasks::ReadField, JSR::Outcomes::DefaultsUsed));
            }
        }
        else
        {
            // Default if neither mapper or id member are present
            result.Combine(JSR::ResultCode(JSR::Tasks::ReadField, JSR::Outcomes::DefaultsUsed));
        }

        return context.Report(result,
            result.GetProcessing() == JSR::Processing::Completed ? "Succesfully loaded Entity Id information." :
            "Failed to load Entity Id information.");
    }

    JsonSerializationResult::Result JsonEntityIdSerializer::Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
        [[maybe_unused]] const Uuid& valueTypeId, JsonSerializerContext& context)
    {
        namespace JSR = JsonSerializationResult;

        AZ_Assert(azrtti_typeid<AZ::EntityId>() == valueTypeId, "Unable to Serialize Entity Id because the provided type is %s",
            valueTypeId.ToString<AZStd::string>().c_str());

        const EntityId* entityIdInstance = reinterpret_cast<const EntityId*>(inputValue);
        AZ_Assert(entityIdInstance, "Input value for JsonEntityIdSerializer can't be null.");
        const EntityId* defaultEntityIdInstance = reinterpret_cast<const EntityId*>(defaultValue);

        JSR::ResultCode result(JSR::Tasks::WriteValue);
        {
            JsonEntityIdMapper** idMapper = context.GetMetadata().Find<JsonEntityIdMapper*>();

            // Store the id via a mapper if provided
            if (idMapper && *idMapper)
            {
                result.Combine((*idMapper)->MapIdToJson(outputValue, *entityIdInstance, context));
            }
            else
            {
                const AZ::u64* id = &entityIdInstance->m_id;
                const AZ::u64* defaultId = defaultEntityIdInstance ? &defaultEntityIdInstance->m_id : nullptr;

                AZ::ScopedContextPath subPathId(context, "m_id");
                result.Combine(ContinueStoringToJsonObjectField(outputValue, "id", id, defaultId, azrtti_typeid<AZ::u64>(), context));
            }
        }

        return context.Report(result,
            result.GetProcessing() == JSR::Processing::Completed ? "Succesfully stored Entity Id information." :
            "Failed to store Entity Id information.");
    }
}
