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

namespace AZ
{
    class EntityId;

    class JsonEntityIdSerializer
        : public BaseJsonSerializer
    {
    public:
        AZ_RTTI(JsonEntityIdSerializer, "{AEA75997-087C-4E23-8E4F-465A4142EC77}", BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR_DECL;

        class JsonEntityIdMapper
        {
        public:
            AZ_RTTI(JsonEntityIdMapper, "{8E139C95-827F-45B1-BCF0-F54F2D02C594}");

            virtual JsonSerializationResult::Result MapJsonToId(EntityId& outputValue, const rapidjson::Value& inputValue, JsonDeserializerContext& context) = 0;
            virtual JsonSerializationResult::Result MapIdToJson(rapidjson::Value& outputValue, const EntityId& inputValue, JsonSerializerContext& context) = 0;

            inline void SetIsEntityReference(bool isEntityReference) { m_isEntityReference = isEntityReference; }
            inline bool GetAcceptUnregisteredEntity() { return m_acceptUnregisteredEntity; }
            inline void SetAcceptUnregisteredEntity(bool acceptUnregisteredEntity) { m_acceptUnregisteredEntity = acceptUnregisteredEntity; }

        protected:
            bool m_isEntityReference = true;
            bool m_acceptUnregisteredEntity = false;
        };

        JsonSerializationResult::Result Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
            JsonDeserializerContext& context) override;

        JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
            const Uuid& valueTypeId, JsonSerializerContext& context) override;
    };
}
