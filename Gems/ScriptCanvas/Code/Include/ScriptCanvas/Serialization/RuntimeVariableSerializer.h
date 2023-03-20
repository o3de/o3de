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
    class RuntimeVariableSerializer
        : public BaseJsonSerializer
    {
    public:
        AZ_RTTI(RuntimeVariableSerializer, "{7E5FC193-8CDB-4251-A68B-F337027381DF}", BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR_DECL;

    private:
        JsonSerializationResult::Result Load
            ( void* outputValue
            , const Uuid& outputValueTypeId
            , const rapidjson::Value& inputValue
            , JsonDeserializerContext& context) override;

        JsonSerializationResult::Result Store
            ( rapidjson::Value& outputValue
            , const void* inputValue
            , const void* defaultValue
            , const Uuid& valueTypeId, JsonSerializerContext& context) override;
    };
}
