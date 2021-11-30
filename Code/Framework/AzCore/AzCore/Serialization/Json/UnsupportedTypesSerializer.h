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

    class JsonBitsetSerializer : public JsonUnsupportedTypesSerializer
    {
    public:
        AZ_RTTI(JsonBitsetSerializer, "{10CE969D-D69E-4B3F-8593-069736F8F705}", JsonUnsupportedTypesSerializer);
        AZ_CLASS_ALLOCATOR_DECL;

    protected:
        AZStd::string_view GetMessage() const override;
    };
} // namespace AZ
