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
#include <AzCore/Name/Name.h>
#include <AzCore/Serialization/Json/BaseJsonSerializer.h>

namespace AZ
{
    namespace Render
    {
        // Custom JSON serializer for material assignment objects containing AZStd::any property overrides,
        // which aren't supported by the system
        class JsonMaterialAssignmentSerializer : public BaseJsonSerializer
        {
        public:
            AZ_RTTI(JsonMaterialAssignmentSerializer, "{3D33653E-4582-483F-91F5-BBCC347C3DF0}", BaseJsonSerializer);
            AZ_CLASS_ALLOCATOR_DECL;

            JsonSerializationResult::Result Load(
                void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
                JsonDeserializerContext& context) override;

            JsonSerializationResult::Result Store(
                rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue, const Uuid& valueTypeId,
                JsonSerializerContext& context) override;

        private:
            template<typename T>
            bool LoadAny(
                AZStd::any& propertyValue, const rapidjson::Value& inputPropertyValue, AZ::JsonDeserializerContext& context,
                AZ::JsonSerializationResult::ResultCode& result);
            template<typename T>
            bool StoreAny(
                const AZStd::any& propertyValue, rapidjson::Value& outputPropertyValue, AZ::JsonSerializerContext& context,
                AZ::JsonSerializationResult::ResultCode& result);
        };
    } // namespace Render
} // namespace AZ
