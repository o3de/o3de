/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/Json/JsonSerializationResult.h>
#include <Mesh/EditorMeshStatsSerializer.h>

namespace AZ
{
    namespace Render
    {
        AZ_CLASS_ALLOCATOR_IMPL(JsonEditorMeshStatsSerializer, AZ::SystemAllocator);

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
