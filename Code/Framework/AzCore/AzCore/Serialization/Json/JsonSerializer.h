/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/JSON/document.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>

namespace AZ
{
    class JsonSerializerContext;

    class JsonSerializer final
    {
        friend class JsonSerialization;
        friend class BaseJsonSerializer;
    private:
        enum class StoreTypeId : bool
        {
            No,
            Yes
        };
        enum class UseTypeSerializer : bool
        {
            No,
            Yes
        };
        enum class ResolvePointerResult
        {
            FullyProcessed,
            WriteNull,
            ContinueProcessing
        };

        JsonSerializer() = delete;
        ~JsonSerializer() = delete;
        JsonSerializer& operator=(const JsonSerializer& rhs) = delete;
        JsonSerializer& operator=(JsonSerializer&& rhs) = delete;
        JsonSerializer(const JsonSerializer& rhs) = delete;
        JsonSerializer(JsonSerializer&& rhs) = delete;

        static JsonSerializationResult::ResultCode Store(rapidjson::Value& output, const void* object, const void* defaultObject,
            const Uuid& typeId, UseTypeSerializer useCustom, JsonSerializerContext& context);

        static JsonSerializationResult::ResultCode StoreFromPointer(rapidjson::Value& output, const void* object, const void* defaultObject,
            const Uuid& typeId, UseTypeSerializer custom, JsonSerializerContext& context);

        static JsonSerializationResult::ResultCode StoreWithClassData(rapidjson::Value& node, const void* object, const void* defaultObject,
            const SerializeContext::ClassData& classData, StoreTypeId storeTypeId, UseTypeSerializer custom,
            JsonSerializerContext& context);

        static JsonSerializationResult::ResultCode StoreWithClassDataFromPointer(rapidjson::Value& output, const void* object,
            const void* defaultObject, const SerializeContext::ClassData& classData, UseTypeSerializer custom,
            JsonSerializerContext& context);

        static JsonSerializationResult::ResultCode StoreWithClassElement(rapidjson::Value& parentNode, const void* object,
            const void* defaultObject, const SerializeContext::ClassElement& classElement, JsonSerializerContext& context);

        static JsonSerializationResult::ResultCode StoreClass(rapidjson::Value& output, const void* object, const void* defaultObject,
            const SerializeContext::ClassData& classData, JsonSerializerContext& context);

        static JsonSerializationResult::ResultCode StoreEnum(rapidjson::Value& output, const void* object, const void* defaultObject,
            const SerializeContext::ClassData& classData, JsonSerializerContext& context);

        template<typename UnderlyingType>
        static JsonSerializationResult::Result StoreUnderlyingEnumType(rapidjson::Value& outputValue, const UnderlyingType* value,
            const UnderlyingType* defaultValue, const SerializeContext::ClassData& classData, JsonSerializerContext& context);

        //! Extracts information on the provided pointer and moves it to the content of the pointer. If FullyProcessed is returned 
        //! there's no more information to process and if ContinueProcessing is returned the elementClassData will be updated to 
        //! the element the pointer was pointing to and storeTypeId will indicate if the type needs to be explicitly written.
        static ResolvePointerResult ResolvePointer(JsonSerializationResult::ResultCode& status, StoreTypeId& storeTypeId,
            const void*& object, const void*& defaultObject, AZStd::any& defaultObjectStorage,
            const SerializeContext::ClassData*& elementClassData, const AZ::IRttiHelper& rtti,  JsonSerializerContext& context);

        static rapidjson::Value StoreTypeName(const SerializeContext::ClassData& classData, JsonSerializerContext& context);
        static JsonSerializationResult::ResultCode StoreTypeName(rapidjson::Value& output,
            const Uuid& typeId, JsonSerializerContext& context);

        static JsonSerializationResult::ResultCode InsertTypeId(
            rapidjson::Value& output, const SerializeContext::ClassData& classData, JsonSerializerContext& context);

        static rapidjson::Value GetExplicitDefault();
    };
} // namespace AZ
