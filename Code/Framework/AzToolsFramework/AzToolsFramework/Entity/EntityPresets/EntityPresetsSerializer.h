/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/AzToolsFrameworkAPI.h>

#include <AzCore/Memory/Memory.h>
#include <AzCore/Serialization/Json/BaseJsonSerializer.h>

namespace AzToolsFramework
{
    namespace EntityPresets
    {
        //! Reads and writes a property assignment as one flat, hand-editable object:
        //!
        //!     { "path": "Controller|Configuration|Light type", "type": "int", "value": 1 }
        //!
        //! "value" is interpreted according to "type", since JSON alone cannot tell an integer from
        //! a float or a string from an asset path.
        class AZTF_API JsonPropertyAssignmentSerializer : public AZ::BaseJsonSerializer
        {
        public:
            AZ_RTTI(JsonPropertyAssignmentSerializer, "{A9EF03F4-6B32-4CC6-BD65-8570B386EF65}", AZ::BaseJsonSerializer);
            AZ_CLASS_ALLOCATOR_DECL;

            AZ::JsonSerializationResult::Result Load(
                void* outputValue,
                const AZ::Uuid& outputValueTypeId,
                const rapidjson::Value& inputValue,
                AZ::JsonDeserializerContext& context) override;

            AZ::JsonSerializationResult::Result Store(
                rapidjson::Value& outputValue,
                const void* inputValue,
                const void* defaultValue,
                const AZ::Uuid& valueTypeId,
                AZ::JsonSerializerContext& context) override;
        };
    } // namespace EntityPresets
} // namespace AzToolsFramework
