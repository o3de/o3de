/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/Json/JsonImporter.h>
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
        
        if (jsonDoc.IsObject())
        {
            for (auto& field : jsonDoc.GetObject())
            {
                if(strncmp(field.name.GetString(), JsonSerialization::ImportDirectiveIdentifier, field.name.GetStringLength()) == 0)
                {
                    const rapidjson::Value& importDirective = field.value;
                    AZ::IO::FixedMaxPath importAbsPath = importPathStack.back();
                    importAbsPath.RemoveFilename();
                    AZStd::string importName;
                    if (importDirective.IsObject())
                    {
                        auto filenameField = importDirective.FindMember("filename");
                        if (filenameField != importDirective.MemberEnd())
                        {
                            importName = AZStd::string(filenameField->value.GetString(), filenameField->value.GetStringLength());
                        }
                    }
                    else
                    {
                        importName = AZStd::string(importDirective.GetString(), importDirective.GetStringLength());
                    }
                    importAbsPath.Append(importName);

                    rapidjson::Value patch;
                    ResultCode resolveResult = settings.m_importer->ResolveImport(&jsonDoc, patch, importDirective, importAbsPath, allocator);
                    if (resolveResult.GetOutcome() == Outcomes::Catastrophic)
                    {
                        return resolveResult;
                    }

                    if ((settings.m_resolveFlags & ImportTracking::Imports) == ImportTracking::Imports)
                    {
                        rapidjson::Pointer path(element.Get().data(), element.Get().size());
                        settings.m_importer->AddImportDirective(path, importName);
                    }
                    if ((settings.m_resolveFlags & ImportTracking::Dependencies) == ImportTracking::Dependencies)
                    {
                        settings.m_importer->AddImportedFile(importAbsPath.String());
                    }

                    ResultCode result = ResolveNestedImports(jsonDoc, allocator, importPathStack, settings, importAbsPath, element);
                    if (result.GetOutcome() == Outcomes::Catastrophic)
                    {
                        return result;
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

    JsonSerializationResult::ResultCode JsonImportResolver::RestoreImports(rapidjson::Value& jsonDoc,
        rapidjson::Document::AllocatorType& allocator, JsonImportSettings& settings)
    {
        using namespace JsonSerializationResult;

        if (jsonDoc.IsObject() || jsonDoc.IsArray())
        {
            const BaseJsonImporter::ImportDirectivesList& importDirectives = settings.m_importer->GetImportDirectives();
            for (auto& import : importDirectives)
            {
                rapidjson::Pointer importPtr = import.first;
                rapidjson::Value* currentValue = importPtr.Get(jsonDoc);
                
                rapidjson::Value importedValue(rapidjson::kObjectType);
                importedValue.AddMember(rapidjson::StringRef(JsonSerialization::ImportDirectiveIdentifier), rapidjson::StringRef(import.second.c_str()), allocator);
                ResultCode resolveResult = JsonSerialization::ResolveImports(importedValue, allocator, settings);
                if (resolveResult.GetOutcome() == Outcomes::Catastrophic)
                {
                    return resolveResult;
                }

                rapidjson::Value patch;
                settings.m_importer->CreatePatch(patch, importedValue, *currentValue, allocator);
                settings.m_importer->RestoreImport(currentValue, patch, allocator, import.second);
            }
        }

        return ResultCode(Tasks::Import, Outcomes::Success);
    }

    JsonSerializationResult::ResultCode BaseJsonImporter::ResolveImport(rapidjson::Value* importPtr,
        rapidjson::Value& patch, const rapidjson::Value& importDirective,
        const AZ::IO::FixedMaxPath& importedFilePath, rapidjson::Document::AllocatorType& allocator)
    {
        using namespace JsonSerializationResult;

        auto importedObject = JsonSerializationUtils::ReadJsonFile(importedFilePath.Native());
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
            return ResultCode(Tasks::Import, Outcomes::Catastrophic);
        }

        return ResultCode(Tasks::Import, Outcomes::Success);
    }

    JsonSerializationResult::ResultCode BaseJsonImporter::RestoreImport(rapidjson::Value* importPtr,
        rapidjson::Value& patch, rapidjson::Document::AllocatorType& allocator, const AZStd::string& importFilename)
    {
        using namespace JsonSerializationResult;
        
        importPtr->SetObject();
        if ((patch.IsObject() && patch.MemberCount() > 0) || (patch.IsArray() && !patch.Empty()))
        {
            rapidjson::Value importDirective(rapidjson::kObjectType);
            importDirective.AddMember(rapidjson::StringRef("filename"), rapidjson::StringRef(importFilename.c_str()), allocator);
            importDirective.AddMember(rapidjson::StringRef("patch"), patch, allocator);
            importPtr->AddMember(rapidjson::StringRef(JsonSerialization::ImportDirectiveIdentifier), importDirective, allocator);
        }
        else
        {
            importPtr->AddMember(rapidjson::StringRef(JsonSerialization::ImportDirectiveIdentifier), rapidjson::StringRef(importFilename.c_str()), allocator);
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

    void BaseJsonImporter::AddImportDirective(const rapidjson::Pointer& jsonPtr, AZStd::string importFile)
    {
        m_importDirectives.emplace_back(jsonPtr, AZStd::move(importFile));
    }

    void BaseJsonImporter::AddImportedFile(AZStd::string importedFile)
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
