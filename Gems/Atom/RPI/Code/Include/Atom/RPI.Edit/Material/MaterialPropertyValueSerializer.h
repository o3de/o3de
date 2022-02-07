/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Edit/Material/MaterialSourceData.h>
#include <AzCore/Serialization/Json/BaseJsonSerializer.h>

namespace AZ
{
    class ReflectContext;

    namespace RPI
    {
        class JsonMaterialPropertyValueSerializer
            : public BaseJsonSerializer
        {
        public:
            AZ_RTTI(AZ::RPI::JsonMaterialPropertyValueSerializer, "{A52B1ED8-C849-4269-9AA7-9D0814D2EC59}", BaseJsonSerializer);
            AZ_CLASS_ALLOCATOR_DECL;

            //! A LoadContext object must be passed down to the serializer via JsonDeserializerContext::GetMetadata().Add(...)
            struct LoadContext
            {
                AZ_TYPE_INFO(JsonMaterialPropertyValueSerializer::LoadContext, "{5E0A891A-27F6-4AD7-88A5-B9EA50F88B45}");
                uint32_t m_materialTypeVersion; //!< The version number from the .materialtype file
            };

            JsonSerializationResult::Result Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
                JsonDeserializerContext& context) override;

            JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue,
                const void* defaultValue, const Uuid& valueTypeId, JsonSerializerContext& context) override;

        private:
            //! Loads a JSON value into AZStd::variant-based MaterialPropertyValue, using the template data type T
            template<typename T>
            JsonSerializationResult::ResultCode LoadVariant(MaterialPropertyValue& intoValue, const T& defaultValue, const rapidjson::Value& inputValue, JsonDeserializerContext& context);
        };

    } // namespace RPI
} // namespace AZ
