/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/AssetDatabase/AssetDatabaseConnection.h>

#include <sqlite3.h>

#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/SystemFile.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/SQLite/SQLiteConnection.h>
#include <AzToolsFramework/API/AssetDatabaseBus.h>
#include <AzToolsFramework/SQLite/SQLiteQuery.h>
#include <AzToolsFramework/SQLite/SQLiteBoundColumnSet.h>
#include <cinttypes>

namespace AzToolsFramework
{
    namespace AssetDatabase
    {
        using namespace AzFramework;
        using namespace AzToolsFramework::SQLite;

        //since we can be derived from, prefix the namespace to avoid statement name collisions
        namespace
        {
            static const char* LOG_NAME = "AzToolsFramework::AssetDatabase";

            // when you add a table, be sure to add it here to check the database for corruption
            static const char* EXPECTED_TABLES[] = {
                "BuilderInfo",
                "Files",
                "Jobs",
                "LegacySubIDs",
                "MissingProductDependencies",
                "ProductDependencies",
                "Products",
                "ScanFolders",
                "SourceDependency",
                "Sources",
                "Stats",
                "dbinfo"
            };

            //////////////////////////////////////////////////////////////////////////
            //table queries
            static const char* QUERY_DATABASEINFO_TABLE = "AzToolsFramework::AssetDatabase::QueryDatabaseInfoTable";
            static const char* QUERY_DATABASEINFO_TABLE_STATEMENT =
                "SELECT * from dbinfo;";
            static const auto s_queryDatabaseinfoTable = MakeSqlQuery(QUERY_DATABASEINFO_TABLE, QUERY_DATABASEINFO_TABLE_STATEMENT, LOG_NAME);

            static const char* QUERY_BUILDERINFO_TABLE = "AzToolsFramework::AssetDatabase::QueryBuilderInfo";
            static const char* QUERY_BUILDERINFO_TABLE_STATEMENT =
                "SELECT * from BuilderInfo;";
            static const auto s_queryBuilderInfoTable = MakeSqlQuery(QUERY_BUILDERINFO_TABLE, QUERY_BUILDERINFO_TABLE_STATEMENT, LOG_NAME);

            static const char* QUERY_SCANFOLDERS_TABLE = "AzToolsFramework::AssetDatabase::QueryScanFoldersTable";
            static const char* QUERY_SCANFOLDERS_TABLE_STATEMENT =
                "SELECT * from ScanFolders;";

            static const auto s_queryScanfoldersTable = MakeSqlQuery(QUERY_SCANFOLDERS_TABLE, QUERY_SCANFOLDERS_TABLE_STATEMENT, LOG_NAME);

            static const char* QUERY_SOURCES_TABLE = "AzToolsFramework::AssetDatabase::QuerySourcesTable";
            static const char* QUERY_SOURCES_TABLE_STATEMENT =
                "SELECT * from Sources;";

            static const auto s_querySourcesTable = MakeSqlQuery(QUERY_SOURCES_TABLE, QUERY_SOURCES_TABLE_STATEMENT, LOG_NAME);

            static const char* QUERY_JOBS_TABLE = "AzToolsFramework::AssetDatabase::QueryJobsTable";
            static const char* QUERY_JOBS_TABLE_STATEMENT =
                "SELECT * from Jobs;";

            static const auto s_queryJobsTable = MakeSqlQuery(QUERY_JOBS_TABLE, QUERY_JOBS_TABLE_STATEMENT, LOG_NAME);

            static const char* QUERY_JOBS_TABLE_PLATFORM = "AzToolsFramework::AssetDatabase::QueryJobsTablePlatform";
            static const char* QUERY_JOBS_TABLE_PLATFORM_STATEMENT =
                "SELECT * from Jobs WHERE "
                "Platform = :platform;";

            static const auto s_queryJobsTablePlatform = MakeSqlQuery(QUERY_JOBS_TABLE_PLATFORM, QUERY_JOBS_TABLE_PLATFORM_STATEMENT, LOG_NAME,
                    SqlParam<const char*>(":platform"));

            static const char* QUERY_PRODUCTS_TABLE = "AzToolsFramework::AssetDatabase::QueryProductsTable";
            static const char* QUERY_PRODUCTS_TABLE_STATEMENT =
                "SELECT * from Products INNER JOIN Jobs "
                "ON Products.JobPK = Jobs.JobID;";

            static const auto s_queryProductsTable = MakeSqlQuery(QUERY_PRODUCTS_TABLE, QUERY_PRODUCTS_TABLE_STATEMENT, LOG_NAME);

            static const char* QUERY_PRODUCTS_TABLE_PLATFORM = "AzToolsFramework::AssetDatabase::QueryProductsTablePlatform";
            static const char* QUERY_PRODUCTS_TABLE_PLATFORM_STATEMENT =
                "SELECT * from Products INNER JOIN Jobs "
                "ON Products.JobPK = Jobs.JobID WHERE "
                "Jobs.Platform = :platform;";

            static const auto s_queryProductsTablePlatform = MakeSqlQuery(QUERY_PRODUCTS_TABLE_PLATFORM, QUERY_PRODUCTS_TABLE_PLATFORM_STATEMENT, LOG_NAME,
                    SqlParam<const char*>(":platform"));

            static const char* QUERY_LEGACYSUBIDSBYPRODUCTID = "AzToolsFramework::AssetDatabase::QueryLegacySubIDsByProductID";
            static const char* QUERY_LEGACYSUBIDSBYPRODUCTID_STATEMENT =
                "SELECT * from LegacySubIDs "
                " WHERE "
                "   LegacySubIDs.ProductPK = :productId;";

            static const auto s_queryLegacysubidsbyproductid = MakeSqlQuery(QUERY_LEGACYSUBIDSBYPRODUCTID, QUERY_LEGACYSUBIDSBYPRODUCTID_STATEMENT, LOG_NAME,
                    SqlParam<AZ::s64>(":productId"));

            static const char* QUERY_PRODUCTDEPENDENCIES_TABLE = "AzToolsFramework::AssetDatabase::QueryProductDependencies";
            static const char* QUERY_PRODUCTDEPENDENCIES_TABLE_STATEMENT =
                "SELECT ProductDependencies.*, SourceGUID, SubID FROM ProductDependencies "
                "INNER JOIN Products ON ProductPK = ProductID "
                "INNER JOIN Jobs ON JobPK = JobID "
                "INNER JOIN Sources ON SourcePK = SourceID;";

            static const auto s_queryProductdependenciesTable = MakeSqlQuery(QUERY_PRODUCTDEPENDENCIES_TABLE, QUERY_PRODUCTDEPENDENCIES_TABLE_STATEMENT, LOG_NAME);

            static const char* QUERY_FILES_TABLE = "AzToolsFramework::AssetDatabase::QueryFilesTable";
            static const char* QUERY_FILES_TABLE_STATEMENT =
                "SELECT * from Files;";

            static const auto s_queryFilesTable = MakeSqlQuery(QUERY_FILES_TABLE, QUERY_FILES_TABLE_STATEMENT, LOG_NAME);

            static const char* QUERY_STATS_TABLE = "AzToolsFramework::AssetDatabase::QueryStatsTable";
            static const char* QUERY_STATS_TABLE_STATEMENT = "SELECT * from Stats;";

            static const auto s_queryStatsTable = MakeSqlQuery(QUERY_STATS_TABLE, QUERY_STATS_TABLE_STATEMENT, LOG_NAME);

            //////////////////////////////////////////////////////////////////////////
            //projection and combination queries

            // lookup by primary key
            static const char* QUERY_SCANFOLDER_BY_SCANFOLDERID = "AzToolsFramework::AssetDatabase::QueryScanfolderByScanfolderID";
            static const char* QUERY_SCANFOLDER_BY_SCANFOLDERID_STATEMENT =
                "SELECT * FROM ScanFolders WHERE "
                "ScanFolderID = :scanfolderid;";

            static const auto s_queryScanfolderByScanfolderid = MakeSqlQuery(QUERY_SCANFOLDER_BY_SCANFOLDERID, QUERY_SCANFOLDER_BY_SCANFOLDERID_STATEMENT, LOG_NAME,
                    SqlParam<AZ::s64>(":scanfolderid"));

            static const char* QUERY_SCANFOLDER_BY_DISPLAYNAME = "AzToolsFramework::AssetDatabase::QueryScanfolderByDisplayName";
            static const char* QUERY_SCANFOLDER_BY_DISPLAYNAME_STATEMENT =
                "SELECT * FROM ScanFolders WHERE "
                "DisplayName = :displayname;";

            static const auto s_queryScanfolderByDisplayname = MakeSqlQuery(QUERY_SCANFOLDER_BY_DISPLAYNAME, QUERY_SCANFOLDER_BY_DISPLAYNAME_STATEMENT, LOG_NAME,
                    SqlParam<const char*>(":displayname"));

            static const char* QUERY_SCANFOLDER_BY_PORTABLEKEY = "AzToolsFramework::AssetDatabase::QueryScanfolderByPortableKey";
            static const char* QUERY_SCANFOLDER_BY_PORTABLEKEY_STATEMENT =
                "SELECT * FROM ScanFolders WHERE "
                "PortableKey = :portablekey;";

            static const auto s_queryScanfolderByPortablekey = MakeSqlQuery(QUERY_SCANFOLDER_BY_PORTABLEKEY, QUERY_SCANFOLDER_BY_PORTABLEKEY_STATEMENT, LOG_NAME,
                    SqlParam<const char*>(":portablekey"));

            // lookup by primary key
            static const char* QUERY_SOURCE_BY_SOURCEID = "AzToolsFramework::AssetDatabase::QuerySourceBySourceID";
            static const char* QUERY_SOURCE_BY_SOURCEID_STATEMENT =
                "SELECT * FROM Sources WHERE "
                "SourceID = :sourceid;";

            static const auto s_querySourceBySourceid = MakeSqlQuery(QUERY_SOURCE_BY_SOURCEID, QUERY_SOURCE_BY_SOURCEID_STATEMENT, LOG_NAME,
                    SqlParam<AZ::s64>(":sourceid"));

            static const char* QUERY_SOURCE_BY_SCANFOLDERID = "AzToolsFramework::AssetDatabase::QuerySourceByScanFolderID";
            static const char* QUERY_SOURCE_BY_SCANFOLDERID_STATEMENT =
                "SELECT * FROM Sources WHERE "
                "ScanFolderPK = :scanfolderid;";

            static const auto s_querySourceByScanfolderid = MakeSqlQuery(QUERY_SOURCE_BY_SCANFOLDERID, QUERY_SOURCE_BY_SCANFOLDERID_STATEMENT, LOG_NAME,
                    SqlParam<AZ::s64>(":scanfolderid"));

            static const char* QUERY_SOURCE_BY_SOURCEGUID = "AzToolsFramework::AssetDatabase::QuerySourceBySourceGuid";
            static const char* QUERY_SOURCE_BY_SOURCEGUID_STATEMENT =
                "SELECT * FROM Sources WHERE "
                "SourceGuid = :sourceguid;";

            static const auto s_querySourceBySourceguid = MakeSqlQuery(QUERY_SOURCE_BY_SOURCEGUID, QUERY_SOURCE_BY_SOURCEGUID_STATEMENT, LOG_NAME,
                    SqlParam<AZ::Uuid>(":sourceguid"));

            static const char* QUERY_SOURCE_BY_SOURCENAME = "AzToolsFramework::AssetDatabase::QuerySourceBySourceName";
            static const char* QUERY_SOURCE_BY_SOURCENAME_STATEMENT =
                "SELECT * FROM Sources WHERE "
                "SourceName = :sourcename;";

            static const auto s_querySourceBySourcename = MakeSqlQuery(QUERY_SOURCE_BY_SOURCENAME, QUERY_SOURCE_BY_SOURCENAME_STATEMENT, LOG_NAME,
                    SqlParam<const char*>(":sourcename"));

            static const char* QUERY_SOURCE_BY_SOURCENAME_SCANFOLDERID = "AzToolsFramework::AssetDatabase::QuerySourceBySourceNameScanFolderID";
            static const char* QUERY_SOURCE_BY_SOURCENAME_SCANFOLDERID_STATEMENT =
                "SELECT * FROM Sources WHERE "
                "SourceName = :sourcename AND "
                "ScanFolderPK = :scanfolderid;";
            static const auto s_querySourceBySourcenameScanfolderid = MakeSqlQuery(QUERY_SOURCE_BY_SOURCENAME_SCANFOLDERID, QUERY_SOURCE_BY_SOURCENAME_SCANFOLDERID_STATEMENT, LOG_NAME,
                SqlParam<const char*>(":sourcename"),
                SqlParam<AZ::s64>(":scanfolderid"));

            static const char* QUERY_SOURCE_ANALYSISFINGERPRINT = "AzToolsFramework::AssetDatabase::QuerySourceFingerprint";
            static const char* QUERY_SOURCE_ANALYSISFINGERPRINT_STATEMENT =
                "SELECT AnalysisFingerprint FROM Sources WHERE "
                "SourceName = :sourcename AND "
                "ScanFolderPK = :scanfolderid;";
            static const auto s_querySourceAnalysisFingerprint = MakeSqlQuery(QUERY_SOURCE_ANALYSISFINGERPRINT, QUERY_SOURCE_ANALYSISFINGERPRINT_STATEMENT, LOG_NAME,
                SqlParam<const char*>(":sourcename"),
                SqlParam<AZ::s64>(":scanfolderid"));

            static const char* QUERY_SOURCES_AND_SCANFOLDERS = "AzToolsFramework::AssetDatabase::QuerySourcesAndScanfolders";
            static const char* QUERY_SOURCES_AND_SCANFOLDERS_STATEMENT =
                "SELECT * FROM Sources "
                "LEFT OUTER JOIN ScanFolders ON ScanFolderPK = ScanFolderID;";

            static const auto s_querySourcesAndScanfolders = MakeSqlQuery(QUERY_SOURCES_AND_SCANFOLDERS, QUERY_SOURCES_AND_SCANFOLDERS_STATEMENT, LOG_NAME);

            static const char* QUERY_SOURCE_LIKE_SOURCENAME = "AzToolsFramework::AssetDatabase::QuerySourceLikeSourceName";
            static const char* QUERY_SOURCE_LIKE_SOURCENAME_STATEMENT =
                "SELECT * FROM Sources WHERE "
                "SourceName LIKE :sourcename ESCAPE '|';"; // use the pipe to escape since its NOT a valid file path or operator

            static const auto s_querySourceLikeSourcename = MakeSqlQuery(QUERY_SOURCE_LIKE_SOURCENAME, QUERY_SOURCE_LIKE_SOURCENAME_STATEMENT, LOG_NAME,
                    SqlParam<const char*>(":sourcename"));

            static const char* QUERY_SOURCE_LIKE_SOURCENAME_SCANFOLDER =
                "AzToolsFramework::AssetDatabase::QuerySourceLikeSourceNameScanfolder";
            static const char* QUERY_SOURCE_LIKE_SOURCENAME_SCANFOLDER_STATEMENT =
                "SELECT * FROM Sources WHERE "
                "SourceName LIKE :sourcename ESCAPE '|' "
                "AND ScanFolderPK = :scanfolder ";

            static const auto s_querySourceLikeSourcenameScanfolder = MakeSqlQuery(
                QUERY_SOURCE_LIKE_SOURCENAME_SCANFOLDER,
                QUERY_SOURCE_LIKE_SOURCENAME_SCANFOLDER_STATEMENT,
                LOG_NAME,
                SqlParam<const char*>(":sourcename"),
                SqlParam<AZ::s64>(":scanfolder"));

            // lookup by primary key
            static const char* QUERY_JOB_BY_JOBID = "AzToolsFramework::AssetDatabase::QueryJobByJobID";
            static const char* QUERY_JOB_BY_JOBID_STATEMENT =
                "SELECT * FROM Jobs WHERE "
                "JobID = :jobid;";

            static const auto s_queryJobByJobid = MakeSqlQuery(QUERY_JOB_BY_JOBID, QUERY_JOB_BY_JOBID_STATEMENT, LOG_NAME,
                    SqlParam<AZ::s64>(":jobid"));

            static const char* QUERY_JOB_BY_JOBKEY = "AzToolsFramework::AssetDatabase::QueryJobByJobKey";
            static const char* QUERY_JOB_BY_JOBKEY_STATEMENT =
                "SELECT * FROM Jobs WHERE "
                "JobKey = :jobKey;";

            static const auto s_queryJobByJobkey = MakeSqlQuery(QUERY_JOB_BY_JOBKEY, QUERY_JOB_BY_JOBKEY_STATEMENT, LOG_NAME,
                    SqlParam<const char*>(":jobKey"));

            static const char* QUERY_JOB_BY_JOBRUNKEY = "AzToolsFramework::AssetDatabase::QueryJobByJobRunKey";
            static const char* QUERY_JOB_BY_JOBRUNKEY_STATEMENT =
                "SELECT * FROM Jobs WHERE "
                "JobRunKey = :jobrunkey;";

            static const auto s_queryJobByJobrunkey = MakeSqlQuery(QUERY_JOB_BY_JOBRUNKEY, QUERY_JOB_BY_JOBRUNKEY_STATEMENT, LOG_NAME,
                    SqlParam<AZ::u64>(":jobrunkey"));

            static const char* QUERY_JOB_BY_PRODUCTID = "AzToolsFramework::AssetDatabase::QueryJobByProductID";
            static const char* QUERY_JOB_BY_PRODUCTID_STATEMENT =
                "SELECT Jobs.* FROM Jobs INNER JOIN Products "
                "ON Jobs.JobID = Products.JobPK WHERE "
                "Products.ProductID = :productid;";

            static const auto s_queryJobByProductid = MakeSqlQuery(QUERY_JOB_BY_PRODUCTID, QUERY_JOB_BY_PRODUCTID_STATEMENT, LOG_NAME,
                    SqlParam<AZ::s64>(":productid"));

            static const char* QUERY_JOB_BY_SOURCEID = "AzToolsFramework::AssetDatabase::QueryJobBySourceID";
            static const char* QUERY_JOB_BY_SOURCEID_STATEMENT =
                "SELECT * FROM Jobs WHERE "
                "SourcePK = :sourceid;";

            static const auto s_queryJobBySourceid = MakeSqlQuery(QUERY_JOB_BY_SOURCEID, QUERY_JOB_BY_SOURCEID_STATEMENT, LOG_NAME,
                    SqlParam<AZ::s64>(":sourceid"));

            static const char* QUERY_JOB_BY_SOURCEID_PLATFORM = "AzToolsFramework::AssetDatabase::QueryJobBySourceIDPlatform";
            static const char* QUERY_JOB_BY_SOURCEID_PLATFORM_STATEMENT =
                "SELECT * FROM Jobs WHERE "
                "SourcePK = :sourceid AND "
                "Platform = :platform;";

            static const auto s_queryJobBySourceidPlatform = MakeSqlQuery(QUERY_JOB_BY_SOURCEID_PLATFORM, QUERY_JOB_BY_SOURCEID_PLATFORM_STATEMENT, LOG_NAME,
                    SqlParam<AZ::s64>(":sourceid"),
                    SqlParam<const char*>(":platform"));

            static const char* QUERY_JOBS_BY_FAILURECAUSESOURCEID = "AzToolsFramework::AssetDatabase::QueryJobByFailureCauseSourceID";
            static const char* QUERY_JOBS_BY_FAILURECAUSESOURCEID_STATEMENT =
                "SELECT * FROM Jobs WHERE "
                "FailureCauseSourcePK = :sourceid;";

            static const auto s_queryJobsByFailureCauseSourceId = MakeSqlQuery(QUERY_JOBS_BY_FAILURECAUSESOURCEID, QUERY_JOBS_BY_FAILURECAUSESOURCEID_STATEMENT, LOG_NAME,
                SqlParam<AZ::s64>(":sourceid"));

            // lookup by primary key
            static const char* QUERY_PRODUCT_BY_PRODUCTID = "AzToolsFramework::AssetDatabase::QueryProductByProductID";
            static const char* QUERY_PRODUCT_BY_PRODUCTID_STATEMENT =
                "SELECT Products.*, Jobs.* FROM Jobs INNER JOIN Products "
                "ON Jobs.JobID = Products.JobPK WHERE "
                "Products.ProductID = :productid;";

            static const auto s_queryProductByProductid = MakeSqlQuery(QUERY_PRODUCT_BY_PRODUCTID, QUERY_PRODUCT_BY_PRODUCTID_STATEMENT, LOG_NAME,
                    SqlParam<AZ::s64>(":productid"));

            static const char* QUERY_PRODUCT_BY_JOBID = "AzToolsFramework::AssetDatabase::QueryProductByJobID";
            static const char* QUERY_PRODUCT_BY_JOBID_STATEMENT =
                "SELECT Products.*, Jobs.* FROM Jobs INNER JOIN Products "
                "ON Jobs.JobID = Products.JobPK WHERE "
                "Products.JobPK = :jobid;";

            static const auto s_queryProductByJobid = MakeSqlQuery(QUERY_PRODUCT_BY_JOBID, QUERY_PRODUCT_BY_JOBID_STATEMENT, LOG_NAME,
                    SqlParam<AZ::s64>(":jobid"));

            static const char* QUERY_PRODUCT_BY_JOBID_PLATFORM = "AzToolsFramework::AssetDatabase::QueryProductByJobIDPlatform";
            static const char* QUERY_PRODUCT_BY_JOBID_PLATFORM_STATEMENT =
                "SELECT Products.*, Jobs.* FROM Jobs INNER JOIN Products "
                "ON Jobs.JobID = Products.JobPK WHERE "
                "Products.JobPK = :jobid AND "
                "Jobs.Platform = :platform;";

            static const auto s_queryProductByJobidPlatform = MakeSqlQuery(QUERY_PRODUCT_BY_JOBID_PLATFORM, QUERY_PRODUCT_BY_JOBID_PLATFORM_STATEMENT, LOG_NAME,
                    SqlParam<AZ::s64>(":jobid"),
                    SqlParam<const char*>(":platform"));

            static const char* QUERY_PRODUCT_BY_SOURCEID = "AzToolsFramework::AssetDatabase::QueryProductBySourceID";
            static const char* QUERY_PRODUCT_BY_SOURCEID_STATEMENT =
                "SELECT Products.*, Jobs.* FROM Jobs INNER JOIN Products "
                "ON Jobs.JobID = Products.JobPK WHERE "
                "Jobs.SourcePK = :sourceid;";

            static const auto s_queryProductBySourceid = MakeSqlQuery(QUERY_PRODUCT_BY_SOURCEID, QUERY_PRODUCT_BY_SOURCEID_STATEMENT, LOG_NAME,
                    SqlParam<AZ::s64>(":sourceid"));

            static const char* QUERY_PRODUCT_BY_SOURCEGUID_SUBID = "AzToolsFramework::AssetDatabase::QueryProductBySourceGuidSubid";
            static const char* QUERY_PRODUCT_BY_SOURCEGUID_SUBID_STATEMENT =
                "SELECT Sources.SourceGuid, Products.* FROM Sources INNER JOIN Jobs "
                "ON Sources.SourceID = Jobs.SourcePK INNER JOIN Products "
                "ON Jobs.JobID = Products.JobPK WHERE "
                "Sources.SourceGuid = :sourceguid AND Products.SubID = :productsubid;";

            static const auto s_queryProductBySourceGuidSubid = MakeSqlQuery(QUERY_PRODUCT_BY_SOURCEGUID_SUBID, QUERY_PRODUCT_BY_SOURCEGUID_SUBID_STATEMENT, LOG_NAME,
                    SqlParam<AZ::Uuid>(":sourceguid"),
                    SqlParam<AZ::u32>(":productsubid"));

            static const char* QUERY_PRODUCT_BY_SOURCEID_PLATFORM = "AzToolsFramework::AssetDatabase::QueryProductBySourceIDPlatform";
            static const char* QUERY_PRODUCT_BY_SOURCEID_PLATFORM_STATEMENT =
                "SELECT Products.*, Jobs.* FROM Jobs INNER JOIN Products "
                "ON Jobs.JobID = Products.JobPK WHERE "
                "Jobs.SourcePK = :sourceid AND "
                "Platform = :platform;";

            static const auto s_queryProductBySourceidPlatform = MakeSqlQuery(QUERY_PRODUCT_BY_SOURCEID_PLATFORM, QUERY_PRODUCT_BY_SOURCEID_PLATFORM_STATEMENT, LOG_NAME,
                    SqlParam<AZ::s64>(":sourceid"),
                    SqlParam<const char*>(":platform"));

            static const char* QUERY_PRODUCT_BY_PRODUCTNAME = "AzToolsFramework::AssetDatabase::QueryProductByProductName";
            static const char* QUERY_PRODUCT_BY_PRODUCTNAME_STATEMENT =
                "SELECT Products.*, Jobs.* FROM Jobs INNER JOIN Products "
                "ON Jobs.JobID = Products.JobPK WHERE "
                "Products.ProductName = :productname;";

            static const auto s_queryProductByProductname = MakeSqlQuery(QUERY_PRODUCT_BY_PRODUCTNAME, QUERY_PRODUCT_BY_PRODUCTNAME_STATEMENT, LOG_NAME,
                    SqlParam<const char*>(":productname"));

            static const char* QUERY_PRODUCT_BY_PRODUCTNAME_PLATFORM = "AzToolsFramework::AssetDatabase::QueryProductByProductNamePlatform";
            static const char* QUERY_PRODUCT_BY_PRODUCTNAME_PLATFORM_STATEMENT =
                "SELECT Products.*, Jobs.* FROM Jobs INNER JOIN Products "
                "ON Jobs.JobID = Products.JobPK WHERE "
                "Jobs.Platform = :platform AND "
                "Products.ProductName = :productname;";

            static const auto s_queryProductByProductnamePlatform = MakeSqlQuery(QUERY_PRODUCT_BY_PRODUCTNAME_PLATFORM, QUERY_PRODUCT_BY_PRODUCTNAME_PLATFORM_STATEMENT, LOG_NAME,
                    SqlParam<const char*>(":productname"),
                    SqlParam<const char*>(":platform"));

            static const char* QUERY_PRODUCT_LIKE_PRODUCTNAME = "AzToolsFramework::AssetDatabase::QueryProductLikeProductName";
            static const char* QUERY_PRODUCT_LIKE_PRODUCTNAME_STATEMENT =
                "SELECT Products.*, Jobs.* FROM Jobs INNER JOIN Products "
                "ON Jobs.JobID = Products.JobPK WHERE "
                "Products.ProductName LIKE :productname ESCAPE '|';"; // use the pipe to escape since its NOT a valid file path or operator

            static const auto s_queryProductLikeProductname = MakeSqlQuery(QUERY_PRODUCT_LIKE_PRODUCTNAME, QUERY_PRODUCT_LIKE_PRODUCTNAME_STATEMENT, LOG_NAME,
                    SqlParam<const char*>(":productname"));

            static const char* QUERY_PRODUCT_LIKE_PRODUCTNAME_PLATFORM = "AzToolsFramework::AssetDatabase::QueryProductLikeProductNamePlatform";
            static const char* QUERY_PRODUCT_LIKE_PRODUCTNAME_PLATFORM_STATEMENT =
                "SELECT Products.*, Jobs.* FROM Jobs INNER JOIN Products "
                "ON Jobs.JobID = Products.JobPK WHERE "
                "Jobs.Platform = :platform AND "
                "Products.ProductName LIKE :productname ESCAPE '|';"; // use the pipe to escape since its NOT a valid file path or operator

            static const auto s_queryProductLikeProductnamePlatform = MakeSqlQuery(QUERY_PRODUCT_LIKE_PRODUCTNAME_PLATFORM, QUERY_PRODUCT_LIKE_PRODUCTNAME_PLATFORM_STATEMENT, LOG_NAME,
                    SqlParam<const char*>(":productname"),
                    SqlParam<const char*>(":platform"));

            static const char* QUERY_PRODUCT_BY_SOURCENAME = "AzToolsFramework::AssetDatabase::QueryProductBySourceName";
            static const char* QUERY_PRODUCT_BY_SOURCENAME_STATEMENT =
                "SELECT Products.*, Jobs.* FROM Sources LEFT JOIN Jobs "
                "ON Sources.SourceID = Jobs.SourcePK INNER JOIN Products "
                "ON Jobs.JobID = Products.JobPK WHERE "
                "Sources.SourceName = :sourcename;";

            static const auto s_queryProductBySourcename = MakeSqlQuery(QUERY_PRODUCT_BY_SOURCENAME, QUERY_PRODUCT_BY_SOURCENAME_STATEMENT, LOG_NAME,
                    SqlParam<const char*>(":sourcename"));

            static const char* QUERY_PRODUCT_BY_SOURCENAME_SCANFOLDERID = "AzToolsFramework::AssetDatabase::QueryProductBySourceNameScanFolderID";
            static const char* QUERY_PRODUCT_BY_SOURCENAME_SCANFOLDERID_STATEMENT =
                "SELECT Products.*, Jobs.* FROM Sources LEFT JOIN Jobs "
                                                                       "ON Sources.SourceID = Jobs.SourcePK INNER JOIN Products "
                                                                       "ON Jobs.JobID = Products.JobPK WHERE "
                                                                       "Sources.SourceName = :sourcename AND Sources.ScanFolderPK = :scanfolderid;";

            static const auto s_queryProductBySourcenameScanFolderId = MakeSqlQuery(
                QUERY_PRODUCT_BY_SOURCENAME_SCANFOLDERID,
                QUERY_PRODUCT_BY_SOURCENAME_SCANFOLDERID_STATEMENT,
                LOG_NAME,
                SqlParam<const char*>(":sourcename"),
                SqlParam<AZ::s64>(":scanfolderid"));

            //add sql statement for querying everything by source name
            static const char* QUERY_PRODUCT_BY_SOURCENAME_PLATFORM = "AzToolsFramework::AssetDatabase::QueryProductBySourceNamePlatform";
            static const char* QUERY_PRODUCT_BY_SOURCENAME_PLATFORM_STATEMENT =
                "SELECT Products.*, Jobs.* FROM Sources LEFT JOIN Jobs "
                "ON Sources.SourceID = Jobs.SourcePK INNER JOIN Products "
                "ON Jobs.JobID = Products.JobPK WHERE "
                "Jobs.Platform = :platform AND "
                "Sources.SourceName = :sourcename;";

            static const auto s_queryProductBySourcenamePlatform = MakeSqlQuery(QUERY_PRODUCT_BY_SOURCENAME_PLATFORM, QUERY_PRODUCT_BY_SOURCENAME_PLATFORM_STATEMENT, LOG_NAME,
                    SqlParam<const char*>(":sourcename"),
                    SqlParam<const char*>(":platform"));

            static const char* QUERY_PRODUCT_BY_SOURCENAME_SCANFOLDERID_PLATFORM = "AzToolsFramework::AssetDatabase::QueryProductBySourceNameScanFolderIDPlatform";
            static const char* QUERY_PRODUCT_BY_SOURCENAME_SCANFOLDERID_PLATFORM_STATEMENT =
                "SELECT Products.*, Jobs.* FROM Sources LEFT JOIN Jobs "
                "ON Sources.SourceID = Jobs.SourcePK INNER JOIN Products "
                "ON Jobs.JobID = Products.JobPK WHERE "
                "Jobs.Platform = :platform AND "
                "Sources.SourceName = :sourcename AND Sources.ScanFolderPK = :scanfolderid;";

            static const auto s_queryProductBySourcenameScanFolderIdPlatform = MakeSqlQuery(
                QUERY_PRODUCT_BY_SOURCENAME_SCANFOLDERID_PLATFORM,
                QUERY_PRODUCT_BY_SOURCENAME_SCANFOLDERID_PLATFORM_STATEMENT,
                LOG_NAME,
                SqlParam<const char*>(":sourcename"),
                SqlParam<const char*>(":platform"),
                SqlParam<AZ::s64>(":scanfolderid"));

            //add sql statement for querying everything by source name
            static const char* QUERY_PRODUCT_LIKE_SOURCENAME = "AzToolsFramework::AssetDatabase::QueryProductLikeSourceName";
            static const char* QUERY_PRODUCT_LIKE_SOURCENAME_STATEMENT =
                "SELECT Products.*, Jobs.* FROM Sources LEFT JOIN Jobs "
                "ON Sources.SourceID = Jobs.SourcePK INNER JOIN Products "
                "ON Jobs.JobID = Products.JobPK WHERE "
                "Sources.SourceName LIKE :sourcename ESCAPE '|';"; // use the pipe to escape since its NOT a valid file path or operator

            static const auto s_queryProductLikeSourcename = MakeSqlQuery(QUERY_PRODUCT_LIKE_SOURCENAME, QUERY_PRODUCT_LIKE_SOURCENAME_STATEMENT, LOG_NAME,
                    SqlParam<const char*>(":sourcename"));

            //add sql statement for querying everything by source name and platform
            static const char* QUERY_PRODUCT_LIKE_SOURCENAME_PLATFORM = "AzToolsFramework::AssetDatabase::QueryProductLikeSourceNamePlatform";
            static const char* QUERY_PRODUCT_LIKE_SOURCENAME_PLATFORM_STATEMENT =
                "SELECT Products.*, Jobs.* FROM Sources LEFT JOIN Jobs "
                "ON Sources.SourceID = Jobs.SourcePK INNER JOIN Products "
                "ON Jobs.JobID = Products.JobPK WHERE "
                "Jobs.Platform = :platform AND "
                "Sources.SourceName LIKE :sourcename ESCAPE '|';"; // use the pipe to escape since its NOT a valid file path or operator

            static const auto s_queryProductLikeSourcenamePlatform = MakeSqlQuery(QUERY_PRODUCT_LIKE_SOURCENAME_PLATFORM, QUERY_PRODUCT_LIKE_SOURCENAME_PLATFORM_STATEMENT, LOG_NAME,
                SqlParam<const char*>(":sourcename"),
                SqlParam<const char*>(":platform"));

            // JobPK and subid together uniquely identify a product.  Since JobPK is indexed, this is a fast query if you happen to know those details.
            static const char* QUERY_PRODUCT_BY_JOBID_SUBID = "AzToolsFramework::AssetDatabase::QueryProductByJobIDSubID";
            static const char* QUERY_PRODUCT_BY_JOBID_SUBID_STATEMENT =
                "SELECT * FROM Products "
                "WHERE JobPK = :jobpk "
                "AND SubID = :subid;";

            static const auto s_queryProductByJobIdSubId = MakeSqlQuery(QUERY_PRODUCT_BY_JOBID_SUBID, QUERY_PRODUCT_BY_JOBID_SUBID_STATEMENT, LOG_NAME,
                SqlParam<AZ::s64>(":jobpk"),
                SqlParam<AZ::u32>(":subid"));

            //add sql statement for querying everything by platform
            static const char* QUERY_COMBINED = "AzToolsFramework::AssetDatabase::QueryCombined";
            static const char* QUERY_COMBINED_STATEMENT =
                "SELECT * FROM ScanFolders INNER JOIN Sources "
                "ON ScanFolders.ScanFolderID = Sources.ScanFolderPK INNER JOIN Jobs "
                "ON Sources.SourceID = Jobs.SourcePK INNER JOIN Products "
                "ON Jobs.JobID = Products.JobPK;";

            static const auto s_queryCombined = MakeSqlQuery(QUERY_COMBINED, QUERY_COMBINED_STATEMENT, LOG_NAME);

            //add sql statement for querying everything by platform
            static const char* QUERY_COMBINED_BY_PLATFORM = "AzToolsFramework::AssetDatabase::QueryCombinedByPlatform";
            static const char* QUERY_COMBINED_BY_PLATFORM_STATEMENT =
                "SELECT * FROM Jobs LEFT JOIN Sources "
                "ON Sources.SourceID = Jobs.SourcePK INNER JOIN ScanFolders "
                "ON ScanFolders.ScanFolderID = Sources.ScanFolderPK INNER JOIN Products "
                "ON Jobs.JobID = Products.JobPK WHERE "
                "Jobs.Platform = :platform;";

            static const auto s_queryCombinedByPlatform = MakeSqlQuery(QUERY_COMBINED_BY_PLATFORM, QUERY_COMBINED_BY_PLATFORM_STATEMENT, LOG_NAME,
                    SqlParam<const char*>(":platform"));

            static const char* QUERY_COMBINED_BY_SOURCEID = "AzToolsFramework::AssetDatabase::QueryCombinedBySourceID";
            static const char* QUERY_COMBINED_BY_SOURCEID_STATEMENT =
                "SELECT * FROM Sources LEFT JOIN Jobs "
                "ON Sources.SourceID = Jobs.SourcePK INNER JOIN ScanFolders "
                "ON ScanFolders.ScanFolderID = Sources.ScanFolderPK INNER JOIN Products "
                "ON Jobs.JobID = Products.JobPK WHERE "
                "Sources.SourceID = :sourceid;";

            static const auto s_queryCombinedBySourceid = MakeSqlQuery(QUERY_COMBINED_BY_SOURCEID, QUERY_COMBINED_BY_SOURCEID_STATEMENT, LOG_NAME,
                    SqlParam<AZ::s64>(":sourceid"));

            static const char* QUERY_COMBINED_BY_SOURCEID_PLATFORM = "AzToolsFramework::AssetDatabase::QueryCombinedBySourceIDPlatform";
            static const char* QUERY_COMBINED_BY_SOURCEID_PLATFORM_STATEMENT =
                "SELECT * FROM Sources LEFT JOIN Jobs "
                "ON Sources.SourceID = Jobs.SourcePK INNER JOIN ScanFolders "
                "ON ScanFolders.ScanFolderID = Sources.ScanFolderPK INNER JOIN Products "
                "ON Jobs.JobID = Products.JobPK WHERE "
                "Sources.SourceID = :sourceid AND "
                "Jobs.Platform = :platform;";

            static const auto s_queryCombinedBySourceidPlatform = MakeSqlQuery(QUERY_COMBINED_BY_SOURCEID_PLATFORM, QUERY_COMBINED_BY_SOURCEID_PLATFORM_STATEMENT, LOG_NAME,
                    SqlParam<AZ::s64>(":sourceid"),
                    SqlParam<const char*>(":platform"));

            static const char* QUERY_COMBINED_BY_JOBID = "AzToolsFramework::AssetDatabase::QueryCombinedByJobID";
            static const char* QUERY_COMBINED_BY_JOBID_STATEMENT =
                "SELECT * FROM Jobs LEFT JOIN Sources "
                "ON Sources.SourceID = Jobs.SourcePK INNER JOIN ScanFolders "
                "ON ScanFolders.ScanFolderID = Sources.ScanFolderPK INNER JOIN Products "
                "ON Jobs.JobID = Products.JobPK WHERE "
                "Jobs.JobID = :jobid;";

            static const auto s_queryCombinedByJobid = MakeSqlQuery(QUERY_COMBINED_BY_JOBID, QUERY_COMBINED_BY_JOBID_STATEMENT, LOG_NAME,
                    SqlParam<AZ::s64>(":jobid"));

            static const char* QUERY_COMBINED_BY_JOBID_PLATFORM = "AzToolsFramework::AssetDatabase::QueryCombinedByJobIDPlatform";
            static const char* QUERY_COMBINED_BY_JOBID_PLATFORM_STATEMENT =
                "SELECT * FROM Jobs LEFT JOIN Sources "
                "ON Sources.SourceID = Jobs.SourcePK INNER JOIN ScanFolders "
                "ON ScanFolders.ScanFolderID = Sources.ScanFolderPK INNER JOIN Products "
                "ON Jobs.JobID = Products.JobPK WHERE "
                "Jobs.JobID = :jobid AND "
                "Jobs.Platform = :platform;";

            static const auto s_queryCombinedByJobidPlatform = MakeSqlQuery(QUERY_COMBINED_BY_JOBID_PLATFORM, QUERY_COMBINED_BY_JOBID_PLATFORM_STATEMENT, LOG_NAME,
                    SqlParam<AZ::s64>(":jobid"),
                    SqlParam<const char*>(":platform"));

            static const char* QUERY_COMBINED_BY_PRODUCTID = "AzToolsFramework::AssetDatabase::QueryCombinedByProcductID";
            static const char* QUERY_COMBINED_BY_PRODUCTID_STATEMENT =
                "SELECT * FROM Products LEFT JOIN Jobs "
                "ON Jobs.JobID = Products.JobPK INNER JOIN Sources "
                "ON Sources.SourceID = Jobs.SourcePK INNER JOIN ScanFolders "
                "ON ScanFolders.ScanFolderID = Sources.ScanFolderPK WHERE "
                "Products.ProductID = :productid;";

            static const auto s_queryCombinedByProductid = MakeSqlQuery(QUERY_COMBINED_BY_PRODUCTID, QUERY_COMBINED_BY_PRODUCTID_STATEMENT, LOG_NAME,
                    SqlParam<AZ::s64>(":productid"));

            static const char* QUERY_COMBINED_BY_PRODUCTID_PLATFORM = "AzToolsFramework::AssetDatabase::QueryCombinedByProductIDPlatform";
            static const char* QUERY_COMBINED_BY_PRODUCTID_PLATFORM_STATEMENT =
                "SELECT * FROM Products LEFT JOIN Jobs "
                "ON Jobs.JobID = Products.JobPK INNER JOIN Sources "
                "ON Sources.SourceID = Jobs.SourcePK INNER JOIN ScanFolders "
                "ON ScanFolders.ScanFolderID = Sources.ScanFolderPK WHERE "
                "Products.ProductID = :productid AND "
                "Jobs.Platform = :platform;";

            static const auto s_queryCombinedByProductidPlatform = MakeSqlQuery(QUERY_COMBINED_BY_PRODUCTID_PLATFORM, QUERY_COMBINED_BY_PRODUCTID_PLATFORM_STATEMENT, LOG_NAME,
                    SqlParam<AZ::s64>(":productid"),
                    SqlParam<const char*>(":platform"));


            //add sql statement for querying everything by source guid and product subid
            static const char* QUERY_COMBINED_BY_SOURCEGUID_PRODUCTSUBID = "AzToolsFramework::AssetDatabase::QueryCombinedBySourceGuidProductSubID";
            static const char* QUERY_COMBINED_BY_SOURCEGUID_PRODUCTSUBID_STATEMENT =
                "SELECT * FROM Products LEFT JOIN Jobs "
                "ON Jobs.JobID = Products.JobPK INNER JOIN Sources "
                "ON Sources.SourceID = Jobs.SourcePK INNER JOIN ScanFolders "
                "ON ScanFolders.ScanFolderID = Sources.ScanFolderPK WHERE "
                "Products.SubID = :productsubid AND "
                "(Sources.SourceGuid = :sourceguid OR "
                "Products.LegacyGuid = :sourceguid);";

            static const auto s_queryCombinedBySourceguidProductsubid = MakeSqlQuery(QUERY_COMBINED_BY_SOURCEGUID_PRODUCTSUBID, QUERY_COMBINED_BY_SOURCEGUID_PRODUCTSUBID_STATEMENT, LOG_NAME,
                    SqlParam<AZ::u32>(":productsubid"),
                    SqlParam<AZ::Uuid>(":sourceguid"));

            //add sql statement for querying everything by source guid and product subid and platform
            static const char* QUERY_COMBINED_BY_SOURCEGUID_PRODUCTSUBID_PLATFORM = "AzToolsFramework::AssetDatabase::QueryCombinedBySourceGuidProductSubIDPlatform";
            static const char* QUERY_COMBINED_BY_SOURCEGUID_PRODUCTSUBID_PLATFORM_STATEMENT =
                "SELECT * FROM Products LEFT JOIN Jobs "
                "ON Jobs.JobID = Products.JobPK INNER JOIN Sources "
                "ON Sources.SourceID = Jobs.SourcePK INNER JOIN ScanFolders "
                "ON ScanFolders.ScanFolderID = Sources.ScanFolderPK  WHERE "
                "Products.SubID = :productsubid AND "
                "(Sources.SourceGuid = :sourceguid OR "
                "Products.LegacyGuid = :sourceguid) AND "
                "Jobs.Platform = :platform;";

            static const auto s_queryCombinedBySourceguidProductsubidPlatform = MakeSqlQuery(QUERY_COMBINED_BY_SOURCEGUID_PRODUCTSUBID_PLATFORM, QUERY_COMBINED_BY_SOURCEGUID_PRODUCTSUBID_PLATFORM_STATEMENT, LOG_NAME,
                    SqlParam<AZ::u32>(":productsubid"),
                    SqlParam<AZ::Uuid>(":sourceguid"),
                    SqlParam<const char*>(":platform"));

            //add sql statement for querying everything by source name
            static const char* QUERY_COMBINED_BY_SOURCENAME = "AzToolsFramework::AssetDatabase::QueryCombinedBySourceName";
            static const char* QUERY_COMBINED_BY_SOURCENAME_STATEMENT =
                "SELECT * FROM Sources LEFT JOIN Jobs "
                "ON Sources.SourceID = Jobs.SourcePK INNER JOIN ScanFolders "
                "ON ScanFolders.ScanFolderID = Sources.ScanFolderPK INNER JOIN Products "
                "ON Jobs.JobID = Products.JobPK WHERE "
                "Sources.SourceName = :sourcename;";

            static const auto s_queryCombinedBySourcename = MakeSqlQuery(QUERY_COMBINED_BY_SOURCENAME, QUERY_COMBINED_BY_SOURCENAME_STATEMENT, LOG_NAME,
                    SqlParam<const char*>(":sourcename"));

            //add sql statement for querying everything by source name
            static const char* QUERY_COMBINED_BY_SOURCENAME_PLATFORM = "AzToolsFramework::AssetDatabase::QueryCombinedBySourceNamePlatform";
            static const char* QUERY_COMBINED_BY_SOURCENAME_PLATFORM_STATEMENT =
                "SELECT * FROM Sources LEFT JOIN Jobs "
                "ON Sources.SourceID = Jobs.SourcePK INNER JOIN ScanFolders "
                "ON ScanFolders.ScanFolderID = Sources.ScanFolderPK INNER JOIN Products "
                "ON Jobs.JobID = Products.JobPK WHERE "
                "Jobs.Platform = :platform AND "
                "Sources.SourceName = :sourcename;";

            static const auto s_queryCombinedBySourcenamePlatform = MakeSqlQuery(QUERY_COMBINED_BY_SOURCENAME_PLATFORM, QUERY_COMBINED_BY_SOURCENAME_PLATFORM_STATEMENT, LOG_NAME,
                    SqlParam<const char*>(":sourcename"),
                    SqlParam<const char*>(":platform"));

            //add sql statement for querying everything by source name
            static const char* QUERY_COMBINED_LIKE_SOURCENAME = "AzToolsFramework::AssetDatabase::QueryCombinedLikeSourceName";
            static const char* QUERY_COMBINED_LIKE_SOURCENAME_STATEMENT =
                "SELECT * FROM Sources LEFT JOIN Jobs "
                "ON Sources.SourceID = Jobs.SourcePK INNER JOIN Products "
                "ON Jobs.JobID = Products.JobPK INNER JOIN ScanFolders "
                "ON ScanFolders.ScanFolderID = Sources.ScanFolderPK WHERE "
                "Sources.SourceName LIKE :sourcename ESCAPE '|';"; // use the pipe to escape since its NOT a valid file path or operator

            static const auto s_queryCombinedLikeSourcename = MakeSqlQuery(QUERY_COMBINED_LIKE_SOURCENAME, QUERY_COMBINED_LIKE_SOURCENAME_STATEMENT, LOG_NAME,
                    SqlParam<const char*>(":sourcename"));

            //add sql statement for querying everything by source name and platform
            static const char* QUERY_COMBINED_LIKE_SOURCENAME_PLATFORM = "AzToolsFramework::AssetDatabase::QueryCombinedLikeSourceNamePlatform";
            static const char* QUERY_COMBINED_LIKE_SOURCENAME_PLATFORM_STATEMENT =
                "SELECT * FROM Sources LEFT JOIN Jobs "
                "ON Sources.SourceID = Jobs.SourcePK INNER JOIN Products "
                "ON Jobs.JobID = Products.JobPK WHERE "
                "Jobs.Platform = :platform AND "
                "Sources.SourceName LIKE :sourcename ESCAPE '|';"; // use the pipe to escape since its NOT a valid file path or operator

            static const auto s_queryCombinedLikeSourcenamePlatform = MakeSqlQuery(QUERY_COMBINED_LIKE_SOURCENAME_PLATFORM, QUERY_COMBINED_LIKE_SOURCENAME_PLATFORM_STATEMENT, LOG_NAME,
                    SqlParam<const char*>(":sourcename"),
                    SqlParam<const char*>(":platform"));

            //add sql statement for querying everything by product name
            static const char* QUERY_COMBINED_BY_PRODUCTNAME = "AzToolsFramework::AssetDatabase::QueryCombinedByProductName";
            static const char* QUERY_COMBINED_BY_PRODUCTNAME_STATEMENT =
                "SELECT * "
                "FROM Products "
                "LEFT OUTER JOIN Jobs ON Jobs.JobID = Products.JobPK "
                "LEFT OUTER JOIN Sources ON Sources.SourceID = Jobs.SourcePK "
                "LEFT OUTER JOIN ScanFolders ON ScanFolders.ScanFolderID = Sources.SourceID "
                "WHERE "
                "Products.ProductName = :productname;";

            static const auto s_queryCombinedByProductname = MakeSqlQuery(QUERY_COMBINED_BY_PRODUCTNAME, QUERY_COMBINED_BY_PRODUCTNAME_STATEMENT, LOG_NAME,
                    SqlParam<const char*>(":productname"));

            //add sql statement for querying everything by product name and platform
            static const char* QUERY_COMBINED_BY_PRODUCTNAME_PLATFORM = "AzToolsFramework::AssetDatabase::QueryCombinedByProductNamePlatorm";
            static const char* QUERY_COMBINED_BY_PRODUCTNAME_PLATFORM_STATEMENT =
                "SELECT * "
                "FROM Products "
                "LEFT OUTER JOIN Jobs ON Jobs.JobID = Products.JobPK "
                "LEFT OUTER JOIN Sources ON Sources.SourceID = Jobs.SourcePK "
                "LEFT OUTER JOIN ScanFolders ON ScanFolders.ScanFolderID = Sources.SourceID "
                "WHERE "
                "Products.ProductName = :productname AND"
                "Jobs.Platform = :platform;";

            static const auto s_queryCombinedByProductnamePlatform = MakeSqlQuery(QUERY_COMBINED_BY_PRODUCTNAME_PLATFORM, QUERY_COMBINED_BY_PRODUCTNAME_PLATFORM_STATEMENT, LOG_NAME,
                    SqlParam<const char*>(":productname"),
                    SqlParam<const char*>(":platform"));

            //add sql statement for querying everything by product name
            static const char* QUERY_COMBINED_LIKE_PRODUCTNAME = "AzToolsFramework::AssetDatabase::QueryCombinedLikeProductName";
            static const char* QUERY_COMBINED_LIKE_PRODUCTNAME_STATEMENT =
                "SELECT * FROM Products LEFT JOIN Jobs "
                "ON Jobs.JobID = Products.JobPK INNER JOIN Sources "
                "ON Sources.SourceID = Jobs.SourcePK INNER JOIN ScanFolders "
                "ON ScanFolders.ScanFolderID = Sources.ScanFolderPK WHERE "
                "Products.ProductName LIKE :productname ESCAPE '|';"; // use the pipe to escape since its NOT a valid file path or operator

            static const auto s_queryCombinedLikeProductname = MakeSqlQuery(QUERY_COMBINED_LIKE_PRODUCTNAME, QUERY_COMBINED_LIKE_PRODUCTNAME_STATEMENT, LOG_NAME,
                    SqlParam<const char*>(":productname"));

            //add sql statement for querying everything by product name and platform
            static const char* QUERY_COMBINED_LIKE_PRODUCTNAME_PLATFORM = "AzToolsFramework::AssetDatabase::QueryCombinedLikeProductNamePlatorm";
            static const char* QUERY_COMBINED_LIKE_PRODUCTNAME_PLATFORM_STATEMENT =
                "SELECT * FROM Products LEFT JOIN Jobs "
                "ON Jobs.JobID = Products.JobPK INNER JOIN Sources "
                "ON Sources.SourceID = Jobs.SourcePK INNER JOIN ScanFolders "
                "ON ScanFolders.ScanFolderID = Sources.ScanFolderPK WHERE "
                "Jobs.Platform = :platform AND "
                "Products.ProductName LIKE :productname ESCAPE '|';"; // use the pipe to escape since its NOT a valid file path or operator

            static const auto s_queryCombinedLikeProductnamePlatform = MakeSqlQuery(QUERY_COMBINED_LIKE_PRODUCTNAME_PLATFORM, QUERY_COMBINED_LIKE_PRODUCTNAME_PLATFORM_STATEMENT, LOG_NAME,
                    SqlParam<const char*>(":productname"),
                    SqlParam<const char*>(":platform"));

            static const char* QUERY_SOURCEDEPENDENCY = "AzToolsFramework::AssetDatabase::QuerySourceDependency";
            static const char* QUERY_SOURCEDEPENDENCY_STATEMENT = "SELECT * FROM SourceDependency";

            static const auto s_querySourceDependency = MakeSqlQuery(QUERY_SOURCEDEPENDENCY, QUERY_SOURCEDEPENDENCY_STATEMENT, LOG_NAME);

            static const char* QUERY_SOURCEDEPENDENCY_BY_SOURCEDEPENDENCYID = "AzToolsFramework::AssetDatabase::QuerySourceDependencyBySourceDependencyID";
            static const char* QUERY_SOURCEDEPENDENCY_BY_SOURCEDEPENDENCYID_STATEMENT =
                "SELECT * FROM SourceDependency WHERE "
                "SourceDependencyID = :sourceDependencyid;";

            static const auto s_querySourcedependencyBySourcedependencyid = MakeSqlQuery(QUERY_SOURCEDEPENDENCY_BY_SOURCEDEPENDENCYID, QUERY_SOURCEDEPENDENCY_BY_SOURCEDEPENDENCYID_STATEMENT, LOG_NAME,
                    SqlParam<AZ::s64>(":sourceDependencyid"));

            static const char* QUERY_SOURCEDEPENDENCY_BY_DEPENDSONSOURCE = "AzToolsFramework::AssetDatabase::QuerySourceDependencyByDependsOnSource";
            static const char* QUERY_SOURCEDEPENDENCY_BY_DEPENDSONSOURCE_STATEMENT =
                "SELECT * from SourceDependency WHERE "
                "DependsOnSource IN (:relativepath, :absolutepath, :uuid) AND "
                "TypeOfDependency & :typeOfDependency;";

            static const auto s_querySourceDependencyByDependsOnSource = MakeSqlQuery(QUERY_SOURCEDEPENDENCY_BY_DEPENDSONSOURCE, QUERY_SOURCEDEPENDENCY_BY_DEPENDSONSOURCE_STATEMENT, LOG_NAME,
                SqlParam<const char*>(":uuid"), // DependsOnSource is a text field, so UUIDs are stored as text with no dashes or brackets
                SqlParam<const char*>(":relativepath"),
                SqlParam<const char*>(":absolutepath"),
                SqlParam<AZ::u32>(":typeOfDependency"));

            // Query handles finding files which depend on "this" by: relative path (w or w/o wildcard), absolute path (w or w/o wildcard), or UUID
            static const char* QUERY_SOURCEDEPENDENCY_BY_DEPENDSONSOURCE_WILDCARD = "AzToolsFramework::AssetDatabase::QuerySourceDependencyByDependsOnSourceWildcard";
            static const char* QUERY_SOURCEDEPENDENCY_BY_DEPENDSONSOURCE_WILDCARD_STATEMENT =
                "SELECT *, CASE WHEN :relativepath LIKE DependsOnSource THEN 0 ELSE 1 END as AbsolutePath from SourceDependency WHERE "
                "(TypeOfDependency & :typeOfDependency AND "
                "DependsOnSource IN (:relativepath, :absolutepath, :uuid)) OR "
                "(TypeOfDependency = :wildCardDependency AND "
                "(:relativepath LIKE DependsOnSource OR :absolutepath LIKE DependsOnSource));";

            static const auto s_querySourceDependencyByDependsOnSourceWildcard = MakeSqlQuery(QUERY_SOURCEDEPENDENCY_BY_DEPENDSONSOURCE_WILDCARD, QUERY_SOURCEDEPENDENCY_BY_DEPENDSONSOURCE_WILDCARD_STATEMENT, LOG_NAME,
                SqlParam<const char*>(":uuid"), // DependsOnSource is a text field, so UUIDs are stored as text with no dashes or brackets
                SqlParam<const char*>(":relativepath"),
                SqlParam<const char*>(":absolutepath"),
                SqlParam<AZ::u32>(":typeOfDependency"),
                SqlParam<AZ::u32>(":wildCardDependency"));

            static const char* QUERY_PRODUCTDEPENDENCIES_THAT_DEPEND_ON_PRODUCT_BY_SOURCEID = "AzToolsFramework::AssetDatabase::QueryProductDependenciesThatDependOnProductBySourceId";
            static const char* QUERY_PRODUCTDEPENDENCIES_THAT_DEPEND_ON_PRODUCT_BY_SOURCEID_STATEMENT =
                "select ProductDependencies.* from Sources A "
                "inner join ProductDependencies on A.SourceGuid = DependencySourceGuid "
                "inner join products on productpk = ProductID "
                "inner join jobs on jobid = JobPK "
                "inner join sources B on B.SourceID = sourcepk "
                "where A.sourceid = :sourceid;";

            static const auto s_queryProductDependenciesThatDependOnProductBySourceId = MakeSqlQuery(QUERY_PRODUCTDEPENDENCIES_THAT_DEPEND_ON_PRODUCT_BY_SOURCEID, QUERY_PRODUCTDEPENDENCIES_THAT_DEPEND_ON_PRODUCT_BY_SOURCEID_STATEMENT, LOG_NAME,
                SqlParam<AZ::s64>(":sourceid"));

            static const char* QUERY_DEPENDSONSOURCE_BY_SOURCE = "AzToolsFramework::AssetDatabase::QueryDependsOnSourceBySource";
            static const char* QUERY_DEPENDSONSOURCE_BY_SOURCE_STATEMENT =
                "SELECT * from SourceDependency WHERE "
                "SourceGuid = :source AND "
                "TypeOfDependency & :typeOfDependency;";
            static const auto s_queryDependsOnSourceBySource = MakeSqlQuery(QUERY_DEPENDSONSOURCE_BY_SOURCE, QUERY_DEPENDSONSOURCE_BY_SOURCE_STATEMENT, LOG_NAME,
                SqlParam<AZ::Uuid>(":source"),
                SqlParam<AZ::u32>(":typeOfDependency"));

            static const char* QUERY_PRODUCTDEPENDENCY_BY_PRODUCTDEPENDENCYID = "AzToolsFramework::AssetDatabase::QueryProductDependencyByProductDependencyID";
            static const char* QUERY_PRODUCTDEPENDENCY_BY_PRODUCTDEPENDENCYID_STATEMENT =
                "SELECT * FROM ProductDependencies WHERE "
                "ProductDependencyID = :productdependencyid;";

            static const auto s_queryProductdependencyByProductdependencyid = MakeSqlQuery(QUERY_PRODUCTDEPENDENCY_BY_PRODUCTDEPENDENCYID, QUERY_PRODUCTDEPENDENCY_BY_PRODUCTDEPENDENCYID_STATEMENT, LOG_NAME,
                    SqlParam<AZ::s64>(":productdependencyid"));

            static const char* QUERY_PRODUCTDEPENDENCY_BY_PRODUCTID = "AzToolsFramework::AssetDatabase::QueryProductDependencyByProductID";
            static const char* QUERY_PRODUCTDEPENDENCY_BY_PRODUCTID_STATEMENT =
                "SELECT * FROM ProductDependencies WHERE "
                "ProductPK = :productid;";

            static const auto s_queryProductdependencyByProductid = MakeSqlQuery(QUERY_PRODUCTDEPENDENCY_BY_PRODUCTID, QUERY_PRODUCTDEPENDENCY_BY_PRODUCTID_STATEMENT, LOG_NAME,
                    SqlParam<AZ::s64>(":productid"));

            static const char* QUERY_PRODUCTDEPENDENCY_BY_SOURCEGUID_SUBID = "AzToolsFramework::AssetDatabase::QueryProductDependencyBySourceGuidSubId";
            static const char* QUERY_PRODUCTDEPENDENCY_BY_SOURCEGUID_SUBID_STATEMENT =
                "select ProductDependencies.* from sources "
                "inner join jobs on jobs.SourcePK = sources.SourceID and jobs.Platform = :platform "
                "inner join products on products.JobPK = jobs.JobID and SubId = :subid "
                "inner join ProductDependencies on ProductDependencies.ProductPK = products.ProductID "
                "where sources.SourceGuid = :uuid;";

            static const auto s_queryProductdependencyBySourceGuidSubId = MakeSqlQuery(QUERY_PRODUCTDEPENDENCY_BY_SOURCEGUID_SUBID, QUERY_PRODUCTDEPENDENCY_BY_SOURCEGUID_SUBID_STATEMENT, LOG_NAME,
                SqlParam<AZ::Uuid>(":uuid"),
                SqlParam<AZ::u32>(":subid"),
                SqlParam<const char*>(":platform"));

            static const char* QUERY_MISSING_PRODUCT_DEPENDENCY_BY_PRODUCTID =
                "AzToolsFramework::AssetDatabase::QueryMissingProductDependencyByProductId";
            static const char* QUERY_MISSING_PRODUCT_DEPENDENCY_BY_PRODUCTID_STATEMENT =
                "SELECT * FROM MissingProductDependencies WHERE "
                "ProductPK = :productId;";
            static const auto s_queryMissingProductDependencyByProductId =
                MakeSqlQuery(QUERY_MISSING_PRODUCT_DEPENDENCY_BY_PRODUCTID, QUERY_MISSING_PRODUCT_DEPENDENCY_BY_PRODUCTID_STATEMENT, LOG_NAME,
                SqlParam<AZ::s64>(":productId"));

            static const char* QUERY_MISSING_PRODUCT_DEPENDENCY_BY_MISSING_PRODUCT_DEPENDENCYID =
                "AzToolsFramework::AssetDatabase::QueryMissingProductDependencyByMissingProductDependencyId";
            static const char* QUERY_MISSING_PRODUCT_DEPENDENCY_BY_MISSING_PRODUCT_DEPENDENCYID_STATEMENT =
                "SELECT * FROM MissingProductDependencies WHERE "
                "MissingProductDependencyId = :missingProductDependencyId;";
            static const auto s_queryMissingProductDependencyByMissingProductDependencyId =
                MakeSqlQuery(QUERY_MISSING_PRODUCT_DEPENDENCY_BY_MISSING_PRODUCT_DEPENDENCYID, QUERY_MISSING_PRODUCT_DEPENDENCY_BY_MISSING_PRODUCT_DEPENDENCYID_STATEMENT, LOG_NAME,
                SqlParam<AZ::s64>(":missingProductDependencyId"));

            static const char* DELETE_MISSING_PRODUCT_DEPENDENCY_BY_PRODUCTID =
                "AzToolsFramework::AssetDatabase::DeleteMissingProductDependencyByProductId";
            static const char* DELETE_MISSING_PRODUCT_DEPENDENCY_BY_PRODUCTID_STATEMENT =
                "DELETE FROM MissingProductDependencies WHERE "
                "ProductPK = :productId;";
            static const auto s_deleteMissingProductDependencyByProductId =
                MakeSqlQuery(DELETE_MISSING_PRODUCT_DEPENDENCY_BY_PRODUCTID, DELETE_MISSING_PRODUCT_DEPENDENCY_BY_PRODUCTID_STATEMENT, LOG_NAME,
                    SqlParam<AZ::s64>(":productId"));

            static const char* QUERY_DIRECT_PRODUCTDEPENDENCIES = "AzToolsFramework::AssetDatabase::QueryDirectProductDependencies";
            static const char* QUERY_DIRECT_PRODUCTDEPENDENCIES_STATEMENT =
                "SELECT * FROM Products "
                "LEFT OUTER JOIN Jobs ON Jobs.JobID = Products.JobPK "
                "LEFT OUTER JOIN Sources ON Sources.SourceID = Jobs.SourcePK "
                "LEFT OUTER JOIN ProductDependencies "
                "  ON Sources.SourceGuid = ProductDependencies.DependencySourceGuid "
                "  AND Products.SubID = ProductDependencies.DependencySubID "
                "WHERE ProductDependencies.ProductPK = :productid;";

            static const auto s_queryDirectProductdependencies = MakeSqlQuery(QUERY_DIRECT_PRODUCTDEPENDENCIES, QUERY_DIRECT_PRODUCTDEPENDENCIES_STATEMENT, LOG_NAME,
                    SqlParam<AZ::s64>(":productid"));

            static const char* QUERY_DIRECT_REVERSE_PRODUCTDEPENDENCIES_BY_SOURCE_GUID_SUB_ID = "AzToolsFramework::AssetDatabase::QueryDirectReverseProductDependenciesBySourceGuidSubId";
            static const char* QUERY_DIRECT_REVERSE_PRODUCTDEPENDENCIES_BY_SOURCE_GUID_SUB_ID_STATEMENT =
                "SELECT * FROM Products "
                "LEFT OUTER JOIN ProductDependencies "
                "   ON ProductDependencies.ProductPK = Products.ProductID "
                "WHERE ProductDependencies.DependencySourceGuid = :dependencySourceGuid "
                "   AND ProductDependencies.DependencySubID = :dependencySubId;";

            static const auto s_queryDirectReverseProductdependenciesBySourceGuidSubId = MakeSqlQuery(QUERY_DIRECT_REVERSE_PRODUCTDEPENDENCIES_BY_SOURCE_GUID_SUB_ID,
                QUERY_DIRECT_REVERSE_PRODUCTDEPENDENCIES_BY_SOURCE_GUID_SUB_ID_STATEMENT, LOG_NAME, SqlParam<AZ::Uuid>(":dependencySourceGuid"), SqlParam<AZ::s32>(":dependencySubId"));

            static const char* QUERY_DIRECT_REVERSE_PRODUCTDEPENDENCIES_BY_SOURCE_GUID = "AzToolsFramework::AssetDatabase::QueryDirectReverseProductDependenciesBySourceGuid";
            static const char* QUERY_DIRECT_REVERSE_PRODUCTDEPENDENCIES_BY_SOURCE_GUID_STATEMENT =
                "SELECT * FROM ProductDependencies "
                "WHERE DependencySourceGuid = :dependencySourceGuid"
            ;

            static const auto s_queryDirectReverseProductDependenciesBySourceGuidAllPlatforms = MakeSqlQuery(QUERY_DIRECT_REVERSE_PRODUCTDEPENDENCIES_BY_SOURCE_GUID,
                QUERY_DIRECT_REVERSE_PRODUCTDEPENDENCIES_BY_SOURCE_GUID_STATEMENT, LOG_NAME,
                SqlParam<AZ::Uuid>(":dependencySourceGuid"));

            static const char* QUERY_ALL_PRODUCTDEPENDENCIES = "AzToolsFramework::AssetDatabase::QueryAllProductDependencies";
            static const char* QUERY_ALL_PRODUCTDEPENDENCIES_STATEMENT =
                "WITH RECURSIVE "
                "  allProductDeps AS (  "
                "    SELECT * FROM Products "
                "    WHERE ProductID = :productid "
                "    UNION "
                "    SELECT P.* FROM Products P, allProductDeps"
                "    LEFT OUTER JOIN Jobs ON Jobs.JobID = P.JobPK "
                "    LEFT OUTER JOIN Sources ON Sources.SourceID = Jobs.SourcePK "
                "    LEFT OUTER JOIN ProductDependencies"
                "    ON Sources.SourceGuid = ProductDependencies.DependencySourceGuid "
                "    AND P.SubID = ProductDependencies.DependencySubID "
                "    WHERE ProductDependencies.ProductPK = allProductDeps.ProductID "
                "    LIMIT -1 OFFSET 1 " // This will ignore the first Product selected which is not a dependency but the root of the tree
                "  ) "
                "SELECT * FROM allProductDeps;";

            static const auto s_queryAllProductdependencies = MakeSqlQuery(QUERY_ALL_PRODUCTDEPENDENCIES, QUERY_ALL_PRODUCTDEPENDENCIES_STATEMENT, LOG_NAME,
                    SqlParam<AZ::s64>(":productid"));

            static const char* GET_UNRESOLVED_PRODUCT_DEPENDENCIES = "AssetProcessor::GetUnresolvedProductDependencies";
            static const char* GET_UNRESOLVED_PRODUCT_DEPENDENCIES_STATEMENT =
                "SELECT * FROM ProductDependencies where UnresolvedPath != ''";
            static const auto s_queryUnresolvedProductDependencies = MakeSqlQuery(GET_UNRESOLVED_PRODUCT_DEPENDENCIES, GET_UNRESOLVED_PRODUCT_DEPENDENCIES_STATEMENT, LOG_NAME);

            static const char* GET_PRODUCT_DEPENDENCY_EXCLUSIONS = "AssetProcessor::GetProductDependencyExclusions";
            static const char* GET_PRODUCT_DEPENDENCY_EXCLUSIONS_STATEMENT =
                "SELECT * FROM ProductDependencies WHERE UnresolvedPath LIKE \":%\"";
            static const auto s_queryProductDependencyExclusions = MakeSqlQuery(GET_PRODUCT_DEPENDENCY_EXCLUSIONS, GET_PRODUCT_DEPENDENCY_EXCLUSIONS_STATEMENT, LOG_NAME);

            static const char* CREATE_UNRESOLVED_PRODUCT_DEPENDENCIES_TEMP_TABLE =
                "AssetProcessor::CreateUnresolvedProductDependenciesTempTable";
            static const char* CREATE_UNRESOLVED_PRODUCT_DEPENDENCIES_TEMP_TABLE_STATEMENT =
                "CREATE TEMPORARY TABLE QueryProductDependenciesUnresolvedAdvanced(search)";
            static const auto s_createUnresolvedProductDependenciesTempTable = MakeSqlQuery(
                CREATE_UNRESOLVED_PRODUCT_DEPENDENCIES_TEMP_TABLE, CREATE_UNRESOLVED_PRODUCT_DEPENDENCIES_TEMP_TABLE_STATEMENT, LOG_NAME);

            static const char* INSERT_PRODUCT_DEPENDENCY_TEMP_TABLE_VALUES = "AssetProcessor::InsertProductDependencyTempTableValues";
            static const char* INSERT_PRODUCT_DEPENDENCY_TEMP_TABLE_VALUES_STATEMENT =
                "INSERT INTO QueryProductDependenciesUnresolvedAdvanced VALUES (:filename)";
            static const auto s_queryInsertProductDependencyTempTableValues = MakeSqlQuery(
                INSERT_PRODUCT_DEPENDENCY_TEMP_TABLE_VALUES,
                INSERT_PRODUCT_DEPENDENCY_TEMP_TABLE_VALUES_STATEMENT,
                LOG_NAME,
                SqlParam<const char*>(":filename"));

            static const char* GET_UNRESOLVED_PRODUCT_DEPENDENCIES_USING_TEMP_TABLE =
                "AssetProcessor::GetUnresolvedProductDependenciesUsingTempTable";
            static const char* GET_UNRESOLVED_PRODUCT_DEPENDENCIES_USING_TEMP_TABLE_STATEMENT =
                "SELECT * FROM ProductDependencies INNER JOIN QueryProductDependenciesUnresolvedAdvanced "
                "ON (UnresolvedPath LIKE \"%*%\" AND search LIKE REPLACE(UnresolvedPath, \"*\", \"%\")) OR search = UnresolvedPath";
            static const auto s_queryGetUnresolvedProductDependenciesUsingTempTable = MakeSqlQuery(
                GET_UNRESOLVED_PRODUCT_DEPENDENCIES_USING_TEMP_TABLE,
                GET_UNRESOLVED_PRODUCT_DEPENDENCIES_USING_TEMP_TABLE_STATEMENT,
                LOG_NAME);

            // lookup by primary key
            static const char* QUERY_FILE_BY_FILEID = "AzToolsFramework::AssetDatabase::QueryFileByFileID";
            static const char* QUERY_FILE_BY_FILEID_STATEMENT =
                "SELECT * FROM Files WHERE "
                "FileID = :fileid;";

            static const auto s_queryFileByFileid = MakeSqlQuery(QUERY_FILE_BY_FILEID, QUERY_FILE_BY_FILEID_STATEMENT, LOG_NAME,
                    SqlParam<AZ::s64>(":fileid"));

            static const char* QUERY_FILES_BY_FILENAME_AND_SCANFOLDER = "AzToolsFramework::AssetDatabase::QueryFilesByFileNameAndScanFolderID";
            static const char* QUERY_FILES_BY_FILENAME_AND_SCANFOLDER_STATEMENT =
                "SELECT * FROM Files WHERE "
                    "ScanFolderPK = :scanfolderpk AND "
                    "FileName = :filename;";

            static const auto s_queryFilesByFileName = MakeSqlQuery(QUERY_FILES_BY_FILENAME_AND_SCANFOLDER, QUERY_FILES_BY_FILENAME_AND_SCANFOLDER_STATEMENT, LOG_NAME,
                    SqlParam<AZ::s64>(":scanfolderpk"),
                    SqlParam<const char*>(":filename")
                );

            static const char* QUERY_FILES_LIKE_FILENAME_SCANFOLDERID = "AzToolsFramework::AssetDatabase::QueryFilesLikeFileNameScanFolderID";
            static const char* QUERY_FILES_LIKE_FILENAME_SCANFOLDERID_STATEMENT =
                "SELECT * FROM Files WHERE "
                "FileName LIKE :filename ESCAPE '|' "
                "AND ScanFolderPK = :scanfolderid;";

            static const auto s_queryFilesLikeFileNameScanfolderid = MakeSqlQuery(
                QUERY_FILES_LIKE_FILENAME_SCANFOLDERID,
                QUERY_FILES_LIKE_FILENAME_SCANFOLDERID_STATEMENT,
                LOG_NAME,
                    SqlParam<const char*>(":filename"),
                SqlParam<AZ::s64>(":scanfolderid"));

            static const char* QUERY_FILES_BY_SCANFOLDERID = "AzToolsFramework::AssetDatabase::QueryFilesByScanFolderID";
            static const char* QUERY_FILES_BY_SCANFOLDERID_STATEMENT =
                "SELECT * FROM Files WHERE "
                "ScanFolderPK = :scanfolderid;";

            static const auto s_queryFilesByScanfolderid = MakeSqlQuery(QUERY_FILES_BY_SCANFOLDERID, QUERY_FILES_BY_SCANFOLDERID_STATEMENT, LOG_NAME,
                SqlParam<AZ::s64>(":scanfolderid"));

            static const char* QUERY_FILE_BY_FILENAME_SCANFOLDERID = "AzToolsFramework::AssetDatabase::QueryFileByFileNameScanFolderID";
            static const char* QUERY_FILE_BY_FILENAME_SCANFOLDERID_STATEMENT =
                "SELECT * FROM Files WHERE "
                "ScanFolderPK = :scanfolderid AND "
                "FileName = :filename;";

            static const auto s_queryFileByFileNameScanfolderid = MakeSqlQuery(QUERY_FILE_BY_FILENAME_SCANFOLDERID, QUERY_FILE_BY_FILENAME_SCANFOLDERID_STATEMENT, LOG_NAME,
                    SqlParam<AZ::s64>(":scanfolderid"),
                    SqlParam<const char*>(":filename"));

            static const char* QUERY_STAT_BY_STATNAME = "AzToolsFramework::AssetDatabase::QueryStatByStatName";
            static const char* QUERY_STAT_BY_STATNAME_STATEMENT = "SELECT * FROM Stats WHERE "
                                                                  "StatName = :statname;";

            static const auto s_queryStatByStatName = MakeSqlQuery(
                QUERY_STAT_BY_STATNAME,
                QUERY_STAT_BY_STATNAME_STATEMENT,
                LOG_NAME,
                SqlParam<const char*>(":statname"));

            static const char* QUERY_STAT_LIKE_STATNAME= "AzToolsFramework::AssetDatabase::QueryStatLikeStatName";
            static const char* QUERY_STAT_LIKE_STATNAME_STATEMENT = "SELECT * FROM Stats WHERE "
                                                                  "StatName LIKE :statname ;";
            static const auto s_queryStatLikeStatName= MakeSqlQuery(
                QUERY_STAT_LIKE_STATNAME, QUERY_STAT_LIKE_STATNAME_STATEMENT, LOG_NAME, SqlParam<const char*>(":statname"));

            void PopulateJobInfo(AzToolsFramework::AssetSystem::JobInfo& jobinfo, JobDatabaseEntry& jobDatabaseEntry)
            {
                jobinfo.m_platform = AZStd::move(jobDatabaseEntry.m_platform);
                jobinfo.m_builderGuid = jobDatabaseEntry.m_builderGuid;
                jobinfo.m_jobKey = AZStd::move(jobDatabaseEntry.m_jobKey);
                jobinfo.m_status = jobDatabaseEntry.m_status;
                jobinfo.m_jobRunKey = jobDatabaseEntry.m_jobRunKey;
                jobinfo.m_firstFailLogTime = jobDatabaseEntry.m_firstFailLogTime;
                jobinfo.m_firstFailLogFile = AZStd::move(jobDatabaseEntry.m_firstFailLogFile);
                jobinfo.m_lastFailLogTime = jobDatabaseEntry.m_lastFailLogTime;
                jobinfo.m_lastFailLogFile = AZStd::move(jobDatabaseEntry.m_lastFailLogFile);
                jobinfo.m_lastLogTime = jobDatabaseEntry.m_lastLogTime;
                jobinfo.m_lastLogFile = AZStd::move(jobDatabaseEntry.m_lastLogFile);
                jobinfo.m_jobID = jobDatabaseEntry.m_jobID;
                jobinfo.m_warningCount = jobDatabaseEntry.m_warningCount;
                jobinfo.m_errorCount = jobDatabaseEntry.m_errorCount;
            }

            bool GetDatabaseInfoResult(const char* callName, SQLite::Statement* statement, AssetDatabaseConnection::databaseInfoHandler handler);
            bool GetScanFolderResult(const char* callName, SQLite::Statement* statement, AssetDatabaseConnection::scanFolderHandler handler);
            bool GetSourceResult(const char* callName, SQLite::Statement* statement, AssetDatabaseConnection::sourceHandler handler);
            bool GetSourceAndScanfolderResult(const char* callName, SQLite::Statement* statement, AssetDatabaseConnection::combinedSourceScanFolderHandler handler);
            bool GetSourceDependencyResult(const char* callName, SQLite::Statement* statement, AssetDatabaseConnection::sourceFileDependencyHandler handler);
            bool GetWildcardSourceDependencyResult(const char* callName, SQLite::Statement* statement, AssetDatabaseConnection::sourceFileDependencyHandler handler);
            bool GetJobResultSimple(const char* name, SQLite::Statement* statement, AssetDatabaseConnection::jobHandler handler);
            bool GetJobResult(
                const char* callName,
                SQLite::Statement* statement,
                AssetDatabaseConnection::jobHandler handler,
                AZ::Uuid builderGuid = AZ::Uuid::CreateNull(),
                const char* jobKey = nullptr,
                AssetSystem::JobStatus status = AssetSystem::JobStatus::Any);
            bool GetProductResultSimple(const char* name, SQLite::Statement* statement, AssetDatabaseConnection::productHandler handler);
            bool GetProductResult(
                const char* callName,
                SQLite::Statement* statement,
                AssetDatabaseConnection::productHandler handler,
                AZ::Uuid builderGuid = AZ::Uuid::CreateNull(),
                const char* jobKey = nullptr,
                AssetSystem::JobStatus status = AssetSystem::JobStatus::Any);
            bool GetLegacySubIDsResult(const char* callname, SQLite::Statement* statement, AssetDatabaseConnection::legacySubIDsHandler handler);
            bool GetProductDependencyResult(const char* callName, SQLite::Statement* statement, AssetDatabaseConnection::productDependencyHandler handler);
            bool GetProductDependencyAndPathResult(const char* callName, SQLite::Statement* statement, AssetDatabaseConnection::productDependencyAndPathHandler handler);
            bool GetMissingProductDependencyResult(const char* callName, SQLite::Statement* statement, AssetDatabaseConnection::missingProductDependencyHandler handler);
            bool GetCombinedDependencyResult(const char* callName, SQLite::Statement* statement, AssetDatabaseConnection::combinedProductDependencyHandler handler);
            bool GetFileResult(const char* callName, SQLite::Statement* statement, AssetDatabaseConnection::fileHandler handler);
            bool GetStatResult(const char* callName, SQLite::Statement* statement, AssetDatabaseConnection::statHandler handler);
        }

        //////////////////////////////////////////////////////////////////////////
        //DatabaseInfoEntry
        DatabaseInfoEntry::DatabaseInfoEntry(AZ::s64 rowID, DatabaseVersion version)
            : m_rowID(rowID)
            , m_version(version)
        {
        }

        auto DatabaseInfoEntry::GetColumns()
        {
            return MakeColumns(
                MakeColumn("rowID", m_rowID),
                MakeColumn("version", m_version)
            );
        }

        //////////////////////////////////////////////////////////////////////////
        //ScanFolderDatabaseEntry
        ScanFolderDatabaseEntry::ScanFolderDatabaseEntry(
            AZ::s64 scanFolderID,
            const char* scanFolder,
            const char* displayName,
            const char* portableKey,
            int isRoot)
            : m_scanFolderID(scanFolderID)
            , m_isRoot(isRoot)
        {
            if (scanFolder)
            {
                m_scanFolder = scanFolder;
            }
            if (displayName)
            {
                m_displayName = displayName;
            }
            if (portableKey)
            {
                m_portableKey = portableKey;
            }
        }

        ScanFolderDatabaseEntry::ScanFolderDatabaseEntry(
            const char* scanFolder,
            const char* displayName,
            const char* portableKey,
            int isRoot)
            : m_isRoot(isRoot)
        {
            if (scanFolder)
            {
                m_scanFolder = scanFolder;
            }

            if (displayName)
            {
                m_displayName = displayName;
            }

            if (portableKey)
            {
                m_portableKey = portableKey;
            }
        }

        ScanFolderDatabaseEntry::ScanFolderDatabaseEntry(const ScanFolderDatabaseEntry& other)
            : m_scanFolderID(other.m_scanFolderID)
            , m_scanFolder(other.m_scanFolder)
            , m_displayName(other.m_displayName)
            , m_portableKey(other.m_portableKey)
            , m_isRoot(other.m_isRoot)
        {
        }

        ScanFolderDatabaseEntry::ScanFolderDatabaseEntry(ScanFolderDatabaseEntry&& other)
        {
            *this = AZStd::move(other);
        }

        ScanFolderDatabaseEntry& ScanFolderDatabaseEntry::operator=(ScanFolderDatabaseEntry&& other)
        {
            if (this != &other)
            {
                m_scanFolder = AZStd::move(other.m_scanFolder);
                m_scanFolderID = other.m_scanFolderID;
                m_displayName = AZStd::move(other.m_displayName);
                m_portableKey = AZStd::move(other.m_portableKey);
                m_isRoot = other.m_isRoot;
            }
            return *this;
        }

        ScanFolderDatabaseEntry& ScanFolderDatabaseEntry::operator=(const ScanFolderDatabaseEntry& other)
        {
            m_scanFolder = other.m_scanFolder;
            m_scanFolderID = other.m_scanFolderID;
            m_displayName = other.m_displayName;
            m_portableKey = other.m_portableKey;
            m_isRoot = other.m_isRoot;
            return *this;
        }

        bool ScanFolderDatabaseEntry::operator==(const ScanFolderDatabaseEntry& other) const
        {
            // its the same database entry when the portable key is the same.
            return m_portableKey == other.m_portableKey;
        }

        AZStd::string ScanFolderDatabaseEntry::ToString() const
        {
            return AZStd::string::format("ScanFolderDatabaseEntry id:%" PRId64 " path: %s, displayname: %s, portable key: %s",
                static_cast<int64_t>(m_scanFolderID),
                m_scanFolder.c_str(),
                m_displayName.c_str(),
                m_portableKey.c_str());
        }

        auto ScanFolderDatabaseEntry::GetColumns()
        {
            return MakeColumns(
                MakeColumn("ScanFolderID", m_scanFolderID),
                MakeColumn("ScanFolder", m_scanFolder),
                MakeColumn("DisplayName", m_displayName),
                MakeColumn("PortableKey", m_portableKey),
                MakeColumn("IsRoot", m_isRoot)
            );
        }

        //////////////////////////////////////////////////////////////////////////
        //SourceDatabaseEntry
        SourceDatabaseEntry::SourceDatabaseEntry(AZ::s64 sourceID, AZ::s64 scanFolderPK, const char* sourceName, AZ::Uuid sourceGuid, const char* analysisFingerprint)
            : m_sourceID(sourceID)
            , m_scanFolderPK(scanFolderPK)
            , m_sourceGuid(sourceGuid)
        {
            if (sourceName)
            {
                m_sourceName = sourceName;
            }
            if (analysisFingerprint)
            {
                m_analysisFingerprint = analysisFingerprint;
            }
        }

        SourceDatabaseEntry::SourceDatabaseEntry(AZ::s64 scanFolderPK, const char* sourceName, AZ::Uuid sourceGuid, const char* analysisFingerprint)
            : m_scanFolderPK(scanFolderPK)
            , m_sourceGuid(sourceGuid)
        {
            if (sourceName)
            {
                m_sourceName = sourceName;
            }

            if (analysisFingerprint)
            {
                m_analysisFingerprint = analysisFingerprint;
            }
        }

        AZStd::string SourceDatabaseEntry::ToString() const
        {
            return AZStd::string::format("SourceDatabaseEntry id:%" PRId64 " scanfolderpk: %" PRId64 " sourcename: %s sourceguid: %s",
                                         static_cast<int64_t>(m_sourceID), static_cast<int64_t>(m_scanFolderPK), m_sourceName.c_str(), m_sourceGuid.ToString<AZStd::string>().c_str());
        }

        auto SourceDatabaseEntry::GetColumns()
        {
            return MakeColumns(
                MakeColumn("SourceID", m_sourceID),
                MakeColumn("ScanFolderPK", m_scanFolderPK),
                MakeColumn("SourceName", m_sourceName),
                MakeColumn("SourceGuid", m_sourceGuid),
                MakeColumn("AnalysisFingerprint", m_analysisFingerprint)
            );
        }

        //////////////////////////////////////////////////////////////////////////
        // BuilderInfoEntry
        BuilderInfoEntry::BuilderInfoEntry(AZ::s64 builderInfoID, const AZ::Uuid& builderUuid, const char* analysisFingerprint)
            : m_builderInfoID(builderInfoID)
            , m_builderUuid(builderUuid)
        {
            if (analysisFingerprint)
            {
                m_analysisFingerprint = analysisFingerprint;
            }
        }

        AZStd::string BuilderInfoEntry::ToString() const
        {
            return AZStd::string::format("BuilderInfoEntry id:%" PRId64 " uuid: %s fingerprint: %s", static_cast<int64_t>(m_builderInfoID),
                m_builderUuid.ToString<AZStd::string>().c_str(), m_analysisFingerprint.c_str());
        }

        auto BuilderInfoEntry::GetColumns()
        {
            return MakeColumns(
                MakeColumn("BuilderID", m_builderInfoID),
                MakeColumn("Guid", m_builderUuid),
                MakeColumn("AnalysisFingerprint", m_analysisFingerprint)
            );
        }

        //////////////////////////////////////////////////////////////////////////
        //SourceFileDependencyEntry

        SourceFileDependencyEntry::SourceFileDependencyEntry(AZ::Uuid builderGuid, AZ::Uuid sourceGuid, PathOrUuid dependsOnSource, SourceFileDependencyEntry::TypeOfDependency dependencyType, bool fromAssetId, const char* subIds)
            : m_builderGuid(builderGuid)
            , m_sourceGuid(sourceGuid)
            , m_dependsOnSource(AZStd::move(dependsOnSource))
            , m_typeOfDependency(dependencyType)
            , m_fromAssetId(fromAssetId)
            , m_subIds(subIds)
        {
            AZ_Assert(dependencyType != SourceFileDependencyEntry::DEP_Any, "You may only store actual dependency types in the database, not DEP_Any");
        }

        AZStd::string SourceFileDependencyEntry::ToString() const
        {
            return AZStd::string::format(
                "SourceFileDependencyEntry id:%" PRId64 " builderGuid: %s source: %s "
                " dependsOnSource: %s type: %s fromAssetId: %u subIds: %s",
                static_cast<int64_t>(m_sourceDependencyID),
                m_builderGuid.ToFixedString().c_str(),
                m_sourceGuid.ToFixedString().c_str(),
                m_dependsOnSource.ToString().c_str(),
                m_typeOfDependency == DEP_SourceToSource ? "source" : "job",
                m_fromAssetId,
                m_subIds.c_str());
        }

        auto SourceFileDependencyEntry::GetColumns()
        {
            return MakeColumns(
                MakeColumn("SourceDependencyID", m_sourceDependencyID),
                MakeColumn("BuilderGuid", m_builderGuid),
                MakeColumn("SourceGuid", m_sourceGuid),
                MakeColumn("DependsOnSource", m_dependsOnSource),
                MakeColumn("TypeOfDependency", m_typeOfDependency),
                MakeColumn("FromAssetId", m_fromAssetId),
                MakeColumn("SubIds", m_subIds)
            );
        }

        //////////////////////////////////////////////////////////////////////////
        //JobDatabaseEntry
        JobDatabaseEntry::JobDatabaseEntry(AZ::s64 jobID, AZ::s64 sourcePK, const char* jobKey, AZ::u32 fingerprint, const char* platform, AZ::Uuid builderGuid, AssetSystem::JobStatus status, AZ::u64 jobRunKey, AZ::s64 firstFailLogTime, const char* firstFailLogFile, AZ::s64 lastFailLogTime, const char* lastFailLogFile, AZ::s64 lastLogTime, const char* lastLogFile, AZ::u32 warningCount, AZ::u32 errorCount)
            : m_jobID(jobID)
            , m_sourcePK(sourcePK)
            , m_fingerprint(fingerprint)
            , m_builderGuid(builderGuid)
            , m_status(status)
            , m_jobRunKey(jobRunKey)
            , m_firstFailLogTime(firstFailLogTime)
            , m_lastFailLogTime(lastFailLogTime)
            , m_lastLogTime(lastLogTime)
            , m_warningCount(warningCount)
            , m_errorCount(errorCount)
        {
            if (jobKey)
            {
                m_jobKey = jobKey;
            }
            if (platform)
            {
                m_platform = platform;
            }
            if (firstFailLogFile)
            {
                m_firstFailLogFile = firstFailLogFile;
            }
            if (lastFailLogFile)
            {
                m_lastFailLogFile = lastFailLogFile;
            }
            if (lastLogFile)
            {
                m_lastLogFile = lastLogFile;
            }
        }

        JobDatabaseEntry::JobDatabaseEntry(AZ::s64 sourcePK, const char* jobKey, AZ::u32 fingerprint, const char* platform, AZ::Uuid builderGuid, AssetSystem::JobStatus status, AZ::u64 jobRunKey, AZ::s64 firstFailLogTime, const char* firstFailLogFile, AZ::s64 lastFailLogTime, const char* lastFailLogFile, AZ::s64 lastLogTime, const char* lastLogFile, AZ::u32 warningCount, AZ::u32 errorCount)
            : m_sourcePK(sourcePK)
            , m_fingerprint(fingerprint)
            , m_builderGuid(builderGuid)
            , m_status(status)
            , m_jobRunKey(jobRunKey)
            , m_firstFailLogTime(firstFailLogTime)
            , m_lastFailLogTime(lastFailLogTime)
            , m_lastLogTime(lastLogTime)
            , m_warningCount(warningCount)
            , m_errorCount(errorCount)
        {
            if (jobKey)
            {
                m_jobKey = jobKey;
            }
            if (platform)
            {
                m_platform = platform;
            }
            if (firstFailLogFile)
            {
                m_firstFailLogFile = firstFailLogFile;
            }
            if (lastFailLogFile)
            {
                m_lastFailLogFile = lastFailLogFile;
            }
            if (lastLogFile)
            {
                m_lastLogFile = lastLogFile;
            }
        }

        bool JobDatabaseEntry::operator==(const JobDatabaseEntry& other) const
        {
            //equivalence is when everything but the id is the same
            return m_sourcePK == other.m_sourcePK &&
                   m_fingerprint == other.m_fingerprint &&
                   AzFramework::StringFunc::Equal(m_jobKey.c_str(), other.m_jobKey.c_str()) &&
                   AzFramework::StringFunc::Equal(m_platform.c_str(), other.m_platform.c_str()) &&
                   m_builderGuid == other.m_builderGuid &&
                   m_status == other.m_status &&
                   m_jobRunKey == other.m_jobRunKey &&
                   m_firstFailLogTime == other.m_firstFailLogTime &&
                   AzFramework::StringFunc::Equal(m_firstFailLogFile.c_str(), other.m_firstFailLogFile.c_str()) &&
                   m_lastFailLogTime == other.m_lastFailLogTime &&
                   AzFramework::StringFunc::Equal(m_lastFailLogFile.c_str(), other.m_lastFailLogFile.c_str()) &&
                   m_lastLogTime == other.m_lastLogTime &&
                   AzFramework::StringFunc::Equal(m_lastLogFile.c_str(), other.m_lastLogFile.c_str()) &&
                   m_errorCount == other.m_errorCount &&
                   m_warningCount == other.m_warningCount;
        }

        AZStd::string JobDatabaseEntry::ToString() const
        {
            return AZStd::string::format(
                "JobDatabaseEntry id:%" PRId64 " sourcepk:%" PRId64
                " jobkey: %s fingerprint: %i platform: %s builderguid: %s status: %s, failurecausesource: %" PRId64
                ", failurecausefingerprint: %i, warnings: %u, errors %u",
                static_cast<int64_t>(m_jobID),
                static_cast<int64_t>(m_sourcePK),
                m_jobKey.c_str(),
                m_fingerprint,
                m_platform.c_str(),
                m_builderGuid.ToString<AZStd::string>().c_str(),
                AssetSystem::JobStatusString(m_status),
                static_cast<int64_t>(m_failureCauseSourcePK),
                m_failureCauseFingerprint,
                m_warningCount,
                m_errorCount);
        }

        auto JobDatabaseEntry::GetColumns()
        {
            return MakeColumns(
                MakeColumn("JobID", m_jobID),
                MakeColumn("SourcePK", m_sourcePK),
                MakeColumn("JobKey", m_jobKey),
                MakeColumn("Fingerprint", m_fingerprint),
                MakeColumn("Platform", m_platform),
                MakeColumn("BuilderGuid", m_builderGuid),
                MakeColumn("Status", m_status),
                MakeColumn("JobRunKey", m_jobRunKey),
                MakeColumn("FailureCauseSourcePK", m_failureCauseSourcePK),
                MakeColumn("FailureCauseFingerprint", m_failureCauseFingerprint),
                MakeColumn("FirstFailLogTime", m_firstFailLogTime),
                MakeColumn("FirstFailLogFile", m_firstFailLogFile),
                MakeColumn("LastFailLogTime", m_lastFailLogTime),
                MakeColumn("LastFailLogFile", m_lastFailLogFile),
                MakeColumn("LastLogTime", m_lastLogTime),
                MakeColumn("LastLogFile", m_lastLogFile),
                MakeColumn("WarningCount", m_warningCount),
                MakeColumn("ErrorCount", m_errorCount)
            );
        }

        //////////////////////////////////////////////////////////////////////////
        //ProductDatabaseEntry
        ProductDatabaseEntry::ProductDatabaseEntry(AZ::s64 productID, AZ::s64 jobPK, AZ::u32 subID, const char* productName,
            AZ::Data::AssetType assetType, AZ::Uuid legacyGuid, AZ::u64 hash, AZStd::bitset<64> flags)
            : m_productID(productID)
            , m_jobPK(jobPK)
            , m_subID(subID)
            , m_assetType(assetType)
            , m_legacyGuid(legacyGuid)
            , m_hash(hash)
            , m_flags(flags)
        {
            if (productName)
            {
                m_productName = productName;
            }
        }

        ProductDatabaseEntry::ProductDatabaseEntry(AZ::s64 jobPK, AZ::u32 subID, const char* productName,
            AZ::Data::AssetType assetType, AZ::Uuid legacyGuid, AZ::u64 hash, AZStd::bitset<64> flags)
            : m_jobPK(jobPK)
            , m_subID(subID)
            , m_assetType(assetType)
            , m_legacyGuid(legacyGuid)
            , m_hash(hash)
            , m_flags(flags)
        {
            if (productName)
            {
                m_productName = productName;
            }
        }

        bool ProductDatabaseEntry::operator==(const ProductDatabaseEntry& other) const
        {
            //equivalence is when everything but the id is the same
            return m_jobPK == other.m_jobPK &&
                   m_subID == other.m_subID &&
                   m_assetType == other.m_assetType &&
                   m_hash == other.m_hash &&
                   AzFramework::StringFunc::Equal(m_productName.c_str(), other.m_productName.c_str()) &&
                   m_flags == other.m_flags;//don't compare legacy guid
        }

        bool ProductDatabaseEntry::IsSameLogicalProductAs(const ProductDatabaseEntry& other) const
        {
            return m_jobPK == other.m_jobPK &&
                m_subID == other.m_subID &&
                m_assetType == other.m_assetType &&
                AzFramework::StringFunc::Equal(m_productName.c_str(), other.m_productName.c_str());
        }

        AZStd::string ProductDatabaseEntry::ToString() const
        {
            return AZStd::string::format("ProductDatabaseEntry id:%" PRId64 " jobpk: %" PRId64 " subid: %i productname: %s assettype: %s hash: %" PRId64 " flags: %" PRId64,
                                         static_cast<int64_t>(m_productID), static_cast<int64_t>(m_jobPK), m_subID, m_productName.c_str(), m_assetType.ToString<AZStd::string>().c_str(), static_cast<int64_t>(m_hash), static_cast<int64_t>(m_flags.to_ullong()));
        }

        auto ProductDatabaseEntry::GetColumns()
        {
            return SQLite::MakeColumns(
                SQLite::MakeColumn("ProductID", m_productID),
                SQLite::MakeColumn("JobPK", m_jobPK),
                SQLite::MakeColumn("ProductName", m_productName),
                SQLite::MakeColumn("SubID", m_subID),
                SQLite::MakeColumn("AssetType", m_assetType),
                SQLite::MakeColumn("LegacyGuid", m_legacyGuid),
                SQLite::MakeColumn("Hash", m_hash),
                SQLite::MakeColumn("Flags", m_flags)
            );
        }

        /////////////////////////////
        // LegacySubIDsEntry
        // loaded from db, and thus includes the PK
        LegacySubIDsEntry::LegacySubIDsEntry(AZ::s64 subIDsEntryID, AZ::s64 productPK, AZ::u32 subId)
            : m_subIDsEntryID(subIDsEntryID)
            , m_productPK(productPK)
            , m_subID(subId)
        {
        }

        LegacySubIDsEntry::LegacySubIDsEntry(AZ::s64 productPK, AZ::u32 subId)
            : m_productPK(productPK)
            , m_subID(subId)
        {
        }

        bool LegacySubIDsEntry::operator==(const LegacySubIDsEntry& other) const
        {
            return m_productPK == other.m_productPK
                && m_subID == other.m_subID;
        }

        auto LegacySubIDsEntry::GetColumns()
        {
            return MakeColumns(
                MakeColumn("LegacySubID", m_subIDsEntryID),
                MakeColumn("ProductPK", m_productPK),
                MakeColumn("SubID", m_subID)
            );
        }

        //////////////////////////////////////////////////////////////////////////
        //ProductDependencyDatabaseEntry
        ProductDependencyDatabaseEntry::ProductDependencyDatabaseEntry(AZ::s64 productDependencyID, AZ::s64 productPK, AZ::Uuid dependencySourceGuid, AZ::u32 dependencySubID, AZStd::bitset<64> dependencyFlags, const AZStd::string& platform, const AZStd::string& unresolvedPath, DependencyType dependencyType, AZ::u32 fromAssetId)
            : m_productDependencyID(productDependencyID)
            , m_productPK(productPK)
            , m_dependencySourceGuid(dependencySourceGuid)
            , m_dependencySubID(dependencySubID)
            , m_dependencyFlags(dependencyFlags)
            , m_platform(platform)
            , m_unresolvedPath(unresolvedPath)
            , m_dependencyType(dependencyType)
            , m_fromAssetId(fromAssetId)
        {
        }

        ProductDependencyDatabaseEntry::ProductDependencyDatabaseEntry(AZ::s64 productPK, AZ::Uuid dependencySourceGuid, AZ::u32 dependencySubID, AZStd::bitset<64> dependencyFlags, const AZStd::string& platform, AZ::u32 fromAssetId, const AZStd::string& unresolvedPath, DependencyType dependencyType)
            : m_productPK(productPK)
            , m_dependencySourceGuid(dependencySourceGuid)
            , m_dependencySubID(dependencySubID)
            , m_dependencyFlags(dependencyFlags)
            , m_platform(platform)
            , m_unresolvedPath(unresolvedPath)
            , m_dependencyType(dependencyType)
            , m_fromAssetId(fromAssetId)
        {
        }

        bool ProductDependencyDatabaseEntry::operator==(const ProductDependencyDatabaseEntry& other) const
        {
            //equivalence is when everything but the id is the same
            return m_productPK == other.m_productPK &&
                   m_dependencySourceGuid == other.m_dependencySourceGuid &&
                   m_dependencySubID == other.m_dependencySubID &&
                   m_dependencyFlags == other.m_dependencyFlags &&
                   m_unresolvedPath == other.m_unresolvedPath &&
                   m_dependencyType == other.m_dependencyType &&
                   m_platform == other.m_platform;
        }

        AZStd::string ProductDependencyDatabaseEntry::ToString() const
        {
            return AZStd::string::format("ProductDependencyDatabaseEntry id: %" PRId64 " productpk: %" PRId64 " dependencysourceguid: %s dependencysubid: %i dependencyflags: %lu unresolvedPath: %s dependencyType: %u fromAssetId: %u",
                static_cast<int64_t>(m_productDependencyID), static_cast<int64_t>(m_productPK), m_dependencySourceGuid.ToString<AZStd::string>().c_str(),
                m_dependencySubID, m_dependencyFlags.to_ulong(), m_unresolvedPath.c_str(), static_cast<AZ::u32>(m_dependencyType), m_fromAssetId);
        }

        auto ProductDependencyDatabaseEntry::GetColumns()
        {
            return MakeColumns(
                MakeColumn("ProductDependencyID", m_productDependencyID),
                MakeColumn("ProductPK", m_productPK),
                MakeColumn("DependencySourceGuid", m_dependencySourceGuid),
                MakeColumn("DependencySubID", m_dependencySubID),
                MakeColumn("DependencyFlags", m_dependencyFlags),
                MakeColumn("Platform", m_platform),
                MakeColumn("UnresolvedPath", m_unresolvedPath),
                MakeColumn("UnresolvedDependencyType", m_dependencyType),
                MakeColumn("FromAssetId", m_fromAssetId)
            );
        }

        //////////////////////////////////////////////////////////////////////////
        //MissingProductDependencyDatabaseEntry
        MissingProductDependencyDatabaseEntry::MissingProductDependencyDatabaseEntry(
            AZ::s64 missingProductDependencyId,
            AZ::s64 productPK,
            const AZStd::string& scannerId,
            const AZStd::string& scannerVersion,
            const AZStd::string& sourceFileFingerprint,
            AZ::Uuid dependencySourceGuid,
            AZ::u32 dependencySubId,
            const AZStd::string& missingDependencyString,
            const AZStd::string& lastScanTime,
            AZ::u64 scanTimeSecondsSinceEpoch)
            : m_missingProductDependencyId(missingProductDependencyId)
            , m_productPK(productPK)
            , m_scannerId(scannerId)
            , m_scannerVersion(scannerVersion)
            , m_sourceFileFingerprint(sourceFileFingerprint)
            , m_dependencySourceGuid(dependencySourceGuid)
            , m_dependencySubId(dependencySubId)
            , m_missingDependencyString(missingDependencyString)
            , m_lastScanTime(lastScanTime)
            , m_scanTimeSecondsSinceEpoch(scanTimeSecondsSinceEpoch)
        {
        }

        MissingProductDependencyDatabaseEntry::MissingProductDependencyDatabaseEntry(
            AZ::s64 productPK,
            const AZStd::string& scannerId,
            const AZStd::string& scannerVersion,
            const AZStd::string& sourceFileFingerprint,
            AZ::Uuid dependencySourceGuid,
            AZ::u32 dependencySubId,
            const AZStd::string& missingDependencyString,
            const AZStd::string& lastScanTime,
            AZ::u64 scanTimeSecondsSinceEpoch)
            : m_productPK(productPK)
            , m_scannerId(scannerId)
            , m_scannerVersion(scannerVersion)
            , m_sourceFileFingerprint(sourceFileFingerprint)
            , m_dependencySourceGuid(dependencySourceGuid)
            , m_dependencySubId(dependencySubId)
            , m_missingDependencyString(missingDependencyString)
            , m_lastScanTime(lastScanTime)
            , m_scanTimeSecondsSinceEpoch(scanTimeSecondsSinceEpoch)
        {
        }

        bool MissingProductDependencyDatabaseEntry::operator==(const MissingProductDependencyDatabaseEntry& other) const
        {
            return m_productPK == other.m_productPK &&
                m_scannerId == other.m_scannerId &&
                m_scannerVersion == other.m_scannerVersion &&
                m_sourceFileFingerprint == other.m_sourceFileFingerprint &&
                m_dependencySourceGuid == other.m_dependencySourceGuid &&
                m_dependencySubId == other.m_dependencySubId &&
                m_missingDependencyString == other.m_missingDependencyString &&
                m_lastScanTime == other.m_lastScanTime &&
                m_scanTimeSecondsSinceEpoch == other.m_scanTimeSecondsSinceEpoch;
        }

        AZStd::string MissingProductDependencyDatabaseEntry::ToString() const
        {
            return AZStd::string::format(
                "MissingProductDependencyDatabaseEntry "
                "id: %" PRId64 " productpk: %" PRId64 " scannerid: %s scannerversion: %s sourceFileFingerprint: %s "
                "dependencysourceguid: %s dependencysubid: %u missingDependencyString: %s lastScanTime: %s scanTimeSecondsSinceEpoch %" PRIu64,
                static_cast<int64_t>(m_missingProductDependencyId),
                static_cast<int64_t>(m_productPK),
                m_scannerId.c_str(),
                m_scannerVersion.c_str(),
                m_sourceFileFingerprint.c_str(),
                m_dependencySourceGuid.ToString<AZStd::string>().c_str(),
                m_dependencySubId,
                m_missingDependencyString.c_str(),
                m_lastScanTime.c_str(),
                aznumeric_cast<uint64_t>(m_scanTimeSecondsSinceEpoch));
        }

        auto MissingProductDependencyDatabaseEntry::GetColumns()
        {
            return MakeColumns(
                MakeColumn("MissingProductDependencyId", m_missingProductDependencyId),
                MakeColumn("ProductPK", m_productPK),
                MakeColumn("ScannerId", m_scannerId),
                MakeColumn("ScannerVersion", m_scannerVersion),
                MakeColumn("SourceFileFingerprint", m_sourceFileFingerprint),
                MakeColumn("DependencySourceGuid", m_dependencySourceGuid),
                MakeColumn("DependencySubId", m_dependencySubId),
                MakeColumn("MissingDependencyString", m_missingDependencyString),
                MakeColumn("LastScanTime", m_lastScanTime),
                MakeColumn("ScanTimeSecondsSinceEpoch", m_scanTimeSecondsSinceEpoch)
            );
        }


        //////////////////////////////////////////////////////////////////////////
        //FileDatabaseEntry

        bool FileDatabaseEntry::operator==(const FileDatabaseEntry& other) const
        {
            return m_scanFolderPK == other.m_scanFolderPK
                && m_fileName == other.m_fileName
                && m_isFolder == other.m_isFolder
                && m_modTime == other.m_modTime
                && m_hash == other.m_hash;
        }

        AZStd::string FileDatabaseEntry::ToString() const
        {
            return AZStd::string::format("FileDatabaseEntry id: %" PRId64 " scanfolderpk: %" PRId64 " filename: %s isfolder: %i modtime: %" PRIu64 " hash: %" PRIu64,
                static_cast<int64_t>(m_fileID), static_cast<int64_t>(m_scanFolderPK), m_fileName.c_str(), m_isFolder, static_cast<uint64_t>(m_modTime), static_cast<uint64_t>(m_hash));
        }

        auto FileDatabaseEntry::GetColumns()
        {
            return MakeColumns(
                MakeColumn("FileID", m_fileID),
                MakeColumn("ScanFolderPK", m_scanFolderPK),
                MakeColumn("FileName", m_fileName),
                MakeColumn("IsFolder", m_isFolder),
                MakeColumn("ModTime", m_modTime),
                MakeColumn("Hash", m_hash)
            );
        }

        //////////////////////////////////////////////////////////////////////////

        auto SourceAndScanFolderDatabaseEntry::GetColumns()
        {
            return CombineColumns(
                ScanFolderDatabaseEntry::GetColumns(),
                SourceDatabaseEntry::GetColumns()
            );
        }

        auto CombinedDatabaseEntry::GetColumns()
        {
            return CombineColumns(ScanFolderDatabaseEntry::GetColumns(),
                SourceDatabaseEntry::GetColumns(),
                JobDatabaseEntry::GetColumns(),
                ProductDatabaseEntry::GetColumns());
        }

        //////////////////////////////////////////////////////////////////////////
        // StatDatabaseEntry
        bool StatDatabaseEntry::operator==(const StatDatabaseEntry& other) const
        {
            return m_statName == other.m_statName && m_statValue == other.m_statValue && m_lastLogTime == other.m_lastLogTime;
        }

        bool StatDatabaseEntry::operator!=(const StatDatabaseEntry& other) const
        {
            return m_statName != other.m_statName || m_statValue != other.m_statValue || m_lastLogTime != other.m_lastLogTime;
        }

        AZStd::string StatDatabaseEntry::ToString() const
        {
            return AZStd::string::format(
                "StatDatabaseEntry statname: %s statvalue: %" PRId64 " lastlogtime: %" PRId64,
                m_statName.c_str(),
                aznumeric_cast<int64_t>(m_statValue),
                aznumeric_cast<int64_t>(m_lastLogTime));
        }

        auto StatDatabaseEntry::GetColumns()
        {
            return MakeColumns(
                MakeColumn("StatName", m_statName),
                MakeColumn("StatValue", m_statValue),
                MakeColumn("LastLogTime", m_lastLogTime));
        }

        //////////////////////////////////////////////////////////////////////////
        //AssetDatabaseConnection
        AssetDatabaseConnection::AssetDatabaseConnection()
            : m_databaseConnection(nullptr)
        {
        }

        AssetDatabaseConnection::~AssetDatabaseConnection()
        {
            CloseDatabase();

            if (SQLite::SQLiteQueryLogBus::Handler::BusIsConnected())
            {
                SQLite::SQLiteQueryLogBus::Handler::BusDisconnect();
            }
        }

        void AssetDatabaseConnection::CloseDatabase()
        {
            if (m_databaseConnection)
            {
                m_databaseConnection->Close();
                delete m_databaseConnection;
                m_databaseConnection = nullptr;
            }
            m_validatedTables.clear();
        }

        AZStd::string AssetDatabaseConnection::GetAssetDatabaseFilePath()
        {
            AZStd::string databaseLocation;
            AssetDatabaseRequests::Bus::Broadcast(&AssetDatabaseRequests::Bus::Events::GetAssetDatabaseLocation, databaseLocation);
            if (databaseLocation.empty())
            {
                databaseLocation = "assetdb.sqlite";
            }
            return databaseLocation;
        }

        bool AssetDatabaseConnection::OpenDatabase(bool ignoreFutureAssetDBVersionError)
        {
            AZ_Assert(!m_databaseConnection, "Already open!");
            AZStd::string assetDatabaseLocation = GetAssetDatabaseFilePath();
            AZStd::string parentFolder = assetDatabaseLocation;
            AzFramework::StringFunc::Path::StripFullName(parentFolder);
            if (!parentFolder.empty())
            {
                AZ::IO::SystemFile::CreateDir(parentFolder.c_str());
            }

            if ((IsReadOnly()) && (!AZ::IO::SystemFile::Exists(assetDatabaseLocation.c_str())))
            {
                AZ_Error("Connection", false, "There is no asset data base in the cache folder.  Cannot open the database.  Make sure Asset Processor is running.");
                return false;
            }

            if (!IsReadOnly() && AZ::IO::SystemFile::Exists(assetDatabaseLocation.c_str()) && !AZ::IO::SystemFile::IsWritable(assetDatabaseLocation.c_str()))
            {
                AZ_Error("Connection", false, "Asset database file %s is marked read-only.  The cache should not be checked into source control.", assetDatabaseLocation.c_str());
                return false;
            }

            m_databaseConnection = aznew SQLite::Connection();
            AZ_Assert(m_databaseConnection, "No database created");

            if (!m_databaseConnection->Open(assetDatabaseLocation, IsReadOnly()))
            {
                delete m_databaseConnection;
                m_databaseConnection = nullptr;
                AZ_Warning("Connection", false, "Unable to open the asset database at %s\n", assetDatabaseLocation.c_str());
                return false;
            }

            m_validatedTables.clear();
            CreateStatements();

            if (!PostOpenDatabase(ignoreFutureAssetDBVersionError))
            {
                CloseDatabase();
                return false;
            }

            return true;
        }

        bool AssetDatabaseConnection::PostOpenDatabase([[maybe_unused]] bool ignoreFutureAssetDBVersionError)
        {
            // AssetDatabase.cpp handles the upgrading and version info, so ignoreFutureAssetDBVersionError isn't checked here.
            if (QueryDatabaseVersion() != CurrentDatabaseVersion())
            {
                AZ_Error(LOG_NAME, false, "Unable to open database - invalid version - database has %i and we want %i\n", QueryDatabaseVersion(), CurrentDatabaseVersion());
                return false;
            }

            // make sure that we can see all required tables.
            for (size_t tableIndex = 0; tableIndex < AZ_ARRAY_SIZE(EXPECTED_TABLES); ++tableIndex)
            {
                if (!ValidateDatabaseTable("PostOpenDatabase", EXPECTED_TABLES[tableIndex]))
                {
                    AZ_Error(LOG_NAME, false, "The asset database in the Cache folder appears to be from a newer version of Asset Processor.  The Asset Processor will close, to prevent data loss.\n");
                    return false;
                }
            }

            return true;
        }

        void AssetDatabaseConnection::CreateStatements()
        {
            AZ_Assert(m_databaseConnection, "No connection!");
            AZ_Assert(m_databaseConnection->IsOpen(), "Connection is not open");

            //////////////////////////////////////////////////////////////////////////
            //table queries
            AddStatement(m_databaseConnection, s_queryDatabaseinfoTable);
            AddStatement(m_databaseConnection, s_queryScanfoldersTable);
            AddStatement(m_databaseConnection, s_querySourcesTable);
            AddStatement(m_databaseConnection, s_queryJobsTable);
            AddStatement(m_databaseConnection, s_queryJobsTablePlatform);
            AddStatement(m_databaseConnection, s_queryProductsTable);
            AddStatement(m_databaseConnection, s_queryProductsTablePlatform);
            AddStatement(m_databaseConnection, s_queryLegacysubidsbyproductid);
            AddStatement(m_databaseConnection, s_queryProductdependenciesTable);
            AddStatement(m_databaseConnection, s_queryFilesTable);
            AddStatement(m_databaseConnection, s_queryStatsTable);

            //////////////////////////////////////////////////////////////////////////
            //projection and combination queries
            AddStatement(m_databaseConnection, s_queryScanfolderByScanfolderid);
            AddStatement(m_databaseConnection, s_queryScanfolderByDisplayname);
            AddStatement(m_databaseConnection, s_queryScanfolderByPortablekey);

            AddStatement(m_databaseConnection, s_querySourceBySourceid);
            AddStatement(m_databaseConnection, s_querySourceByScanfolderid);
            AddStatement(m_databaseConnection, s_querySourceBySourceguid);

            AddStatement(m_databaseConnection, s_querySourceBySourcename);
            AddStatement(m_databaseConnection, s_querySourceBySourcenameScanfolderid);
            AddStatement(m_databaseConnection, s_querySourceLikeSourcename);
            AddStatement(m_databaseConnection, s_querySourceLikeSourcenameScanfolder);
            AddStatement(m_databaseConnection, s_querySourceAnalysisFingerprint);
            AddStatement(m_databaseConnection, s_querySourcesAndScanfolders);

            AddStatement(m_databaseConnection, s_queryJobByJobid);
            AddStatement(m_databaseConnection, s_queryJobByJobkey);
            AddStatement(m_databaseConnection, s_queryJobByJobrunkey);
            AddStatement(m_databaseConnection, s_queryJobByProductid);
            AddStatement(m_databaseConnection, s_queryJobBySourceid);
            AddStatement(m_databaseConnection, s_queryJobBySourceidPlatform);
            AddStatement(m_databaseConnection, s_queryJobsByFailureCauseSourceId);

            AddStatement(m_databaseConnection, s_queryProductByProductid);
            AddStatement(m_databaseConnection, s_queryProductByJobid);
            AddStatement(m_databaseConnection, s_queryProductByJobidPlatform);
            AddStatement(m_databaseConnection, s_queryProductBySourceid);
            AddStatement(m_databaseConnection, s_queryProductBySourceidPlatform);

            AddStatement(m_databaseConnection, s_queryProductByProductname);
            AddStatement(m_databaseConnection, s_queryProductByProductnamePlatform);
            AddStatement(m_databaseConnection, s_queryProductLikeProductname);
            AddStatement(m_databaseConnection, s_queryProductLikeProductnamePlatform);

            AddStatement(m_databaseConnection, s_queryProductBySourcename);
            AddStatement(m_databaseConnection, s_queryProductBySourcenameScanFolderId);
            AddStatement(m_databaseConnection, s_queryProductBySourcenamePlatform);
            AddStatement(m_databaseConnection, s_queryProductBySourcenameScanFolderIdPlatform);
            AddStatement(m_databaseConnection, s_queryProductLikeSourcename);
            AddStatement(m_databaseConnection, s_queryProductLikeSourcenamePlatform);
            AddStatement(m_databaseConnection, s_queryProductByJobIdSubId);
            AddStatement(m_databaseConnection, s_queryProductBySourceGuidSubid);

            AddStatement(m_databaseConnection, s_queryCombined);
            AddStatement(m_databaseConnection, s_queryCombinedByPlatform);

            AddStatement(m_databaseConnection, s_queryCombinedBySourceid);
            AddStatement(m_databaseConnection, s_queryCombinedBySourceidPlatform);

            AddStatement(m_databaseConnection, s_queryCombinedByJobid);
            AddStatement(m_databaseConnection, s_queryCombinedByJobidPlatform);

            AddStatement(m_databaseConnection, s_queryCombinedByProductid);
            AddStatement(m_databaseConnection, s_queryCombinedByProductidPlatform);

            AddStatement(m_databaseConnection, s_queryCombinedBySourceguidProductsubid);
            AddStatement(m_databaseConnection, s_queryCombinedBySourceguidProductsubidPlatform);

            AddStatement(m_databaseConnection, s_queryCombinedBySourcename);
            AddStatement(m_databaseConnection, s_queryCombinedBySourcenamePlatform);

            AddStatement(m_databaseConnection, s_queryCombinedLikeSourcename);
            AddStatement(m_databaseConnection, s_queryCombinedLikeSourcenamePlatform);

            AddStatement(m_databaseConnection, s_queryCombinedByProductname);
            AddStatement(m_databaseConnection, s_queryCombinedByProductnamePlatform);

            AddStatement(m_databaseConnection, s_queryCombinedLikeProductname);
            AddStatement(m_databaseConnection, s_queryCombinedLikeProductnamePlatform);

            AddStatement(m_databaseConnection, s_querySourceDependency);
            AddStatement(m_databaseConnection, s_querySourcedependencyBySourcedependencyid);
            AddStatement(m_databaseConnection, s_querySourceDependencyByDependsOnSource);
            AddStatement(m_databaseConnection, s_querySourceDependencyByDependsOnSourceWildcard);
            AddStatement(m_databaseConnection, s_queryDependsOnSourceBySource);

            AddStatement(m_databaseConnection, s_queryProductdependencyByProductdependencyid);
            AddStatement(m_databaseConnection, s_queryProductdependencyByProductid);
            AddStatement(m_databaseConnection, s_queryProductdependencyBySourceGuidSubId);
            AddStatement(m_databaseConnection, s_queryProductDependenciesThatDependOnProductBySourceId);

            AddStatement(m_databaseConnection, s_queryMissingProductDependencyByProductId);
            AddStatement(m_databaseConnection, s_queryMissingProductDependencyByMissingProductDependencyId);
            AddStatement(m_databaseConnection, s_deleteMissingProductDependencyByProductId);

            AddStatement(m_databaseConnection, s_queryDirectProductdependencies);
            AddStatement(m_databaseConnection, s_queryDirectReverseProductdependenciesBySourceGuidSubId);
            AddStatement(m_databaseConnection, s_queryDirectReverseProductDependenciesBySourceGuidAllPlatforms);
            AddStatement(m_databaseConnection, s_queryAllProductdependencies);
            AddStatement(m_databaseConnection, s_queryUnresolvedProductDependencies);
            AddStatement(m_databaseConnection, s_queryProductDependencyExclusions);
            AddStatement(m_databaseConnection, s_createUnresolvedProductDependenciesTempTable);
            AddStatement(m_databaseConnection, s_queryInsertProductDependencyTempTableValues);
            AddStatement(m_databaseConnection, s_queryGetUnresolvedProductDependenciesUsingTempTable);

            AddStatement(m_databaseConnection, s_queryFileByFileid);
            AddStatement(m_databaseConnection, s_queryFilesByFileName);
            AddStatement(m_databaseConnection, s_queryFilesLikeFileNameScanfolderid);
            AddStatement(m_databaseConnection, s_queryFilesByScanfolderid);
            AddStatement(m_databaseConnection, s_queryFileByFileNameScanfolderid);

            AddStatement(m_databaseConnection, s_queryStatByStatName);
            AddStatement(m_databaseConnection, s_queryStatLikeStatName);

            AddStatement(m_databaseConnection, s_queryBuilderInfoTable);
        }

        //////////////////////////////////////////////////////////////////////////
        //Like
        AZStd::string AssetDatabaseConnection::GetLikeActualSearchTerm(const char* likeString, LikeType likeType)
        {
            AZStd::string actualSearchTerm = likeString;
            if (likeType == StartsWith)
            {
                StringFunc::Replace(actualSearchTerm, "%", "|%");
                StringFunc::Replace(actualSearchTerm, "_", "|_");
                StringFunc::Append(actualSearchTerm, "%");
            }
            else if (likeType == EndsWith)
            {
                StringFunc::Replace(actualSearchTerm, "%", "|%");
                StringFunc::Replace(actualSearchTerm, "_", "|_");
                StringFunc::Prepend(actualSearchTerm, "%");
            }
            else if (likeType == Matches)
            {
                StringFunc::Replace(actualSearchTerm, "%", "|%");
                StringFunc::Replace(actualSearchTerm, "_", "|_");
                StringFunc::Prepend(actualSearchTerm, "%");
                StringFunc::Append(actualSearchTerm, "%");
            }
            //raw default

            return actualSearchTerm;
        }

        //////////////////////////////////////////////////////////////////////////
        //Table queries

        bool AssetDatabaseConnection::QueryDatabaseInfoTable(databaseInfoHandler handler)
        {
            return s_queryDatabaseinfoTable.BindAndQuery(*m_databaseConnection, handler, &GetDatabaseInfoResult);
        }

        DatabaseVersion AssetDatabaseConnection::QueryDatabaseVersion()
        {
            DatabaseVersion dbVersion;
            bool res = QueryDatabaseInfoTable(
                    [&](DatabaseInfoEntry& entry)
                    {
                        dbVersion = entry.m_version;
                        return true; //see all of them
                    });

            if (res)
            {
                return dbVersion;
            }

            return DatabaseVersion::DatabaseDoesNotExist;
        }

        bool AssetDatabaseConnection::QueryScanFoldersTable(scanFolderHandler handler)
        {
            return s_queryScanfoldersTable.BindAndQuery(*m_databaseConnection, handler, &GetScanFolderResult);
        }

        bool AssetDatabaseConnection::QuerySourcesTable(sourceHandler handler)
        {
            return s_querySourcesTable.BindAndQuery(*m_databaseConnection, handler, &GetSourceResult);
        }

        bool AssetDatabaseConnection::QueryJobsTable(jobHandler handler, AZ::Uuid builderGuid, const char* jobKey, const char* platform, AssetSystem::JobStatus status)
        {
            if (platform && strlen(platform))
            {
                return s_queryJobsTablePlatform.BindAndThen(*m_databaseConnection, handler, platform).Query(&GetJobResult, builderGuid, jobKey, status);
            }

            return s_queryJobsTable.BindAndThen(*m_databaseConnection, handler).Query(&GetJobResult, builderGuid, jobKey, status);
        }

        bool AssetDatabaseConnection::QueryProductsTable(productHandler handler, AZ::Uuid builderGuid, const char* jobKey, const char* platform, AssetSystem::JobStatus status)
        {
            if (platform && strlen(platform))
            {
                return s_queryProductsTablePlatform.BindAndThen(*m_databaseConnection, handler, platform).Query(&GetProductResult, builderGuid, jobKey, status);
            }

            return s_queryProductsTable.BindAndThen(*m_databaseConnection, handler).Query(&GetProductResult, builderGuid, jobKey, status);
        }

        bool AssetDatabaseConnection::QueryProductDependenciesTable(combinedProductDependencyHandler handler)
        {
            return s_queryProductdependenciesTable.BindAndQuery(*m_databaseConnection, handler, &GetCombinedDependencyResult);
        }

        bool AssetDatabaseConnection::QueryFilesTable(fileHandler handler)
        {
            return s_queryFilesTable.BindAndQuery(*m_databaseConnection, handler, &GetFileResult);
        }

        bool AssetDatabaseConnection::QueryStatsTable(statHandler handler)
        {
            return s_queryStatsTable.BindAndQuery(*m_databaseConnection, handler, &GetStatResult);
        }

        bool AssetDatabaseConnection::QueryScanFolderByScanFolderID(AZ::s64 scanfolderid, scanFolderHandler handler)
        {
            return s_queryScanfolderByScanfolderid.BindAndQuery(*m_databaseConnection, handler, &GetScanFolderResult, scanfolderid);
        }

        bool AssetDatabaseConnection::QueryScanFolderBySourceID(AZ::s64 sourceID, scanFolderHandler handler)
        {
            bool found = false;
            bool succeeded = QueryCombinedBySourceID(sourceID,
                    [&](CombinedDatabaseEntry& combined)
                    {
                        found = true;
                        ScanFolderDatabaseEntry scanFolder = AZStd::move(combined);
                        return handler(scanFolder);
                    });
            return found && succeeded;
        }

        bool AssetDatabaseConnection::QueryScanFolderByJobID(AZ::s64 jobID, scanFolderHandler handler)
        {
            bool found = false;
            bool succeeded = QueryCombinedByJobID(jobID,
                    [&](CombinedDatabaseEntry& combined)
                    {
                        found = true;
                        ScanFolderDatabaseEntry scanFolder = AZStd::move(combined);
                        return handler(scanFolder);
                    });
            return found && succeeded;
        }

        bool AssetDatabaseConnection::QueryScanFolderByProductID(AZ::s64 productID, scanFolderHandler handler)
        {
            bool found = false;
            bool succeeded = QueryCombinedBySourceID(productID,
                    [&](CombinedDatabaseEntry& combined)
                    {
                        found = true;
                        ScanFolderDatabaseEntry scanFolder = AZStd::move(combined);
                        return handler(scanFolder);
                    });
            return found && succeeded;
        }

        bool AssetDatabaseConnection::QueryScanFolderByDisplayName(const char* displayName, scanFolderHandler handler)
        {
            return s_queryScanfolderByDisplayname.BindAndQuery(*m_databaseConnection, handler, &GetScanFolderResult, displayName);
        }

        bool AssetDatabaseConnection::QueryScanFolderByPortableKey(const char* portableKey, scanFolderHandler handler)
        {
            return s_queryScanfolderByPortablekey.BindAndQuery(*m_databaseConnection, handler, &GetScanFolderResult, portableKey);
        }

        bool AssetDatabaseConnection::QuerySourceBySourceID(AZ::s64 sourceid, sourceHandler handler)
        {
            return s_querySourceBySourceid.BindAndQuery(*m_databaseConnection, handler, &GetSourceResult, sourceid);
        }

        bool AssetDatabaseConnection::QuerySourceByScanFolderID(AZ::s64 scanFolderID, sourceHandler handler)
        {
            return s_querySourceByScanfolderid.BindAndQuery(*m_databaseConnection, handler, &GetSourceResult, scanFolderID);
        }

        bool AssetDatabaseConnection::QuerySourceByJobID(AZ::s64 jobid, sourceHandler handler)
        {
            return QueryCombinedByJobID(jobid,
                [&](CombinedDatabaseEntry& combined)
                {
                    SourceDatabaseEntry source;
                    source = AZStd::move(combined);
                    handler(source);
                    return false;//one
                });
        }

        bool AssetDatabaseConnection::QuerySourceByProductID(AZ::s64 productid, sourceHandler handler)
        {
            return QueryCombinedByProductID(productid,
                [&](CombinedDatabaseEntry& combined)
                {
                    SourceDatabaseEntry source;
                    source = AZStd::move(combined);
                    handler(source);
                    return false;//one
                });
        }

        bool AssetDatabaseConnection::QuerySourceBySourceGuid(AZ::Uuid sourceGuid, sourceHandler handler)
        {
            return s_querySourceBySourceguid.BindAndQuery(*m_databaseConnection, handler, &GetSourceResult, sourceGuid);
        }

        bool AssetDatabaseConnection::QuerySourceBySourceName(const char* exactSourceName, sourceHandler handler)
        {
            return s_querySourceBySourcename.BindAndQuery(*m_databaseConnection, handler, &GetSourceResult, exactSourceName);
        }

        bool AssetDatabaseConnection::QuerySourceAnalysisFingerprint(const char* exactSourceName, AZ::s64 scanFolderID, AZStd::string& outFingerprint)
        {
            outFingerprint.clear();

            StatementAutoFinalizer autoFinal;
            if (!s_querySourceAnalysisFingerprint.Bind(*m_databaseConnection, autoFinal, exactSourceName, scanFolderID))
            {
                return false;
            }

            Statement* statement = autoFinal.Get();
            Statement::SqlStatus result = statement->Step();

            bool validResult = result == Statement::SqlDone; // this means no results, but no error
            if (result == Statement::SqlOK) // this means results, no error
            {
                // this is a highly optimized query and always results in only one column.
                outFingerprint = statement->GetColumnText(0);
                validResult = true;
            }
            return validResult;
        }

        bool AssetDatabaseConnection::QuerySourceAndScanfolder(combinedSourceScanFolderHandler handler)
        {
            return s_querySourcesAndScanfolders.BindAndQuery(*m_databaseConnection, handler, &GetSourceAndScanfolderResult);
        }

        bool AssetDatabaseConnection::QuerySourceBySourceNameScanFolderID(const char* exactSourceName, AZ::s64 scanFolderID, sourceHandler handler)
        {
            return s_querySourceBySourcenameScanfolderid.BindAndQuery(*m_databaseConnection, handler, &GetSourceResult, exactSourceName, scanFolderID);
        }

        bool AssetDatabaseConnection::QuerySourceLikeSourceName(const char* likeSourceName, LikeType likeType, sourceHandler handler)
        {
            AZStd::string actualSearchTerm = GetLikeActualSearchTerm(likeSourceName, likeType);

            return s_querySourceLikeSourcename.BindAndQuery(*m_databaseConnection, handler, &GetSourceResult, actualSearchTerm.c_str());
        }

        bool AssetDatabaseConnection::QuerySourceLikeSourceNameScanFolderID(
            const char* likeSourceName, AZ::s64 scanFolderID, LikeType likeType, sourceHandler handler)
        {
            AZStd::string actualSearchTerm = GetLikeActualSearchTerm(likeSourceName, likeType);

            return s_querySourceLikeSourcenameScanfolder.BindAndQuery(*m_databaseConnection, handler, &GetSourceResult, actualSearchTerm.c_str(), scanFolderID);
        }

        bool AssetDatabaseConnection::QueryJobByJobID(AZ::s64 jobid, jobHandler handler)
        {
            return s_queryJobByJobid.BindAndQuery(*m_databaseConnection, handler, &GetJobResultSimple, jobid);
        }

        bool AssetDatabaseConnection::QueryJobByJobKey(AZStd::string jobKey, jobHandler handler)
        {
            return s_queryJobByJobkey.BindAndQuery(*m_databaseConnection, handler, &GetJobResultSimple, jobKey.c_str());
        }

        bool AssetDatabaseConnection::QueryJobByJobRunKey(AZ::u64 jobrunkey, jobHandler handler)
        {
            return s_queryJobByJobrunkey.BindAndQuery(*m_databaseConnection, handler, &GetJobResultSimple, jobrunkey);
        }

        bool AssetDatabaseConnection::QueryJobByProductID(AZ::s64 productid, jobHandler handler)
        {
            return s_queryJobByProductid.BindAndQuery(*m_databaseConnection, handler, &GetJobResultSimple, productid);
        }

        bool AssetDatabaseConnection::QueryJobBySourceID(AZ::s64 sourceID, jobHandler handler, AZ::Uuid builderGuid, const char* jobKey, const char* platform, AssetSystem::JobStatus status)
        {
            if (platform && strlen(platform))
            {
                return s_queryJobBySourceidPlatform.BindAndThen(*m_databaseConnection, handler, sourceID, platform).Query(&GetJobResult, builderGuid, jobKey, status);
            }

            return s_queryJobBySourceid.BindAndThen(*m_databaseConnection, handler, sourceID).Query(&GetJobResult, builderGuid, jobKey, status);
        }

        bool AssetDatabaseConnection::QueryJobsByFailureCauseSourceID(AZ::s64 sourceID, jobHandler handler)
        {
            return s_queryJobsByFailureCauseSourceId.BindAndQuery(*m_databaseConnection, handler, &GetJobResultSimple, sourceID);
        }

        bool AssetDatabaseConnection::QueryProductByProductID(AZ::s64 productid, productHandler handler)
        {
            return s_queryProductByProductid.BindAndQuery(*m_databaseConnection, handler, &GetProductResultSimple, productid);
        }

        bool AssetDatabaseConnection::QueryProductByJobID(AZ::s64 jobid, productHandler handler, AZ::Uuid builderGuid, const char* jobKey, const char* platform, AssetSystem::JobStatus status)
        {
            if (platform && strlen(platform))
            {
                return s_queryProductByJobidPlatform.BindAndThen(*m_databaseConnection, handler, jobid, platform).Query(&GetProductResult, builderGuid, jobKey, status);
            }

            return s_queryProductByJobid.BindAndThen(*m_databaseConnection, handler, jobid).Query(&GetProductResult, builderGuid, jobKey, status);
        }

        bool AssetDatabaseConnection::QueryProductBySourceID(AZ::s64 sourceid, productHandler handler, AZ::Uuid builderGuid, const char* jobKey, const char* platform, AssetSystem::JobStatus status)
        {
            if (platform && strlen(platform))
            {
                return s_queryProductBySourceidPlatform.BindAndThen(*m_databaseConnection, handler, sourceid, platform).Query(&GetProductResult, builderGuid, jobKey, status);
            }

            return s_queryProductBySourceid.BindAndThen(*m_databaseConnection, handler, sourceid).Query(&GetProductResult, builderGuid, jobKey, status);
        }

        bool AssetDatabaseConnection::QueryProductBySourceGuidSubID(AZ::Uuid sourceGuid, AZ::u32 productSubID, productHandler handler)
        {
            return s_queryProductBySourceGuidSubid.BindAndQuery(*m_databaseConnection, handler, &GetProductResultSimple, sourceGuid, productSubID);
        }

        bool AssetDatabaseConnection::QueryProductByProductName(const char* exactProductname, productHandler handler, AZ::Uuid builderGuid, const char* jobKey, const char* platform, AssetSystem::JobStatus status)
        {
            if (platform && strlen(platform))
            {
                return s_queryProductByProductnamePlatform.BindAndThen(*m_databaseConnection, handler, exactProductname, platform).Query(&GetProductResult, builderGuid, jobKey, status);
            }

            return s_queryProductByProductname.BindAndThen(*m_databaseConnection, handler, exactProductname).Query(&GetProductResult, builderGuid, jobKey, status);
        }

        bool AssetDatabaseConnection::QueryProductLikeProductName(const char* likeProductname, LikeType likeType, productHandler handler, AZ::Uuid builderGuid, const char* jobKey, const char* platform, AssetSystem::JobStatus status)
        {
            AZStd::string actualSearchTerm = GetLikeActualSearchTerm(likeProductname, likeType);

            if (platform && strlen(platform))
            {
                return s_queryProductLikeProductnamePlatform.BindAndThen(*m_databaseConnection, handler, actualSearchTerm.c_str(), platform).Query(&GetProductResult, builderGuid, jobKey, status);
            }

            return s_queryProductLikeProductname.BindAndThen(*m_databaseConnection, handler, actualSearchTerm.c_str()).Query(&GetProductResult, builderGuid, jobKey, status);
        }

        bool AssetDatabaseConnection::QueryProductBySourceName(const char* exactSourceName, productHandler handler, AZ::Uuid builderGuid, const char* jobKey, const char* platform, AssetSystem::JobStatus status)
        {
            if (platform && strlen(platform))
            {
                return s_queryProductBySourcenamePlatform.BindAndThen(*m_databaseConnection, handler, exactSourceName, platform).Query(&GetProductResult, builderGuid, jobKey, status);
            }

            return s_queryProductBySourcename.BindAndThen(*m_databaseConnection, handler, exactSourceName).Query(&GetProductResult, builderGuid, jobKey, status);
        }

        bool AssetDatabaseConnection::QueryProductBySourceNameScanFolderID(
            const char* exactSourceName,
            AZ::s64 scanFolderID,
            productHandler handler,
            AZ::Uuid builderGuid,
            const char* jobKey,
            const char* platform,
            AssetSystem::JobStatus status)
        {
            if (platform && strlen(platform))
            {
                return s_queryProductBySourcenameScanFolderIdPlatform.BindAndThen(*m_databaseConnection, handler, exactSourceName, platform, scanFolderID)
                    .Query(&GetProductResult, builderGuid, jobKey, status);
            }

            return s_queryProductBySourcenameScanFolderId.BindAndThen(*m_databaseConnection, handler, exactSourceName, scanFolderID)
                .Query(&GetProductResult, builderGuid, jobKey, status);
        }

        bool AssetDatabaseConnection::QueryProductLikeSourceName(const char* likeSourceName, LikeType likeType, productHandler handler, AZ::Uuid builderGuid, const char* jobKey, const char* platform, AssetSystem::JobStatus status)
        {
            AZStd::string actualSearchTerm = GetLikeActualSearchTerm(likeSourceName, likeType);

            if (platform && strlen(platform))
            {
                return s_queryProductLikeSourcenamePlatform.BindAndThen(*m_databaseConnection, handler, actualSearchTerm.c_str(), platform).Query(&GetProductResult, builderGuid, jobKey, status);
            }

            return s_queryProductLikeSourcename.BindAndThen(*m_databaseConnection, handler, actualSearchTerm.c_str()).Query(&GetProductResult, builderGuid, jobKey, status);
        }

        bool AssetDatabaseConnection::QueryProductByJobIDSubID(AZ::s64 jobID, AZ::u32 subId, productHandler handler)
        {
            return s_queryProductByJobIdSubId.BindAndQuery(*m_databaseConnection, handler, &GetProductResultSimple, jobID, subId);
        }

        bool AssetDatabaseConnection::QueryLegacySubIdsByProductID(AZ::s64 productId, legacySubIDsHandler handler)
        {
            return s_queryLegacysubidsbyproductid.BindAndQuery(*m_databaseConnection, handler, &GetLegacySubIDsResult, productId);
        }

        bool AssetDatabaseConnection::QueryCombined(combinedHandler handler, AZ::Uuid builderGuid, const char* jobKey, const char* platform, AssetSystem::JobStatus status, bool includeLegacySubIDs)
        {
            auto callback = [this, builderGuid, jobKey, status, includeLegacySubIDs](const char* callName, SQLite::Statement* statement, AssetDatabaseConnection::combinedHandler handler)
            {
                return this->GetCombinedResult(callName, statement, handler, builderGuid, jobKey, status, includeLegacySubIDs);
            };

            if (platform && strlen(platform))
            {
                return s_queryCombinedByPlatform.BindAndQuery(*m_databaseConnection, handler, callback, platform);
            }

            return s_queryCombined.BindAndQuery(*m_databaseConnection, handler, callback);
        }

        auto AssetDatabaseConnection::GetCombinedResultAsLambda()
        {
            return [this](const char* name, Statement* statement, combinedHandler handler, AZ::Uuid builderGuid, const char* jobKey, AzToolsFramework::AssetSystem::JobStatus status)
            {
                return GetCombinedResult(name, statement, handler, builderGuid, jobKey, status);
            };
        }

        bool AssetDatabaseConnection::QueryCombinedBySourceID(AZ::s64 sourceID, combinedHandler handler, AZ::Uuid builderGuid, const char* jobKey, const char* platform, AssetSystem::JobStatus status)
        {
            if (platform && strlen(platform))
            {
                return s_queryCombinedBySourceidPlatform.BindAndThen(*m_databaseConnection, handler, sourceID, platform)
                    .Query(GetCombinedResultAsLambda(), builderGuid, jobKey, status);
            }

            return s_queryCombinedBySourceid.BindAndThen(*m_databaseConnection, handler, sourceID)
                .Query(GetCombinedResultAsLambda(), builderGuid, jobKey, status);
        }

        bool AssetDatabaseConnection::QueryCombinedByJobID(AZ::s64 jobID, combinedHandler handler, AZ::Uuid builderGuid, const char* jobKey, const char* platform, AssetSystem::JobStatus status)
        {
            if (platform && strlen(platform))
            {
                return s_queryCombinedByJobidPlatform.BindAndThen(*m_databaseConnection, handler, jobID, platform)
                    .Query(GetCombinedResultAsLambda(), builderGuid, jobKey, status);
            }

            return s_queryCombinedByJobid.BindAndThen(*m_databaseConnection, handler, jobID)
                .Query(GetCombinedResultAsLambda(), builderGuid, jobKey, status);
        }

        bool AssetDatabaseConnection::QueryCombinedByProductID(AZ::s64 productID, combinedHandler handler, AZ::Uuid builderGuid, const char* jobKey, const char* platform, AssetSystem::JobStatus status)
        {
            if (platform && strlen(platform))
            {
                return s_queryCombinedByProductidPlatform.BindAndThen(*m_databaseConnection, handler, productID, platform)
                    .Query(GetCombinedResultAsLambda(), builderGuid, jobKey, status);
            }

            return s_queryCombinedByProductid.BindAndThen(*m_databaseConnection, handler, productID)
                .Query(GetCombinedResultAsLambda(), builderGuid, jobKey, status);
        }

        bool AssetDatabaseConnection::QueryCombinedBySourceGuidProductSubId(AZ::Uuid sourceGuid, AZ::u32 productSubID, combinedHandler handler, AZ::Uuid builderGuid, const char* jobKey, const char* platform, AssetSystem::JobStatus status)
        {
            if (platform && strlen(platform))
            {
                return s_queryCombinedBySourceguidProductsubidPlatform.BindAndThen(*m_databaseConnection, handler, productSubID, sourceGuid, platform)
                    .Query(GetCombinedResultAsLambda(), builderGuid, jobKey, status);
            }

            return s_queryCombinedBySourceguidProductsubid.BindAndThen(*m_databaseConnection, handler, productSubID, sourceGuid)
                .Query(GetCombinedResultAsLambda(), builderGuid, jobKey, status);
        }

        bool AssetDatabaseConnection::QueryCombinedBySourceName(const char* exactSourceName, combinedHandler handler, AZ::Uuid builderGuid, const char* jobKey, const char* platform, AssetSystem::JobStatus status)
        {
            if (platform && strlen(platform))
            {
                return s_queryCombinedBySourcenamePlatform.BindAndThen(*m_databaseConnection, handler, exactSourceName, platform)
                    .Query(GetCombinedResultAsLambda(), builderGuid, jobKey, status);
            }

            return s_queryCombinedBySourcename.BindAndThen(*m_databaseConnection, handler, exactSourceName)
                .Query(GetCombinedResultAsLambda(), builderGuid, jobKey, status);
        }

        bool AssetDatabaseConnection::QueryCombinedLikeSourceName(const char* likeSourceName, LikeType likeType, combinedHandler handler, AZ::Uuid builderGuid, const char* jobKey, const char* platform, AssetSystem::JobStatus status)
        {
            AZStd::string actualSearchTerm = GetLikeActualSearchTerm(likeSourceName, likeType);

            if (platform && strlen(platform))
            {
                return s_queryCombinedLikeSourcenamePlatform.BindAndThen(*m_databaseConnection, handler, actualSearchTerm.c_str(), platform)
                    .Query(GetCombinedResultAsLambda(), builderGuid, jobKey, status);
            }

            return s_queryCombinedLikeSourcename.BindAndThen(*m_databaseConnection, handler, actualSearchTerm.c_str())
                .Query(GetCombinedResultAsLambda(), builderGuid, jobKey, status);
        }

        bool AssetDatabaseConnection::QueryCombinedByProductName(const char* exactProductName, combinedHandler handler, AZ::Uuid builderGuid, const char* jobKey, const char* platform, AssetSystem::JobStatus status)
        {
            if (platform && strlen(platform))
            {
                return s_queryCombinedByProductnamePlatform.BindAndThen(*m_databaseConnection, handler, exactProductName, platform)
                    .Query(GetCombinedResultAsLambda(), builderGuid, jobKey, status);
            }

            return s_queryCombinedByProductname.BindAndThen(*m_databaseConnection, handler, exactProductName)
                .Query(GetCombinedResultAsLambda(), builderGuid, jobKey, status);
        }

        bool AssetDatabaseConnection::QueryCombinedLikeProductName(const char* likeProductName, LikeType likeType, combinedHandler handler, AZ::Uuid builderGuid, const char* jobKey, const char* platform, AssetSystem::JobStatus status)
        {
            AZStd::string actualSearchTerm = GetLikeActualSearchTerm(likeProductName, likeType);

            if (platform && strlen(platform))
            {
                return s_queryCombinedLikeProductnamePlatform.BindAndThen(*m_databaseConnection, handler, actualSearchTerm.c_str(), platform)
                    .Query(GetCombinedResultAsLambda(), builderGuid, jobKey, status);
            }

            return s_queryCombinedLikeProductname.BindAndThen(*m_databaseConnection, handler, actualSearchTerm.c_str())
                .Query(GetCombinedResultAsLambda(), builderGuid, jobKey, status);
        }

        bool AssetDatabaseConnection::QueryJobInfoByJobID(AZ::s64 jobID, jobInfoHandler handler)
        {
            SourceDatabaseEntry source;

            bool found = false;
            bool succeeded = QuerySourceByJobID(jobID,
                    [&](SourceDatabaseEntry& entry)
                    {
                        found = true;
                        source = AZStd::move(entry);
                        return false;//one
                    });

            if (!found || !succeeded)
            {
                return false;
            }

            found = false;
            succeeded = QueryJobByJobID(jobID,
                    [&](JobDatabaseEntry& entry)
                    {
                        found = true;
                        AzToolsFramework::AssetSystem::JobInfo jobinfo;
                        jobinfo.m_sourceFile = source.m_sourceName;
                        PopulateJobInfo(jobinfo, entry);
                        return handler(jobinfo);
                    });
            return found && succeeded;
        }

        bool AssetDatabaseConnection::QueryJobInfoByJobRunKey(AZ::u64 jobRunKey, jobInfoHandler handler)
        {
            bool found = false;
            bool succeeded = QueryJobByJobRunKey(jobRunKey,
                    [&](JobDatabaseEntry& entry)
                    {
                        AzToolsFramework::AssetSystem::JobInfo jobinfo;
                        AZ::s64 scanFolderId{};

                        succeeded = QuerySourceBySourceID(entry.m_sourcePK,
                                [&](SourceDatabaseEntry& sourceEntry)
                                {
                                    found = true;
                                    jobinfo.m_sourceFile = AZStd::move(sourceEntry.m_sourceName);
                                    scanFolderId = sourceEntry.m_scanFolderPK;
                                    return true;
                                });

                        succeeded = succeeded && found &&
                            QueryScanFolderByScanFolderID(
                                scanFolderId,
                                [&jobinfo](AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry scanfolder)
                                {
                                    jobinfo.m_watchFolder = scanfolder.m_scanFolder;
                                    return true;
                                });

                        if (!found)
                        {
                            return false;
                        }

                        PopulateJobInfo(jobinfo, entry);

                        return handler(jobinfo);
                    });
            return found && succeeded;
        }

        bool AssetDatabaseConnection::QueryJobInfoByJobKey(AZStd::string jobKey, jobInfoHandler handler)
        {
            bool found = false;
            bool succeeded = QueryJobByJobKey(jobKey,
                    [&](JobDatabaseEntry& entry)
                    {
                        AzToolsFramework::AssetSystem::JobInfo jobinfo;
                        succeeded = QuerySourceBySourceID(entry.m_sourcePK,
                                [&](SourceDatabaseEntry& sourceEntry)
                                {
                                    jobinfo.m_sourceFile = AZStd::move(sourceEntry.m_sourceName);
                                    QueryScanFolderBySourceID(sourceEntry.m_sourceID,
                                        [&](ScanFolderDatabaseEntry& scanFolderEntry)
                                        {
                                            found = true;
                                            jobinfo.m_watchFolder = scanFolderEntry.m_scanFolder;
                                            return false;
                                        });
                                    return true;
                                });

                        if (!found)
                        {
                            return false;
                        }

                        PopulateJobInfo(jobinfo, entry);

                        return handler(jobinfo);
                    });
            return found && succeeded;
        }

        bool AssetDatabaseConnection::QueryJobInfoBySourceNameScanFolderId(const char* sourceName, AZ::s64 scanfolderId, jobInfoHandler handler, AZ::Uuid builderGuid, const char* jobKey, const char* platform, AssetSystem::JobStatus status)
        {
            SourceDatabaseEntry source;

            bool found = false;
            bool succeeded = QuerySourceBySourceNameScanFolderID(
                sourceName, scanfolderId,
                    [&](SourceDatabaseEntry& entry)
                    {
                        found = true;
                        source = AZStd::move(entry);
                        return false;//one
                    });

            if (!found || !succeeded)
            {
                return false;
            }

            found = false;
            succeeded = QueryJobBySourceID(source.m_sourceID,
                    [&](JobDatabaseEntry& entry)
                    {
                        AzToolsFramework::AssetSystem::JobInfo jobinfo;
                        jobinfo.m_sourceFile = source.m_sourceName; //dont move, we may have many that need this name
                        QueryScanFolderBySourceID(source.m_sourceID,
                            [&](ScanFolderDatabaseEntry& scanFolderEntry)
                            {
                                found = true;
                                jobinfo.m_watchFolder = scanFolderEntry.m_scanFolder;
                                return false;
                            });
                        PopulateJobInfo(jobinfo, entry);

                        return handler(jobinfo);
                    }, builderGuid,
                    jobKey,
                    platform,
                    status);
            return found && succeeded;
        }

        bool AssetDatabaseConnection::QuerySourceDependencies(sourceFileDependencyHandler handler)
        {
            return s_querySourceDependency.BindAndQuery(*m_databaseConnection, handler, &GetSourceDependencyResult);
        }

        bool AssetDatabaseConnection::QuerySourceDependencyBySourceDependencyId(
            AZ::s64 sourceDependencyID, sourceFileDependencyHandler handler)
        {
            return s_querySourcedependencyBySourcedependencyid.BindAndQuery(*m_databaseConnection, handler, &GetSourceDependencyResult, sourceDependencyID);
        }

        bool AssetDatabaseConnection::QuerySourceDependencyByDependsOnSource(
            AZ::Uuid sourceGuid,
            const char* sourceName,
            const char* absolutePath,
            AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::TypeOfDependency dependencyType,
            sourceFileDependencyHandler handler)
        {
            if (dependencyType & AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_SourceLikeMatch)
            {
                return QuerySourceDependencyByDependsOnSourceWildcard(sourceGuid, sourceName, absolutePath, handler);
            }

            return s_querySourceDependencyByDependsOnSource.BindAndQuery(*m_databaseConnection, handler, &GetSourceDependencyResult, sourceGuid.ToFixedString(false, false).c_str(), sourceName, absolutePath, dependencyType);
        }

        bool AssetDatabaseConnection::QuerySourceDependencyByDependsOnSourceWildcard(AZ::Uuid sourceGuid, const char* sourceName, const char* absolutePath, sourceFileDependencyHandler handler)
        {
            SourceFileDependencyEntry::TypeOfDependency matchDependency = SourceFileDependencyEntry::TypeOfDependency::DEP_SourceOrJob;
            SourceFileDependencyEntry::TypeOfDependency wildcardDependency = SourceFileDependencyEntry::TypeOfDependency::DEP_SourceLikeMatch;

            return s_querySourceDependencyByDependsOnSourceWildcard.BindAndQuery(
                *m_databaseConnection,
                handler,
                &GetWildcardSourceDependencyResult,
                sourceGuid.ToFixedString(false, false).c_str(),
                sourceName,
                absolutePath,
                matchDependency,
                wildcardDependency);
        }

        bool AssetDatabaseConnection::QueryDependsOnSourceBySourceDependency(AZ::Uuid sourceGuid, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::TypeOfDependency dependencyType, sourceFileDependencyHandler handler)
        {
            return s_queryDependsOnSourceBySource.BindAndQuery(*m_databaseConnection, handler, &GetSourceDependencyResult, sourceGuid, dependencyType);
        }

        bool AzToolsFramework::AssetDatabase::AssetDatabaseConnection::QueryProductDependenciesThatDependOnProductBySourceId(AZ::s64 sourceId, productDependencyHandler handler)
        {
            return s_queryProductDependenciesThatDependOnProductBySourceId.BindAndQuery(*m_databaseConnection, handler, &GetProductDependencyResult, sourceId);
        }

        // Product Dependencies
        bool AssetDatabaseConnection::QueryProductDependencyByProductDependencyId(AZ::s64 productDependencyID, productDependencyHandler handler)
        {
            return s_queryProductdependencyByProductdependencyid.BindAndQuery(*m_databaseConnection, handler, &GetProductDependencyResult, productDependencyID);
        }

        bool AssetDatabaseConnection::QueryUnresolvedProductDependencies(productDependencyHandler handler)
        {
            return s_queryUnresolvedProductDependencies.BindAndQuery(*m_databaseConnection, handler, &GetProductDependencyResult);
        }

        bool AssetDatabaseConnection::QueryProductDependencyExclusions(productDependencyHandler handler)
        {
            return s_queryProductDependencyExclusions.BindAndQuery(*m_databaseConnection, handler, &GetProductDependencyResult);
        }

        bool AssetDatabaseConnection::QueryProductDependenciesUnresolvedAdvanced(
            const AZStd::vector<AZStd::string>& searchPaths, productDependencyAndPathHandler handler)
        {
            ScopedTransaction transaction(m_databaseConnection);

            bool result = s_createUnresolvedProductDependenciesTempTable.BindAndStep(*m_databaseConnection);

            for (auto&& path : searchPaths)
            {
                result = s_queryInsertProductDependencyTempTableValues.BindAndStep(*m_databaseConnection, path.c_str()) && result;
            }

            result = s_queryGetUnresolvedProductDependenciesUsingTempTable.BindAndQuery(
                         *m_databaseConnection, handler, &GetProductDependencyAndPathResult) &&
                result;

            result = m_databaseConnection->ExecuteRawSqlQuery("DROP TABLE QueryProductDependenciesUnresolvedAdvanced", nullptr, nullptr) &&
                result;

            transaction.Commit();

            return result;
        }

        bool AssetDatabaseConnection::QueryProductDependencyByProductId(AZ::s64 productID, productDependencyHandler handler)
        {
            return s_queryProductdependencyByProductid.BindAndQuery(*m_databaseConnection, handler, &GetProductDependencyResult, productID);
        }

        bool AssetDatabaseConnection::QueryProductDependencyBySourceGuidSubId(AZ::Uuid guid, AZ::u32 subId, const AZStd::string& platform, productDependencyHandler handler)
        {
            return s_queryProductdependencyBySourceGuidSubId.BindAndQuery(*m_databaseConnection, handler, &GetProductDependencyResult, guid, subId, platform.c_str());
        }

        bool AssetDatabaseConnection::QueryMissingProductDependencyByProductId(AZ::s64 productId, missingProductDependencyHandler handler)
        {
            return s_queryMissingProductDependencyByProductId.BindAndQuery(*m_databaseConnection, handler, &GetMissingProductDependencyResult, productId);
        }

        bool AssetDatabaseConnection::DeleteMissingProductDependencyByProductId(AZ::s64 productId)
        {
            return s_deleteMissingProductDependencyByProductId.BindAndStep(*m_databaseConnection, productId);
        }

        bool AssetDatabaseConnection::QueryMissingProductDependencyByMissingProductDependencyId(AZ::s64 missingProductDependencyId, missingProductDependencyHandler handler)
        {
            return s_queryMissingProductDependencyByMissingProductDependencyId.BindAndQuery(*m_databaseConnection, handler, &GetMissingProductDependencyResult, missingProductDependencyId);
        }

        bool AssetDatabaseConnection::QueryDirectProductDependencies(AZ::s64 productID, productHandler handler)
        {
            return s_queryDirectProductdependencies.BindAndQuery(*m_databaseConnection, handler, &GetProductResultSimple, productID);
        }

        bool AssetDatabaseConnection::QueryDirectReverseProductDependenciesBySourceGuidSubId(AZ::Uuid dependencySourceGuid, AZ::u32 dependencySubId, productHandler handler)
        {
            return s_queryDirectReverseProductdependenciesBySourceGuidSubId.BindAndQuery(*m_databaseConnection, handler, &GetProductResultSimple, dependencySourceGuid, dependencySubId);
        }

        bool AssetDatabaseConnection::QueryDirectReverseProductDependenciesBySourceGuidAllPlatforms(AZ::Uuid dependencySourceGuid, productDependencyHandler handler)
        {
            return s_queryDirectReverseProductDependenciesBySourceGuidAllPlatforms.BindAndQuery(*m_databaseConnection, handler, &GetProductDependencyResult, dependencySourceGuid);
        }

        bool AssetDatabaseConnection::QueryAllProductDependencies(AZ::s64 productID, productHandler handler)
        {
            return s_queryAllProductdependencies.BindAndQuery(*m_databaseConnection, handler, &GetProductResultSimple, productID);
        }

        bool AssetDatabaseConnection::QueryFileByFileID(AZ::s64 fileID, fileHandler handler)
        {
            return s_queryFileByFileid.BindAndQuery(*m_databaseConnection, handler, &GetFileResult, fileID);
        }

        bool AssetDatabaseConnection::QueryFilesByFileNameAndScanFolderID(const char* fileName, AZ::s64 scanFolderID, fileHandler handler)
        {
            return s_queryFilesByFileName.BindAndQuery(*m_databaseConnection, handler, &GetFileResult, scanFolderID, fileName);
        }

        bool AssetDatabaseConnection::QueryFilesLikeFileNameAndScanFolderID(const char* likeFileName, LikeType likeType, AZ::s64 scanFolderID, fileHandler handler)
        {
            AZStd::string actualSearchTerm = GetLikeActualSearchTerm(likeFileName, likeType);

            return s_queryFilesLikeFileNameScanfolderid.BindAndQuery(*m_databaseConnection, handler, &GetFileResult, actualSearchTerm.c_str(), scanFolderID);
        }

        bool AssetDatabaseConnection::QueryFilesByScanFolderID(AZ::s64 scanFolderID, fileHandler handler)
        {
            return s_queryFilesByScanfolderid.BindAndQuery(*m_databaseConnection, handler, &GetFileResult, scanFolderID);
        }

        bool AssetDatabaseConnection::QueryFileByFileNameScanFolderID(const char* fileName, AZ::s64 scanFolderID, fileHandler handler)
        {
            return s_queryFileByFileNameScanfolderid.BindAndQuery(*m_databaseConnection, handler, &GetFileResult, scanFolderID, fileName);
        }

        bool AssetDatabaseConnection::QueryStatByStatName(const char* statName, statHandler handler)
        {
            return s_queryStatByStatName.BindAndQuery(*m_databaseConnection, handler, &GetStatResult, statName);
        }

        bool AssetDatabaseConnection::QueryStatLikeStatName(const char* statName, statHandler handler)
        {
            return s_queryStatLikeStatName.BindAndQuery(*m_databaseConnection, handler, &GetStatResult, statName);
        }

        void AssetDatabaseConnection::SetQueryLogging(bool enableLogging)
        {
            if (enableLogging)
            {
                if (!SQLite::SQLiteQueryLogBus::Handler::BusIsConnected())
                {
                    SQLite::SQLiteQueryLogBus::Handler::BusConnect();
                }
            }
            else
            {
                if (SQLite::SQLiteQueryLogBus::Handler::BusIsConnected())
                {
                    SQLite::SQLiteQueryLogBus::Handler::BusDisconnect();
                }
            }
        }

        void AssetDatabaseConnection::LogQuery(const char* statement, const AZStd::string& params)
        {
            // Some builds, like dedicated server builds, compile out AZ_TracePrintf.
            AZ_UNUSED(statement);
            AZ_UNUSED(params);
            AZ_TracePrintf("SQLiteQuery", "%s = Params %s\n", statement, params.c_str());
        }

        void AssetDatabaseConnection::LogResultId(AZ::s64 rowId)
        {
            AZ_UNUSED(rowId);
            AZ_TracePrintf("SQLiteQuery", "Last Insert Row Id: %d\n", rowId);
        }

        bool AssetDatabaseConnection::QueryBuilderInfoTable(const BuilderInfoHandler& handler)
        {
            StatementAutoFinalizer autoFinal;
            if (!s_queryBuilderInfoTable.Bind(*m_databaseConnection, autoFinal))
            {
                return false;
            }

            Statement* statement = autoFinal.Get();

            Statement::SqlStatus result = statement->Step();

            if (result == Statement::SqlError)
            {
                AZ_Error(LOG_NAME, false, "SqlError occurred!");
                return false;
            }

            BuilderInfoEntry entry;
            auto boundColumns = entry.GetColumns();

            // possible values for result:
            // SqlDone:  There is no more data available (but there was no error)
            // SqlOk: There is data available.
            while (result == Statement::SqlOK)
            {
                if(!boundColumns.Fetch(statement))
                {
                    return false;
                }

                if(!handler(BuilderInfoEntry(entry)))
                {
                    break; // handler returned false meaning "do not continue to return rows"
                }
                result = statement->Step();
            }

            if (result == Statement::SqlError)
            {
                AZ_Warning(LOG_NAME, false, "SqlError occurred!");
                return false;
            }

            return true;
        }

        bool AssetDatabaseConnection::ValidateDatabaseTable(const char* callName, const char* tableName)
        {
            AZ_UNUSED(callName); // for release mode, when AZ_Error is compiled down to nothing.
            AZ_UNUSED(tableName);

            if (m_validatedTables.find(tableName) != m_validatedTables.end())
            {
                return true; // already validated.
            }

            if (!m_databaseConnection)
            {
                AZ_Error(LOG_NAME, false, "Fatal: attempt to work on a database connection that doesn't exist: %s", callName);
                return false;
            }

            if (!m_databaseConnection->IsOpen())
            {
                AZ_Error(LOG_NAME, false, "Fatal: attempt to work on a database connection that isn't open: %s", callName);
                return false;
            }

            if (!m_databaseConnection->DoesTableExist(tableName))
            {
                return false;
            }

            m_validatedTables.insert(tableName);

            return true;
        }

        // Simple helper function to determine if we should call the handler based on optional filtering criteria
        bool ResultMatchesJobCriteria(const char* jobKey, AZ::Uuid builderGuid, AssetSystem::JobStatus status, AZStd::string_view savedJobKey, AZ::Uuid savedBuilderGuid, AssetSystem::JobStatus savedJobStatus)
        {
            return (jobKey ? savedJobKey == jobKey : true)
                && (!builderGuid.IsNull() ? savedBuilderGuid == builderGuid : true)
                && (status != AssetSystem::JobStatus::Any ? savedJobStatus == status : true);
        }

        bool AssetDatabaseConnection::GetCombinedResult(const char* callName, Statement* statement, AssetDatabaseConnection::combinedHandler handler, AZ::Uuid builderGuid, const char* jobKey, AssetSystem::JobStatus status, bool includeLegacySubIDs)
        {
            AZ_UNUSED(callName); // AZ_Error may be compiled out entirely in release builds.
            Statement::SqlStatus result = statement->Step();

            CombinedDatabaseEntry combined;
            auto boundColumns = combined.GetColumns();

            bool validResult = result == Statement::SqlDone;
            while (result == Statement::SqlOK)
            {
                if (!boundColumns.Fetch(statement))
                {
                    return false;
                }

                if (ResultMatchesJobCriteria(jobKey, builderGuid, status, combined.m_jobKey, combined.m_builderGuid, combined.m_status))
                {
                    if (includeLegacySubIDs)
                    {
                        QueryLegacySubIdsByProductID(combined.m_productID, [&combined](LegacySubIDsEntry& entry)
                        {
                            combined.m_legacySubIDs.emplace_back(AZStd::move(entry));
                            return true;
                        }
                        );
                    }

                    if (handler(combined))
                    {
                        result = statement->Step();
                    }
                    else
                    {
                        result = Statement::SqlDone;
                    }
                }
                else
                {
                    result = statement->Step();
                }
                validResult = true;
                combined.m_legacySubIDs.clear();
            }

            if (result == Statement::SqlError)
            {
                AZ_Error(LOG_NAME, false, "Error occurred while stepping %s", callName);
                return false;
            }
            return validResult;
        }

        namespace
        {
            // Helper function that can handle the majority use-case of iterating every row and passing the TEntry result to the handler
            template<typename TEntry>
            bool GetResult(const char* callName, Statement* statement, AZStd::function<bool(TEntry& entry)> handler)
            {
                AZ_UNUSED(callName); // AZ_Error may be compiled out entirely in release builds.

                Statement::SqlStatus result = statement->Step();

                TEntry entry;
                auto boundColumns = entry.GetColumns();

                bool validResult = result == Statement::SqlDone;
                while (result == Statement::SqlOK)
                {
                    if (!boundColumns.Fetch(statement))
                    {
                        return false;
                    }

                    if (handler(entry))
                    {
                        result = statement->Step();
                    }
                    else
                    {
                        result = Statement::SqlDone;
                    }
                    validResult = true;
                }

                if (result == Statement::SqlError)
                {
                    AZ_Error(LOG_NAME, false, "Error occurred while stepping %s", callName);
                    return false;
                }

                return validResult;
            }

            bool GetDatabaseInfoResult(const char* callName, Statement* statement, AssetDatabaseConnection::databaseInfoHandler handler)
            {
                return GetResult(callName, statement, handler);
            }

            bool GetScanFolderResult(const char* callName, Statement* statement, AssetDatabaseConnection::scanFolderHandler handler)
            {
                return GetResult(callName, statement, handler);
            }

            bool GetSourceResult(const char* callName, Statement* statement, AssetDatabaseConnection::sourceHandler handler)
            {
                return GetResult(callName, statement, handler);
            }

            bool GetSourceAndScanfolderResult(const char* callName, Statement* statement, AssetDatabaseConnection::combinedSourceScanFolderHandler handler)
            {
                return GetResult(callName, statement, handler);
            }

            bool GetSourceDependencyResult(const char* callName, SQLite::Statement* statement, AssetDatabaseConnection::sourceFileDependencyHandler handler)
            {
                return GetResult(callName, statement, handler);
            }

            bool GetWildcardSourceDependencyResult(const char* callName, SQLite::Statement* statement, AssetDatabaseConnection::sourceFileDependencyHandler handler)
            {
                AZ_UNUSED(callName); // AZ_Error may be compiled out entirely in release builds.

                Statement::SqlStatus result = statement->Step();

                SourceFileDependencyEntry entry;
                int absolutePathMatch;
                auto boundColumns = CombineColumns(entry.GetColumns(), MakeColumns(MakeColumn("AbsolutePath", absolutePathMatch)));

                bool validResult = result == Statement::SqlDone;
                while (result == Statement::SqlOK)
                {
                    if (!boundColumns.Fetch(statement))
                    {
                        return false;
                    }

                    // Its possible for certain relative wildcard dependencies to match an absolute path (ex c%.foo matches c:/bar.foo)
                    // If that happens, skip this result and move on to the next one
                    if(!entry.m_dependsOnSource.IsUuid() && absolutePathMatch == 1 && !AZ::IO::PathView(entry.m_dependsOnSource.GetPath()).IsAbsolute())
                    {
                        result = statement->Step();
                        continue;
                    }

                    if (handler(entry))
                    {
                        result = statement->Step();
                    }
                    else
                    {
                        result = Statement::SqlDone;
                    }
                    validResult = true;
                }

                if (result == Statement::SqlError)
                {
                    AZ_Error(LOG_NAME, false, "Error occurred while stepping %s", callName);
                    return false;
                }

                return validResult;
            }

            bool GetProductDependencyResult(const char* callName, Statement* statement, AssetDatabaseConnection::productDependencyHandler handler)
            {
                return GetResult(callName, statement, handler);
            }

            bool GetProductDependencyAndPathResult(
                [[maybe_unused]] const char* callName,
                Statement* statement,
                AssetDatabaseConnection::productDependencyAndPathHandler handler)
            {
                Statement::SqlStatus result = statement->Step();

                ProductDependencyDatabaseEntry productDependency;

                AZStd::string relativeSearchPath;
                auto boundColumns = CombineColumns(productDependency.GetColumns(), MakeColumns(MakeColumn("search", relativeSearchPath)));

                bool validResult = result == Statement::SqlDone;
                while (result == Statement::SqlOK)
                {
                    if (!boundColumns.Fetch(statement))
                    {
                        return false;
                    }

                    if (handler(productDependency, relativeSearchPath))
                    {
                        result = statement->Step();
                    }
                    else
                    {
                        result = Statement::SqlDone;
                    }
                    validResult = true;
                }

                if (result == Statement::SqlError)
                {
                    AZ_Error(LOG_NAME, false, "Error occurred while stepping %s", callName);
                    return false;
                }

                return validResult;
            }

            bool GetMissingProductDependencyResult(const char* callName, SQLite::Statement* statement, AssetDatabaseConnection::missingProductDependencyHandler handler)
            {
                return GetResult(callName, statement, handler);
            }

            bool GetLegacySubIDsResult(const char* callName, SQLite::Statement* statement, AssetDatabaseConnection::legacySubIDsHandler handler)
            {
                return GetResult(callName, statement, handler);
            }

            bool GetFileResult(const char* callName, SQLite::Statement* statement, AssetDatabaseConnection::fileHandler handler)
            {
                return GetResult(callName, statement, handler);
            }

            bool GetStatResult(const char* callName, SQLite::Statement* statement, AssetDatabaseConnection::statHandler handler)
            {
                return GetResult(callName, statement, handler);
            }

            bool GetJobResultSimple(const char* name, Statement* statement, AssetDatabaseConnection::jobHandler handler)
            {
                return GetJobResult(name, statement, handler);
            }

            bool GetJobResult(const char* callName, Statement* statement, AssetDatabaseConnection::jobHandler handler, AZ::Uuid builderGuid, const char* jobKey, AssetSystem::JobStatus status)
            {
                AZ_UNUSED(callName); // AZ_Error may be compiled out entirely in release builds.

                Statement::SqlStatus result = statement->Step();

                JobDatabaseEntry job;
                auto boundColumns = job.GetColumns();

                bool validResult = result == Statement::SqlDone;
                while (result == Statement::SqlOK)
                {
                    if (!boundColumns.Fetch(statement))
                    {
                        return false;
                    }

                    if (ResultMatchesJobCriteria(jobKey, builderGuid, status, job.m_jobKey, job.m_builderGuid, job.m_status))
                    {
                        if (handler(job))
                        {
                            result = statement->Step();
                        }
                        else
                        {
                            result = Statement::SqlDone;
                        }
                    }
                    else
                    {
                        result = statement->Step();
                    }
                    validResult = true;
                }

                if (result == Statement::SqlError)
                {
                    AZ_Error(LOG_NAME, false, "Error occurred while stepping %s", callName);
                    return false;
                }

                return validResult;
            }

            bool GetProductResultSimple(const char* name, Statement* statement, AssetDatabaseConnection::productHandler handler)
            {
                return GetProductResult(name, statement, handler);
            }

            bool GetProductResult(const char* callName, Statement* statement, AssetDatabaseConnection::productHandler handler, AZ::Uuid builderGuid, const char* jobKey, AssetSystem::JobStatus status)
            {
                AZ_UNUSED(callName); // AZ_Error may be compiled out entirely in release builds.
                Statement::SqlStatus result = statement->Step();

                ProductDatabaseEntry product;
                AZStd::string savedJobKey;
                AZ::Uuid savedBuilderGuid;
                int savedJobStatus;

                auto productColumns = product.GetColumns();
                auto jobKeyColumn = MakeColumn("JobKey", savedJobKey);
                auto builderGuidColumn = MakeColumn("BuilderGuid", savedBuilderGuid);
                auto statusColumn = MakeColumn("Status", savedJobStatus);

                bool validResult = result == Statement::SqlDone;
                while (result == Statement::SqlOK)
                {
                    if ((jobKey && !jobKeyColumn.Fetch(statement))
                        || (!builderGuid.IsNull() && !builderGuidColumn.Fetch(statement))
                        || (status != AssetSystem::JobStatus::Any && !statusColumn.Fetch(statement)))
                    {
                        return false;
                    }

                    if (ResultMatchesJobCriteria(jobKey, builderGuid, status, savedJobKey, savedBuilderGuid, static_cast<AssetSystem::JobStatus>(savedJobStatus)))
                    {
                        if (!productColumns.Fetch(statement))
                        {
                            return false;
                        }

                        if (handler(product))
                        {
                            result = statement->Step();
                        }
                        else
                        {
                            result = Statement::SqlDone;
                        }
                    }
                    else
                    {
                        result = statement->Step();
                    }
                    validResult = true;
                }

                if (result == Statement::SqlError)
                {
                    AZ_Error(LOG_NAME, false, "Error occurred while stepping %s", callName);
                    return false;
                }

                return validResult;
            }

            bool GetCombinedDependencyResult(const char* callName, SQLite::Statement* statement, AssetDatabaseConnection::combinedProductDependencyHandler handler)
            {
                AZ_UNUSED(callName); // AZ_Error may be compiled out entirely in release builds.

                Statement::SqlStatus result = statement->Step();

                ProductDependencyDatabaseEntry entry;
                AZ::Uuid sourceGuid;
                AZ::s32 subId;

                auto boundColumns = entry.GetColumns();
                auto guidColumn = MakeColumn("SourceGuid", sourceGuid);
                auto subIdColumn = MakeColumn("SubID", subId);

                bool validResult = result == Statement::SqlDone;
                while (result == Statement::SqlOK)
                {
                    if (!boundColumns.Fetch(statement) || !guidColumn.Fetch(statement) || !subIdColumn.Fetch(statement))
                    {
                        return false;
                    }

                    AZ::Data::AssetId assetId(sourceGuid, subId);

                    if (handler(assetId, entry))
                    {
                        result = statement->Step();
                    }
                    else
                    {
                        result = Statement::SqlDone;
                    }
                    validResult = true;
                }

                if (result == Statement::SqlError)
                {
                    AZ_Error(LOG_NAME, false, "Error occurred while stepping %s", callName);
                    return false;
                }

                return validResult;
            }
        } // namespace Internal
    } // namespace AssetDatabase
} // namespace AZToolsFramework
