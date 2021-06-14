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

#include <AzCore/Serialization/Json/JsonSerializationResult.h>
#include <Mesh/EditorMeshStatsSerializer.h>

namespace AZ
{
    namespace Render
    {
        AZ_CLASS_ALLOCATOR_IMPL(JsonEditorMeshStatsSerializer, AZ::SystemAllocator, 0);

        JsonSerializationResult::Result JsonEditorMeshStatsSerializer::Load(
            [[maybe_unused]] void* outputValue,
            [[maybe_unused]] const Uuid& outputValueTypeId,
            [[maybe_unused]] const rapidjson::Value& inputValue,
            JsonDeserializerContext& context)
        {
            namespace JSR = JsonSerializationResult;

            JSR::ResultCode result(JSR::Tasks::ReadField);

            return context.Report(
                result,
                "Successfully loaded EditorMeshStats information.");
        }

        JsonSerializationResult::Result JsonEditorMeshStatsSerializer::Store(
            [[maybe_unused]] rapidjson::Value& outputValue,
            [[maybe_unused]] const void* inputValue,
            [[maybe_unused]] const void* defaultValue,
            [[maybe_unused]] const Uuid& valueTypeId,
            JsonSerializerContext& context)
        {
            namespace JSR = JsonSerializationResult;

            JSR::ResultCode result(JSR::Tasks::WriteValue);
            
            return context.Report(
                result,
                "Successfully stored EditorMeshStats information.");
        }
        
    } // namespace Render
} // namespace AZ
