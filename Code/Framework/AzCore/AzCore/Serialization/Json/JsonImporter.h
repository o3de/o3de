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

        typedef AZStd::vector<AZStd::pair<rapidjson::Pointer, AZStd::string>> ImportDirectivesList;
        typedef AZStd::unordered_set<AZStd::string> ImportedFilesList;

        virtual JsonSerializationResult::ResultCode ResolveImport(rapidjson::Value& importedValueOut,
            const rapidjson::Value& importDirective, const AZ::IO::FixedMaxPath& importedFilePath,
            rapidjson::Document::AllocatorType& allocator);

        virtual JsonSerializationResult::ResultCode RestoreImport(rapidjson::Value& importDirectiveOut,
            const rapidjson::Value& currentValue, const rapidjson::Value& importedValue,
            rapidjson::Document::AllocatorType& allocator, const AZStd::string& importFilename);

        void AddImportDirective(const rapidjson::Pointer& jsonPtr, const AZStd::string& importFile);
        const ImportDirectivesList& GetImportDirectives();

        void AddImportedFile(const AZStd::string& importedFile);
        const ImportedFilesList& GetImportedFiles();

        virtual ~BaseJsonImporter() = default;

    protected:

        ImportDirectivesList m_importDirectives;
        ImportedFilesList m_importedFiles;
    };


    class JsonImportResolver final
    {
    public:

        static const AZ::u8 TRACK_NONE = 0;
        static const AZ::u8 TRACK_DEPENDENCIES = (1<<0);
        static const AZ::u8 TRACK_IMPORTS = (1<<1);
        static const AZ::u8 TRACK_ALL = (TRACK_DEPENDENCIES | TRACK_IMPORTS);

        typedef AZStd::vector<AZ::IO::FixedMaxPath> ImportPathStack;

        JsonImportResolver() = delete;
        JsonImportResolver& operator=(const JsonImportResolver& rhs) = delete;
        JsonImportResolver& operator=(JsonImportResolver&& rhs) = delete;
        JsonImportResolver(const JsonImportResolver& rhs) = delete;
        JsonImportResolver(JsonImportResolver&& rhs) = delete;
        ~JsonImportResolver() = delete;

        static bool ResolveImports(rapidjson::Value& jsonDoc,
            rapidjson::Document::AllocatorType& allocator, ImportPathStack& importPathStack,
            BaseJsonImporter* importer, StackedString& element, AZ::u8 loadFlags = TRACK_ALL);

        static bool RestoreImports(rapidjson::Value& jsonDoc,
            rapidjson::Document::AllocatorType& allocator, const AZ::IO::FixedMaxPath& loadedJsonPath,
            BaseJsonImporter* importer);
        
    private:

        static bool ResolveNestedImports(rapidjson::Value& jsonDoc,
            rapidjson::Document::AllocatorType& allocator, ImportPathStack& importPathStack,
            BaseJsonImporter* importer, const AZ::IO::FixedMaxPath& importPath);
    };
} // namespace AZ
