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
#include <AzCore/std/containers/vector.h>

namespace AZ
{
    using JsonByteStream = AZStd::vector<AZ::u8>; //!< Alias for AZStd::vector<AZ::u8>.

    //! Serialize a stream of bytes (usually binary data) as a json string value.
    //! @note Related to GenericClassByteStream (part of SerializeGenericTypeInfo<AZStd::vector<AZ::u8>> - see AZStdContainers.inl for more
    //! details).
    class JsonByteStreamSerializer : public BaseJsonSerializer
    {
    public:
        AZ_RTTI(JsonByteStreamSerializer, "{30F0EA5A-CD13-4BA7-BAE1-D50D851CAC45}", BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR_DECL;

        JsonSerializationResult::Result Load(
            void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
            JsonDeserializerContext& context) override;
        JsonSerializationResult::Result Store(
            rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue, const Uuid& valueTypeId,
            JsonSerializerContext& context) override;
    };
} // namespace AZ
