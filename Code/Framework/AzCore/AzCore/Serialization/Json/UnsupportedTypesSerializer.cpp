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

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/Json/UnsupportedTypesSerializer.h>

namespace AZ
{
    AZ_CLASS_ALLOCATOR_IMPL(JsonUnsupportedTypesSerializer, SystemAllocator, 0);
    AZ_CLASS_ALLOCATOR_IMPL(JsonAnySerializer, SystemAllocator, 0);
    AZ_CLASS_ALLOCATOR_IMPL(JsonVariantSerializer, SystemAllocator, 0);
    AZ_CLASS_ALLOCATOR_IMPL(JsonOptionalSerializer, SystemAllocator, 0);

    JsonSerializationResult::Result JsonUnsupportedTypesSerializer::Load(void*, const Uuid&, const rapidjson::Value&,
        JsonDeserializerContext& context)
    {
        namespace JSR = JsonSerializationResult;
        return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Invalid, GetMessage());
    }

    JsonSerializationResult::Result JsonUnsupportedTypesSerializer::Store(rapidjson::Value&, const void*, const void*, const Uuid&,
        JsonSerializerContext& context)
    {
        namespace JSR = JsonSerializationResult;
        return context.Report(JSR::Tasks::WriteValue, JSR::Outcomes::Invalid, GetMessage());
    }

    AZStd::string_view JsonAnySerializer::GetMessage() const
    {
        return "The Json Serialization doesn't support AZStd::any by design. The Json Serialization attempts to minimize the use of $type, "
               "in particular the guid version, but no way has yet been found to use AZStd::any without explicitly and always requiring "
               "one.";
    }

    AZStd::string_view JsonVariantSerializer::GetMessage() const
    {
        return "The Json Serialization doesn't support AZStd::variant by design. The Json Serialization attempts to minimize the use of "
               "$type, in particular the guid version. While combinations of AZStd::variant can be constructed that don't require a $type, "
               "this cannot be guaranteed in general.";
    }

    AZStd::string_view JsonOptionalSerializer::GetMessage() const
    {
        return "The Json Serialization doesn't support AZStd::optional by design. No JSON format has yet been found that wasn't deemed too "
               "complex or overly verbose.";
    }
} // namespace AZ
