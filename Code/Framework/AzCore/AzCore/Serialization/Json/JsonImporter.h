/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include<AzCore/IO/Path/Path.h>
#include <AzCore/JSON/document.h>
#include <AzCore/JSON/pointer.h>
#include <AzCore/Serialization/Json/JsonDeserializer.h>
#include <AzCore/Serialization/Json/JsonSerializer.h>
#include <AzCore/Serialization/Json/JsonSerializationResult.h>

namespace AZ
{
    class BaseJsonImporter
    {
    public:
        AZ_RTTI(BaseJsonImporter, "{7B225807-7B43-430F-8B11-C794DCF5ACA5}");

        virtual JsonSerializationResult::ResultCode Load(rapidjson::Value& importedValueOut, const rapidjson::Value& importDirective, rapidjson::Pointer pathToImportDirective, rapidjson::Document::AllocatorType& allocator);
        virtual JsonSerializationResult::ResultCode Store(rapidjson::Value& importDirectiveOut, const rapidjson::Value& importedValue, rapidjson::Pointer pathToImportDirective, rapidjson::Document::AllocatorType& allocator, AZStd::string& importFilename);
        virtual void SetLoadedJsonPath(AZStd::string& loadedJsonPath);

        virtual ~BaseJsonImporter() = default;

    protected:
        IO::FixedMaxPath m_loadedJsonPath;
    };


    class JsonImportResolver final
    {
        friend class JsonDeserializer;
        friend class JsonSerializer;

    public:
        AZ_RTTI(BaseJsonImporter, "{E855633D-95F6-4EE3-A4AD-4FF29176C4F2}");

        JsonImportResolver() = delete;

        JsonImportResolver(AZStd::string& loadedJsonPath)
        {
            m_importerObj.reset(new BaseJsonImporter);
            m_importerObj->SetLoadedJsonPath(loadedJsonPath);
        }
        
        JsonImportResolver(BaseJsonImporter* jsonImporter)
        {
            m_importerObj.reset(jsonImporter);
        }

        ~JsonImportResolver()
        {
            m_importerObj.reset();
        }

        bool LoadImports(rapidjson::Value& jsonDoc, StackedString& element, rapidjson::Document::AllocatorType& allocator);
        bool StoreImports(rapidjson::Value& jsonDoc, StackedString& element, rapidjson::Document::AllocatorType& allocator);
        
    private:

        AZStd::unique_ptr<BaseJsonImporter> m_importerObj;
        AZStd::vector<AZStd::pair<rapidjson::Pointer, AZStd::string>> m_importPaths;
    };
} // namespace AZ
