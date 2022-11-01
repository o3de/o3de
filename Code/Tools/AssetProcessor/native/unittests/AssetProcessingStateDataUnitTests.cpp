/*
* Copyright (c) Contributors to the Open 3D Engine Project.
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
*
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/
#include <native/tests/AssetProcessorTest.h>

#include <native/unittests/AssetProcessorUnitTests.h>
#include <native/unittests/UnitTestRunner.h> // for the assert absorber.

namespace AssetProcessor
{
    class AssetProcessingStateDataUnitTest
        : public UnitTest::AssetProcessorUnitTestBase
    {
    protected:
        void SetUp() override
        {
            UnitTest::AssetProcessorUnitTestBase::SetUp();

            EXPECT_TRUE(m_connection.DataExists());
            EXPECT_TRUE(m_connection.OpenDatabase());

        }

        void TearDown() override
        {
            m_connection.CloseDatabase();

            UnitTest::AssetProcessorUnitTestBase::TearDown();
        }

        AssetProcessor::AssetDatabaseConnection m_connection;
    };

    TEST_F(AssetProcessingStateDataUnitTest, DataTest_ValidDatabaseConnectionProvided_OperationsSucceed)
    {
        using namespace AzToolsFramework::AssetDatabase;

        ScanFolderDatabaseEntry scanFolder;
        SourceDatabaseEntry source;
        JobDatabaseEntry job;
        ProductDatabaseEntry product;
        ProductDependencyDatabaseEntry productDependency;

        ScanFolderDatabaseEntryContainer scanFolders;
        SourceDatabaseEntryContainer sources;
        JobDatabaseEntryContainer jobs;
        ProductDatabaseEntryContainer products;
        ProductDependencyDatabaseEntryContainer productDependencies;
        MissingProductDependencyDatabaseEntryContainer missingDependencies;

        QString outName;
        QString outPlat;
        QString outJobDescription;

        AZ::Uuid validSourceGuid1 = AZ::Uuid::CreateRandom();
        AZ::Uuid validSourceGuid2 = AZ::Uuid::CreateRandom();
        AZ::Uuid validSourceGuid3 = AZ::Uuid::CreateRandom();
        AZ::Uuid validSourceGuid4 = AZ::Uuid::CreateRandom();
        AZ::Uuid validSourceGuid5 = AZ::Uuid::CreateRandom();
        AZ::Uuid validSourceGuid6 = AZ::Uuid::CreateRandom();

        AZ::u32 validFingerprint1 = 1;
        AZ::u32 validFingerprint2 = 2;
        AZ::u32 validFingerprint3 = 3;
        AZ::u32 validFingerprint4 = 4;
        AZ::u32 validFingerprint5 = 5;
        AZ::u32 validFingerprint6 = 6;

        AZ::Uuid validBuilderGuid1 = AZ::Uuid::CreateRandom();
        AZ::Uuid validBuilderGuid2 = AZ::Uuid::CreateRandom();
        AZ::Uuid validBuilderGuid3 = AZ::Uuid::CreateRandom();
        AZ::Uuid validBuilderGuid4 = AZ::Uuid::CreateRandom();
        AZ::Uuid validBuilderGuid5 = AZ::Uuid::CreateRandom();
        AZ::Uuid validBuilderGuid6 = AZ::Uuid::CreateRandom();

        AZ::Data::AssetType validAssetType1 = AZ::Data::AssetType::CreateRandom();
        AZ::Data::AssetType validAssetType2 = AZ::Data::AssetType::CreateRandom();
        AZ::Data::AssetType validAssetType3 = AZ::Data::AssetType::CreateRandom();
        AZ::Data::AssetType validAssetType4 = AZ::Data::AssetType::CreateRandom();
        AZ::Data::AssetType validAssetType5 = AZ::Data::AssetType::CreateRandom();
        AZ::Data::AssetType validAssetType6 = AZ::Data::AssetType::CreateRandom();

        AzToolsFramework::AssetSystem::JobStatus statusQueued = AzToolsFramework::AssetSystem::JobStatus::Queued;
        AzToolsFramework::AssetSystem::JobStatus statusCompleted = AzToolsFramework::AssetSystem::JobStatus::Completed;

        //////////////////////////////////////////////////////////////////////////
        //ScanFolder
        //the database all starts with a scan folder since all sources are have one
        auto ScanFoldersContainScanFolderID = [](const ScanFolderDatabaseEntryContainer& scanFolders, AZ::s64 scanFolderID) -> bool
        {
            for (const auto& scanFolder : scanFolders)
            {
                if (scanFolder.m_scanFolderID == scanFolderID)
                {
                    return true;
                }
            }
            return false;
        };

        auto ScanFoldersContainScanPath = [](const ScanFolderDatabaseEntryContainer& scanFolders, const char* scanPath) -> bool
        {
            for (const auto& scanFolder : scanFolders)
            {
                if (scanFolder.m_scanFolder == scanPath)
                {
                    return true;
                }
            }
            return false;
        };

        auto ScanFoldersContainPortableKey = [](const ScanFolderDatabaseEntryContainer& scanFolders, const char* portableKey) -> bool
        {
            for (const auto& scanFolder : scanFolders)
            {
                if (scanFolder.m_portableKey == portableKey)
                {
                    return true;
                }
            }
            return false;
        };

        //there are no scan folders yet so trying to find one should fail
        EXPECT_FALSE(m_connection.GetScanFolders(scanFolders));
        EXPECT_FALSE(m_connection.GetScanFolderByScanFolderID(0, scanFolder));
        EXPECT_FALSE(m_connection.GetScanFolderBySourceID(0, scanFolder));
        EXPECT_FALSE(m_connection.GetScanFolderByProductID(0, scanFolder));
        EXPECT_FALSE(m_connection.GetScanFolderByPortableKey("sadfsadfsadfsadfs", scanFolder));
        scanFolders.clear();

        //add a scanfolder
        scanFolder = ScanFolderDatabaseEntry("c:/O3DE/dev", "dev", "rootportkey");
        EXPECT_TRUE(m_connection.SetScanFolder(scanFolder));
        EXPECT_NE(scanFolder.m_scanFolderID, AzToolsFramework::AssetDatabase::InvalidEntryId) << "AssetProcessingStateDataTest Failed - scan folder failed to add";

        //add the same folder again, should not add another because it already exists, so we should get the same id
        // not only that, but the path should update.
        ScanFolderDatabaseEntry dupeScanFolder("c:/O3DE/dev2", "dev", "rootportkey");
        dupeScanFolder.m_scanFolderID = AzToolsFramework::AssetDatabase::InvalidEntryId;
        EXPECT_TRUE(m_connection.SetScanFolder(dupeScanFolder));
        EXPECT_EQ(dupeScanFolder, scanFolder) << "AssetProcessingStateDataTest Failed - scan folder failed to add";

        EXPECT_EQ(dupeScanFolder.m_portableKey, scanFolder.m_portableKey);
        EXPECT_EQ(dupeScanFolder.m_scanFolderID, scanFolder.m_scanFolderID);

        //get all scan folders, there should only the one we added
        scanFolders.clear();
        EXPECT_TRUE(m_connection.GetScanFolders(scanFolders));
        EXPECT_EQ(scanFolders.size(), 1);
        EXPECT_TRUE(ScanFoldersContainScanPath(scanFolders, "c:/O3DE/dev2"));
        EXPECT_TRUE(ScanFoldersContainScanFolderID(scanFolders, scanFolder.m_scanFolderID));
        EXPECT_TRUE(ScanFoldersContainPortableKey(scanFolders, scanFolder.m_portableKey.c_str()));
        EXPECT_TRUE(ScanFoldersContainPortableKey(scanFolders, "rootportkey"));

        //retrieve the one we just made by id
        ScanFolderDatabaseEntry retrieveScanfolderById;
        EXPECT_TRUE(m_connection.GetScanFolderByScanFolderID(scanFolder.m_scanFolderID, retrieveScanfolderById));
        EXPECT_NE(retrieveScanfolderById.m_scanFolderID, AzToolsFramework::AssetDatabase::InvalidEntryId) << "AssetProcessingStateDataTest Failed - scan folder failed to add";
        EXPECT_EQ(retrieveScanfolderById.m_scanFolderID, scanFolder.m_scanFolderID) << "AssetProcessingStateDataTest Failed - scan folder failed to add";

        //retrieve the one we just made by portable key
        ScanFolderDatabaseEntry retrieveScanfolderByScanPath;
        EXPECT_TRUE(m_connection.GetScanFolderByPortableKey("rootportkey", retrieveScanfolderByScanPath));
        EXPECT_NE(retrieveScanfolderByScanPath.m_scanFolderID, AzToolsFramework::AssetDatabase::InvalidEntryId) << "AssetProcessingStateDataTest Failed - scan folder failed to add";
        EXPECT_EQ(retrieveScanfolderByScanPath.m_scanFolderID, scanFolder.m_scanFolderID) << "AssetProcessingStateDataTest Failed - scan folder failed to add";

        //add another folder
        ScanFolderDatabaseEntry gameScanFolderEntry("c:/O3DE/game", "game", "gameportkey");
        EXPECT_TRUE(m_connection.SetScanFolder(gameScanFolderEntry));
        EXPECT_NE(gameScanFolderEntry.m_scanFolderID, AzToolsFramework::AssetDatabase::InvalidEntryId) << "AssetProcessingStateDataTest Failed - scan folder failed to add";
        EXPECT_NE(gameScanFolderEntry.m_scanFolderID, scanFolder.m_scanFolderID)<< "AssetProcessingStateDataTest Failed - scan folder failed to add";

        //get all scan folders, there should only the two we added
        scanFolders.clear();
        EXPECT_TRUE(m_connection.GetScanFolders(scanFolders));
        EXPECT_EQ(scanFolders.size(), 2);
        EXPECT_TRUE(ScanFoldersContainScanPath(scanFolders, "c:/O3DE/dev2"));
        EXPECT_TRUE(ScanFoldersContainScanPath(scanFolders, "c:/O3DE/game"));
        EXPECT_TRUE(ScanFoldersContainScanFolderID(scanFolders, scanFolder.m_scanFolderID));
        EXPECT_TRUE(ScanFoldersContainScanFolderID(scanFolders, gameScanFolderEntry.m_scanFolderID));

        //remove the game scan folder
        EXPECT_TRUE(m_connection.RemoveScanFolder(848475));//should return true even if it doesn't exist, false only means SQL failed
        EXPECT_TRUE(m_connection.RemoveScanFolder(gameScanFolderEntry.m_scanFolderID));

        //get all scan folders again, there should now only the first we added
        scanFolders.clear();
        EXPECT_TRUE(m_connection.GetScanFolders(scanFolders));
        EXPECT_EQ(scanFolders.size(), 1);
        EXPECT_TRUE(ScanFoldersContainScanPath(scanFolders, "c:/O3DE/dev2"));
        EXPECT_TRUE(ScanFoldersContainScanFolderID(scanFolders, scanFolder.m_scanFolderID));

        //add another folder again
        gameScanFolderEntry = ScanFolderDatabaseEntry("c:/O3DE/game", "game", "gameportkey2");
        EXPECT_TRUE(m_connection.SetScanFolder(gameScanFolderEntry));
        EXPECT_NE(gameScanFolderEntry.m_scanFolderID, AzToolsFramework::AssetDatabase::InvalidEntryId) << "AssetProcessingStateDataTest Failed - scan folder failed to add";
        EXPECT_NE(gameScanFolderEntry.m_scanFolderID, scanFolder.m_scanFolderID) << "AssetProcessingStateDataTest Failed - scan folder failed to add";

        //get all scan folders, there should only the two we added
        scanFolders.clear();
        EXPECT_TRUE(m_connection.GetScanFolders(scanFolders));
        EXPECT_EQ(scanFolders.size(), 2);
        EXPECT_TRUE(ScanFoldersContainScanPath(scanFolders, "c:/O3DE/dev2"));
        EXPECT_TRUE(ScanFoldersContainScanPath(scanFolders, "c:/O3DE/game"));
        EXPECT_TRUE(ScanFoldersContainScanFolderID(scanFolders, scanFolder.m_scanFolderID));
        EXPECT_TRUE(ScanFoldersContainScanFolderID(scanFolders, gameScanFolderEntry.m_scanFolderID));

        //remove scan folder by using a container
        ScanFolderDatabaseEntryContainer tempScanFolderDatabaseEntryContainer; // note that on clang, its illegal to call a non-const function with a temp variable container as the param
        EXPECT_TRUE(m_connection.RemoveScanFolders(tempScanFolderDatabaseEntryContainer)); // call with empty
        EXPECT_TRUE(m_connection.RemoveScanFolders(scanFolders));
        scanFolders.clear();
        EXPECT_FALSE(m_connection.GetScanFolders(scanFolders));

        ///////////////////////////////////////////////////////////
        //setup for sources tests
        //for the rest of the test lets add the original scan folder
        scanFolder = ScanFolderDatabaseEntry("c:/O3DE/dev", "dev", "devkey2");
        EXPECT_TRUE(m_connection.SetScanFolder(scanFolder));
        ///////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        //Sources
        auto SourcesContainSourceID = [](const SourceDatabaseEntryContainer& sources, AZ::s64 sourceID) -> bool
        {
            for (const auto& source : sources)
            {
                if (source.m_sourceID == sourceID)
                {
                    return true;
                }
            }
            return false;
        };

        auto SourcesContainSourceName = [](const SourceDatabaseEntryContainer& sources, const char* sourceName) -> bool
        {
            for (const auto& source : sources)
            {
                if (source.m_sourceName == sourceName)
                {
                    return true;
                }
            }
            return false;
        };

        auto SourcesContainSourceGuid = [](const SourceDatabaseEntryContainer& sources, AZ::Uuid sourceGuid) -> bool
        {
            for (const auto& source : sources)
            {
                if (source.m_sourceGuid == sourceGuid)
                {
                    return true;
                }
            }
            return false;
        };

        //there are no sources yet so trying to find one should fail
        sources.clear();
        EXPECT_FALSE(m_connection.GetSources(sources));
        EXPECT_FALSE(m_connection.GetSourceBySourceID(3443, source));
        EXPECT_FALSE(m_connection.GetSourceBySourceGuid(AZ::Uuid::Create(), source));
        EXPECT_FALSE(m_connection.GetSourcesLikeSourceName("source", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::Raw, sources));
        EXPECT_FALSE(m_connection.GetSourcesLikeSourceName("source", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::StartsWith, sources));
        EXPECT_FALSE(m_connection.GetSourcesLikeSourceName("source", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::EndsWith, sources));
        EXPECT_FALSE(m_connection.GetSourcesLikeSourceName("source", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::Matches, sources));

        //trying to add a source without a valid scan folder pk should fail
        source = SourceDatabaseEntry(234234, "SomeSource1.tif", validSourceGuid1, "");
        {
            UnitTestUtils::AssertAbsorber absorber;
            EXPECT_FALSE(m_connection.SetSource(source));
            EXPECT_GE(absorber.m_numWarningsAbsorbed, 0);
        }

        //setting a valid scan folder pk should allow it to be added
        source = SourceDatabaseEntry(scanFolder.m_scanFolderID, "SomeSource1.tif", validSourceGuid1, "tEsTFingerPrint_TEST");
        EXPECT_TRUE(m_connection.SetSource(source));
        EXPECT_NE(source.m_sourceID, AzToolsFramework::AssetDatabase::InvalidEntryId) << "AssetProcessingStateDataTest Failed - source failed to add";

        //get all sources, there should only the one we added
        sources.clear();
        EXPECT_TRUE(m_connection.GetSources(sources));
        EXPECT_EQ(sources.size(), 1);
        EXPECT_EQ(sources[0].m_analysisFingerprint, "tEsTFingerPrint_TEST");
        EXPECT_TRUE(SourcesContainSourceName(sources, "SomeSource1.tif"));
        EXPECT_TRUE(SourcesContainSourceID(sources, source.m_sourceID));
        EXPECT_TRUE(SourcesContainSourceGuid(sources, source.m_sourceGuid));

        //add the same source again, should not add another because it already exists, so we should get the same id
        SourceDatabaseEntry dupeSource(source);
        dupeSource.m_sourceID = AzToolsFramework::AssetDatabase::InvalidEntryId;
        EXPECT_TRUE(m_connection.SetSource(dupeSource));
        EXPECT_EQ(dupeSource.m_sourceID, source.m_sourceID) << "AssetProcessingStateDataTest Failed - scan folder failed to add";

        //get all sources, there should still only the one we added
        sources.clear();
        EXPECT_TRUE(m_connection.GetSources(sources));
        EXPECT_EQ(sources.size(), 1);
        EXPECT_EQ(sources[0].m_analysisFingerprint, "tEsTFingerPrint_TEST");
        EXPECT_TRUE(SourcesContainSourceName(sources, "SomeSource1.tif"));
        EXPECT_TRUE(SourcesContainSourceID(sources, source.m_sourceID));
        EXPECT_TRUE(SourcesContainSourceGuid(sources, source.m_sourceGuid));

        // make sure that changing a field like fingerprint writes the new field to database but does not
        // add a new entry (ie, its just modifying existing data)
        SourceDatabaseEntry sourceWithDifferentFingerprint(source);
        sourceWithDifferentFingerprint.m_analysisFingerprint = "otherFingerprint";
        EXPECT_TRUE(m_connection.SetSource(sourceWithDifferentFingerprint));
        EXPECT_EQ(sourceWithDifferentFingerprint.m_sourceID, source.m_sourceID);
        sources.clear();
        EXPECT_TRUE(m_connection.GetSources(sources));
        EXPECT_EQ(sources.size(), 1);
        EXPECT_EQ(sources[0].m_analysisFingerprint, "otherFingerprint");
        EXPECT_TRUE(SourcesContainSourceName(sources, "SomeSource1.tif"));
        EXPECT_TRUE(SourcesContainSourceID(sources, source.m_sourceID));
        EXPECT_TRUE(SourcesContainSourceGuid(sources, source.m_sourceGuid));

        //add the same source again, but change the scan folder.  This should NOT add a new source - even if we don't know what the sourceID is:
        ScanFolderDatabaseEntry scanfolder2 = ScanFolderDatabaseEntry("c:/O3DE/dev2", "dev2", "devkey3");
        EXPECT_TRUE(m_connection.SetScanFolder(scanfolder2));

        SourceDatabaseEntry dupeSource2(source);
        dupeSource2.m_scanFolderPK = scanfolder2.m_scanFolderID;
        dupeSource2.m_analysisFingerprint = "new different fingerprint";
        dupeSource2.m_sourceID = AzToolsFramework::AssetDatabase::InvalidEntryId;
        EXPECT_TRUE(m_connection.SetSource(dupeSource2));
        EXPECT_EQ(dupeSource2.m_sourceID, source.m_sourceID) << "AssetProcessingStateDataTest Failed - scan folder failed to add";

        //get all sources, there should still only the one we added
        sources.clear();
        EXPECT_TRUE(m_connection.GetSources(sources));
        EXPECT_EQ(sources.size(), 1);
        EXPECT_EQ(sources[0].m_analysisFingerprint, "new different fingerprint"); // verify that this column IS updated.
        EXPECT_TRUE(SourcesContainSourceName(sources, "SomeSource1.tif"));
        EXPECT_TRUE(SourcesContainSourceID(sources, source.m_sourceID));
        EXPECT_TRUE(SourcesContainSourceGuid(sources, source.m_sourceGuid));

        //add the same source again, but change the scan folder back  This should NOT add a new source - this time we do know what the source ID is!
        SourceDatabaseEntry dupeSource3(source);
        dupeSource3.m_scanFolderPK = scanFolder.m_scanFolderID; // changing it back here.
        EXPECT_TRUE(m_connection.SetSource(dupeSource3));
        EXPECT_EQ(dupeSource3.m_sourceID, source.m_sourceID) << "AssetProcessingStateDataTest Failed - scan folder failed to add";

        //get all sources, there should still only the one we added
        sources.clear();
        EXPECT_TRUE(m_connection.GetSources(sources));
        EXPECT_EQ(sources.size(), 1);
        EXPECT_TRUE(SourcesContainSourceName(sources, "SomeSource1.tif"));
        EXPECT_TRUE(SourcesContainSourceID(sources, source.m_sourceID));
        EXPECT_TRUE(SourcesContainSourceGuid(sources, source.m_sourceGuid));

        // remove the extra scan folder, make sure it doesn't drop the source since it should now be bound to the original scan folder agian
        EXPECT_TRUE(m_connection.RemoveScanFolder(scanfolder2.m_scanFolderID));
        sources.clear();
        EXPECT_TRUE(m_connection.GetSources(sources));
        EXPECT_EQ(sources.size(), 1);
        EXPECT_TRUE(SourcesContainSourceName(sources, "SomeSource1.tif"));
        EXPECT_TRUE(SourcesContainSourceID(sources, source.m_sourceID));
        EXPECT_TRUE(SourcesContainSourceGuid(sources, source.m_sourceGuid));

        //try retrieving this source by id
        SourceDatabaseEntry retrieveSourceBySourceID;
        EXPECT_TRUE(m_connection.GetSourceBySourceID(source.m_sourceID, retrieveSourceBySourceID));
        EXPECT_NE(retrieveSourceBySourceID.m_sourceID, AzToolsFramework::AssetDatabase::InvalidEntryId) << "AssetProcessingStateDataTest Failed - GetSourceBySourceID failed";
        EXPECT_EQ(retrieveSourceBySourceID.m_sourceID, source.m_sourceID) << "AssetProcessingStateDataTest Failed - GetSourceBySourceID failed";
        EXPECT_EQ(retrieveSourceBySourceID.m_scanFolderPK, source.m_scanFolderPK) << "AssetProcessingStateDataTest Failed - GetSourceBySourceID failed";
        EXPECT_EQ(retrieveSourceBySourceID.m_sourceGuid, source.m_sourceGuid) << "AssetProcessingStateDataTest Failed - GetSourceBySourceID failed";
        EXPECT_EQ(retrieveSourceBySourceID.m_sourceName, source.m_sourceName) << "AssetProcessingStateDataTest Failed - GetSourceBySourceID failed";

        //try retrieving this source by guid
        SourceDatabaseEntry retrieveSourceBySourceGuid;
        EXPECT_TRUE(m_connection.GetSourceBySourceGuid(source.m_sourceGuid, retrieveSourceBySourceGuid));
        EXPECT_NE(retrieveSourceBySourceGuid.m_sourceID, AzToolsFramework::AssetDatabase::InvalidEntryId) << "AssetProcessingStateDataTest Failed - GetSourceBySourceID failed";
        EXPECT_EQ(retrieveSourceBySourceGuid.m_sourceID, source.m_sourceID) << "AssetProcessingStateDataTest Failed - GetSourceBySourceID failed";
        EXPECT_EQ(retrieveSourceBySourceGuid.m_scanFolderPK, source.m_scanFolderPK) << "AssetProcessingStateDataTest Failed - GetSourceBySourceID failed";
        EXPECT_EQ(retrieveSourceBySourceGuid.m_sourceGuid, source.m_sourceGuid) << "AssetProcessingStateDataTest Failed - GetSourceBySourceID failed";
        EXPECT_EQ(retrieveSourceBySourceGuid.m_sourceName, source.m_sourceName) << "AssetProcessingStateDataTest Failed - GetSourceBySourceID failed";

        //try retrieving this source by source name
        sources.clear();
        EXPECT_FALSE(m_connection.GetSourcesLikeSourceName("Source1.tif", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::Raw, sources));
        sources.clear();
        EXPECT_FALSE(m_connection.GetSourcesLikeSourceName("_SomeSource1_", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::Raw, sources));
        sources.clear();
        EXPECT_TRUE(m_connection.GetSourcesLikeSourceName("SomeSource1%", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::Raw, sources));
        EXPECT_EQ(sources.size(), 1);
        EXPECT_TRUE(SourcesContainSourceName(sources, "SomeSource1.tif"));
        EXPECT_TRUE(SourcesContainSourceID(sources, source.m_sourceID));
        EXPECT_TRUE(SourcesContainSourceGuid(sources, source.m_sourceGuid));
        sources.clear();
        EXPECT_TRUE(m_connection.GetSourcesLikeSourceName("%SomeSource1%", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::Raw, sources));
        EXPECT_EQ(sources.size(), 1);
        EXPECT_TRUE(SourcesContainSourceName(sources, "SomeSource1.tif"));
        EXPECT_TRUE(SourcesContainSourceID(sources, source.m_sourceID));
        EXPECT_TRUE(SourcesContainSourceGuid(sources, source.m_sourceGuid));
        sources.clear();
        EXPECT_FALSE(m_connection.GetSourcesLikeSourceName("Source1", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::StartsWith, sources));
        sources.clear();
        EXPECT_TRUE(m_connection.GetSourcesLikeSourceName("Some", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::StartsWith, sources));
        EXPECT_EQ(sources.size(), 1);
        EXPECT_TRUE(SourcesContainSourceName(sources, "SomeSource1.tif"));
        EXPECT_TRUE(SourcesContainSourceID(sources, source.m_sourceID));
        EXPECT_TRUE(SourcesContainSourceGuid(sources, source.m_sourceGuid));
        sources.clear();
        EXPECT_FALSE(m_connection.GetSourcesLikeSourceName("SomeSource", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::EndsWith, sources));
        sources.clear();
        EXPECT_TRUE(m_connection.GetSourcesLikeSourceName(".tif", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::EndsWith, sources));
        EXPECT_EQ(sources.size(), 1);
        EXPECT_TRUE(SourcesContainSourceName(sources, "SomeSource1.tif"));
        EXPECT_TRUE(SourcesContainSourceID(sources, source.m_sourceID));
        EXPECT_TRUE(SourcesContainSourceGuid(sources, source.m_sourceGuid));
        sources.clear();
        EXPECT_FALSE(m_connection.GetSourcesLikeSourceName("blah", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::Matches, sources));
        sources.clear();
        EXPECT_TRUE(m_connection.GetSourcesLikeSourceName("meSour", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::Matches, sources));
        EXPECT_EQ(sources.size(), 1);
        EXPECT_TRUE(SourcesContainSourceName(sources, "SomeSource1.tif"));
        EXPECT_TRUE(SourcesContainSourceID(sources, source.m_sourceID));
        EXPECT_TRUE(SourcesContainSourceGuid(sources, source.m_sourceGuid));


        //remove a source
        EXPECT_TRUE(m_connection.RemoveSource(432234));//should return true even if it doesn't exist, false only if SQL failed
        EXPECT_TRUE(m_connection.RemoveSource(source.m_sourceID));

        //get all sources, there shouldn't be any
        sources.clear();
        EXPECT_FALSE(m_connection.GetSources(sources));

        //Add two sources then delete the via container
        SourceDatabaseEntry source2(scanFolder.m_scanFolderID, "SomeSource2.tif", validSourceGuid2, "");
        EXPECT_TRUE(m_connection.SetSource(source2));
        SourceDatabaseEntry source3(scanFolder.m_scanFolderID, "SomeSource3.tif", validSourceGuid3, "");
        EXPECT_TRUE(m_connection.SetSource(source3));

        //get all sources, there should only the two we added
        sources.clear();
        EXPECT_TRUE(m_connection.GetSources(sources));
        EXPECT_EQ(sources.size(), 2);
        EXPECT_TRUE(SourcesContainSourceName(sources, "SomeSource2.tif"));
        EXPECT_TRUE(SourcesContainSourceName(sources, "SomeSource3.tif"));
        EXPECT_TRUE(SourcesContainSourceID(sources, source2.m_sourceID));
        EXPECT_TRUE(SourcesContainSourceGuid(sources, source2.m_sourceGuid));
        EXPECT_TRUE(SourcesContainSourceID(sources, source3.m_sourceID));
        EXPECT_TRUE(SourcesContainSourceGuid(sources, source3.m_sourceGuid));

        //Remove source via container
        SourceDatabaseEntryContainer tempSourceDatabaseEntryContainer;
        EXPECT_TRUE(m_connection.RemoveSources(tempSourceDatabaseEntryContainer)); // try it with an empty one.
        EXPECT_TRUE(m_connection.RemoveSources(sources));

        //get all sources, there should none
        sources.clear();
        EXPECT_FALSE(m_connection.GetSources(sources));

        //Add two sources then delete the via removing by scan folder id
        source2 = SourceDatabaseEntry(scanFolder.m_scanFolderID, "SomeSource2.tif", validSourceGuid2, "fingerprint");
        EXPECT_TRUE(m_connection.SetSource(source2));
        source3 = SourceDatabaseEntry(scanFolder.m_scanFolderID, "SomeSource3.tif", validSourceGuid3, "fingerprint");
        EXPECT_TRUE(m_connection.SetSource(source3));

        //remove all sources for a scan folder
        sources.clear();
        EXPECT_FALSE(m_connection.RemoveSourcesByScanFolderID(3245532));
        EXPECT_TRUE(m_connection.RemoveSourcesByScanFolderID(scanFolder.m_scanFolderID));

        //get all sources, there should none
        sources.clear();
        EXPECT_FALSE(m_connection.GetSources(sources));

        //Add two sources then delete the via removing the scan folder
        source2 = SourceDatabaseEntry(scanFolder.m_scanFolderID, "SomeSource2.tif", validSourceGuid2, "");
        EXPECT_TRUE(m_connection.SetSource(source2));
        source3 = SourceDatabaseEntry(scanFolder.m_scanFolderID, "SomeSource3.tif", validSourceGuid3, "");
        EXPECT_TRUE(m_connection.SetSource(source3));

        //remove the scan folder for these sources, the sources should cascade delete
        EXPECT_TRUE(m_connection.RemoveScanFolder(scanFolder.m_scanFolderID));

        //get all sources, there should none
        sources.clear();
        EXPECT_FALSE(m_connection.GetSources(sources));

        ////////////////////////////////////////////////////////////////
        //Setup for jobs tests by having a scan folder and some sources
        //Add a scan folder
        scanFolder = ScanFolderDatabaseEntry("c:/O3DE/dev", "dev", "devkey3");
        EXPECT_TRUE(m_connection.SetScanFolder(scanFolder));

        auto& config = m_appManager->m_platformConfig;
        config->AddScanFolder(AssetProcessor::ScanFolderInfo{scanFolder.m_scanFolder.c_str(), scanFolder.m_displayName.c_str(), scanFolder.m_portableKey.c_str(), false, true, {} , 0, scanFolder.m_scanFolderID});

        //Add some sources
        source = SourceDatabaseEntry(scanFolder.m_scanFolderID, "SomeSource1.tif", validSourceGuid1, "");
        EXPECT_TRUE(m_connection.SetSource(source));
        source2 = SourceDatabaseEntry(scanFolder.m_scanFolderID, "SomeSource2.tif", validSourceGuid2, "");
        EXPECT_TRUE(m_connection.SetSource(source2));
        source3 = SourceDatabaseEntry(scanFolder.m_scanFolderID, "SomeSource3.tif", validSourceGuid3, "");
        EXPECT_TRUE(m_connection.SetSource(source3));
        /////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        //Jobs
        auto JobsContainJobID = [](const JobDatabaseEntryContainer& jobs, AZ::s64 jobId) -> bool
        {
            for (const auto& job : jobs)
            {
                if (job.m_jobID == jobId)
                {
                    return true;
                }
            }
            return false;
        };

        auto JobsContainJobKey = [](const JobDatabaseEntryContainer& jobs, const char* jobKey) -> bool
        {
            for (const auto& job : jobs)
            {
                if (job.m_jobKey == jobKey)
                {
                    return true;
                }
            }
            return false;
        };

        auto JobsContainFingerprint = [](const JobDatabaseEntryContainer& jobs, AZ::u32 fingerprint) -> bool
        {
            for (const auto& job : jobs)
            {
                if (job.m_fingerprint == fingerprint)
                {
                    return true;
                }
            }
            return false;
        };

        auto JobsContainPlatform = [](const JobDatabaseEntryContainer& jobs, const char* platform) -> bool
        {
            for (const auto& job : jobs)
            {
                if (job.m_platform == platform)
                {
                    return true;
                }
            }
            return false;
        };

        auto JobsContainBuilderGuid = [](const JobDatabaseEntryContainer& jobs, AZ::Uuid builderGuid) -> bool
        {
            for (const auto& job : jobs)
            {
                if (job.m_builderGuid == builderGuid)
                {
                    return true;
                }
            }
            return false;
        };

        auto JobsContainStatus = [](const JobDatabaseEntryContainer& jobs, AzToolsFramework::AssetSystem::JobStatus status) -> bool
        {
            for (const auto& job : jobs)
            {
                if (job.m_status == status)
                {
                    return true;
                }
            }
            return false;
        };


        auto JobsContainRunKey = [](const JobDatabaseEntryContainer& jobs, AZ::u64 runKey) -> bool
        {
            for (const auto& job : jobs)
            {
                if (job.m_jobRunKey == runKey)
                {
                    return true;
                }
            }
            return false;
        };


        //there are no jobs yet so trying to find one should fail
        jobs.clear();
        EXPECT_FALSE(m_connection.GetJobs(jobs));
        EXPECT_FALSE(m_connection.GetJobByJobID(3443, job));
        EXPECT_FALSE(m_connection.GetJobsBySourceID(3234, jobs));
        EXPECT_FALSE(m_connection.GetJobsBySourceName(AssetProcessor::SourceAssetReference("c:/O3DE/dev/none"), jobs));

        //trying to add a job without a valid source pk should fail:
        {
            UnitTestUtils::AssertAbsorber absorber;
            job = JobDatabaseEntry(234234, "jobkey", validFingerprint1, "pc", validBuilderGuid1, statusQueued, 1);
            EXPECT_FALSE(m_connection.SetJob(job));
            EXPECT_GE(absorber.m_numWarningsAbsorbed, 0);
        }

        //trying to add a job with a valid source pk but an invalid job id  should fail:
        {
            UnitTestUtils::AssertAbsorber absorber;
            job = JobDatabaseEntry(source.m_sourceID, "jobkey", validFingerprint1, "pc", validBuilderGuid1, statusQueued, 0);
            EXPECT_FALSE(m_connection.SetJob(job));
            EXPECT_GE(absorber.m_numErrorsAbsorbed, 0);
        }

        //setting a valid scan folder pk should allow it to be added AND should tell you what the job ID will be.
        // the run key should be untouched.
        job = JobDatabaseEntry(source.m_sourceID, "jobKey1", validFingerprint1, "pc", validBuilderGuid1, statusQueued, 1);
        EXPECT_TRUE(m_connection.SetJob(job));
        EXPECT_NE(job.m_jobID, AzToolsFramework::AssetDatabase::InvalidEntryId);
        EXPECT_EQ(job.m_jobRunKey, 1);

        //get all jobs, there should only the one we added
        jobs.clear();
        EXPECT_TRUE(m_connection.GetJobs(jobs));
        EXPECT_EQ(jobs.size(), 1);
        EXPECT_TRUE(JobsContainJobID(jobs, job.m_jobID));
        EXPECT_TRUE(JobsContainJobKey(jobs, job.m_jobKey.c_str()));
        EXPECT_TRUE(JobsContainFingerprint(jobs, job.m_fingerprint));
        EXPECT_TRUE(JobsContainPlatform(jobs, job.m_platform.c_str()));
        EXPECT_TRUE(JobsContainBuilderGuid(jobs, job.m_builderGuid));
        EXPECT_TRUE(JobsContainStatus(jobs, job.m_status));
        EXPECT_TRUE(JobsContainRunKey(jobs, job.m_jobRunKey));

        //add the same job again, should not add another because it already exists, so we should get the same id
        JobDatabaseEntry dupeJob(job);
        dupeJob.m_jobID = AzToolsFramework::AssetDatabase::InvalidEntryId;
        EXPECT_TRUE(m_connection.SetJob(dupeJob));
        EXPECT_EQ(dupeJob, job) << "AssetProcessingStateDataTest Failed - SetJob failed to add";

        //get all jobs, there should still only the one we added
        jobs.clear();
        EXPECT_TRUE(m_connection.GetJobs(jobs));
        EXPECT_EQ(jobs.size(), 1);
        EXPECT_TRUE(JobsContainJobID(jobs, job.m_jobID));
        EXPECT_TRUE(JobsContainJobKey(jobs, job.m_jobKey.c_str()));
        EXPECT_TRUE(JobsContainFingerprint(jobs, job.m_fingerprint));
        EXPECT_TRUE(JobsContainPlatform(jobs, job.m_platform.c_str()));
        EXPECT_TRUE(JobsContainBuilderGuid(jobs, job.m_builderGuid));
        EXPECT_TRUE(JobsContainStatus(jobs, job.m_status));

        //try retrieving this source by id
        EXPECT_TRUE(m_connection.GetJobByJobID(job.m_jobID, job));
        EXPECT_NE(job.m_jobID, AzToolsFramework::AssetDatabase::InvalidEntryId) << "AssetProcessingStateDataTest Failed - GetJobByJobID failed";

        //try retrieving jobs by source id
        jobs.clear();
        EXPECT_TRUE(m_connection.GetJobsBySourceID(source.m_sourceID, jobs));
        EXPECT_EQ(jobs.size(), 1);
        EXPECT_TRUE(JobsContainJobID(jobs, job.m_jobID));
        EXPECT_TRUE(JobsContainJobKey(jobs, job.m_jobKey.c_str()));
        EXPECT_TRUE(JobsContainFingerprint(jobs, job.m_fingerprint));
        EXPECT_TRUE(JobsContainPlatform(jobs, job.m_platform.c_str()));
        EXPECT_TRUE(JobsContainBuilderGuid(jobs, job.m_builderGuid));
        EXPECT_TRUE(JobsContainStatus(jobs, job.m_status));

        //try retrieving jobs by source name
        jobs.clear();
        EXPECT_TRUE(m_connection.GetJobsBySourceName(AssetProcessor::SourceAssetReference(source.m_scanFolderPK, source.m_sourceName.c_str()), jobs));
        EXPECT_EQ(jobs.size(), 1);
        EXPECT_TRUE(JobsContainJobID(jobs, job.m_jobID));
        EXPECT_TRUE(JobsContainJobKey(jobs, job.m_jobKey.c_str()));
        EXPECT_TRUE(JobsContainFingerprint(jobs, job.m_fingerprint));
        EXPECT_TRUE(JobsContainPlatform(jobs, job.m_platform.c_str()));
        EXPECT_TRUE(JobsContainBuilderGuid(jobs, job.m_builderGuid));
        EXPECT_TRUE(JobsContainStatus(jobs, job.m_status));

        //remove a job
        EXPECT_TRUE(m_connection.RemoveJob(432234));
        EXPECT_TRUE(m_connection.RemoveJob(job.m_jobID));

        //get all jobs, there shouldn't be any
        jobs.clear();
        EXPECT_FALSE(m_connection.GetJobs(jobs));

        //Add two jobs then delete the via container
        JobDatabaseEntry job2(source2.m_sourceID, "jobkey2", validFingerprint2, "pc", validBuilderGuid2, statusQueued, 2);
        EXPECT_TRUE(m_connection.SetJob(job2));
        JobDatabaseEntry job3(source3.m_sourceID, "jobkey3", validFingerprint3, "pc", validBuilderGuid3, statusQueued, 3);
        EXPECT_TRUE(m_connection.SetJob(job3));

        //get all jobs, there should be 3
        jobs.clear();
        EXPECT_TRUE(m_connection.GetJobs(jobs));
        EXPECT_EQ(jobs.size(), 2);
        EXPECT_TRUE(JobsContainJobID(jobs, job2.m_jobID));
        EXPECT_TRUE(JobsContainJobKey(jobs, job2.m_jobKey.c_str()));
        EXPECT_TRUE(JobsContainFingerprint(jobs, job2.m_fingerprint));
        EXPECT_TRUE(JobsContainPlatform(jobs, job2.m_platform.c_str()));
        EXPECT_TRUE(JobsContainBuilderGuid(jobs, job2.m_builderGuid));
        EXPECT_TRUE(JobsContainStatus(jobs, job2.m_status));
        EXPECT_TRUE(JobsContainJobID(jobs, job3.m_jobID));
        EXPECT_TRUE(JobsContainJobKey(jobs, job3.m_jobKey.c_str()));
        EXPECT_TRUE(JobsContainFingerprint(jobs, job3.m_fingerprint));
        EXPECT_TRUE(JobsContainPlatform(jobs, job3.m_platform.c_str()));
        EXPECT_TRUE(JobsContainBuilderGuid(jobs, job3.m_builderGuid));
        EXPECT_TRUE(JobsContainStatus(jobs, job3.m_status));

        //Remove job via container
        JobDatabaseEntryContainer tempJobDatabaseEntryContainer;
        EXPECT_TRUE(m_connection.RemoveJobs(tempJobDatabaseEntryContainer)); // make sure it works on an empty container.
        EXPECT_TRUE(m_connection.RemoveJobs(jobs));

        //get all jobs, there should none
        jobs.clear();
        EXPECT_FALSE(m_connection.GetJobs(jobs));

        //Add two jobs then delete the via removing by source
        job2 = JobDatabaseEntry(source.m_sourceID, "jobkey2", validFingerprint2, "pc", validBuilderGuid2, statusQueued, 4);
        EXPECT_TRUE(m_connection.SetJob(job2));
        job3 = JobDatabaseEntry(source.m_sourceID, "jobkey3", validFingerprint3, "pc", validBuilderGuid3, statusQueued, 5);
        EXPECT_TRUE(m_connection.SetJob(job3));

        //remove the scan folder for these jobs, the jobs should cascade delete
        EXPECT_TRUE(m_connection.RemoveSource(source.m_sourceID));

        //get all jobs, there should none
        jobs.clear();
        EXPECT_FALSE(m_connection.GetJobs(jobs));

        ////////////////////////////////////////////////////////////////
        //Setup for product tests by having a some sources and jobs
        source = SourceDatabaseEntry(scanFolder.m_scanFolderID, "SomeSource1.tif", validSourceGuid1, "");
        EXPECT_TRUE(m_connection.SetSource(source));

        //Add jobs
        job = JobDatabaseEntry(source.m_sourceID, "jobkey1", validFingerprint1, "pc", validBuilderGuid1, statusCompleted, 6);
        EXPECT_TRUE(m_connection.SetJob(job));
        job2 = JobDatabaseEntry(source.m_sourceID, "jobkey2", validFingerprint2, "pc", validBuilderGuid2, statusCompleted, 7);
        EXPECT_TRUE(m_connection.SetJob(job2));
        job3 = JobDatabaseEntry(source.m_sourceID, "jobkey3", validFingerprint3, "pc", validBuilderGuid3, statusCompleted, 8);
        EXPECT_TRUE(m_connection.SetJob(job3));
        /////////////////////////////////////////////////////////////////

        auto ProductsContainProductID = [](const ProductDatabaseEntryContainer& products, AZ::s64 productId) -> bool
        {
            for (const auto& product : products)
            {
                if (product.m_productID == productId)
                {
                    return true;
                }
            }
            return false;
        };


        //Add jobs
        job = JobDatabaseEntry(source.m_sourceID, "jobkey1", validFingerprint1, "pc", validBuilderGuid1, statusCompleted, 9);
        EXPECT_TRUE(m_connection.SetJob(job));
        job2 = JobDatabaseEntry(source.m_sourceID, "jobkey2", validFingerprint2, "pc", validBuilderGuid2, statusCompleted, 10);
        EXPECT_TRUE(m_connection.SetJob(job2));
        job3 = JobDatabaseEntry(source.m_sourceID, "jobkey3", validFingerprint3, "pc", validBuilderGuid3, statusCompleted, 11);
        EXPECT_TRUE(m_connection.SetJob(job3));

        //Add two products then delete the via removing the job
        ProductDatabaseEntry product2 = ProductDatabaseEntry(job.m_jobID, 2, "SomeProduct2.dds", validAssetType2);
        EXPECT_TRUE(m_connection.SetProduct(product2));
        ProductDatabaseEntry product3 = ProductDatabaseEntry(job.m_jobID, 3, "SomeProduct3.dds", validAssetType3);
        EXPECT_TRUE(m_connection.SetProduct(product3));

        //the products should cascade delete
        EXPECT_TRUE(m_connection.RemoveSource(source.m_sourceID));

        //get all products, there should none
        products.clear();
        EXPECT_FALSE(m_connection.GetProducts(products));

        // ---- test legacy subIds table ----

        // setup:
        // SomeSource1.tif
        //   jobkey1
        //     someproduct1
        //        legacy ids...

        source = SourceDatabaseEntry(scanFolder.m_scanFolderID, "SomeSource1.tif", validSourceGuid1, "");
        EXPECT_TRUE(m_connection.SetSource(source));

        job = JobDatabaseEntry(source.m_sourceID, "jobkey1", validFingerprint1, "pc", validBuilderGuid1, statusCompleted, 6);
        EXPECT_TRUE(m_connection.SetJob(job));

        product = ProductDatabaseEntry(job.m_jobID, 1, "SomeProduct1.dds", validAssetType1);
        product2 = ProductDatabaseEntry(job.m_jobID, 2, "SomeProduct2.dds", validAssetType1);
        EXPECT_TRUE(m_connection.SetProduct(product));
        EXPECT_TRUE(m_connection.SetProduct(product2));

        // test invalid insert for non existant legacy subids.
        LegacySubIDsEntry legacyEntry(1, product.m_productID, 3);
        {
            UnitTestUtils::AssertAbsorber absorber;
            EXPECT_FALSE(m_connection.CreateOrUpdateLegacySubID(legacyEntry));
            EXPECT_GT(absorber.m_numWarningsAbsorbed, 0);
        }

        // test invalid insert for non-existant legacy product FK constraint
        legacyEntry = LegacySubIDsEntry(AzToolsFramework::AssetDatabase::InvalidEntryId, 9999, 3);
        {
            UnitTestUtils::AssertAbsorber absorber;
            EXPECT_FALSE(m_connection.CreateOrUpdateLegacySubID(legacyEntry));
            EXPECT_GT(absorber.m_numWarningsAbsorbed, 0);
        }

        // test valid insert of another for same product
        legacyEntry = LegacySubIDsEntry(AzToolsFramework::AssetDatabase::InvalidEntryId, product.m_productID, 3);
        EXPECT_TRUE(m_connection.CreateOrUpdateLegacySubID(legacyEntry));
        AZ::s64 newPK = legacyEntry.m_subIDsEntryID;
        EXPECT_NE(newPK, AzToolsFramework::AssetDatabase::InvalidEntryId); // it should have also updated the PK

        legacyEntry = LegacySubIDsEntry(AzToolsFramework::AssetDatabase::InvalidEntryId, product.m_productID, 4);
        EXPECT_TRUE(m_connection.CreateOrUpdateLegacySubID(legacyEntry));
        EXPECT_NE(legacyEntry.m_subIDsEntryID, AzToolsFramework::AssetDatabase::InvalidEntryId); // it should have also updated the PK
        EXPECT_NE(legacyEntry.m_subIDsEntryID, newPK); // pk should be unique

                                                                     // test valid insert of another for different product
        legacyEntry = LegacySubIDsEntry(AzToolsFramework::AssetDatabase::InvalidEntryId, product2.m_productID, 5);
        EXPECT_TRUE(m_connection.CreateOrUpdateLegacySubID(legacyEntry));

        // test that the ones inserted can be retrieved.
        AZStd::vector<LegacySubIDsEntry> entriesReturned;
        auto handler = [&entriesReturned](LegacySubIDsEntry& entry)
        {
            entriesReturned.push_back(entry);
            return true;
        };
        EXPECT_TRUE(m_connection.QueryLegacySubIdsByProductID(product.m_productID, handler));
        EXPECT_EQ(entriesReturned.size(), 2);

        bool foundSubID3 = false;
        bool foundSubID4 = false;
        for (const LegacySubIDsEntry& entryFound : entriesReturned)
        {
            EXPECT_NE(entryFound.m_subIDsEntryID, AzToolsFramework::AssetDatabase::InvalidEntryId);
            EXPECT_EQ(entryFound.m_productPK, product.m_productID);
            if (entryFound.m_subID == 3)
            {
                foundSubID3 = true;
            }
            else if (entryFound.m_subID == 4)
            {
                foundSubID4 = true;
            }
        }

        EXPECT_TRUE(foundSubID3);
        EXPECT_TRUE(foundSubID4);

        entriesReturned.clear();

        EXPECT_TRUE(m_connection.QueryLegacySubIdsByProductID(product2.m_productID, handler));
        EXPECT_EQ(entriesReturned.size(), 1);
        EXPECT_NE(entriesReturned[0].m_subIDsEntryID, AzToolsFramework::AssetDatabase::InvalidEntryId);
        EXPECT_EQ(entriesReturned[0].m_productPK, product2.m_productID);
        EXPECT_EQ(entriesReturned[0].m_subID, 5);

        // test UPDATE -> overwrite existing.
        entriesReturned[0].m_subID = 6;
        EXPECT_TRUE(m_connection.CreateOrUpdateLegacySubID(entriesReturned[0]));
        entriesReturned.clear();

        EXPECT_TRUE(m_connection.QueryLegacySubIdsByProductID(product2.m_productID, handler));
        EXPECT_EQ(entriesReturned.size(), 1);
        EXPECT_NE(entriesReturned[0].m_subIDsEntryID, AzToolsFramework::AssetDatabase::InvalidEntryId);
        EXPECT_EQ(entriesReturned[0].m_productPK, product2.m_productID);
        EXPECT_EQ(entriesReturned[0].m_subID, 6);

        // test delete by product ID
        EXPECT_TRUE(m_connection.RemoveLegacySubIDsByProductID(product2.m_productID));
        entriesReturned.clear();

        EXPECT_TRUE(m_connection.QueryLegacySubIdsByProductID(product2.m_productID, handler));
        EXPECT_TRUE(entriesReturned.empty());

        // test delete by PK.  The prior entries should be here for product1. This also makes sure the above
        // delete statement didn't delete more than it should have.

        EXPECT_TRUE(m_connection.QueryLegacySubIdsByProductID(product.m_productID, handler));
        EXPECT_EQ(entriesReturned.size(), 2);

        AZ::s64 toRemove = entriesReturned[0].m_subIDsEntryID;
        AZ::u32 removingSubID = entriesReturned[0].m_subID;

        EXPECT_TRUE(m_connection.RemoveLegacySubID(toRemove));
        entriesReturned.clear();
        EXPECT_TRUE(m_connection.QueryLegacySubIdsByProductID(product.m_productID, handler));
        EXPECT_EQ(entriesReturned.size(), 1);
        EXPECT_NE(entriesReturned[0].m_subIDsEntryID, AzToolsFramework::AssetDatabase::InvalidEntryId);
        EXPECT_NE(entriesReturned[0].m_subIDsEntryID, toRemove);
        EXPECT_EQ(entriesReturned[0].m_productPK, product.m_productID);
        EXPECT_NE(entriesReturned[0].m_subID, removingSubID);

        ////////////////////////////////////////////////////////////////
        //Setup for product dependency tests by having a some sources and jobs
        source = SourceDatabaseEntry(scanFolder.m_scanFolderID, "SomeSource1.tif", validSourceGuid1, "");
        EXPECT_TRUE(m_connection.SetSource(source));
        source2 = SourceDatabaseEntry(scanFolder.m_scanFolderID, "SomeSource2.tif", validSourceGuid2, "");
        EXPECT_TRUE(m_connection.SetSource(source2));
        source3 = SourceDatabaseEntry(scanFolder.m_scanFolderID, "SomeSource3.tif", validSourceGuid3, "");
        EXPECT_TRUE(m_connection.SetSource(source3));
        SourceDatabaseEntry source4 = SourceDatabaseEntry(scanFolder.m_scanFolderID, "SomeSource4.tif", validSourceGuid4, "");
        EXPECT_TRUE(m_connection.SetSource(source4));
        SourceDatabaseEntry source5 = SourceDatabaseEntry(scanFolder.m_scanFolderID, "SomeSource5.tif", validSourceGuid5, "");
        EXPECT_TRUE(m_connection.SetSource(source5));
        SourceDatabaseEntry source6 = SourceDatabaseEntry(scanFolder.m_scanFolderID, "SomeSource6.tif", validSourceGuid6, "");
        EXPECT_TRUE(m_connection.SetSource(source6));

        //Add jobs
        job = JobDatabaseEntry(source.m_sourceID, "jobkey1", validFingerprint1, "pc", validBuilderGuid1, statusCompleted, 6);
        EXPECT_TRUE(m_connection.SetJob(job));
        job2 = JobDatabaseEntry(source2.m_sourceID, "jobkey2", validFingerprint2, "pc", validBuilderGuid2, statusCompleted, 7);
        EXPECT_TRUE(m_connection.SetJob(job2));
        job3 = JobDatabaseEntry(source3.m_sourceID, "jobkey3", validFingerprint3, "pc", validBuilderGuid3, statusCompleted, 8);
        EXPECT_TRUE(m_connection.SetJob(job3));
        JobDatabaseEntry job4 = JobDatabaseEntry(source4.m_sourceID, "jobkey4", validFingerprint4, "pc", validBuilderGuid4, statusCompleted, 9);
        EXPECT_TRUE(m_connection.SetJob(job4));
        JobDatabaseEntry job5 = JobDatabaseEntry(source5.m_sourceID, "jobkey5", validFingerprint5, "pc", validBuilderGuid5, statusCompleted, 10);
        EXPECT_TRUE(m_connection.SetJob(job5));
        JobDatabaseEntry job6 = JobDatabaseEntry(source6.m_sourceID, "jobkey6", validFingerprint6, "pc", validBuilderGuid6, statusCompleted, 11);
        EXPECT_TRUE(m_connection.SetJob(job6));

        //Add products
        product = ProductDatabaseEntry(job.m_jobID, 1, "SomeProduct1.dds", validAssetType1);
        EXPECT_TRUE(m_connection.SetProduct(product));
        product2 = ProductDatabaseEntry(job2.m_jobID, 2, "SomeProduct2.dds", validAssetType2);
        EXPECT_TRUE(m_connection.SetProduct(product2));
        product3 = ProductDatabaseEntry(job3.m_jobID, 3, "SomeProduct3.dds", validAssetType3);
        EXPECT_TRUE(m_connection.SetProduct(product3));
        ProductDatabaseEntry product4 = ProductDatabaseEntry(job4.m_jobID, 4, "SomeProduct4.dds", validAssetType4);
        EXPECT_TRUE(m_connection.SetProduct(product4));
        ProductDatabaseEntry product5 = ProductDatabaseEntry(job5.m_jobID, 5, "SomeProduct5.dds", validAssetType5);
        EXPECT_TRUE(m_connection.SetProduct(product5));
        ProductDatabaseEntry product6 = ProductDatabaseEntry(job6.m_jobID, 6, "SomeProduct6.dds", validAssetType6);
        EXPECT_TRUE(m_connection.SetProduct(product6));


        /////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        //productDependencies
        auto ProductDependenciesContainProductDependencyID = [](const ProductDependencyDatabaseEntryContainer& productDependencies, AZ::s64 productDepdendencyId) -> bool
        {
            for (const auto& productDependency : productDependencies)
            {
                if (productDependency.m_productDependencyID == productDepdendencyId)
                {
                    return true;
                }
            }
            return false;
        };

        auto ProductDependenciesContainProductID = [](const ProductDependencyDatabaseEntryContainer& productDependencies, AZ::s64 productId) -> bool
        {
            for (const auto& productDependency : productDependencies)
            {
                if (productDependency.m_productPK == productId)
                {
                    return true;
                }
            }
            return false;
        };

        auto ProductDependenciesContainDependencySoureGuid = [](const ProductDependencyDatabaseEntryContainer& productDependencies, AZ::Uuid dependencySourceGuid) -> bool
        {
            for (const auto& productDependency : productDependencies)
            {
                if (productDependency.m_dependencySourceGuid == dependencySourceGuid)
                {
                    return true;
                }
            }
            return false;
        };

        auto ProductDependenciesContainDependencySubID = [](const ProductDependencyDatabaseEntryContainer& productDependencies, AZ::u32 dependencySubID) -> bool
        {
            for (const auto& productDependency : productDependencies)
            {
                if (productDependency.m_dependencySubID == dependencySubID)
                {
                    return true;
                }
            }
            return false;
        };

        auto ProductDependenciesContainDependencyFlags = [](const ProductDependencyDatabaseEntryContainer& productDependencies, AZStd::bitset<64> dependencyFlags) -> bool
        {
            for (const auto& productDependency : productDependencies)
            {
                if (productDependency.m_dependencyFlags == dependencyFlags)
                {
                    return true;
                }
            }
            return false;
        };

        //there are no product dependencies yet so trying to find one should fail
        productDependencies.clear();
        products.clear();
        EXPECT_FALSE(m_connection.GetProductDependencies(productDependencies));
        EXPECT_FALSE(m_connection.GetProductDependencyByProductDependencyID(3443, productDependency));
        EXPECT_FALSE(m_connection.GetProductDependenciesByProductID(3443, productDependencies));
        EXPECT_FALSE(m_connection.GetDirectProductDependencies(3443, products));
        EXPECT_FALSE(m_connection.GetAllProductDependencies(3443, products));

        AZStd::string platform;
        //trying to add a product dependency without a valid product pk should fail
        productDependency = ProductDependencyDatabaseEntry(234234, validSourceGuid1, 1, 0, platform, true);
        {
            UnitTestUtils::AssertAbsorber absorber;
            EXPECT_FALSE(m_connection.SetProductDependency(productDependency));
            EXPECT_GE(absorber.m_numWarningsAbsorbed, 0);
        }

        //setting a valid product pk should allow it to be added
        //Product -> Product2
        productDependency = ProductDependencyDatabaseEntry(product.m_productID, validSourceGuid2, 2, 0, platform, true);
        EXPECT_TRUE(m_connection.SetProductDependency(productDependency));
        EXPECT_NE(productDependency.m_productDependencyID, AzToolsFramework::AssetDatabase::InvalidEntryId) << "AssetProcessingStateDataTest Failed - SetProductDependency failed to add";

        //get all product dependencies, there should only the one we added
        productDependencies.clear();
        EXPECT_TRUE(m_connection.GetProductDependencies(productDependencies));
        EXPECT_EQ(productDependencies.size(), 1);

        EXPECT_TRUE(ProductDependenciesContainProductDependencyID(productDependencies, productDependency.m_productDependencyID));
        EXPECT_TRUE(ProductDependenciesContainProductID(productDependencies, productDependency.m_productPK));
        EXPECT_TRUE(ProductDependenciesContainDependencySoureGuid(productDependencies, productDependency.m_dependencySourceGuid));
        EXPECT_TRUE(ProductDependenciesContainDependencySubID(productDependencies, productDependency.m_dependencySubID));
        EXPECT_TRUE(ProductDependenciesContainDependencyFlags(productDependencies, productDependency.m_dependencyFlags));

        //add the same product again, should not add another because it already exists, so we should get the same id
        ProductDependencyDatabaseEntry dupeProductDependency(productDependency);
        dupeProductDependency.m_productDependencyID = AzToolsFramework::AssetDatabase::InvalidEntryId;
        EXPECT_TRUE(m_connection.SetProductDependency(dupeProductDependency));
        EXPECT_EQ(dupeProductDependency, dupeProductDependency) << "AssetProcessingStateDataTest Failed - SetProductDependency failed to add";

        //get all product dependencies, there should still only the one we added
        productDependencies.clear();
        EXPECT_TRUE(m_connection.GetProductDependencies(productDependencies));
        EXPECT_EQ(productDependencies.size(), 1);
        EXPECT_TRUE(ProductDependenciesContainProductDependencyID(productDependencies, productDependency.m_productDependencyID));
        EXPECT_TRUE(ProductDependenciesContainProductID(productDependencies, productDependency.m_productPK));
        EXPECT_TRUE(ProductDependenciesContainDependencySoureGuid(productDependencies, productDependency.m_dependencySourceGuid));
        EXPECT_TRUE(ProductDependenciesContainDependencySubID(productDependencies, productDependency.m_dependencySubID));
        EXPECT_TRUE(ProductDependenciesContainDependencyFlags(productDependencies, productDependency.m_dependencyFlags));

        // Setup some more dependencies

        //Product2 -> Product3
        productDependency = ProductDependencyDatabaseEntry(product2.m_productID, validSourceGuid3, 3, 0, platform, true);
        EXPECT_TRUE(m_connection.SetProductDependency(productDependency));

        //Product2 -> Product4
        productDependency = ProductDependencyDatabaseEntry(product2.m_productID, validSourceGuid4, 4, 0, platform, true);
        EXPECT_TRUE(m_connection.SetProductDependency(productDependency));

        //Product3 -> Product5
        productDependency = ProductDependencyDatabaseEntry(product3.m_productID, validSourceGuid5, 5, 0, platform, true);
        EXPECT_TRUE(m_connection.SetProductDependency(productDependency));

        //Product5 -> Product6
        productDependency = ProductDependencyDatabaseEntry(product5.m_productID, validSourceGuid6, 6, 0, platform, true);
        EXPECT_TRUE(m_connection.SetProductDependency(productDependency));

        /* Dependency Tree
        *
        * Product -> Product2 -> Product3 -> Product5 -> Product 6->
        *                    \
        *                     -> Product4
        */

        // Direct Deps

        // Product -> Product2
        products.clear();
        EXPECT_TRUE(m_connection.GetDirectProductDependencies(product.m_productID, products));
        EXPECT_EQ(products.size(), 1);
        EXPECT_TRUE(ProductsContainProductID(products, product2.m_productID));

        // Product2 -> Product3, Product4
        products.clear();
        EXPECT_TRUE(m_connection.GetDirectProductDependencies(product2.m_productID, products));
        EXPECT_EQ(products.size(), 2);
        EXPECT_TRUE(ProductsContainProductID(products, product3.m_productID));
        EXPECT_TRUE(ProductsContainProductID(products, product4.m_productID));

        // Product3 -> Product5
        products.clear();
        EXPECT_TRUE(m_connection.GetDirectProductDependencies(product3.m_productID, products));
        EXPECT_EQ(products.size(), 1);
        EXPECT_TRUE(ProductsContainProductID(products, product5.m_productID));

        // Product4 ->
        products.clear();
        EXPECT_FALSE(m_connection.GetDirectProductDependencies(product4.m_productID, products));
        EXPECT_EQ(products.size(), 0);

        // Product5 -> Product6
        products.clear();
        EXPECT_TRUE(m_connection.GetDirectProductDependencies(product5.m_productID, products));
        EXPECT_EQ(products.size(), 1);
        EXPECT_TRUE(ProductsContainProductID(products, product6.m_productID));

        // Product6 ->
        products.clear();
        EXPECT_FALSE(m_connection.GetDirectProductDependencies(product6.m_productID, products));
        EXPECT_EQ(products.size(), 0);

        // All Deps

        // Product -> Product2, Product3, Product4, Product5, Product6
        products.clear();
        EXPECT_TRUE(m_connection.GetAllProductDependencies(product.m_productID, products));
        EXPECT_EQ(products.size(), 5);
        EXPECT_TRUE(ProductsContainProductID(products, product2.m_productID));
        EXPECT_TRUE(ProductsContainProductID(products, product3.m_productID));
        EXPECT_TRUE(ProductsContainProductID(products, product4.m_productID));
        EXPECT_TRUE(ProductsContainProductID(products, product5.m_productID));
        EXPECT_TRUE(ProductsContainProductID(products, product6.m_productID));

        // Product2 -> Product3, Product4, Product5, Product6
        products.clear();
        EXPECT_TRUE(m_connection.GetAllProductDependencies(product2.m_productID, products));
        EXPECT_EQ(products.size(), 4);
        EXPECT_TRUE(ProductsContainProductID(products, product3.m_productID));
        EXPECT_TRUE(ProductsContainProductID(products, product4.m_productID));
        EXPECT_TRUE(ProductsContainProductID(products, product5.m_productID));
        EXPECT_TRUE(ProductsContainProductID(products, product6.m_productID));

        // Product3 -> Product5, Product6
        products.clear();
        EXPECT_TRUE(m_connection.GetAllProductDependencies(product3.m_productID, products));
        EXPECT_EQ(products.size(), 2);
        EXPECT_TRUE(ProductsContainProductID(products, product5.m_productID));
        EXPECT_TRUE(ProductsContainProductID(products, product6.m_productID));

        // Product4 ->
        products.clear();
        EXPECT_FALSE(m_connection.GetAllProductDependencies(product4.m_productID, products));
        EXPECT_EQ(products.size(), 0);

        // Product5 -> Product6
        products.clear();
        EXPECT_TRUE(m_connection.GetAllProductDependencies(product5.m_productID, products));
        EXPECT_EQ(products.size(), 1);
        EXPECT_TRUE(ProductsContainProductID(products, product6.m_productID));

        // Product6 ->
        products.clear();
        EXPECT_FALSE(m_connection.GetAllProductDependencies(product6.m_productID, products));
        EXPECT_EQ(products.size(), 0);

        //Product6 -> Product (This creates a circular dependency)
        productDependency = ProductDependencyDatabaseEntry(product6.m_productID, validSourceGuid1, 1, 0, platform, true);
        EXPECT_TRUE(m_connection.SetProductDependency(productDependency));

        /* Circular Dependency Tree
        * v--------------------------------------------------------<
        * |                                                        |
        * Product -> Product2 -> Product3 -> Product5 -> Product 6-^
        *                    \
        *                     -> Product4
        */

        // Product6 -> Product
        products.clear();
        EXPECT_TRUE(m_connection.GetDirectProductDependencies(product6.m_productID, products));
        EXPECT_EQ(products.size(), 1);
        EXPECT_TRUE(ProductsContainProductID(products, product.m_productID));

        // Product3 -> Product5, Product6, Product, Product2, Product4
        products.clear();
        EXPECT_TRUE(m_connection.GetAllProductDependencies(product3.m_productID, products));
        EXPECT_EQ(products.size(), 5);
        EXPECT_TRUE(ProductsContainProductID(products, product5.m_productID));
        EXPECT_TRUE(ProductsContainProductID(products, product6.m_productID));
        EXPECT_TRUE(ProductsContainProductID(products, product.m_productID));
        EXPECT_TRUE(ProductsContainProductID(products, product2.m_productID));
        EXPECT_TRUE(ProductsContainProductID(products, product4.m_productID));

        m_connection.RemoveProductDependencyByProductId(product5.m_productID);
        products.clear();
        EXPECT_TRUE(m_connection.GetAllProductDependencies(product2.m_productID, products));
        EXPECT_EQ(products.size(), 3);
        EXPECT_TRUE(ProductsContainProductID(products, product3.m_productID));
        EXPECT_TRUE(ProductsContainProductID(products, product4.m_productID));
        EXPECT_TRUE(ProductsContainProductID(products, product5.m_productID));

        // Teardown
        //The product dependencies should cascade delete
        EXPECT_TRUE(m_connection.RemoveSource(source.m_sourceID));
        EXPECT_TRUE(m_connection.RemoveSource(source2.m_sourceID));
        EXPECT_TRUE(m_connection.RemoveSource(source3.m_sourceID));
        EXPECT_TRUE(m_connection.RemoveSource(source4.m_sourceID));
        EXPECT_TRUE(m_connection.RemoveSource(source5.m_sourceID));
        EXPECT_TRUE(m_connection.RemoveSource(source6.m_sourceID));

        productDependencies.clear();
        products.clear();
        EXPECT_FALSE(m_connection.GetProductDependencies(productDependencies));
        EXPECT_FALSE(m_connection.GetDirectProductDependencies(product.m_productID, products));
        EXPECT_FALSE(m_connection.GetAllProductDependencies(product.m_productID, products));
    }

    TEST_F(AssetProcessingStateDataUnitTest, BuilderInfoTest_ValidDatabaseConnectionProvided_OperationsSucceed)
    {
        using BuilderInfoEntry = AzToolsFramework::AssetDatabase::BuilderInfoEntry;
        using BuilderInfoEntryContainer = AzToolsFramework::AssetDatabase::BuilderInfoEntryContainer;

        // empty database should have no builder info:

        BuilderInfoEntryContainer results;
        auto resultGatherer = [&results](BuilderInfoEntry&& element)
        {
            results.push_back(AZStd::move(element));
            return true; // returning false would stop iterating.  We want all results, so we return true.
        };

        EXPECT_TRUE(m_connection.QueryBuilderInfoTable(resultGatherer));
        EXPECT_TRUE(results.empty());

        BuilderInfoEntryContainer newEntries;

        newEntries.emplace_back(BuilderInfoEntry(AzToolsFramework::AssetDatabase::InvalidEntryId, AZ::Uuid::CreateString("{648B7B06-27A3-42AC-897D-FA4557C28654}"), "Finger_Print"));
        newEntries.emplace_back(BuilderInfoEntry(AzToolsFramework::AssetDatabase::InvalidEntryId, AZ::Uuid::CreateString("{0B657D45-A5B0-485B-BF34-0E8779F9A482}"), "Finger_Print"));

        EXPECT_TRUE(m_connection.SetBuilderInfoTable(newEntries));
        // make sure each entry has a number assigned:
        EXPECT_NE(newEntries[0].m_builderInfoID, AzToolsFramework::AssetDatabase::InvalidEntryId);
        EXPECT_NE(newEntries[1].m_builderInfoID, AzToolsFramework::AssetDatabase::InvalidEntryId);

        EXPECT_TRUE(m_connection.QueryBuilderInfoTable(resultGatherer));
        EXPECT_EQ(results.size(), 2);
        EXPECT_NE(results[0].m_builderInfoID, AzToolsFramework::AssetDatabase::InvalidEntryId);
        EXPECT_NE(results[1].m_builderInfoID, AzToolsFramework::AssetDatabase::InvalidEntryId);

        // they could be in any order, so fix that first:
        bool isInCorrectOrder = (results[0].m_builderInfoID == newEntries[0].m_builderInfoID) && (results[1].m_builderInfoID == newEntries[1].m_builderInfoID);
        bool isInReverseOrder = (results[1].m_builderInfoID == newEntries[0].m_builderInfoID) && (results[0].m_builderInfoID == newEntries[1].m_builderInfoID);

        EXPECT_TRUE(isInCorrectOrder || isInReverseOrder);

        if (isInReverseOrder)
        {
            BuilderInfoEntry temp = results[0];
            results[0] = results[1];
            results[1] = temp;
        }

        for (size_t idx = 0; idx < 2; ++idx)
        {
            EXPECT_EQ(results[idx].m_builderUuid, newEntries[idx].m_builderUuid);
            EXPECT_EQ(results[idx].m_builderInfoID, newEntries[idx].m_builderInfoID);
            EXPECT_EQ(results[idx].m_analysisFingerprint, newEntries[idx].m_analysisFingerprint);
        }

        // now REPLACE the entries with fewer and make sure it actually chops it down and also replaces the fields.
        newEntries.clear();
        results.clear();
        newEntries.emplace_back(BuilderInfoEntry(AzToolsFramework::AssetDatabase::InvalidEntryId, AZ::Uuid::CreateString("{8863194A-BCB2-4A4C-A7D9-4E90D68814D4}"), "Finger_Print2"));
        EXPECT_TRUE(m_connection.SetBuilderInfoTable(newEntries));
        // make sure each entry has a number assigned:
        EXPECT_NE(newEntries[0].m_builderInfoID, AzToolsFramework::AssetDatabase::InvalidEntryId);
        EXPECT_TRUE(m_connection.QueryBuilderInfoTable(resultGatherer));
        EXPECT_EQ(results.size(), 1);
        EXPECT_NE(results[0].m_builderInfoID, AzToolsFramework::AssetDatabase::InvalidEntryId);
        EXPECT_EQ(results[0].m_builderUuid, newEntries[0].m_builderUuid);
        EXPECT_EQ(results[0].m_builderInfoID, newEntries[0].m_builderInfoID);
        EXPECT_EQ(results[0].m_analysisFingerprint, newEntries[0].m_analysisFingerprint);
    }

    TEST_F(AssetProcessingStateDataUnitTest, SourceDependencyTest_ValidDatabaseConnectionProvided_OperationsSucceed)
    {
        using namespace AzToolsFramework::AssetDatabase;

        //  A depends on B, which depends on both C and D
        AZ::Uuid aUuid{ "{B3FCF51E-BDB3-430D-B360-E57913725250}" };
        AZ::Uuid bUuid{ "{E040466C-8B26-4ABB-9E7A-2FF9D1660DB6}" };

        SourceFileDependencyEntry newEntry1;  // a depends on B
        newEntry1.m_sourceDependencyID = AzToolsFramework::AssetDatabase::InvalidEntryId;
        newEntry1.m_builderGuid = AZ::Uuid::CreateRandom();
        newEntry1.m_sourceGuid = aUuid;
        newEntry1.m_dependsOnSource = PathOrUuid(bUuid);

        SourceFileDependencyEntry newEntry2; // b depends on C
        newEntry2.m_sourceDependencyID = AzToolsFramework::AssetDatabase::InvalidEntryId;
        newEntry2.m_builderGuid = AZ::Uuid::CreateRandom();
        newEntry2.m_sourceGuid = bUuid;
        newEntry2.m_dependsOnSource = PathOrUuid("c.txt");

        SourceFileDependencyEntry newEntry3;  // b also depends on D
        newEntry3.m_sourceDependencyID = AzToolsFramework::AssetDatabase::InvalidEntryId;
        newEntry3.m_builderGuid = AZ::Uuid::CreateRandom();
        newEntry3.m_sourceGuid = bUuid;
        newEntry3.m_dependsOnSource = PathOrUuid("d.txt");

        EXPECT_TRUE(m_connection.SetSourceFileDependency(newEntry1));
        EXPECT_TRUE(m_connection.SetSourceFileDependency(newEntry2));
        EXPECT_TRUE(m_connection.SetSourceFileDependency(newEntry3));

        SourceFileDependencyEntryContainer results;

        // what depends on b?  a does.
        EXPECT_TRUE(m_connection.GetSourceFileDependenciesByDependsOnSource(bUuid, "b.txt", "unused", SourceFileDependencyEntry::DEP_Any, results));
        EXPECT_EQ(results.size(), 1);
        EXPECT_EQ(results[0].m_sourceGuid, aUuid);
        EXPECT_EQ(results[0].m_builderGuid, newEntry1.m_builderGuid);
        EXPECT_EQ(results[0].m_sourceDependencyID, newEntry1.m_sourceDependencyID);

        // what does B depend on?
        results.clear();
        EXPECT_TRUE(m_connection.GetDependsOnSourceBySource(bUuid, SourceFileDependencyEntry::DEP_Any, results));
        // b depends on 2 things: c and d
        EXPECT_EQ(results.size(), 2);
        EXPECT_EQ(results[0].m_sourceGuid, bUuid);  // note that both of these are B, since its B that has the dependency on the others.
        EXPECT_EQ(results[1].m_sourceGuid, bUuid);
        EXPECT_EQ(results[0].m_dependsOnSource.GetPath(), "c.txt");
        EXPECT_EQ(results[1].m_dependsOnSource.GetPath(), "d.txt");

        // what does b depend on, but filtered to only one builder?
        results.clear();
        EXPECT_TRUE(m_connection.GetSourceFileDependenciesByBuilderGUIDAndSource(newEntry2.m_builderGuid, bUuid, SourceFileDependencyEntry::DEP_SourceToSource, results));
        // b depends on 1 thing from that builder: c
        EXPECT_EQ(results.size(), 1);
        EXPECT_EQ(results[0].m_sourceGuid, bUuid);
        EXPECT_EQ(results[0].m_dependsOnSource.GetPath(), "c.txt");

        // make sure that we can look these up by ID (a)
        EXPECT_TRUE(m_connection.GetSourceFileDependencyBySourceDependencyId(newEntry1.m_sourceDependencyID, results[0]));
        EXPECT_EQ(results[0].m_sourceGuid, aUuid);
        EXPECT_EQ(results[0].m_builderGuid, newEntry1.m_builderGuid);
        EXPECT_EQ(results[0].m_sourceDependencyID, newEntry1.m_sourceDependencyID);

        // remove D, b now should only depend on C
        results.clear();
        EXPECT_TRUE(m_connection.RemoveSourceFileDependency(newEntry3.m_sourceDependencyID));
        EXPECT_TRUE(m_connection.GetDependsOnSourceBySource(bUuid, SourceFileDependencyEntry::DEP_Any, results));
        EXPECT_EQ(results.size(), 1);
        EXPECT_EQ(results[0].m_dependsOnSource.GetPath(), "c.txt");


        // clean up
        EXPECT_TRUE(m_connection.RemoveSourceFileDependency(newEntry1.m_sourceDependencyID));
        EXPECT_TRUE(m_connection.RemoveSourceFileDependency(newEntry2.m_sourceDependencyID));
    }

    TEST_F(AssetProcessingStateDataUnitTest, SourceFingerprintTest_ValidDatabaseConnectionProvided_OperationsSucceed)
    {
        using SourceDatabaseEntry = AzToolsFramework::AssetDatabase::SourceDatabaseEntry;
        using ScanFolderDatabaseEntry = AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry;

        // to add a source file you have to add a scan folder first
        ScanFolderDatabaseEntry scanFolder;
        scanFolder.m_displayName = "test scan folder";
        scanFolder.m_isRoot = false;
        scanFolder.m_portableKey = "1234";
        scanFolder.m_scanFolder = "//test//test";
        scanFolder.m_scanFolderID = AzToolsFramework::AssetDatabase::InvalidEntryId;

        EXPECT_TRUE(m_connection.SetScanFolder(scanFolder));

        SourceDatabaseEntry sourceFile1;
        sourceFile1.m_analysisFingerprint = "12345";
        sourceFile1.m_scanFolderPK = scanFolder.m_scanFolderID;
        sourceFile1.m_sourceGuid = AZ::Uuid::CreateRandom();
        sourceFile1.m_sourceName = "a.txt";
        EXPECT_TRUE(m_connection.SetSource(sourceFile1));

        SourceDatabaseEntry sourceFile2;
        sourceFile2.m_analysisFingerprint = "54321";
        sourceFile2.m_scanFolderPK = scanFolder.m_scanFolderID;
        sourceFile2.m_sourceGuid = AZ::Uuid::CreateRandom();
        sourceFile2.m_sourceName = "b.txt";

        EXPECT_TRUE(m_connection.SetSource(sourceFile2));

        AZStd::string resultString("garbage");
        // its not a database error to ask for a file that does not exist:
        EXPECT_TRUE(m_connection.QuerySourceAnalysisFingerprint("does not exist", scanFolder.m_scanFolderID, resultString));
        // but we do expect it to empty the result:
        EXPECT_TRUE(resultString.empty());
        EXPECT_TRUE(m_connection.QuerySourceAnalysisFingerprint("a.txt", scanFolder.m_scanFolderID, resultString));
        EXPECT_EQ(resultString, "12345");
        EXPECT_TRUE(m_connection.QuerySourceAnalysisFingerprint("b.txt", scanFolder.m_scanFolderID, resultString));
        EXPECT_EQ(resultString, "54321");
    }
}
