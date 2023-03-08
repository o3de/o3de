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
    class ScriptPropertySerializer
        : public BaseJsonSerializer
    {
    public:
        AZ_RTTI(ScriptPropertySerializer, "{C7BECA49-84EF-45E6-A89D-052D61766197}", BaseJsonSerializer);
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
