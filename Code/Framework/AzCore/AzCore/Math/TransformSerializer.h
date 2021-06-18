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

#include <AzCore/Serialization/Json/BaseJsonSerializer.h>

namespace AZ
{
    class JsonTransformSerializer : public BaseJsonSerializer
    {
    public:
        AZ_RTTI(JsonTransformSerializer, "{51C321B8-9214-4E85-AA5C-B720428A3B17}", BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR_DECL;

        JsonSerializationResult::Result Load(
            void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
            JsonDeserializerContext& context) override;

        JsonSerializationResult::Result Store(
            rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue, const Uuid& valueTypeId,
            JsonSerializerContext& context) override;

        OperationFlags GetOperationsFlags() const override;
        
    private:
        // Note: These need to be defined as "const char[]" instead of "const char*" so that they can be implicitly converted
        // to a rapidjson::GenericStringRef<>.  (This also lets rapidjson get the string length at compile time)
        static inline constexpr const char TranslationTag[] = "Translation";
        static inline constexpr const char RotationTag[] = "Rotation";
        static inline constexpr const char ScaleTag[] = "Scale";
    };

} // namespace AZ
