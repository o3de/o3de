/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetJsonSerializer.h>

namespace GraphCanvas
{
    class TranslationFormatSerializer
        : public AZ::BaseJsonSerializer
    {
    public:
        AZ_RTTI(TranslationFormatSerializer, "{DA2EE2D2-4BF9-430F-BDA2-41D8A7EF2B31}", BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR_DECL;
        AZ::JsonSerializationResult::Result Load(void* outputValue, const AZ::Uuid& outputValueTypeId,
            const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context) override;
        AZ::JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue,
            const void* defaultValue, const AZ::Uuid& valueTypeId, AZ::JsonSerializerContext& context) override;
    };

    namespace Schema
    {
        namespace Field
        {
            inline constexpr char key[] = "base";
            inline constexpr char context[] = "context";
            inline constexpr char variant[] = "variant";
            inline constexpr char entries[] = "entries";
        }
    }
}
