/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/FileIO.h>
#include <AzCore/Serialization/Json/JsonImporter.h>
#include <AzCore/Serialization/Json/JsonMerger.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>

namespace AZ
{
    JsonSerializationResult::ResultCode JsonImportResolver::ResolveNestedImports(rapidjson::Value& jsonDoc,
        rapidjson::Document::AllocatorType& allocator, ImportPathStack& importPathStack,
        JsonImportSettings& settings, const AZ::IO::FixedMaxPath& importPath, StackedString& element)
    {
        using namespace JsonSerializationResult;

        for (auto& path : importPathStack)
        {
            if (importPath == path)
            {
                return settings.m_reporting(
                    AZStd::string::format("'%s' was already imported in this chain. This indicates a cyclic dependency.", importPath.c_str()),
                    ResultCode(Tasks::Import, Outcomes::Catastrophic), element);
            }
        }

        importPathStack.push_back(importPath);
        AZ::StackedString importElement(AZ::StackedString::Format::JsonPointer);
        JsonImportSettings nestedImportSettings;
        nestedImportSettings.m_importer = settings.m_importer;
        nestedImportSettings.m_reporting = settings.m_reporting;
        nestedImportSettings.m_resolveFlags = ImportTracking::Dependencies;
        ResultCode result = ResolveImports(jsonDoc, allocator, importPathStack, nestedImportSettings, importElement);
        importPathStack.pop_back();

        if (result.GetOutcome() == Outcomes::Catastrophic)
        {
            return result;
        }

        return ResultCode(Tasks::Import, Outcomes::Success);
    }

    JsonSerializationResult::ResultCode JsonImportResolver::ResolveImports(rapidjson::Value& jsonDoc,
        rapidjson::Document::AllocatorType& allocator, ImportPathStack& importPathStack,
        JsonImportSettings& settings, StackedString& element)
    {
        using namespace JsonSerializationResult;

        if (settings.m_importer == nullptr)
        {
            AZStd::string reportMessage = "Json Importer is nullptr for ResolveImports.";
            if (!importPathStack.empty())
            {
                reportMessage += AZStd::string::format(" Skipping resolving imports for file '%s'.", importPathStack.back().c_str());
            }
            return settings.m_reporting(reportMessage,
                ResultCode(Tasks::Import, Outcomes::Skipped), element);
        }

        if (jsonDoc.IsObject())
        {
            for (auto& field : jsonDoc.GetObject())
            {
                if(strncmp(field.name.GetString(), JsonSerialization::ImportDirectiveIdentifier, field.name.GetStringLength()) == 0)
                {
                    const rapidjson::Value& importDirective = field.value;
                    AZ::IO::FixedMaxPath importAbsPath = importPathStack.back();
                    importAbsPath.RemoveFilename();
                    AZ::IO::FixedMaxPath importName;
                    if (importDirective.IsObject())
                    {
                        auto filenameField = importDirective.FindMember("filename");
                        if (filenameField != importDirective.MemberEnd())
                        {
                            importName = AZ::IO::FixedMaxPath(
                                AZStd::string_view(filenameField->value.GetString(), filenameField->value.GetStringLength()));
                        }
                    }
                    else
                    {
                        importName =
                            AZ::IO::FixedMaxPath(AZStd::string_view(importDirective.GetString(), importDirective.GetStringLength()));
                    }

                    // Resolve the any file @..@ aliases in the relative importName if it starts with one
                    if (auto fileIo = AZ::IO::FileIOBase::GetInstance(); fileIo != nullptr)
                    {
                        // Replace alias doesn't "resolve" the path as FileIOBase::ResolvePath would
                        // It only replaces an alias, it doesn't make a relative path absolute by
                        // making subpath of the asset cache
                        fileIo->ReplaceAlias(importName, importName);
                    }

                    importAbsPath /= importName;

                    rapidjson::Value patch;
                    ResultCode resolveResult = settings.m_importer->ResolveImport(&jsonDoc, patch, importDirective, importAbsPath, allocator, settings);
                    if (resolveResult.GetOutcome() == Outcomes::Catastrophic)
                    {
                        return resolveResult;
                    }

                    if ((settings.m_resolveFlags & ImportTracking::Imports) == ImportTracking::Imports)
                    {
                        rapidjson::Pointer path(element.Get().data(), element.Get().size());
                        settings.m_importer->AddImportDirective(path, importName.String(), importAbsPath.String());
                    }
                    if ((settings.m_resolveFlags & ImportTracking::Dependencies) == ImportTracking::Dependencies)
                    {
                        settings.m_importer->AddImportedFile(importAbsPath.String());
                    }

                    if (settings.m_resolveNestedImports)
                    {
                        if (ResultCode result = ResolveNestedImports(jsonDoc, allocator, importPathStack,
                            settings, importAbsPath, element);
                            result.GetOutcome() == Outcomes::Catastrophic)
                        {
                            return result;
                        }
                    }
                    settings.m_importer->ApplyPatch(jsonDoc, patch, allocator);
                }
                else if (field.value.IsObject() || field.value.IsArray())
                {
                    ScopedStackedString entryName(element, AZStd::string_view(field.name.GetString(), field.name.GetStringLength()));
                    ResultCode result = ResolveImports(field.value, allocator, importPathStack, settings, element);
                    if (result.GetOutcome() == Outcomes::Catastrophic)
                    {
                        return result;
                    }
                }
            }
        }
        else if(jsonDoc.IsArray())
        {
            int index = 0;
            for (rapidjson::Value::ValueIterator elem = jsonDoc.Begin(); elem != jsonDoc.End(); ++elem, ++index)
            {
                if (!elem->IsObject() && !elem->IsArray())
                {
                    continue;
                }
                ScopedStackedString entryName(element, index);
                ResultCode result = ResolveImports(*elem, allocator, importPathStack, settings, element);
                if (result.GetOutcome() == Outcomes::Catastrophic)
                {
                    return result;
                }
            }
        }

        return ResultCode(Tasks::Import, Outcomes::Success);
    }

    // Load the Imported Files and and Merge the JSON contents of the file
    // into the target
    auto JsonImportResolver::LoadAndMergeImportFile(rapidjson::Value& target, rapidjson::Document::AllocatorType& allocator,
        const rapidjson::Value& importDirective, AZ::IO::PathView importAbsPath,
        const AZ::StackedString& element, const JsonSerializationResult::JsonIssueCallback& issueReporter)
        -> JsonSerializationResult::ResultCode
    {
        using ReporterString = AZStd::fixed_string<1024>;

        if (auto importedObject = JsonSerializationUtils::ReadJsonFile(importAbsPath.Native());
            importedObject.IsSuccess())
        {
            JsonApplyPatchSettings applyPatchSettings;
            applyPatchSettings.m_reporting = issueReporter;

            rapidjson::Value& importedDoc = importedObject.GetValue();
            if (importDirective.IsObject())
            {
                auto patchField = importDirective.FindMember("patch");
                if (patchField != importDirective.MemberEnd())
                {
                    JsonSerialization::ApplyPatch(importedDoc, allocator, patchField->value,
                        AZ::JsonMergeApproach::JsonMergePatch, applyPatchSettings);
                }
            }

            constexpr AZStd::string_view JsonPatchExtension = ".jsonpatch";
            constexpr AZStd::string_view JsonPatchExtensionForSetreg = ".setregpatch";
            const auto mergeApproach = importAbsPath.Extension() != JsonPatchExtension
                && importAbsPath.Extension() != JsonPatchExtensionForSetreg
                ? AZ::JsonMergeApproach::JsonMergePatch : AZ::JsonMergeApproach::JsonPatch;

            JsonSerializationResult::ResultCode importResult = JsonSerialization::ApplyPatch(target, allocator, importedDoc,
                mergeApproach, applyPatchSettings);

            if (importResult.GetProcessing() != JsonSerializationResult::Processing::Completed)
            {
                return issueReporter(
                    ReporterString::format(R"(Patching the imported JSON file "%.*s" into the target JSON document has failed.)",
                        AZ_PATH_ARG(importAbsPath)),
                    importResult, element.Get());
            }

            return issueReporter(
                ReporterString::format(R"(Successfully merged imported JSON file "%.*s into the target JSON document".)",
                    AZ_PATH_ARG(importAbsPath)),
                importResult, element.Get());
        }
        else
        {
            return issueReporter(
                ReporterString::format(R"(Failed to import file "%.*s". Reason: "%s")",
                    AZ_PATH_ARG(importAbsPath), importedObject.GetError().c_str()),
                JsonSerializationResult::ResultCode(JsonSerializationResult::Tasks::Import,
                    JsonSerializationResult::Outcomes::Catastrophic), element.Get());
        }
    }

    JsonSerializationResult::ResultCode JsonImportResolver::ResolveNestedImportsInOrder(rapidjson::Value& target,
        rapidjson::Document::AllocatorType& allocator, const rapidjson::Value& source,
        ImportPathStack& importPathStack,
        JsonImportSettings& settings, StackedString& element, AZ::IO::PathView importPath)
    {
        for (auto& path : importPathStack)
        {
            if (importPath == path)
            {
                return settings.m_reporting(
                    AZStd::string::format(R"("%s" was already imported in this chain.)"
                        " This indicates a cyclic dependency.", AZ_PATH_ARG(importPath)),
                    JsonSerializationResult::ResultCode(JsonSerializationResult::Tasks::Import,
                        JsonSerializationResult::Outcomes::Catastrophic), element);
            }
        }

        importPathStack.push_back(importPath);
        AZ::StackedString importElement(AZ::StackedString::Format::JsonPointer);
        JsonImportSettings nestedImportSettings;
        nestedImportSettings.m_importer = settings.m_importer;
        nestedImportSettings.m_reporting = settings.m_reporting;
        nestedImportSettings.m_resolveFlags = ImportTracking::Dependencies;
        JsonSerializationResult::ResultCode result = ResolveImportsInOrder(target, allocator, source, importPathStack, nestedImportSettings, importElement);
        importPathStack.pop_back();

        if (result.GetOutcome() == JsonSerializationResult::Outcomes::Catastrophic)
        {
            return result;
        }

        return JsonSerializationResult::ResultCode(JsonSerializationResult::Tasks::Import,
            JsonSerializationResult::Outcomes::Success);
    }

    auto JsonImportResolver::GetImportPaths(const rapidjson::Value& importDirective,
        const ImportPathStack& importPathStack)
        -> ImportPathsResult
    {
        AZ::IO::FixedMaxPath importAbsPath = importPathStack.back();
        importAbsPath.RemoveFilename();
        AZ::IO::FixedMaxPath importName;
        if (importDirective.IsObject())
        {
            auto filenameField = importDirective.FindMember("filename");
            if (filenameField != importDirective.MemberEnd())
            {
                importName = AZ::IO::FixedMaxPath(AZStd::string_view(
                    filenameField->value.GetString(), filenameField->value.GetStringLength()));
            }
        }
        else
        {
            importName = AZ::IO::FixedMaxPath(AZStd::string_view(
                importDirective.GetString(), importDirective.GetStringLength()));
        }

        // Resolve the any file @..@ aliases in the relative importName if it starts with one
        if (auto fileIo = AZ::IO::FileIOBase::GetInstance(); fileIo != nullptr)
        {
            // Replace alias doesn't "resolve" the path as FileIOBase::ResolvePath would
            // It only replaces an alias, it doesn't make a relative path absolute by
            // making subpath of the asset cache
            fileIo->ReplaceAlias(importName, importName);
        }

        importAbsPath /= importName;

        ImportPathsResult pathsResult;
        pathsResult.m_importAbsPath = importAbsPath;
        pathsResult.m_importRelPath = importName;

        return pathsResult;
    }

    JsonSerializationResult::ResultCode JsonImportResolver::ResolveImportsInOrder(rapidjson::Value& target,
        rapidjson::Document::AllocatorType& allocator, const rapidjson::Value& source,
        ImportPathStack& importPathStack,
        JsonImportSettings& settings, StackedString& element)
    {
        using namespace JsonSerializationResult;

        using ReporterString = AZStd::fixed_string<1024>;

        if (settings.m_importer == nullptr)
        {
            ReporterString reportMessage = "Json Importer is nullptr for ResolveImportsInOrder.";
            if (!importPathStack.empty())
            {
                reportMessage += ReporterString::format(R"( Cannot resolve imports in field order for file "%s".)", importPathStack.back().c_str());
            }
            return settings.m_reporting(reportMessage,
                ResultCode(Tasks::Import, Outcomes::Skipped), element);
        }

        // If the source json is neither and Object or an Array
        // Just make a copy of the data and return
        if (!source.IsObject() && !source.IsArray())
        {
            target.CopyFrom(source, allocator, true);
            return ResultCode(Tasks::Import, Outcomes::Success);
        }

        // Stores the currently copied field from the source object or array
        // into a local JSON value that will be moved to the target JSON value at the end of this function
        rapidjson::Value targetDataAtCurrentDepth{ rapidjson::kObjectType };

        // If this point is reached, the source is either an object or an array
        size_t fieldCount = source.IsObject() ? source.MemberCount() : source.Size();
        for (size_t fieldIndex = 0; fieldIndex < fieldCount; ++fieldIndex)
        {
            // Used to store the converted field name for an array field
            using FieldNameString = AZStd::fixed_string<256>;
            // The field name storage lifetime is larger than the fieldName string_view that references
            // its string data
            FieldNameString fieldNameStorage;
            AZStd::string_view fieldName;

            const rapidjson::Value* fieldValue{};
            if (source.IsObject())
            {
                const rapidjson::Value::Member& objectField = *AZStd::next(source.MemberBegin(), fieldIndex);
                // Reference the field name and field value from the GenericMember object
                fieldName = AZStd::string_view(objectField.name.GetString(), objectField.name.GetStringLength());
                fieldValue = &objectField.value;
            }
            else
            {
                AZStd::to_string(fieldNameStorage, fieldIndex);
                fieldName = fieldNameStorage;
                // Reference the field value from the GenericValue field
                const rapidjson::Value& arrayField = *AZStd::next(source.Begin(), fieldIndex);
                fieldValue = &arrayField;
            }

            if (fieldName == JsonSerialization::ImportDirectiveIdentifier)
            {
                const rapidjson::Value& importDirective = *fieldValue;
                ImportPathsResult pathsResult = GetImportPaths(importDirective, importPathStack);
                const auto& importAbsPath = pathsResult.m_importAbsPath;
                const auto& importName = pathsResult.m_importRelPath;

                if (ResultCode resolveResult = LoadAndMergeImportFile(targetDataAtCurrentDepth, allocator, importDirective,
                    importAbsPath, element, settings.m_reporting);
                    resolveResult.GetOutcome() == Outcomes::Catastrophic)
                {
                    return resolveResult;
                }

                if ((settings.m_resolveFlags & ImportTracking::Imports) == ImportTracking::Imports)
                {
                    rapidjson::Pointer path(element.Get().data(), element.Get().size());
                    settings.m_importer->AddImportDirective(path, importName.String(), importAbsPath.String());
                }
                if ((settings.m_resolveFlags & ImportTracking::Dependencies) == ImportTracking::Dependencies)
                {
                    settings.m_importer->AddImportedFile(importAbsPath.String());
                }

                if (settings.m_resolveNestedImports)
                {
                    // Resolve the nested import by using the targetDataAtCurrentDepth as both the source
                    // object to iterate over and the target object to copy to.
                    // As the targetDataAtCurrentDepth has merged the current imported file,
                    // the call will search for any $import directives in that imported file
                    // and recursively resolve them
                    ResultCode result = ResolveNestedImportsInOrder(targetDataAtCurrentDepth, allocator,
                        targetDataAtCurrentDepth, importPathStack,
                        settings, element, importAbsPath);
                    if (result.GetOutcome() == Outcomes::Catastrophic)
                    {
                        return result;
                    }
                }
            }
            else
            {
                // Check if the target data already contains a field with the same name
                auto fieldNameStringRef = rapidjson::Value::StringRefType(fieldName.data(),
                    static_cast<rapidjson::SizeType>(fieldName.size()));
                auto targetField = targetDataAtCurrentDepth.FindMember(fieldNameStringRef);
                // Initialize the Merge Patch result to success
                // Any recursive calls to ResolveImportsInOrder would be responsible
                // for converting the result code to failure
                ResultCode mergePatchResult{ Tasks::Import, Outcomes::Success };
                if (fieldValue->IsObject() && targetField != targetDataAtCurrentDepth.MemberEnd())
                {
                    mergePatchResult.Combine(ResolveImportsInOrder(targetField->value, allocator, *fieldValue,
                        importPathStack, settings, element));
                }
                else if (fieldValue->IsObject() || fieldValue->IsArray())
                {
                    // And Object or Array is checked for children $import directives
                    rapidjson::Value name;
                    name.CopyFrom(rapidjson::Value(fieldNameStringRef), allocator, true);
                    rapidjson::Value value;
                    // Add an empty object field and then use ResolveImportsInOrder to merge to it.
                    targetDataAtCurrentDepth.AddMember(AZStd::move(name),
                        rapidjson::Value{ rapidjson::kObjectType }, allocator);
                    auto newFieldMemberIter = targetDataAtCurrentDepth.FindMember(fieldNameStringRef);
                    if (newFieldMemberIter == targetDataAtCurrentDepth.MemberEnd())
                    {
                        auto reportMessage = ReporterString::format("Failed to Add Member %.*s to JSON while importing.",
                            AZ_STRING_ARG(fieldName));
                        if (!importPathStack.empty())
                        {
                            reportMessage += ReporterString::format(R"( This occured while importing file "%s".)",
                                importPathStack.back().c_str());
                        }
                        return settings.m_reporting(reportMessage, ResultCode(Tasks::Import, Outcomes::Catastrophic), element);
                    }

                    mergePatchResult.Combine(ResolveImportsInOrder(newFieldMemberIter->value, allocator, *fieldValue,
                        importPathStack, settings, element));
                }
                else if (fieldValue->IsNull())
                {
                    if (targetField != targetDataAtCurrentDepth.MemberEnd())
                    {
                        targetDataAtCurrentDepth.RemoveMember(targetField);
                    }
                }
                else
                {
                    if (targetField != targetDataAtCurrentDepth.MemberEnd())
                    {
                        targetField->value.CopyFrom(*fieldValue, allocator, true);
                    }
                    else
                    {
                        rapidjson::Value name;
                        rapidjson::Value value;
                        name.CopyFrom(rapidjson::Value(fieldNameStringRef), allocator, true);
                        value.CopyFrom(*fieldValue, allocator, true);
                        targetDataAtCurrentDepth.AddMember(AZStd::move(name), AZStd::move(value), allocator);

                    }
                }
            }
        }

        // Move the local target data json value to the target result variable
        target = AZStd::move(targetDataAtCurrentDepth);

        return ResultCode(Tasks::Import, Outcomes::Success);
    }

    JsonSerializationResult::ResultCode JsonImportResolver::StoreNestedImports(const rapidjson::Value& jsonDoc,
        ImportPathStack& importPathStack,
        JsonImportSettings& settings, const AZ::IO::FixedMaxPath& importPath, StackedString& element)
    {
        using namespace JsonSerializationResult;

        for (auto& path : importPathStack)
        {
            if (importPath == path)
            {
                return settings.m_reporting(
                    AZStd::string::format("'%s' was already imported in this chain. This indicates a cyclic dependency.", importPath.c_str()),
                    ResultCode(Tasks::Import, Outcomes::Catastrophic), element);
            }
        }

        importPathStack.push_back(importPath);
        AZ::StackedString importElement(AZ::StackedString::Format::JsonPointer);
        JsonImportSettings nestedImportSettings;
        nestedImportSettings.m_importer = settings.m_importer;
        nestedImportSettings.m_reporting = settings.m_reporting;
        nestedImportSettings.m_resolveFlags = ImportTracking::All;
        ResultCode result = StoreImports(jsonDoc, importPathStack, nestedImportSettings, importElement);
        importPathStack.pop_back();

        if (result.GetOutcome() == Outcomes::Catastrophic)
        {
            return result;
        }

        return ResultCode(Tasks::Import, Outcomes::Success);
    }

    JsonSerializationResult::ResultCode JsonImportResolver::StoreImports(const rapidjson::Value& jsonDoc,
        ImportPathStack& importPathStack,
        JsonImportSettings& settings, StackedString& element)
    {
        using namespace JsonSerializationResult;

        if (settings.m_importer == nullptr)
        {
            AZStd::string reportMessage = "Json Importer is nullptr for StoreImports.";
            if (!importPathStack.empty())
            {
                reportMessage += AZStd::string::format(" Skipping storing imports for file '%s'.", importPathStack.back().c_str());
            }
            return settings.m_reporting(reportMessage,
                ResultCode(Tasks::Import, Outcomes::Skipped), element);
        }

        if (jsonDoc.IsObject())
        {
            for (auto& field : jsonDoc.GetObject())
            {
                if (AZStd::string_view fieldName(field.name.GetString(), field.name.GetStringLength());
                    fieldName == JsonSerialization::ImportDirectiveIdentifier)
                {
                    // Read the import directive path from the $import field
                    const rapidjson::Value& importDirective = field.value;
                    AZ::IO::FixedMaxPath importAbsPath = importPathStack.back();
                    importAbsPath.RemoveFilename();
                    AZ::IO::FixedMaxPath importName;
                    if (importDirective.IsObject())
                    {
                        auto filenameField = importDirective.FindMember("filename");
                        if (filenameField != importDirective.MemberEnd())
                        {
                            importName = AZ::IO::FixedMaxPath(
                                AZStd::string_view(filenameField->value.GetString(), filenameField->value.GetStringLength()));
                        }
                    }
                    else
                    {
                        importName =
                            AZ::IO::FixedMaxPath(AZStd::string_view(importDirective.GetString(), importDirective.GetStringLength()));
                    }

                    // Resolve the any file @..@ aliases in the relative importName if it starts with one
                    if (auto fileIo = AZ::IO::FileIOBase::GetInstance(); fileIo != nullptr)
                    {
                        // Replace alias doesn't "resolve" the path as FileIOBase::ResolvePath would
                        // It only replaces an alias, it doesn't make a relative path absolute by
                        // making subpath of the asset cache
                        fileIo->ReplaceAlias(importName, importName);
                    }

                    // Create an absolute path using the directoy of the current JSON file and appending the $import
                    // directive path to it.
                    importAbsPath /= importName;

                    if ((settings.m_resolveFlags & ImportTracking::Imports) == ImportTracking::Imports)
                    {
                        rapidjson::Pointer path(element.Get().data(), element.Get().size());
                        settings.m_importer->AddImportDirective(path, importName.String(), AZStd::move(importAbsPath).String());
                    }
                    if ((settings.m_resolveFlags & ImportTracking::Dependencies) == ImportTracking::Dependencies)
                    {
                        settings.m_importer->StoreImport(importAbsPath);
                    }

                    if (settings.m_resolveNestedImports)
                    {
                        if (ResultCode result = StoreNestedImports(jsonDoc, importPathStack,
                            settings, importAbsPath, element);
                            result.GetOutcome() == Outcomes::Catastrophic)
                        {
                            return result;
                        }
                    }
                }
                else if (field.value.IsObject() || field.value.IsArray())
                {
                    ScopedStackedString entryName(element, AZStd::string_view(field.name.GetString(), field.name.GetStringLength()));
                    ResultCode result = StoreImports(field.value, importPathStack, settings, element);
                    if (result.GetOutcome() == Outcomes::Catastrophic)
                    {
                        return result;
                    }
                }
            }
        }
        else if (jsonDoc.IsArray())
        {
            int index = 0;
            for (rapidjson::Value::ConstValueIterator elem = jsonDoc.Begin(); elem != jsonDoc.End(); ++elem, ++index)
            {
                if (!elem->IsObject() && !elem->IsArray())
                {
                    continue;
                }
                ScopedStackedString entryName(element, index);
                ResultCode result = StoreImports(*elem, importPathStack, settings, element);
                if (result.GetOutcome() == Outcomes::Catastrophic)
                {
                    return result;
                }
            }
        }

        return ResultCode(Tasks::Import, Outcomes::Success);
    }

    JsonSerializationResult::ResultCode JsonImportResolver::RestoreImports(rapidjson::Value& jsonDoc,
        rapidjson::Document::AllocatorType& allocator, JsonImportSettings& settings)
    {
        using namespace JsonSerializationResult;

        if (settings.m_importer == nullptr)
        {
            AZStd::string reportMessage = "Json Importer is nullptr for RestoreImports.";
            return settings.m_reporting(reportMessage,
                ResultCode(Tasks::Import, Outcomes::Skipped), "");
        }

        if (jsonDoc.IsObject() || jsonDoc.IsArray())
        {
            const BaseJsonImporter::ImportDirectivesList& importDirectives = settings.m_importer->GetImportDirectives();
            for (auto& import : importDirectives)
            {
                rapidjson::Pointer importPtr = import.m_importDirectiveJsonPath;
                rapidjson::Value* currentValue = importPtr.Get(jsonDoc);

                rapidjson::Value importedValue(rapidjson::kObjectType);
                importedValue.AddMember(rapidjson::StringRef(JsonSerialization::ImportDirectiveIdentifier),
                    rapidjson::StringRef(import.m_importDirectiveFilePath.c_str()), allocator);
                ResultCode resolveResult = JsonSerialization::ResolveImports(importedValue, allocator, settings);
                if (resolveResult.GetOutcome() == Outcomes::Catastrophic)
                {
                    return resolveResult;
                }

                rapidjson::Value patch;
                settings.m_importer->CreatePatch(patch, importedValue, *currentValue, allocator);
                settings.m_importer->RestoreImport(currentValue, patch, allocator, import.m_importDirectiveFilePath);
            }
        }

        return ResultCode(Tasks::Import, Outcomes::Success);
    }

    JsonSerializationResult::ResultCode BaseJsonImporter::ResolveImport(rapidjson::Value* importPtr,
        rapidjson::Value& patch, const rapidjson::Value& importDirective,
        const AZ::IO::FixedMaxPath& importedFilePath, rapidjson::Document::AllocatorType& allocator,
        JsonImportSettings& settings)
    {
        using namespace JsonSerializationResult;

        // Attempt to replace any file aliases at the beginning of the path
        // with a mapped file path if it exist.
        AZ::IO::FixedMaxPath unaliasedImportedPath;
        if (auto fileIo = AZ::IO::FileIOBase::GetInstance(); fileIo != nullptr)
        {
            // Replace alias doesn't "resolve" the path as FileIOBase::ResolvePath would
            // It only replaces an alias, it doesn't make a relative path absolute by
            // making subpath of the asset cache
            fileIo->ReplaceAlias(unaliasedImportedPath, importedFilePath);
        }
        else
        {
            unaliasedImportedPath = importedFilePath;
        }

        auto importedObject = JsonSerializationUtils::ReadJsonFile(unaliasedImportedPath.Native());
        if (importedObject.IsSuccess())
        {
            rapidjson::Value& importedDoc = importedObject.GetValue();

            if (importDirective.IsObject())
            {
                auto patchField = importDirective.FindMember("patch");
                if (patchField != importDirective.MemberEnd())
                {
                    patch.CopyFrom(patchField->value, allocator);
                }
            }

            importPtr->CopyFrom(importedDoc, allocator);
        }
        else
        {
            return settings.m_reporting(
                AZStd::string::format("Failed to import file '%s'. Reason: '%s'", importedFilePath.c_str(), importedObject.GetError().c_str()),
                ResultCode(Tasks::Import, Outcomes::Catastrophic), importedFilePath.Native());
        }

        return ResultCode(Tasks::Import, Outcomes::Success);
    }

    JsonSerializationResult::ResultCode BaseJsonImporter::StoreImport(AZ::IO::PathView importedFilePath)
    {
        using namespace JsonSerializationResult;

        // Attempt to replace any file aliases at the beginning of the path
        // with a mapped file path if it exist.
        AZ::IO::FixedMaxPath unaliasedImportedPath;
        if (auto fileIo = AZ::IO::FileIOBase::GetInstance(); fileIo != nullptr)
        {
            // Replace alias doesn't "resolve" the path as FileIOBase::ResolvePath would
            // It only replaces an alias, it doesn't make a relative path absolute by
            // making subpath of the asset cache
            fileIo->ReplaceAlias(unaliasedImportedPath, importedFilePath);
        }
        else
        {
            unaliasedImportedPath = importedFilePath;
        }

        m_importedFiles.emplace(unaliasedImportedPath.String());

        return ResultCode(Tasks::Import, Outcomes::Success);
    }

    JsonSerializationResult::ResultCode BaseJsonImporter::RestoreImport(rapidjson::Value* importPtr,
        rapidjson::Value& patch, rapidjson::Document::AllocatorType& allocator, AZ::IO::PathView importFilename)
    {
        using namespace JsonSerializationResult;

        importPtr->SetObject();
        if ((patch.IsObject() && patch.MemberCount() > 0) || (patch.IsArray() && !patch.Empty()))
        {
            rapidjson::Value importDirective(rapidjson::kObjectType);
            importDirective.AddMember(rapidjson::StringRef("filename"),
                rapidjson::StringRef(importFilename.Native().data(), importFilename.Native().size()),
                allocator);
            importDirective.AddMember(rapidjson::StringRef("patch"), patch, allocator);
            importPtr->AddMember(rapidjson::StringRef(JsonSerialization::ImportDirectiveIdentifier), importDirective, allocator);
        }
        else
        {
            importPtr->AddMember(rapidjson::StringRef(JsonSerialization::ImportDirectiveIdentifier),
                rapidjson::StringRef(importFilename.Native().data(), importFilename.Native().size()),
                allocator);
        }

        return ResultCode(Tasks::Import, Outcomes::Success);
    }

    JsonSerializationResult::ResultCode BaseJsonImporter::ApplyPatch(rapidjson::Value& target,
        const rapidjson::Value& patch, rapidjson::Document::AllocatorType& allocator)
    {
        using namespace JsonSerializationResult;

        if ((patch.IsObject() && patch.MemberCount() > 0) || (patch.IsArray() && !patch.Empty()))
        {
            return AZ::JsonSerialization::ApplyPatch(target, allocator, patch, JsonMergeApproach::JsonMergePatch);
        }

        return ResultCode(Tasks::Import, Outcomes::Success);
    }

    JsonSerializationResult::ResultCode BaseJsonImporter::CreatePatch(rapidjson::Value& patch,
        const rapidjson::Value& source, const rapidjson::Value& target,
        rapidjson::Document::AllocatorType& allocator)
    {
        return JsonSerialization::CreatePatch(patch, allocator, source, target, JsonMergeApproach::JsonMergePatch);
    }

    void BaseJsonImporter::AddImportDirective(const rapidjson::Pointer& jsonPtr, AZ::IO::Path importFile,
        AZ::IO::Path resolvedFile)
    {
        auto IsNewImportDirective = [&jsonPtr, &importFile](const JsonImportMetadata& importMetadata)
        {
            return jsonPtr == importMetadata.m_importDirectiveJsonPath
                && importFile == importMetadata.m_importDirectiveFilePath;
        };
        if (auto foundImportIt = AZStd::find_if(m_importDirectives.begin(), m_importDirectives.end(), IsNewImportDirective);
            foundImportIt == m_importDirectives.end())
        {
            m_importDirectives.push_back({ jsonPtr, AZStd::move(importFile), AZStd::move(resolvedFile) });
        }
    }

    void BaseJsonImporter::AddImportedFile(AZ::IO::Path importedFile)
    {
        m_importedFiles.insert(AZStd::move(importedFile));
    }

    const BaseJsonImporter::ImportDirectivesList& BaseJsonImporter::GetImportDirectives()
    {
        return m_importDirectives;
    }

    const BaseJsonImporter::ImportedFilesList& BaseJsonImporter::GetImportedFiles()
    {
        return m_importedFiles;
    }
} // namespace AZ
