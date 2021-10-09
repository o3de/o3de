/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/Json/JsonImporter.h>
#include <AzCore/Serialization/Json/JsonUtils.h>

namespace AZ
{
    bool JsonImportResolver::ResolveNestedImports(rapidjson::Value& jsonDoc,
        rapidjson::Document::AllocatorType& allocator, ImportPathStack& importPathStack,
        BaseJsonImporter* importer, const AZ::IO::FixedMaxPath& importPath)
    {
        for (auto& path : importPathStack)
        {
            if (importPath.Compare(path) == 0)
            {
                AZ_Error("JsonImportResolver", false, "%s was already imported in this chain. This indicates a cyclic dependency.", importPath.c_str());
                return false;
            }
        }

        importPathStack.push_back(importPath);
        AZ::StackedString importElement(AZ::StackedString::Format::JsonPointer);
        ResolveImports(jsonDoc, allocator, importPathStack, importer, importElement, TRACK_DEPENDENCIES);
        importPathStack.pop_back();
        
        return true;
    }

    bool JsonImportResolver::ResolveImports(rapidjson::Value& jsonDoc,
        rapidjson::Document::AllocatorType& allocator, ImportPathStack& importPathStack,
        BaseJsonImporter* importer, StackedString& element, AZ::u8 loadFlags)
    {
        if (jsonDoc.IsObject())
        {
            for (auto& field : jsonDoc.GetObject())
            {
                if(strcmp(field.name.GetString(), "$import") == 0)
                {
                    const rapidjson::Value& importDirective = field.value;
                    AZ::IO::FixedMaxPath importPath = importPathStack.back();
                    importPath.RemoveFilename();
                    if (importDirective.IsObject())
                    {
                        auto filenameField = importDirective.FindMember("filename");
                        if (filenameField != importDirective.MemberEnd())
                        {
                            importPath.Append(filenameField->value.GetString());
                        }
                    }
                    else
                    {
                        importPath.Append(importDirective.GetString());
                    }

                    importer->ResolveImport(jsonDoc, importDirective, importPath, allocator);

                    if (loadFlags & TRACK_IMPORTS)
                    {
                        rapidjson::Pointer path(element.Get().data(), element.Get().size());
                        AZStd::string importField = AZStd::string(field.value.GetString());
                        importer->AddImportDirective(path, importField);
                    }
                    if (loadFlags & TRACK_DEPENDENCIES)
                    {
                        importer->AddImportedFile(importPath.String());
                    }

                    ResolveNestedImports(jsonDoc, allocator, importPathStack, importer, importPath);
                }
                else if (field.value.IsObject() || field.value.IsArray())
                {
                    ScopedStackedString entryName(element, AZStd::string_view(field.name.GetString(), field.name.GetStringLength()));
                    ResolveImports(field.value, allocator, importPathStack, importer, element, loadFlags);
                }
            }
        }
        else if(jsonDoc.IsArray())
        {
            int index = 0;
            for (rapidjson::Value::ValueIterator elem = jsonDoc.Begin(); elem != jsonDoc.End(); elem++, index++)
            {
                if (!elem->IsObject() && !elem->IsArray())
                {
                    continue;
                }
                AZStd::string indexStr = AZStd::to_string(index);
                ScopedStackedString entryName(element, AZStd::string_view(indexStr.c_str(), indexStr.size()));
                ResolveImports(*elem, allocator, importPathStack, importer, element, loadFlags);
            }
        }

        return true;
    }

    bool JsonImportResolver::RestoreImports(rapidjson::Value& jsonDoc,
        rapidjson::Document::AllocatorType& allocator, const AZ::IO::FixedMaxPath& loadedJsonPath,
        BaseJsonImporter* importer)
    {
        if (jsonDoc.IsObject() || jsonDoc.IsArray())
        {
            const BaseJsonImporter::ImportDirectivesList& importDirectives = importer->GetImportDirectives();
            for (auto import = importDirectives.begin(); import != importDirectives.end(); import++)
            {
                rapidjson::Pointer importPtr = import->first;
                rapidjson::Value* currentValue = importPtr.Get(jsonDoc);
                
                rapidjson::Value importedValue(rapidjson::kObjectType);
                importedValue.AddMember(rapidjson::StringRef("$import"), rapidjson::StringRef(import->second.c_str()), allocator);
                AZ::StackedString importElement(AZ::StackedString::Format::JsonPointer);
                ImportPathStack importPathStack;
                importPathStack.push_back(loadedJsonPath);
                ResolveImports(importedValue, allocator, importPathStack, importer, importElement, TRACK_NONE);

                rapidjson::Value importDirective(rapidjson::kObjectType);
                importer->RestoreImport(importDirective, *currentValue, importedValue, allocator, import->second);
                
                currentValue->SetObject();
                if (importDirective.IsObject() && importDirective.MemberCount() > 0)
                {
                    currentValue->AddMember(rapidjson::StringRef("$import"), importDirective, allocator);
                }
                else
                {
                    currentValue->AddMember(rapidjson::StringRef("$import"), rapidjson::StringRef(import->second.c_str()), allocator);
                }
            }
        }

        return true;
    }

    JsonSerializationResult::ResultCode BaseJsonImporter::ResolveImport(rapidjson::Value& importedValueOut,
        const rapidjson::Value& importDirective, const AZ::IO::FixedMaxPath& importedFilePath,
        rapidjson::Document::AllocatorType& allocator)
    {
        JsonSerializationResult::ResultCode resultCode(JsonSerializationResult::Tasks::Import);

        auto importedObject = JsonSerializationUtils::ReadJsonFile(importedFilePath.Native());
        if (importedObject.IsSuccess())
        {
            rapidjson::Value& importedDoc = importedObject.GetValue();

            rapidjson::Value patch;
            if (importDirective.IsObject())
            {
                auto patchField = importDirective.FindMember("patch");
                if (patchField != importDirective.MemberEnd())
                {
                    patch.CopyFrom(patchField->value, allocator);
                }
            }

            if ((patch.IsObject() && patch.MemberCount() > 0) || (patch.IsArray() && !patch.Empty()))
            {
                AZ::JsonSerialization::ApplyPatch(importedDoc, allocator, patch, JsonMergeApproach::JsonMergePatch);
            }
            importedValueOut.CopyFrom(importedDoc, allocator);
        }

        return resultCode;
    }

    JsonSerializationResult::ResultCode BaseJsonImporter::RestoreImport(rapidjson::Value& importDirectiveOut,
        const rapidjson::Value& currentValue, const rapidjson::Value& importedValue,
        rapidjson::Document::AllocatorType& allocator, const AZStd::string& importFilename)
    {
        JsonSerializationResult::ResultCode resultCode(JsonSerializationResult::Tasks::Import);

        rapidjson::Value patch;
        JsonSerialization::CreatePatch(patch, allocator, importedValue, currentValue, JsonMergeApproach::JsonMergePatch);
        if ((patch.IsObject() && patch.MemberCount() > 0) || (patch.IsArray() && !patch.Empty()))
        {
            importDirectiveOut.AddMember(rapidjson::StringRef("filename"), rapidjson::StringRef(importFilename.c_str()), allocator);
            importDirectiveOut.AddMember(rapidjson::StringRef("patch"), patch, allocator);
        }

        return resultCode;
    }

    void BaseJsonImporter::AddImportDirective(const rapidjson::Pointer& jsonPtr, const AZStd::string& importFile)
    {
        m_importDirectives.emplace_back(AZStd::make_pair(jsonPtr, importFile));
    }

    void BaseJsonImporter::AddImportedFile(const AZStd::string& importedFile)
    {
        m_importedFiles.insert(importedFile);
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
