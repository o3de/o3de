/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        AZ_CLASS_ALLOCATOR(JsonSerializerMock, AZ::SystemAllocator);
        AZ_RTTI(JsonSerializerMock, "{9FC48652-A00B-4EFA-8FD9-345A8E625439}", BaseJsonSerializer);

        ~JsonSerializerMock() override = default;

        MOCK_METHOD4(Load, AZ::JsonSerializationResult::Result(void*, const AZ::Uuid&, const rapidjson::Value&,
            AZ::JsonDeserializerContext&));
        MOCK_METHOD5(Store, AZ::JsonSerializationResult::Result(rapidjson::Value&, const void*, const void*,
            const AZ::Uuid&, AZ::JsonSerializerContext& context));
    };
} //namespace JsonSerializationTests
