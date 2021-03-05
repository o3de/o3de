/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#if defined(UNIT_TEST)

#include <AzCore/base.h>

#include "AssetCatalogUnitTests.h"

#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Casting/lossy_cast.h>
#include <AzCore/IO/FileIO.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/Asset/AssetProcessorMessages.h>

#include <AzToolsFramework/Asset/AssetProcessorMessages.h>
#include <native/AssetManager/assetProcessorManager.h>
#include <native/utilities/PlatformConfiguration.h>
#include <native/utilities/AssetBuilderInfo.h>
#include <native/AssetManager/assetScanFolderInfo.h>
#include <native/utilities/assetUtils.h>
#include <native/resourcecompiler/RCBuilder.h>
#include <native/assetprocessor.h>

#include <QTemporaryDir>
#include <QString>
#include <QCoreApplication>
#include <QSet>
#include <QList>
#include <QTime>
#include <QThread>
#include <QPair>
#include <AzToolsFramework/API/AssetDatabaseBus.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

namespace AssetProcessor
{
    using namespace UnitTestUtils;
    using namespace AzFramework::AssetSystem;
    using namespace AzToolsFramework::AssetSystem;
    using namespace AzToolsFramework::AssetDatabase;

    REGISTER_UNIT_TEST(AssetCatalogUnitTests)
    REGISTER_UNIT_TEST(AssetCatalogUnitTests_AssetInfo)

    namespace
    {

        // a utility class to redirect the location the database is stored to a different location so that we don't
        // touch real data during unit tests.
        class FakeDatabaseLocationListener
            : protected AzToolsFramework::AssetDatabase::AssetDatabaseRequests::Bus::Handler
        {
        public:
            FakeDatabaseLocationListener(const char* desiredLocation, const char* assetPath)
                : m_location(desiredLocation)
                , m_assetPath(assetPath)
            {
                AzToolsFramework::AssetDatabase::AssetDatabaseRequests::Bus::Handler::BusConnect();
            }
            ~FakeDatabaseLocationListener()
            {
                AzToolsFramework::AssetDatabase::AssetDatabaseRequests::Bus::Handler::BusDisconnect();
            }
        protected:
            // IMPLEMENTATION OF -------------- AzToolsFramework::AssetDatabase::AssetDatabaseRequests::Bus::Listener
            bool GetAssetDatabaseLocation(AZStd::string& location) override
            {
                location = m_location;
                return true;
            }

            // ------------------------------------------------------------

        private:
            AZStd::string m_location;
            AZStd::string m_assetPath;
        };

    // Adds a scan folder to the config and to the database
    void AddScanFolder(const ScanFolderInfo& scanFolderInfo, PlatformConfiguration& config, AssetDatabaseConnection* dbConn)
    {
        config.AddScanFolder(scanFolderInfo);
        ScanFolderDatabaseEntry newScanFolder(
                                scanFolderInfo.ScanPath().toStdString().c_str(),
                                scanFolderInfo.GetDisplayName().toStdString().c_str(),
                                scanFolderInfo.GetPortableKey().toStdString().c_str(),
                                scanFolderInfo.GetOutputPrefix().toStdString().c_str(),
                                scanFolderInfo.IsRoot());
        dbConn->SetScanFolder(newScanFolder);
    }

    void BuildConfig(const QDir& tempPath, AssetDatabaseConnection* dbConn, PlatformConfiguration& config)
    {
        config.EnablePlatform({ "pc" ,{ "desktop", "renderer" } }, true);
        config.EnablePlatform({ "es3" ,{ "mobile", "renderer" } }, true);
        config.EnablePlatform({ "fandango" ,{ "console", "renderer" } }, false);
        AZStd::vector<AssetBuilderSDK::PlatformInfo> platforms;
        config.PopulatePlatformsForScanFolder(platforms);
        //                                               PATH         DisplayName    PortKey      outputfolder root    recurse  platforms     order
        AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder4"), "subfolder4", "subfolder4",    "",       false,   false, platforms, -6), config, dbConn); // subfolder 4 overrides subfolder3
        AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder3"), "subfolder3", "subfolder3",    "",       false,   false, platforms, -5), config, dbConn); // subfolder 3 overrides subfolder2
        AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder2"), "subfolder2", "subfolder2",    "",       false,   true,  platforms, -2), config, dbConn); // subfolder 2 overrides subfolder1
        AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder1"), "subfolder1", "subfolder1",    "editor",       false,   true,  platforms, -1), config, dbConn); // subfolder1 overrides root
        AddScanFolder(ScanFolderInfo(tempPath.absolutePath(),         "temp",       "tempfolder",    "",       true,    false, platforms,  0), config, dbConn); // add the root

        config.AddMetaDataType("exportsettings", QString());

        AZ::Uuid buildIDRcLegacy;
        BUILDER_ID_RC.GetUuid(buildIDRcLegacy);

        AssetRecognizer rec;
        AssetPlatformSpec specpc;
        AssetPlatformSpec speces3;

        speces3.m_extraRCParams = "somerandomparam";
        rec.m_name = "random files";
        rec.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.random", AssetBuilderSDK::AssetBuilderPattern::Wildcard);
        rec.m_platformSpecs.insert("pc", specpc);
        config.AddRecognizer(rec);

        specpc.m_extraRCParams = ""; // blank must work
        speces3.m_extraRCParams = "testextraparams";

        const char* builderTxt1Name = "txt files";
        rec.m_name = builderTxt1Name;
        rec.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.txt", AssetBuilderSDK::AssetBuilderPattern::Wildcard);
        rec.m_platformSpecs.insert("pc", specpc);
        rec.m_platformSpecs.insert("es3", speces3);

        config.AddRecognizer(rec);

        // Ignore recognizer
        AssetPlatformSpec ignore_spec;
        ignore_spec.m_extraRCParams = "skip";
        AssetRecognizer ignore_rec;
        ignore_rec.m_name = "ignore files";
        ignore_rec.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.ignore", AssetBuilderSDK::AssetBuilderPattern::Wildcard);
        ignore_rec.m_platformSpecs.insert("pc", specpc);
        ignore_rec.m_platformSpecs.insert("es3", ignore_spec);
        config.AddRecognizer(ignore_rec);

        ExcludeAssetRecognizer excludeRecogniser;
        excludeRecogniser.m_name = "backup";
        excludeRecogniser.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher(".*\\/savebackup\\/.*", AssetBuilderSDK::AssetBuilderPattern::Regex);
        config.AddExcludeRecognizer(excludeRecogniser);
    }

    // Adds a source file and job entry to the database, jobId is output
    bool AddSourceAndJob(const char* scanFolder, const char* sourceRelPath, AssetDatabaseConnection* dbConn, AZ::s64& jobId, AZ::Uuid assetId = AZ::Uuid::CreateRandom())
    {
        ScanFolderDatabaseEntry scanFolderEntry;
        bool result = dbConn->GetScanFolderByPortableKey(scanFolder, scanFolderEntry);

        if(!result)
        {
            return false;
        }

        SourceDatabaseEntry sourceEntry(scanFolderEntry.m_scanFolderID, sourceRelPath, assetId, "fingerprint");
        dbConn->SetSource(sourceEntry);

        JobDatabaseEntry jobEntry(sourceEntry.m_sourceID, "test", 1234, "pc", assetId, AzToolsFramework::AssetSystem::JobStatus::Completed, 12345);
        dbConn->SetJob(jobEntry);

        jobId = jobEntry.m_jobID;
        return true;
    };

    // Calls the GetRelativeProductPathFromFullSourceOrProductPath function and checks the return results, returning true if it matches both of the expected results
    bool TestGetRelativeProductPath(const QString fileToCheck, bool expectedToFind, AZStd::initializer_list<const char*> expectedPaths)
    {
        bool relPathfound = false;
        AZStd::string relPath;
        AZStd::string fullPath(fileToCheck.toStdString().c_str());

        EBUS_EVENT_RESULT(relPathfound, AzToolsFramework::AssetSystemRequestBus, GetRelativeProductPathFromFullSourceOrProductPath, fullPath, relPath);

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

    // Calls the GetFullSourcePathFromRelativeProductPath function and checks the return results, returning true if it matches both of the expected results
    bool TestGetFullSourcePath(const QString& fileToCheck, const QDir& tempPath, bool expectToFind, const char* expectedPath)
    {
        bool fullPathfound = false;
        AZStd::string fullPath;
        AZStd::string relPath(fileToCheck.toStdString().c_str());

        EBUS_EVENT_RESULT(fullPathfound, AzToolsFramework::AssetSystemRequestBus, GetFullSourcePathFromRelativeProductPath, relPath, fullPath);

        if (fullPathfound != expectToFind)
        {
            return false;
        }

        QString output(fullPath.c_str());
        output.remove(0, tempPath.path().length() + 1); //adding one for the native separator

        return (output == expectedPath);
    }

    } // end anon namespace

    void AssetCatalogUnitTests::StartTest()
    {
        QDir oldRoot;
        AssetUtilities::ComputeAssetRoot(oldRoot);
        AssetUtilities::ResetAssetRoot();

        // the canonicalization of the path here is to get around the fact that on some platforms
        // the "temporary" folder location could be junctioned into some other folder and getting "QDir::current()"
        // and other similar functions may actually return a different string but still be referring to the same folder   
        QTemporaryDir dir;
        QDir tempPath(dir.path());
        QString canonicalTempDirPath = AssetUtilities::NormalizeDirectoryPath(tempPath.canonicalPath());
        UnitTestUtils::ScopedDir changeDir(canonicalTempDirPath);
        tempPath = QDir(canonicalTempDirPath);
        NetworkRequestID requestId(1, 1);

        FakeDatabaseLocationListener listener(tempPath.filePath("statedatabase.sqlite").toUtf8().constData(), "displayString");

        AZStd::unique_ptr<AssetDatabaseConnection> dbConn = AZStd::make_unique<AssetDatabaseConnection>();
        dbConn->OpenDatabase();

        CreateDummyFile(tempPath.absoluteFilePath("bootstrap.cfg"), QString("sys_game_folder=SamplesProject\n"));

        // system is already actually initialized, along with gEnv, so this will always return that game name.
        QString gameName = AssetUtilities::ComputeGameName();

        // update the engine root
        AssetUtilities::ResetAssetRoot();
        QDir newRoot;
        AssetUtilities::ComputeAssetRoot(newRoot, &tempPath);

        UNIT_TEST_EXPECT_FALSE(gameName.isEmpty());
        // should create cache folder in the root, and read everything from there.

        QSet<QString> expectedFiles;
        // set up some interesting files:
        expectedFiles << tempPath.absoluteFilePath("rootfile2.txt");
        expectedFiles << tempPath.absoluteFilePath("subfolder1/rootfile1.txt"); // note:  Must override the actual root file
        expectedFiles << tempPath.absoluteFilePath("subfolder1/basefile.txt");
        expectedFiles << tempPath.absoluteFilePath("subfolder2/basefile.txt");
        expectedFiles << tempPath.absoluteFilePath("subfolder2/aaa/basefile.txt");
        expectedFiles << tempPath.absoluteFilePath("subfolder2/aaa/bbb/basefile.txt");
        expectedFiles << tempPath.absoluteFilePath("subfolder2/aaa/bbb/ccc/basefile.txt");
        expectedFiles << tempPath.absoluteFilePath("subfolder2/aaa/bbb/ccc/ddd/basefile.txt");
        expectedFiles << tempPath.absoluteFilePath("subfolder3/BaseFile.txt"); // note the case upper here
        expectedFiles << tempPath.absoluteFilePath("subfolder8/a/b/c/test.txt");

        // subfolder3 is not recursive so none of these should show up in any scan or override check
        expectedFiles << tempPath.absoluteFilePath("subfolder3/aaa/basefile.txt");
        expectedFiles << tempPath.absoluteFilePath("subfolder3/aaa/bbb/basefile.txt");
        expectedFiles << tempPath.absoluteFilePath("subfolder3/aaa/bbb/ccc/basefile.txt");

        expectedFiles << tempPath.absoluteFilePath("subfolder3/uniquefile.txt"); // only exists in subfolder3
        expectedFiles << tempPath.absoluteFilePath("subfolder3/uniquefile.ignore"); // only exists in subfolder3

        expectedFiles << tempPath.absoluteFilePath("subfolder3/rootfile3.txt"); // must override rootfile3 in root
        expectedFiles << tempPath.absoluteFilePath("rootfile1.txt");
        expectedFiles << tempPath.absoluteFilePath("rootfile3.txt");
        expectedFiles << tempPath.absoluteFilePath("unrecognised.file"); // a file that should not be recognised
        expectedFiles << tempPath.absoluteFilePath("unrecognised2.file"); // a file that should not be recognised
        expectedFiles << tempPath.absoluteFilePath("subfolder1/test/test.format"); // a file that should be recognised
        expectedFiles << tempPath.absoluteFilePath("test.format"); // a file that should NOT be recognised
        expectedFiles << tempPath.absoluteFilePath("subfolder3/somefile.xxx");
        expectedFiles << tempPath.absoluteFilePath("subfolder3/savebackup/test.txt");//file that should be excluded
        expectedFiles << tempPath.absoluteFilePath("subfolder3/somerandomfile.random");

        for (const QString& expect : expectedFiles)
        {
            UNIT_TEST_EXPECT_TRUE(CreateDummyFile(expect));
            AZ_TracePrintf(AssetProcessor::DebugChannel, "Created file %s with msecs %llu\n", expect.toUtf8().constData(),
                QFileInfo(expect).lastModified().toMSecsSinceEpoch());

#if defined(AZ_PLATFORM_WINDOWS)
            QThread::msleep(35); // give at least some milliseconds so that the files never share the same timestamp exactly
#else
            // on platforms such as mac, the file time resolution is only a second :(
            QThread::msleep(1001);
#endif
        }

        PlatformConfiguration config;
        BuildConfig(tempPath, dbConn.get(), config);

        AssetCatalog assetCatalog(nullptr, &config);
        QDir cacheRoot;
        UNIT_TEST_EXPECT_TRUE(AssetUtilities::ComputeProjectCacheRoot(cacheRoot));
        QString normalizedCacheRoot = AssetUtilities::NormalizeDirectoryPath(cacheRoot.absolutePath());

        // make sure it picked up the one in the current folder

        QString normalizedDirPathCheck = AssetUtilities::NormalizeDirectoryPath(tempPath.absoluteFilePath("Cache/" + gameName));
        UNIT_TEST_EXPECT_TRUE(normalizedCacheRoot == normalizedDirPathCheck);
        QDir normalizedCacheRootDir(normalizedCacheRoot);

        // ----- Test the get asset path functions, which given a full path to an asset, checks the mappings and turns it into an Asset ID ---
        {
            // sanity check - make sure it does not crash or misbehave when given empty names
            QString fileToCheck = "";
            {
                UnitTestUtils::AssertAbsorber absorb;
                // empty requests should generate an assert.
                GetRelativeProductPathFromFullSourceOrProductPathRequest request(fileToCheck.toUtf8().constData());
                UNIT_TEST_EXPECT_TRUE(absorb.m_numAssertsAbsorbed == 1);
                GetFullSourcePathFromRelativeProductPathRequest sourceRequest(fileToCheck.toUtf8().constData());
                UNIT_TEST_EXPECT_TRUE(absorb.m_numAssertsAbsorbed == 2);
            }

            UNIT_TEST_EXPECT_TRUE(TestGetRelativeProductPath("", false, {""}));
            UNIT_TEST_EXPECT_TRUE(TestGetFullSourcePath("", tempPath, false, ""));

            // Add a source file with 4 products
            {
                AZ::s64 jobId;
                bool result = AddSourceAndJob("subfolder3", "BaseFile.txt", dbConn.get(), jobId);
                UNIT_TEST_EXPECT_TRUE(result);

                AZ::u32 productSubId = 0;
                for (auto& relativeProductPath : { "subfolder3/basefilez.arc2", "subfolder3/basefileaz.azm2", "subfolder3/basefile.arc2", "subfolder3/basefile.azm2" })
                {
                    ProductDatabaseEntry newProduct(jobId, productSubId++, cacheRoot.relativeFilePath(relativeProductPath).toStdString().c_str(), AZ::Data::AssetType::CreateRandom());
                    dbConn->SetProduct(newProduct);
                }
            }

            // GetRelativeProductPathFromFullSourceOrProductPath has 4 code paths:
            // 1) Relative input paths are returned straight away
            // 2) Paths inside the cache folder are transformed to product paths, no database involvement
            // 3) Source files that have a product return the product path
            // 4) Source files that don't have a product return the source file's relative path since that is the path a product might have

            // Failure case
#if defined(AZ_PLATFORM_WINDOWS)
            fileToCheck = "d:\\test.txt";
#else
            fileToCheck = "/test.txt"; // rooted
#endif
            UNIT_TEST_EXPECT_TRUE(TestGetRelativeProductPath(fileToCheck, false, { fileToCheck.toStdString().c_str() }));

            // (Case 1) feed it a relative path
            fileToCheck = "\test.txt";
            UNIT_TEST_EXPECT_TRUE(TestGetRelativeProductPath(fileToCheck, true, { "\test.txt" }));

            // (Case 2) feed it a product path with gamename
            fileToCheck = normalizedCacheRootDir.filePath("pc/" + gameName + "/aaa/basefile.txt");
            UNIT_TEST_EXPECT_TRUE(TestGetRelativeProductPath(fileToCheck, true, { "aaa/basefile.txt" }));

            // (Case 2) feed it a product path without gamename
            fileToCheck = normalizedCacheRootDir.filePath("pc/basefile.txt");
            UNIT_TEST_EXPECT_TRUE(TestGetRelativeProductPath(fileToCheck, true, { "basefile.txt" }));

            // (Case 2) feed it a product path with gamename but poor casing (test 1:  the pc platform is not matching case)
            fileToCheck = normalizedCacheRootDir.filePath("Pc/" + gameName + "/aaa/basefile.txt");
            UNIT_TEST_EXPECT_TRUE(TestGetRelativeProductPath(fileToCheck, true, { "aaa/basefile.txt" }));

            // (Case 2) feed it a product path with gamename but poor casing (test 2:  the gameName is not matching case)
            fileToCheck = normalizedCacheRootDir.filePath("pc/" + gameName.toUpper() + "/aaa/basefile.txt");
            UNIT_TEST_EXPECT_TRUE(TestGetRelativeProductPath(fileToCheck, true, { "aaa/basefile.txt" }));

            // (Case 2) feed it a product path that resolves to a directory name instead of a file.
            fileToCheck = normalizedCacheRootDir.filePath("pc/" + gameName.toUpper() + "/aaa");
            UNIT_TEST_EXPECT_TRUE(TestGetRelativeProductPath(fileToCheck, true, { "aaa" }));

            // Case 3
            fileToCheck = tempPath.absoluteFilePath("subfolder3/BaseFile.txt");
            UNIT_TEST_EXPECT_TRUE(TestGetRelativeProductPath(fileToCheck, true, { "basefilez.arc2", "basefileaz.azm2", "basefile.arc2", "basefile.azm2" }));

            // Case 4
            fileToCheck = tempPath.absoluteFilePath("subfolder2/aaa/basefile.txt");
            UNIT_TEST_EXPECT_TRUE(TestGetRelativeProductPath(fileToCheck, true, { "aaa/basefile.txt" }));

            // ----- Test the ProcessGetFullAssetPath function
            {
                QStringList pcouts;
                pcouts.push_back(cacheRoot.filePath(QString("pc/") + gameName + "/subfolder3/randomfileoutput.random"));
                pcouts.push_back(cacheRoot.filePath(QString("pc/") + gameName + "/subfolder3/randomfileoutput.random1"));
                pcouts.push_back(cacheRoot.filePath(QString("pc/") + gameName + "/subfolder3/randomfileoutput.random2"));

                AZ::s64 jobId;
                bool result = AddSourceAndJob("subfolder3", "somerandomfile.random", dbConn.get(), jobId);
                UNIT_TEST_EXPECT_TRUE(result);

                AZ::u32 productSubID = 0;
                for (auto& product : pcouts)
                {
                    ProductDatabaseEntry newProduct(jobId, productSubID++, cacheRoot.relativeFilePath(product).toStdString().c_str(), AZ::Data::AssetType::CreateRandom());
                    dbConn->SetProduct(newProduct);
                }
            }

            //feed it an relative product, and expect a full, absolute source file path in return.
            fileToCheck = "subfolder3/randomfileoutput.random1";
            UNIT_TEST_EXPECT_TRUE(TestGetFullSourcePath(fileToCheck, tempPath, true, "subfolder3/somerandomfile.random"));

            //feed it another relative product
            fileToCheck = "subfolder3/randomfileoutput.random2";
            UNIT_TEST_EXPECT_TRUE(TestGetFullSourcePath(fileToCheck, tempPath, true, "subfolder3/somerandomfile.random"));

            //feed it the same relative product with different separators
            fileToCheck = "subfolder3\\randomfileoutput.random2";
            UNIT_TEST_EXPECT_TRUE(TestGetFullSourcePath(fileToCheck, tempPath, true, "subfolder3/somerandomfile.random"));

            //feed it a full path
            fileToCheck = tempPath.filePath("somefolder/somefile.txt");
            UNIT_TEST_EXPECT_TRUE(TestGetFullSourcePath(fileToCheck, tempPath, true, "somefolder/somefile.txt"));

            //feed it a path with alias and asset id
            fileToCheck = "@assets@/subfolder3/randomfileoutput.random1";
            UNIT_TEST_EXPECT_TRUE(TestGetFullSourcePath(fileToCheck, tempPath, true, "subfolder3/somerandomfile.random"));

            //feed it a path with some random alias and asset id
            fileToCheck = "@somerandomalias@/subfolder3/randomfileoutput.random1";
            UNIT_TEST_EXPECT_TRUE(TestGetFullSourcePath(fileToCheck, tempPath, true, "subfolder3/somerandomfile.random"));

            //feed it a path with some random alias and asset id but no separator
            fileToCheck = "@somerandomalias@subfolder3/randomfileoutput.random1";
            UNIT_TEST_EXPECT_TRUE(TestGetFullSourcePath(fileToCheck, tempPath, true, "subfolder3/somerandomfile.random"));

            //feed it a path with alias and input name
            fileToCheck = "@assets@/somerandomfile.random";
            UNIT_TEST_EXPECT_TRUE(TestGetFullSourcePath(fileToCheck, tempPath, true, "subfolder3/somerandomfile.random"));

            //feed it an absolute path with cacheroot
            fileToCheck = normalizedCacheRootDir.filePath("pc/" + gameName + "/subfolder3/randomfileoutput.random1");
            UNIT_TEST_EXPECT_TRUE(TestGetFullSourcePath(fileToCheck, tempPath, true, "subfolder3/somerandomfile.random"));

            //feed it a productName directly
            fileToCheck = "pc/" + gameName + "/subfolder3/randomfileoutput.random1";
            UNIT_TEST_EXPECT_TRUE(TestGetFullSourcePath(fileToCheck, tempPath, true, "subfolder3/somerandomfile.random"));
        }

        Q_EMIT UnitTestPassed();
    }

    //////////////////////////////////////////////////////////////////////////

    void AssetCatalogUnitTests_AssetInfo::StartTest()
    {
        using namespace AZ::Data;
        using namespace AzToolsFramework;

        // the canonicalization of the path here is to get around the fact that on some platforms
        // the "temporary" folder location could be junctioned into some other folder and getting "QDir::current()"
        // and other similar functions may actually return a different string but still be referring to the same folder   
        QTemporaryDir dir;
        QDir tempPath(dir.path());
        QString canonicalTempDirPath = AssetUtilities::NormalizeDirectoryPath(tempPath.canonicalPath());
        UnitTestUtils::ScopedDir changeDir(canonicalTempDirPath);
        tempPath = QDir(canonicalTempDirPath);
        
        CreateDummyFile(tempPath.absoluteFilePath("bootstrap.cfg"), QString("sys_game_folder=SamplesProject\n"));

        // system is already actually initialized, along with gEnv, so this will always return that game name.
        QString gameName = AssetUtilities::ComputeGameName();

        // update the engine root
        AssetUtilities::ResetAssetRoot();
        QDir newRoot;
        AssetUtilities::ComputeAssetRoot(newRoot, &tempPath);
        QDir cacheRoot;
        AssetUtilities::ComputeProjectCacheRoot(cacheRoot);
        AZStd::string cacheRootPath = cacheRoot.absolutePath().toStdString().c_str();

        FakeDatabaseLocationListener listener(tempPath.filePath("statedatabase.sqlite").toUtf8().constData(), "displayString");

        AZStd::unique_ptr<AssetDatabaseConnection> dbConn = AZStd::make_unique<AssetDatabaseConnection>();
        dbConn->OpenDatabase();

        PlatformConfiguration config;
        BuildConfig(tempPath, dbConn.get(), config);

        AssetCatalog assetCatalog(nullptr, &config);

        //////////////////////////////////////////////////////////////////////////

        AssetId assetA(AZ::Uuid::CreateRandom(), 0);
        AZ::Uuid assetALegacyUuid = AZ::Uuid::CreateRandom();
        AssetType assetAType = AssetType::CreateRandom();
        AZStd::string assetAFileFilter = "*.source";
        AZStd::string subfolder1AbsolutePath = tempPath.absoluteFilePath("subfolder1").toStdString().c_str();
        AZStd::string assetASourceRelPath = "assetA.source";
        AZStd::string assetASourceDatabasePath = "editor/assetA.source";
        AZStd::string assetAProductRelPath = "editor/assetA.product";

        AZStd::string assetAFullPath;
        AzFramework::StringFunc::Path::Join(subfolder1AbsolutePath.c_str(), assetASourceRelPath.c_str(), assetAFullPath);
        CreateDummyFile(QString::fromUtf8(assetAFullPath.c_str()), "Its the Asset A"); // 15 bytes of data

        AZStd::string assetAProductFullPath;
        AzFramework::StringFunc::Path::Join(cacheRootPath.c_str(), assetAProductRelPath.c_str(), assetAProductFullPath);
        CreateDummyFile(QString::fromUtf8(assetAProductFullPath.c_str()), "Its a product A"); // 15 bytes of data

        auto getAssetInfoById = [assetA, assetAType, subfolder1AbsolutePath](bool expectedResult, AZStd::string expectedRelPath, AZStd::string expectedRootPath, AssetType assetType) -> bool
        {
            bool result = false;
            AssetInfo assetInfo;
            AZStd::string rootPath;
            AssetSystemRequestBus::BroadcastResult(result, &AssetSystem::AssetSystemRequest::GetAssetInfoById, assetA, assetType, assetInfo, rootPath);

            if (result != expectedResult)
            {
                return false;
            }

            if (expectedResult)
            {
                return (assetInfo.m_assetId == assetA)
                    && (assetInfo.m_assetType == assetAType)
                    && (assetInfo.m_relativePath == expectedRelPath)
                    && (assetInfo.m_sizeBytes == 15)
                    && (rootPath == expectedRootPath);
            }

            return true;
        };

        auto getAssetInfoByIdPair = [&](bool expectedResult, AZStd::string expectedRelPath, AZStd::string expectedRootPath) -> bool
        {
            // First test without providing the assetType
            bool result = getAssetInfoById(expectedResult, expectedRelPath, expectedRootPath, AssetType::CreateNull());

            // If successful, test again, this time providing the assetType
            if (result)
            {
                result = getAssetInfoById(expectedResult, expectedRelPath, expectedRootPath, assetAType);
            }

            return result;
        };

        auto getSourceInfoBySourcePath = [](bool expectedResult, AZStd::string sourcePath, AZ::Uuid expectedUuid, AZStd::string expectedRelPath, AZStd::string expectedRootPath, AZ::Data::AssetType expectedType = AZ::Data::s_invalidAssetType) -> bool
        {
            bool result = false;
            AssetInfo assetInfo;
            AZStd::string rootPath;
            AssetSystemRequestBus::BroadcastResult(result, &AssetSystem::AssetSystemRequest::GetSourceInfoBySourcePath, sourcePath.c_str(), assetInfo, rootPath);

            if (result != expectedResult)
            {
                return false;
            }

            if (expectedResult)
            {
                return (assetInfo.m_assetId == expectedUuid)
                    && (assetInfo.m_assetType == expectedType)
                    && (assetInfo.m_relativePath == expectedRelPath)
                    && (assetInfo.m_sizeBytes == 15)
                    && (rootPath == expectedRootPath)
                    ;
            }

            return true;
        };

        //Test 1: Asset not in database
        UNIT_TEST_EXPECT_TRUE(getAssetInfoByIdPair(false, "", ""));
        UNIT_TEST_EXPECT_TRUE(getSourceInfoBySourcePath(false, "", AZ::Uuid::CreateNull(), "", ""));

        // Add asset to database
        AZ::s64 jobId;
        UNIT_TEST_EXPECT_TRUE(AddSourceAndJob("subfolder1", assetASourceDatabasePath.c_str(), dbConn.get(), jobId, assetA.m_guid));
        ProductDatabaseEntry newProductEntry(jobId, 0, assetAProductRelPath.c_str(), assetAType);
        dbConn->SetProduct(newProductEntry);

        // Test 2: Asset in database, not registered as source asset
        // note that when asking for products, a performance improvement causes the catalog to use its REGISTRY
        // rather than the database to ask for products, so to set this up the registry must be present and must have the asset registered within it
        AzFramework::AssetSystem::AssetNotificationMessage message(assetAProductRelPath.c_str(), AssetNotificationMessage::AssetChanged, assetAType);
        message.m_sizeBytes = 15;
        message.m_assetId = AZ::Data::AssetId(assetA.m_guid, 0);
        assetCatalog.OnAssetMessage("pc", message);

        // also of note:  When looking up products, you don't get a root path since they are all in the cache.
        // its important here that we specifically get an empty root path.
        UNIT_TEST_EXPECT_TRUE(getAssetInfoByIdPair(true, assetAProductRelPath, ""));

        // this call has to work with full and relative path.
        UNIT_TEST_EXPECT_TRUE(getSourceInfoBySourcePath(true, assetASourceRelPath.c_str(), assetA.m_guid, assetASourceRelPath.c_str(), subfolder1AbsolutePath.c_str()));
        UNIT_TEST_EXPECT_TRUE(getSourceInfoBySourcePath(true, assetAFullPath.c_str(), assetA.m_guid, assetASourceRelPath.c_str(), subfolder1AbsolutePath.c_str()));

        dbConn->RemoveProductsByJobID(jobId);

        // similar to the above, because its not the DB that is used for products, we have to inform the catalog that its gone
        message.m_type = AssetNotificationMessage::AssetRemoved;
        assetCatalog.OnAssetMessage("pc", message);

        // Add to queue
        assetCatalog.OnSourceQueued(assetA.m_guid, assetALegacyUuid, subfolder1AbsolutePath.c_str(), assetASourceRelPath.c_str());

        //Test 3: Asset in queue, not registered as source asset
        UNIT_TEST_EXPECT_TRUE(getAssetInfoByIdPair(false, "", ""));

        // this call should STILL work even after the above call to "OnSourceQueued" since its explicitly asking for the source details.
        UNIT_TEST_EXPECT_TRUE(getSourceInfoBySourcePath(true, assetASourceRelPath.c_str(), assetA.m_guid, assetASourceRelPath.c_str(), subfolder1AbsolutePath.c_str()));
        UNIT_TEST_EXPECT_TRUE(getSourceInfoBySourcePath(true, assetAFullPath.c_str(), assetA.m_guid, assetASourceRelPath.c_str(), subfolder1AbsolutePath.c_str()));

        // Register as source type
        // note that once this call has been made, ALL REQUESTS for this type of asset should always include an appropriate type (non zero)
        ToolsAssetSystemBus::Broadcast(&ToolsAssetSystemRequests::RegisterSourceAssetType, assetAType, assetAFileFilter.c_str());

        //Test 4: Asset in queue, registered as source asset
        UNIT_TEST_EXPECT_TRUE(getAssetInfoByIdPair(true, assetASourceRelPath, subfolder1AbsolutePath));

        // these calls are identical to the two above, but should continue to work even though we have registered the asset type as a source asset type.
        UNIT_TEST_EXPECT_TRUE(getSourceInfoBySourcePath(true, assetASourceRelPath.c_str(), assetA.m_guid, assetASourceRelPath.c_str(), subfolder1AbsolutePath.c_str(), assetAType));
        UNIT_TEST_EXPECT_TRUE(getSourceInfoBySourcePath(true, assetAFullPath.c_str(), assetA.m_guid, assetASourceRelPath.c_str(), subfolder1AbsolutePath.c_str(), assetAType));

        // Remove from queue
        assetCatalog.OnSourceFinished(assetA.m_guid, assetALegacyUuid);

        // Add asset to database
        ProductDatabaseEntry assetAEntry(jobId, 0, assetAProductRelPath.c_str(), assetAType);
        dbConn->SetProduct(assetAEntry);

        //Test 5: Asset in database, registered as source asset
        UNIT_TEST_EXPECT_TRUE(getAssetInfoByIdPair(true, assetASourceRelPath, subfolder1AbsolutePath));

        // at this point the details about the asset in question is no longer in memory, only the database.  However, these calls should continue find the
        // information, because the system is supposed check both the database AND the in-memory queue in the to find the info being requested.
        UNIT_TEST_EXPECT_TRUE(getSourceInfoBySourcePath(true, assetASourceRelPath.c_str(), assetA.m_guid, assetASourceRelPath.c_str(), subfolder1AbsolutePath.c_str(), assetAType));
        UNIT_TEST_EXPECT_TRUE(getSourceInfoBySourcePath(true, assetAFullPath.c_str(), assetA.m_guid, assetASourceRelPath.c_str(), subfolder1AbsolutePath.c_str(), assetAType));

        Q_EMIT UnitTestPassed();
    }

#include <native/unittests/moc_AssetCatalogUnitTests.cpp>
} // namespace AssetProcessor

#endif
