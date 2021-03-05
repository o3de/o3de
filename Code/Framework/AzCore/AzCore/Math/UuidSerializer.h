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
#include <AzCore/Memory/Memory.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/std/string/regex.h>

namespace AZ
{
    class JsonUuidSerializer
        : public BaseJsonSerializer
    {
    public:
        AZ_RTTI(JsonUuidSerializer, "{52D40D04-8B0D-44EA-A15D-92035C5F05E6}", BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR_DECL;

        struct MessageResult
        {
            AZStd::string_view m_message;
            JsonSerializationResult::ResultCode m_result;

            MessageResult(AZStd::string_view message, JsonSerializationResult::ResultCode result);
        };

        JsonUuidSerializer();

        JsonSerializationResult::Result Load(void* outputValue, const Uuid& outputValueTypeId,
            const rapidjson::Value& inputValue, JsonDeserializerContext& context) override;
        JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
            const Uuid& valueTypeId, JsonSerializerContext& context) override;

        //! Does the same as load, but doesn't report through the provided callback in the settings. Instead the final
        //! ResultCode and message are returned and it's up to the caller to report if need needed.
        MessageResult UnreportedLoad(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue);
        //! Does the same as store, but doesn't report through the provided callback in the settings. Instead the final
        //! ResultCode and message are returned and it's up to the caller to report if need needed.
        MessageResult UnreportedStore(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
            const Uuid& valueTypeId, JsonSerializerContext& context);

    private:
        AZStd::regex m_uuidFormat;
    };
} // namespace AZ
