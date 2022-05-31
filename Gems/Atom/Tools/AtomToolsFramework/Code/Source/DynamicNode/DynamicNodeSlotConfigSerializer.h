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
#include <AzCore/std/any.h>

namespace AtomToolsFramework
{
    // Custom JSON serializer for DynamicNodeSlotConfig containing AZStd::any, which isn't natively supported by the system
    class JsonDynamicNodeSlotConfigSerializer : public AZ::BaseJsonSerializer
    {
    public:
        AZ_RTTI(JsonDynamicNodeSlotConfigSerializer, "{265FA2FE-FBD0-4A61-98CD-1D61AE48E221}", AZ::BaseJsonSerializer);
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

    private:
        template<typename T>
        bool LoadAny(
            AZStd::any& propertyValue,
            const rapidjson::Value& inputPropertyValue,
            AZ::JsonDeserializerContext& context,
            AZ::JsonSerializationResult::ResultCode& result);

        template<typename T>
        bool StoreAny(
            const AZStd::any& propertyValue,
            rapidjson::Value& outputPropertyValue,
            AZ::JsonSerializerContext& context,
            AZ::JsonSerializationResult::ResultCode& result);
    };
} // namespace AtomToolsFramework
