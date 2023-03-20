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

namespace AzToolsFramework::Components
{
    class JsonEditorEntitySortComponentSerializer
        : public AZ::BaseJsonSerializer
    {
    public:
        AZ_RTTI(JsonEditorEntitySortComponentSerializer, "{5104782E-B34F-4D87-B1DF-BDFB1AF20D58}", BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR_DECL;

        AZ::JsonSerializationResult::Result Load(
            void* outputValue, const AZ::Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
            AZ::JsonDeserializerContext& context) override;

        AZ::JsonSerializationResult::Result Store(
            rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue, const AZ::Uuid& valueTypeId,
            AZ::JsonSerializerContext& context) override;
    };
} // namespace AzToolsFramework::Components
