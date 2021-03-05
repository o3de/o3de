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

#include <AzCore/JSON/document.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/std/utils.h>

namespace AZ
{
    struct Uuid;

    class JsonDeserializer final
    {
        friend class JsonSerialization;
        friend class BaseJsonSerializer;

    private:
        enum class ResolvePointerResult : bool
        {
            FullyProcessed,
            ContinueProcessing
        };
        enum class TypeIdDetermination : u8
        {
            ExplicitTypeId, // Type id was explicitly defined using "$type".
            ImplicitTypeId, // Type id was determined based on the provided function arguments.
            FailedToDetermine, // Type id couldn't be determined.
            FailedDueToMultipleTypeIds // Type id couldn't be determined because there were multiple options.
        };
        struct LoadTypeIdResult
        {
            Uuid m_typeId;
            TypeIdDetermination m_determination;
        };
        struct ElementDataResult
        {
            void* m_data{ nullptr };
            const SerializeContext::ClassElement* m_info{ nullptr };
            bool m_found{ false };
        };

        JsonDeserializer() = delete;
        ~JsonDeserializer() = delete;
        JsonDeserializer& operator=(const JsonDeserializer& rhs) = delete;
        JsonDeserializer& operator=(JsonDeserializer&& rhs) = delete;
        JsonDeserializer(const JsonDeserializer& rhs) = delete;
        JsonDeserializer(JsonDeserializer&& rhs) = delete;

        static JsonSerializationResult::ResultCode Load(void* object, const Uuid& typeId, const rapidjson::Value& value,
            JsonDeserializerContext& context);

        static JsonSerializationResult::ResultCode LoadToPointer(void* object, const Uuid& typeId, const rapidjson::Value& value,
            JsonDeserializerContext& context);

        static JsonSerializationResult::ResultCode LoadWithClassElement(void* object, const rapidjson::Value& value,
            const SerializeContext::ClassElement& classElement, JsonDeserializerContext& context);
        
        static JsonSerializationResult::ResultCode LoadClass(void* object, const SerializeContext::ClassData& classData, const rapidjson::Value& value,
            JsonDeserializerContext& context);

        static JsonSerializationResult::ResultCode LoadEnum(void* object, const SerializeContext::ClassData& classData, const rapidjson::Value& value,
            JsonDeserializerContext& context);

        template<typename UnderlyingType>
        static JsonSerializationResult::Result LoadUnderlyingEnumType(UnderlyingType& outputValue,
            const SerializeContext::ClassData& classData, const rapidjson::Value& inputValue, JsonDeserializerContext& context);

        template<typename UnderlyingType>
        static JsonSerializationResult::Result LoadEnumFromNumber(UnderlyingType& outputValue,
            const SerializeContext::ClassData& classData, const rapidjson::Value& inputValue, JsonDeserializerContext& context);

        template<typename UnderlyingType>
        static JsonSerializationResult::Result LoadEnumFromString(UnderlyingType& outputValue,
            const SerializeContext::ClassData& classData, const rapidjson::Value& inputValue, JsonDeserializerContext& context);

        template<typename UnderlyingType>
        static JsonSerializationResult::Result LoadEnumFromContainer(UnderlyingType& outputValue,
            const SerializeContext::ClassData& classData, const rapidjson::Value& inputValue, JsonDeserializerContext& context);

        static AZStd::string ReportAvailableEnumOptions(AZStd::string_view message,
            const AZStd::vector<AttributeSharedPair, AZStdFunctorAllocator>& attributes, bool signedValues);

        //! Fills out a pointer if needed based on the information in the provide json value. If FullyProcessed is returned
        //! there's no more information to process and if ContinueProcessing is returned the pointer needs to be further loaded.
        static ResolvePointerResult ResolvePointer(void** object, Uuid& objectType, JsonSerializationResult::ResultCode& status, const rapidjson::Value& pointerData,
            const AZ::IRttiHelper& rtti, JsonDeserializerContext& context);

        static LoadTypeIdResult LoadTypeIdFromJsonObject(const rapidjson::Value& node, const AZ::IRttiHelper& rtti, JsonDeserializerContext& context);
        static LoadTypeIdResult LoadTypeIdFromJsonString(const rapidjson::Value& node, const AZ::IRttiHelper* baseClassRtti, JsonDeserializerContext& context);
        static JsonSerializationResult::ResultCode LoadTypeId(Uuid& typeId, const rapidjson::Value& input, JsonDeserializerContext& context,
            const Uuid* baseTypeId = nullptr, bool* isExplicit = nullptr);

        //! Searches the class data matching typeId at object for a child element that matches nameCrc
        //! Sets the outSubclassTypeId to the found element data's typeId, and returns the offset pointer within storageClass to the found element.
        //! If nullptr is returned, outSubclassTypeId is invalid.
        static ElementDataResult FindElementByNameCrc(SerializeContext& serializeContext, void* object, 
            const SerializeContext::ClassData& classData, const Crc32 nameCrc);

        //! Counts the total number of elements that would be at the root of a json object.
        static size_t CountElements(SerializeContext& serializeContext, const SerializeContext::ClassData& classData);

        //! Checks if a value is an explicit default. This means the value is an object with no members.
        static bool IsExplicitDefault(const rapidjson::Value& value);
    };
} // namespace AZ
