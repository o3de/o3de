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
    class JsonEntitySerializer
        : public BaseJsonSerializer
    {
    public:
        AZ_RTTI(JsonEntitySerializer, "{015BBF46-E21A-41AA-816A-C63119FB2852}", BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR_DECL;

        JsonSerializationResult::Result Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
            JsonDeserializerContext& context) override;

        JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue, const Uuid& valueTypeId,
            JsonSerializerContext& context) override;

    private:
        void ConvertComponentVectorToMap(const AZ::Entity::ComponentArrayType& components,
            AZStd::unordered_map<AZStd::string, AZ::Component*>& componentMapOut);
    };
}
