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
#include <AzCore/std/string/string_view.h>

namespace AZ
{
    class JsonUnsupportedTypesSerializer : public BaseJsonSerializer
    {
    public:
        AZ_RTTI(JsonUnsupportedTypesSerializer, "{AFCC76B9-1F28-429D-8B4E-020BFD95ADAC}", BaseJsonSerializer);
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

    protected:
        virtual AZStd::string_view GetMessage() const = 0;
    };

    class JsonAnySerializer : public JsonUnsupportedTypesSerializer
    {
    public:
        AZ_RTTI(JsonAnySerializer, "{699A0864-C4E2-4266-8141-99793C76870F}", JsonUnsupportedTypesSerializer);
        AZ_CLASS_ALLOCATOR_DECL;

    protected:
        AZStd::string_view GetMessage() const override;
    };

    class JsonVariantSerializer : public JsonUnsupportedTypesSerializer
    {
    public:
        AZ_RTTI(JsonVariantSerializer, "{08F8E746-F8A4-4E83-8902-713E90F3F498}", JsonUnsupportedTypesSerializer);
        AZ_CLASS_ALLOCATOR_DECL;

    protected:
        AZStd::string_view GetMessage() const override;
    };

    class JsonOptionalSerializer : public JsonUnsupportedTypesSerializer
    {
    public:
        AZ_RTTI(JsonOptionalSerializer, "{F8AF1C95-BD1B-44D2-9B4A-F5726133A104}", JsonUnsupportedTypesSerializer);
        AZ_CLASS_ALLOCATOR_DECL;

    protected:
        AZStd::string_view GetMessage() const override;
    };
} // namespace AZ
