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

    struct JsonImportMetadata
    {
        rapidjson::Pointer m_importDirectiveJsonPath;
        AZ::IO::Path m_importDirectiveFilePath;
        AZ::IO::Path m_resolvedFilePath;
    };

    class BaseJsonImporter
    {
    public:
        AZ_RTTI(BaseJsonImporter, "{7B225807-7B43-430F-8B11-C794DCF5ACA5}");

        using ImportDirectivesList = AZStd::vector<JsonImportMetadata>;
        using ImportedFilesList = AZStd::unordered_set<AZ::IO::Path>;

        virtual JsonSerializationResult::ResultCode ResolveImport(rapidjson::Value* importPtr,
            rapidjson::Value& patch, const rapidjson::Value& importDirective,
            const AZ::IO::FixedMaxPath& importedFilePath, rapidjson::Document::AllocatorType& allocator,
            JsonImportSettings& settings);

        //! Gathers the list of resolved $import file locates, but does not merge their
        //! contents into the importPtr
        virtual JsonSerializationResult::ResultCode StoreImport(AZ::IO::PathView importedFilePath);

        virtual JsonSerializationResult::ResultCode RestoreImport(rapidjson::Value* importPtr,
            rapidjson::Value& patch, rapidjson::Document::AllocatorType& allocator,
            AZ::IO::PathView importFilename);

        virtual JsonSerializationResult::ResultCode ApplyPatch(rapidjson::Value& target,
            const rapidjson::Value& patch, rapidjson::Document::AllocatorType& allocator);

        virtual JsonSerializationResult::ResultCode CreatePatch(rapidjson::Value& patch,
            const rapidjson::Value& source, const rapidjson::Value& target,
            rapidjson::Document::AllocatorType& allocator);

        void AddImportDirective(const rapidjson::Pointer& jsonPtr, AZ::IO::Path importFile,
            AZ::IO::Path resolvedFile);
        const ImportDirectivesList& GetImportDirectives();

        void AddImportedFile(AZ::IO::Path importedFile);
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

        //! Resolves the imports from the source JSON object in the field order.
        //! It does this by iterating over the fields of the source JSON object and copies them
        //! to the target object as it processes them.
        //! Any $import directives are merged to the target object at that point using the JSON Merge Patch
        //! algorithm. This allows fields before the $import directive to be overridden via the patching mechanism
        //! and fields that appear after the $import directive to override the imported JSON
        static JsonSerializationResult::ResultCode ResolveImportsInOrder(rapidjson::Value& target,
            rapidjson::Document::AllocatorType& allocator, const rapidjson::Value& source,
            ImportPathStack& importPathStack,
            JsonImportSettings& settings, StackedString& element);

        //! Store imports only gathers the list of resolved $import paths without merging
        //! their contents into the json document
        static JsonSerializationResult::ResultCode StoreImports(const rapidjson::Value& jsonDoc,
            ImportPathStack& importPathStack,
            JsonImportSettings& settings, StackedString& element);

        static JsonSerializationResult::ResultCode RestoreImports(rapidjson::Value& jsonDoc,
            rapidjson::Document::AllocatorType& allocator, JsonImportSettings& settings);

    private:

        static JsonSerializationResult::ResultCode ResolveNestedImports(rapidjson::Value& jsonDoc,
            rapidjson::Document::AllocatorType& allocator, ImportPathStack& importPathStack,
            JsonImportSettings& settings, const AZ::IO::FixedMaxPath& importPath, StackedString& element);
        static JsonSerializationResult::ResultCode StoreNestedImports(const rapidjson::Value& jsonDoc,
            ImportPathStack& importPathStack,
            JsonImportSettings& settings, const AZ::IO::FixedMaxPath& importPath, StackedString& element);

        static JsonSerializationResult::ResultCode LoadAndMergeImportFile(
            rapidjson::Value& target, rapidjson::Document::AllocatorType& allocator,
            const rapidjson::Value& importDirective, AZ::IO::PathView importAbsPath,
            const AZ::StackedString& element, const JsonSerializationResult::JsonIssueCallback& issueReporter);

        static JsonSerializationResult::ResultCode ResolveNestedImportsInOrder(rapidjson::Value& target,
            rapidjson::Document::AllocatorType& allocator, const rapidjson::Value& source,
            ImportPathStack& importPathStack,
            JsonImportSettings& settings, StackedString& element, AZ::IO::PathView importPath);

        static JsonSerializationResult::ResultCode PatchField(rapidjson::Value& target,
            rapidjson::Document::AllocatorType& allocator, const rapidjson::Value& source,
            rapidjson::Value::StringRefType fieldNameStringRef, const rapidjson::Value& fieldValue,
            ImportPathStack& importPathStack,
            JsonImportSettings& settings, StackedString& element);

        // Stores the absolute import path to use when merging the JSON file
        // and the import path as read from the source JSON file
        struct ImportPathsResult
        {
            AZ::IO::FixedMaxPath m_importAbsPath;
            AZ::IO::FixedMaxPath m_importRelPath;
        };
        static ImportPathsResult GetImportPaths(const rapidjson::Value& importDirective,
            const ImportPathStack&);
    };

    //! Settings used to configure how resolution of a $import directive
    //! during JSON deserialization occurs
    struct JsonImportSettings final
    {
        //! Reporting callback used to output errors around $import resolution
        JsonSerializationResult::JsonIssueCallback m_reporting;
        //! Importer object which to invoke ResolveImports/RestoreImports function
        //! to resolve the $import directive
        //! If nullptr, then the operation is a no-op
        BaseJsonImporter* m_importer = nullptr;
        //! Enum options that control how the BaseJsonImporter stores
        //! the results of the import operation
        //! ImportTracking::Imports stores the value of `$import` field as found in the JSON file
        //! as well as the resolved absolute path of the file on disk
        //! Any Filesystem @<alias>@ aliases would be resolved
        //! ImportTracking::Dependences stores only the absolute path of the file to import
        //! that should reside on the file system
        ImportTracking m_resolveFlags = ImportTracking::All;
        //! Boolean to control whether imports inside of the imported files are resolved
        //! as well.
        //! If false, then only the imports in the supplied rapidjson::Value is resolved
        bool m_resolveNestedImports = true;
        //! Path of the file which corresponds to the Json Document that is being checked for imports
        AZ::IO::FixedMaxPath m_loadedJsonPath;
    };
} // namespace AZ
