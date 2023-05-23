/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QHash>
#include <AzCore/base.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <utility>
#include "AzCore/EBus/EBus.h"
#include "AzCore/Interface/Interface.h"
#include "AzToolsFramework/AssetDatabase/AssetDatabaseConnection.h"
#include <utilities/PlatformConfiguration.h>
#include "AssetDatabase/AssetDatabase.h"

// This needs to be up here so it is declared before the hash, which needs to be declared before the first usage
namespace AssetProcessor
{
    struct FileUpdateTask
    {
        FileUpdateTask(AZStd::vector<AZStd::string> oldString, AZStd::vector<AZStd::string> newString, AZStd::string absPathFileToUpdate, bool isAssetIdReference, bool skipTask)
            : m_oldStrings(AZStd::move(oldString)),
            m_newStrings(AZStd::move(newString)),
            m_absPathFileToUpdate(AZStd::move(absPathFileToUpdate)),
            m_isAssetIdReference(isAssetIdReference),
            m_skipTask(skipTask)
        {
        }

        bool operator==(const FileUpdateTask& rhs) const
        {
            return m_isAssetIdReference == rhs.m_isAssetIdReference
                && m_absPathFileToUpdate == rhs.m_absPathFileToUpdate
                && m_oldStrings == rhs.m_oldStrings
                && m_newStrings == rhs.m_newStrings;
        }

        AZStd::vector<AZStd::string> m_oldStrings; // The old path or uuid strings to search for
        AZStd::vector<AZStd::string> m_newStrings; // The new path or uuid strings to replace
        AZStd::string m_absPathFileToUpdate;
        bool m_isAssetIdReference = false;
        bool m_succeeded = false;
        bool m_skipTask = false;
    };
}

namespace AZStd
{
    template<>
    struct hash<AssetProcessor::FileUpdateTask>
    {
        size_t operator()(const AssetProcessor::FileUpdateTask& obj) const
        {
            size_t h = 0;
            hash_combine(h, obj.m_isAssetIdReference);
            hash_combine(h, obj.m_absPathFileToUpdate);
            hash_range(h, obj.m_oldStrings.begin(), obj.m_oldStrings.end());
            hash_range(h, obj.m_newStrings.begin(), obj.m_newStrings.end());
            return h;
        }
    };
}

namespace AssetProcessor
{
    enum class SourceFileRelocationStatus
    {
        None,
        Failed,
        Succeeded
    };

    enum RelocationParameters
    {
        RelocationParameters_PreviewOnlyFlag = 1 << 0,
        RelocationParameters_RemoveEmptyFoldersFlag = 1 << 1,
        RelocationParameters_AllowDependencyBreakingFlag = 1 << 2,
        RelocationParameters_UpdateReferencesFlag = 1 << 3,
        RelocationParameters_ExcludeMetaDataFilesFlag = 1 << 4,
        RelocationParameters_AllowNonDatabaseFilesFlag = 1 << 5
    };

    static constexpr int SourceFileRelocationInvalidIndex = -1;

    struct SourceFileRelocationInfo
    {
        SourceFileRelocationInfo(
            AzToolsFramework::AssetDatabase::SourceDatabaseEntry sourceEntry,
            AZStd::unordered_map<int, AzToolsFramework::AssetDatabase::ProductDatabaseEntry> products,
            const AZStd::string& oldRelativePath,
            const ScanFolderInfo* scanFolder,
            bool isMetadataEnabledType)
            : m_sourceEntry(AZStd::move(sourceEntry))
            , m_products(AZStd::move(products))
            , m_oldRelativePath(oldRelativePath)
            , m_isMetadataEnabledType(isMetadataEnabledType)
        {
            AzFramework::StringFunc::Path::ConstructFull(
                scanFolder->ScanPath().toUtf8().constData(), m_oldRelativePath.c_str(), m_oldAbsolutePath, false);
            m_oldAbsolutePath = AssetUtilities::NormalizeFilePath(m_oldAbsolutePath.c_str()).toUtf8().constData();
        }

        SourceFileRelocationInfo(const AZStd::string& filePath, const ScanFolderInfo* scanFolder)
        {
            QString relFilePath;
            PlatformConfiguration::ConvertToRelativePath(filePath.c_str(), scanFolder, relFilePath);
            m_oldRelativePath = relFilePath.toUtf8().data();
            AzFramework::StringFunc::Path::ConstructFull(scanFolder->ScanPath().toUtf8().constData(), m_oldRelativePath.c_str(), m_oldAbsolutePath, false);
            m_oldAbsolutePath = AssetUtilities::NormalizeFilePath(m_oldAbsolutePath.c_str()).toUtf8().constData();
        }

        AzToolsFramework::AssetDatabase::SourceDatabaseEntry m_sourceEntry;
        AZStd::unordered_map<int, AzToolsFramework::AssetDatabase::ProductDatabaseEntry> m_products; // Key = product SubId
        AzToolsFramework::AssetDatabase::SourceFileDependencyEntryContainer m_sourceDependencyEntries;
        AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer m_productDependencyEntries;
        AZ::Uuid m_newUuid;
        AZStd::string m_oldRelativePath;
        AZStd::string m_newRelativePath;
        AZStd::string m_oldAbsolutePath;
        AZStd::string m_newAbsolutePath;
        bool m_hasPathDependencies = false;
        SourceFileRelocationStatus m_operationStatus = SourceFileRelocationStatus::None;
        // If >= 0, this is a metadata file.  This is the index into the
        // PlatformConfiguration metadata list (use with GetMetaDataFileTypeAt)
        int m_metadataIndex = SourceFileRelocationInvalidIndex;
        bool m_isMetadataEnabledType = false; // Indicates if this file uses metadata relocation
        // This is a cached index of the SourceFile in the SourceFileRelocationContainer.
        // This is only used by the metadata file to determine the destination path if needed.
        int m_sourceFileIndex = AssetProcessor::SourceFileRelocationInvalidIndex;
    };

    using SourceFileRelocationContainer = AZStd::vector<SourceFileRelocationInfo>;
    using FileUpdateTasks = AZStd::unordered_set<FileUpdateTask>;

    struct MoveFailure
    {
        MoveFailure(AZStd::string reason, bool dependencyFailure)
            : m_reason(AZStd::move(reason)),
              m_dependencyFailure(dependencyFailure)
        {
        }

        AZStd::string m_reason;
        bool m_dependencyFailure{};
    };

    struct RelocationSuccess
    {
        RelocationSuccess() = default;

        RelocationSuccess(int moveSuccessCount, int moveFailureCount, int moveTotalCount, int updateSuccessCount, int updateFailureCount, int updateTotalCount, SourceFileRelocationContainer sourceFileRelocationInfos, FileUpdateTasks fileUpdateTasks)
            : m_moveSuccessCount(moveSuccessCount),
              m_moveFailureCount(moveFailureCount),
              m_moveTotalCount(moveTotalCount),
              m_updateSuccessCount(updateSuccessCount),
              m_updateFailureCount(updateFailureCount),
              m_updateTotalCount(updateTotalCount),
              m_relocationContainer(AZStd::move(sourceFileRelocationInfos)),
              m_updateTasks(AZStd::move(fileUpdateTasks))
        {
        }

        int m_moveSuccessCount{};
        int m_moveFailureCount{};
        int m_moveTotalCount{};
        int m_updateSuccessCount{};
        int m_updateFailureCount{};
        int m_updateTotalCount{};
        SourceFileRelocationContainer m_relocationContainer;
        FileUpdateTasks m_updateTasks;
    };

    class ISourceFileRelocation
    {
    public:
        AZ_RTTI(ISourceFileRelocation, "{FEDD188E-D5FF-4852-B945-F82F7CC1CA5F}");

        ISourceFileRelocation() = default;
        virtual ~ISourceFileRelocation() = default;

        //! Moves source files or renames a file.  Source and destination can be absolute paths or scanfolder relative paths.  Wildcards are supported for source.
        //! By default no changes are made to the disk.  Set previewOnly to false to actually move files.
        //! If allowDependencyBreaking is false, the move will fail if moving any files will break existing dependencies.  Set to true to ignore and move anyway.
        virtual AZ::Outcome<RelocationSuccess, MoveFailure> Move(const AZStd::string& source, const AZStd::string& destination, int flags = RelocationParameters_PreviewOnlyFlag | RelocationParameters_RemoveEmptyFoldersFlag) = 0;

        //! Deletes source files.  Source can be an absolute path or a scanfolder relative path.  Wildcards are supported.
        //! By default no changes are made to the disk.  Set previewOnly to false to actually delete files.
        //! If allowDependencyBreaking is false, the delete will fail if deleting any file breaks existing dependencies.  Set to true to ignore and delete anyway.
        virtual AZ::Outcome<RelocationSuccess, AZStd::string> Delete(const AZStd::string& source, int flags = RelocationParameters_PreviewOnlyFlag | RelocationParameters_RemoveEmptyFoldersFlag) = 0;

        //! Takes a relocation set and builds a string report to output the result of what files will change and what dependencies will break
        virtual AZStd::string BuildReport(const SourceFileRelocationContainer& relocationEntries, const FileUpdateTasks& updateTasks, bool isMove, bool updateReference) const = 0;

         //! Takes a relocation set and builds a string report to output the result of what files will change and what dependencies will break formatted for use in a dialog box
        virtual AZStd::string BuildChangeReport(const SourceFileRelocationContainer& relocationEntries, const FileUpdateTasks& updateTasks) const = 0;
       AZ_DISABLE_COPY_MOVE(ISourceFileRelocation);
    };

    class SourceFileRelocator
        : public ISourceFileRelocation
    {
    public:
        SourceFileRelocator(AZStd::shared_ptr<AzToolsFramework::AssetDatabase::AssetDatabaseConnection> stateData, PlatformConfiguration* platformConfiguration);
        ~SourceFileRelocator();

        static void MakePathRelative(const AZStd::string& parentPath, const AZStd::string& childPath, AZStd::string& parentRelative, AZStd::string& childRelative);

        static AZ::Outcome<AZStd::string, AZStd::string> HandleWildcard(AZStd::string_view absFile, AZStd::string_view absSearch, AZStd::string destination);

        static void FixDestinationMissingFilename(AZStd::string& destination, const AZStd::string& source);

        // Takes a relocation set, scanfolder, source, and destination and calculates the new file path of every file
        AZ::Outcome<void, AZStd::string> ComputeDestination(SourceFileRelocationContainer& relocationContainer, const ScanFolderInfo* sourceScanFolder, const AZStd::string& source, AZStd::string destination, const ScanFolderInfo*& destinationScanFolderOut) const;

        // Takes a QStringList of paths and populates sources with all the corresponding source database entries
        QHash<QString, int> GetSources(
            QStringList pathMatches,
            const ScanFolderInfo* scanFolderInfo,
            SourceFileRelocationContainer& sources,
            bool allowNonDatabaseFiles = false) const;

        // Takes a QStringList of paths and populates metadata files.
        void HandleMetaDataFiles(QStringList pathMatches, QHash<QString, int>& pathIndexMap, const ScanFolderInfo* scanFolderInfo, SourceFileRelocationContainer& metadataFiles, bool excludeMetaDataFiles) const;

        // Returns a map of SubId -> ProductEntry for all the products of a source
        AZStd::unordered_map<int, AzToolsFramework::AssetDatabase::ProductDatabaseEntry> GetProductMapForSource(AZ::s64 sourceId) const;

        bool GetFilesFromSourceControl(
            SourceFileRelocationContainer& sources,
            const ScanFolderInfo* scanFolderInfo,
            QString absolutePath,
            bool excludeMetaDataFiles = false,
            bool allowNonDatabaseFiles = false) const;

        // Populates a relocation set with all direct source and product dependency database entries for every file
        void PopulateDependencies(SourceFileRelocationContainer& relocationContainer) const;

        // Gets the scanfolder and relative path given an input of an absolute or relative path (wildcard paths not supported).  Fails if the source path is not within a scanfolder or can't be made relative
        AZ::Outcome<void, AZStd::string> GetScanFolderAndRelativePath(const AZStd::string& normalizedSource, bool allowNonexistentPath, const ScanFolderInfo*& scanFolderInfo, AZStd::string& relativePath) const;

        // Given a path, populates a relocation set with all source files that match.  Will fail if a scanfolder itself is selected or the source string matches files from multiple scanfolders
        AZ::Outcome<void, AZStd::string> GetSourcesByPath(const AZStd::string& normalizedSource, SourceFileRelocationContainer& sources, const ScanFolderInfo*& scanFolderInfoOut, bool excludeMetaDataFiles = false, bool allowNonDatabaseFiles = false) const;

        int DoSourceControlMoveFiles(AZStd::string normalizedSource, AZStd::string normalizedDestination, SourceFileRelocationContainer& relocationContainer, const ScanFolderInfo* sourceScanFolderInfo, const ScanFolderInfo* destinationScanFolderInfo, bool removeEmptyFolders) const;
        int DoSourceControlDeleteFiles(AZStd::string normalizedSource, SourceFileRelocationContainer& relocationContainer, const ScanFolderInfo* sourceScanFolderInfo, bool removeEmptyFolders) const;

        static bool UpdateFileReferences(const FileUpdateTask& updateTask);
        bool ComputeProductDependencyUpdatePaths(const SourceFileRelocationInfo& relocationInfo, const AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry& productDependency, AZStd::vector<AZStd::string>& oldPaths, AZStd::vector<AZStd::string>& newPaths, AZStd::string& absPathFileToUpdate) const;
        FileUpdateTasks UpdateReferences(const SourceFileRelocationContainer& relocationContainer, bool useSourceControl) const;

        // ISourceFileRelocation implementation
        AZ::Outcome<RelocationSuccess, MoveFailure> Move(const AZStd::string& source, const AZStd::string& destination, int flags = RelocationParameters_PreviewOnlyFlag | RelocationParameters_RemoveEmptyFoldersFlag) override;
        AZ::Outcome<RelocationSuccess, AZStd::string> Delete(const AZStd::string& source, int flags = RelocationParameters_PreviewOnlyFlag | RelocationParameters_RemoveEmptyFoldersFlag) override;
        AZStd::string BuildReport(const SourceFileRelocationContainer& relocationEntries, const FileUpdateTasks& updateTasks, bool isMove, bool updateReference) const override;
        AZStd::string BuildChangeReport(const SourceFileRelocationContainer& relocationEntries, const FileUpdateTasks& updateTasks) const override;

    private:
        AZStd::shared_ptr<AzToolsFramework::AssetDatabase::AssetDatabaseConnection> m_stateData;
        PlatformConfiguration* m_platformConfig;
        AZStd::unordered_map<AZStd::string, AZStd::string> m_additionalHelpTextMap;
    };
} // namespace AssetProcessor
