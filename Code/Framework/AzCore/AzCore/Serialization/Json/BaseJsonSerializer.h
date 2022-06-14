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
#include <AzCore/Serialization/Json/JsonSerializationMetadata.h>
#include <AzCore/Serialization/Json/JsonSerializationSettings.h>
#include <AzCore/Serialization/Json/StackedString.h>
#include <AzCore/std/containers/stack.h>

namespace AZ
{
    struct Uuid;

    class JsonBaseContext
    {
    public:
        JsonBaseContext(JsonSerializationMetadata& metadata, JsonSerializationResult::JsonIssueCallback reporting,
            StackedString::Format pathFormat, SerializeContext* serializeContext, JsonRegistrationContext* registrationContext);
        virtual ~JsonBaseContext() = default;

        //! Report progress and issues. Users can change the return code to change the behavior of the (de)serializer.
        //! @result The result code of the operation that's being reported on.
        //! @message The message to report to the caller of the (de)serializer.
        //! @return The result code passed in as an argument or an updated version if the caller desires a different behavior.
        JsonSerializationResult::Result Report(JsonSerializationResult::ResultCode result, AZStd::string_view message) const;
        //! Report progress and issues. Users can change the return code to change the behavior of the (de)serializer.
        //! @task The task that was being performed.
        //! @outcome The result of the task.
        //! @message The message to report to the caller of the (de)serializer.
        //! @return The result code passed in as an argument or an updated version if the caller desires a different behavior.
        JsonSerializationResult::Result Report(JsonSerializationResult::Tasks task, JsonSerializationResult::Outcomes outcome,
            AZStd::string_view message) const;
        //! Push a (temporary) new callback to the reporter stack. The top reporter will always be used.
        void PushReporter(JsonSerializationResult::JsonIssueCallback callback);
        //! Removes a previously pushed reporter. This function will guarantee that there's always one reporter
        //! Even if there are too many calls to PopReporter.
        void PopReporter();
        //! Get the currently active reporter.
        const JsonSerializationResult::JsonIssueCallback& GetReporter() const;
        //! Get the currently active reporter.
        JsonSerializationResult::JsonIssueCallback& GetReporter();

        //! Add a child name to the path.
        void PushPath(AZStd::string_view child);
        //! Add an index to the path.
        void PushPath(size_t index);
        //! Remove a previously added entry to the path.
        void PopPath();
        //! Gets the path to the element that's currently being operated on.
        const StackedString& GetPath() const;

        JsonSerializationMetadata& GetMetadata();
        const JsonSerializationMetadata& GetMetadata() const;

        SerializeContext* GetSerializeContext();
        const SerializeContext* GetSerializeContext() const;

        JsonRegistrationContext* GetRegistrationContext();
        const JsonRegistrationContext* GetRegistrationContext() const;

    protected:
        //! Callback used to report progress and issues. Users of the serialization can update the return code to change
        //! the behavior of the serializer.
        AZStd::stack<JsonSerializationResult::JsonIssueCallback> m_reporters;

        //! Path to the element that's currently being operated on.
        StackedString m_path;

        //! Metadata that's passed in by the settings as additional configuration options or metadata that's collected
        //! during processing for later use.
        JsonSerializationMetadata& m_metadata;

        //! The Serialize Context that can be used to retrieve meta data during processing.
        SerializeContext* m_serializeContext = nullptr;
        //! The registration context for the json serialization. This can be used to retrieve the handlers for specific types.
        JsonRegistrationContext* m_registrationContext = nullptr;
    };

    class JsonDeserializerContext final
        : public JsonBaseContext
    {
    public:
        explicit JsonDeserializerContext(JsonDeserializerSettings& settings);
        ~JsonDeserializerContext() override = default;

        JsonDeserializerContext(const JsonDeserializerContext&) = delete;
        JsonDeserializerContext(JsonDeserializerContext&&) = delete;
        JsonDeserializerContext& operator=(const JsonDeserializerContext&) = delete;
        JsonDeserializerContext& operator=(JsonDeserializerContext&&) = delete;
        
        //! If true then all containers should be cleared in the object before applying the data from the json document. If set to false
        //! any values in the container will be kept and not overwritten.
        //! Note that this does not apply to containers where elements have a fixed location such as smart pointers or AZStd::tuple.
        bool ShouldClearContainers() const;

    private:
        bool m_clearContainers = false;
    };

    class JsonSerializerContext final
        : public JsonBaseContext
    {
    public:
        JsonSerializerContext(JsonSerializerSettings& settings, rapidjson::Document::AllocatorType& jsonAllocator);
        ~JsonSerializerContext() override = default;

        JsonSerializerContext(const JsonSerializerContext&) = delete;
        JsonSerializerContext(JsonSerializerContext&&) = delete;
        JsonSerializerContext& operator=(const JsonSerializerContext&) = delete;
        JsonSerializerContext& operator=(JsonSerializerContext&&) = delete;

        //! The allocator used to add new values to the a json value, if needed.
        rapidjson::Document::AllocatorType& GetJsonAllocator();

        //! If true default value will be stored, otherwise only changed values will be stored.
        bool ShouldKeepDefaults() const;

    private:
        rapidjson::Document::AllocatorType& m_jsonAllocator;

        bool m_keepDefaults = false;
    };

    class ScopedContextReporter
    {
    public:
        ScopedContextReporter(JsonBaseContext& context, JsonSerializationResult::JsonIssueCallback callback);
        ~ScopedContextReporter();

    private:
        JsonBaseContext& m_context;
    };

    class ScopedContextPath
    {
    public:
        ScopedContextPath(JsonBaseContext& context, AZStd::string_view child);
        ScopedContextPath(JsonBaseContext& context, size_t index);
        ~ScopedContextPath();

    private:
        JsonBaseContext& m_context;
    };

    //! The abstract class for primitive Json Serializers. 
    //! It is intentionally not templated, and uses void* for output value parameters.
    class BaseJsonSerializer
    {
    public:
        AZ_RTTI(BaseJsonSerializer, "{7291FFDC-D339-40B5-BB26-EA067A327B21}");
        
        enum class ContinuationFlags
        {
            None = 0,                       //! No extra flags.
            ResolvePointer = 1 << 0,        //! The pointer passed in contains a pointer. The (de)serializer will attempt to resolve to an instance.
            ReplaceDefault = 1 << 1,        //! The default value provided for storing will be replaced with a newly created one.
            LoadAsNewInstance = 1 << 2,     //! Treats the value as if it's a newly created instance. This may trigger serializers marked with
                                            //! OperationFlags::InitializeNewInstance. Used for instance by pointers or new instances added to
                                            //! an array.
            IgnoreTypeSerializer =  1 << 3, //! Ignore the custom/specific serializer for the TypeId
        };

        enum class OperationFlags
        {
            None = 0,                       //! No flags that control how the custom json serializer is used.
            ManualDefault = 1 << 0,         //! Even if an (explicit) default is found the custom json serializer will still be called.
            InitializeNewInstance = 1 << 1  //! If set, the custom json serializer will be called with an explicit default if a new
                                            //! instance of its target type is created.
        };

        virtual ~BaseJsonSerializer() = default;

        //! Transforms the data from the rapidjson Value to outputValue, if the conversion is possible and supported.
        //! The serializer is responsible for casting to the proper type and safely writing to the outputValue memory.
        //! \note The default implementation is to load the object ignoring a custom serializers for the type, which allows for custom serializers
        //! to modify the object after all default loading has occurred.
        virtual JsonSerializationResult::Result Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
            JsonDeserializerContext& context);
        
        //! Write the input value to a rapidjson value if the default value is not null and doesn't match the input value, otherwise
        //! an error is returned and sets the rapidjson value to a null value.
        //! \note The default implementation is to store the object ignoring custom serializers.
        virtual JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
            const Uuid& valueTypeId, JsonSerializerContext& context);

        //! Returns the operation flags which tells the Json Serialization how this custom json serializer can be used.
        virtual OperationFlags GetOperationsFlags() const;

    protected:
        //! Continues loading of a (sub)value. Use this function to load member variables for instance. This is more optimal than 
        //! directly calling the json serialization.
        //! @param object Pointer to the object where the data will be loaded into.
        //! @param typeId Type id of the object passed in.
        //! @param value The value in the JSON document where the deserializer will start reading data from.
        //! @param context The context used during deserialization. Use the value passed in from Load.
        JsonSerializationResult::ResultCode ContinueLoading(
            void* object, const Uuid& typeId, const rapidjson::Value& value, JsonDeserializerContext& context,
            ContinuationFlags flags = ContinuationFlags::None);

        //! Continues storing of a (sub)value. Use this function to store member variables for instance. This is more optimal than 
        //! directly calling the json serialization.
        //! @param output The value in the json document where the converted data will start writing to.
        //! @param allocator The memory allocator used by RapidJSON to create the json document.
        //! @param object Pointer to the object that will be read from for values to convert.
        //! @param defaultObject Pointer to a default object used to compare the object to in order to determine if values are
        //!     defaulted or not. This argument can be null, in which case a temporary default may be created if required by
        //!     the settings.
        //! @param typeId The type id of the object and default object.
        //! @param context The context used during serialization. Use the value passed in from Store.
        JsonSerializationResult::ResultCode ContinueStoring(
            rapidjson::Value& output, const void* object, const void* defaultObject, const Uuid& typeId, JsonSerializerContext& context,
            ContinuationFlags flags = ContinuationFlags::None);

        //! Retrieves the type id from a json object or json string.
        //! @param typeId The retrieved type id.
        //! @param input The json object to read type information from. This call will fail if input is not an object.
        //! @param context The context used during serialization. Use the value passed in from Store.
        //! @param baseTypeId An optional base type that the target type id is expected to inherit from. If two or more types share
        //!     the same then the baseTypeId is used to disambiguate between candidates.
        //! @param isExplicit An optional boolean to indicate if the type id was explicitly (true) declared in the json file or that
        //!     it was derived from the provided information (false).
        JsonSerializationResult::ResultCode LoadTypeId(Uuid& typeId, const rapidjson::Value& input, JsonDeserializerContext& context,
            const Uuid* baseTypeId = nullptr, bool* isExplicit = nullptr);

        //! Stores the name of the type id or if there are conflicts the type's uuid.
        //! @param output The json value that will receive the converted type id.
        //! @param typId The Type id to store.
        //! @param context The context used during serialization. Use the value passed in from Store.
        JsonSerializationResult::ResultCode StoreTypeId(rapidjson::Value& output,
            const Uuid& typeId, JsonSerializerContext& context);

        //! Helper function similar to ContinueLoading, but loads the data as a member of 'value' rather than 'value' itself, if it exists.
        JsonSerializationResult::ResultCode ContinueLoadingFromJsonObjectField(
            void* object, const Uuid& typeId, const rapidjson::Value& value, rapidjson::Value::StringRefType memberName,
            JsonDeserializerContext& context, ContinuationFlags flags = ContinuationFlags::None);

        //! Helper function similar to ContinueStoring, but stores the data as a member of 'output' rather than overwriting 'output'.
        JsonSerializationResult::ResultCode ContinueStoringToJsonObjectField(
            rapidjson::Value& output, rapidjson::Value::StringRefType newMemberName, const void* object, const void* defaultObject,
            const Uuid& typeId, JsonSerializerContext& context, ContinuationFlags flags = ContinuationFlags::None);

        //! Checks if a value is an explicit default. This useful for containers where not storing anything as a default would mean
        //! a slot wouldn't be used so something has to be added to represent the fully default target.
        bool IsExplicitDefault(const rapidjson::Value& value);

        //! Gets a value to represent an explicit default. This useful for containers where not storing anything as a default would mean
        //! a slot wouldn't be used so something has to be added to represent the fully default target.
        rapidjson::Value GetExplicitDefault();
    };

    AZ_DEFINE_ENUM_BITWISE_OPERATORS(AZ::BaseJsonSerializer::ContinuationFlags)
    AZ_DEFINE_ENUM_BITWISE_OPERATORS(AZ::BaseJsonSerializer::OperationFlags)

} // namespace AZ
