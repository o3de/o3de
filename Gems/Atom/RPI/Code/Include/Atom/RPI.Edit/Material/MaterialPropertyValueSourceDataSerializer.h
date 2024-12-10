/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Edit/Configuration.h>
#include <Atom/RPI.Edit/Material/MaterialPropertyValueSourceData.h>
#include <AzCore/Serialization/Json/BaseJsonSerializer.h>


namespace AZ
{
    class ReflectContext;

    namespace RPI
    {
        AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
        class ATOM_RPI_EDIT_API JsonMaterialPropertyValueSourceDataSerializer
            : public BaseJsonSerializer
        {
            AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING

        public:
            AZ_RTTI(AZ::RPI::JsonMaterialPropertyValueSourceDataSerializer, "{940D9CA4-8BA5-4747-A566-3ED92CD217F7}", BaseJsonSerializer);
            AZ_CLASS_ALLOCATOR_DECL;

            JsonSerializationResult::Result Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
                JsonDeserializerContext& context) override;

            JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue,
                const void* defaultValue, const Uuid& valueTypeId, JsonSerializerContext& context) override;

        private:
            template<typename T>
            bool LoadAny(MaterialPropertyValueSourceData& propertyValue, const T& defaultValue,
                const rapidjson::Value& inputValue, JsonDeserializerContext& context);
        };

    } // namespace RPI
} // namespace AZ
