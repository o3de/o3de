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

namespace AZ
{
    namespace Render
    {
        // Custom JSON serializer for Mesh Stats
        // Mesh stats should not be serialized with EditorMeshComponent, because they are calculated based on active model,
        // so the serializer simply provides empty load/store functions
        class JsonEditorMeshStatsSerializer : public BaseJsonSerializer
        {
        public:
            AZ_RTTI(JsonEditorMeshStatsSerializer, "{571290F5-98C9-45AE-BC8C-E40968B5BA6F}", BaseJsonSerializer);
            AZ_CLASS_ALLOCATOR_DECL;

            JsonSerializationResult::Result Load(
                void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
                JsonDeserializerContext& context) override;

            JsonSerializationResult::Result Store(
                rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue, const Uuid& valueTypeId,
                JsonSerializerContext& context) override;            
        };
    } // namespace Render
} // namespace AZ
