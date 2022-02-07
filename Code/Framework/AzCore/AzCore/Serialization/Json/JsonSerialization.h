/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/JSON/document.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Serialization/Json/JsonSerializationSettings.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>

namespace AZ
{
    class BaseJsonSerializer;

    struct JsonImportSettings;
    
    enum class JsonMergeApproach
    {
        JsonPatch, //!< Uses JSON Patch to merge two json documents. See https://tools.ietf.org/html/rfc6902
        JsonMergePatch //!< Uses JSON Merge Patch to merge two json documents. See https://tools.ietf.org/html/rfc7386
    };

    enum class JsonSerializerCompareResult
    {
        Less,
        Equal,
        Greater,
        Error
    };
    
    //! Core class to handle serialization to and from json documents.
    //! The Json Serialization works by taking a default constructed object and then apply the information found in the JSON document
    //! on top of that object. This allows the Json Serialization to avoid storing default values and helps guarantee that the final
    //! object is in a valid state even if non-fatal issues are encountered.
    //! Note on containers: Containers such as vector or map are always considered to be empty even if there's entries in the provided
    //!     default object. During deserialization entries will be appended to any existing values. A flag is provided to automatically
    //!     clear containers during deserialization.
    //! Note on maps: If the key for map containers such as unordered_map can be interpret as a string the Json Serialization will use
    //!     a JSON Object to store the data in instead of an array with key/value objects.
    //! Note on pointers: The Json Serialization assumes that are always constructed, so a default JSON value of "{}" is interpret as
    //!     creating a new default instance even if the default value is a null pointer. A JSON Null needs to be explicitly stored in
    //!     the JSON Document in order to default or explicitly set a pointer to null.
    //! Note on pointer memory: Objects created/destroyed by the Json Serialization for pointers require that the AZ_CLASS_ALLOCATOR is
    //!     declared and the object is created using aznew or memory is allocated using azmalloc. Without these the application may
    //!     crash if the Json Serialization tries to create or destroy an object pointed to by a pointer.
    class JsonSerialization final
    {
    public:
        static constexpr const char* TypeIdFieldIdentifier = "$type";
        static constexpr const char* DefaultStringIdentifier = "{}";
        static constexpr const char* KeyFieldIdentifier = "Key";
        static constexpr const char* ValueFieldIdentifier = "Value";
        static constexpr const char* ImportDirectiveIdentifier = "$import";

        //! Merges two json values together by applying "patch" to "target" using the selected merge algorithm.
        //! This version of ApplyPatch is destructive to "target". If the patch can't be correctly applied it will
        //! leave target in a partially patched state. Use the other version of ApplyPatch if target should be copied.
        //! @param target The value where the patch will be applied to.
        //! @param allocator The allocator associated with the document that holds the target.
        //! @param patch The value holding the patch information.
        //! @param approach The merge algorithm that will be used to apply the patch on top of the target.
        //! @param settings Optional additional settings to control the way the patch is applied.
        static JsonSerializationResult::ResultCode ApplyPatch(rapidjson::Value& target, rapidjson::Document::AllocatorType& allocator,
            const rapidjson::Value& patch, JsonMergeApproach approach, const JsonApplyPatchSettings& settings = JsonApplyPatchSettings{});
        //! Merges two json values together by applying "patch" to "target" using the selected merge algorithm.
        //! This version of ApplyPatch is destructive to "target". If the patch can't be correctly applied it will
        //! leave target in a partially patched state. Use the other version of ApplyPatch if target should be copied.
        //! @param target The value where the patch will be applied to.
        //! @param allocator The allocator associated with the document that holds the target.
        //! @param patch The value holding the patch information.
        //! @param approach The merge algorithm that will be used to apply the patch on top of the target.
        //! @param settings Additional settings to control the way the patch is applied.
        static JsonSerializationResult::ResultCode ApplyPatch(
            rapidjson::Value& target, rapidjson::Document::AllocatorType& allocator, const rapidjson::Value& patch,
            JsonMergeApproach approach, JsonApplyPatchSettings& settings);

        //! Merges two json values together by applying "patch" to a copy of "output" and written to output using the
        //! selected merge algorithm. This version of ApplyPatch is non-destructive to "source". If the patch couldn't be
        //! fully applied "output" will be left set to an empty (default) object.
        //! @param source A copy of source with the patch applied to it or an empty object if the patch couldn't be applied.
        //! @param allocator The allocator associated with the document that holds the source.
        //! @param target The value where the patch will be applied to.
        //! @param patch The value holding the patch information.
        //! @param approach The merge algorithm that will be used to apply the patch on top of the target.
        //! @param settings Optional additional settings to control the way the patch is applied.
        static JsonSerializationResult::ResultCode ApplyPatch(rapidjson::Value& output, rapidjson::Document::AllocatorType& allocator,
            const rapidjson::Value& source, const rapidjson::Value& patch, JsonMergeApproach approach,
            const JsonApplyPatchSettings& settings = JsonApplyPatchSettings{});
        //! Merges two json values together by applying "patch" to a copy of "output" and written to output using the
        //! selected merge algorithm. This version of ApplyPatch is non-destructive to "source". If the patch couldn't be
        //! fully applied "output" will be left set to an empty (default) object.
        //! @param source A copy of source with the patch applied to it or an empty object if the patch couldn't be applied.
        //! @param allocator The allocator associated with the document that holds the source.
        //! @param target The value where the patch will be applied to.
        //! @param patch The value holding the patch information.
        //! @param approach The merge algorithm that will be used to apply the patch on top of the target.
        //! @param settings Additional settings to control the way the patch is applied.
        static JsonSerializationResult::ResultCode ApplyPatch(
            rapidjson::Value& output, rapidjson::Document::AllocatorType& allocator, const rapidjson::Value& source,
            const rapidjson::Value& patch, JsonMergeApproach approach, JsonApplyPatchSettings& settings);

        //! Creates a patch using the selected merge algorithm such that when applied to source it results in target.
        //! @param patch The value containing the differences between source and target.
        //! @param allocator The allocator associated with the document that will hold the patch.
        //! @param source The value used as a starting point.
        //! @param target The value that will result if the patch is applied to the source.
        //! @param approach The algorithm that will be used when the patch is applied to the source.
        //! @param settings Optional additional settings to control the way the patch is created.
        static JsonSerializationResult::ResultCode CreatePatch(rapidjson::Value& patch, rapidjson::Document::AllocatorType& allocator,
            const rapidjson::Value& source, const rapidjson::Value& target, JsonMergeApproach approach,
            const JsonCreatePatchSettings& settings = JsonCreatePatchSettings{});
        //! Creates a patch using the selected merge algorithm such that when applied to source it results in target.
        //! @param patch The value containing the differences between source and target.
        //! @param allocator The allocator associated with the document that will hold the patch.
        //! @param source The value used as a starting point.
        //! @param target The value that will result if the patch is applied to the source.
        //! @param approach The algorithm that will be used when the patch is applied to the source.
        //! @param settings Additional settings to control the way the patch is created.
        static JsonSerializationResult::ResultCode CreatePatch(
            rapidjson::Value& patch, rapidjson::Document::AllocatorType& allocator, const rapidjson::Value& source,
            const rapidjson::Value& target, JsonMergeApproach approach, JsonCreatePatchSettings& settings);

        //! Loads the data from the provided json value into the supplied object. The object is expected to be created before calling load.
        //! @param object Object where the data will be loaded into.
        //! @param root The Value or Document where the deserializer will start reading data from.
        //! @param settings Optional additional settings to control the way document is deserialized.
        template<typename T>
        static JsonSerializationResult::ResultCode Load(
            T& object, const rapidjson::Value& root, const JsonDeserializerSettings& settings = JsonDeserializerSettings{});
        //! Loads the data from the provided json value into the supplied object. The object is expected to be created before calling load.
        //! @param object Object where the data will be loaded into.
        //! @param root The Value or Document where the deserializer will start reading data from.
        //! @param settings Additional settings to control the way document is deserialized.
        template<typename T>
        static JsonSerializationResult::ResultCode Load(T& object, const rapidjson::Value& root, JsonDeserializerSettings& settings);
        //! Loads the data from the provided json value into the supplied object. The object is expected to be created before calling load.
        //! @param object Pointer to the object where the data will be loaded into.
        //! @param objectType Type id of the object passed in.
        //! @param root The Value or Document from where the deserializer will start reading data.
        //! @param settings Optional additional settings to control the way document is deserialized.
        static JsonSerializationResult::ResultCode Load(
            void* object, const Uuid& objectType, const rapidjson::Value& root,
            const JsonDeserializerSettings& settings = JsonDeserializerSettings{});
        //! Loads the data from the provided json value into the supplied object. The object is expected to be created before calling load.
        //! @param object Pointer to the object where the data will be loaded into.
        //! @param objectType Type id of the object passed in.
        //! @param root The Value or Document from where the deserializer will start reading data.
        //! @param settings Additional settings to control the way document is deserialized.
        static JsonSerializationResult::ResultCode Load(
            void* object, const Uuid& objectType, const rapidjson::Value& root, JsonDeserializerSettings& settings);

        //! Loads the type id from the provided input.
        //! Note: it's not recommended to use this function (frequently) as it requires users of the json file to have knowledge of the internal
        //!     type structure and is therefore harder to use.
        //! @param typeId The uuid where the loaded data will be written to. If loading fails this will be a null uuid.
        //! @param input The json node to load from. The node is expected to contain a string.
        //! @param baseClassTypeId. An optional type id for the base class, if known. If a type name is stored in the string which
        //!     references multiple types then the baseClassTypeId will be used to disambiguate between the different types by looking
        //!     if exactly one of the types inherits from the base class that baseClassTypeId points to.
        //! @param jsonPath An optional path to the json node. This will be used for reporting.
        //! @param settings Optional settings object to change where this function collects information from. This can be same settings
        //!     as used for the other Load functions.
        static JsonSerializationResult::ResultCode LoadTypeId(Uuid& typeId, const rapidjson::Value& input, 
            const Uuid* baseClassTypeId = nullptr, AZStd::string_view jsonPath = AZStd::string_view{}, 
            const JsonDeserializerSettings& settings = JsonDeserializerSettings{});
        //! Loads the type id from the provided input.
        //! Note: it's not recommended to use this function (frequently) as it requires users of the json file to have knowledge of the
        //! internal type structure and is therefore harder to use.
        //! @param typeId The uuid where the loaded data will be written to. If loading fails this will be a null uuid.
        //! @param input The json node to load from. The node is expected to contain a string.
        //! @param baseClassTypeId. An optional type id for the base class, if known. If a type name is stored in the string which
        //!     references multiple types then the baseClassTypeId will be used to disambiguate between the different types by looking
        //!     if exactly one of the types inherits from the base class that baseClassTypeId points to.
        //! @param jsonPath An optional path to the json node. This will be used for reporting.
        //! @param settings Settings object to change where this function collects information from. This can be same settings
        //!     as used for the other Load functions.
        static JsonSerializationResult::ResultCode LoadTypeId(
            Uuid& typeId, const rapidjson::Value& input, const Uuid* baseClassTypeId,
            AZStd::string_view jsonPath, JsonDeserializerSettings& settings);

        //! Stores the data in the provided object as json values starting at the provided value.
        //! @param output The Value or Document where the converted data will start writing to.
        //! @param allocator The memory allocator used by RapidJSON to create the json document.
        //! @param object The object that will be read from for values to convert.
        //! @param settings Optional additional settings to control the way document is serialized.
        template<typename T>
        static JsonSerializationResult::ResultCode Store(
            rapidjson::Value& output, rapidjson::Document::AllocatorType& allocator, const T& object,
            const JsonSerializerSettings& settings = JsonSerializerSettings{});
        //! Stores the data in the provided object as json values starting at the provided value.
        //! @param output The Value or Document where the converted data will start writing to.
        //! @param allocator The memory allocator used by RapidJSON to create the json document.
        //! @param object The object that will be read from for values to convert.
        //! @param settings Additional settings to control the way document is serialized.
        template<typename T>
        static JsonSerializationResult::ResultCode Store(
            rapidjson::Value& output, rapidjson::Document::AllocatorType& allocator, const T& object,
            JsonSerializerSettings& settings);
        
        //! Stores the data in the provided object as json values starting at the provided value.
        //! @param output The Value or Document where the converted data will start writing to.
        //! @param allocator The memory allocator used by RapidJSON to create the json document.
        //! @param object The object that will be read from for values to convert.
        //! @param defaultObject Default object used to compare the object to in order to determine if values are
        //!     defaulted or not. If this is argument is provided m_keepDefaults in the settings will automatically 
        //!     be set to true.
        //! @param settings Optional additional settings to control the way document is serialized.
        template<typename T>
        static JsonSerializationResult::ResultCode Store(
            rapidjson::Value& output, rapidjson::Document::AllocatorType& allocator, const T& object, const T& defaultObject,
            const JsonSerializerSettings& settings = JsonSerializerSettings{});
        //! Stores the data in the provided object as json values starting at the provided value.
        //! @param output The Value or Document where the converted data will start writing to.
        //! @param allocator The memory allocator used by RapidJSON to create the json document.
        //! @param object The object that will be read from for values to convert.
        //! @param defaultObject Default object used to compare the object to in order to determine if values are
        //!     defaulted or not. If this is argument is provided m_keepDefaults in the settings will automatically
        //!     be set to true.
        //! @param settings Additional settings to control the way document is serialized.
        template<typename T>
        static JsonSerializationResult::ResultCode Store(
            rapidjson::Value& output, rapidjson::Document::AllocatorType& allocator, const T& object, const T& defaultObject,
            JsonSerializerSettings& settings);
        
        //! Stores the data in the provided object as json values starting at the provided value.
        //! @param output The Value or Document where the converted data will start writing to.
        //! @param allocator The memory allocator used by RapidJSON to create the json document.
        //! @param object Pointer to the object that will be read from for values to convert.
        //! @param defaultObject Pointer to a default object used to compare the object to in order to determine if values are
        //!     defaulted or not. This argument can be null, in which case a temporary default may be created if required by
        //!     the settings. If this is argument is provided m_keepDefaults in the settings will automatically  be set to true.
        //! @param objectType The type id of the object and default object.
        //! @param settings Optional additional settings to control the way document is serialized.
        static JsonSerializationResult::ResultCode Store(
            rapidjson::Value& output, rapidjson::Document::AllocatorType& allocator, const void* object, const void* defaultObject,
            const Uuid& objectType, const JsonSerializerSettings& settings = JsonSerializerSettings{});
        //! Stores the data in the provided object as json values starting at the provided value.
        //! @param output The Value or Document where the converted data will start writing to.
        //! @param allocator The memory allocator used by RapidJSON to create the json document.
        //! @param object Pointer to the object that will be read from for values to convert.
        //! @param defaultObject Pointer to a default object used to compare the object to in order to determine if values are
        //!     defaulted or not. This argument can be null, in which case a temporary default may be created if required by
        //!     the settings. If this is argument is provided m_keepDefaults in the settings will automatically  be set to true.
        //! @param objectType The type id of the object and default object.
        //! @param settings Additional settings to control the way document is serialized.
        static JsonSerializationResult::ResultCode Store(
            rapidjson::Value& output, rapidjson::Document::AllocatorType& allocator, const void* object, const void* defaultObject,
            const Uuid& objectType, JsonSerializerSettings& settings);

        //! Stores a name for the type id in the provided output. The name can be safely used to reference a type such as a class during loading.
        //! Note: it's not recommended to use this function (frequently) as it requires users of the json file to have knowledge of the internal
        //!     type structure and is therefore harder to use.
        //! @param output The json value the result will be written to. If successful this will contain a string object otherwise a default object.
        //! @param allocator The allocator associated with the document that will or already holds the output.
        //! @param typeId The type id to store.
        //! @param elementPath An optional path to the element. This will be used for reporting.
        //! @param settings Optional settings to change where this function collects information from. This can be the same settings
        //!     as used for the other Store functions.
        static JsonSerializationResult::ResultCode StoreTypeId(
            rapidjson::Value& output, rapidjson::Document::AllocatorType& allocator, const Uuid& typeId,
            AZStd::string_view elementPath = AZStd::string_view{}, const JsonSerializerSettings& settings = JsonSerializerSettings{});
        //! Stores a name for the type id in the provided output. The name can be safely used to reference a type such as a class during
        //! loading. Note: it's not recommended to use this function (frequently) as it requires users of the json file to have knowledge of
        //! the internal
        //!     type structure and is therefore harder to use.
        //! @param output The json value the result will be written to. If successful this will contain a string object otherwise a default
        //! object.
        //! @param allocator The allocator associated with the document that will or already holds the output.
        //! @param typeId The type id to store.
        //! @param elementPath The path to the element. This will be used for reporting.
        //! @param settings Settings to change where this function collects information from. This can be the same settings
        //!     as used for the other Store functions.
        static JsonSerializationResult::ResultCode StoreTypeId(
            rapidjson::Value& output, rapidjson::Document::AllocatorType& allocator, const Uuid& typeId,
            AZStd::string_view elementPath, JsonSerializerSettings& settings);

        //! Compares two json values of any type and determines if the left is less, equal or greater than the right.
        //! @param lhs The left hand side value for the compare.
        //! @param rhs The right hand side value for the compare.
        //! @return An enum containing less, equal or greater. In case of an error, the value for the enum will "error".
        static JsonSerializerCompareResult Compare(const rapidjson::Value& lhs, const rapidjson::Value& rhs);

        //! Resolves all import directives, including nested imports, in the given document. An importer object needs to be passed
        //! in through the settings.
        //! @param jsonDoc The json document in which to resolve imports.
        //! @param allocator The allocator associated with the json document.
        //! @param settings Additional settings that control the way the imports are resolved.
        static JsonSerializationResult::ResultCode ResolveImports(
            rapidjson::Value& jsonDoc, rapidjson::Document::AllocatorType& allocator, JsonImportSettings& settings);
        
        //! Restores all import directives that were present in the json document. The same importer object that was
        //! passed into ResolveImports through the settings needs to be passed here through settings as well.
        //! @param jsonDoc The json document in which to restore imports.
        //! @param allocator The allocator associated with the json document.
        //! @param settings Additional settings that control the way the imports are restored.
        static JsonSerializationResult::ResultCode RestoreImports(
            rapidjson::Value& jsonDoc, rapidjson::Document::AllocatorType& allocator, JsonImportSettings& settings);

    private:
        JsonSerialization() = delete;
        ~JsonSerialization() = delete;
        JsonSerialization& operator=(const JsonSerialization& rhs) = delete;
        JsonSerialization& operator=(JsonSerialization&& rhs) = delete;
        JsonSerialization(const JsonSerialization& rhs) = delete;
        JsonSerialization(JsonSerialization&& rhs) = delete;

        static JsonSerializationResult::ResultCode DefaultIssueReporter(AZStd::string& scratchBuffer, AZStd::string_view message,
            JsonSerializationResult::ResultCode result, AZStd::string_view path);

        static JsonSerializerCompareResult CompareObject(const rapidjson::Value& lhs, const rapidjson::Value& rhs);
        static JsonSerializerCompareResult CompareArray(const rapidjson::Value& lhs, const rapidjson::Value& rhs);
        static JsonSerializerCompareResult CompareString(const rapidjson::Value& lhs, const rapidjson::Value& rhs);
    };

    template<typename T>
    JsonSerializationResult::ResultCode JsonSerialization::Load(T& object, const rapidjson::Value& root, const JsonDeserializerSettings& settings)
    {
        return Load(&object, azrtti_typeid(object), root, settings);
    }

    template<typename T>
    JsonSerializationResult::ResultCode JsonSerialization::Load(T& object, const rapidjson::Value& root, JsonDeserializerSettings& settings)
    {
        return Load(&object, azrtti_typeid(object), root, settings);
    }

    template<typename T>
    JsonSerializationResult::ResultCode JsonSerialization::Store(
        rapidjson::Value& output, rapidjson::Document::AllocatorType& allocator, const T& object, const JsonSerializerSettings& settings)
    {
        return Store(output, allocator, &object, nullptr, azrtti_typeid(object), settings);
    }

    template<typename T>
    JsonSerializationResult::ResultCode JsonSerialization::Store(
        rapidjson::Value& output, rapidjson::Document::AllocatorType& allocator, const T& object, JsonSerializerSettings& settings)
    {
        return Store(output, allocator, &object, nullptr, azrtti_typeid(object), settings);
    }

    template<typename T>
    JsonSerializationResult::ResultCode JsonSerialization::Store(rapidjson::Value& output, rapidjson::Document::AllocatorType& allocator,
        const T& object, const T& defaultObject, const JsonSerializerSettings& settings)
    {
        return Store(output, allocator, &object,& defaultObject, azrtti_typeid(object), settings);
    }

    template<typename T>
    JsonSerializationResult::ResultCode JsonSerialization::Store(
        rapidjson::Value& output, rapidjson::Document::AllocatorType& allocator, const T& object, const T& defaultObject,
        JsonSerializerSettings& settings)
    {
        return Store(output, allocator, &object, &defaultObject, azrtti_typeid(object), settings);
    }
} // namespace AZ
