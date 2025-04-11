/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/JSON/stringbuffer.h>
#include <AzCore/Serialization/Json/JsonMerger.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/StackedString.h>
#include <AzCore/std/string/fixed_string.h>

namespace AZ
{
    JsonSerializationResult::ResultCode JsonMerger::ApplyPatch(rapidjson::Value& target,
        rapidjson::Document::AllocatorType& allocator, const rapidjson::Value& patch,
        JsonApplyPatchSettings& settings)
    {
        using namespace JsonSerializationResult;

        StackedString element(StackedString::Format::JsonPointer);
        if (!patch.IsArray())
        {
            return settings.m_reporting("JSON Patch only supports arrays at the root.",
                ResultCode(Tasks::Merge, Outcomes::TypeMismatch), element);
        }

        ResultCode result(Tasks::Merge);
        size_t counter = 0;
        for (const rapidjson::Value& entry : patch.GetArray())
        {
            ScopedStackedString entryName(element, counter++);
            if (!entry.IsObject())
            {
                return settings.m_reporting("Entry in JSON Patch is not an object.",
                    ResultCode(Tasks::Merge, Outcomes::TypeMismatch), element);
            }

            auto operation = entry.FindMember("op");
            if (operation == entry.MemberEnd())
            {
                ScopedStackedString entryOp(element, "op");
                return settings.m_reporting(R"(The required field "op" is missing in entry of JSON Patch.)",
                    ResultCode(Tasks::Merge, Outcomes::Missing), element);
            }
            if (!operation->value.IsString())
            {
                ScopedStackedString entryOp(element, "op");
                return settings.m_reporting(R"(The value for "op" is not a string.)",
                    ResultCode(Tasks::Merge, Outcomes::TypeMismatch), element);
            }

            // Every operation requires having a path.
            auto path = entry.FindMember("path");
            if (path == entry.MemberEnd())
            {
                ScopedStackedString entryPath(element, "path");
                return settings.m_reporting(R"(The required field "path" is missing in entry of JSON Patch.)",
                    ResultCode(Tasks::Merge, Outcomes::Missing), element);
            }
            if (!path->value.IsString())
            {
                ScopedStackedString entryPath(element, "path");
                return settings.m_reporting(R"(The value for "path" is not a string.)",
                    ResultCode(Tasks::Merge, Outcomes::TypeMismatch), element);
            }
            rapidjson::Pointer pathPointer(path->value.GetString(), path->value.GetStringLength());
            if (!pathPointer.IsValid())
            {
                ScopedStackedString entryPath(element, "path");
                return settings.m_reporting(R"(The value for "path" is not a valid JSON Pointer.)",
                    ResultCode(Tasks::Merge, Outcomes::Invalid), element);
            }

            AZStd::string_view operationName(operation->value.GetString(), operation->value.GetStringLength());
            if (operationName.compare("add") == 0)
            {
                result.Combine(ApplyPatch_Add(target, allocator, entry, pathPointer, element, settings));
            }
            else if (operationName.compare("remove") == 0)
            {
                result.Combine(ApplyPatch_Remove(target, pathPointer, element, settings));
            }
            else if (operationName.compare("replace") == 0)
            {
                result.Combine(ApplyPatch_Replace(target, allocator, entry, pathPointer, element, settings));
            }
            else if (operationName.compare("move") == 0)
            {
                result.Combine(ApplyPatch_Move(target, allocator, entry, pathPointer, element, settings));
            }
            else if (operationName.compare("copy") == 0)
            {
                result.Combine(ApplyPatch_Copy(target, allocator, entry, pathPointer, element, settings));
            }
            else if (operationName.compare("test") == 0)
            {
                result.Combine(ApplyPatch_Test(target, entry, pathPointer, element, settings));
            }
            else
            {
                auto message = ReporterString::format(R"(Unknown operation "%.*s".)", AZ_STRING_ARG(operationName));
                return settings.m_reporting(message.c_str(), ResultCode(Tasks::Merge, Outcomes::Unknown), element);
            }

            if (result.GetProcessing() == Processing::Halted)
            {
                return result;
            }
        }

        result.Combine(ResultCode(Tasks::Merge, Outcomes::Success));
        return result;
    }

    JsonSerializationResult::ResultCode JsonMerger::CreatePatch(rapidjson::Value& patch,
        rapidjson::Document::AllocatorType& allocator, const rapidjson::Value& source,
        const rapidjson::Value& target, JsonCreatePatchSettings& settings)
    {
        StackedString element(StackedString::Format::JsonPointer);
        return CreatePatchInternal(patch.SetArray(), allocator, source, target, element, settings);
    }

    JsonSerializationResult::ResultCode JsonMerger::ApplyMergePatch(rapidjson::Value& target,
        rapidjson::Document::AllocatorType& allocator, const rapidjson::Value& patch,
        JsonApplyPatchSettings& settings)
    {
        StackedString element(StackedString::Format::JsonPointer);
        return ApplyMergePatchInternal(target, allocator, patch, settings, element);
    }

    JsonSerializationResult::ResultCode JsonMerger::ApplyMergePatchInternal(rapidjson::Value& target,
        rapidjson::Document::AllocatorType& allocator, const rapidjson::Value& patch,
        JsonApplyPatchSettings& settings, StackedString& element)
    {
        using namespace JsonSerializationResult;

        ResultCode result(Tasks::Merge);

        if (patch.IsObject())
        {
            if (!target.IsObject())
            {
                target.SetObject();
            }

            for (const auto& field : patch.GetObject())
            {
                auto targetField = target.FindMember(field.name);
                if (field.value.IsObject())
                {
                    if (targetField != target.MemberEnd())
                    {
                        ScopedStackedString fieldNameScope{ element,
                            AZStd::string_view(field.name.GetString(), field.name.GetStringLength()) };
                        result.Combine(ApplyMergePatchInternal(targetField->value, allocator, field.value, settings, element));
                    }
                    else
                    {
                        rapidjson::Value name;
                        name.CopyFrom(field.name, allocator, true);
                        rapidjson::Value value;
                        ScopedStackedString fieldNameScope{ element,
                            AZStd::string_view(field.name.GetString(), field.name.GetStringLength()) };
                        result.Combine(ApplyMergePatchInternal(value, allocator, field.value, settings, element));
                        target.AddMember(AZStd::move(name), AZStd::move(value), allocator);
                    }
                }
                else if (field.value.IsNull())
                {
                    if (targetField != target.MemberEnd())
                    {
                        ScopedStackedString fieldNameScope{ element,
                            AZStd::string_view(field.name.GetString(), field.name.GetStringLength()) };
                        AZStd::string_view jsonPath = element.Get();

                        target.RemoveMember(targetField);
                        result.Combine(settings.m_reporting(ReporterString::format(
                            R"(Successfully removed member from "%.*s" using JSON Merge Patch)", AZ_STRING_ARG(jsonPath)),
                            ResultCode(Tasks::Merge, Outcomes::Success), element));
                    }
                }
                else
                {
                    if (targetField != target.MemberEnd())
                    {
                        targetField->value.CopyFrom(field.value, allocator, true);

                        ScopedStackedString fieldNameScope{ element, AZStd::string_view(field.name.GetString(), field.name.GetStringLength()) };
                        AZStd::string_view jsonPath = element.Get();
                        result.Combine(settings.m_reporting(ReporterString::format(
                            R"(Successfully updated JSON field "%.*s" using JSON Merge Patch)", AZ_STRING_ARG(jsonPath)),
                            ResultCode(Tasks::Merge, Outcomes::Success), element));
                    }
                    else
                    {
                        rapidjson::Value name;
                        rapidjson::Value value;
                        name.CopyFrom(field.name, allocator, true);
                        value.CopyFrom(field.value, allocator, true);
                        target.AddMember(AZStd::move(name), AZStd::move(value), allocator);

                        ScopedStackedString fieldNameScope{ element, AZStd::string_view(field.name.GetString(), field.name.GetStringLength()) };
                        AZStd::string_view jsonPath = element.Get();
                        result.Combine(settings.m_reporting(ReporterString::format(
                            R"(Successfully added JSON field "%.*s" using JSON Merge Patch)", AZ_STRING_ARG(jsonPath)),
                            ResultCode(Tasks::Merge, Outcomes::Success), element));
                    }
                }
            }
        }
        else
        {
            target.CopyFrom(patch, allocator, true);
        }
        result.Combine(settings.m_reporting("Successfully applied patch to target using JSON Merge Patch.",
            ResultCode(Tasks::Merge, Outcomes::Success), element));
        return result;
    }

    JsonSerializationResult::ResultCode JsonMerger::CreateMergePatch(rapidjson::Value& patch,
        rapidjson::Document::AllocatorType& allocator, const rapidjson::Value& source,
        const rapidjson::Value& target, JsonCreatePatchSettings& settings)
    {
        StackedString element(StackedString::Format::JsonPointer);
        return CreateMergePatchInternal(patch, allocator, source, target, element, settings);
    }

    JsonSerializationResult::ResultCode JsonMerger::ApplyPatch_GetFromValue(rapidjson::Value** fromValue, rapidjson::Pointer& fromPointer,
        rapidjson::Value& target, const rapidjson::Value& entry, StackedString& element, JsonApplyPatchSettings& settings)
    {
        using namespace JsonSerializationResult;

        auto from = entry.FindMember("from");
        if (from == entry.MemberEnd())
        {
            ScopedStackedString entryOp(element, "from");
            return settings.m_reporting(R"(The required field "from" is missing in entry of JSON Patch.)",
                ResultCode(Tasks::Merge, Outcomes::Missing), element);
        }
        if (!from->value.IsString())
        {
            ScopedStackedString entryOp(element, "from");
            return settings.m_reporting(R"(The value for "from" is not a string.)",
                ResultCode(Tasks::Merge, Outcomes::TypeMismatch), element);
        }

        fromPointer = rapidjson::Pointer(from->value.GetString(), from->value.GetStringLength());
        if (!fromPointer.IsValid())
        {
            ScopedStackedString entryPath(element, "from");
            return settings.m_reporting(R"(The value for "from" is not a valid JSON Pointer.)",
                ResultCode(Tasks::Merge, Outcomes::Invalid), element);
        }

        *fromValue = fromPointer.Get(target);
        if (!(*fromValue))
        {
            ScopedStackedString entryPath(element, "from");
            return settings.m_reporting(R"(The path specified in "from" doesn't exist.)",
                ResultCode(Tasks::Merge, Outcomes::Invalid), element);
        }

        return ResultCode(Tasks::Merge, Outcomes::Success);
    }

    JsonSerializationResult::ResultCode JsonMerger::ApplyPatch_Add(rapidjson::Value& target,
        rapidjson::Document::AllocatorType& allocator, const rapidjson::Value& entry, const rapidjson::Pointer& path,
        StackedString& element, JsonApplyPatchSettings& settings)
    {
        using namespace JsonSerializationResult;

        auto value = entry.FindMember("value");
        if (value == entry.MemberEnd())
        {
            ScopedStackedString valueName(element, "value");
            return settings.m_reporting(R"(The required field "value" for "add" operation is missing.)",
                ResultCode(Tasks::Merge, Outcomes::Missing), element);
        }

        rapidjson::Value newValue;
        newValue.CopyFrom(value->value, allocator, true);
        return ApplyPatch_AddValue(target, allocator, path, AZStd::move(newValue), element, settings);
    }

    JsonSerializationResult::ResultCode JsonMerger::ApplyPatch_AddValue(rapidjson::Value& target,
        rapidjson::Document::AllocatorType& allocator, const rapidjson::Pointer& path, rapidjson::Value&& newValue,
        StackedString& element, JsonApplyPatchSettings& settings)
    {
        using namespace JsonSerializationResult;

        const rapidjson::Pointer::Token* const tokens = path.GetTokens();
        if (path.GetTokenCount() == 0)
        {
            rapidjson::StringBuffer pointerPathString;
            path.Stringify(pointerPathString);
            target = AZStd::move(newValue);
            return settings.m_reporting(R"(Successfully applied "add" operation.)",
                ResultCode(Tasks::Merge, Outcomes::Success), pointerPathString.GetString());
        }

        rapidjson::Pointer parent = rapidjson::Pointer(tokens, path.GetTokenCount() - 1);
        rapidjson::Value* parentValue = parent.Get(target);
        if (!parentValue)
        {
            return settings.m_reporting(R"(The target path for "add" operation does not exist.)",
                ResultCode(Tasks::Merge, Outcomes::Invalid), element);
        }

        if (parentValue->IsObject())
        {
            const char* member = tokens[path.GetTokenCount() - 1].name;
            auto it = parentValue->FindMember(member);
            if (it != parentValue->MemberEnd())
            {
                // Update existing value, if found.
                it->value = AZStd::move(newValue);
            }
            else
            {
                // Add new member as the value wasn't found.
                parentValue->AddMember(rapidjson::Value(member, allocator), AZStd::move(newValue), allocator);
            }
        }
        else if (parentValue->IsArray())
        {
            const rapidjson::Pointer::Token& elementToken = tokens[path.GetTokenCount() - 1];
            rapidjson::SizeType index = elementToken.index;
            if (index == rapidjson::kPointerInvalidIndex)
            {
                if (elementToken.name[0] == '-' && elementToken.name[1] == 0)
                {
                    parentValue->PushBack(AZStd::move(newValue), allocator);
                }
                else
                {
                    return settings.m_reporting(R"(The target path for "add" operation is not an index value.)",
                        ResultCode(Tasks::Merge, Outcomes::Invalid), element);
                }
            }
            else
            {
                rapidjson::SizeType count = parentValue->Size();
                if (index < count)
                {
                    parentValue->PushBack(rapidjson::Value(), allocator);
                    rapidjson::Value::Array array = parentValue->GetArray();
                    for (rapidjson::SizeType i = count; i > index; --i)
                    {
                        array[i] = AZStd::move(array[i - 1]);
                    }
                    array[index] = AZStd::move(newValue);
                }
                else if (index == count)
                {
                    parentValue->PushBack(AZStd::move(newValue), allocator);
                }
                else
                {
                    return settings.m_reporting(R"(The target path for "add" operation is not a valid index.)",
                        ResultCode(Tasks::Merge, Outcomes::Invalid), element);
                }
            }
        }
        else
        {
            return settings.m_reporting(R"(The target for "add" operation is not an object or array.)",
                ResultCode(Tasks::Merge, Outcomes::TypeMismatch), element);
        }

        rapidjson::StringBuffer pointerPathString;
        path.Stringify(pointerPathString);
        return settings.m_reporting(R"(Successfully applied "add" operation.)",
            ResultCode(Tasks::Merge, Outcomes::Success), pointerPathString.GetString());
    }

    JsonSerializationResult::ResultCode JsonMerger::ApplyPatch_Remove(rapidjson::Value& target, const rapidjson::Pointer& path,
        StackedString& element, JsonApplyPatchSettings& settings)
    {
        using namespace JsonSerializationResult;

        const rapidjson::Pointer::Token* const tokens = path.GetTokens();
        if (path.GetTokenCount() == 0)
        {
            return settings.m_reporting(R"(The target path for "remove" operation can't point at the root.)",
                ResultCode(Tasks::Merge, Outcomes::Invalid), element);
        }

        rapidjson::Pointer parent = rapidjson::Pointer(tokens, path.GetTokenCount() - 1);
        rapidjson::Value* parentValue = parent.Get(target);
        if (!parentValue)
        {
            return settings.m_reporting(R"(The target path for "remove" operation does not exist.)",
                ResultCode(Tasks::Merge, Outcomes::Invalid), element);
        }

        if (parentValue->IsObject())
        {
            if (!parentValue->EraseMember(tokens[path.GetTokenCount() - 1].name))
            {
                rapidjson::StringBuffer pathString;
                path.Stringify(pathString);
                return settings.m_reporting(
                    AZStd::string::format(
                        R"(The "remove" operation failed to remove member '%s' from object at path '%s'.)",
                        tokens[path.GetTokenCount() - 1].name, pathString.GetString()),
                    ResultCode(Tasks::Merge, Outcomes::Invalid), element);
            }
        }
        else if (parentValue->IsArray())
        {
            const rapidjson::SizeType index = tokens[path.GetTokenCount() - 1].index;
            if (index != rapidjson::kPointerInvalidIndex && index < parentValue->Size())
            {
                parentValue->Erase(parentValue->Begin() + index);
            }
            else
            {
                return settings.m_reporting(R"(The target path for "remove" operation has an invalid index.)",
                    ResultCode(Tasks::Merge, Outcomes::Invalid), element);
            }
        }
        else
        {
            return settings.m_reporting(R"(The target for "remove" operation is not an object or array.)",
                ResultCode(Tasks::Merge, Outcomes::TypeMismatch), element);
        }

        rapidjson::StringBuffer pointerPathString;
        path.Stringify(pointerPathString);
        return settings.m_reporting(R"(Successfully applied "remove" operation.)",
            ResultCode(Tasks::Merge, Outcomes::Success), pointerPathString.GetString());
    }

    JsonSerializationResult::ResultCode JsonMerger::ApplyPatch_Replace(rapidjson::Value& target,
        rapidjson::Document::AllocatorType& allocator, const rapidjson::Value& entry, const rapidjson::Pointer& path,
        StackedString& element, JsonApplyPatchSettings& settings)
    {
        using namespace JsonSerializationResult;

        auto value = entry.FindMember("value");
        if (value == entry.MemberEnd())
        {
            ScopedStackedString valueName(element, "value");
            return settings.m_reporting(R"(The required field "value" for "replace" operation is missing.)",
                ResultCode(Tasks::Merge, Outcomes::Missing), element);
        }

        rapidjson::Value* memberValue = path.Get(target);
        if (!memberValue)
        {
            return settings.m_reporting(R"(The target for "replace" operation doesn't exist.)",
                ResultCode(Tasks::Merge, Outcomes::Invalid), element);
        }

        memberValue->CopyFrom(value->value, allocator, true);

        rapidjson::StringBuffer pointerPathString;
        path.Stringify(pointerPathString);
        return settings.m_reporting(R"(Successfully applied "replace" operation.)",
            ResultCode(Tasks::Merge, Outcomes::Success), pointerPathString.GetString());
    }

    JsonSerializationResult::ResultCode JsonMerger::ApplyPatch_Move(rapidjson::Value& target,
        rapidjson::Document::AllocatorType& allocator, const rapidjson::Value& entry, const rapidjson::Pointer& path,
        StackedString& element, JsonApplyPatchSettings& settings)
    {
        using namespace JsonSerializationResult;

        rapidjson::Value* fromValue;
        rapidjson::Pointer fromPointer;
        ResultCode result = ApplyPatch_GetFromValue(&fromValue, fromPointer, target, entry, element, settings);
        if (result.GetProcessing() != Processing::Halted)
        {
            AZ_Assert(fromValue, R"(The "fromValue" was retrieved but is still null.)");
            rapidjson::Value moved = AZStd::move(*fromValue);
            result.Combine(ApplyPatch_Remove(target, fromPointer, element, settings));
            result.Combine(ApplyPatch_AddValue(target, allocator, path, AZStd::move(moved), element, settings));
        }
        return result;
    }

    JsonSerializationResult::ResultCode JsonMerger::ApplyPatch_Copy(rapidjson::Value& target,
        rapidjson::Document::AllocatorType& allocator, const rapidjson::Value& entry, const rapidjson::Pointer& path,
        StackedString& element, JsonApplyPatchSettings& settings)
    {
        using namespace JsonSerializationResult;

        rapidjson::Value* fromValue;
        rapidjson::Pointer fromPointer;
        ResultCode result = ApplyPatch_GetFromValue(&fromValue, fromPointer, target, entry, element, settings);
        if (result.GetProcessing() != Processing::Halted)
        {
            AZ_Assert(fromValue, R"(The "fromValue" was retrieved but is still null.)");
            rapidjson::Value copiedValue;
            copiedValue.CopyFrom(*fromValue, allocator, true);
            result.Combine(ApplyPatch_AddValue(target, allocator, path, AZStd::move(copiedValue), element, settings));
        }
        return result;
    }

    JsonSerializationResult::ResultCode JsonMerger::ApplyPatch_Test(rapidjson::Value& target, const rapidjson::Value& entry,
        const rapidjson::Pointer& path, StackedString& element, JsonApplyPatchSettings& settings)
    {
        using namespace JsonSerializationResult;

        auto value = entry.FindMember("value");
        if (value == entry.MemberEnd())
        {
            ScopedStackedString entryOp(element, "value");
            return settings.m_reporting(R"(The required field "value" for "test" operation is missing.)",
                ResultCode(Tasks::Merge, Outcomes::Missing), element);
        }

        rapidjson::Value* compareValue = path.Get(target);
        if (!compareValue)
        {
            ScopedStackedString entryPath(element, "path");
            return settings.m_reporting(R"(The path specified in "path" for the "test" operation doesn't exist.)",
                ResultCode(Tasks::Merge, Outcomes::Invalid), element);
        }

        if (JsonSerialization::Compare(*compareValue, value->value) != JsonSerializerCompareResult::Equal)
        {
            return settings.m_reporting(R"(The values for the "test" operation are not equal.)",
                ResultCode(Tasks::Merge, Outcomes::TestFailed), element);
        }

        return settings.m_reporting(R"(Successfully applied "test" operation.)",
            ResultCode(Tasks::Merge, Outcomes::Success), element);
    }

    JsonSerializationResult::ResultCode JsonMerger::CreatePatchInternal(rapidjson::Value& patch,
        rapidjson::Document::AllocatorType& allocator, const rapidjson::Value& source,
        const rapidjson::Value& target, StackedString& element, JsonCreatePatchSettings& settings)
    {
        using namespace JsonSerializationResult;
        
        if (source.IsObject() && target.IsObject())
        {
            ResultCode resultCode(Tasks::CreatePatch);
            for (const auto& field : target.GetObject())
            {
                ScopedStackedString entryName(element, AZStd::string_view(field.name.GetString(), field.name.GetStringLength()));
                auto sourceField = source.FindMember(field.name);
                if (sourceField != source.MemberEnd())
                {
                    resultCode.Combine(CreatePatchInternal(patch, allocator, sourceField->value, field.value, element, settings));
                    if (resultCode.GetProcessing() == Processing::Halted)
                    {
                        return resultCode;
                    }
                }
                else
                {
                    patch.PushBack(CreatePatchInternal_Add(allocator, element, field.value), allocator);
                    resultCode.Combine(settings.m_reporting("Successfully completed add operation for new member value in JSON Patch.",
                        ResultCode(Tasks::CreatePatch, Outcomes::Success), element));
                }
            }

            // Do an extra pass to find all the fields that are removed.
            for (const auto& field : source.GetObject())
            {
                ScopedStackedString entryName(element, AZStd::string_view(field.name.GetString(), field.name.GetStringLength()));

                auto targetField = target.FindMember(field.name);
                if (targetField == target.MemberEnd())
                {
                    patch.PushBack(CreatePatchInternal_Remove(allocator, element), allocator);
                    resultCode.Combine(settings.m_reporting("Removed member from object in JSON Patch.",
                        ResultCode(Tasks::CreatePatch, Outcomes::Success), element));
                }
            }

            return resultCode;
        }
        else if (source.IsArray() && target.IsArray())
        {
            ResultCode resultCode(Tasks::CreatePatch);
            rapidjson::SizeType count = AZStd::min(source.Size(), target.Size());
            for (rapidjson::SizeType i = 0; i < count; ++i)
            {
                ScopedStackedString entryName(element, i);
                resultCode.Combine(CreatePatchInternal(patch, allocator, source[i], target[i], element, settings));
                if (resultCode.GetProcessing() == Processing::Halted)
                {
                    return resultCode;
                }
            }

            if (source.Size() < target.Size())
            {
                rapidjson::SizeType targetCount = target.Size();
                for (rapidjson::SizeType i = count; i < targetCount; ++i)
                {
                    ScopedStackedString entryName(element, i);
                    patch.PushBack(CreatePatchInternal_Add(allocator, element, target[i]), allocator);
                    resultCode.Combine(settings.m_reporting("Added entry to array in JSON Patch.",
                        ResultCode(Tasks::CreatePatch, Outcomes::Success), element));
                }
            }
            else if (source.Size() > target.Size())
            {
                // Loop backwards through the removals so that each removal has a valid index when processing in order.
                for (rapidjson::SizeType i = source.Size(); i > count; --i)
                {
                    // (We use "i - 1" here instead of in the loop to ensure we don't wrap around our unsigned numbers in the case
                    // where count is 0.)
                    ScopedStackedString entryName(element, i - 1);
                    patch.PushBack(CreatePatchInternal_Remove(allocator, element), allocator);
                    resultCode.Combine(settings.m_reporting("Removed member from array in JSON Patch.",
                        ResultCode(Tasks::CreatePatch, Outcomes::Success), element));
                }
            }
            return resultCode;
        }
        else
        {
            if (source != target)
            {
                patch.PushBack(CreatePatchInternal_Replace(allocator, element, target), allocator);
                return settings.m_reporting("Successfully completed replace operation for value to JSON Patch.",
                    ResultCode(Tasks::CreatePatch, Outcomes::Success), element);
            }
            else
            {
                return settings.m_reporting("Values are identical so no operation was not added to JSON Patch.",
                    ResultCode(Tasks::CreatePatch, Outcomes::DefaultsUsed), element);
            }
        }
    }

    rapidjson::Value JsonMerger::CreatePatchInternal_Add(rapidjson::Document::AllocatorType& allocator,
        StackedString& path, const rapidjson::Value& value)
    {
        rapidjson::Value replaceOp(rapidjson::kObjectType);
        rapidjson::Value pathArg = rapidjson::Value(path.Get().data(), aznumeric_caster(path.Get().length()), allocator);
        rapidjson::Value valueArg;
        valueArg.CopyFrom(value, allocator, true);
        replaceOp.
            AddMember(rapidjson::StringRef("op"), rapidjson::StringRef("add"), allocator).
            AddMember(rapidjson::StringRef("path"), AZStd::move(pathArg), allocator).
            AddMember(rapidjson::StringRef("value"), AZStd::move(valueArg), allocator);
        return replaceOp;
    }

    rapidjson::Value JsonMerger::CreatePatchInternal_Remove(rapidjson::Document::AllocatorType& allocator,
        StackedString& path)
    {
        rapidjson::Value removeOp(rapidjson::kObjectType);
        rapidjson::Value pathArg = rapidjson::Value(path.Get().data(), aznumeric_caster(path.Get().length()), allocator);
        removeOp.
            AddMember(rapidjson::StringRef("op"), rapidjson::StringRef("remove"), allocator).
            AddMember(rapidjson::StringRef("path"), AZStd::move(pathArg), allocator);
        return removeOp;
    }

    rapidjson::Value JsonMerger::CreatePatchInternal_Replace(rapidjson::Document::AllocatorType& allocator,
        StackedString& path, const rapidjson::Value& value)
    {
        rapidjson::Value replaceOp(rapidjson::kObjectType);
        rapidjson::Value pathArg = rapidjson::Value(path.Get().data(), aznumeric_caster(path.Get().length()), allocator);
        rapidjson::Value valueArg;
        valueArg.CopyFrom(value, allocator, true);
        replaceOp.
            AddMember(rapidjson::StringRef("op"), rapidjson::StringRef("replace"), allocator).
            AddMember(rapidjson::StringRef("path"), AZStd::move(pathArg), allocator).
            AddMember(rapidjson::StringRef("value"), AZStd::move(valueArg), allocator);
        return replaceOp;
    }

    JsonSerializationResult::ResultCode JsonMerger::CreateMergePatchInternal(rapidjson::Value& patch,
        rapidjson::Document::AllocatorType& allocator, const rapidjson::Value& source,
        const rapidjson::Value& target, StackedString& element, JsonCreatePatchSettings& settings)
    {
        using namespace JsonSerializationResult;

        if (target.IsObject())
        {
            ResultCode resultCode(Tasks::CreatePatch);
            rapidjson::Value resultValue(rapidjson::kObjectType);

            if (source.IsObject())
            {
                for (const auto& field : target.GetObject())
                {
                    ScopedStackedString entryName(element, AZStd::string_view(field.name.GetString(), field.name.GetStringLength()));

                    rapidjson::Value value;
                    ResultCode result(Tasks::CreatePatch);
                    auto sourceField = source.FindMember(field.name);
                    if (sourceField != source.MemberEnd())
                    {
                        result = CreateMergePatchInternal(value, allocator, sourceField->value, field.value, element, settings);
                    }
                    else
                    {
                        result = CreateMergePatchInternal(value, allocator,
                            rapidjson::Value(rapidjson::kNullType), field.value, element, settings);
                    }

                    if (result.GetOutcome() == Outcomes::Success || result.GetOutcome() == Outcomes::PartialDefaults)
                    {
                        rapidjson::Value name;
                        name.CopyFrom(field.name, allocator, true);
                        resultValue.AddMember(AZStd::move(name), AZStd::move(value), allocator);
                        resultCode.Combine(settings.m_reporting("Added new member to object in JSON Merge Patch.", result, element));
                    }
                    else if (result.GetProcessing() != Processing::Completed)
                    {
                        return result;
                    }
                    else
                    {
                        resultCode.Combine(result);
                    }
                }

                // Do an extra pass to find all the fields that are removed.
                for (const auto& field : source.GetObject())
                {
                    ScopedStackedString entryName(element, AZStd::string_view(field.name.GetString(), field.name.GetStringLength()));

                    auto targetField = target.FindMember(field.name);
                    if (targetField == target.MemberEnd())
                    {
                        rapidjson::Value name;
                        name.CopyFrom(field.name, allocator, true);
                        resultValue.AddMember(AZStd::move(name), rapidjson::Value(rapidjson::kNullType), allocator);
                        resultCode.Combine(settings.m_reporting("Removed member from object in JSON Merge Patch.",
                            ResultCode(Tasks::CreatePatch, Outcomes::Success), element));
                    }
                }
            }
            else
            {
                for (const auto& field : target.GetObject())
                {
                    ScopedStackedString entryName(element, AZStd::string_view(field.name.GetString(), field.name.GetStringLength()));

                    // Have to do the test here because null is also the default value to pass in for recursion.
                    if (field.value.IsNull())
                    {
                        return settings.m_reporting("JSON Merge Patch can't be used to write nulls.",
                            ResultCode(Tasks::CreatePatch, Outcomes::Unsupported), element);
                    }

                    rapidjson::Value value;
                    ResultCode result = CreateMergePatchInternal(value, allocator,
                        rapidjson::Value(rapidjson::kNullType), field.value, element, settings);
                    if (result.GetOutcome() == Outcomes::Success || result.GetOutcome() == Outcomes::PartialDefaults)
                    {
                        rapidjson::Value name;
                        name.CopyFrom(field.name, allocator, true);
                        resultValue.AddMember(AZStd::move(name), AZStd::move(value), allocator);
                        resultCode.Combine(settings.m_reporting("Added new member to object in JSON Merge Patch.", result, element));
                    }
                    else if (result.GetProcessing() != Processing::Completed)
                    {
                        return result;
                    }
                    else
                    {
                        resultCode.Combine(result);
                    }
                }
                
                if (target.MemberCount() == 0)
                {
                    resultCode.Combine(settings.m_reporting("Added empty object to JSON Merge Patch.",
                        ResultCode(Tasks::CreatePatch, Outcomes::Success), element));
                }
            }

            patch = AZStd::move(resultValue);
            return resultCode;
        }
        else
        {
            if (source != target)
            {
                if (target.IsNull())
                {
                    return settings.m_reporting("JSON Merge Patch can't be used to write nulls.",
                        ResultCode(Tasks::CreatePatch, Outcomes::Unsupported), element);
                }

                patch.CopyFrom(target, allocator, true);
                return settings.m_reporting("Value successfully added to JSON Merge Patch.",
                    ResultCode(Tasks::CreatePatch, Outcomes::Success), element);
            }
            else
            {
                return settings.m_reporting("Values are identical so value was not added to JSON Merge Patch.",
                    ResultCode(Tasks::CreatePatch, Outcomes::DefaultsUsed), element);
            }
        }
    }
} // namespace AZ
