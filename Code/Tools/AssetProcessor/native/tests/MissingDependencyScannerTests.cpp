/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <native/tests/AssetProcessorTest.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzToolsFramework/API/AssetDatabaseBus.h>
#include <native/utilities/MissingDependencyScanner.h>
#include <AssetDatabase/AssetDatabase.h>

namespace AssetProcessor
{

    class MissingDependencyScanner_Test
        : public MissingDependencyScanner
    {
    public:
        AZStd::unordered_map<AZStd::string, AZStd::vector<AZStd::string>>& GetDependenciesRulesMap()
        {
            return m_dependenciesRulesMap;
        }
    };
    class MissingDependencyScannerTestsMockDatabaseLocationListener : public AzToolsFramework::AssetDatabase::AssetDatabaseRequests::Bus::Handler
    {
    public:
        MOCK_METHOD1(GetAssetDatabaseLocation, bool(AZStd::string&));
    };

    class MissingDependencyScannerTest
        : public AssetProcessorTest
    {
    public:
        MissingDependencyScannerTest()
        {

        }

    protected:
        void SetUp() override
        {
            using namespace testing;
            using ::testing::NiceMock;

            AssetProcessorTest::SetUp();

            m_errorAbsorber = nullptr;

            m_data = AZStd::make_unique<StaticData>();

            QDir tempPath(m_data->m_tempDir.path());

            m_data->m_databaseLocationListener.BusConnect();

            m_data->m_databaseLocation = tempPath.absoluteFilePath("test_database.sqlite").toUtf8().constData();

            ON_CALL(m_data->m_databaseLocationListener, GetAssetDatabaseLocation(_))
                .WillByDefault(
                    DoAll( // set the 0th argument ref (string) to the database location and return true.
                        SetArgReferee<0>(m_data->m_databaseLocation),
                        Return(true)));

            m_data->m_dbConn = AZStd::shared_ptr<AssetDatabaseConnection>(aznew AssetDatabaseConnection());
            m_data->m_dbConn->OpenDatabase();

            m_data->m_scopedDir.Setup(tempPath.absolutePath());
        }

        void TearDown() override
        {
            m_data = nullptr;

            AssetProcessorTest::TearDown();
        }

        AZ::Outcome<AZ::s64, AZStd::string> CreateScanFolder(const AZStd::string& scanFolderName, const AZStd::string& scanFolderPath)
        {
            AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry scanFolder;
            scanFolder.m_displayName = scanFolderName;
            scanFolder.m_portableKey = scanFolderName;
            scanFolder.m_scanFolder = scanFolderPath;
            if (!m_data->m_dbConn->SetScanFolder(scanFolder))
            {
                return AZ::Failure(AZStd::string::format("Could not set create scan folder %s", scanFolderName.c_str()));
            }
            return AZ::Success(scanFolder.m_scanFolderID);
        }

        struct SourceAndProductInfo
        {
            AZ::Uuid m_uuid;
            AZ::s64 m_productId;
        };
        AZ::Outcome<SourceAndProductInfo, AZStd::string> CreateSourceAndProductAsset(AZ::s64 scanFolderPK, const AZStd::string& sourceName, const AZStd::string& platform, const AZStd::string& productName)
        {
            using namespace AzToolsFramework::AssetDatabase;
            SourceDatabaseEntry sourceEntry;
            sourceEntry.m_sourceName = sourceName;
            sourceEntry.m_sourceGuid = AssetUtilities::CreateSafeSourceUUIDFromName(sourceEntry.m_sourceName.c_str());
            sourceEntry.m_scanFolderPK = scanFolderPK;
            if (!m_data->m_dbConn->SetSource(sourceEntry))
            {
                return AZ::Failure(AZStd::string::format("Could not set source in the asset database for %s", sourceName.c_str()));
            }

            SourceAndProductInfo result;
            result.m_uuid = sourceEntry.m_sourceGuid;

            JobDatabaseEntry jobEntry;
            jobEntry.m_sourcePK = sourceEntry.m_sourceID;
            jobEntry.m_platform = platform;
            jobEntry.m_jobRunKey = 1;
            if(!m_data->m_dbConn->SetJob(jobEntry))
            {
                return AZ::Failure(AZStd::string::format("Could not set job in the asset database for %s", sourceName.c_str()));
            }

            ProductDatabaseEntry productEntry;
            productEntry.m_jobPK = jobEntry.m_jobID;
            productEntry.m_productName = AZStd::string::format("%s/%s", platform.c_str(), productName.c_str());
            if(!m_data->m_dbConn->SetProduct(productEntry))
            {
                return AZ::Failure(AZStd::string::format("Could not set product in the asset database for %s", sourceName.c_str()));
            }

            result.m_productId = productEntry.m_productID;
            return AZ::Success(result);
        }

        void CreateAndValidateMissingProductDependency(const AZStd::string& missingProductName)
        {
            using namespace AzToolsFramework::AssetDatabase;

            QDir tempPath(m_data->m_tempDir.path());
            QString testFilePath = tempPath.absoluteFilePath("subfolder1/assetProcessorManagerTest.txt");

            AZStd::string testPlatform("pc");
            AZStd::string missingProductPath(AZStd::string::format("test/%s", missingProductName.c_str()));
            ASSERT_TRUE(UnitTestUtils::CreateDummyFile(testFilePath, missingProductName.c_str()));

            // Create the referenced product
            AZ::Outcome<AZ::s64, AZStd::string> scanResult = CreateScanFolder("Test", tempPath.absoluteFilePath("subfolder1").toUtf8().constData());
            ASSERT_TRUE(scanResult.IsSuccess());
            AZ::s64 scanFolderIndex(scanResult.GetValue());
            AZ::Outcome<SourceAndProductInfo, AZStd::string> firstAsset = CreateSourceAndProductAsset(scanFolderIndex, "tests/1", testPlatform, missingProductPath);
            ASSERT_TRUE(firstAsset.IsSuccess());
            AZ::Uuid actualTestGuid(firstAsset.GetValue().m_uuid);

            // Create the product that references the product above.  This represents the dummy file we created up above
            AZ::Outcome<SourceAndProductInfo, AZStd::string> secondAsset = CreateSourceAndProductAsset(scanFolderIndex, "tests/2", testPlatform, "test/tests/2.product");
            ASSERT_TRUE(secondAsset.IsSuccess());
            AZ::s64 productId = secondAsset.GetValue().m_productId;

            AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer container;

            m_data->m_scanner.ScanFile(testFilePath.toUtf8().constData(), AssetProcessor::MissingDependencyScanner::DefaultMaxScanIteration, productId, container, m_data->m_dbConn, false, [](AZStd::string /*dependencyFile*/) {});

            MissingProductDependencyDatabaseEntryContainer missingDeps;
            ASSERT_TRUE(m_data->m_dbConn->GetMissingProductDependenciesByProductId(productId, missingDeps));

            ASSERT_EQ(missingDeps.size(), 1);
            ASSERT_EQ(missingDeps[0].m_productPK, productId);
            ASSERT_EQ(missingDeps[0].m_dependencySourceGuid, actualTestGuid);
        }

        struct StaticData
        {
            QTemporaryDir m_tempDir;
            AZStd::string m_databaseLocation;
            ::testing::NiceMock<MissingDependencyScannerTestsMockDatabaseLocationListener> m_databaseLocationListener;
            AZStd::shared_ptr<AssetDatabaseConnection> m_dbConn;
            MissingDependencyScanner_Test m_scanner;
            UnitTestUtils::ScopedDir m_scopedDir; // Sets up FileIO instance
        };

        AZStd::unique_ptr<StaticData> m_data;
    };

    TEST_F(MissingDependencyScannerTest, ScanFile_FindsValidReferenceToProduct)
    {
        CreateAndValidateMissingProductDependency("tests/1.product");
    }

    TEST_F(MissingDependencyScannerTest, ScanFile_ValidReferenceToFileWithDash_FindsMissingReference)
    {
        CreateAndValidateMissingProductDependency("tests/1-withdash.product");
    }

    TEST_F(MissingDependencyScannerTest, ScanFile_CPP_File_FindsValidReferenceToProduct)
    {
        using namespace AzToolsFramework::AssetDatabase;

        QDir tempPath(m_data->m_tempDir.path());

        // Create the referenced product
        ScanFolderDatabaseEntry scanFolder;
        scanFolder.m_displayName = "Test";
        scanFolder.m_portableKey = "Test";
        scanFolder.m_scanFolder = tempPath.absoluteFilePath("subfolder1").toUtf8().constData();
        ASSERT_TRUE(m_data->m_dbConn->SetScanFolder(scanFolder));

        SourceDatabaseEntry sourceEntry;
        sourceEntry.m_sourceName = "tests/1.source";
        sourceEntry.m_sourceGuid = AssetUtilities::CreateSafeSourceUUIDFromName(sourceEntry.m_sourceName.c_str());
        sourceEntry.m_scanFolderPK = 1;
        ASSERT_TRUE(m_data->m_dbConn->SetSource(sourceEntry));

        JobDatabaseEntry jobEntry;
        jobEntry.m_sourcePK = sourceEntry.m_sourceID;
        jobEntry.m_platform = "pc";
        jobEntry.m_jobRunKey = 1;
        ASSERT_TRUE(m_data->m_dbConn->SetJob(jobEntry));

        ProductDatabaseEntry productEntry;
        productEntry.m_jobPK = jobEntry.m_jobID;
        productEntry.m_productName = "pc/test/tests/1.product";
        ASSERT_TRUE(m_data->m_dbConn->SetProduct(productEntry));

        AZStd::string productReference("tests/1.product");

        // Create a cpp file that references the product above.
        QString sourceFilePath = tempPath.absoluteFilePath("subfolder1/TestFile.cpp");
        AZStd::string codeSourceCode = AZStd::string::format(R"(#include <Dummy/Dummy.h>;
                                          #define PRODUCT_REFERENCE "%s")", productReference.c_str());
        ASSERT_TRUE(UnitTestUtils::CreateDummyFile(sourceFilePath, codeSourceCode.c_str()));
        AZStd::string productDependency;
        auto missingDependencyCallback = [&](AZStd::string relativeDependencyFilePath)
        {
            productDependency = relativeDependencyFilePath;
        };

        AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer container;
        AZStd::string dependencyToken = "dummy";

        // Since dependency rule map is empty this should show a missing dependency 
        m_data->m_scanner.ScanFile(sourceFilePath.toUtf8().constData(), AssetProcessor::MissingDependencyScanner::DefaultMaxScanIteration, m_data->m_dbConn, dependencyToken, false, missingDependencyCallback);
        ASSERT_EQ(productDependency, productReference);

        productDependency.clear();
        QString anotherSourceFilePath = tempPath.absoluteFilePath("subfolder1/TestFile.cpp");
        codeSourceCode = AZStd::string::format(R"(#include <Dummy/Dummy.h>;
                            AZStd::string filePath("%s")", productReference.c_str());
        ASSERT_TRUE(UnitTestUtils::CreateDummyFile(anotherSourceFilePath, codeSourceCode.c_str()));
        m_data->m_scanner.ScanFile(anotherSourceFilePath.toUtf8().constData(), AssetProcessor::MissingDependencyScanner::DefaultMaxScanIteration, m_data->m_dbConn, dependencyToken, false, missingDependencyCallback);
        ASSERT_EQ(productDependency, productReference);

        AZStd::vector<AZStd::string> rulesMap;
        rulesMap.emplace_back("*.product");
        m_data->m_scanner.GetDependenciesRulesMap()[dependencyToken] = rulesMap;
        productDependency.clear();
        m_data->m_scanner.ScanFile(sourceFilePath.toUtf8().constData(), AssetProcessor::MissingDependencyScanner::DefaultMaxScanIteration, m_data->m_dbConn, dependencyToken, false, missingDependencyCallback);
        ASSERT_TRUE(productDependency.empty());
    }
}
