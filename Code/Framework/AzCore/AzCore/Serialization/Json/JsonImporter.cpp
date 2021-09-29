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
    bool JsonImportResolver::LoadImports(rapidjson::Value& jsonDoc, StackedString& element, rapidjson::Document::AllocatorType& allocator)
    {
        if (jsonDoc.IsObject())
        {
            for (auto& field : jsonDoc.GetObject())
            {
                if(strcmp(field.name.GetString(), "$import") == 0)
                {
                    rapidjson::Value importedValue;
                    rapidjson::Pointer path(element.Get().data(), element.Get().size());
                    m_importerObj->Load(jsonDoc, field.value, path, allocator);
                    m_importPaths.emplace_back(AZStd::make_pair(path, field.value.GetString()));
                }
                else if (field.value.IsObject() || field.value.IsArray())
                {
                    ScopedStackedString entryName(element, AZStd::string_view(field.name.GetString(), field.name.GetStringLength()));
                    LoadImports(field.value, element, allocator);
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
                LoadImports(*elem, element, allocator);
            }
        }

        return true;
    }

    bool JsonImportResolver::StoreImports(rapidjson::Value& jsonDoc, StackedString& element, rapidjson::Document::AllocatorType& allocator)
    {
        if (jsonDoc.IsObject())
        {
            for (auto import = m_importPaths.begin(); import != m_importPaths.end(); import++)
            {
                rapidjson::Pointer importPath = import->first;
                rapidjson::Value importDirective, storedValue;
                rapidjson::Value* currentValue = importPath.Get(jsonDoc);
                m_importerObj->Store(importDirective, *currentValue, importPath, allocator, import->second);
                currentValue->SetObject();
                if (!importDirective.IsObject())
                {
                    currentValue->AddMember(rapidjson::StringRef("$import"), rapidjson::StringRef(import->second.c_str()), allocator);
                }
                else
                {
                    currentValue->AddMember(rapidjson::StringRef("$import"), importDirective, allocator);
                }
            }
        }

        return true;
    }

    JsonSerializationResult::ResultCode BaseJsonImporter::Load(rapidjson::Value& importedValueOut, const rapidjson::Value& importDirective, rapidjson::Pointer pathToImportDirective, rapidjson::Document::AllocatorType& allocator)
    {
        JsonSerializationResult::ResultCode resultCode(JsonSerializationResult::Tasks::Import);
        AZ::IO::FixedMaxPath filePath = m_loadedJsonPath.RemoveFilename();
        rapidjson::Value patch;
        bool hasPatch = false;
        if (importDirective.IsObject())
        {
            auto filenameField = importDirective.FindMember("filename");
            if (filenameField != importDirective.MemberEnd())
            {
                filePath.Append(filenameField->value.GetString());
            }
            auto patchField = importDirective.FindMember("patch");
            if (patchField != importDirective.MemberEnd())
            {
                patch.CopyFrom(patchField->value, allocator);
                hasPatch = true;
            }
        }
        else
        {
            filePath.Append(importDirective.GetString());
        }

        auto importedObject = JsonSerializationUtils::ReadJsonFile(filePath.Native());
        if (importedObject.IsSuccess())
        {
            rapidjson::Value& importedDoc = importedObject.GetValue();
            if (hasPatch)
            {
                AZ::JsonSerialization::ApplyPatch(importedDoc, allocator, patch, JsonMergeApproach::JsonMergePatch);
            }
            importedValueOut.CopyFrom(importedDoc, allocator);
        }

        return resultCode;
    }

    JsonSerializationResult::ResultCode BaseJsonImporter::Store(rapidjson::Value& importDirectiveOut, const rapidjson::Value& importedValue, rapidjson::Pointer pathToImportDirective, rapidjson::Document::AllocatorType& allocator, AZStd::string& importFilename)
    {
        JsonSerializationResult::ResultCode resultCode(JsonSerializationResult::Tasks::Import);
        AZ::IO::FixedMaxPath filePath = m_loadedJsonPath.RemoveFilename();
        filePath.Append(importFilename.c_str());

        auto importedObject = JsonSerializationUtils::ReadJsonFile(filePath.Native());
        if (importedObject.IsSuccess())
        {
            rapidjson::Value& importedDoc = importedObject.GetValue();
            rapidjson::Value patch;
            JsonSerialization::CreatePatch(patch, allocator, importedDoc, importedValue, JsonMergeApproach::JsonMergePatch);
            if (patch.IsObject())
            {
                importDirectiveOut.AddMember(rapidjson::StringRef("filename"), rapidjson::StringRef(importFilename.c_str()), allocator);
                importDirectiveOut.AddMember(rapidjson::StringRef("patch"), patch, allocator);
            }
        }

        return resultCode;
    }

    void BaseJsonImporter::SetLoadedJsonPath(AZStd::string& loadedJsonPath)
    {
        m_loadedJsonPath = loadedJsonPath;
    }
} // namespace AZ
