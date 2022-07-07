/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/string/string.h>
#include <AzCore/IO/Path/Path_fwd.h>
#include <AzFramework/FileTag/FileTagBus.h>

namespace AzFramework
{
    namespace FileTag
    {
        //! These are list of some common file tags 
        extern const char* FileTags[];

        extern const char* ExcludeFileName;
        extern const char* IncludeFileName;

        //! helper enum for retrieving indexes in FileTags array 
        enum class FileTagsIndex : unsigned int
        {
            Ignore = 0,
            Error,
            ProductDependency,
            EditorOnly,
            Shader
        };
        using FileTagAssetsMap = AZStd::map<FileTagType, AZStd::unique_ptr<AzFramework::FileTag::FileTagAsset>>;

        //! File Tag Manager class can be used to add/remove tags based on either filepaths or file patterns.
        class FileTagManager
            : public FileTagsEventBus::Handler
        {
        public:
            AZ_TYPE_INFO(FileTagManager, "{049234E0-9EC8-4527-AEAF-BF6D4833BF07}");
            AZ_CLASS_ALLOCATOR(FileTagManager, AZ::SystemAllocator, 0);

            FileTagManager();
            ~FileTagManager();

            //////////////////////////////////////////////////////////////////////////
            // FileTagsEventBus Interface

            bool Save(FileTagType FileTagType, const AZStd::string& destinationFilePath) override;
            AZ::Outcome<AZStd::string, AZStd::string> AddFileTags(const AZStd::string& filePath, FileTagType fileTagType, const AZStd::vector<AZStd::string>& fileTags) override;
            AZ::Outcome<AZStd::string, AZStd::string> RemoveFileTags(const AZStd::string& filePath, FileTagType fileTagType, const AZStd::vector<AZStd::string>& fileTags) override;
            AZ::Outcome<AZStd::string, AZStd::string> AddFilePatternTags(const AZStd::string& pattern, AzFramework::FileTag::FilePatternType filePatternType, FileTagType fileTagType, const AZStd::vector<AZStd::string>& fileTags) override;
            AZ::Outcome<AZStd::string, AZStd::string> RemoveFilePatternTags(const AZStd::string& pattern, AzFramework::FileTag::FilePatternType filePatternType, FileTagType fileTagType, const AZStd::vector<AZStd::string>& fileTags) override;

            /////////////////////////////////////////////////////////////////////////

        protected:
            AZ::Outcome<AZStd::string, AZStd::string> AddTagsInternal(AZStd::string filePath, FileTagType fileTagType, AZStd::vector<AZStd::string> fileTags, AzFramework::FileTag::FilePatternType filePatternType = AzFramework::FileTag::FilePatternType::Exact);
            AZ::Outcome<AZStd::string, AZStd::string> RemoveTagsInternal(AZStd::string filePath, FileTagType fileTagType, AZStd::vector<AZStd::string> fileTags, AzFramework::FileTag::FilePatternType filePatternType = AzFramework::FileTag::FilePatternType::Exact);
        
        private:

            AzFramework::FileTag::FileTagAsset* GetFileTagAsset(FileTagType fileTagType);

            FileTagAssetsMap m_fileTagAssetsMap;
        };

        //! File Tag Query Manager class can be used to retreive tags based on either filepaths or patterns.
        class FileTagQueryManager
            : public QueryFileTagsEventBus::Handler
        {
        public:
            AZ_TYPE_INFO(FileTagQueryManager, "{082E821D-D207-4974-9322-76DE01A5704F}");
            AZ_CLASS_ALLOCATOR(FileTagQueryManager, AZ::SystemAllocator, 0);

            FileTagQueryManager(FileTagType fileTagType);
            ~FileTagQueryManager();

            //////////////////////////////////////////////////////////////////////////
            // QueryFileTagsEventBus Interface

            bool Load(const AZStd::string& filePath = AZStd::string()) override;
            bool LoadEngineDependencies(const AZStd::string& filePath) override;
            bool Match(const AZStd::string& filePath, AZStd::vector<AZStd::string> fileTags) override;
            AZStd::set<AZStd::string> GetTags(const AZStd::string& filePath) override;

            /////////////////////////////////////////////////////////////////////////

            static AZ::IO::Path GetDefaultFileTagFilePath(FileTagType fileTagType);

        protected:

            AzFramework::FileTag::FileTagMap m_fileTagsMap;
            AzFramework::FileTag::FileTagMap m_patternTagsMap;
            FileTagType m_fileTagType;
        };
    }
}
