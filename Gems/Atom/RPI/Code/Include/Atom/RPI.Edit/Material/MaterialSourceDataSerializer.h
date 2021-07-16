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
        //! This custom serializer is needed to load the material type file and saves its data in the
        //! JsonDeserializerSettings for JsonMaterialPropertyValueSerializer to use.
        //! (Note we could have made a custom serializer specifically for the 'materialType' field but that
        //! would require 'materialType' to appear before 'properties'. By having a custom serializer for the common 
        //! parent of 'materialType' and 'properties', we can avoid an order dependency within the JSON file).
        class JsonMaterialSourceDataSerializer
            : public BaseJsonSerializer
        {
        public:
            AZ_RTTI(AZ::RPI::JsonMaterialSourceDataSerializer, "{008A7423-8DF6-4BA3-BF5E-B0C189CCBE58}", BaseJsonSerializer);
            AZ_CLASS_ALLOCATOR_DECL;
            
            JsonSerializationResult::Result Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
                JsonDeserializerContext& context) override;

            JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue,
                const void* defaultValue, const Uuid& valueTypeId, JsonSerializerContext& context) override;
        };

    } // namespace RPI
} // namespace AZ
