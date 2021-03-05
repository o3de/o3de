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

        JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
            const Uuid& valueTypeId, JsonSerializerContext& context) override;
    };
}
