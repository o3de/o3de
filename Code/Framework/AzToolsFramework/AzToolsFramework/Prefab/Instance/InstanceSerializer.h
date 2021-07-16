/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/Serialization/Json/BaseJsonSerializer.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class Instance;

        class JsonInstanceSerializer
            : public AZ::BaseJsonSerializer
        {
        public:

            AZ_RTTI(JsonInstanceSerializer, "{C255AE3E-844C-49A1-9519-09E53750D400}", BaseJsonSerializer);
            AZ_CLASS_ALLOCATOR_DECL;

            AZ::JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
                const AZ::Uuid& valueTypeId, AZ::JsonSerializerContext& context) override;

            AZ::JsonSerializationResult::Result Load(void* outputValue, const AZ::Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
                AZ::JsonDeserializerContext& context) override;

        private:
            //! Adds the entities of an instance to a InstanceEntityScrubber object in the metadata of JsonDeserializerContext
            //! so that they can be scrubbed later.
            void AddEntitiesToScrub(const Instance* instance, AZ::JsonDeserializerContext& jsonDeserializercontext);
        };
    }
}
