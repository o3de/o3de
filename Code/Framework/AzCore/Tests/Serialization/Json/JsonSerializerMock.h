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
#include <gmock/gmock.h>

namespace JsonSerializationTests
{
    class JsonSerializerMock
        : public AZ::BaseJsonSerializer
    {
    public:
        AZ_CLASS_ALLOCATOR(JsonSerializerMock, AZ::SystemAllocator, 0);
        AZ_RTTI(JsonSerializerMock, "{9FC48652-A00B-4EFA-8FD9-345A8E625439}", BaseJsonSerializer);

        ~JsonSerializerMock() override = default;

        MOCK_METHOD4(Load, AZ::JsonSerializationResult::Result(void*, const AZ::Uuid&, const rapidjson::Value&,
            AZ::JsonDeserializerContext&));
        MOCK_METHOD5(Store, AZ::JsonSerializationResult::Result(rapidjson::Value&, const void*, const void*,
            const AZ::Uuid&, AZ::JsonSerializerContext& context));
    };
} //namespace JsonSerializationTests
