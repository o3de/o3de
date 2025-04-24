/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Edit/Configuration.h>
#include <AzCore/Serialization/Json/BaseJsonSerializer.h>

namespace AZ
{
    class ReflectContext;

    namespace RPI
    {
        struct MaterialPropertySourceData;
        class MaterialPropertyValue;

        AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
        class ATOM_RPI_EDIT_API JsonMaterialPropertySerializer
            : public BaseJsonSerializer
        {
            AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING

        public:
            AZ_RTTI(JsonMaterialPropertySerializer, "{1AFEC5BD-AB2E-4BDB-911B-80727EDA0C26}", BaseJsonSerializer);
            AZ_CLASS_ALLOCATOR_DECL;
            JsonSerializationResult::Result Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
                JsonDeserializerContext& context) override;

            JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue,
                const void* defaultValue, const Uuid& valueTypeId, JsonSerializerContext& context) override;

        private:
            //! Loads a JSON value into AZStd::variant-based MaterialPropertyValue, using the template data type T
            template<typename T>
            JsonSerializationResult::ResultCode LoadVariant(MaterialPropertyValue& intoValue, const rapidjson::Value& inputValue, JsonDeserializerContext& context);
            template<typename T>
            JsonSerializationResult::ResultCode LoadVariant(MaterialPropertyValue& intoValue, const T& defaultValue, const rapidjson::Value& inputValue, JsonDeserializerContext& context);

            //! Loads a property's value fields from JSON, for use with numeric types like int and float, which support range limits.
            template<typename T>
            JsonSerializationResult::ResultCode LoadNumericValues(MaterialPropertySourceData* intoProperty, const T& defaultValue,
                const rapidjson::Value& inputValue, JsonDeserializerContext& context);

            //! Loads a property's value fields from JSON, for use with non-numeric types like Vector and Color, which don't support range limits.
            template<typename T>
            JsonSerializationResult::ResultCode LoadNonNumericValues(MaterialPropertySourceData* intoProperty, const T& defaultValue,
                const rapidjson::Value& inputValue, JsonDeserializerContext& context);

            JsonSerializationResult::ResultCode LoadVectorLabels(MaterialPropertySourceData* intoProperty,
                const rapidjson::Value& inputValue, JsonDeserializerContext& context);

            //! Stores a property's value fields to JSON, for use with numeric types like int and float, which support range limits.
            template<typename T>
            JsonSerializationResult::ResultCode StoreNumericValues(rapidjson::Value& outputValue,
                const MaterialPropertySourceData* property, const T& defaultValue, JsonSerializerContext& context);

            //! Stores a property's value fields to JSON, for use with non-numeric types like Vector and Color, which don't support range limits.
            template<typename T>
            JsonSerializationResult::ResultCode StoreNonNumericValues(rapidjson::Value& outputValue,
                const MaterialPropertySourceData* property, const T& defaultValue, JsonSerializerContext& context);

            JsonSerializationResult::ResultCode StoreVectorLabels(rapidjson::Value& outputValue,
                const MaterialPropertySourceData* property, JsonSerializerContext& context);
        };

    } // namespace RPI
} // namespace AZ
