/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory_fwd.h>
#include <AzCore/RTTI/RTTIMacros.h>
#include <AzCore/Serialization/Json/BaseJsonSerializer.h>

namespace AZ
{
    //! JSON serializer for PointerObject
    //! This is only used for marshaling a pointer to/from a Dom Value in-meory
    class PointerJsonSerializer : public BaseJsonSerializer
    {
    public:
        AZ_TYPE_INFO_WITH_NAME_DECL(PointerJsonSerializer);
        AZ_RTTI_NO_TYPE_INFO_DECL();
        AZ_CLASS_ALLOCATOR_DECL;

        JsonSerializationResult::Result Load(
            void* outputValue,
            const Uuid& outputValueTypeId,
            const rapidjson::Value& inputValue,
            JsonDeserializerContext& context) override;
        JsonSerializationResult::Result Store(
            rapidjson::Value& outputValue,
            const void* inputValue,
            const void* defaultValue,
            const Uuid& valueTypeId,
            JsonSerializerContext& context) override;

        BaseJsonSerializer::OperationFlags GetOperationsFlags() const override;
    };
} // namespace AZ
