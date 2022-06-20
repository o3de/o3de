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
    namespace Render
    {
        // JsonEditorMaterialComponentSerializer skips serialization of EditorMaterialComponentSlot(s) which are only needed at runtime in
        // the editor
        class JsonEditorMaterialComponentSerializer : public AZ::BaseJsonSerializer
        {
        public:
            AZ_RTTI(JsonEditorMaterialComponentSerializer, "{D354FE3C-34D2-4E80-B3F9-49450D252336}", BaseJsonSerializer);
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

    } // namespace Render
} // namespace AZ
