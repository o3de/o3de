/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI.Reflect/PhysicalDeviceDescriptor.h>
#include <AzCore/Serialization/Json/BaseJsonSerializer.h>

namespace AZ::RHI
{
    // Used to serialize or deserialize PhysicalDeviceDriverInfo.
    class JsonPhysicalDeviceDriverInfoSerializer : public BaseJsonSerializer
    {
    public:
        AZ_RTTI(AZ::RHI::JsonPhysicalDeviceDriverInfoSerializer, "{E7C48E1F-7483-4451-897D-34472611E824}", BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR_DECL;

        JsonSerializationResult::Result Load(
            void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
            JsonDeserializerContext& context) override;

        JsonSerializationResult::Result Store(
            rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue, const Uuid& valueTypeId,
            JsonSerializerContext& context) override;
    };
}
