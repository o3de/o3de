/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/Serialization/Json/BaseJsonSerializer.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    class JsonMapSerializer
        : public BaseJsonSerializer
    {
    public:
        AZ_RTTI(JsonMapSerializer, "{9D53A00C-EE25-4B39-A9C5-9EBAEB5CCF26}", BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR_DECL;
        
        JsonSerializationResult::Result Load(void* outputValue, const Uuid& outputValueTypeId,
            const rapidjson::Value& inputValue, JsonDeserializerContext& context) override;
        JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
            const Uuid& valueTypeId, JsonSerializerContext& context) override;

    protected:
        virtual JsonSerializationResult::Result LoadContainer(void* outputValue, const Uuid& outputValueTypeId,
            const rapidjson::Value& inputValue, JsonDeserializerContext& context);
        virtual JsonSerializationResult::Result LoadElement(void* outputValue, SerializeContext::IDataContainer* container, 
            const SerializeContext::ClassElement* pairElement, SerializeContext::IDataContainer* pairContainer, 
            const SerializeContext::ClassElement* keyElement, const SerializeContext::ClassElement* valueElement, 
            const rapidjson::Value& key, const rapidjson::Value& value, JsonDeserializerContext& context);
        
        virtual JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
            const Uuid& valueTypeId, JsonSerializerContext& context, bool sortResult);

        virtual bool CanBeConvertedToObject(const rapidjson::Value& outputValue);
    };

    class JsonUnorderedMapSerializer
        : public JsonMapSerializer
    {
    public:
        AZ_RTTI(JsonUnorderedMapSerializer, "{EF4478D3-1820-4FDB-A7B7-C9711EB41602}", JsonMapSerializer);
        AZ_CLASS_ALLOCATOR_DECL;

        using JsonMapSerializer::Store;
        JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
            const Uuid& valueTypeId, JsonSerializerContext& context) override;
    };

    class JsonUnorderedMultiMapSerializer
        : public JsonMapSerializer
    {
    public:
        AZ_RTTI(JsonUnorderedMultiMapSerializer, "{2D22FFE5-6AED-4D6B-8F69-05C757757037}", JsonMapSerializer);
        AZ_CLASS_ALLOCATOR_DECL;

        JsonSerializationResult::Result LoadElement(void* outputValue, SerializeContext::IDataContainer* container,
            const SerializeContext::ClassElement* pairElement, SerializeContext::IDataContainer* pairContainer,
            const SerializeContext::ClassElement* keyElement, const SerializeContext::ClassElement* valueElement,
            const rapidjson::Value& key, const rapidjson::Value& value, JsonDeserializerContext& context) override;

        using JsonMapSerializer::Store;
        JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
            const Uuid& valueTypeId, JsonSerializerContext& context) override;
    };
}
