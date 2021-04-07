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
}
