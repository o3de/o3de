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
