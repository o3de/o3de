/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/base.h>
#include <AzCore/Component/ComponentApplication.h>
#if !defined(Q_MOC_RUN)
#include <AzCore/UnitTest/TestTypes.h>
#endif
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzToolsFramework/API/AssetDatabaseBus.h>

#include <QCoreApplication>

#include <native/unittests/UnitTestUtils.h> // for UnitTestUtils like CreateDummyFile / AssertAbsorber.
#include <native/tests/MockAssetDatabaseRequestsHandler.h>
#include <native/resourcecompiler/RCBuilder.h>

#include "AssetManager/FileStateCache.h"
#include <tests/UnitTestUtilities.h>

namespace AssetProcessor
{
    using namespace AZ;
    using namespace AZ::Data;
    using namespace testing;
    using namespace UnitTestUtils;
    using namespace UnitTest;
    using namespace AzFramework::AssetSystem;
    using namespace AzToolsFramework::AssetSystem;
    using namespace AzToolsFramework::AssetDatabase;

    class AssetCatalogForUnitTest : public AssetProcessor::AssetCatalog
    {
    public:
        AssetCatalogForUnitTest(QObject* parent, AssetProcessor::PlatformConfiguration* platformConfiguration)
            : AssetCatalog(parent, platformConfiguration) {}

        // prevent automatic save on shutdown, no point in doing that in unit test mode, just wastes time.
        virtual ~AssetCatalogForUnitTest()
        {
            ClearDirtyFlag();
        }
        void ClearDirtyFlag()
        {
            m_catalogIsDirty = false;
        }

        AzFramework::AssetRegistry& GetRegistry(QString platformKey)
        {
            return m_registries[platformKey];
        }

    protected:

    };

    class AssetCatalogTest
        : public ScopedAllocatorSetupFixture
    {
    protected:

        // store all data we create here so that it can be destroyed on shutdown before we remove allocators
        struct DataMembers
        {
            MockAssetDatabaseRequestsHandler m_databaseLocationListener;
            FileStatePassthrough m_fileStateCache;
            QDir m_assetRootSourceDir;
            QDir m_priorAssetRoot;
            AssetDatabaseConnection m_dbConn;
            UnitTestUtils::ScopedDir m_scopedDir;
            PlatformConfiguration m_config;
            AZStd::unique_ptr<AssetCatalogForUnitTest> m_assetCatalog;
            QDir m_cacheRootDir; // where the 'cache' folder lives.
            QString m_gameName;
            AssertAbsorber m_absorber;
            AZStd::string m_databaseLocation;
            QCoreApplication coreApp;
            int argc = 0;
            DataMembers() : coreApp(argc, nullptr)
            {

            }
        };

        // The component application creates and returns a system entity, but doesn't keep track of it
        // It's the responsibility of whatever owns the component application to also track and manage the lifetime
        // of this entity.
        AZ::Entity* m_systemEntity = nullptr;
        DataMembers* m_data = nullptr;
        AZStd::unique_ptr<AZ::ComponentApplication> m_app; // the app is created seperately so that we can control its lifetime.

        void SetUp() override
        {
            m_app.reset(aznew AZ::ComponentApplication());
            AZ::ComponentApplication::Descriptor desc;
            desc.m_useExistingAllocator = true;
            m_systemEntity = m_app->Create(desc);

            m_data = azcreate(DataMembers, ());

            AssetUtilities::ComputeAssetRoot(m_data->m_priorAssetRoot);
            AssetUtilities::ResetAssetRoot();

            m_data->m_assetRootSourceDir = QDir(m_data->m_databaseLocationListener.GetAssetRootDir().c_str());
            m_data->m_scopedDir.Setup(m_data->m_assetRootSourceDir.path());
            m_data->m_gameName = AssetUtilities::ComputeProjectName("AutomatedTesting"); // uses the above file.

            AssetUtilities::ResetAssetRoot();
            QDir newRoot; // throwaway dummy var - we just want to invoke the below function
            AssetUtilities::ComputeAssetRoot(newRoot, &m_data->m_assetRootSourceDir);

            auto settingsRegistry = AZ::SettingsRegistry::Get();
            auto cacheRootKey =
                AZ::SettingsRegistryInterface::FixedValueString(AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey) + "/project_cache_path";
            settingsRegistry->Set(cacheRootKey, m_data->m_assetRootSourceDir.absoluteFilePath("Cache").toUtf8().constData());
            auto projectPathKey =
                AZ::SettingsRegistryInterface::FixedValueString(AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey) + "/project_path";
            AZ::IO::FixedMaxPath enginePath;
            settingsRegistry->Get(enginePath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
            settingsRegistry->Set(projectPathKey, (enginePath / "AutomatedTesting").Native());
            AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddRuntimeFilePaths(*settingsRegistry);
            AssetUtilities::ComputeProjectCacheRoot(m_data->m_cacheRootDir);
            QString normalizedCacheRoot = AssetUtilities::NormalizeDirectoryPath(m_data->m_cacheRootDir.absolutePath());
            m_data->m_cacheRootDir = QDir(normalizedCacheRoot);

            // create the files we'll use for this test:
            QSet<QString> expectedFiles;
            // set up some interesting files:
            expectedFiles << m_data->m_assetRootSourceDir.absoluteFilePath("rootfile2.txt");
            expectedFiles << m_data->m_assetRootSourceDir.absoluteFilePath("subfolder1/rootfile1.txt"); // note:  Must override the actual root file
            expectedFiles << m_data->m_assetRootSourceDir.absoluteFilePath("subfolder1/basefile.txt");
            expectedFiles << m_data->m_assetRootSourceDir.absoluteFilePath("subfolder2/basefile.txt");
            expectedFiles << m_data->m_assetRootSourceDir.absoluteFilePath("subfolder2/aaa/basefile.txt");
            expectedFiles << m_data->m_assetRootSourceDir.absoluteFilePath("subfolder2/aaa/bbb/basefile.txt");
            expectedFiles << m_data->m_assetRootSourceDir.absoluteFilePath("subfolder2/aaa/bbb/ccc/basefile.txt");
            expectedFiles << m_data->m_assetRootSourceDir.absoluteFilePath("subfolder2/aaa/bbb/ccc/ddd/basefile.txt");
            expectedFiles << m_data->m_assetRootSourceDir.absoluteFilePath("subfolder3/BaseFile.txt"); // note the case upper here
            expectedFiles << m_data->m_assetRootSourceDir.absoluteFilePath("subfolder8/a/b/c/test.txt");

            // subfolder3 is not recursive so none of these should show up in any scan or override check
            expectedFiles << m_data->m_assetRootSourceDir.absoluteFilePath("subfolder3/aaa/basefile.txt");
            expectedFiles << m_data->m_assetRootSourceDir.absoluteFilePath("subfolder3/aaa/bbb/basefile.txt");
            expectedFiles << m_data->m_assetRootSourceDir.absoluteFilePath("subfolder3/aaa/bbb/ccc/basefile.txt");

            expectedFiles << m_data->m_assetRootSourceDir.absoluteFilePath("subfolder3/uniquefile.txt"); // only exists in subfolder3
            expectedFiles << m_data->m_assetRootSourceDir.absoluteFilePath("subfolder3/uniquefile.ignore"); // only exists in subfolder3

            expectedFiles << m_data->m_assetRootSourceDir.absoluteFilePath("subfolder3/rootfile3.txt"); // must override rootfile3 in root
            expectedFiles << m_data->m_assetRootSourceDir.absoluteFilePath("rootfile1.txt");
            expectedFiles << m_data->m_assetRootSourceDir.absoluteFilePath("rootfile3.txt");
            expectedFiles << m_data->m_assetRootSourceDir.absoluteFilePath("unrecognised.file"); // a file that should not be recognised
            expectedFiles << m_data->m_assetRootSourceDir.absoluteFilePath("unrecognised2.file"); // a file that should not be recognised
            expectedFiles << m_data->m_assetRootSourceDir.absoluteFilePath("subfolder1/test/test.format"); // a file that should be recognised
            expectedFiles << m_data->m_assetRootSourceDir.absoluteFilePath("test.format"); // a file that should NOT be recognised
            expectedFiles << m_data->m_assetRootSourceDir.absoluteFilePath("subfolder3/somefile.xxx");
            expectedFiles << m_data->m_assetRootSourceDir.absoluteFilePath("subfolder3/savebackup/test.txt");//file that should be excluded
            expectedFiles << m_data->m_assetRootSourceDir.absoluteFilePath("subfolder3/somerandomfile.random");

            for (const QString& expect : expectedFiles)
            {
                CreateDummyFile(expect);
            }

            m_data->m_dbConn.OpenDatabase();

            BuildConfig(m_data->m_assetRootSourceDir, &(m_data->m_dbConn), m_data->m_config);
            m_data->m_assetCatalog.reset(new AssetCatalogForUnitTest(nullptr, &(m_data->m_config)));
        }

        void TearDown() override
        {
            // if you EXPECT warnings/asserts/errors you need to check in your test, and you need to also
            // reset it before returning from your test.
            EXPECT_EQ(m_data->m_absorber.m_numAssertsAbsorbed, 0);
            EXPECT_EQ(m_data->m_absorber.m_numErrorsAbsorbed, 0);
            EXPECT_EQ(m_data->m_absorber.m_numWarningsAbsorbed, 0);
            AssetUtilities::ResetAssetRoot();

            azdestroy(m_data);
            m_systemEntity = nullptr;
            m_app->Destroy();
            m_app.reset();
        }

        // -- utility functions to create default state data --

        // Adds a scan folder to the config and to the database
        void AddScanFolder(ScanFolderInfo scanFolderInfo, PlatformConfiguration& config, AssetDatabaseConnection* dbConn)
        {
            ScanFolderDatabaseEntry newScanFolder(
                scanFolderInfo.ScanPath().toStdString().c_str(),
                scanFolderInfo.GetDisplayName().toStdString().c_str(),
                scanFolderInfo.GetPortableKey().toStdString().c_str(),
                scanFolderInfo.IsRoot());
            dbConn->SetScanFolder(newScanFolder);

            scanFolderInfo.SetScanFolderID(newScanFolder.m_scanFolderID);
            config.AddScanFolder(scanFolderInfo);
        }

        virtual void AddScanFolders(
            const QDir& tempPath, AssetDatabaseConnection* dbConn, PlatformConfiguration& config,
            const AZStd::vector<AssetBuilderSDK::PlatformInfo>& platforms)
        {
            //                                               PATH         DisplayName    PortKey      root    recurse  platforms     order
            AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder4"), "subfolder4", "subfolder4", false, false, platforms, -6), config, dbConn); // subfolder 4 overrides subfolder3
            AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder3"), "subfolder3", "subfolder3", false, false, platforms, -5), config, dbConn); // subfolder 3 overrides subfolder2
            AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder2"), "subfolder2", "subfolder2", false, true, platforms, -2), config, dbConn); // subfolder 2 overrides subfolder1
            AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder1"), "subfolder1", "subfolder1", false, true, platforms, -1), config, dbConn); // subfolder1 overrides root
            AddScanFolder(ScanFolderInfo(tempPath.absolutePath(), "temp", "tempfolder", true, false, platforms, 0), config, dbConn); // add the root
        }

        // build some default configs.
        void BuildConfig(const QDir& tempPath, AssetDatabaseConnection* dbConn, PlatformConfiguration& config)
        {
            config.EnablePlatform({ "pc" ,{ "desktop", "renderer" } }, true);
            config.EnablePlatform({ "android" ,{ "mobile", "renderer" } }, true);
            config.EnablePlatform({ "fandango" ,{ "console", "renderer" } }, false);
            AZStd::vector<AssetBuilderSDK::PlatformInfo> platforms;
            config.PopulatePlatformsForScanFolder(platforms);

            AddScanFolders(tempPath, dbConn, config, platforms);

            config.AddMetaDataType("exportsettings", QString());

            AssetRecognizer rec;

            rec.m_name = "random files";
            rec.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.random", AssetBuilderSDK::AssetBuilderPattern::Wildcard);
            rec.m_platformSpecs.insert({"pc", AssetInternalSpec::Copy});
            config.AddRecognizer(rec);

            const char* builderTxt1Name = "txt files";
            rec.m_name = builderTxt1Name;
            rec.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.txt", AssetBuilderSDK::AssetBuilderPattern::Wildcard);
            rec.m_platformSpecs.insert({"pc", AssetInternalSpec::Copy});
            rec.m_platformSpecs.insert({"android", AssetInternalSpec::Copy});

            config.AddRecognizer(rec);

            // Ignore recognizer
            AssetRecognizer ignore_rec;
            ignore_rec.m_name = "ignore files";
            ignore_rec.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.ignore", AssetBuilderSDK::AssetBuilderPattern::Wildcard);
            ignore_rec.m_platformSpecs.insert({"pc", AssetInternalSpec::Copy});
            ignore_rec.m_platformSpecs.insert({"android", AssetInternalSpec::Skip});
            config.AddRecognizer(ignore_rec);

            ExcludeAssetRecognizer excludeRecogniser;
            excludeRecogniser.m_name = "backup";
            excludeRecogniser.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("(^|.+/)savebackup/.*", AssetBuilderSDK::AssetBuilderPattern::Regex);
            config.AddExcludeRecognizer(excludeRecogniser);
        }

        // Adds a source file and job entry to the database, jobId is output
        bool AddSourceAndJob(const char* scanFolder, const char* sourceRelPath, AssetDatabaseConnection* dbConn, AZ::s64& jobId, AZ::Uuid assetId = AZ::Uuid::CreateRandom())
        {
            ScanFolderDatabaseEntry scanFolderEntry;
            bool result = dbConn->GetScanFolderByPortableKey(scanFolder, scanFolderEntry);

            if (!result)
            {
                return false;
            }

            SourceDatabaseEntry sourceEntry(scanFolderEntry.m_scanFolderID, sourceRelPath, assetId, "fingerprint1");
            dbConn->SetSource(sourceEntry);

            JobDatabaseEntry jobEntry(sourceEntry.m_sourceID, "test", 1234, "pc", assetId, AzToolsFramework::AssetSystem::JobStatus::Completed, 12345);
            result = dbConn->SetJob(jobEntry);

            jobId = jobEntry.m_jobID;
            return result;
        };

        bool AddSourceAndJobForMultiplePlatforms(
            const char* scanFolder,
            const char* sourceRelPath,
            AssetDatabaseConnection* dbConn,
            AZStd::map<AZStd::string, AZ::s64>& platformsToJobIds,
            const AZStd::vector<AZStd::string>& platforms,
            AZ::Uuid assetId = AZ::Uuid::CreateRandom())
        {
            ScanFolderDatabaseEntry scanFolderEntry;
            bool result = dbConn->GetScanFolderByPortableKey(scanFolder, scanFolderEntry);

            if (!result)
            {
                return false;
            }

            SourceDatabaseEntry sourceEntry(scanFolderEntry.m_scanFolderID, sourceRelPath, assetId, "fingerprint1");
            dbConn->SetSource(sourceEntry);

            for (const AZStd::string& platform : platforms)
            {
                JobDatabaseEntry jobEntry(sourceEntry.m_sourceID, "test", 1234, platform.c_str(), assetId, AzToolsFramework::AssetSystem::JobStatus::Completed, 12345);
                result = dbConn->SetJob(jobEntry);
                if (!result)
                {
                    return false;
                }
                platformsToJobIds[platform] = jobEntry.m_jobID;
            }
            return true;
        };

        // Calls the GetRelativeProductPathFromFullSourceOrProductPath function and checks the return results, returning true if it matches both of the expected results
        bool TestGetRelativeProductPath(const QString fileToCheck, bool expectedToFind, AZStd::initializer_list<const char*> expectedPaths)
        {
            bool relPathfound = false;
            AZStd::string relPath;
            AZStd::string fullPath(fileToCheck.toUtf8().constData());

            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(relPathfound, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetRelativeProductPathFromFullSourceOrProductPath, fullPath, relPath);

            if (relPathfound != expectedToFind)
            {
                return false;
            }

            for (auto& path : expectedPaths)
            {
                if (relPath == path)
                {
                    return true;
                }
            }

            return false;
        }

        // Calls the GetFullSourcePathFromRelativeProductPath function and checks the return results, returning true if it matches both of
        // the expected results
        bool TestGetFullSourcePath(const QString& fileToCheck, const QDir& tempPath, bool expectToFind, const char* expectedPath)
        {
            bool fullPathfound = false;
            AZStd::string fullPath;
            AZStd::string relPath(fileToCheck.toUtf8().constData());

            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(fullPathfound, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetFullSourcePathFromRelativeProductPath, relPath, fullPath);

            if (fullPathfound != expectToFind)
            {
                return false;
            }

            QString output(fullPath.c_str());
            output.remove(0, tempPath.path().length() + 1); //adding one for the native separator

            return (output == expectedPath);
        }

        AZ::s64 CreateProductAndGetProductId(AZ::s64 jobId, const AZStd::string& productPath, const AZ::u32 productSubId)
        {
            ProductDatabaseEntry product(
                jobId,
                productSubId,
                m_data->m_cacheRootDir.relativeFilePath(productPath.c_str()).toStdString().c_str(),
                AZ::Data::AssetType::CreateRandom());
            bool result = m_data->m_dbConn.SetProduct(product);
            EXPECT_TRUE(result);
            return product.m_productID;
        }
    };

    class AssetCatalogTestWithProducts
        : public AssetCatalogTest
    {
    public:
        void SetUp() override
        {
            AssetCatalogTest::SetUp();

            // Add a source file with 4 products
            AZ::s64 jobId;
            bool result = AddSourceAndJob("subfolder3", "BaseFile.txt", &(m_data->m_dbConn), jobId);
            EXPECT_TRUE(result);

            AZ::u32 productSubId = 0;
            for (auto& relativeProductPath : { "subfolder3/basefilez.arc2", "subfolder3/basefileaz.azm2", "subfolder3/basefile.arc2", "subfolder3/basefile.azm2" })
            {
                ProductDatabaseEntry newProduct(jobId, productSubId++, m_data->m_cacheRootDir.relativeFilePath(relativeProductPath).toStdString().c_str(), AZ::Data::AssetType::CreateRandom());
                m_data->m_dbConn.SetProduct(newProduct);
            }
        }
    };

    TEST_F(AssetCatalogTest, EmptyConstructors_Sanity)
    {
        // make sure constructors do not crash or misbehave when given empty names
        QString fileToCheck = "";

        // empty requests should generate an assert since it is a programming error to call this API with bad data.
        // however, the app should not crash even if the assert is absorbed.
        GetRelativeProductPathFromFullSourceOrProductPathRequest request(fileToCheck.toUtf8().constData());
        ASSERT_EQ(m_data->m_absorber.m_numAssertsAbsorbed, 1);
        GetFullSourcePathFromRelativeProductPathRequest sourceRequest(fileToCheck.toUtf8().constData());
        ASSERT_EQ(m_data->m_absorber.m_numAssertsAbsorbed, 2);
        // reset the absorber before we leave this assert-test, so that it doesn't cause failure of the test itself
        m_data->m_absorber.Clear();

        ASSERT_TRUE(TestGetRelativeProductPath("", false, { "" }));
        ASSERT_TRUE(TestGetFullSourcePath("", m_data->m_assetRootSourceDir, false, ""));
    }

    TEST_F(AssetCatalogTestWithProducts, GetRelativePath_GivenRootPath_ReturnsFailure)
    {
        // Failure case
#if defined(AZ_PLATFORM_WINDOWS)
        QString fileToCheck = "d:\\test.txt";
#else
        QString fileToCheck = "/test.txt"; // rooted
#endif
        ASSERT_TRUE(TestGetRelativeProductPath(fileToCheck, false, { fileToCheck.toStdString().c_str() }));
    }

    TEST_F(AssetCatalogTestWithProducts, RelativePath)
    {
        // feed it a relative path with a TAB in the front :)
        QString fileToCheck = "\test.txt";
        ASSERT_TRUE(TestGetRelativeProductPath(fileToCheck, true, { "\test.txt" }));
    }

    TEST_F(AssetCatalogTestWithProducts, GetRelativeProductPathFromFullSourceOrProductPath_WithGameName_ReturnsFileInGameFolder)
    {
        // feed it a product path with a platform name, returns it
        QString fileToCheck = m_data->m_cacheRootDir.filePath(QString("pc") + "/aaa/basefile.txt");
        ASSERT_TRUE(TestGetRelativeProductPath(fileToCheck, true, { "aaa/basefile.txt" }));
    }

    TEST_F(AssetCatalogTestWithProducts, GetRelativeProductPathFromFullSourceOrProductPath_WithoutGameName_ReturnsFileInRootFolder)
    {
        // feed it a product path, just the file name since its supposed to be a root file
        QString fileToCheck = m_data->m_cacheRootDir.filePath("pc/basefile.txt");
        ASSERT_TRUE(TestGetRelativeProductPath(fileToCheck, true, { "basefile.txt" }));
    }

    TEST_F(AssetCatalogTestWithProducts, GetRelativeProductPathFromFullSourceOrProductPath_BadCasingInPlatform_ReturnsRelativePath)
    {
        // feed it a product path but with poor casing (test 1:  the pc platform is not matching case)
        QString fileToCheck = m_data->m_cacheRootDir.filePath(QString("Pc") + "/aaa/basefile.txt");
        ASSERT_TRUE(TestGetRelativeProductPath(fileToCheck, true, { "aaa/basefile.txt" }));
    }

    TEST_F(AssetCatalogTestWithProducts, GetRelativeProductPathFromFullSourceOrProductPath_BadCasingInGameName_ReturnsRelativePath)
    {
        //feed it a product path but with poor casing (test 2:  the gameName is not matching case)
        QString fileToCheck = m_data->m_cacheRootDir.filePath(QString("pc") + "/aaa/basefile.txt");
        ASSERT_TRUE(TestGetRelativeProductPath(fileToCheck, true, { "aaa/basefile.txt" }));
    }

    TEST_F(AssetCatalogTestWithProducts, GetRelativeProductPathFromFullSourceOrProductPath_FolderName_ReturnsFolderNameOnly)
    {
        // feed it a product path that resolves to a directory name instead of a file.
        QString fileToCheck = m_data->m_cacheRootDir.filePath(QString("pc") + "/aaa");
        ASSERT_TRUE(TestGetRelativeProductPath(fileToCheck, true, { "aaa" }));
    }

    TEST_F(AssetCatalogTestWithProducts, GetRelativeProductPathFromFullSourceOrProductPath_FolderNameExtraSlash_ReturnsFolderNameOnlyNoExtraSlash)
    {
        //  make sure it doesn't keep any trailing slashes.
        QString fileToCheck = m_data->m_cacheRootDir.filePath(QString("pc") + "/aaa/"); // extra trailing slash
        ASSERT_TRUE(TestGetRelativeProductPath(fileToCheck, true, { "aaa" })); // the API should never result in a trailing slash
    }

    TEST_F(AssetCatalogTestWithProducts, GetRelativeProductPathFromFullSourceOrProductPath_FolderNameExtraWrongWaySlash_ReturnsFolderNameOnlyNoExtraWrongSlash)
    {
        //  make sure it doesn't keep any trailing slashes.
        QString fileToCheck = m_data->m_cacheRootDir.filePath(QString("pc") + "/aaa\\"); // extra trailing wrongway slash
        ASSERT_TRUE(TestGetRelativeProductPath(fileToCheck, true, { "aaa" })); // the API should never result in a trailing slash
    }

    TEST_F(AssetCatalogTestWithProducts, GetRelativeProductPathFromFullSourceOrProductPath_RelativeDirectoryNameWhichDoesNotExist_ReturnsFolderNameOnly)
    {
        QString fileToCheck = m_data->m_cacheRootDir.filePath(QString("pc") + "/nonexistantfolder"); // extra trailing wrongway slash
        ASSERT_TRUE(TestGetRelativeProductPath(fileToCheck, true, { "nonexistantfolder" }));
    }

    TEST_F(AssetCatalogTestWithProducts, GetRelativeProductPathFromFullSourceOrProductPath_RelativeDirectoryNameWhichDoesNotExistWithExtraSlash_ReturnsFolderNameOnly)
    {
        QString fileToCheck = m_data->m_cacheRootDir.filePath(QString("pc") + "/nonexistantfolder/"); // extra trailing wrongway slash
        ASSERT_TRUE(TestGetRelativeProductPath(fileToCheck, true, { "nonexistantfolder" }));
    }

    TEST_F(AssetCatalogTestWithProducts, GetRelativeProductPathFromFullSourceOrProductPath_RelativeDirectoryNameWhichDoesNotExistWithExtraWrongWaySlash_ReturnsFolderNameOnly)
    {
        QString fileToCheck = m_data->m_cacheRootDir.filePath(QString("pc") + "\\nonexistantfolder\\"); // extra trailing wrongway slash
        ASSERT_TRUE(TestGetRelativeProductPath(fileToCheck, true, { "nonexistantfolder" }));
    }

    TEST_F(AssetCatalogTestWithProducts, GetRelativeProductPathFromFullSourceOrProductPath_RelativePathToSourceFile_ReturnsProductFilePath)
    {
        QString fileToCheck = m_data->m_assetRootSourceDir.absoluteFilePath("subfolder3/BaseFile.txt");
        ASSERT_TRUE(TestGetRelativeProductPath(fileToCheck, true, { "basefilez.arc2", "basefileaz.azm2",
            "basefile.arc2", "basefile.azm2" }));
    }

    TEST_F(AssetCatalogTestWithProducts, GetRelativeProductPathFromFullSourceOrProductPath_RelativePathToSourceFile_BadCasing_ReturnsProductFilePath)
    {
        // note that the casing of the source file is not correct.  It must still work.
        QString fileToCheck = m_data->m_assetRootSourceDir.absoluteFilePath("subfolder2/aaa/basefile.txt");
        ASSERT_TRUE(TestGetRelativeProductPath(fileToCheck, true, { "aaa/basefile.txt" }));
    }

    struct MockConnection : AssetProcessor::ConnectionBus::Handler
    {
        MockConnection(int connectionId)
        {
            BusConnect(connectionId);
        }

        ~MockConnection()
        {
            BusDisconnect();
        }

        virtual size_t Send([[maybe_unused]] unsigned int serial, const AzFramework::AssetSystem::BaseAssetProcessorMessage& message)
        {
            auto* bulkMessage = azrtti_cast<const BulkAssetNotificationMessage*>(&message);

            EXPECT_TRUE(bulkMessage);
            EXPECT_EQ(bulkMessage->m_type, AssetNotificationMessage::AssetChanged);
            EXPECT_GT(bulkMessage->m_messages.size(), 0);
            m_messages += bulkMessage->m_messages.size();

            return sizeof(message);
        }
        virtual size_t SendRaw(unsigned int /*type*/, unsigned int /*serial*/, const QByteArray& /*data*/)
        {
            GTEST_NONFATAL_FAILURE_("Not supported");
            return 0;
        }
        virtual size_t SendPerPlatform(
            unsigned int /*serial*/, const AzFramework::AssetSystem::BaseAssetProcessorMessage& /*message*/, const QString& /*platform*/)
        {
            GTEST_NONFATAL_FAILURE_("Not supported");
            return 0;
        }
        virtual size_t SendRawPerPlatform(
            unsigned int /*type*/, unsigned int /*serial*/, const QByteArray& /*data*/, const QString& /*platform*/)
        {
            GTEST_NONFATAL_FAILURE_("Not supported");
            return 0;
        }

        virtual unsigned int SendRequest(
            const AzFramework::AssetSystem::BaseAssetProcessorMessage& /*message*/, const ResponseCallback& /*callback*/)
        {
            GTEST_NONFATAL_FAILURE_("Not supported");
            return 0;
        }
        virtual size_t SendResponse(unsigned int /*serial*/, const AzFramework::AssetSystem::BaseAssetProcessorMessage& /*message*/)
        {
            GTEST_NONFATAL_FAILURE_("Not supported");
            return 0;
        }
        virtual void RemoveResponseHandler(unsigned int /*serial*/)
        {
            GTEST_NONFATAL_FAILURE_("Not supported");
        }

        size_t m_messages = 0;
    };

    TEST_F(AssetCatalogTestWithProducts, SendAssetUpdateOnConnect)
    {
        static constexpr int ConnId = 1;

        AssetNotificationMessage message;
        message.m_type = AssetNotificationMessage::AssetChanged;
        message.m_data = "filea.png";
        message.m_assetId = AZ::Data::AssetId("{4DBBC5A7-ACEE-4084-A435-9CA8AA05B01B}");
        message.m_assetType = AZ::Data::AssetType("{01E432B8-4252-40F5-86CC-4CB554004C49}");
        message.m_platform = "pc";
        message.m_sizeBytes = 10;

        // Add 2 assets to the catalog
        m_data->m_assetCatalog->OnAssetMessage(message);

        message.m_data = "fileb.png";
        message.m_assetId = AZ::Data::AssetId("{29AA7E27-4A80-4443-8DFD-6FC459833BD2}");

        m_data->m_assetCatalog->OnAssetMessage(message);

        // Simulate a connection afterwards
        MockConnection mockConnection(ConnId);
        MockConnection android(ConnId + 1);

        EXPECT_EQ(mockConnection.m_messages, 0);
        EXPECT_EQ(android.m_messages, 0);

        m_data->m_assetCatalog->OnConnect(ConnId, { "pc" });

        // Should recieve both asset messages
        EXPECT_EQ(mockConnection.m_messages, 2);

        m_data->m_assetCatalog->OnConnect(ConnId + 1, { "android" });

        EXPECT_EQ(android.m_messages, 0); // No assets for the android platform
        EXPECT_EQ(mockConnection.m_messages, 2); // No extra messages for the pc platform
    }

    class AssetCatalogTestRelativeSourcePath : public AssetCatalogTest
    {
    public:
        QDir GetRoot()
        {
            // Return an OS-friendly absolute root directory for our tests ("C:/sourceRoot" or "/sourceRoot").  It doesn't
            // need to exist, it just needs to be an absolute path.
            return QDir::root().filePath("sourceRoot");
        }

        // Set up custom scan folders for the "relative source path" tests, so that we can try out specific combinations of watch folders
        void AddScanFolders(
            [[maybe_unused]] const QDir& tempPath, AssetDatabaseConnection* dbConn, PlatformConfiguration& config,
            const AZStd::vector<AssetBuilderSDK::PlatformInfo>& platforms) override
        {
            QDir root = GetRoot();

            // This will set up the following watch folders, in highest to lowest priority:

            // /sourceRoot/recurseNested/nested (recurse)
            // /sourceRoot/noRecurse            (no recurse)
            // /sourceRoot/recurseNotNested     (recurse)
            // /sourceRoot/recurseNested        (recurse)

            AddScanFolder(
                ScanFolderInfo(root.filePath("recurseNested/nested"), "nested", "nested", false, true, platforms, -4), config, dbConn);
            AddScanFolder(
                ScanFolderInfo(root.filePath("noRecurse"), "noRecurse", "noRecurse", false, false, platforms, -3), config, dbConn);
            AddScanFolder(
                ScanFolderInfo(root.filePath("recurseNotNested"), "recurseNotNested", "recurseNotNested", false, true, platforms, -2),
                config, dbConn);
            AddScanFolder(
                ScanFolderInfo(root.filePath("recurseNested"), "recurseNested", "recurseNested", false, true, platforms, -1),
                config, dbConn);
        }

        // Calls the GenerateRelativeSourcePath function and validates that the results match the expected inputs.
        void TestGetRelativeSourcePath(
            const AZStd::string& sourcePath, bool expectedToFind, const AZStd::string& expectedPath, const AZStd::string& expectedRoot)
        {
            bool relPathFound = false;
            AZStd::string relPath;
            AZStd::string rootFolder;

            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                relPathFound, &AzToolsFramework::AssetSystem::AssetSystemRequest::GenerateRelativeSourcePath, sourcePath,
                relPath, rootFolder);

            EXPECT_EQ(relPathFound, expectedToFind);
            EXPECT_EQ(relPath, expectedPath);
            EXPECT_EQ(rootFolder, expectedRoot);
        }
    };

    TEST_F(AssetCatalogTestRelativeSourcePath, GenerateRelativeSourcePath_EmptySourcePath_ReturnsNoMatch)
    {
        // Test passes in an empty source path, which shouldn't produce a valid result.
        // Input:  empty source path
        // Output: empty, not found result
        TestGetRelativeSourcePath("", false, "", "");
    }

    TEST_F(AssetCatalogTestRelativeSourcePath, GenerateRelativeSourcePath_AbsolutePathOutsideWatchFolders_ReturnsNoMatch)
    {
        // Test passes in an invalid absolute source path, which shouldn't produce a valid result.
        // Input:  "/sourceRoot/noWatchFolder/test.txt"
        // Output: not found result, which also returns the input as the relative file name
        QDir watchFolder = GetRoot().filePath("noWatchFolder/");
        QString fileToCheck = watchFolder.filePath("test.txt");

        TestGetRelativeSourcePath(fileToCheck.toUtf8().constData(), false, fileToCheck.toUtf8().constData(), "");
    }

    TEST_F(AssetCatalogTestRelativeSourcePath, GenerateRelativeSourcePath_AbsolutePathUnderWatchFolder_ReturnsRelativePath)
    {
        // Test passes in a valid absolute source path, which should produce a valid relative path
        // Input:  "/sourceRoot/noRecurse/test.txt"
        // Output: "test.txt" in folder "/sourceRoot/noRecurse/"
        QDir watchFolder = GetRoot().filePath("noRecurse/");
        QString fileToCheck = watchFolder.filePath("test.txt");

        TestGetRelativeSourcePath(fileToCheck.toUtf8().constData(), true, "test.txt", watchFolder.path().toUtf8().constData());
    }

    TEST_F(AssetCatalogTestRelativeSourcePath, GenerateRelativeSourcePath_AbsolutePathUnderNestedWatchFolders_ReturnsRelativePath)
    {
        // Test passes in a valid absolute source path that matches a watch folder and a nested watch folder.
        // The output relative path should match the nested folder, because the nested folder has a higher priority registered with the AP.
        // Input:  "/sourceRoot/recurseNested/nested/test.txt"
        // Output: "test.txt" in folder "/sourceRoot/recurseNested/nested/"
        QDir watchFolder = GetRoot().filePath("recurseNested/nested/");
        QString fileToCheck = watchFolder.filePath("test.txt");

        TestGetRelativeSourcePath(fileToCheck.toUtf8().constData(), true, "test.txt", watchFolder.path().toUtf8().constData());
    }

    TEST_F(AssetCatalogTestRelativeSourcePath, GenerateRelativeSourcePath_BareFileNameValidInWatchFolder_ReturnsHighestPriorityWatchFolder)
    {
        // Test passes in a simple file name.  The output should be relative to the highest-priority watch folder.
        // Input:  "test.txt"
        // Output: "test.txt" in folder "/sourceRoot/recurseNested/nested/"
        QDir watchFolder = GetRoot().filePath("recurseNested/nested/");

        TestGetRelativeSourcePath("test.txt", true, "test.txt", watchFolder.path().toUtf8().constData());
    }

    TEST_F(AssetCatalogTestRelativeSourcePath, GenerateRelativeSourcePath_RelativePathValidInWatchFolder_ReturnsHighestPriorityWatchFolder)
    {
        // Test passes in a relative path.  The output should preserve the relative path, but list it as relative to the highest-priority
        // watch folder.
        // Input:  "a/b/c/test.txt"
        // Output: "a/b/c/test.txt" in folder "/sourceRoot/recurseNested/nested/"
        QDir watchFolder = GetRoot().filePath("recurseNested/nested/");

        TestGetRelativeSourcePath("a/b/c/test.txt", true, "a/b/c/test.txt", watchFolder.path().toUtf8().constData());
    }

    TEST_F(AssetCatalogTestRelativeSourcePath, GenerateRelativeSourcePath_RelativePathNotInWatchFolder_ReturnsNoMatch)
    {
        // Test passes in a relative path that "backs up" two directories.  This will be invalid, because no matter which watch directory
        // we start at, the result will be outside of any watch directory.
        // Input:  "../../test.txt"
        // Output: not found result, which also returns the input as the relative file name
        TestGetRelativeSourcePath("../../test.txt", false, "../../test.txt", "");
    }

    TEST_F(AssetCatalogTestRelativeSourcePath, GenerateRelativeSourcePath_RelativePathValidFromNestedWatchFolder_ReturnsOuterFolder)
    {
        // Test passes in a relative path that "backs up" one directory.  This will produce a valid result, because we can back up from
        // the "recurseNested/nested/" watch folder to "recurseNested", which is also a valid watch folder.
        // Input:  "../test.txt"
        // Output: "test.txt" in folder "/sourceRoot/recurseNested"
        QDir watchFolder = GetRoot().filePath("recurseNested/");
        TestGetRelativeSourcePath("../test.txt", true, "test.txt", watchFolder.path().toUtf8().constData());
    }

    TEST_F(AssetCatalogTestRelativeSourcePath, GenerateRelativeSourcePath_RelativePathMovesToParentWatchFolder_ReturnsOuterFolder)
    {
        // Test passes in a relative path that backs up one directory and then forward into a directory.  This will produce a valid
        // result, because it can validly start in the highest-priority watch folder (recurseNested/nested), move back one into the
        // outer watch folder (recurseNested), and then have a subdirectory within it.
        // Note that it would also be valid to move from recurseNested to recurseNotNested, but that won't be the result of this test
        // because that's a lower-priority match.
        // Input:  "../recurseNotNested/test.txt"
        // Output: "recurseNotNested/test.txt" in folder "/sourceRoot/recurseNested/"
        QDir watchFolder = GetRoot().filePath("recurseNested/");

        TestGetRelativeSourcePath("../recurseNotNested/test.txt", true, "recurseNotNested/test.txt", watchFolder.path().toUtf8().constData());
    }

    TEST_F(AssetCatalogTestRelativeSourcePath, GenerateRelativeSourcePath_RelativePathMovesToSiblingWatchFolder_ReturnsSiblingFolder)
    {
        // Test passes in a relative path that backs up two directories and then forward into a directory.  This will produce a valid
        // result, because it can validly start in the recurseNested/nested folder, move back two folders, then forward into the sibling
        // recurseNotNested folder.  The result will be a relative path to the sibling folder.
        // Input:  "../../recurseNotNested/test.txt"
        // Output: "test.txt" in folder "/sourceRoot/recurseNotNested/"
        QDir watchFolder = GetRoot().filePath("recurseNotNested/");

        TestGetRelativeSourcePath("../../recurseNotNested/test.txt", true, "test.txt", watchFolder.path().toUtf8().constData());
    }

    TEST_F(AssetCatalogTestRelativeSourcePath, GenerateRelativeSourcePath_RelativePathBacksOutOfWatchFolder_ReturnsNoMatch)
    {
        // Test passes in a relative path that adds a directory, then "backs up" three directories.  This will be invalid, because no
        // matter which watch directory we start at, the result will be outside of any watch directory.
        // Input:  "../test.txt"
        // Output: "test.txt" in folder "/sourceRoot/recurseNested"
        TestGetRelativeSourcePath("a/../../../test.txt", false, "a/../../../test.txt", "");
    }

    class AssetCatalogTest_GetFullSourcePath
        : public AssetCatalogTest
    {
    public:
        void SetUp() override
        {
            AssetCatalogTest::SetUp();

            // ----- Test the ProcessGetFullAssetPath function on product files
            {
                QStringList pcouts;
                pcouts.push_back(m_data->m_cacheRootDir.filePath(QString("pc") + "/subfolder3/randomfileoutput.random"));
                pcouts.push_back(m_data->m_cacheRootDir.filePath(QString("pc") + "/subfolder3/randomfileoutput.random1"));
                pcouts.push_back(m_data->m_cacheRootDir.filePath(QString("pc") + "/subfolder3/randomfileoutput.random2"));

                AZ::s64 jobId;
                ASSERT_TRUE(AddSourceAndJob("subfolder3", "somerandomfile.random", &(m_data->m_dbConn), jobId));

                AZ::u32 productSubID = 0;
                for (auto& product : pcouts)
                {
                    ProductDatabaseEntry newProduct(jobId, productSubID++, m_data->m_cacheRootDir.relativeFilePath(product).toStdString().c_str(), AZ::Data::AssetType::CreateRandom());
                    ASSERT_TRUE(m_data->m_dbConn.SetProduct(newProduct));
                }
            }
        }
    };

    TEST_F(AssetCatalogTest_GetFullSourcePath, NormalUsage_ReturnsAbsolutePathToSource)
    {
        // feed it an relative product, and expect a full, absolute source file path in return.
        QString fileToCheck = "subfolder3/randomfileoutput.random1";
        EXPECT_TRUE(TestGetFullSourcePath(fileToCheck, m_data->m_assetRootSourceDir, true, "subfolder3/somerandomfile.random"));
    }

    TEST_F(AssetCatalogTest_GetFullSourcePath, SecondProduct_ReturnsAbsolutePathToSource)
    {
        // feed it another relative product from the same source file, expect the same source.
        QString fileToCheck = "subfolder3/randomfileoutput.random2";
        EXPECT_TRUE(TestGetFullSourcePath(fileToCheck, m_data->m_assetRootSourceDir, true, "subfolder3/somerandomfile.random"));
    }

    TEST_F(AssetCatalogTest_GetFullSourcePath, IncorrectSeperators_ReturnsAbsolutePathToSource)
    {
        //feed it the same relative product with different separators
        QString fileToCheck = "subfolder3\\randomfileoutput.random2";
        EXPECT_TRUE(TestGetFullSourcePath(fileToCheck, m_data->m_assetRootSourceDir, true, "subfolder3/somerandomfile.random"));
    }

    TEST_F(AssetCatalogTest_GetFullSourcePath, SourcePath_ReturnsSourcePath)
    {
        // feed it a full path to a source file, we expect that path back
        QString fileToCheck = m_data->m_assetRootSourceDir.filePath("somefolder/somefile.txt");
        EXPECT_TRUE(TestGetFullSourcePath(fileToCheck, m_data->m_assetRootSourceDir, true, "somefolder/somefile.txt"));
    }

    TEST_F(AssetCatalogTest_GetFullSourcePath, AliasedCachePath_ReturnsAbsolutePathToSource)
    {
        //feed it a path with alias and asset id
        QString fileToCheck = "@products@/subfolder3/randomfileoutput.random1";
        EXPECT_TRUE(TestGetFullSourcePath(fileToCheck, m_data->m_assetRootSourceDir, true, "subfolder3/somerandomfile.random"));
    }

    TEST_F(AssetCatalogTest_GetFullSourcePath, InvalidAlias_ReturnsAbsolutePathToSource)
    {
        //feed it a path with some random alias and asset id
        QString fileToCheck = "@somerandomalias@/subfolder3/randomfileoutput.random1";
        EXPECT_TRUE(TestGetFullSourcePath(fileToCheck, m_data->m_assetRootSourceDir, true, "subfolder3/somerandomfile.random"));
    }

    TEST_F(AssetCatalogTest_GetFullSourcePath, InvalidAliasMissingSeperator_ReturnsAbsolutePathToSource)
    {
        //feed it a path with some random alias and asset id but no separator
        QString fileToCheck = "@somerandomalias@subfolder3/randomfileoutput.random1";
        EXPECT_TRUE(TestGetFullSourcePath(fileToCheck, m_data->m_assetRootSourceDir, true, "subfolder3/somerandomfile.random"));
    }

    TEST_F(AssetCatalogTest_GetFullSourcePath, InvalidSourcePathContainingCacheAlias_ReturnsAbsolutePathToSource)
    {
        //feed it a path with alias and input name
        QString fileToCheck = "@products@/somerandomfile.random";
        EXPECT_TRUE(TestGetFullSourcePath(fileToCheck, m_data->m_assetRootSourceDir, true, "subfolder3/somerandomfile.random"));
    }

    TEST_F(AssetCatalogTest_GetFullSourcePath, AbsolutePathToCache_ReturnsAbsolutePathToSource)
    {
        //feed it an absolute path with cacheroot
        QString fileToCheck = m_data->m_cacheRootDir.filePath(QString("pc") + "/subfolder3/randomfileoutput.random1");
        EXPECT_TRUE(TestGetFullSourcePath(fileToCheck, m_data->m_assetRootSourceDir, true, "subfolder3/somerandomfile.random"));
    }

    TEST_F(AssetCatalogTest_GetFullSourcePath, ProductNameIncludingPlatformAndGameName_ReturnsAbsolutePathToSource)
    {
        //feed it a productName directly
        QString fileToCheck = QString("pc") + "/subfolder3/randomfileoutput.random1";
        EXPECT_TRUE(TestGetFullSourcePath(fileToCheck, m_data->m_assetRootSourceDir, true, "subfolder3/somerandomfile.random"));
    }

    class AssetCatalogTest_AssetInfo
        : public AssetCatalogTest
    {
    public:

        struct AssetCatalogTest_AssetInfo_DataMembers
        {
            AssetId m_assetA = AssetId(Uuid::CreateRandom(), 0);
            Uuid m_assetALegacyUuid = Uuid::CreateRandom();
            AssetType m_assetAType = AssetType::CreateRandom();
            AZStd::string m_assetAFileFilter  = "*.source";
            AZStd::string m_subfolder1AbsolutePath;
            AZStd::string m_assetASourceRelPath = "assetA.source";
            AZStd::string m_assetAProductRelPath = "assetA.product";
            AZStd::string m_assetAFullPath;
            AZStd::string m_assetAProductFullPath;
            AZStd::string m_assetTestString    = "Its the Asset A";
            AZStd::string m_productTestString  = "Its a product A";
        };

        AssetCatalogTest_AssetInfo_DataMembers* m_customDataMembers = nullptr;

        void SetUp() override
        {
            AssetCatalogTest::SetUp();
            m_customDataMembers = azcreate(AssetCatalogTest_AssetInfo_DataMembers, ());
            m_customDataMembers->m_subfolder1AbsolutePath = m_data->m_assetRootSourceDir.absoluteFilePath("subfolder1").toStdString().c_str();

            AzFramework::StringFunc::Path::Join(m_customDataMembers->m_subfolder1AbsolutePath.c_str(), m_customDataMembers->m_assetASourceRelPath.c_str(), m_customDataMembers->m_assetAFullPath);
            CreateDummyFile(QString::fromUtf8(m_customDataMembers->m_assetAFullPath.c_str()), m_customDataMembers->m_assetTestString.c_str());

            AzFramework::StringFunc::Path::Join(m_data->m_cacheRootDir.absolutePath().toUtf8().constData(), m_customDataMembers->m_assetAProductRelPath.c_str(), m_customDataMembers->m_assetAProductFullPath);
            CreateDummyFile(QString::fromUtf8(m_customDataMembers->m_assetAProductFullPath.c_str()), m_customDataMembers->m_productTestString.c_str());
        }

        bool GetAssetInfoById(bool expectedResult, AZStd::string expectedRelPath, AZStd::string expectedRootPath, AssetType assetType)
        {
            bool result = false;
            AssetInfo assetInfo;
            AZStd::string rootPath;
            const AZStd::string platformName = ""; // Empty for default
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(result, &AssetSystemRequest::GetAssetInfoById, m_customDataMembers->m_assetA, assetType, platformName, assetInfo, rootPath);

            if (result != expectedResult)
            {
                return false;
            }

            if (expectedResult)
            {
                EXPECT_EQ(assetInfo.m_assetId, m_customDataMembers->m_assetA);
                EXPECT_EQ(assetInfo.m_assetType, m_customDataMembers->m_assetAType);
                EXPECT_EQ(assetInfo.m_relativePath, expectedRelPath);
                EXPECT_EQ(assetInfo.m_sizeBytes, m_customDataMembers->m_assetTestString.size());
                EXPECT_EQ(rootPath, expectedRootPath);

                return (assetInfo.m_assetId == m_customDataMembers->m_assetA)
                    && (assetInfo.m_assetType == m_customDataMembers->m_assetAType)
                    && (assetInfo.m_relativePath == expectedRelPath)
                    && (assetInfo.m_sizeBytes == m_customDataMembers->m_assetTestString.size())
                    && (rootPath == expectedRootPath);
            }

            return true;
        };

        bool GetAssetInfoByIdPair(bool expectedResult, AZStd::string expectedRelPath, AZStd::string expectedRootPath)
        {
            // First test without providing the assetType
            bool result = GetAssetInfoById(expectedResult, expectedRelPath, expectedRootPath, AssetType::CreateNull());

            // If successful, test again, this time providing the assetType
            if (result)
            {
                result = GetAssetInfoById(expectedResult, expectedRelPath, expectedRootPath, m_customDataMembers->m_assetAType);
            }

            return result;
        };

        bool GetSourceInfoBySourcePath(
            bool expectedResult,
            const AZStd::string& sourcePath,
            const AZ::Uuid& expectedUuid,
            const AZStd::string& expectedRelPath,
            const AZStd::string& expectedRootPath,
            const AZ::Data::AssetType& expectedAssetType = AZ::Data::s_invalidAssetType) // sources will have an asset type if registered using AzToolsFramework::ToolsAssetSystemRequests::RegisterSourceAssetType
        {
            bool result = false;
            AssetInfo assetInfo;
            AZStd::string rootPath;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(result, &AssetSystemRequest::GetSourceInfoBySourcePath, sourcePath.c_str(), assetInfo, rootPath);

            if (result != expectedResult)
            {
                return false;
            }

            if (expectedResult)
            {
                return (assetInfo.m_assetId == expectedUuid)
                    && (assetInfo.m_assetType == expectedAssetType)
                    && (assetInfo.m_relativePath == expectedRelPath)
                    && (assetInfo.m_sizeBytes == m_customDataMembers->m_assetTestString.size())
                    && (rootPath == expectedRootPath);
            }

            return true;
        };

        void TearDown() override
        {
            azdestroy(m_customDataMembers);
            AssetCatalogTest::TearDown();
        }

    };

    TEST_F(AssetCatalogTest_AssetInfo, Sanity_InvalidCalls)
    {
        //Test 1: Asset not in database
        EXPECT_TRUE(GetAssetInfoByIdPair(false, "", ""));
        EXPECT_TRUE(GetSourceInfoBySourcePath(false, "", AZ::Uuid::CreateNull(), "", ""));
    }

    TEST_F(AssetCatalogTest_AssetInfo, Sanity_InvalidPath)
    {
        auto* ebus = AzToolsFramework::AssetSystemRequestBus::FindFirstHandler();

        AZ::Data::AssetInfo assetInfo;
        AZStd::string watchFolder;

        EXPECT_FALSE(ebus->GetSourceInfoBySourcePath("G:/random/folder/does/not/exist.png", assetInfo, watchFolder)); // Absolute path
        EXPECT_FALSE(ebus->GetSourceInfoBySourcePath("random/folder/does/not/exist.png", assetInfo, watchFolder)); // Relative path
    }

    TEST_F(AssetCatalogTest_AssetInfo, FindAssetNotRegisteredAsSource_FindsProduct)
    {
        // Setup: Add asset to database
        AZ::s64 jobId;
        EXPECT_TRUE(AddSourceAndJob("subfolder1", m_customDataMembers->m_assetASourceRelPath.c_str(), &(m_data->m_dbConn), jobId, m_customDataMembers->m_assetA.m_guid));
        ProductDatabaseEntry newProductEntry(jobId, 0, m_customDataMembers->m_assetAProductRelPath.c_str(), m_customDataMembers->m_assetAType);
        m_data->m_dbConn.SetProduct(newProductEntry);

        // Test 2: Asset in database, not registered as source asset
        // note that when asking for products, a performance improvement causes the catalog to use its REGISTRY
        // rather than the database to ask for products, so to set this up the registry must be present and must have the asset registered within it
        AzFramework::AssetSystem::AssetNotificationMessage message(m_customDataMembers->m_assetAProductRelPath.c_str(), AssetNotificationMessage::AssetChanged, m_customDataMembers->m_assetAType, "pc");
        message.m_sizeBytes = m_customDataMembers->m_productTestString.size();
        message.m_assetId = AZ::Data::AssetId(m_customDataMembers->m_assetA.m_guid, 0);
        message.m_platform = "pc";
        m_data->m_assetCatalog->OnAssetMessage(message);

        // also of note:  When looking up products, you don't get a root path since they are all in the cache.
        // its important here that we specifically get an empty root path.
        EXPECT_TRUE(GetAssetInfoByIdPair(true, m_customDataMembers->m_assetAProductRelPath, ""));

        // this call has to work with both full and relative path.
        EXPECT_TRUE(GetSourceInfoBySourcePath(true, m_customDataMembers->m_assetASourceRelPath.c_str(), m_customDataMembers->m_assetA.m_guid, m_customDataMembers->m_assetASourceRelPath.c_str(), m_customDataMembers->m_subfolder1AbsolutePath.c_str()));
        EXPECT_TRUE(GetSourceInfoBySourcePath(true, m_customDataMembers->m_assetAFullPath.c_str(), m_customDataMembers->m_assetA.m_guid, m_customDataMembers->m_assetASourceRelPath.c_str(), m_customDataMembers->m_subfolder1AbsolutePath.c_str()));
    }

    TEST_F(AssetCatalogTest_AssetInfo, FindAssetInBuildQueue_FindsSource)
    {
        // Setup:  Add a source to queue.
        m_data->m_assetCatalog->OnSourceQueued(
            m_customDataMembers->m_assetA.m_guid,
            m_customDataMembers->m_assetALegacyUuid,
            AssetProcessor::SourceAssetReference(
                m_customDataMembers->m_subfolder1AbsolutePath.c_str(), m_customDataMembers->m_assetASourceRelPath.c_str()));

        // TEST: Asset in queue, not registered as source asset
        EXPECT_TRUE(GetAssetInfoByIdPair(false, "", ""));

        // this call should STILL work even after the above call to "OnSourceQueued" since its explicitly asking for the source details.
        EXPECT_TRUE(GetSourceInfoBySourcePath(true, m_customDataMembers->m_assetASourceRelPath.c_str(), m_customDataMembers->m_assetA.m_guid, m_customDataMembers->m_assetASourceRelPath.c_str(), m_customDataMembers->m_subfolder1AbsolutePath.c_str()));
        EXPECT_TRUE(GetSourceInfoBySourcePath(true, m_customDataMembers->m_assetAFullPath.c_str(), m_customDataMembers->m_assetA.m_guid, m_customDataMembers->m_assetASourceRelPath.c_str(), m_customDataMembers->m_subfolder1AbsolutePath.c_str()));
    }

    TEST_F(AssetCatalogTest_AssetInfo, FindAssetInBuildQueue_RegisteredAsSourceType_StillFindsSource)
    {
        // Setup:  Add a source to queue.
        m_data->m_assetCatalog->OnSourceQueued(
            m_customDataMembers->m_assetA.m_guid,
            m_customDataMembers->m_assetALegacyUuid,
            AssetProcessor::SourceAssetReference(
                m_customDataMembers->m_subfolder1AbsolutePath.c_str(), m_customDataMembers->m_assetASourceRelPath.c_str()));

        // Register as source type
        AzToolsFramework::ToolsAssetSystemBus::Broadcast(&AzToolsFramework::ToolsAssetSystemRequests::RegisterSourceAssetType, m_customDataMembers->m_assetAType, m_customDataMembers->m_assetAFileFilter.c_str());

        // Test: Asset in queue, registered as source asset
        EXPECT_TRUE(GetAssetInfoByIdPair(true, m_customDataMembers->m_assetASourceRelPath, m_customDataMembers->m_subfolder1AbsolutePath));

        // these calls are identical to the two in the prior test, but should continue to work even though we have registered the asset type as a source asset type.
        EXPECT_TRUE(GetSourceInfoBySourcePath(true, m_customDataMembers->m_assetASourceRelPath.c_str(), m_customDataMembers->m_assetA.m_guid, m_customDataMembers->m_assetASourceRelPath.c_str(), m_customDataMembers->m_subfolder1AbsolutePath.c_str(), m_customDataMembers->m_assetAType));
        EXPECT_TRUE(GetSourceInfoBySourcePath(true, m_customDataMembers->m_assetAFullPath.c_str(), m_customDataMembers->m_assetA.m_guid, m_customDataMembers->m_assetASourceRelPath.c_str(), m_customDataMembers->m_subfolder1AbsolutePath.c_str(), m_customDataMembers->m_assetAType));
    }

    TEST_F(AssetCatalogTest_AssetInfo, FindSource_FinishedProcessing_RegisteredAsSource_FindsSource)
    {
        // Register as source type
        AzToolsFramework::ToolsAssetSystemBus::Broadcast(&AzToolsFramework::ToolsAssetSystemRequests::RegisterSourceAssetType, m_customDataMembers->m_assetAType, m_customDataMembers->m_assetAFileFilter.c_str());

        // Setup:  Add a source to queue, then notify its finished and add it to the database (simulates a full pipeline)
        AZ::s64 jobId;
        EXPECT_TRUE(AddSourceAndJob("subfolder1", m_customDataMembers->m_assetASourceRelPath.c_str(), &(m_data->m_dbConn), jobId, m_customDataMembers->m_assetA.m_guid));
        m_data->m_assetCatalog->OnSourceQueued(
            m_customDataMembers->m_assetA.m_guid,
            m_customDataMembers->m_assetALegacyUuid,
            AssetProcessor::SourceAssetReference(
                m_customDataMembers->m_subfolder1AbsolutePath.c_str(), m_customDataMembers->m_assetASourceRelPath.c_str()));
        m_data->m_assetCatalog->OnSourceFinished(m_customDataMembers->m_assetA.m_guid, m_customDataMembers->m_assetALegacyUuid);
        ProductDatabaseEntry assetAEntry(jobId, 0, m_customDataMembers->m_assetAProductRelPath.c_str(), m_customDataMembers->m_assetAType);
        m_data->m_dbConn.SetProduct(assetAEntry);

        // TEST: Asset in database, registered as source asset
        EXPECT_TRUE(GetAssetInfoByIdPair(true, m_customDataMembers->m_assetASourceRelPath, m_customDataMembers->m_subfolder1AbsolutePath));

        // at this point the details about the asset in question is no longer in memory, only the database.  However, these calls should continue find the
        // information, because the system is supposed check both the database AND the in-memory queue in the to find the info being requested.
        EXPECT_TRUE(GetSourceInfoBySourcePath(true, m_customDataMembers->m_assetASourceRelPath.c_str(), m_customDataMembers->m_assetA.m_guid, m_customDataMembers->m_assetASourceRelPath.c_str(), m_customDataMembers->m_subfolder1AbsolutePath.c_str(), m_customDataMembers->m_assetAType));
        EXPECT_TRUE(GetSourceInfoBySourcePath(true, m_customDataMembers->m_assetAFullPath.c_str(), m_customDataMembers->m_assetA.m_guid, m_customDataMembers->m_assetASourceRelPath.c_str(), m_customDataMembers->m_subfolder1AbsolutePath.c_str(), m_customDataMembers->m_assetAType));
    }

    TEST_F(AssetCatalogTest_AssetInfo, FindSource_NotProcessed_NotInQueue_FindsSource)
    {
        // Get accurate UUID based on source database name instead of using the one that was randomly generated
        AZ::Uuid expectedSourceUuid = AssetUtilities::CreateSafeSourceUUIDFromName(m_customDataMembers->m_assetASourceRelPath.c_str());

        // These calls should find the information even though the asset is not in the database and hasn't been queued up yet
        EXPECT_TRUE(GetSourceInfoBySourcePath(true, m_customDataMembers->m_assetASourceRelPath.c_str(), expectedSourceUuid, m_customDataMembers->m_assetASourceRelPath.c_str(), m_customDataMembers->m_subfolder1AbsolutePath.c_str()));
        EXPECT_TRUE(GetSourceInfoBySourcePath(true, m_customDataMembers->m_assetAFullPath.c_str(), expectedSourceUuid, m_customDataMembers->m_assetASourceRelPath.c_str(), m_customDataMembers->m_subfolder1AbsolutePath.c_str()));
    }

    TEST_F(AssetCatalogTest_AssetInfo, FindSource_NotProcessed_NotInQueue_RegisteredAsSourceType_FindsSource)
    {
        // Get accurate UUID based on source database name instead of using the one that was randomly generated
        AZ::Uuid expectedSourceUuid = AssetUtilities::CreateSafeSourceUUIDFromName(m_customDataMembers->m_assetASourceRelPath.c_str());

        // Register as source type
        AzToolsFramework::ToolsAssetSystemBus::Broadcast(&AzToolsFramework::ToolsAssetSystemRequests::RegisterSourceAssetType, m_customDataMembers->m_assetAType, m_customDataMembers->m_assetAFileFilter.c_str());

        // These calls should find the information even though the asset is not in the database and hasn't been queued up yet
        EXPECT_TRUE(GetSourceInfoBySourcePath(true, m_customDataMembers->m_assetASourceRelPath.c_str(), expectedSourceUuid, m_customDataMembers->m_assetASourceRelPath.c_str(), m_customDataMembers->m_subfolder1AbsolutePath.c_str(), m_customDataMembers->m_assetAType));
        EXPECT_TRUE(GetSourceInfoBySourcePath(true, m_customDataMembers->m_assetAFullPath.c_str(), expectedSourceUuid, m_customDataMembers->m_assetASourceRelPath.c_str(), m_customDataMembers->m_subfolder1AbsolutePath.c_str(), m_customDataMembers->m_assetAType));
    }

    TEST_F(AssetCatalogTest, Multithread_AccessCatalogWhileInitializing_IsThreadSafe)
    {
        static constexpr int NumTestAssets = 1000;
        static constexpr int NumUpdateIterations = 1000;

        using namespace AssetProcessor;
        using namespace AzFramework::AssetSystem;

        PlatformConfiguration config;

        config.EnablePlatform(AssetBuilderSDK::PlatformInfo("pc", { "test" }));

        {
            using namespace AzToolsFramework::AssetDatabase;
            auto& db = m_data->m_dbConn;

            for (int i = 0; i < NumTestAssets; ++i)
            {
                SourceDatabaseEntry sourceEntry;
                sourceEntry.m_sourceName = AZStd::to_string(i);
                sourceEntry.m_sourceGuid = AssetUtilities::CreateSafeSourceUUIDFromName(sourceEntry.m_sourceName.c_str());
                sourceEntry.m_scanFolderPK = 1;
                db.SetSource(sourceEntry);

                JobDatabaseEntry jobEntry;
                jobEntry.m_sourcePK = sourceEntry.m_sourceID;
                jobEntry.m_platform = "pc";
                jobEntry.m_jobRunKey = i + 1;
                db.SetJob(jobEntry);

                ProductDatabaseEntry productEntry;
                productEntry.m_jobPK = jobEntry.m_jobID;
                productEntry.m_productName = AZStd::to_string(i) + ".product";
                db.SetProduct(productEntry);
            }
        }

        AZStd::thread_desc threadDesc;
        threadDesc.m_name = "AssetCatalog Thread";
        AZStd::thread catalogThread(threadDesc, [this]()
            {
                m_data->m_assetCatalog->BuildRegistry();
            }
        );

        AssetNotificationMessage message("some/path/image.png", AssetNotificationMessage::NotificationType::AssetChanged, AZ::Data::AssetType::CreateRandom(), "pc");
        message.m_assetId = AZ::Data::AssetId{ "{C1A73521-E770-475F-8D91-30DF88E4D4C9}" };

        for (int i = 0; i < NumUpdateIterations; ++i)
        {
            m_data->m_assetCatalog->OnAssetMessage(message);
        }

        catalogThread.join();
    }

    class AssetCatalogTestForProductDependencies
        : public AssetCatalogTest
    {
    public:
        void SetUp() override
        {
            AssetCatalogTest::SetUp();
            m_platforms.push_back("pc");
            m_platforms.push_back("android");

            // 4 products for one platform, 1 product for the other.
            m_platformToProductsForSourceWithDifferentProducts["pc"].push_back("subfolder3/basefilez.arc2");
            m_platformToProductsForSourceWithDifferentProducts["pc"].push_back("subfolder3/basefileaz.azm2");
            m_platformToProductsForSourceWithDifferentProducts["pc"].push_back("subfolder3/basefile.arc2");
            m_platformToProductsForSourceWithDifferentProducts["pc"].push_back("subfolder3/basefile.azm2");

            m_platformToProductsForSourceWithDifferentProducts["android"].push_back("subfolder3/androidexclusivefile.azm2");

            m_sourceFileWithDifferentProductsPerPlatform = AZ::Uuid::CreateString("{38032FC9-2838-4D6A-9DA0-79E5E4F20C1B}");
            m_sourceFileWithDependency = AZ::Uuid::CreateString("{807C4174-1D19-42AD-B8BC-A59291D9388C}");

            // Setup:
            //  2 source files: MultiplatformFile.txt and FileWithDependency.txt.
            //      MultiplatformFile.txt has different products on different platforms.
            //      FileWithDependency.txt has the same product on each platform, but these products have different product dependencies per platform.
            // This setup is meant to mimic a pattern we've seen with materials and mipmaps: Mipmap generation settings can be different per platform,
            // resulting in image processing jobs having different products per platform. Because of this, the material jobs will then have different
            // dependencies per platform, because each material will depend on a referenced texture and all of that texture's mipmaps.

            // Add a source file with 4 products on pc, but 1 on android
            bool result = AddSourceAndJobForMultiplePlatforms(
                "subfolder3",
                "MultiplatformFile.txt",
                &(m_data->m_dbConn),
                m_sourceFileWithDifferentProductsJobsPerPlatform,
                m_platforms,
                m_sourceFileWithDifferentProductsPerPlatform);
            EXPECT_TRUE(result);

            // Add a source file with 1 product on each platform, that has different dependencies per platform.
            AZStd::map<AZStd::string, AZ::s64> sourceFileWithSameProductsJobsPerPlatform;
            result = AddSourceAndJobForMultiplePlatforms("subfolder3", "FileWithDependency.txt", &(m_data->m_dbConn), sourceFileWithSameProductsJobsPerPlatform, m_platforms, m_sourceFileWithDependency);
            EXPECT_TRUE(result);

            const AZStd::string fileWithDependencyProductPath = "subfolder3/androidexclusivefile.azm2";

            for (const AZStd::string& platform : m_platforms)
            {
                m_platformToSourceIdToProductIds[platform][m_sourceFileWithDependency].push_back(
                    CreateProductAndGetProductId(sourceFileWithSameProductsJobsPerPlatform[platform], fileWithDependencyProductPath, 0));
            }
        }

        void CreateProducts()
        {
            for (const AZStd::string& platform : m_platforms)
            {
                AZ::u32 productSubId = 0;
                for (const auto& relativeProductPath : m_platformToProductsForSourceWithDifferentProducts[platform])
                {
                    m_sourceWithMultipleProductsPlatformToProductIds[platform].push_back(
                        CreateProductAndGetProductId(m_sourceFileWithDifferentProductsJobsPerPlatform[platform], relativeProductPath, productSubId++));
                }
            }
        }

        // Containers are stored in unique pointers so they can be destroyed in teardown before the allocators are freed.
        AZStd::vector<AZStd::string> m_platforms;
        AZStd::map<AZStd::string, AZStd::vector<AZStd::string>> m_platformToProductsForSourceWithDifferentProducts;
        AZ::Uuid m_sourceFileWithDifferentProductsPerPlatform;
        AZ::Uuid m_sourceFileWithDependency;
        AZStd::map<AZStd::string, AZ::s64> m_sourceFileWithDifferentProductsJobsPerPlatform;
        AZStd::map<AZStd::string, AZStd::map<AZ::Uuid, AZStd::vector<AZ::s64>>> m_platformToSourceIdToProductIds;
        AZStd::map<AZStd::string, AZStd::vector<AZ::s64>> m_sourceWithMultipleProductsPlatformToProductIds;
    };

    TEST_F(AssetCatalogTestForProductDependencies, SaveCatalog_DifferentDependenciesPerPlatform_CorrectDependenciesSavedToCatalog)
    {
        CreateProducts();
        for (const AZStd::string& platform : m_platforms)
        {
            AZ::s64 productIdForPlatform = m_platformToSourceIdToProductIds[platform][m_sourceFileWithDependency][0];
            for (AZ::s64 subIdAndProductIndex : m_sourceWithMultipleProductsPlatformToProductIds[platform])
            {
                // SubId matches index.
                ProductDependencyDatabaseEntry productDependency(
                    productIdForPlatform,
                    m_sourceFileWithDifferentProductsPerPlatform,
                    aznumeric_cast<AZ::u32>(subIdAndProductIndex),
                    /*dependencyFlags*/ 0,
                    platform,
                    true);
                bool result = m_data->m_dbConn.SetProductDependency(productDependency);
                EXPECT_TRUE(result);
                // Don't need to cache anything at this point, the dependency ID isn't tracked in the catalog.
            }
        }

        m_data->m_assetCatalog->BuildRegistry();

        // Verify that the dependencies are correct.
        // Without the bug fix to AssetCatalog, every platform's registry included the dependencies for all other platforms.
        const AZ::Data::AssetId productWithDependency(m_sourceFileWithDependency, 0);
        for (const AZStd::string& platform : m_platforms)
        {
            AzFramework::AssetRegistry& registry = m_data->m_assetCatalog->GetRegistry(platform.c_str());
            EXPECT_EQ(registry.m_assetDependencies[productWithDependency].size(), m_sourceWithMultipleProductsPlatformToProductIds[platform].size());
        }
    }

    TEST_F(AssetCatalogTestForProductDependencies, SaveCatalog_DifferentDependenciesPerPlatformResolvedFromPaths_CorrectDependenciesSavedToCatalog)
    {
        // Setup:
        //  2 source files: MultiplatformFile.txt and FileWithDependency.txt.
        //      MultiplatformFile.txt has different products on different platforms.
        //      FileWithDependency.txt has the same product on each platform, but these products have different product dependencies per platform.
        //          FileWithDependency.txt initially emits dependencies as path dependencies, which are resolved later into asset IDs.
        // This test differs from the previous test in that it forces OnDependencyResolved to be called, which is where we've seen
        // bugs in the past related to the asset catalog.

        // Set up the path dependencies.
        AZStd::vector<ProductDependencyDatabaseEntry> productDependencies;
        for (const AZStd::string& platform : m_platforms)
        {
            AZ::s64 productIdForPlatform = m_platformToSourceIdToProductIds[platform][m_sourceFileWithDependency][0];
            for (const auto& relativeProductPath : m_platformToProductsForSourceWithDifferentProducts[platform])
            {
                // SubId matches index.
                ProductDependencyDatabaseEntry productDependency(
                    productIdForPlatform,
                    AZ::Uuid::CreateNull(),
                    /*subId*/ 0,
                    /*dependencyFlags*/ 0,
                    platform,
                    false,
                    relativeProductPath);
                bool result = m_data->m_dbConn.SetProductDependency(productDependency);
                EXPECT_TRUE(result);
                productDependencies.push_back(productDependency);
            }
        }

        // Create the products that match the path dependencies.
        CreateProducts();

        // Resolve the path dependencies.
        AZStd::map<AZStd::string, size_t> platformToProductIdIndex;
        for (const AZStd::string& platform : m_platforms)
        {
            platformToProductIdIndex[platform] = 0;
        }
        QDir cacheRoot;
        EXPECT_TRUE(AssetUtilities::ComputeProjectCacheRoot(cacheRoot));
        for (ProductDependencyDatabaseEntry& productDependency : productDependencies)
        {
            // These were generated in this same order previously, but it also doesn't
            // matter to this test which dependencies are upgraded from paths to asset ID,
            // what matters is calling AssetCatalog::OnDependencyResolved to replace paths with asset IDs.
            AZ::s64 subId = m_sourceWithMultipleProductsPlatformToProductIds[productDependency.m_platform][platformToProductIdIndex[productDependency.m_platform]];
            platformToProductIdIndex[productDependency.m_platform]++;

            productDependency.m_dependencySubID = aznumeric_cast<AZ::u32>(subId);
            productDependency.m_dependencySourceGuid = m_sourceFileWithDifferentProductsPerPlatform;
            productDependency.m_unresolvedPath = AZStd::string();

            QString platformGameDir = QDir(cacheRoot.absoluteFilePath(productDependency.m_platform.c_str())).filePath(AssetUtilities::ComputeProjectName().toLower());
            QString assetCatalogFile = QDir(platformGameDir).filePath("assetcatalog.xml");
            QFileInfo fileInfo(assetCatalogFile);

            EXPECT_FALSE(fileInfo.exists());

            m_data->m_assetCatalog->OnDependencyResolved(m_sourceFileWithDependency, productDependency);

            // process all events
            QCoreApplication::processEvents(QEventLoop::AllEvents);

            // This ensures that no save catalog event was queued when we resolve dependency
            EXPECT_FALSE(fileInfo.exists());
        }

        // Verify the catalog is correct.
        m_data->m_assetCatalog->BuildRegistry();

        // Verify that the dependencies are correct.
        // Without the bug fix to AssetCatalog, every platform's registry included the dependencies for all other platforms.
        const AZ::Data::AssetId productWithDependency(m_sourceFileWithDependency, 0);
        for (const AZStd::string& platform : m_platforms)
        {
            AzFramework::AssetRegistry& registry = m_data->m_assetCatalog->GetRegistry(platform.c_str());
            EXPECT_EQ(registry.m_assetDependencies[productWithDependency].size(), m_sourceWithMultipleProductsPlatformToProductIds[platform].size());
        }

        EXPECT_TRUE(true);
    }

} // namespace AssetProcessor




