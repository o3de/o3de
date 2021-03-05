/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/Serialization/Json/BaseJsonSerializer.h>

namespace AZ
{
    class JsonColorSerializer
        : public BaseJsonSerializer
    {
    public:
        AZ_RTTI(JsonColorSerializer, "{4C0CF19D-7F92-459A-88C3-47C07BE8BD45}", BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR_DECL;
        JsonSerializationResult::Result Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
            JsonDeserializerContext& context) override;
        JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
            const Uuid& valueTypeId, JsonSerializerContext& context) override;

    private:
        enum class LoadAlpha
        {
            Yes,
            No,
            IfAvailable
        };

        JsonSerializationResult::Result LoadObject(Color& output, const rapidjson::Value& inputValue,
            JsonDeserializerContext& context);
        JsonSerializationResult::Result LoadFloatArray(Color& output, const rapidjson::Value& inputValue, LoadAlpha loadAlpha,
            JsonDeserializerContext& context);
        JsonSerializationResult::Result LoadByteArray(Color& output, const rapidjson::Value& inputValue, bool loadAlpha,
            JsonDeserializerContext& context);
        JsonSerializationResult::Result LoadHexString(Color& output, const rapidjson::Value& inputValue, bool loadAlpha,
            JsonDeserializerContext& context);
        uint32_t ClampToByteColorRange(int64_t value) const;
    };
}
