#ifndef ASSETPROCESSOR_ASSETDATABASE_H
#define ASSETPROCESSOR_ASSETDATABASE_H

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzToolsFramework/AssetDatabase/AssetDatabaseConnection.h>

#include <QtCore/QSet>
#include <QtCore/QString>

#include "AzToolsFramework/API/EditorAssetSystemAPI.h"
#include <native/AssetManager/SourceAssetReference.h>

class QStringList;

namespace AssetProcessor
{
    //! the Asset Processor's database manager's job is to create and modify the actual underlying
    //! SQL database. All queries to make changes to the database go through here.  This includes
    //! connecting to existing database and altering or creating database tables, etc.
    class AssetDatabaseConnection
        : public AzToolsFramework::AssetDatabase::AssetDatabaseConnection
    {
    public:
        AZ_CLASS_ALLOCATOR(AssetDatabaseConnection, AZ::SystemAllocator, 0);

        AssetDatabaseConnection();
        ~AssetDatabaseConnection();

        //////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::AssetDatabase::Connection
    public:
        bool IsReadOnly() const override
        {
            return false;// return false, we actually curate/write to this database.
        }
        void VacuumAndAnalyze();

    protected:
        void CreateStatements() override;
        bool PostOpenDatabase(bool ignoreFutureAssetDBVersionError) override;
        //////////////////////////////////////////////////////////////////////////

    public:
        bool DataExists();
        void LoadData();
        void ClearData();

        //////////////////////////////////////////////////////////////////////////
        //Queries
        //NOTE: When passing in a structure to the Set<> functions, a default constructed structure has -1 for
        //the table generated id and is filled in by the query, that is why it is passed by non const reference.
        //For instance when SetSource(scanFolderEntry); is called it evaluates the query and fills in the
        //main m_sourceID from the query. If you pass in a structure with the id already filled in, i.e. not -1,
        //it is interpreted as an update to the database only if the contents differ in any way from whats
        //already in he database. Obviously if it is filled in and does not exist it returns false.
        //NOTE: The return bool for these queries only return true if both the query succeeded AND you got a result

        //////////////////////////////////////////////////////////////////////////
        //scan folders
        bool GetScanFolders(AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntryContainer& container);
        bool GetScanFolderByScanFolderID(AZ::s64 scanFolderID, AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry& entry);
        bool GetScanFolderBySourceID(AZ::s64 sourceID, AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry& entry);
        bool GetScanFolderByJobID(AZ::s64 jobID, AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry& entry);
        bool GetScanFolderByProductID(AZ::s64 productID, AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry& entry);

        bool GetScanFolderByPortableKey(QString portableKey, AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry& entry);
        bool SetScanFolder(AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry& entry); //on success sets scanfolderID, if already exists updates it
        bool RemoveScanFolder(AZ::s64 scanFolderID);
        bool RemoveScanFolders(AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntryContainer& container);

        //sources
        bool GetSources(AzToolsFramework::AssetDatabase::SourceDatabaseEntryContainer& container);

        bool GetSourceBySourceGuid(AZ::Uuid sourceGuid, AzToolsFramework::AssetDatabase::SourceDatabaseEntry& entry);

        bool GetSourceBySourceID(AZ::s64 sourceID, AzToolsFramework::AssetDatabase::SourceDatabaseEntry& entry);
        bool GetSourceBySourceNameScanFolderId(QString exactSourceName, AZ::s64 scanFolderID, AzToolsFramework::AssetDatabase::SourceDatabaseEntry& entry);
        bool GetSourcesBySourceName(QString exactSourceName, AzToolsFramework::AssetDatabase::SourceDatabaseEntryContainer& container);
        bool GetSourcesLikeSourceName(QString likeSourceName, LikeType likeType, AzToolsFramework::AssetDatabase::SourceDatabaseEntryContainer& container);
        bool GetSourcesLikeSourceNameScanFolderId(QString likeSourceName, AZ::s64 scanFolderID, LikeType likeType, AzToolsFramework::AssetDatabase::SourceDatabaseEntryContainer& container);

        bool GetSourceByJobID(AZ::s64 jobID, AzToolsFramework::AssetDatabase::SourceDatabaseEntry& entry);

        bool GetSourceByProductID(AZ::s64 productID, AzToolsFramework::AssetDatabase::SourceDatabaseEntry& source);
        bool GetSourcesByProductName(QString exactProductName, AzToolsFramework::AssetDatabase::SourceDatabaseEntryContainer& container);
        bool GetSourcesLikeProductName(QString likeProductName, LikeType likeType, AzToolsFramework::AssetDatabase::SourceDatabaseEntryContainer& container);

        bool SetSource(AzToolsFramework::AssetDatabase::SourceDatabaseEntry& entry); //on success sets sourceID, if it already exists updates it
        bool RemoveSource(AZ::s64 sourceID);
        bool RemoveSources(AzToolsFramework::AssetDatabase::SourceDatabaseEntryContainer& container);
        bool RemoveSourcesByScanFolderID(AZ::s64 scanFolderID);

        bool InvalidateSourceAnalysisFingerprints();

        //jobs

        // used to initialize the predictor for job Run Keys
        AZ::s64 GetHighestJobRunKey();
        bool GetJobs(AzToolsFramework::AssetDatabase::JobDatabaseEntryContainer& container, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), QString jobKey = QString(), QString platform = QString(), AzToolsFramework::AssetSystem::JobStatus status = AzToolsFramework::AssetSystem::JobStatus::Any);
        bool GetJobByJobID(AZ::s64 jobID, AzToolsFramework::AssetDatabase::JobDatabaseEntry& entry);
        bool GetJobByProductID(AZ::s64 productID, AzToolsFramework::AssetDatabase::JobDatabaseEntry& entry);

        bool GetJobsBySourceID(AZ::s64 sourceID, AzToolsFramework::AssetDatabase::JobDatabaseEntryContainer& container, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), QString jobKey = QString(), QString platform = QString(), AzToolsFramework::AssetSystem::JobStatus status = AzToolsFramework::AssetSystem::JobStatus::Any);
        bool GetJobsBySourceName(const AssetProcessor::SourceAssetReference& sourceAsset, AzToolsFramework::AssetDatabase::JobDatabaseEntryContainer& container, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), QString jobKey = QString(), QString platform = QString(), AzToolsFramework::AssetSystem::JobStatus status = AzToolsFramework::AssetSystem::JobStatus::Any);
        bool GetJobsLikeSourceName(QString likeSourceName, LikeType likeType, AzToolsFramework::AssetDatabase::JobDatabaseEntryContainer& container, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), QString jobKey = QString(), QString platform = QString(), AzToolsFramework::AssetSystem::JobStatus status = AzToolsFramework::AssetSystem::JobStatus::Any);

        bool GetJobsByProductName(QString exactProductName, AzToolsFramework::AssetDatabase::JobDatabaseEntryContainer& container, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), QString jobKey = QString(), QString platform = QString(), AzToolsFramework::AssetSystem::JobStatus status = AzToolsFramework::AssetSystem::JobStatus::Any);
        bool GetJobsLikeProductName(QString likeProductName, LikeType likeType, AzToolsFramework::AssetDatabase::JobDatabaseEntryContainer& container, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), QString jobKey = QString(), QString platform = QString(), AzToolsFramework::AssetSystem::JobStatus status = AzToolsFramework::AssetSystem::JobStatus::Any);

        bool SetJob(AzToolsFramework::AssetDatabase::JobDatabaseEntry& entry); //on success sets jobID, if it already exists updates it
        bool RemoveJob(AZ::s64 jobID);
        bool RemoveJobs(AzToolsFramework::AssetDatabase::JobDatabaseEntryContainer& container);
        bool RemoveJobByProductID(AZ::s64 productID);

        //products
        bool GetProducts(AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer& container, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), QString jobKey = QString(), QString platform = QString(), AzToolsFramework::AssetSystem::JobStatus status = AzToolsFramework::AssetSystem::JobStatus::Any);
        bool GetProductsByJobID(AZ::s64 jobID, AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer& container);
        // note that the pair of (JobID, SubID) uniquely identifies a single job, and thus the result is always only one entry:
        bool GetProductByJobIDSubId(AZ::s64 jobID, AZ::u32 subID, AzToolsFramework::AssetDatabase::ProductDatabaseEntry& result);
        bool GetProductBySourceGuidSubId(AZ::Uuid sourceGuid, AZ::u32 subId, AzToolsFramework::AssetDatabase::ProductDatabaseEntry& result);

        bool GetProductByProductID(AZ::s64 productID, AzToolsFramework::AssetDatabase::ProductDatabaseEntry& entry);
        bool GetProductsByProductName(QString exactProductName, AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer& container, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), QString jobKey = QString(), QString platform = QString(), AzToolsFramework::AssetSystem::JobStatus status = AzToolsFramework::AssetSystem::JobStatus::Any);
        bool GetProductsLikeProductName(QString likeProductName, LikeType likeType, AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer& container, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), QString jobKey = QString(), QString platform = QString(), AzToolsFramework::AssetSystem::JobStatus status = AzToolsFramework::AssetSystem::JobStatus::Any);

        bool GetProductsBySourceID(AZ::s64 sourceID, AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer& container, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), QString jobKey = QString(), QString platform = QString(), AzToolsFramework::AssetSystem::JobStatus status = AzToolsFramework::AssetSystem::JobStatus::Any);
        bool GetProductsBySourceName(QString exactSourceName, AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer& container, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), QString jobKey = QString(), QString platform = QString(), AzToolsFramework::AssetSystem::JobStatus status = AzToolsFramework::AssetSystem::JobStatus::Any);
        bool GetProductsLikeSourceName(QString likeSourceName, LikeType likeType, AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer& container, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), QString jobKey = QString(), QString platform = QString(), AzToolsFramework::AssetSystem::JobStatus status = AzToolsFramework::AssetSystem::JobStatus::Any);

        bool SetProduct(AzToolsFramework::AssetDatabase::ProductDatabaseEntry& entry); //on success sets productID, if it already exists updates it
        bool SetProducts(AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer& container); //on success sets productID, if it already exists updates it
        bool RemoveProducts(AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer& container);
        bool RemoveProduct(AZ::s64 productID);
        bool RemoveProductsByJobID(AZ::s64 jobID);
        bool RemoveProductsBySourceID(AZ::s64 sourceID, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), QString jobKey = QString(), QString platform = QString(), AzToolsFramework::AssetSystem::JobStatus status = AzToolsFramework::AssetSystem::JobStatus::Any);

        //jobinfo
        bool GetJobInfoByJobID(AZ::s64 jobID, AzToolsFramework::AssetSystem::JobInfo& jobInfo);
        bool GetJobInfoByJobKey(AZStd::string jobKey, AzToolsFramework::AssetSystem::JobInfoContainer& container);
        bool GetJobInfoByJobRunKey(AZ::u64 jobRunKey, AzToolsFramework::AssetSystem::JobInfoContainer& container);
        bool GetJobInfoBySourceNameScanFolderId(QString exactSourceName, AZ::s64 scanfolderId, AzToolsFramework::AssetSystem::JobInfoContainer& container, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), QString jobKey = QString(), QString platform = QString(), AzToolsFramework::AssetSystem::JobStatus status = AzToolsFramework::AssetSystem::JobStatus::Any);

        /* --------------------- Source Dependency Table -------------------
        *    For example, this table records when a source file depends on another source file either directly (DEP_SourceToSource)
        *    but also whether then source file depends on another source file indirectly because it depends on a job which processes
        *    that source file
        */
        /// Set a row in the table.  It is invalid to overwrite existing rows without removing them first.
        bool SetSourceFileDependency(AzToolsFramework::AssetDatabase::SourceFileDependencyEntry& entry);
        /// Set a batch of rows.  It is invalid to overwrite existing rows, so consider using RemoveSourceFileDependencies first.
        bool SetSourceFileDependencies(AzToolsFramework::AssetDatabase::SourceFileDependencyEntryContainer& container);
        /// Remove a dependency, given a row ID
        bool RemoveSourceFileDependency(AZ::s64 sourceFileDependencyId);
        /// Batch remove a bunch of rows by container
        bool RemoveSourceFileDependencies(const AzToolsFramework::AssetDatabase::SourceFileDependencyEntryContainer& container);
        /// Batch remove a bunch of rows by IDs
        bool RemoveSourceFileDependencies(const AZStd::unordered_set<AZ::s64>& container);
        /// Direct retrieval by ID (does not use any filtering)
        bool GetSourceFileDependencyBySourceDependencyId(AZ::s64 sourceDependencyId, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry& sourceDependencyEntry);

        // The following functions are all search functions (as opposed to the above functions which fetch or operate on specific rows)
        // They tend to take a "Type of Dependency" filter - you can use DEP_Any to query all kinds of dependencies.
        /// Given a source file, what does it DEPEND ON?
        bool GetDependsOnSourceBySource(AZ::Uuid sourceUuid, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::TypeOfDependency typeOfDependency, AzToolsFramework::AssetDatabase::SourceFileDependencyEntryContainer& container);
        /// Given a source file and a builder UUID, does it DEPEND ON?
        bool GetSourceFileDependenciesByBuilderGUIDAndSource(const AZ::Uuid& builderGuid, AZ::Uuid sourceGuid, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::TypeOfDependency typeOfDependency, AzToolsFramework::AssetDatabase::SourceFileDependencyEntryContainer& container);
        /// Given a source file, what depends ON IT? ('reverse dependency')
        bool GetSourceFileDependenciesByDependsOnSource(
            AZ::Uuid sourceGuid,
            const char* sourceName,
            const char* absolutePath,
            AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::TypeOfDependency typeOfDependency,
            AzToolsFramework::AssetDatabase::SourceFileDependencyEntryContainer& container);

        // --------------------- Legacy SUBID table -------------------
        bool CreateOrUpdateLegacySubID(AzToolsFramework::AssetDatabase::LegacySubIDsEntry& entry);  // create or overwrite operation.
        bool RemoveLegacySubID(AZ::s64 legacySubIDsEntryID);
        bool RemoveLegacySubIDsByProductID(AZ::s64 productID);

        //ProductDependencies
        bool GetProductDependencies(AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer& container);
        bool GetProductDependencyByProductDependencyID(AZ::s64 productDependencyID, AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry& productDependencyEntry);
        bool GetProductDependenciesByProductID(AZ::s64 productID, AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer& container);
        bool GetDirectProductDependencies(AZ::s64 productID, AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer& container);
        bool GetDirectReverseProductDependenciesBySourceGuidSubId(AZ::Uuid dependencySourceGuid, AZ::u32 dependencySubId, AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer& container);
        bool GetDirectReverseProductDependenciesBySourceGuidAllPlatforms(
            AZ::Uuid dependencySourceGuid,
            AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer& container);
        bool GetAllProductDependencies(AZ::s64 productID, AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer& container);
        bool GetUnresolvedProductDependencies(AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer& container);
        bool SetProductDependency(AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry& entry);

        // Missing product dependencies
        bool SetMissingProductDependency(AzToolsFramework::AssetDatabase::MissingProductDependencyDatabaseEntry& entry);
        bool GetMissingProductDependenciesByProductId(AZ::s64 productId, AzToolsFramework::AssetDatabase::MissingProductDependencyDatabaseEntryContainer& container);
        bool GetMissingProductDependencyByMissingProductDependencyId(AZ::s64 missingProductDependencyId, AzToolsFramework::AssetDatabase::MissingProductDependencyDatabaseEntry& missingProductDependencyEntry);

        // updates or inserts multiple dependencies in a single transaction.  Unlike SetProductDependencies, this does *not* delete existing dependencies
        bool UpdateProductDependencies(AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer& container);

        // bulk inserts are lighter weight and don't change the input data.  Note that this also deletes old dependencies for the products mentioned in the container.
        bool SetProductDependencies(const AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer& container);

        bool RemoveProductDependencyByProductId(AZ::s64 productID);

        // bulk replace builder info table with new builder info table.  Replaces the existing table of data.
        // Note:  newEntries will have their m_builderInfoID member set to their inserted rowId if this call succeeds.
        bool SetBuilderInfoTable(AzToolsFramework::AssetDatabase::BuilderInfoEntryContainer& newEntries);

        //Files
        bool GetFileByFileID(AZ::s64 fileID, AzToolsFramework::AssetDatabase::FileDatabaseEntry& entry);
        bool GetFileByFileNameAndScanFolderId(QString fileName, AZ::s64 scanFolderId, AzToolsFramework::AssetDatabase::FileDatabaseEntry& entry);
        bool GetFilesLikeFileNameScanFolderId(QString likeFileName, LikeType likeType, AZ::s64 scanFolderId, AzToolsFramework::AssetDatabase::FileDatabaseEntryContainer& container);

        bool InsertFiles(AzToolsFramework::AssetDatabase::FileDatabaseEntryContainer& entry);
        bool InsertFile(AzToolsFramework::AssetDatabase::FileDatabaseEntry& entry, bool& entryAlreadyExists);
        bool UpdateFile(AzToolsFramework::AssetDatabase::FileDatabaseEntry& entry, bool& entryAlreadyExists);

        // updates the modtime and hash for a file if it exists.  Only returns true if the row existed and was successfully updated
        bool UpdateFileModTimeAndHashByFileNameAndScanFolderId(QString fileName, AZ::s64 scanFolderId, AZ::u64 modTime, AZ::u64 hash);
        bool RemoveFile(AZ::s64 sourceID);

        //Stats
        bool GetStatByStatName(QString statName, AzToolsFramework::AssetDatabase::StatDatabaseEntryContainer& container);
        bool GetStatLikeStatName(QString statName, AzToolsFramework::AssetDatabase::StatDatabaseEntryContainer& container);
        bool ReplaceStat(AzToolsFramework::AssetDatabase::StatDatabaseEntry& stat);
    protected:
        void SetDatabaseVersion(AzToolsFramework::AssetDatabase::DatabaseVersion ver);
        void ExecuteCreateStatements();

    private:
        AZStd::vector<AZStd::string> m_createStatements; // contains all statements required to create the tables
    };
}//namespace EditorFramework

#endif // ASSETPROCESSOR_ASSETDATABASE_H
