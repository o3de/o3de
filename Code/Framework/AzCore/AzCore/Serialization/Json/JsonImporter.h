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
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Serialization/Json/JsonSerializationResult.h>
#include <AzCore/Serialization/Json/StackedString.h>

namespace AZ
{
    struct JsonImportSettings;

    class BaseJsonImporter
    {
    public:
        AZ_RTTI(BaseJsonImporter, "{7B225807-7B43-430F-8B11-C794DCF5ACA5}");

        using ImportDirectivesList = AZStd::vector<AZStd::pair<rapidjson::Pointer, AZStd::string>>;
        using ImportedFilesList = AZStd::unordered_set<AZStd::string>;

        virtual JsonSerializationResult::ResultCode ResolveImport(rapidjson::Value* importPtr,
            rapidjson::Value& patch, const rapidjson::Value& importDirective,
            const AZ::IO::FixedMaxPath& importedFilePath, rapidjson::Document::AllocatorType& allocator);

        virtual JsonSerializationResult::ResultCode RestoreImport(rapidjson::Value* importPtr,
            rapidjson::Value& patch, rapidjson::Document::AllocatorType& allocator,
            const AZStd::string& importFilename);

        virtual JsonSerializationResult::ResultCode ApplyPatch(rapidjson::Value& target,
            const rapidjson::Value& patch, rapidjson::Document::AllocatorType& allocator);

        virtual JsonSerializationResult::ResultCode CreatePatch(rapidjson::Value& patch,
            const rapidjson::Value& source, const rapidjson::Value& target,
            rapidjson::Document::AllocatorType& allocator);

        void AddImportDirective(const rapidjson::Pointer& jsonPtr, AZStd::string importFile);
        const ImportDirectivesList& GetImportDirectives();

        void AddImportedFile(AZStd::string importedFile);
        const ImportedFilesList& GetImportedFiles();

        virtual ~BaseJsonImporter() = default;

    protected:

        ImportDirectivesList m_importDirectives;
        ImportedFilesList m_importedFiles;
    };

    enum class ImportTracking : AZ::u8
    {
        None = 0,
        Dependencies = (1<<0),
        Imports = (1<<1),
        All = (Dependencies | Imports)
    };
    AZ_DEFINE_ENUM_BITWISE_OPERATORS(ImportTracking);

    class JsonImportResolver final
    {
    public:

        using ImportPathStack = AZStd::vector<AZ::IO::FixedMaxPath>;

        JsonImportResolver() = delete;
        JsonImportResolver& operator=(const JsonImportResolver& rhs) = delete;
        JsonImportResolver& operator=(JsonImportResolver&& rhs) = delete;
        JsonImportResolver(const JsonImportResolver& rhs) = delete;
        JsonImportResolver(JsonImportResolver&& rhs) = delete;
        ~JsonImportResolver() = delete;

        static JsonSerializationResult::ResultCode ResolveImports(rapidjson::Value& jsonDoc,
            rapidjson::Document::AllocatorType& allocator, ImportPathStack& importPathStack,
            JsonImportSettings& settings, StackedString& element);

        static JsonSerializationResult::ResultCode RestoreImports(rapidjson::Value& jsonDoc,
            rapidjson::Document::AllocatorType& allocator, JsonImportSettings& settings);
        
    private:

        static JsonSerializationResult::ResultCode ResolveNestedImports(rapidjson::Value& jsonDoc,
            rapidjson::Document::AllocatorType& allocator, ImportPathStack& importPathStack,
            JsonImportSettings& settings, const AZ::IO::FixedMaxPath& importPath, StackedString& element);
    };


    struct JsonImportSettings final
    {
        JsonSerializationResult::JsonIssueCallback m_reporting;
        
        BaseJsonImporter* m_importer = nullptr;

        ImportTracking m_resolveFlags = ImportTracking::All;

        AZ::IO::FixedMaxPath m_loadedJsonPath;
    };
} // namespace AZ
