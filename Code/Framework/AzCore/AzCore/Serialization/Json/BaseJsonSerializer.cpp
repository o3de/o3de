/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Json/BaseJsonSerializer.h>
#include <AzCore/Serialization/Json/JsonDeserializer.h>
#include <AzCore/Serialization/Json/JsonSerializer.h>
#include <AzCore/Serialization/Json/JsonSerializationResult.h>

namespace AZ
{
    //
    // JsonBaseContext
    //

    JsonBaseContext::JsonBaseContext(JsonSerializationMetadata& metadata, JsonSerializationResult::JsonIssueCallback reporting,
        StackedString::Format pathFormat, SerializeContext* serializeContext, JsonRegistrationContext* registrationContext)
        : m_metadata(metadata)
        , m_serializeContext(serializeContext)
        , m_registrationContext(registrationContext)
        , m_path(pathFormat)
    {
        m_reporters.push(AZStd::move(reporting));
    }

    JsonSerializationResult::Result JsonBaseContext::Report(JsonSerializationResult::ResultCode result, AZStd::string_view message) const
    {
        AZ_Assert(!m_reporters.empty(), "A JsonBaseContext should always have at least one callback function.");
        return JsonSerializationResult::Result(m_reporters.top(), message, result, m_path);
    }

    JsonSerializationResult::Result JsonBaseContext::Report(JsonSerializationResult::Tasks task,
        JsonSerializationResult::Outcomes outcome, AZStd::string_view message) const
    {
        return Report(JsonSerializationResult::ResultCode(task, outcome), message);
    }

    void JsonBaseContext::PushReporter(JsonSerializationResult::JsonIssueCallback callback)
    {
        m_reporters.push(AZStd::move(callback));
    }

    void JsonBaseContext::PopReporter()
    {
        AZ_Assert(!m_reporters.empty(), "Unable to pop a reporter from the JsonBaseContext as there's no entry to pop.");
        if (!m_reporters.empty())
        {
            m_reporters.pop();
        }
    }

    const JsonSerializationResult::JsonIssueCallback& JsonBaseContext::GetReporter() const
    {
        AZ_Assert(!m_reporters.empty(), "A JsonBaseContext should always have at least one callback function.");
        return m_reporters.top();
    }

    JsonSerializationResult::JsonIssueCallback& JsonBaseContext::GetReporter()
    {
        AZ_Assert(!m_reporters.empty(), "A JsonBaseContext should always have at least one callback function.");
        return m_reporters.top();
    }

    void JsonBaseContext::PushPath(AZStd::string_view child)
    {
        m_path.Push(child);
    }

    void JsonBaseContext::PushPath(size_t index)
    {
        m_path.Push(index);
    }

    void JsonBaseContext::PopPath()
    {
        m_path.Pop();
    }

    const StackedString& JsonBaseContext::GetPath() const
    {
        return m_path;
    }

    JsonSerializationMetadata& JsonBaseContext::GetMetadata()
    {
        return m_metadata;
    }

    const JsonSerializationMetadata& JsonBaseContext::GetMetadata() const
    {
        return m_metadata;
    }

    SerializeContext* JsonBaseContext::GetSerializeContext()
    {
        return m_serializeContext;
    }

    const SerializeContext* JsonBaseContext::GetSerializeContext() const
    {
        return m_serializeContext;
    }

    JsonRegistrationContext* JsonBaseContext::GetRegistrationContext()
    {
        return m_registrationContext;
    }

    const JsonRegistrationContext* JsonBaseContext::GetRegistrationContext() const
    {
        return m_registrationContext;
    }



    //
    // JsonDeserializerContext
    //

    JsonDeserializerContext::JsonDeserializerContext(JsonDeserializerSettings& settings)
        : JsonBaseContext(settings.m_metadata, settings.m_reporting,
            StackedString::Format::JsonPointer, settings.m_serializeContext, settings.m_registrationContext)
        , m_clearContainers(settings.m_clearContainers)
    {
    }

    bool JsonDeserializerContext::ShouldClearContainers() const
    {
        return m_clearContainers;
    }



    //
    // JsonSerializerContext
    // 

    JsonSerializerContext::JsonSerializerContext(JsonSerializerSettings& settings, rapidjson::Document::AllocatorType& jsonAllocator)
        : JsonBaseContext(settings.m_metadata, settings.m_reporting, StackedString::Format::ContextPath,
            settings.m_serializeContext, settings.m_registrationContext)
        , m_jsonAllocator(jsonAllocator)
        , m_keepDefaults(settings.m_keepDefaults)
    {
    }

    rapidjson::Document::AllocatorType& JsonSerializerContext::GetJsonAllocator()
    {
        return m_jsonAllocator;
    }

    bool JsonSerializerContext::ShouldKeepDefaults() const
    {
        return m_keepDefaults;
    }



    //
    // ScopedContextReporter
    //

    ScopedContextReporter::ScopedContextReporter(JsonBaseContext& context, JsonSerializationResult::JsonIssueCallback callback)
        : m_context(context)
    {
        context.PushReporter(AZStd::move(callback));
    }

    ScopedContextReporter::~ScopedContextReporter()
    {
        m_context.PopReporter();
    }



    //
    // ScopedContextPath
    //

    ScopedContextPath::ScopedContextPath(JsonBaseContext& context, AZStd::string_view child)
        : m_context(context)
    {
        m_context.PushPath(child);
    }

    ScopedContextPath::ScopedContextPath(JsonBaseContext& context, size_t index)
        : m_context(context)
    {
        m_context.PushPath(index);
    }

    ScopedContextPath::~ScopedContextPath()
    {
        m_context.PopPath();
    }



    //
    // BaseJsonSerializer
    //

    JsonSerializationResult::Result BaseJsonSerializer::Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
        JsonDeserializerContext& context)
    {
        JsonSerializationResult::ResultCode result(JsonSerializationResult::Tasks::ReadField);
        result.Combine(ContinueLoading(outputValue, outputValueTypeId, inputValue, context, ContinuationFlags::IgnoreTypeSerializer));
        return context.Report(result, "Ignoring custom serialization during load");
    }

    JsonSerializationResult::Result BaseJsonSerializer::Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
        const Uuid& valueTypeId, JsonSerializerContext& context)
    {
        JsonSerializationResult::ResultCode result(JsonSerializationResult::Tasks::WriteValue);
        result.Combine(ContinueStoring(outputValue, inputValue, defaultValue, valueTypeId, context, ContinuationFlags::IgnoreTypeSerializer));
        return context.Report(result, "Ignoring custom serialization during store");
    }

    BaseJsonSerializer::OperationFlags BaseJsonSerializer::GetOperationsFlags() const
    {
        return OperationFlags::None;
    }

    JsonSerializationResult::ResultCode BaseJsonSerializer::ContinueLoading(
        void* object, const Uuid& typeId, const rapidjson::Value& value, JsonDeserializerContext& context, ContinuationFlags flags)
    {
        bool loadAsNewInstance = (flags & ContinuationFlags::LoadAsNewInstance) == ContinuationFlags::LoadAsNewInstance;
        JsonDeserializer::UseTypeDeserializer useCustom = (flags & ContinuationFlags::IgnoreTypeSerializer) == ContinuationFlags::IgnoreTypeSerializer
            ? JsonDeserializer::UseTypeDeserializer::No
            : JsonDeserializer::UseTypeDeserializer::Yes;

        return (flags & ContinuationFlags::ResolvePointer) == ContinuationFlags::ResolvePointer
            ? JsonDeserializer::LoadToPointer(object, typeId, value, useCustom, context)
            : JsonDeserializer::Load(object, typeId, value, loadAsNewInstance, useCustom, context);
    }

    JsonSerializationResult::ResultCode BaseJsonSerializer::ContinueStoring(
        rapidjson::Value& output, const void* object, const void* defaultObject, const Uuid& typeId, JsonSerializerContext& context,
        ContinuationFlags flags)
    {
        using namespace JsonSerializationResult;

        JsonSerializer::UseTypeSerializer useCustom = (flags & ContinuationFlags::IgnoreTypeSerializer) == ContinuationFlags::IgnoreTypeSerializer
            ? JsonSerializer::UseTypeSerializer::No
            : JsonSerializer::UseTypeSerializer::Yes;

        if ((flags & ContinuationFlags::ReplaceDefault) == ContinuationFlags::ReplaceDefault && !context.ShouldKeepDefaults())
        {
            if ((flags & ContinuationFlags::ResolvePointer) == ContinuationFlags::ResolvePointer)
            {
                return JsonSerializer::StoreFromPointer(output, object, nullptr, typeId, useCustom, context);
            }
            else
            {
                AZStd::any newDefaultObject = context.GetSerializeContext()->CreateAny(typeId);
                if (newDefaultObject.empty())
                {
                    ResultCode result = context.Report(Tasks::CreateDefault, Outcomes::Unsupported,
                        "No factory available to create a default object for comparison.");
                    if (result.GetProcessing() == Processing::Halted)
                    {
                        return result;
                    }
                    return result.Combine(JsonSerializer::Store(output, object, nullptr, typeId, useCustom, context));
                }
                else
                {
                    void* defaultObjectPtr = AZStd::any_cast<void>(&newDefaultObject);
                    return JsonSerializer::Store(output, object, defaultObjectPtr, typeId, useCustom, context);
                }
            }
        }
        
        return (flags & ContinuationFlags::ResolvePointer) == ContinuationFlags::ResolvePointer ?
            JsonSerializer::StoreFromPointer(output, object, defaultObject, typeId, useCustom, context) :
            JsonSerializer::Store(output, object, defaultObject, typeId, useCustom, context);
    }

    JsonSerializationResult::ResultCode BaseJsonSerializer::LoadTypeId(Uuid& typeId, const rapidjson::Value& input,
        JsonDeserializerContext& context, const Uuid* baseTypeId, bool* isExplicit)
    {
        return JsonDeserializer::LoadTypeId(typeId, input, context, baseTypeId, isExplicit);
    }

    JsonSerializationResult::ResultCode BaseJsonSerializer::StoreTypeId(rapidjson::Value& output,
        const Uuid& typeId, JsonSerializerContext& context)
    {
        return JsonSerializer::StoreTypeName(output, typeId, context);
    }

    JsonSerializationResult::ResultCode BaseJsonSerializer::ContinueLoadingFromJsonObjectField(
        void* object, const Uuid& typeId, const rapidjson::Value& value, rapidjson::Value::StringRefType memberName,
        JsonDeserializerContext& context, ContinuationFlags flags)
    {
        using namespace JsonSerializationResult;

        if (value.IsObject())
        {
            auto iter = value.FindMember(memberName);
            if (iter != value.MemberEnd())
            {
                ScopedContextPath subPath{context, memberName.s};
                return ContinueLoading(object, typeId, iter->value, context, flags);
            }
            else
            {
                return ResultCode(Tasks::ReadField, Outcomes::DefaultsUsed);
            }
        }
        else
        {
            return context.Report(Tasks::ReadField, Outcomes::Unsupported, "Value is not an object");
        }
    }

    JsonSerializationResult::ResultCode BaseJsonSerializer::ContinueStoringToJsonObjectField(rapidjson::Value& output,
        rapidjson::Value::StringRefType newMemberName, const void* object, const void* defaultObject,
        const Uuid& typeId, JsonSerializerContext& context, ContinuationFlags flags)
    {
        using namespace JsonSerializationResult;

        if (!output.IsObject())
        {
            if (!output.IsNull())
            {
                return context.Report(Tasks::WriteValue, Outcomes::Unsupported, "Value is not an object");
            }

            output.SetObject();
        }

        rapidjson::Value newValue;
        ResultCode result = ContinueStoring(newValue, object, defaultObject, typeId, context, flags);
        if (result.GetProcessing() != Processing::Halted &&
            (context.ShouldKeepDefaults() || result.GetOutcome() != Outcomes::DefaultsUsed))
        {
            output.AddMember(newMemberName, newValue, context.GetJsonAllocator());
        }
        return result;
    }

    bool BaseJsonSerializer::IsExplicitDefault(const rapidjson::Value& value)
    {
        return JsonDeserializer::IsExplicitDefault(value);
    }

    rapidjson::Value BaseJsonSerializer::GetExplicitDefault()
    {
        return JsonSerializer::GetExplicitDefault();
    }
} // namespace AZ
