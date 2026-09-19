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
        //! Reads and writes a property assignment as one flat object.
        //!
        //! Reflection on its own would nest the value inside the assignment:
        //!
        //!     { "path": "Controller|Configuration|Light type", "value": { "type": "int", ... } }
        //!
        //! whereas the format is flat, and stays flat:
        //!
        //!     { "path": "Controller|Configuration|Light type", "type": "int", "value": 1 }
        //!
        //! Two reasons to keep it that way. Existing project preset files are already written like
        //! this and should not need migrating. And the file is meant to be opened and edited by
        //! hand - that is most of the point of storing presets as data - so the shape that reads
        //! naturally wins over the shape that falls out of the reflection.
        //!
        //! The "value" member is typed by the sibling "type" member rather than by its own JSON
        //! type, because a JSON number cannot say whether it meant an integer or a float, and a
        //! JSON string cannot say whether it meant a string or an asset path.
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
