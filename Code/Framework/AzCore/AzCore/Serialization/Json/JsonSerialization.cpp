/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/Json/BaseJsonSerializer.h>
#include <AzCore/Serialization/Json/JsonDeserializer.h>
#include <AzCore/Serialization/Json/JsonImporter.h>
#include <AzCore/Serialization/Json/JsonMerger.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/JsonSerializer.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/Json/StackedString.h>
#include <AzCore/std/sort.h>

namespace AZ
{
    namespace JsonSerializationInternal
    {
        template<typename T>
        JsonSerializationResult::ResultCode GetContext(const T& settings, SerializeContext*& serializeContext)
        {
            using namespace JsonSerializationResult;

            ResultCode result = ResultCode(Tasks::RetrieveInfo, Outcomes::Success);
            serializeContext = settings.m_serializeContext;
            if (!serializeContext)
            {
                AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
                if (!serializeContext)
                {
                    result.Combine(ResultCode(Tasks::RetrieveInfo, Outcomes::Catastrophic));
                    if (settings.m_reporting)
                    {
                        settings.m_reporting("Unable to retrieve context information.", result, "");
                    }
                    else
                    {
                        AZ_Warning("JSON Serialization", false, "Failed to retrieve serialize context for json serialization.");
                    }
                }
            }
            return result;
        }

        template<typename T>
        JsonSerializationResult::ResultCode GetContext(const T& settings, JsonRegistrationContext*& registrationContext)
        {
            using namespace JsonSerializationResult;

            ResultCode result = ResultCode(Tasks::RetrieveInfo, Outcomes::Success);
            registrationContext = settings.m_registrationContext;
            if (!registrationContext)
            {
                AZ::ComponentApplicationBus::BroadcastResult(registrationContext, &AZ::ComponentApplicationBus::Events::GetJsonRegistrationContext);
                if (!registrationContext)
                {
                    result.Combine(ResultCode(Tasks::RetrieveInfo, Outcomes::Catastrophic));
                    if (settings.m_reporting)
                    {
                        settings.m_reporting("Failed to retrieve json registration context for json serialization.", result, "");
                    }
                    else
                    {
                        AZ_Warning("JSON Serialization", false, "Failed to retrieve json registration context for json serialization.");
                    }
                }
            }
            return result;
        }

        template<typename T>
        JsonSerializationResult::ResultCode GetContexts(const T& settings,
            SerializeContext*& serializeContext, JsonRegistrationContext*& registrationContext)
        {
            using namespace JsonSerializationResult;

            ResultCode result = GetContext(settings, serializeContext);
            if (result.GetOutcome() == Outcomes::Success)
            {
                result = GetContext(settings, registrationContext);
            }
            return result;
        }
    } // namespace JsonSerializationInternal

    JsonSerializationResult::ResultCode JsonSerialization::ApplyPatch(
        rapidjson::Value& target, rapidjson::Document::AllocatorType& allocator, const rapidjson::Value& patch, JsonMergeApproach approach,
        const JsonApplyPatchSettings& settings)
    {
        // Explicitly make a copy to call the correct overloaded version and avoid infinite recursion on this function.
        JsonApplyPatchSettings settingsCopy{ settings };
        return ApplyPatch(target, allocator, patch, approach, settingsCopy);
    }

    JsonSerializationResult::ResultCode JsonSerialization::ApplyPatch(rapidjson::Value& target,
        rapidjson::Document::AllocatorType& allocator, const rapidjson::Value& patch, JsonMergeApproach approach,
        JsonApplyPatchSettings& settings)
    {
        using namespace JsonSerializationResult;

        AZStd::string scratchBuffer;
        auto issueReportingCallback = [&scratchBuffer](AZStd::string_view message, ResultCode result, AZStd::string_view target) -> ResultCode
        {
            return JsonSerialization::DefaultIssueReporter(scratchBuffer, message, result, target);
        };
        if (!settings.m_reporting)
        {
            settings.m_reporting = issueReportingCallback;
        }

        switch (approach)
        {
        case JsonMergeApproach::JsonPatch:
            return JsonMerger::ApplyPatch(target, allocator, patch, settings);
        case JsonMergeApproach::JsonMergePatch:
            return JsonMerger::ApplyMergePatch(target, allocator, patch, settings);
        default:
            AZ_Assert(false, "Unsupported JsonMergeApproach (%i).", aznumeric_cast<int>(approach));
            return settings.m_reporting("Unsupported JsonMergeApproach.", ResultCode(Tasks::Merge, Outcomes::Catastrophic),
                StackedString(StackedString::Format::JsonPointer));
        }
    }

    JsonSerializationResult::ResultCode JsonSerialization::ApplyPatch(
        rapidjson::Value& output, rapidjson::Document::AllocatorType& allocator, const rapidjson::Value& source,
        const rapidjson::Value& patch, JsonMergeApproach approach, const JsonApplyPatchSettings& settings)
    {
        // Explicitly make a copy to call the correct overloaded version and avoid infinite recursion on this function.
        JsonApplyPatchSettings settingsCopy{settings};
        return ApplyPatch(output, allocator, source, patch, approach, settingsCopy);
    }

    JsonSerializationResult::ResultCode JsonSerialization::ApplyPatch(rapidjson::Value& output, rapidjson::Document::AllocatorType& allocator,
        const rapidjson::Value& source, const rapidjson::Value& patch, JsonMergeApproach approach, JsonApplyPatchSettings& settings)
    {
        using namespace JsonSerializationResult;

        AZStd::string scratchBuffer;
        auto issueReportingCallback = [&scratchBuffer](AZStd::string_view message, ResultCode result, AZStd::string_view target) -> ResultCode
        {
            return JsonSerialization::DefaultIssueReporter(scratchBuffer, message, result, target);
        };
        if (!settings.m_reporting)
        {
            settings.m_reporting = issueReportingCallback;
        }

        output.CopyFrom(source, allocator);

        ResultCode result(Tasks::Merge);
        switch (approach)
        {
        case JsonMergeApproach::JsonPatch:
            result = JsonMerger::ApplyPatch(output, allocator, patch, settings);
            break;
        case JsonMergeApproach::JsonMergePatch:
            result = JsonMerger::ApplyMergePatch(output, allocator, patch, settings);
            break;
        default:
            AZ_Assert(false, "Unsupported JsonMergeApproach (%i).", aznumeric_cast<int>(approach));
            result = settings.m_reporting("Unsupported JsonMergeApproach.", ResultCode(Tasks::Merge, Outcomes::Catastrophic),
                StackedString(StackedString::Format::JsonPointer));
            break;
        }

        if (result.GetProcessing() == Processing::Halted)
        {
            output.SetObject();
        }
        return result;
    }

    JsonSerializationResult::ResultCode JsonSerialization::CreatePatch(
        rapidjson::Value& patch, rapidjson::Document::AllocatorType& allocator, const rapidjson::Value& source,
        const rapidjson::Value& target, JsonMergeApproach approach, const JsonCreatePatchSettings& settings)
    {
        // Explicitly make a copy to call the correct overloaded version and avoid infinite recursion on this function.
        JsonCreatePatchSettings settingsCopy{settings};
        return CreatePatch(patch, allocator, source, target, approach, settingsCopy);
    }

    JsonSerializationResult::ResultCode JsonSerialization::CreatePatch(
        rapidjson::Value& patch, rapidjson::Document::AllocatorType& allocator, const rapidjson::Value& source,
        const rapidjson::Value& target, JsonMergeApproach approach, JsonCreatePatchSettings& settings)
    {
        using namespace JsonSerializationResult;

        AZStd::string scratchBuffer;
        auto issueReportingCallback = [&scratchBuffer](AZStd::string_view message, ResultCode result, AZStd::string_view target) -> ResultCode
        {
            return JsonSerialization::DefaultIssueReporter(scratchBuffer, message, result, target);
        };
        if (!settings.m_reporting)
        {
            settings.m_reporting = issueReportingCallback;
        }

        switch (approach)
        {
        case JsonMergeApproach::JsonPatch:
            return JsonMerger::CreatePatch(patch, allocator, source, target, settings);
        case JsonMergeApproach::JsonMergePatch:
            return JsonMerger::CreateMergePatch(patch, allocator, source, target, settings);
        default:
            AZ_Assert(false, "Unsupported JsonMergeApproach (%i).", aznumeric_cast<int>(approach));
            return ResultCode(Tasks::CreatePatch, Outcomes::Catastrophic);
        }
    }

    JsonSerializationResult::ResultCode JsonSerialization::Load(
        void* object, const Uuid& objectType, const rapidjson::Value& root, const JsonDeserializerSettings& settings)
    {
        // Explicitly make a copy to call the correct overloaded version and avoid infinite recursion on this function.
        JsonDeserializerSettings settingsCopy{settings};
        return Load(object, objectType, root, settingsCopy);
    }

    JsonSerializationResult::ResultCode JsonSerialization::Load(
        void* object, const Uuid& objectType, const rapidjson::Value& root, JsonDeserializerSettings& settings)
    {
        using namespace JsonSerializationResult;

        AZStd::string scratchBuffer;
        auto issueReportingCallback = [&scratchBuffer](AZStd::string_view message, ResultCode result, AZStd::string_view target) -> ResultCode
        {
            return JsonSerialization::DefaultIssueReporter(scratchBuffer, message, result, target);
        };
        if (!settings.m_reporting)
        {
            settings.m_reporting = issueReportingCallback;
        }
        
        ResultCode result = JsonSerializationInternal::GetContexts(settings, settings.m_serializeContext, settings.m_registrationContext);
        if (result.GetOutcome() == Outcomes::Success)
        {
            StackedString path(StackedString::Format::JsonPointer);
            JsonDeserializerContext context(settings);
            result = JsonDeserializer::Load(object, objectType, root, false, JsonDeserializer::UseTypeDeserializer::Yes, context);
        }
        return result;
    }

    JsonSerializationResult::ResultCode JsonSerialization::LoadTypeId(
        Uuid& typeId, const rapidjson::Value& input, const Uuid* baseClassTypeId, AZStd::string_view jsonPath,
        const JsonDeserializerSettings& settings)
    {
        // Explicitly make a copy to call the correct overloaded version and avoid infinite recursion on this function.
        JsonDeserializerSettings settingsCopy{settings};
        return LoadTypeId(typeId, input, baseClassTypeId, jsonPath, settingsCopy);
    }

    JsonSerializationResult::ResultCode JsonSerialization::LoadTypeId(Uuid& typeId, const rapidjson::Value& input,
        const Uuid* baseClassTypeId, AZStd::string_view jsonPath, JsonDeserializerSettings& settings)
    {
        using namespace JsonSerializationResult;

        AZStd::string scratchBuffer;
        auto issueReportingCallback = [&scratchBuffer](AZStd::string_view message, ResultCode result, AZStd::string_view target) -> ResultCode
        {
            return JsonSerialization::DefaultIssueReporter(scratchBuffer, message, result, target);
        };
        if (!settings.m_reporting)
        {
            settings.m_reporting = issueReportingCallback;
        }

        ResultCode result = JsonSerializationInternal::GetContexts(settings, settings.m_serializeContext, settings.m_registrationContext);
        if (result.GetOutcome() == Outcomes::Success)
        {
            JsonDeserializerContext context(settings);
            context.PushPath(jsonPath);

            result = JsonDeserializer::LoadTypeId(typeId, input, context, baseClassTypeId);
        }
        return result;
    }

    JsonSerializationResult::ResultCode JsonSerialization::Store(
        rapidjson::Value& output, rapidjson::Document::AllocatorType& allocator, const void* object, const void* defaultObject,
        const Uuid& objectType, const JsonSerializerSettings& settings)
    {
        // Explicitly make a copy to call the correct overloaded version and avoid infinite recursion on this function.
        JsonSerializerSettings settingsCopy{settings};
        return Store(output, allocator, object, defaultObject, objectType, settingsCopy);
    }

    JsonSerializationResult::ResultCode JsonSerialization::Store(
        rapidjson::Value& output, rapidjson::Document::AllocatorType& allocator, const void* object, const void* defaultObject,
        const Uuid& objectType, JsonSerializerSettings& settings)
    {
        using namespace JsonSerializationResult;

        AZStd::string scratchBuffer;
        auto issueReportingCallback = [&scratchBuffer](AZStd::string_view message, ResultCode result, AZStd::string_view target) -> ResultCode
        {
            return JsonSerialization::DefaultIssueReporter(scratchBuffer, message, result, target);
        };
        if (!settings.m_reporting)
        {
            settings.m_reporting = issueReportingCallback;
        }

        ResultCode result = JsonSerializationInternal::GetContexts(settings, settings.m_serializeContext, settings.m_registrationContext);
        if (result.GetOutcome() == Outcomes::Success)
        {
            if (defaultObject)
            {
                // If a default object is provided by the user, then the intention is to strip defaults, so make sure
                // the settings match this.
                settings.m_keepDefaults = false;
            }

            JsonSerializerContext context(settings, allocator);
            StackedString path(StackedString::Format::ContextPath);
            result = JsonSerializer::Store(output, object, defaultObject, objectType, JsonSerializer::UseTypeSerializer::Yes, context);
        }
        return result;
    }

    JsonSerializationResult::ResultCode JsonSerialization::StoreTypeId(
        rapidjson::Value& output, rapidjson::Document::AllocatorType& allocator, const Uuid& typeId, AZStd::string_view elementPath,
        const JsonSerializerSettings& settings)
    {
        // Explicitly make a copy to call the correct overloaded version and avoid infinite recursion on this function.
        JsonSerializerSettings settingsCopy{settings};
        return StoreTypeId(output, allocator, typeId, elementPath, settingsCopy);
    }

    JsonSerializationResult::ResultCode JsonSerialization::StoreTypeId(rapidjson::Value& output, rapidjson::Document::AllocatorType& allocator,
        const Uuid& typeId, AZStd::string_view elementPath, JsonSerializerSettings& settings)
    {
        using namespace JsonSerializationResult;

        AZStd::string scratchBuffer;
        auto issueReportingCallback = [&scratchBuffer](AZStd::string_view message, ResultCode result, AZStd::string_view target) -> ResultCode
        {
            return JsonSerialization::DefaultIssueReporter(scratchBuffer, message, result, target);
        };
        if (!settings.m_reporting)
        {
            settings.m_reporting = issueReportingCallback;
        }

        ResultCode result = JsonSerializationInternal::GetContexts(settings, settings.m_serializeContext, settings.m_registrationContext);
        if (result.GetOutcome() == Outcomes::Success)
        {
            JsonSerializerContext context(settings, allocator);
            context.PushPath(elementPath);
            result = JsonSerializer::StoreTypeName(output, typeId, context);
        }
        return result;
    }

    JsonSerializerCompareResult JsonSerialization::Compare(const rapidjson::Value& lhs, const rapidjson::Value& rhs)
    {
        if (lhs.GetType() == rhs.GetType())
        {
            switch (lhs.GetType())
            {
            case rapidjson::kNullType:
                return JsonSerializerCompareResult::Equal;
            case rapidjson::kFalseType:
                return JsonSerializerCompareResult::Equal;
            case rapidjson::kTrueType:
                return JsonSerializerCompareResult::Equal;
            case rapidjson::kObjectType:
                return CompareObject(lhs, rhs);
            case rapidjson::kArrayType:
                return CompareArray(lhs, rhs);
            case rapidjson::kStringType:
                return CompareString(lhs, rhs);
            case rapidjson::kNumberType:
                return
                    lhs.GetDouble() < rhs.GetDouble() ? JsonSerializerCompareResult::Less :
                    lhs.GetDouble() == rhs.GetDouble() ? JsonSerializerCompareResult::Equal :
                    JsonSerializerCompareResult::Greater;
            default:
                return JsonSerializerCompareResult::Error;
            }
        }
        else
        {
            return lhs.GetType() < rhs.GetType() ? JsonSerializerCompareResult::Less : JsonSerializerCompareResult::Greater;
        }
    }

    JsonSerializationResult::ResultCode JsonSerialization::ResolveImports(
        rapidjson::Value& jsonDoc, rapidjson::Document::AllocatorType& allocator, JsonImportSettings& settings)
    {
        using namespace JsonSerializationResult;

        if (settings.m_importer == nullptr)
        {
            AZ_Assert(false, "Importer object needs to be provided");
            return ResultCode(Tasks::Import, Outcomes::Catastrophic);
        }

        AZStd::string scratchBuffer;
        auto issueReportingCallback = [&scratchBuffer](AZStd::string_view message, ResultCode result, AZStd::string_view target) -> ResultCode
        {
            return JsonSerialization::DefaultIssueReporter(scratchBuffer, message, result, target);
        };
        if (!settings.m_reporting)
        {
            settings.m_reporting = issueReportingCallback;
        }

        JsonImportResolver::ImportPathStack importPathStack;
        importPathStack.push_back(settings.m_loadedJsonPath);
        StackedString element(StackedString::Format::JsonPointer);

        return JsonImportResolver::ResolveImports(jsonDoc, allocator, importPathStack, settings, element);
    }

    JsonSerializationResult::ResultCode JsonSerialization::RestoreImports(
        rapidjson::Value& jsonDoc, rapidjson::Document::AllocatorType& allocator, JsonImportSettings& settings)
    {
        using namespace JsonSerializationResult;

        if (settings.m_importer == nullptr)
        {
            AZ_Assert(false, "Importer object needs to be provided");
            return ResultCode(Tasks::Import, Outcomes::Catastrophic);
        }

        AZStd::string scratchBuffer;
        auto issueReportingCallback = [&scratchBuffer](AZStd::string_view message, ResultCode result, AZStd::string_view target) -> ResultCode
        {
            return JsonSerialization::DefaultIssueReporter(scratchBuffer, message, result, target);
        };
        if (!settings.m_reporting)
        {
            settings.m_reporting = issueReportingCallback;
        }

        settings.m_resolveFlags = ImportTracking::None;

        return JsonImportResolver::RestoreImports(jsonDoc, allocator, settings);
    }

    JsonSerializationResult::ResultCode JsonSerialization::DefaultIssueReporter(AZStd::string& scratchBuffer,
        AZStd::string_view message, JsonSerializationResult::ResultCode result, AZStd::string_view path)
    {
        using namespace JsonSerializationResult;

        if (result.GetProcessing() != Processing::Completed)
        {
            scratchBuffer.append(message.begin(), message.end());
            scratchBuffer.append("\n    Reason: ");
            result.AppendToString(scratchBuffer, path);
            scratchBuffer.append(".");
            AZ_Warning("JSON Serialization", false, "%s", scratchBuffer.c_str());
            
            scratchBuffer.clear();
        }
        return result;
    }

    JsonSerializerCompareResult JsonSerialization::CompareObject(const rapidjson::Value& lhs, const rapidjson::Value& rhs)
    {
        AZ_Assert(lhs.IsObject() && rhs.IsObject(), R"(CompareObject can only compare values of type "object".)");
        rapidjson::SizeType count = lhs.MemberCount();
        if (count == rhs.MemberCount())
        {
            AZStd::vector<const rapidjson::Value::Member*> left;
            AZStd::vector<const rapidjson::Value::Member*> right;
            left.reserve(count);
            right.reserve(count);

            for (auto it = lhs.MemberBegin(); it != lhs.MemberEnd(); ++it)
            {
                left.push_back(&(*it));
            }
            for (auto it = rhs.MemberBegin(); it != rhs.MemberEnd(); ++it)
            {
                right.push_back(&(*it));
            }

            auto lessByMemberName = [](const rapidjson::Value::Member* lhs, const rapidjson::Value::Member* rhs) -> bool
            {
                return
                    AZStd::string_view(lhs->name.GetString(), lhs->name.GetStringLength()) <
                    AZStd::string_view(rhs->name.GetString(), rhs->name.GetStringLength());
            };
            AZStd::sort(left.begin(), left.end(), lessByMemberName);
            AZStd::sort(right.begin(), right.end(), lessByMemberName);

            for (rapidjson::SizeType i = 0; i < count; ++i)
            {
                const rapidjson::Value::Member* leftMember = left[i];
                const rapidjson::Value::Member* rightMember = right[i];
                AZStd::string_view leftName(leftMember->name.GetString(), leftMember->name.GetStringLength());
                AZStd::string_view rightName(rightMember->name.GetString(), rightMember->name.GetStringLength());
                int nameCompare = leftName.compare(rightName);
                if (nameCompare == 0)
                {
                    JsonSerializerCompareResult valueCompare = Compare(leftMember->value, rightMember->value);
                    if (valueCompare != JsonSerializerCompareResult::Equal)
                    {
                        return valueCompare;
                    }
                }
                else
                {
                    return nameCompare < 0 ? JsonSerializerCompareResult::Less : JsonSerializerCompareResult::Greater;
                }
            }
            return JsonSerializerCompareResult::Equal;
        }
        else
        {
            return lhs.MemberCount() < rhs.MemberCount() ? JsonSerializerCompareResult::Less : JsonSerializerCompareResult::Greater;
        }
    }

    JsonSerializerCompareResult JsonSerialization::CompareArray(const rapidjson::Value& lhs, const rapidjson::Value& rhs)
    {
        AZ_Assert(lhs.IsArray() && rhs.IsArray(), R"(CompareArray can only compare values of type "array".)");

        if (lhs.Size() == rhs.Size())
        {
            for (rapidjson::SizeType i = 0; i < lhs.Size(); ++i)
            {
                JsonSerializerCompareResult compare = Compare(lhs[i], rhs[i]);
                if (compare != JsonSerializerCompareResult::Equal)
                {
                    return compare;
                }
            }
            return JsonSerializerCompareResult::Equal;
        }
        else
        {
            return lhs.Size() < rhs.Size() ? JsonSerializerCompareResult::Less : JsonSerializerCompareResult::Greater;
        }
    }

    JsonSerializerCompareResult JsonSerialization::CompareString(const rapidjson::Value& lhs, const rapidjson::Value& rhs)
    {
        AZStd::string_view leftString(lhs.GetString(), lhs.GetStringLength());
        AZStd::string_view rightString(rhs.GetString(), rhs.GetStringLength());

        int result = leftString.compare(rightString);
        return
            result < 0 ? JsonSerializerCompareResult::Less :
            result == 0 ? JsonSerializerCompareResult::Equal :
            JsonSerializerCompareResult::Greater;
    }
} // namespace AZ
