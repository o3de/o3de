/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZTOOLSFRAMEWORK_Connection_H
#define AZTOOLSFRAMEWORK_Connection_H
#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/bitset.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/SQLite/SQLiteQueryLogBus.h>
#include <AzToolsFramework/AssetDatabase/PathOrUuid.h>

// At the time of writing, AZStd::function does not support RVALUE-Refs.  We use std::function instead.
#include <functional>

namespace AzToolsFramework
{
    namespace SQLite
    {
        class Connection;
        class Statement;
    } // namespace SQLite

    namespace AssetDatabase
    {
        constexpr AZ::s64 InvalidEntryId = -1;

        //! List all database version changes here with descriptive naming to explain what was changed in
        //! the database
        enum class DatabaseVersion : int
        {
            DatabaseDoesNotExist = -1,
            StartingVersion = 2,
            AddedIndices = 3, // the version where we added the indexes
            AddedJobLogTable = 4, // added job log table to the database
            // skipping up to 7 internally so that old tables are cleared.
            NewTables = 7, // the version where we tables refactored to scanfolder/source/job/product
            AddedOutputPrefixToScanFolders = 8,
            AddedJobKeyIndex = 9, // the version where we add jobKey index on job table
            AddedSourceGuidIndex = 10,
            AddedSourceDependencyTable = 11,
            AddedLegacySubIDsTable = 12, // the version where we added the table that contains a map of product -> previously known aliases
            AddedProductDependencyTable = 13,
            ClearAutoSucceedJobs = 14, // version bump to clear out AutoSucceed jobs from the database (they were never intended to be there in the first place)
            AddedFilesTable = 15,
            AddedAnalysisFingerprint = 16,
            AddedSourceDependencyType = 17,
            AddedFileModTimes = 18,
            AddedUnresolvedDependencyField = 19, // Add unresolved path field to dependency table
            AddedUnresolvedDependencyTypeField = 20,
            AddedTypeOfDependencyIndex = 21,
            AddedProductDependencyPlatform = 22,
            AddedMissingProductDependencyTable = 23,
            AddedWarningAndErrorCountToJobs = 24,
            AddedFromAssetIdField = 25,
            AddedProductDependencyIndexes = 26,
            AddedFileHashField = 27,
            AddedLastScanTimeField = 28,
            AddedScanTimeSecondsSinceEpochField = 29,
            ChangedSortFunctionFromQSortToStdStableSort = 30,
            RemoveOutputPrefixFromScanFolders,
            AddedSourceIndexForSourceDependencyTable,
            AddedSourceDependencySubIdsAndProductHashes,
            AddedFlagsColumnToProductTable,
            AddedStatsTable,
            ChangedSourceDependencySourceColumn,
            SplitMaterialBuilderAndMaterialAssetBuilder,
            NewMaterialTypeBuildPipeline,
            AddedJobFailureSourceColumn,
            AddedMissingDependenciesIndex,
            //Add all new versions before this
            DatabaseVersionCount,
            LatestVersion = DatabaseVersionCount - 1
        };

        //////////////////////////////////////////////////////////////////////////
        //DatabaseInfoEntry
        class DatabaseInfoEntry
        {
        public:
            DatabaseInfoEntry() = default;
            DatabaseInfoEntry(AZ::s64 rowID, DatabaseVersion version);

            auto GetColumns();

            AZ::s64 m_rowID = -1;
            DatabaseVersion m_version = DatabaseVersion::DatabaseDoesNotExist;
        };

        typedef AZStd::vector<DatabaseInfoEntry> DatabaseInfoEntryContainer;

        //////////////////////////////////////////////////////////////////////////
        //ScanFolderDatabaseEntry
        class ScanFolderDatabaseEntry
        {
        public:
            ScanFolderDatabaseEntry() = default;
            ScanFolderDatabaseEntry(AZ::s64 scanFolderID,
                const char* scanFolder,
                const char* displayName,
                const char* portableKey,
                int isRoot = 0);
            ScanFolderDatabaseEntry(const char* scanFolder,
                const char* displayName,
                const char* portableKey,
                int isRoot = 0);
            ScanFolderDatabaseEntry(const ScanFolderDatabaseEntry& other);
            ScanFolderDatabaseEntry(ScanFolderDatabaseEntry&& other);

            ScanFolderDatabaseEntry& operator=(ScanFolderDatabaseEntry&& other);
            ScanFolderDatabaseEntry& operator=(const ScanFolderDatabaseEntry& other);
            bool operator==(const ScanFolderDatabaseEntry& other) const;

            AZStd::string ToString() const;
            auto GetColumns();

            AZ::s64 m_scanFolderID = InvalidEntryId; // the database Primary Key row index, not to be used for any logic purpose
            AZStd::string m_scanFolder; // the actual local computer path to that scan folder.
            AZStd::string m_displayName; // a display name, blank means it should not show up in UIs
            AZStd::string m_portableKey; // a key that uniquely identifies a scan folder so that we can recognize the same one in other databases/computer
            int m_isRoot = 0;
        };

        typedef AZStd::vector<ScanFolderDatabaseEntry> ScanFolderDatabaseEntryContainer;

        //////////////////////////////////////////////////////////////////////////
        //SourceDatabaseEntry
        class SourceDatabaseEntry
        {
        public:
            SourceDatabaseEntry() = default;
            SourceDatabaseEntry(AZ::s64 sourceID, AZ::s64 scanFolderPK, const char* sourceName, AZ::Uuid sourceGuid, const char* analysisFingerprint);
            SourceDatabaseEntry(AZ::s64 scanFolderPK, const char* sourceName, AZ::Uuid sourceGuid, const char* analysisFingerprint);

            AZStd::string ToString() const;
            auto GetColumns();

            AZ::s64 m_sourceID = InvalidEntryId;
            AZ::s64 m_scanFolderPK = InvalidEntryId;
            AZStd::string m_sourceName;
            AZ::Uuid m_sourceGuid = AZ::Uuid::CreateNull();
            AZStd::string m_analysisFingerprint;
        };

        typedef AZStd::vector<SourceDatabaseEntry> SourceDatabaseEntryContainer;

        //////////////////////////////////////////////////////////////////////////
        class BuilderInfoEntry
        {
        public:
            BuilderInfoEntry() = default;
            BuilderInfoEntry(AZ::s64 builderInfoID, const AZ::Uuid& builderUuid, const char* analysisFingerprint);
            AZ::s64 m_builderInfoID = InvalidEntryId;
            AZ::Uuid m_builderUuid = AZ::Uuid::CreateNull();
            AZStd::string m_analysisFingerprint;

            AZStd::string ToString() const;
            auto GetColumns();
        };

        using BuilderInfoEntryContainer = AZStd::vector<BuilderInfoEntry>;

        //////////////////////////////////////////////////////////////////////////
        //JobDatabaseEntry
        class JobDatabaseEntry
        {
        public:
            JobDatabaseEntry() = default;
            JobDatabaseEntry(AZ::s64 jobID, AZ::s64 sourcePK, const char* jobKey, AZ::u32 fingerprint, const char* platform, AZ::Uuid builderGuid, AssetSystem::JobStatus status, AZ::u64 jobRunKey, AZ::s64 firstFailLogTime = 0, const char* firstFailLogFile = nullptr, AZ::s64 lastFailLogTime = 0, const char* lastFailLogFile = nullptr, AZ::s64 lastLogTime = 0, const char* lastLogFile = nullptr, AZ::u32 warningCount = 0, AZ::u32 errorCount = 0);
            JobDatabaseEntry(AZ::s64 sourcePK, const char* jobKey, AZ::u32 fingerprint, const char* platform, AZ::Uuid builderGuid, AssetSystem::JobStatus status, AZ::u64 jobRunKey, AZ::s64 firstFailLogTime = 0, const char* firstFailLogFile = nullptr, AZ::s64 lastFailLogTime = 0, const char* lastFailLogFile = nullptr, AZ::s64 lastLogTime = 0, const char* lastLogFile = nullptr, AZ::u32 warningCount = 0, AZ::u32 errorCount = 0);

            bool operator==(const JobDatabaseEntry& other) const;

            AZStd::string ToString() const;
            auto GetColumns();

            AZ::s64 m_jobID = InvalidEntryId;
            AZ::s64 m_sourcePK = InvalidEntryId;
            AZStd::string m_jobKey;
            AZ::u32 m_fingerprint = 0;
            AZStd::string m_platform;
            AZ::Uuid m_builderGuid;
            AssetSystem::JobStatus m_status = AssetSystem::JobStatus::Queued;
            AZ::u64 m_jobRunKey = 0;
            AZ::s64 m_failureCauseSourcePK = InvalidEntryId;
            AZ::u32 m_failureCauseFingerprint = 0;
            AZ::s64 m_firstFailLogTime = 0;
            AZStd::string m_firstFailLogFile;
            AZ::s64 m_lastFailLogTime = 0;
            AZStd::string m_lastFailLogFile;
            AZ::s64 m_lastLogTime = 0;
            AZStd::string m_lastLogFile;
            AZ::u32 m_errorCount = 0;
            AZ::u32 m_warningCount = 0;
        };

        typedef AZStd::vector<JobDatabaseEntry> JobDatabaseEntryContainer;

        //////////////////////////////////////////////////////////////////////////
        //SourceFileDependencyEntry
        class SourceFileDependencyEntry
        {
        public:
            /// This is also used as a bitset when making queries.
            enum TypeOfDependency : AZ::u32
            {
                DEP_SourceToSource  = 1 << 0, ///< source file depends on other source file
                DEP_JobToJob        = 1 << 1, ///< job depends on other job
                DEP_SourceOrJob     = (DEP_SourceToSource | DEP_JobToJob),
                DEP_SourceLikeMatch = 1 << 2, ///< Allow wildcard matches using LIKE
                DEP_Any             = 0xFFFFFFFF ///< convenience value - not allowed to write this to the actual DB.
            };

            SourceFileDependencyEntry() = default;
            SourceFileDependencyEntry(AZ::Uuid builderGuid, AZ::Uuid sourceGuid, PathOrUuid dependsOnSource, TypeOfDependency dependencyType, bool fromAssetId, const char* subIds);

            AZStd::string ToString() const;
            auto GetColumns();

            AZ::s64 m_sourceDependencyID = InvalidEntryId;
            AZ::Uuid m_builderGuid = AZ::Uuid::CreateNull();
            TypeOfDependency m_typeOfDependency = DEP_SourceToSource;
            AZ::Uuid m_sourceGuid = AZ::Uuid::CreateNull();
            PathOrUuid m_dependsOnSource;
            AZ::u32 m_fromAssetId = false; // Indicates if the dependency was converted from an AssetId into a path before being stored in the DB
            AZStd::string m_subIds;
        };

        typedef AZStd::vector<SourceFileDependencyEntry> SourceFileDependencyEntryContainer;

        //////////////////////////////////////////////////////////////////////////
        //ProductDatabaseEntry
        class ProductDatabaseEntry
        {
        public:
            ProductDatabaseEntry() = default;
            ProductDatabaseEntry(AZ::s64 productID, AZ::s64 jobPK,  AZ::u32 subID, const char* productName,
                AZ::Data::AssetType assetType, AZ::Uuid legacyGuid = AZ::Uuid::CreateNull(), AZ::u64 hash = 0, AZStd::bitset<64> flags = 0);
            ProductDatabaseEntry(AZ::s64 jobPK, AZ::u32 subID, const char* productName,
                AZ::Data::AssetType assetType, AZ::Uuid legacyGuid = AZ::Uuid::CreateNull(), AZ::u64 hash = 0, AZStd::bitset<64> flags = 0);
            AZ_DEFAULT_COPY_MOVE(ProductDatabaseEntry);

            // Literal, all-fields equality operator.
            // This includes the hash of the file and its flags.  Use IsSameLogicalProductAs instead, if you need to
            //  compare whether its represents the same product as the other rather than identical in every way (including file data).
            bool operator==(const ProductDatabaseEntry& other) const;

            //! Logical equality compare.
            //! It will return true if the fields that establish the identify of a product are identical, regardless
            //! of the equality of things like its flags and hash.
            bool IsSameLogicalProductAs(const ProductDatabaseEntry& other) const;

            AZStd::string ToString() const;
            auto GetColumns();

            AZ::s64 m_productID = InvalidEntryId;
            AZ::s64 m_jobPK = InvalidEntryId;
            AZ::u32 m_subID = 0;
            AZStd::string m_productName;
            AZ::Data::AssetType m_assetType = AZ::Data::AssetType::CreateNull();
            AZ::Uuid m_legacyGuid = AZ::Uuid::CreateNull();//used only for backward compatibility with old product guid, is generated based on product name
            AZ::u64 m_hash = 0;
            AZStd::bitset<64> m_flags = 0;
        };
        typedef AZStd::vector<ProductDatabaseEntry> ProductDatabaseEntryContainer;

        class LegacySubIDsEntry
        {
        public:
            LegacySubIDsEntry() = default;
            LegacySubIDsEntry(AZ::s64 subIDsEntryID, AZ::s64 productPK, AZ::u32 subId); // loaded from db, and thus includes the PK
            LegacySubIDsEntry(AZ::s64 productPK, AZ::u32 subId);  // being synthesized does not include the PK.

            bool operator==(const LegacySubIDsEntry& other) const;

            auto GetColumns();

            AZ::s64 m_subIDsEntryID = InvalidEntryId; // the main ID of this table.
            AZ::s64 m_productPK = InvalidEntryId; // the foreign key to the Products table.
            AZ::u32 m_subID = 0; // the legacy subID.
        };

        typedef AZStd::vector<LegacySubIDsEntry> LegacySubIDsEntryContainer;

        //////////////////////////////////////////////////////////////////////////
        //ProductDependencyDatabaseEntry
        class ProductDependencyDatabaseEntry
        {
        public:
            enum DependencyType : AZ::u32
            {
                ProductDep_ProductFile = 0,
                ProductDep_SourceFile
            };

            ProductDependencyDatabaseEntry() = default;
            ProductDependencyDatabaseEntry(AZ::s64 productDependencyID, AZ::s64 productPK, AZ::Uuid dependencySourceGuid, AZ::u32 dependencySubID, AZStd::bitset<64> dependencyFlags, const AZStd::string& platform, const AZStd::string& unresolvedPath, DependencyType dependencyType, AZ::u32 fromAssetId);
            ProductDependencyDatabaseEntry(AZ::s64 productPK, AZ::Uuid dependencySourceGuid, AZ::u32 dependencySubID, AZStd::bitset<64> dependencyFlags, const AZStd::string& platform, AZ::u32 fromAssetId, const AZStd::string& unresolvedPath="", DependencyType dependencyType = DependencyType::ProductDep_ProductFile);

            bool operator == (const ProductDependencyDatabaseEntry& other) const;

            AZStd::string ToString() const;
            auto GetColumns();

            AZ::s64 m_productDependencyID = InvalidEntryId;
            AZ::s64 m_productPK = InvalidEntryId;
            AZ::Uuid m_dependencySourceGuid = AZ::Uuid::CreateNull();
            AZ::u32 m_dependencySubID = 0;
            AZStd::bitset<64> m_dependencyFlags = 0;
            AZStd::string m_unresolvedPath;
            AZStd::string m_platform;
            DependencyType m_dependencyType = DependencyType::ProductDep_ProductFile;
            AZ::u32 m_fromAssetId = false; // Indicates if the dependency was originally from an AssetId (true) or if it was originally a path dependency (false)
        };

        typedef AZStd::vector<ProductDependencyDatabaseEntry> ProductDependencyDatabaseEntryContainer;

        //////////////////////////////////////////////////////////////////////////
        //MissingProductDependencyDatabaseEntry
        class MissingProductDependencyDatabaseEntry
        {
        public:
            MissingProductDependencyDatabaseEntry() = default;
            MissingProductDependencyDatabaseEntry(
                AZ::s64 missingProductDependencyId,
                AZ::s64 productPK,
                const AZStd::string& scannerId,
                const AZStd::string& scannerVersion,
                const AZStd::string& sourceFileFingerprint,
                AZ::Uuid dependencySourceGuid,
                AZ::u32 dependencySubId,
                const AZStd::string& missingDependencyString,
                const AZStd::string& lastScanTime,
                AZ::u64 scanTimeSecondsSinceEpoch);
            MissingProductDependencyDatabaseEntry(
                AZ::s64 productPK,
                const AZStd::string& scannerId,
                const AZStd::string& scannerVersion,
                const AZStd::string&  sourceFileFingerprint,
                AZ::Uuid dependencySourceGuid,
                AZ::u32 dependencySubId,
                const AZStd::string& missingDependencyString,
                const AZStd::string& lastScanTime,
                AZ::u64 scanTimeSecondsSinceEpoch);

            bool operator == (const MissingProductDependencyDatabaseEntry& other) const;

            AZStd::string ToString() const;
            auto GetColumns();

            AZ::s64 m_missingProductDependencyId = InvalidEntryId;
            AZ::s64 m_productPK = InvalidEntryId;
            AZStd::string m_scannerId;
            AZStd::string m_scannerVersion;
            AZStd::string m_sourceFileFingerprint;
            AZ::Uuid m_dependencySourceGuid = AZ::Uuid::CreateNull();
            AZ::u32 m_dependencySubId = 0;
            AZStd::string m_missingDependencyString;
            AZStd::string m_lastScanTime; // It's useful to see in the UI if / when a scan was last performed.
            AZ::u64 m_scanTimeSecondsSinceEpoch = 0; // Makes it easy to sort by scan time.
        };

        typedef AZStd::vector<MissingProductDependencyDatabaseEntry> MissingProductDependencyDatabaseEntryContainer;

        //////////////////////////////////////////////////////////////////////////
        //CombinedDatabaseEntry
        class CombinedDatabaseEntry
            : public ScanFolderDatabaseEntry
            , public SourceDatabaseEntry
            , public JobDatabaseEntry
            , public ProductDatabaseEntry
        {
        public:
            // allow it to default move operators and etc.
            bool operator==(const CombinedDatabaseEntry& other) = delete; // its meaningless to compare these
            LegacySubIDsEntryContainer m_legacySubIDs;

            auto GetColumns();
        };

        typedef AZStd::vector<CombinedDatabaseEntry> CombinedDatabaseEntryContainer;

        //////////////////////////////////////////////////////////////////////////
        //FileDatabaseEntry
        class FileDatabaseEntry
        {
        public:
            FileDatabaseEntry() = default;

            FileDatabaseEntry(const FileDatabaseEntry& other) = default;
            FileDatabaseEntry(FileDatabaseEntry&& other) = default;

            FileDatabaseEntry& operator=(FileDatabaseEntry&& other) = default;
            FileDatabaseEntry& operator=(const FileDatabaseEntry& other) = default;
            bool operator==(const FileDatabaseEntry& other) const;

            AZStd::string ToString() const;
            auto GetColumns();

            AZ::s64 m_fileID = InvalidEntryId;
            AZ::s64 m_scanFolderPK = InvalidEntryId;
            AZStd::string m_fileName;
            int m_isFolder = 0;
            AZ::u64 m_modTime{};
            AZ::u64 m_hash{};
        };

        typedef AZStd::vector<FileDatabaseEntry> FileDatabaseEntryContainer;

        //////////////////////////////////////////////////////////////////////////
        //SourceAndScanFolderDatabaseEntry
        class SourceAndScanFolderDatabaseEntry
            : public ScanFolderDatabaseEntry
            , public SourceDatabaseEntry
        {
        public:
            auto GetColumns();
        };

        typedef AZStd::vector<SourceAndScanFolderDatabaseEntry> SourceAndScanFolderDatabaseEntryContainer;

        //////////////////////////////////////////////////////////////////////////
        // StatDatabaseEntry
        class StatDatabaseEntry
        {
        public:
            StatDatabaseEntry() = default;

            StatDatabaseEntry(const StatDatabaseEntry& other) = default;
            StatDatabaseEntry(StatDatabaseEntry&& other) = default;

            StatDatabaseEntry& operator=(StatDatabaseEntry&& other) = default;
            StatDatabaseEntry& operator=(const StatDatabaseEntry& other) = default;
            bool operator==(const StatDatabaseEntry& other) const;
            bool operator!=(const StatDatabaseEntry& other) const;

            AZStd::string ToString() const;
            auto GetColumns();

            AZStd::string m_statName;
            AZ::s64 m_statValue = 0;
            AZ::s64 m_lastLogTime = 0;
        };

        typedef AZStd::vector<StatDatabaseEntry> StatDatabaseEntryContainer;

        //////////////////////////////////////////////////////////////////////////
        //AssetDatabaseConnection
        //! The Connection class represents a read-only connection to the asset database specifically
        //! (as opposed to a sql connection). Things like the Asset Processor derive from this in order
        //! to provide read-write access and curate the database. You may have as many of these
        //! connections as you wish, each represents a single connection to the asset database for use by
        //! a specific thread.
        //! All general read only queries are added here. Since this is meant for tools, we only care
        //! about queries which return data here, no modifications allowed here. Currently the Asset
        //! Processor is the only program allowed to modify the database. As such it derives from this
        //! class and overrides ReadOnly to return false which allows it to write / curate the database.
        //! However most systems only need to read and be notified of changes, so when they derive
        //! they don't override anything but CreateStatements to add specific queries that only make
        //! sense for their specific purpose, otherwise that statement should be added to this class so
        //! it can be reused by any system that needs it. Note that if a system needs read only access
        //! and has no special query needs, then this class can be used directly.
        class AssetDatabaseConnection : public SQLite::SQLiteQueryLogBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(AssetDatabaseConnection, AZ::SystemAllocator);

            AssetDatabaseConnection();
            virtual ~AssetDatabaseConnection();

            //Open / Close the database
            bool OpenDatabase(bool ignoreFutureAssetDBVersionError = false);
            void CloseDatabase();

            //These only need to be overridden by derived class if write access is needed, such as by the
            //asset processor.
            virtual bool IsReadOnly() const
            {
                return true;
            }
            virtual bool PostOpenDatabase(bool ignoreFutureAssetDBVersionError); // used by Asset Processor to upgrade the database

            //returns the current version of the code used for reading the database
            //this is how we know we have to upgrade old databases to the new ones
            virtual DatabaseVersion CurrentDatabaseVersion() const
            {
                return DatabaseVersion::LatestVersion; // DO NOT alter this value, add to the enumeration instead
            }

            //////////////////////////////////////////////////////////////////////////
            //handlers
            using databaseInfoHandler = AZStd::function<bool(DatabaseInfoEntry& entry)>;
            using scanFolderHandler = AZStd::function<bool(ScanFolderDatabaseEntry& entry)>;
            using sourceHandler = AZStd::function<bool(SourceDatabaseEntry& entry)>;
            using combinedSourceScanFolderHandler = AZStd::function<bool(SourceAndScanFolderDatabaseEntry& entry)>;
            using jobHandler = AZStd::function<bool(JobDatabaseEntry& entry)>;
            using productHandler = AZStd::function<bool(ProductDatabaseEntry& entry)>;
            using combinedHandler = AZStd::function<bool(CombinedDatabaseEntry& entry)>;
            using jobInfoHandler = AZStd::function<bool(AzToolsFramework::AssetSystem::JobInfo& job)>;
            using sourceFileDependencyHandler = AZStd::function<bool(SourceFileDependencyEntry& entry)>;
            using legacySubIDsHandler = AZStd::function<bool(LegacySubIDsEntry& entry)>;
            using productDependencyHandler = AZStd::function<bool(ProductDependencyDatabaseEntry& entry)>;
            using productDependencyAndPathHandler = AZStd::function<bool(ProductDependencyDatabaseEntry& entry, const AZStd::string& path)>;
            using missingProductDependencyHandler = AZStd::function<bool(MissingProductDependencyDatabaseEntry& entry)>;
            using combinedProductDependencyHandler = AZStd::function<bool(AZ::Data::AssetId& asset, ProductDependencyDatabaseEntry& entry)>;
            // note that AZStd::function cannot handle rvalue-refs at the time of writing this.
            using BuilderInfoHandler = std::function<bool(BuilderInfoEntry&&)>;
            using fileHandler = AZStd::function<bool(FileDatabaseEntry& entry)>;
            using statHandler = AZStd::function<bool(StatDatabaseEntry& entry)>;

            //////////////////////////////////////////////////////////////////
            //Query entire table
            bool QueryDatabaseInfoTable(databaseInfoHandler handler);
            DatabaseVersion QueryDatabaseVersion();//convenience function to return the version field in the database
            bool QueryScanFoldersTable(scanFolderHandler handler);
            bool QuerySourcesTable(sourceHandler handler);
            bool QueryJobsTable(jobHandler handler, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), const char* jobKey = nullptr, const char* platform = nullptr, AssetSystem::JobStatus status = AssetSystem::JobStatus::Any);
            bool QueryProductsTable(productHandler handler, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), const char* jobKey = nullptr, const char* platform = nullptr, AssetSystem::JobStatus status = AssetSystem::JobStatus::Any);
            bool QueryProductDependenciesTable(combinedProductDependencyHandler handler);
            bool QueryBuilderInfoTable(const BuilderInfoHandler& handler);
            bool QueryFilesTable(fileHandler handler);
            bool QueryStatsTable(statHandler handler);

            //////////////////////////////////////////////////////////////////////////
            //Queries
            //NOTE: There are 2 variations of string equivalence in the database:
            //"By": Means the strings are exactly the same.
            //"Like": Raw LIKE sql query that takes SQL % wild card and _ wild space.
            enum LikeType
            {
                Raw,
                StartsWith,
                EndsWith,
                Matches,
            };
            AZStd::string GetLikeActualSearchTerm(const char* likeString, LikeType likeType);
            //NOTE: The return bool is always whether the database query succeeded. If you
            //succeeded in querying the database but it does not exist in the database it will return true.
            //NOTE: If a query returns true you are guaranteed to have at least one output. However, if it fails
            //you MAY still have some output as it could be the result of multiple internal queries.
            //NOTE: Also these functions are all ADDITIVE, meaning if you pass a container in
            //and the query results in outputs, they are added to whats already there, they never delete
            //before adding outputs.
            //NOTE: A connection is NON MUTATING. This class CAN NOT modify the database in any way whatsoever.
            //This class is to provide a flexible read only interface for people to use. If you need to
            //Write then you should derive from this class and and write your writing code there. The
            //derived class may wrap or even hide this classes interface, but should follow the same rules.
            //NOTE: Any query to do with a non specific job or product has a optional platform filter.
            //NOTE: All combined query also has an optional jobKey filter, except the specific product queries
            //NOTE: Any additional filtering is probably best done in code as permutations of the SQL will
            //most likely not result in any significant speed boost and could easily become unmanageable.

            //scan folder
            bool QueryScanFolderByScanFolderID(AZ::s64 scanfolderID, scanFolderHandler handler);
            bool QueryScanFolderBySourceID(AZ::s64 sourceID, scanFolderHandler handler);
            bool QueryScanFolderByJobID(AZ::s64 jobID, scanFolderHandler handler);
            bool QueryScanFolderByProductID(AZ::s64 productID, scanFolderHandler handler);

            bool QueryScanFolderByDisplayName(const char* displayname, scanFolderHandler handler);
            bool QueryScanFolderByPortableKey(const char* portableKey, scanFolderHandler handler);

            //source
            bool QuerySourceBySourceID(AZ::s64 sourceID, sourceHandler handler);
            bool QuerySourceByScanFolderID(AZ::s64 scanFolderID, sourceHandler handler);
            bool QuerySourceByJobID(AZ::s64 jobID, sourceHandler handler);
            bool QuerySourceByProductID(AZ::s64 productID, sourceHandler handler);
            bool QuerySourceBySourceGuid(AZ::Uuid sourceGuid, sourceHandler handler);

            bool QuerySourceBySourceName(const char* exactSourceName, sourceHandler handler);
            bool QuerySourceBySourceNameScanFolderID(const char* exactSourceName, AZ::s64 scanFolderID, sourceHandler handler);
            bool QuerySourceLikeSourceName(const char* likeSourceName, LikeType likeType, sourceHandler handler);
            bool QuerySourceLikeSourceNameScanFolderID(const char* likeSourceName, AZ::s64 scanFolderID, LikeType likeType, sourceHandler handler);
            bool QuerySourceAnalysisFingerprint(const char* exactSourceName, AZ::s64 scanFolderID, AZStd::string& result);
            bool QuerySourceAndScanfolder(combinedSourceScanFolderHandler handler);

            //job
            bool QueryJobByJobID(AZ::s64 jobID, jobHandler handler);
            bool QueryJobByJobKey(AZStd::string jobKey, jobHandler handler);
            bool QueryJobByJobRunKey(AZ::u64 jobRunKey, jobHandler handler);
            bool QueryJobByProductID(AZ::s64 productID, jobHandler handler);
            bool QueryJobBySourceID(AZ::s64 sourceID, jobHandler handler, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), const char* jobKey = nullptr, const char* platform = nullptr, AssetSystem::JobStatus status = AssetSystem::JobStatus::Any);
            bool QueryJobsByFailureCauseSourceID(AZ::s64 sourceID, jobHandler handler);

            //product
            bool QueryProductByProductID(AZ::s64 productID, productHandler handler);
            bool QueryProductByJobID(AZ::s64 jobID, productHandler handler, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), const char* jobKey = nullptr, const char* platform = nullptr, AssetSystem::JobStatus status = AssetSystem::JobStatus::Any);
            bool QueryProductBySourceID(AZ::s64 sourceID, productHandler handler, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), const char* jobKey = nullptr, const char* platform = nullptr, AssetSystem::JobStatus status = AssetSystem::JobStatus::Any);
            bool QueryProductBySourceGuidSubID(AZ::Uuid sourceGuid, AZ::u32 productSubID, productHandler handler);

            bool QueryProductByProductName(const char* exactProductName, productHandler handler, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), const char* jobKey = nullptr, const char* platform = nullptr, AssetSystem::JobStatus status = AssetSystem::JobStatus::Any);
            bool QueryProductLikeProductName(const char* likeProductName, LikeType likeType, productHandler handler, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), const char* jobKey = nullptr, const char* platform = nullptr, AssetSystem::JobStatus status = AssetSystem::JobStatus::Any);

            bool QueryProductBySourceName(const char* exactSourceName, productHandler handler, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), const char* jobKey = nullptr, const char* platform = nullptr, AssetSystem::JobStatus status = AssetSystem::JobStatus::Any);
            bool QueryProductBySourceNameScanFolderID(const char* exactSourceName, AZ::s64 scanFolderID, productHandler handler, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), const char* jobKey = nullptr, const char* platform = nullptr, AssetSystem::JobStatus status = AssetSystem::JobStatus::Any);
            bool QueryProductLikeSourceName(const char* likeSourceName, LikeType likeType, productHandler handler, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), const char* jobKey = nullptr, const char* platform = nullptr, AssetSystem::JobStatus status = AssetSystem::JobStatus::Any);
            bool QueryProductByJobIDSubID(AZ::s64 jobID, AZ::u32 subId, productHandler handler);

            // legacy SubIDs
            bool QueryLegacySubIdsByProductID(AZ::s64 productId, legacySubIDsHandler handler);

            //combined scan folder/source/job/product
            bool QueryCombined(combinedHandler handler, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), const char* jobKey = nullptr, const char* platform = nullptr, AssetSystem::JobStatus status = AssetSystem::JobStatus::Any, bool includeLegacySubIDs = false);
            bool QueryCombinedBySourceID(AZ::s64 sourceID, combinedHandler handler, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), const char* jobKey = nullptr, const char* platform = nullptr, AssetSystem::JobStatus status = AssetSystem::JobStatus::Any);
            bool QueryCombinedByJobID(AZ::s64 jobID, combinedHandler handler, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), const char* jobKey = nullptr, const char* platform = nullptr, AssetSystem::JobStatus status = AssetSystem::JobStatus::Any);
            bool QueryCombinedByProductID(AZ::s64 productID, combinedHandler handler, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), const char* jobKey = nullptr, const char* platform = nullptr, AssetSystem::JobStatus status = AssetSystem::JobStatus::Any);
            bool QueryCombinedBySourceGuidProductSubId(AZ::Uuid sourceGuid, AZ::u32 productSubID, combinedHandler handler, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), const char* jobKey = nullptr, const char* platform = nullptr, AssetSystem::JobStatus status = AssetSystem::JobStatus::Any);

            bool QueryCombinedBySourceName(const char* exactSourceName, combinedHandler handler, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), const char* jobKey = nullptr, const char* platform = nullptr, AssetSystem::JobStatus status = AssetSystem::JobStatus::Any);
            bool QueryCombinedLikeSourceName(const char* likeSourceName, LikeType likeType, combinedHandler handler, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), const char* jobKey = nullptr, const char* platform = nullptr, AssetSystem::JobStatus status = AssetSystem::JobStatus::Any);

            bool QueryCombinedByProductName(const char* productName, combinedHandler handler, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), const char* jobKey = nullptr, const char* platform = nullptr, AssetSystem::JobStatus status = AssetSystem::JobStatus::Any);
            bool QueryCombinedLikeProductName(const char* productName, LikeType likeType, combinedHandler handler, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), const char* jobKey = nullptr, const char* platform = nullptr, AssetSystem::JobStatus status = AssetSystem::JobStatus::Any);

            //jobinfo
            bool QueryJobInfoByJobID(AZ::s64 jobID, jobInfoHandler handler);
            bool QueryJobInfoByJobRunKey(AZ::u64 jobRunKey, jobInfoHandler handler);
            bool QueryJobInfoByJobKey(AZStd::string jobKey, jobInfoHandler handler);
            bool QueryJobInfoBySourceNameScanFolderId(const char* sourceName, AZ::s64 scanfolderId, jobInfoHandler handler, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), const char* jobKey = nullptr, const char* platform = nullptr, AssetSystem::JobStatus status = AssetSystem::JobStatus::Any);


            //SourceDependency
            //! Query all source dependencies
            bool QuerySourceDependencies(sourceFileDependencyHandler handler);
            //! direct query - look up table row by row ID
            bool QuerySourceDependencyBySourceDependencyId(AZ::s64 sourceDependencyID, sourceFileDependencyHandler handler);

            //! Query sources which depend on 'dependsOnSource'
            //! Reverse dependencies are incoming dependencies: what assets depend on me?
            bool QuerySourceDependencyByDependsOnSource(
                AZ::Uuid sourceGuid,
                const char* sourceName,
                const char* scanFolderPath,
                AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::TypeOfDependency dependencyType,
                sourceFileDependencyHandler handler);

            //! Attempt to match either DEP_SourcetoSource or DEP_JobToJob
            //! Then allow DEP_SourceLikeMatch with Wildcard characters
            bool QuerySourceDependencyByDependsOnSourceWildcard(AZ::Uuid sourceGuid, const char* sourceName, const char* absolutePath, sourceFileDependencyHandler handler);

            //! Query everything 'sourceDependency' depends on.
            //! Optional nullable 'dependentFilter' filters it to only resulting dependencies which are LIKE the filter.
            bool QueryDependsOnSourceBySourceDependency(AZ::Uuid sourceGuid, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::TypeOfDependency dependencyType, sourceFileDependencyHandler handler);

            // Returns all sources who's products depend on any of the products of the specified source
            bool QueryProductDependenciesThatDependOnProductBySourceId(AZ::s64 sourceId, productDependencyHandler handler);

            //ProductDependencies
            bool QueryProductDependencyByProductDependencyId(AZ::s64 productDependencyID, productDependencyHandler handler);
            bool QueryProductDependencyByProductId(AZ::s64 productID, productDependencyHandler handler);
            bool QueryProductDependencyBySourceGuidSubId(AZ::Uuid guid, AZ::u32 subId, const AZStd::string& platform, productDependencyHandler handler);
            bool QueryDirectProductDependencies(AZ::s64 productID, productHandler handler);

            //! Query products which depend on the product with "dependencySubId" and whose source file has "dependencySourceGuid")
            //! These finds all incoming dependencies: everything that depends on the given asset.
            //! Similar to QueryDirectProductDependencies, this query deals with product dependencies but retrieves rows from Products, not ProductDependencies.
            bool QueryDirectReverseProductDependenciesBySourceGuidSubId(AZ::Uuid dependencySourceGuid, AZ::u32 dependencySubId, productHandler handler);
            bool QueryDirectReverseProductDependenciesBySourceGuidAllPlatforms(
                AZ::Uuid dependencySourceGuid,
                productDependencyHandler handler);
            bool QueryAllProductDependencies(AZ::s64 productID, productHandler handler);

            bool QueryUnresolvedProductDependencies(productDependencyHandler handler);
            bool QueryProductDependencyExclusions(productDependencyHandler handler);

            //! Returns any unresolved dependencies which match (by exact or wildcard match) the input searchPaths
            //! The extra path returned for each row is the searchPath entry that was matched with the returned dependency entry
            //! @param searchPaths vector of relative paths to search for matches
            bool QueryProductDependenciesUnresolvedAdvanced(const AZStd::vector<AZStd::string>& searchPaths, productDependencyAndPathHandler handler);

            bool QueryMissingProductDependencyByProductId(AZ::s64 productId, missingProductDependencyHandler handler);
            bool DeleteMissingProductDependencyByProductId(AZ::s64 productId);
            bool QueryMissingProductDependencyByMissingProductDependencyId(AZ::s64 productDependencyId, missingProductDependencyHandler handler);

            //FileInfo
            bool QueryFileByFileID(AZ::s64 fileID, fileHandler handler);
            bool QueryFilesByFileNameAndScanFolderID(const char* fileName, AZ::s64 scanfolderID, fileHandler handler);
            bool QueryFilesLikeFileNameAndScanFolderID(const char* likeFileName, LikeType likeType, AZ::s64 scanfolderID, fileHandler handler);
            bool QueryFilesByScanFolderID(AZ::s64 scanFolderID, fileHandler handler);
            bool QueryFileByFileNameScanFolderID(const char* fileName, AZ::s64 scanFolderID, fileHandler handler);

            //Stat
            bool QueryStatByStatName(const char* statName, statHandler handler);
            bool QueryStatLikeStatName(const char* statName, statHandler handler);

            //////////////////////////////////////////////////////////////////////////

            void SetQueryLogging(bool enableLogging);
            void LogQuery(const char* statement, const AZStd::string& params) override;
            void LogResultId(AZ::s64 rowId) override;
        protected:

            SQLite::Connection* m_databaseConnection;

            //override in your derived class when you need to add specific queries
            virtual void CreateStatements(); // when overriding call the base first then add additional statements

            //return the path to the database
            virtual AZStd::string GetAssetDatabaseFilePath();

            //validation
            bool ValidateDatabaseTable(const char* callName, const char* tableName);

            //boiler plate
            auto GetCombinedResultAsLambda(); // helper to wrap up GetCombinedResult in a lambda, since GetCombinedResult is a member function
            bool GetCombinedResult(const char* callName, SQLite::Statement* statement, AssetDatabaseConnection::combinedHandler handler, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), const char* jobKey = nullptr, AssetSystem::JobStatus status = AssetSystem::JobStatus::Any, bool includeLegacyAssetIDs = false);

            // cache what tables have been validated.  it is assumed that AP is the only app with read-write access to the table, and only
            // one AP can be running on a branch at any given time.  Therefore once the table is validated, there is no reason to continue validating it
            // before every query, since validating it essentially must makes sure it exists.
            AZStd::unordered_set<AZStd::string> m_validatedTables;
        };
    } // namespace AssetDatabase
}// namespace AzToolsFramework

namespace AZStd
{
    template<>
    struct hash<AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry>
    {
        using argument_type = AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry;
        using result_type = size_t;

        result_type operator() (const argument_type& obj) const
        {
            size_t h = 0;
            hash_combine(h, obj.ToString());
            return h;
        }
    };
}

#endif // AZTOOLSFRAMEWORK_Connection_H
