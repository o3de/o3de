/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/Serialization/Json/BaseJsonSerializer.h>

namespace AzToolsFramework
{
    namespace Components
    {
        class JsonTransformComponentSerializer
            : public AZ::BaseJsonSerializer
        {
        public:
            AZ_RTTI(JsonTransformComponentSerializer, "{F8BA0E22-1DD5-4BCC-A371-0988F8815CF4}", BaseJsonSerializer);
            AZ_CLASS_ALLOCATOR_DECL;

            AZ::JsonSerializationResult::Result Load(
                void* outputValue, const AZ::Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
                AZ::JsonDeserializerContext& context) override;

            AZ::JsonSerializationResult::Result Store(
                rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue, const AZ::Uuid& valueTypeId,
                AZ::JsonSerializerContext& context) override;
        };

    } // namespace Components
} // namespace AzToolsFramework
