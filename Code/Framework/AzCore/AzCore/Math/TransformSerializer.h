/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
