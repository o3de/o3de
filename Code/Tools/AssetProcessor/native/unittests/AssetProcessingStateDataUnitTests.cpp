/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "AssetProcessingStateDataUnitTests.h"
#include <AzToolsFramework/API/AssetDatabaseBus.h>
#include "native/AssetDatabase/AssetDatabase.h"

namespace AssetProcessingStateDataUnitTestInternal
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
}

// perform some operations on the state data given.  (Does not perform save and load tests)
void AssetProcessingStateDataUnitTest::DataTest(AssetProcessor::AssetDatabaseConnection* stateData)
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
    UNIT_TEST_EXPECT_FALSE(stateData->GetScanFolders(scanFolders));
    UNIT_TEST_EXPECT_FALSE(stateData->GetScanFolderByScanFolderID(0, scanFolder));
    UNIT_TEST_EXPECT_FALSE(stateData->GetScanFolderBySourceID(0, scanFolder));
    UNIT_TEST_EXPECT_FALSE(stateData->GetScanFolderByProductID(0, scanFolder));
    UNIT_TEST_EXPECT_FALSE(stateData->GetScanFolderByPortableKey("sadfsadfsadfsadfs", scanFolder));
    scanFolders.clear();

    //add a scanfolder
    scanFolder = ScanFolderDatabaseEntry("c:/O3DE/dev", "dev", "rootportkey");
    UNIT_TEST_EXPECT_TRUE(stateData->SetScanFolder(scanFolder));
    if (scanFolder.m_scanFolderID == AzToolsFramework::AssetDatabase::InvalidEntryId)
    {
        Q_EMIT UnitTestFailed("AssetProcessingStateDataTest Failed - scan folder failed to add");
        return;
    }

    //add the same folder again, should not add another because it already exists, so we should get the same id
    // not only that, but the path should update.
    ScanFolderDatabaseEntry dupeScanFolder("c:/O3DE/dev2", "dev", "rootportkey");
    dupeScanFolder.m_scanFolderID = AzToolsFramework::AssetDatabase::InvalidEntryId;
    UNIT_TEST_EXPECT_TRUE(stateData->SetScanFolder(dupeScanFolder));
    if (!(dupeScanFolder == scanFolder))
    {
        Q_EMIT UnitTestFailed("AssetProcessingStateDataTest Failed - scan folder failed to add");
        return;
    }

    UNIT_TEST_EXPECT_TRUE(dupeScanFolder.m_portableKey == scanFolder.m_portableKey);
    UNIT_TEST_EXPECT_TRUE(dupeScanFolder.m_scanFolderID == scanFolder.m_scanFolderID);

    //get all scan folders, there should only the one we added
    scanFolders.clear();
    UNIT_TEST_EXPECT_TRUE(stateData->GetScanFolders(scanFolders));
    UNIT_TEST_EXPECT_TRUE(scanFolders.size() == 1);
    UNIT_TEST_EXPECT_TRUE(ScanFoldersContainScanPath(scanFolders, "c:/O3DE/dev2"));
    UNIT_TEST_EXPECT_TRUE(ScanFoldersContainScanFolderID(scanFolders, scanFolder.m_scanFolderID));
    UNIT_TEST_EXPECT_TRUE(ScanFoldersContainPortableKey(scanFolders, scanFolder.m_portableKey.c_str()));
    UNIT_TEST_EXPECT_TRUE(ScanFoldersContainPortableKey(scanFolders, "rootportkey"));

    //retrieve the one we just made by id
    ScanFolderDatabaseEntry retrieveScanfolderById;
    UNIT_TEST_EXPECT_TRUE(stateData->GetScanFolderByScanFolderID(scanFolder.m_scanFolderID, retrieveScanfolderById));
    if (retrieveScanfolderById.m_scanFolderID == AzToolsFramework::AssetDatabase::InvalidEntryId ||
        retrieveScanfolderById.m_scanFolderID != scanFolder.m_scanFolderID)
    {
        Q_EMIT UnitTestFailed("AssetProcessingStateDataTest Failed - scan folder failed to add");
        return;
    }

    //retrieve the one we just made by portable key
    ScanFolderDatabaseEntry retrieveScanfolderByScanPath;
    UNIT_TEST_EXPECT_TRUE(stateData->GetScanFolderByPortableKey("rootportkey", retrieveScanfolderByScanPath));
    if (retrieveScanfolderByScanPath.m_scanFolderID == AzToolsFramework::AssetDatabase::InvalidEntryId ||
        retrieveScanfolderByScanPath.m_scanFolderID != scanFolder.m_scanFolderID)
    {
        Q_EMIT UnitTestFailed("AssetProcessingStateDataTest Failed - scan folder failed to add");
        return;
    }

    //add another folder
    ScanFolderDatabaseEntry gameScanFolderEntry("c:/O3DE/game", "game", "gameportkey");
    UNIT_TEST_EXPECT_TRUE(stateData->SetScanFolder(gameScanFolderEntry));
    if (gameScanFolderEntry.m_scanFolderID == AzToolsFramework::AssetDatabase::InvalidEntryId ||
        gameScanFolderEntry.m_scanFolderID == scanFolder.m_scanFolderID)
    {
        Q_EMIT UnitTestFailed("AssetProcessingStateDataTest Failed - scan folder failed to add");
        return;
    }

    //get all scan folders, there should only the two we added
    scanFolders.clear();
    UNIT_TEST_EXPECT_TRUE(stateData->GetScanFolders(scanFolders));
    UNIT_TEST_EXPECT_TRUE(scanFolders.size() == 2);
    UNIT_TEST_EXPECT_TRUE(ScanFoldersContainScanPath(scanFolders, "c:/O3DE/dev2"));
    UNIT_TEST_EXPECT_TRUE(ScanFoldersContainScanPath(scanFolders, "c:/O3DE/game"));
    UNIT_TEST_EXPECT_TRUE(ScanFoldersContainScanFolderID(scanFolders, scanFolder.m_scanFolderID));
    UNIT_TEST_EXPECT_TRUE(ScanFoldersContainScanFolderID(scanFolders, gameScanFolderEntry.m_scanFolderID));

    //remove the game scan folder
    UNIT_TEST_EXPECT_TRUE(stateData->RemoveScanFolder(848475));//should return true even if it doesn't exist, false only means SQL failed
    UNIT_TEST_EXPECT_TRUE(stateData->RemoveScanFolder(gameScanFolderEntry.m_scanFolderID));

    //get all scan folders again, there should now only the first we added
    scanFolders.clear();
    UNIT_TEST_EXPECT_TRUE(stateData->GetScanFolders(scanFolders));
    UNIT_TEST_EXPECT_TRUE(scanFolders.size() == 1);
    UNIT_TEST_EXPECT_TRUE(ScanFoldersContainScanPath(scanFolders, "c:/O3DE/dev2"));
    UNIT_TEST_EXPECT_TRUE(ScanFoldersContainScanFolderID(scanFolders, scanFolder.m_scanFolderID));

    //add another folder again
    gameScanFolderEntry = ScanFolderDatabaseEntry("c:/O3DE/game", "game", "gameportkey2");
    UNIT_TEST_EXPECT_TRUE(stateData->SetScanFolder(gameScanFolderEntry));
    if (gameScanFolderEntry.m_scanFolderID == AzToolsFramework::AssetDatabase::InvalidEntryId ||
        gameScanFolderEntry.m_scanFolderID == scanFolder.m_scanFolderID)
    {
        Q_EMIT UnitTestFailed("AssetProcessingStateDataTest Failed - scan folder failed to add");
        return;
    }

    //get all scan folders, there should only the two we added
    scanFolders.clear();
    UNIT_TEST_EXPECT_TRUE(stateData->GetScanFolders(scanFolders));
    UNIT_TEST_EXPECT_TRUE(scanFolders.size() == 2);
    UNIT_TEST_EXPECT_TRUE(ScanFoldersContainScanPath(scanFolders, "c:/O3DE/dev2"));
    UNIT_TEST_EXPECT_TRUE(ScanFoldersContainScanPath(scanFolders, "c:/O3DE/game"));
    UNIT_TEST_EXPECT_TRUE(ScanFoldersContainScanFolderID(scanFolders, scanFolder.m_scanFolderID));
    UNIT_TEST_EXPECT_TRUE(ScanFoldersContainScanFolderID(scanFolders, gameScanFolderEntry.m_scanFolderID));

    //remove scan folder by using a container
    ScanFolderDatabaseEntryContainer tempScanFolderDatabaseEntryContainer; // note that on clang, its illegal to call a non-const function with a temp variable container as the param
    UNIT_TEST_EXPECT_TRUE(stateData->RemoveScanFolders(tempScanFolderDatabaseEntryContainer)); // call with empty
    UNIT_TEST_EXPECT_TRUE(stateData->RemoveScanFolders(scanFolders));
    scanFolders.clear();
    UNIT_TEST_EXPECT_FALSE(stateData->GetScanFolders(scanFolders));

    ///////////////////////////////////////////////////////////
    //setup for sources tests
    //for the rest of the test lets add the original scan folder
    scanFolder = ScanFolderDatabaseEntry("c:/O3DE/dev", "dev", "devkey2");
    UNIT_TEST_EXPECT_TRUE(stateData->SetScanFolder(scanFolder));
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
    UNIT_TEST_EXPECT_FALSE(stateData->GetSources(sources));
    UNIT_TEST_EXPECT_FALSE(stateData->GetSourceBySourceID(3443, source));
    UNIT_TEST_EXPECT_FALSE(stateData->GetSourceBySourceGuid(AZ::Uuid::Create(), source));
    UNIT_TEST_EXPECT_FALSE(stateData->GetSourcesLikeSourceName("source", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::Raw, sources));
    UNIT_TEST_EXPECT_FALSE(stateData->GetSourcesLikeSourceName("source", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::StartsWith, sources));
    UNIT_TEST_EXPECT_FALSE(stateData->GetSourcesLikeSourceName("source", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::EndsWith, sources));
    UNIT_TEST_EXPECT_FALSE(stateData->GetSourcesLikeSourceName("source", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::Matches, sources));

    //trying to add a source without a valid scan folder pk should fail
    source = SourceDatabaseEntry(234234, "SomeSource1.tif", validSourceGuid1, "");
    {
        UnitTestUtils::AssertAbsorber absorb;
        UNIT_TEST_EXPECT_FALSE(stateData->SetSource(source));
        UNIT_TEST_EXPECT_TRUE(absorb.m_numWarningsAbsorbed > 0);
    }

    //setting a valid scan folder pk should allow it to be added
    source = SourceDatabaseEntry(scanFolder.m_scanFolderID, "SomeSource1.tif", validSourceGuid1, "tEsTFingerPrint_TEST");
    UNIT_TEST_EXPECT_TRUE(stateData->SetSource(source));
    if (source.m_sourceID == AzToolsFramework::AssetDatabase::InvalidEntryId)
    {
        Q_EMIT UnitTestFailed("AssetProcessingStateDataTest Failed - source failed to add");
        return;
    }

    //get all sources, there should only the one we added
    sources.clear();
    UNIT_TEST_EXPECT_TRUE(stateData->GetSources(sources));
    UNIT_TEST_EXPECT_TRUE(sources.size() == 1);
    UNIT_TEST_EXPECT_TRUE(sources[0].m_analysisFingerprint == "tEsTFingerPrint_TEST");
    UNIT_TEST_EXPECT_TRUE(SourcesContainSourceName(sources, "SomeSource1.tif"));
    UNIT_TEST_EXPECT_TRUE(SourcesContainSourceID(sources, source.m_sourceID));
    UNIT_TEST_EXPECT_TRUE(SourcesContainSourceGuid(sources, source.m_sourceGuid));

    //add the same source again, should not add another because it already exists, so we should get the same id
    SourceDatabaseEntry dupeSource(source);
    dupeSource.m_sourceID = AzToolsFramework::AssetDatabase::InvalidEntryId;
    UNIT_TEST_EXPECT_TRUE(stateData->SetSource(dupeSource));
    if (dupeSource.m_sourceID != source.m_sourceID)
    {
        Q_EMIT UnitTestFailed("AssetProcessingStateDataTest Failed - scan folder failed to add");
        return;
    }

    //get all sources, there should still only the one we added
    sources.clear();
    UNIT_TEST_EXPECT_TRUE(stateData->GetSources(sources));
    UNIT_TEST_EXPECT_TRUE(sources.size() == 1);
    UNIT_TEST_EXPECT_TRUE(sources[0].m_analysisFingerprint == "tEsTFingerPrint_TEST");
    UNIT_TEST_EXPECT_TRUE(SourcesContainSourceName(sources, "SomeSource1.tif"));
    UNIT_TEST_EXPECT_TRUE(SourcesContainSourceID(sources, source.m_sourceID));
    UNIT_TEST_EXPECT_TRUE(SourcesContainSourceGuid(sources, source.m_sourceGuid));

    // make sure that changing a field like fingerprint writes the new field to database but does not
    // add a new entry (ie, its just modifying existing data)
    SourceDatabaseEntry sourceWithDifferentFingerprint(source);
    sourceWithDifferentFingerprint.m_analysisFingerprint = "otherFingerprint";
    UNIT_TEST_EXPECT_TRUE(stateData->SetSource(sourceWithDifferentFingerprint));
    UNIT_TEST_EXPECT_TRUE(sourceWithDifferentFingerprint.m_sourceID == source.m_sourceID);
    sources.clear();
    UNIT_TEST_EXPECT_TRUE(stateData->GetSources(sources));
    UNIT_TEST_EXPECT_TRUE(sources.size() == 1);
    UNIT_TEST_EXPECT_TRUE(sources[0].m_analysisFingerprint == "otherFingerprint");
    UNIT_TEST_EXPECT_TRUE(SourcesContainSourceName(sources, "SomeSource1.tif"));
    UNIT_TEST_EXPECT_TRUE(SourcesContainSourceID(sources, source.m_sourceID));
    UNIT_TEST_EXPECT_TRUE(SourcesContainSourceGuid(sources, source.m_sourceGuid));

    //add the same source again, but change the scan folder.  This should NOT add a new source - even if we don't know what the sourceID is:
    ScanFolderDatabaseEntry scanfolder2 = ScanFolderDatabaseEntry("c:/O3DE/dev2", "dev2", "devkey3");
    UNIT_TEST_EXPECT_TRUE(stateData->SetScanFolder(scanfolder2));

    SourceDatabaseEntry dupeSource2(source);
    dupeSource2.m_scanFolderPK = scanfolder2.m_scanFolderID;
    dupeSource2.m_analysisFingerprint = "new different fingerprint";
    dupeSource2.m_sourceID = AzToolsFramework::AssetDatabase::InvalidEntryId;
    UNIT_TEST_EXPECT_TRUE(stateData->SetSource(dupeSource2));
    if (dupeSource2.m_sourceID != source.m_sourceID)
    {
        Q_EMIT UnitTestFailed("AssetProcessingStateDataTest Failed - scan folder failed to add");
        return;
    }

    //get all sources, there should still only the one we added
    sources.clear();
    UNIT_TEST_EXPECT_TRUE(stateData->GetSources(sources));
    UNIT_TEST_EXPECT_TRUE(sources.size() == 1);
    UNIT_TEST_EXPECT_TRUE(sources[0].m_analysisFingerprint == "new different fingerprint"); // verify that this column IS updated.
    UNIT_TEST_EXPECT_TRUE(SourcesContainSourceName(sources, "SomeSource1.tif"));
    UNIT_TEST_EXPECT_TRUE(SourcesContainSourceID(sources, source.m_sourceID));
    UNIT_TEST_EXPECT_TRUE(SourcesContainSourceGuid(sources, source.m_sourceGuid));

    //add the same source again, but change the scan folder back  This should NOT add a new source - this time we do know what the source ID is!
    SourceDatabaseEntry dupeSource3(source);
    dupeSource3.m_scanFolderPK = scanFolder.m_scanFolderID; // changing it back here.
    UNIT_TEST_EXPECT_TRUE(stateData->SetSource(dupeSource3));
    if (dupeSource3.m_sourceID != source.m_sourceID)
    {
        Q_EMIT UnitTestFailed("AssetProcessingStateDataTest Failed - scan folder failed to add");
        return;
    }

    //get all sources, there should still only the one we added
    sources.clear();
    UNIT_TEST_EXPECT_TRUE(stateData->GetSources(sources));
    UNIT_TEST_EXPECT_TRUE(sources.size() == 1);
    UNIT_TEST_EXPECT_TRUE(SourcesContainSourceName(sources, "SomeSource1.tif"));
    UNIT_TEST_EXPECT_TRUE(SourcesContainSourceID(sources, source.m_sourceID));
    UNIT_TEST_EXPECT_TRUE(SourcesContainSourceGuid(sources, source.m_sourceGuid));

    // remove the extra scan folder, make sure it doesn't drop the source since it should now be bound to the original scan folder agian
    UNIT_TEST_EXPECT_TRUE(stateData->RemoveScanFolder(scanfolder2.m_scanFolderID));
    sources.clear();
    UNIT_TEST_EXPECT_TRUE(stateData->GetSources(sources));
    UNIT_TEST_EXPECT_TRUE(sources.size() == 1);
    UNIT_TEST_EXPECT_TRUE(SourcesContainSourceName(sources, "SomeSource1.tif"));
    UNIT_TEST_EXPECT_TRUE(SourcesContainSourceID(sources, source.m_sourceID));
    UNIT_TEST_EXPECT_TRUE(SourcesContainSourceGuid(sources, source.m_sourceGuid));

    //try retrieving this source by id
    SourceDatabaseEntry retrieveSourceBySourceID;
    UNIT_TEST_EXPECT_TRUE(stateData->GetSourceBySourceID(source.m_sourceID, retrieveSourceBySourceID));
    if (retrieveSourceBySourceID.m_sourceID == AzToolsFramework::AssetDatabase::InvalidEntryId ||
        retrieveSourceBySourceID.m_sourceID != source.m_sourceID ||
        retrieveSourceBySourceID.m_scanFolderPK != source.m_scanFolderPK ||
        retrieveSourceBySourceID.m_sourceGuid != source.m_sourceGuid ||
        retrieveSourceBySourceID.m_sourceName != source.m_sourceName)
    {
        Q_EMIT UnitTestFailed("AssetProcessingStateDataTest Failed - GetSourceBySourceID failed");
        return;
    }

    //try retrieving this source by guid
    SourceDatabaseEntry retrieveSourceBySourceGuid;
    UNIT_TEST_EXPECT_TRUE(stateData->GetSourceBySourceGuid(source.m_sourceGuid, retrieveSourceBySourceGuid));
    if (retrieveSourceBySourceGuid.m_sourceID == AzToolsFramework::AssetDatabase::InvalidEntryId ||
        retrieveSourceBySourceGuid.m_sourceID != source.m_sourceID ||
        retrieveSourceBySourceGuid.m_scanFolderPK != source.m_scanFolderPK ||
        retrieveSourceBySourceGuid.m_sourceGuid != source.m_sourceGuid ||
        retrieveSourceBySourceGuid.m_sourceName != source.m_sourceName)
    {
        Q_EMIT UnitTestFailed("AssetProcessingStateDataTest Failed - GetSourceBySourceID failed");
        return;
    }

    //try retrieving this source by source name
    sources.clear();
    UNIT_TEST_EXPECT_FALSE(stateData->GetSourcesLikeSourceName("Source1.tif", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::Raw, sources));
    sources.clear();
    UNIT_TEST_EXPECT_FALSE(stateData->GetSourcesLikeSourceName("_SomeSource1_", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::Raw, sources));
    sources.clear();
    UNIT_TEST_EXPECT_TRUE(stateData->GetSourcesLikeSourceName("SomeSource1%", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::Raw, sources));
    UNIT_TEST_EXPECT_TRUE(sources.size() == 1);
    UNIT_TEST_EXPECT_TRUE(SourcesContainSourceName(sources, "SomeSource1.tif"));
    UNIT_TEST_EXPECT_TRUE(SourcesContainSourceID(sources, source.m_sourceID));
    UNIT_TEST_EXPECT_TRUE(SourcesContainSourceGuid(sources, source.m_sourceGuid));
    sources.clear();
    UNIT_TEST_EXPECT_TRUE(stateData->GetSourcesLikeSourceName("%SomeSource1%", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::Raw, sources));
    UNIT_TEST_EXPECT_TRUE(sources.size() == 1);
    UNIT_TEST_EXPECT_TRUE(SourcesContainSourceName(sources, "SomeSource1.tif"));
    UNIT_TEST_EXPECT_TRUE(SourcesContainSourceID(sources, source.m_sourceID));
    UNIT_TEST_EXPECT_TRUE(SourcesContainSourceGuid(sources, source.m_sourceGuid));
    sources.clear();
    UNIT_TEST_EXPECT_FALSE(stateData->GetSourcesLikeSourceName("Source1", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::StartsWith, sources));
    sources.clear();
    UNIT_TEST_EXPECT_TRUE(stateData->GetSourcesLikeSourceName("Some", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::StartsWith, sources));
    UNIT_TEST_EXPECT_TRUE(sources.size() == 1);
    UNIT_TEST_EXPECT_TRUE(SourcesContainSourceName(sources, "SomeSource1.tif"));
    UNIT_TEST_EXPECT_TRUE(SourcesContainSourceID(sources, source.m_sourceID));
    UNIT_TEST_EXPECT_TRUE(SourcesContainSourceGuid(sources, source.m_sourceGuid));
    sources.clear();
    UNIT_TEST_EXPECT_FALSE(stateData->GetSourcesLikeSourceName("SomeSource", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::EndsWith, sources));
    sources.clear();
    UNIT_TEST_EXPECT_TRUE(stateData->GetSourcesLikeSourceName(".tif", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::EndsWith, sources));
    UNIT_TEST_EXPECT_TRUE(sources.size() == 1);
    UNIT_TEST_EXPECT_TRUE(SourcesContainSourceName(sources, "SomeSource1.tif"));
    UNIT_TEST_EXPECT_TRUE(SourcesContainSourceID(sources, source.m_sourceID));
    UNIT_TEST_EXPECT_TRUE(SourcesContainSourceGuid(sources, source.m_sourceGuid));
    sources.clear();
    UNIT_TEST_EXPECT_FALSE(stateData->GetSourcesLikeSourceName("blah", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::Matches, sources));
    sources.clear();
    UNIT_TEST_EXPECT_TRUE(stateData->GetSourcesLikeSourceName("meSour", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::Matches, sources));
    UNIT_TEST_EXPECT_TRUE(sources.size() == 1);
    UNIT_TEST_EXPECT_TRUE(SourcesContainSourceName(sources, "SomeSource1.tif"));
    UNIT_TEST_EXPECT_TRUE(SourcesContainSourceID(sources, source.m_sourceID));
    UNIT_TEST_EXPECT_TRUE(SourcesContainSourceGuid(sources, source.m_sourceGuid));


    //remove a source
    UNIT_TEST_EXPECT_TRUE(stateData->RemoveSource(432234));//should return true even if it doesn't exist, false only if SQL failed
    UNIT_TEST_EXPECT_TRUE(stateData->RemoveSource(source.m_sourceID));

    //get all sources, there shouldn't be any
    sources.clear();
    UNIT_TEST_EXPECT_FALSE(stateData->GetSources(sources));

    //Add two sources then delete the via container
    SourceDatabaseEntry source2(scanFolder.m_scanFolderID, "SomeSource2.tif", validSourceGuid2, "");
    UNIT_TEST_EXPECT_TRUE(stateData->SetSource(source2));
    SourceDatabaseEntry source3(scanFolder.m_scanFolderID, "SomeSource3.tif", validSourceGuid3, "");
    UNIT_TEST_EXPECT_TRUE(stateData->SetSource(source3));

    //get all sources, there should only the two we added
    sources.clear();
    UNIT_TEST_EXPECT_TRUE(stateData->GetSources(sources));
    UNIT_TEST_EXPECT_TRUE(sources.size() == 2);
    UNIT_TEST_EXPECT_TRUE(SourcesContainSourceName(sources, "SomeSource2.tif"));
    UNIT_TEST_EXPECT_TRUE(SourcesContainSourceName(sources, "SomeSource3.tif"));
    UNIT_TEST_EXPECT_TRUE(SourcesContainSourceID(sources, source2.m_sourceID));
    UNIT_TEST_EXPECT_TRUE(SourcesContainSourceGuid(sources, source2.m_sourceGuid));
    UNIT_TEST_EXPECT_TRUE(SourcesContainSourceID(sources, source3.m_sourceID));
    UNIT_TEST_EXPECT_TRUE(SourcesContainSourceGuid(sources, source3.m_sourceGuid));

    //Remove source via container
    SourceDatabaseEntryContainer tempSourceDatabaseEntryContainer;
    UNIT_TEST_EXPECT_TRUE(stateData->RemoveSources(tempSourceDatabaseEntryContainer)); // try it with an empty one.
    UNIT_TEST_EXPECT_TRUE(stateData->RemoveSources(sources));

    //get all sources, there should none
    sources.clear();
    UNIT_TEST_EXPECT_FALSE(stateData->GetSources(sources));

    //Add two sources then delete the via removing by scan folder id
    source2 = SourceDatabaseEntry(scanFolder.m_scanFolderID, "SomeSource2.tif", validSourceGuid2, "fingerprint");
    UNIT_TEST_EXPECT_TRUE(stateData->SetSource(source2));
    source3 = SourceDatabaseEntry(scanFolder.m_scanFolderID, "SomeSource3.tif", validSourceGuid3, "fingerprint");
    UNIT_TEST_EXPECT_TRUE(stateData->SetSource(source3));

    //remove all sources for a scan folder
    sources.clear();
    UNIT_TEST_EXPECT_FALSE(stateData->RemoveSourcesByScanFolderID(3245532));
    UNIT_TEST_EXPECT_TRUE(stateData->RemoveSourcesByScanFolderID(scanFolder.m_scanFolderID));

    //get all sources, there should none
    sources.clear();
    UNIT_TEST_EXPECT_FALSE(stateData->GetSources(sources));

    //Add two sources then delete the via removing the scan folder
    source2 = SourceDatabaseEntry(scanFolder.m_scanFolderID, "SomeSource2.tif", validSourceGuid2, "");
    UNIT_TEST_EXPECT_TRUE(stateData->SetSource(source2));
    source3 = SourceDatabaseEntry(scanFolder.m_scanFolderID, "SomeSource3.tif", validSourceGuid3, "");
    UNIT_TEST_EXPECT_TRUE(stateData->SetSource(source3));

    //remove the scan folder for these sources, the sources should cascade delete
    UNIT_TEST_EXPECT_TRUE(stateData->RemoveScanFolder(scanFolder.m_scanFolderID));

    //get all sources, there should none
    sources.clear();
    UNIT_TEST_EXPECT_FALSE(stateData->GetSources(sources));

    ////////////////////////////////////////////////////////////////
    //Setup for jobs tests by having a scan folder and some sources
    //Add a scan folder
    scanFolder = ScanFolderDatabaseEntry("c:/O3DE/dev", "dev", "devkey3");
    UNIT_TEST_EXPECT_TRUE(stateData->SetScanFolder(scanFolder));

    //Add some sources
    source = SourceDatabaseEntry(scanFolder.m_scanFolderID, "SomeSource1.tif", validSourceGuid1, "");
    UNIT_TEST_EXPECT_TRUE(stateData->SetSource(source));
    source2 = SourceDatabaseEntry(scanFolder.m_scanFolderID, "SomeSource2.tif", validSourceGuid2, "");
    UNIT_TEST_EXPECT_TRUE(stateData->SetSource(source2));
    source3 = SourceDatabaseEntry(scanFolder.m_scanFolderID, "SomeSource3.tif", validSourceGuid3, "");
    UNIT_TEST_EXPECT_TRUE(stateData->SetSource(source3));
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
    UNIT_TEST_EXPECT_FALSE(stateData->GetJobs(jobs));
    UNIT_TEST_EXPECT_FALSE(stateData->GetJobByJobID(3443, job));
    UNIT_TEST_EXPECT_FALSE(stateData->GetJobsBySourceID(3234, jobs));
    UNIT_TEST_EXPECT_FALSE(stateData->GetJobsBySourceName("none", jobs));

    //trying to add a job without a valid source pk should fail:
    {
        UnitTestUtils::AssertAbsorber absorber;
        job = JobDatabaseEntry(234234, "jobkey", validFingerprint1, "pc", validBuilderGuid1, statusQueued, 1);
        UNIT_TEST_EXPECT_FALSE(stateData->SetJob(job));
        UNIT_TEST_EXPECT_TRUE(absorber.m_numWarningsAbsorbed > 0);
    }

    //trying to add a job with a valid source pk but an invalid job id  should fail:
    {
        UnitTestUtils::AssertAbsorber absorb;
        job = JobDatabaseEntry(source.m_sourceID, "jobkey", validFingerprint1, "pc", validBuilderGuid1, statusQueued, 0);
        UNIT_TEST_EXPECT_FALSE(stateData->SetJob(job));
        UNIT_TEST_EXPECT_TRUE(absorb.m_numErrorsAbsorbed > 0);
    }

    //setting a valid scan folder pk should allow it to be added AND should tell you what the job ID will be.
    // the run key should be untouched.
    job = JobDatabaseEntry(source.m_sourceID, "jobKey1", validFingerprint1, "pc", validBuilderGuid1, statusQueued, 1);
    UNIT_TEST_EXPECT_TRUE(stateData->SetJob(job));
    UNIT_TEST_EXPECT_TRUE(job.m_jobID != AzToolsFramework::AssetDatabase::InvalidEntryId);
    UNIT_TEST_EXPECT_TRUE(job.m_jobRunKey == 1);

    //get all jobs, there should only the one we added
    jobs.clear();
    UNIT_TEST_EXPECT_TRUE(stateData->GetJobs(jobs));
    UNIT_TEST_EXPECT_TRUE(jobs.size() == 1);
    UNIT_TEST_EXPECT_TRUE(JobsContainJobID(jobs, job.m_jobID));
    UNIT_TEST_EXPECT_TRUE(JobsContainJobKey(jobs, job.m_jobKey.c_str()));
    UNIT_TEST_EXPECT_TRUE(JobsContainFingerprint(jobs, job.m_fingerprint));
    UNIT_TEST_EXPECT_TRUE(JobsContainPlatform(jobs, job.m_platform.c_str()));
    UNIT_TEST_EXPECT_TRUE(JobsContainBuilderGuid(jobs, job.m_builderGuid));
    UNIT_TEST_EXPECT_TRUE(JobsContainStatus(jobs, job.m_status));
    UNIT_TEST_EXPECT_TRUE(JobsContainRunKey(jobs, job.m_jobRunKey));

    //add the same job again, should not add another because it already exists, so we should get the same id
    JobDatabaseEntry dupeJob(job);
    dupeJob.m_jobID = AzToolsFramework::AssetDatabase::InvalidEntryId;
    UNIT_TEST_EXPECT_TRUE(stateData->SetJob(dupeJob));
    if (!(dupeJob == job))
    {
        Q_EMIT UnitTestFailed("AssetProcessingStateDataTest Failed - SetJob failed to add");
        return;
    }

    //get all jobs, there should still only the one we added
    jobs.clear();
    UNIT_TEST_EXPECT_TRUE(stateData->GetJobs(jobs));
    UNIT_TEST_EXPECT_TRUE(jobs.size() == 1);
    UNIT_TEST_EXPECT_TRUE(JobsContainJobID(jobs, job.m_jobID));
    UNIT_TEST_EXPECT_TRUE(JobsContainJobKey(jobs, job.m_jobKey.c_str()));
    UNIT_TEST_EXPECT_TRUE(JobsContainFingerprint(jobs, job.m_fingerprint));
    UNIT_TEST_EXPECT_TRUE(JobsContainPlatform(jobs, job.m_platform.c_str()));
    UNIT_TEST_EXPECT_TRUE(JobsContainBuilderGuid(jobs, job.m_builderGuid));
    UNIT_TEST_EXPECT_TRUE(JobsContainStatus(jobs, job.m_status));

    //try retrieving this source by id
    UNIT_TEST_EXPECT_TRUE(stateData->GetJobByJobID(job.m_jobID, job));
    if (job.m_jobID == AzToolsFramework::AssetDatabase::InvalidEntryId ||
        job.m_jobID != job.m_jobID ||
        job.m_sourcePK != job.m_sourcePK ||
        job.m_jobKey != job.m_jobKey ||
        job.m_fingerprint != job.m_fingerprint ||
        job.m_platform != job.m_platform)
    {
        Q_EMIT UnitTestFailed("AssetProcessingStateDataTest Failed - GetJobByJobID failed");
        return;
    }

    //try retrieving jobs by source id
    jobs.clear();
    UNIT_TEST_EXPECT_TRUE(stateData->GetJobsBySourceID(source.m_sourceID, jobs));
    UNIT_TEST_EXPECT_TRUE(jobs.size() == 1);
    UNIT_TEST_EXPECT_TRUE(JobsContainJobID(jobs, job.m_jobID));
    UNIT_TEST_EXPECT_TRUE(JobsContainJobKey(jobs, job.m_jobKey.c_str()));
    UNIT_TEST_EXPECT_TRUE(JobsContainFingerprint(jobs, job.m_fingerprint));
    UNIT_TEST_EXPECT_TRUE(JobsContainPlatform(jobs, job.m_platform.c_str()));
    UNIT_TEST_EXPECT_TRUE(JobsContainBuilderGuid(jobs, job.m_builderGuid));
    UNIT_TEST_EXPECT_TRUE(JobsContainStatus(jobs, job.m_status));

    //try retrieving jobs by source name
    jobs.clear();
    UNIT_TEST_EXPECT_TRUE(stateData->GetJobsBySourceName(source.m_sourceName.c_str(), jobs));
    UNIT_TEST_EXPECT_TRUE(jobs.size() == 1);
    UNIT_TEST_EXPECT_TRUE(JobsContainJobID(jobs, job.m_jobID));
    UNIT_TEST_EXPECT_TRUE(JobsContainJobKey(jobs, job.m_jobKey.c_str()));
    UNIT_TEST_EXPECT_TRUE(JobsContainFingerprint(jobs, job.m_fingerprint));
    UNIT_TEST_EXPECT_TRUE(JobsContainPlatform(jobs, job.m_platform.c_str()));
    UNIT_TEST_EXPECT_TRUE(JobsContainBuilderGuid(jobs, job.m_builderGuid));
    UNIT_TEST_EXPECT_TRUE(JobsContainStatus(jobs, job.m_status));

    //remove a job
    UNIT_TEST_EXPECT_TRUE(stateData->RemoveJob(432234));
    UNIT_TEST_EXPECT_TRUE(stateData->RemoveJob(job.m_jobID));

    //get all jobs, there shouldn't be any
    jobs.clear();
    UNIT_TEST_EXPECT_FALSE(stateData->GetJobs(jobs));

    //Add two jobs then delete the via container
    JobDatabaseEntry job2(source2.m_sourceID, "jobkey2", validFingerprint2, "pc", validBuilderGuid2, statusQueued, 2);
    UNIT_TEST_EXPECT_TRUE(stateData->SetJob(job2));
    JobDatabaseEntry job3(source3.m_sourceID, "jobkey3", validFingerprint3, "pc", validBuilderGuid3, statusQueued, 3);
    UNIT_TEST_EXPECT_TRUE(stateData->SetJob(job3));

    //get all jobs, there should be 3
    jobs.clear();
    UNIT_TEST_EXPECT_TRUE(stateData->GetJobs(jobs));
    UNIT_TEST_EXPECT_TRUE(jobs.size() == 2);
    UNIT_TEST_EXPECT_TRUE(JobsContainJobID(jobs, job2.m_jobID));
    UNIT_TEST_EXPECT_TRUE(JobsContainJobKey(jobs, job2.m_jobKey.c_str()));
    UNIT_TEST_EXPECT_TRUE(JobsContainFingerprint(jobs, job2.m_fingerprint));
    UNIT_TEST_EXPECT_TRUE(JobsContainPlatform(jobs, job2.m_platform.c_str()));
    UNIT_TEST_EXPECT_TRUE(JobsContainBuilderGuid(jobs, job2.m_builderGuid));
    UNIT_TEST_EXPECT_TRUE(JobsContainStatus(jobs, job2.m_status));
    UNIT_TEST_EXPECT_TRUE(JobsContainJobID(jobs, job3.m_jobID));
    UNIT_TEST_EXPECT_TRUE(JobsContainJobKey(jobs, job3.m_jobKey.c_str()));
    UNIT_TEST_EXPECT_TRUE(JobsContainFingerprint(jobs, job3.m_fingerprint));
    UNIT_TEST_EXPECT_TRUE(JobsContainPlatform(jobs, job3.m_platform.c_str()));
    UNIT_TEST_EXPECT_TRUE(JobsContainBuilderGuid(jobs, job3.m_builderGuid));
    UNIT_TEST_EXPECT_TRUE(JobsContainStatus(jobs, job3.m_status));

    //Remove job via container
    JobDatabaseEntryContainer tempJobDatabaseEntryContainer;
    UNIT_TEST_EXPECT_TRUE(stateData->RemoveJobs(tempJobDatabaseEntryContainer)); // make sure it works on an empty container.
    UNIT_TEST_EXPECT_TRUE(stateData->RemoveJobs(jobs));

    //get all jobs, there should none
    jobs.clear();
    UNIT_TEST_EXPECT_FALSE(stateData->GetJobs(jobs));

    //Add two jobs then delete the via removing by source
    job2 = JobDatabaseEntry(source.m_sourceID, "jobkey2", validFingerprint2, "pc", validBuilderGuid2, statusQueued, 4);
    UNIT_TEST_EXPECT_TRUE(stateData->SetJob(job2));
    job3 = JobDatabaseEntry(source.m_sourceID, "jobkey3", validFingerprint3, "pc", validBuilderGuid3, statusQueued, 5);
    UNIT_TEST_EXPECT_TRUE(stateData->SetJob(job3));

    //remove the scan folder for these jobs, the jobs should cascade delete
    UNIT_TEST_EXPECT_TRUE(stateData->RemoveSource(source.m_sourceID));

    //get all jobs, there should none
    jobs.clear();
    UNIT_TEST_EXPECT_FALSE(stateData->GetJobs(jobs));

    ////////////////////////////////////////////////////////////////
    //Setup for product tests by having a some sources and jobs
    source = SourceDatabaseEntry(scanFolder.m_scanFolderID, "SomeSource1.tif", validSourceGuid1, "");
    UNIT_TEST_EXPECT_TRUE(stateData->SetSource(source));

    //Add jobs
    job = JobDatabaseEntry(source.m_sourceID, "jobkey1", validFingerprint1, "pc", validBuilderGuid1, statusCompleted, 6);
    UNIT_TEST_EXPECT_TRUE(stateData->SetJob(job));
    job2 = JobDatabaseEntry(source.m_sourceID, "jobkey2", validFingerprint2, "pc", validBuilderGuid2, statusCompleted, 7);
    UNIT_TEST_EXPECT_TRUE(stateData->SetJob(job2));
    job3 = JobDatabaseEntry(source.m_sourceID, "jobkey3", validFingerprint3, "pc", validBuilderGuid3, statusCompleted, 8);
    UNIT_TEST_EXPECT_TRUE(stateData->SetJob(job3));
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
    UNIT_TEST_EXPECT_TRUE(stateData->SetJob(job));
    job2 = JobDatabaseEntry(source.m_sourceID, "jobkey2", validFingerprint2, "pc", validBuilderGuid2, statusCompleted, 10);
    UNIT_TEST_EXPECT_TRUE(stateData->SetJob(job2));
    job3 = JobDatabaseEntry(source.m_sourceID, "jobkey3", validFingerprint3, "pc", validBuilderGuid3, statusCompleted, 11);
    UNIT_TEST_EXPECT_TRUE(stateData->SetJob(job3));

    //Add two products then delete the via removing the job
    ProductDatabaseEntry product2 = ProductDatabaseEntry(job.m_jobID, 2, "SomeProduct2.dds", validAssetType2);
    UNIT_TEST_EXPECT_TRUE(stateData->SetProduct(product2));
    ProductDatabaseEntry product3 = ProductDatabaseEntry(job.m_jobID, 3, "SomeProduct3.dds", validAssetType3);
    UNIT_TEST_EXPECT_TRUE(stateData->SetProduct(product3));

    //the products should cascade delete
    UNIT_TEST_EXPECT_TRUE(stateData->RemoveSource(source.m_sourceID));

    //get all products, there should none
    products.clear();
    UNIT_TEST_EXPECT_FALSE(stateData->GetProducts(products));

    // ---- test legacy subIds table ----

    // setup:
    // SomeSource1.tif
    //   jobkey1
    //     someproduct1
    //        legacy ids...

    source = SourceDatabaseEntry(scanFolder.m_scanFolderID, "SomeSource1.tif", validSourceGuid1, "");
    UNIT_TEST_EXPECT_TRUE(stateData->SetSource(source));

    job = JobDatabaseEntry(source.m_sourceID, "jobkey1", validFingerprint1, "pc", validBuilderGuid1, statusCompleted, 6);
    UNIT_TEST_EXPECT_TRUE(stateData->SetJob(job));

    product = ProductDatabaseEntry(job.m_jobID, 1, "SomeProduct1.dds", validAssetType1);
    product2 = ProductDatabaseEntry(job.m_jobID, 2, "SomeProduct2.dds", validAssetType1);
    UNIT_TEST_EXPECT_TRUE(stateData->SetProduct(product));
    UNIT_TEST_EXPECT_TRUE(stateData->SetProduct(product2));

    // test invalid insert for non existant legacy subids.
    LegacySubIDsEntry legacyEntry(1, product.m_productID, 3);
    {
        UnitTestUtils::AssertAbsorber absorb;
        UNIT_TEST_EXPECT_FALSE(stateData->CreateOrUpdateLegacySubID(legacyEntry));
        UNIT_TEST_EXPECT_TRUE(absorb.m_numWarningsAbsorbed > 0);
    }

    // test invalid insert for non-existant legacy product FK constraint
    legacyEntry = LegacySubIDsEntry(AzToolsFramework::AssetDatabase::InvalidEntryId, 9999, 3);
    {
        UnitTestUtils::AssertAbsorber absorb;
        UNIT_TEST_EXPECT_FALSE(stateData->CreateOrUpdateLegacySubID(legacyEntry));
        UNIT_TEST_EXPECT_TRUE(absorb.m_numWarningsAbsorbed > 0);
    }

    // test valid insert of another for same product
    legacyEntry = LegacySubIDsEntry(AzToolsFramework::AssetDatabase::InvalidEntryId, product.m_productID, 3);
    UNIT_TEST_EXPECT_TRUE(stateData->CreateOrUpdateLegacySubID(legacyEntry));
    AZ::s64 newPK = legacyEntry.m_subIDsEntryID;
    UNIT_TEST_EXPECT_TRUE(newPK != AzToolsFramework::AssetDatabase::InvalidEntryId); // it should have also updated the PK
    
    legacyEntry = LegacySubIDsEntry(AzToolsFramework::AssetDatabase::InvalidEntryId, product.m_productID, 4);
    UNIT_TEST_EXPECT_TRUE(stateData->CreateOrUpdateLegacySubID(legacyEntry));
    UNIT_TEST_EXPECT_TRUE(legacyEntry.m_subIDsEntryID != AzToolsFramework::AssetDatabase::InvalidEntryId); // it should have also updated the PK
    UNIT_TEST_EXPECT_TRUE(legacyEntry.m_subIDsEntryID != newPK); // pk should be unique

    // test valid insert of another for different product
    legacyEntry = LegacySubIDsEntry(AzToolsFramework::AssetDatabase::InvalidEntryId, product2.m_productID, 5);
    UNIT_TEST_EXPECT_TRUE(stateData->CreateOrUpdateLegacySubID(legacyEntry));

    // test that the ones inserted can be retrieved.
    AZStd::vector<LegacySubIDsEntry> entriesReturned;
    auto handler = [&entriesReturned](LegacySubIDsEntry& entry)
    {
        entriesReturned.push_back(entry);
        return true;
    };
    UNIT_TEST_EXPECT_TRUE(stateData->QueryLegacySubIdsByProductID(product.m_productID, handler));
    UNIT_TEST_EXPECT_TRUE(entriesReturned.size() == 2);
    
    bool foundSubID3 = false;
    bool foundSubID4 = false;
    for (const LegacySubIDsEntry& entryFound : entriesReturned)
    {
        UNIT_TEST_EXPECT_TRUE(entryFound.m_subIDsEntryID != AzToolsFramework::AssetDatabase::InvalidEntryId);
        UNIT_TEST_EXPECT_TRUE(entryFound.m_productPK == product.m_productID);
        if (entryFound.m_subID == 3)
        {
            foundSubID3 = true;
        }
        else if (entryFound.m_subID == 4)
        {
            foundSubID4 = true;
        }
    }

    UNIT_TEST_EXPECT_TRUE(foundSubID3);
    UNIT_TEST_EXPECT_TRUE(foundSubID4);

    entriesReturned.clear();

    UNIT_TEST_EXPECT_TRUE(stateData->QueryLegacySubIdsByProductID(product2.m_productID, handler));
    UNIT_TEST_EXPECT_TRUE(entriesReturned.size() == 1);
    UNIT_TEST_EXPECT_TRUE(entriesReturned[0].m_subIDsEntryID != AzToolsFramework::AssetDatabase::InvalidEntryId);
    UNIT_TEST_EXPECT_TRUE(entriesReturned[0].m_productPK == product2.m_productID);
    UNIT_TEST_EXPECT_TRUE(entriesReturned[0].m_subID == 5);

    // test UPDATE -> overwrite existing.
    entriesReturned[0].m_subID = 6;
    UNIT_TEST_EXPECT_TRUE(stateData->CreateOrUpdateLegacySubID(entriesReturned[0]));
    entriesReturned.clear();
    
    UNIT_TEST_EXPECT_TRUE(stateData->QueryLegacySubIdsByProductID(product2.m_productID, handler));
    UNIT_TEST_EXPECT_TRUE(entriesReturned.size() == 1);
    UNIT_TEST_EXPECT_TRUE(entriesReturned[0].m_subIDsEntryID != AzToolsFramework::AssetDatabase::InvalidEntryId);
    UNIT_TEST_EXPECT_TRUE(entriesReturned[0].m_productPK == product2.m_productID);
    UNIT_TEST_EXPECT_TRUE(entriesReturned[0].m_subID == 6);

    // test delete by product ID
    UNIT_TEST_EXPECT_TRUE(stateData->RemoveLegacySubIDsByProductID(product2.m_productID));
    entriesReturned.clear();
    
    UNIT_TEST_EXPECT_TRUE(stateData->QueryLegacySubIdsByProductID(product2.m_productID, handler));
    UNIT_TEST_EXPECT_TRUE(entriesReturned.empty());
    
    // test delete by PK.  The prior entries should be here for product1. This also makes sure the above
    // delete statement didn't delete more than it should have.
    
    UNIT_TEST_EXPECT_TRUE(stateData->QueryLegacySubIdsByProductID(product.m_productID, handler));
    UNIT_TEST_EXPECT_TRUE(entriesReturned.size() == 2);
    
    AZ::s64 toRemove = entriesReturned[0].m_subIDsEntryID;
    AZ::u32 removingSubID = entriesReturned[0].m_subID;

    UNIT_TEST_EXPECT_TRUE(stateData->RemoveLegacySubID(toRemove));
    entriesReturned.clear();
    UNIT_TEST_EXPECT_TRUE(stateData->QueryLegacySubIdsByProductID(product.m_productID, handler));
    UNIT_TEST_EXPECT_TRUE(entriesReturned.size() == 1);
    UNIT_TEST_EXPECT_TRUE(entriesReturned[0].m_subIDsEntryID != AzToolsFramework::AssetDatabase::InvalidEntryId);
    UNIT_TEST_EXPECT_TRUE(entriesReturned[0].m_subIDsEntryID != toRemove);
    UNIT_TEST_EXPECT_TRUE(entriesReturned[0].m_productPK == product.m_productID);
    UNIT_TEST_EXPECT_TRUE(entriesReturned[0].m_subID != removingSubID);

    ////////////////////////////////////////////////////////////////
    //Setup for product dependency tests by having a some sources and jobs
    source = SourceDatabaseEntry(scanFolder.m_scanFolderID, "SomeSource1.tif", validSourceGuid1, "");
    UNIT_TEST_EXPECT_TRUE(stateData->SetSource(source));
    source2 = SourceDatabaseEntry(scanFolder.m_scanFolderID, "SomeSource2.tif", validSourceGuid2, "");
    UNIT_TEST_EXPECT_TRUE(stateData->SetSource(source2));
    source3 = SourceDatabaseEntry(scanFolder.m_scanFolderID, "SomeSource3.tif", validSourceGuid3, "");
    UNIT_TEST_EXPECT_TRUE(stateData->SetSource(source3));
    SourceDatabaseEntry source4 = SourceDatabaseEntry(scanFolder.m_scanFolderID, "SomeSource4.tif", validSourceGuid4, "");
    UNIT_TEST_EXPECT_TRUE(stateData->SetSource(source4));
    SourceDatabaseEntry source5 = SourceDatabaseEntry(scanFolder.m_scanFolderID, "SomeSource5.tif", validSourceGuid5, "");
    UNIT_TEST_EXPECT_TRUE(stateData->SetSource(source5));
    SourceDatabaseEntry source6 = SourceDatabaseEntry(scanFolder.m_scanFolderID, "SomeSource6.tif", validSourceGuid6, "");
    UNIT_TEST_EXPECT_TRUE(stateData->SetSource(source6));

    //Add jobs
    job = JobDatabaseEntry(source.m_sourceID, "jobkey1", validFingerprint1, "pc", validBuilderGuid1, statusCompleted, 6);
    UNIT_TEST_EXPECT_TRUE(stateData->SetJob(job));
    job2 = JobDatabaseEntry(source2.m_sourceID, "jobkey2", validFingerprint2, "pc", validBuilderGuid2, statusCompleted, 7);
    UNIT_TEST_EXPECT_TRUE(stateData->SetJob(job2));
    job3 = JobDatabaseEntry(source3.m_sourceID, "jobkey3", validFingerprint3, "pc", validBuilderGuid3, statusCompleted, 8);
    UNIT_TEST_EXPECT_TRUE(stateData->SetJob(job3));
    JobDatabaseEntry job4 = JobDatabaseEntry(source4.m_sourceID, "jobkey4", validFingerprint4, "pc", validBuilderGuid4, statusCompleted, 9);
    UNIT_TEST_EXPECT_TRUE(stateData->SetJob(job4));
    JobDatabaseEntry job5 = JobDatabaseEntry(source5.m_sourceID, "jobkey5", validFingerprint5, "pc", validBuilderGuid5, statusCompleted, 10);
    UNIT_TEST_EXPECT_TRUE(stateData->SetJob(job5));
    JobDatabaseEntry job6 = JobDatabaseEntry(source6.m_sourceID, "jobkey6", validFingerprint6, "pc", validBuilderGuid6, statusCompleted, 11);
    UNIT_TEST_EXPECT_TRUE(stateData->SetJob(job6));

    //Add products
    product = ProductDatabaseEntry(job.m_jobID, 1, "SomeProduct1.dds", validAssetType1);
    UNIT_TEST_EXPECT_TRUE(stateData->SetProduct(product));
    product2 = ProductDatabaseEntry(job2.m_jobID, 2, "SomeProduct2.dds", validAssetType2);
    UNIT_TEST_EXPECT_TRUE(stateData->SetProduct(product2));
    product3 = ProductDatabaseEntry(job3.m_jobID, 3, "SomeProduct3.dds", validAssetType3);
    UNIT_TEST_EXPECT_TRUE(stateData->SetProduct(product3));
    ProductDatabaseEntry product4 = ProductDatabaseEntry(job4.m_jobID, 4, "SomeProduct4.dds", validAssetType4);
    UNIT_TEST_EXPECT_TRUE(stateData->SetProduct(product4));
    ProductDatabaseEntry product5 = ProductDatabaseEntry(job5.m_jobID, 5, "SomeProduct5.dds", validAssetType5);
    UNIT_TEST_EXPECT_TRUE(stateData->SetProduct(product5));
    ProductDatabaseEntry product6 = ProductDatabaseEntry(job6.m_jobID, 6, "SomeProduct6.dds", validAssetType6);
    UNIT_TEST_EXPECT_TRUE(stateData->SetProduct(product6));


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
    UNIT_TEST_EXPECT_FALSE(stateData->GetProductDependencies(productDependencies));
    UNIT_TEST_EXPECT_FALSE(stateData->GetProductDependencyByProductDependencyID(3443, productDependency));
    UNIT_TEST_EXPECT_FALSE(stateData->GetProductDependenciesByProductID(3443, productDependencies));
    UNIT_TEST_EXPECT_FALSE(stateData->GetDirectProductDependencies(3443, products));
    UNIT_TEST_EXPECT_FALSE(stateData->GetAllProductDependencies(3443, products));

    AZStd::string platform;
    //trying to add a product dependency without a valid product pk should fail
    productDependency = ProductDependencyDatabaseEntry(234234, validSourceGuid1, 1, 0, platform, true);
    {
        UnitTestUtils::AssertAbsorber absorber;
        UNIT_TEST_EXPECT_FALSE(stateData->SetProductDependency(productDependency));
        UNIT_TEST_EXPECT_TRUE(absorber.m_numWarningsAbsorbed > 0);
    }

    //setting a valid product pk should allow it to be added
    //Product -> Product2
    productDependency = ProductDependencyDatabaseEntry(product.m_productID, validSourceGuid2, 2, 0, platform, true);
    UNIT_TEST_EXPECT_TRUE(stateData->SetProductDependency(productDependency));
    if (productDependency.m_productDependencyID == AzToolsFramework::AssetDatabase::InvalidEntryId)
    {
        Q_EMIT UnitTestFailed("AssetProcessingStateDataTest Failed - SetProductDependency failed to add");
        return;
    }

    //get all product dependencies, there should only the one we added
    productDependencies.clear();
    UNIT_TEST_EXPECT_TRUE(stateData->GetProductDependencies(productDependencies));
    UNIT_TEST_EXPECT_TRUE(productDependencies.size() == 1);

    UNIT_TEST_EXPECT_TRUE(ProductDependenciesContainProductDependencyID(productDependencies, productDependency.m_productDependencyID));
    UNIT_TEST_EXPECT_TRUE(ProductDependenciesContainProductID(productDependencies, productDependency.m_productPK));
    UNIT_TEST_EXPECT_TRUE(ProductDependenciesContainDependencySoureGuid(productDependencies, productDependency.m_dependencySourceGuid));
    UNIT_TEST_EXPECT_TRUE(ProductDependenciesContainDependencySubID(productDependencies, productDependency.m_dependencySubID));
    UNIT_TEST_EXPECT_TRUE(ProductDependenciesContainDependencyFlags(productDependencies, productDependency.m_dependencyFlags));

    //add the same product again, should not add another because it already exists, so we should get the same id
    ProductDependencyDatabaseEntry dupeProductDependency(productDependency);
    dupeProductDependency.m_productDependencyID = AzToolsFramework::AssetDatabase::InvalidEntryId;
    UNIT_TEST_EXPECT_TRUE(stateData->SetProductDependency(dupeProductDependency));
    if (!(dupeProductDependency == dupeProductDependency))
    {
        Q_EMIT UnitTestFailed("AssetProcessingStateDataTest Failed - SetProductDependency failed to add");
        return;
    }

    //get all product dependencies, there should still only the one we added
    productDependencies.clear();
    UNIT_TEST_EXPECT_TRUE(stateData->GetProductDependencies(productDependencies));
    UNIT_TEST_EXPECT_TRUE(productDependencies.size() == 1);
    UNIT_TEST_EXPECT_TRUE(ProductDependenciesContainProductDependencyID(productDependencies, productDependency.m_productDependencyID));
    UNIT_TEST_EXPECT_TRUE(ProductDependenciesContainProductID(productDependencies, productDependency.m_productPK));
    UNIT_TEST_EXPECT_TRUE(ProductDependenciesContainDependencySoureGuid(productDependencies, productDependency.m_dependencySourceGuid));
    UNIT_TEST_EXPECT_TRUE(ProductDependenciesContainDependencySubID(productDependencies, productDependency.m_dependencySubID));
    UNIT_TEST_EXPECT_TRUE(ProductDependenciesContainDependencyFlags(productDependencies, productDependency.m_dependencyFlags));
    
    // Setup some more dependencies

    //Product2 -> Product3
    productDependency = ProductDependencyDatabaseEntry(product2.m_productID, validSourceGuid3, 3, 0, platform, true);
    UNIT_TEST_EXPECT_TRUE(stateData->SetProductDependency(productDependency));

    //Product2 -> Product4
    productDependency = ProductDependencyDatabaseEntry(product2.m_productID, validSourceGuid4, 4, 0, platform, true);
    UNIT_TEST_EXPECT_TRUE(stateData->SetProductDependency(productDependency));

    //Product3 -> Product5
    productDependency = ProductDependencyDatabaseEntry(product3.m_productID, validSourceGuid5, 5, 0, platform, true);
    UNIT_TEST_EXPECT_TRUE(stateData->SetProductDependency(productDependency));

    //Product5 -> Product6
    productDependency = ProductDependencyDatabaseEntry(product5.m_productID, validSourceGuid6, 6, 0, platform, true);
    UNIT_TEST_EXPECT_TRUE(stateData->SetProductDependency(productDependency));

    /* Dependency Tree
    *
    * Product -> Product2 -> Product3 -> Product5 -> Product 6->
    *                    \                          
    *                     -> Product4
    */
    
    // Direct Deps

    // Product -> Product2
    products.clear();
    UNIT_TEST_EXPECT_TRUE(stateData->GetDirectProductDependencies(product.m_productID, products));
    UNIT_TEST_EXPECT_TRUE(products.size() == 1);
    UNIT_TEST_EXPECT_TRUE(ProductsContainProductID(products, product2.m_productID));

    // Product2 -> Product3, Product4
    products.clear();
    UNIT_TEST_EXPECT_TRUE(stateData->GetDirectProductDependencies(product2.m_productID, products));
    UNIT_TEST_EXPECT_TRUE(products.size() == 2);
    UNIT_TEST_EXPECT_TRUE(ProductsContainProductID(products, product3.m_productID));
    UNIT_TEST_EXPECT_TRUE(ProductsContainProductID(products, product4.m_productID));

    // Product3 -> Product5
    products.clear();
    UNIT_TEST_EXPECT_TRUE(stateData->GetDirectProductDependencies(product3.m_productID, products));
    UNIT_TEST_EXPECT_TRUE(products.size() == 1);
    UNIT_TEST_EXPECT_TRUE(ProductsContainProductID(products, product5.m_productID));

    // Product4 ->
    products.clear();
    UNIT_TEST_EXPECT_FALSE(stateData->GetDirectProductDependencies(product4.m_productID, products));
    UNIT_TEST_EXPECT_TRUE(products.size() == 0);

    // Product5 -> Product6
    products.clear();
    UNIT_TEST_EXPECT_TRUE(stateData->GetDirectProductDependencies(product5.m_productID, products));
    UNIT_TEST_EXPECT_TRUE(products.size() == 1);
    UNIT_TEST_EXPECT_TRUE(ProductsContainProductID(products, product6.m_productID));

    // Product6 ->
    products.clear();
    UNIT_TEST_EXPECT_FALSE(stateData->GetDirectProductDependencies(product6.m_productID, products));
    UNIT_TEST_EXPECT_TRUE(products.size() == 0);

    // All Deps

    // Product -> Product2, Product3, Product4, Product5, Product6
    products.clear();
    UNIT_TEST_EXPECT_TRUE(stateData->GetAllProductDependencies(product.m_productID, products));
    UNIT_TEST_EXPECT_TRUE(products.size() == 5);
    UNIT_TEST_EXPECT_TRUE(ProductsContainProductID(products, product2.m_productID));
    UNIT_TEST_EXPECT_TRUE(ProductsContainProductID(products, product3.m_productID));
    UNIT_TEST_EXPECT_TRUE(ProductsContainProductID(products, product4.m_productID));
    UNIT_TEST_EXPECT_TRUE(ProductsContainProductID(products, product5.m_productID));
    UNIT_TEST_EXPECT_TRUE(ProductsContainProductID(products, product6.m_productID));

    // Product2 -> Product3, Product4, Product5, Product6
    products.clear();
    UNIT_TEST_EXPECT_TRUE(stateData->GetAllProductDependencies(product2.m_productID, products));
    UNIT_TEST_EXPECT_TRUE(products.size() == 4);
    UNIT_TEST_EXPECT_TRUE(ProductsContainProductID(products, product3.m_productID));
    UNIT_TEST_EXPECT_TRUE(ProductsContainProductID(products, product4.m_productID));
    UNIT_TEST_EXPECT_TRUE(ProductsContainProductID(products, product5.m_productID));
    UNIT_TEST_EXPECT_TRUE(ProductsContainProductID(products, product6.m_productID));

    // Product3 -> Product5, Product6
    products.clear();
    UNIT_TEST_EXPECT_TRUE(stateData->GetAllProductDependencies(product3.m_productID, products));
    UNIT_TEST_EXPECT_TRUE(products.size() == 2);
    UNIT_TEST_EXPECT_TRUE(ProductsContainProductID(products, product5.m_productID));
    UNIT_TEST_EXPECT_TRUE(ProductsContainProductID(products, product6.m_productID));

    // Product4 ->
    products.clear();
    UNIT_TEST_EXPECT_FALSE(stateData->GetAllProductDependencies(product4.m_productID, products));
    UNIT_TEST_EXPECT_TRUE(products.size() == 0);

    // Product5 -> Product6
    products.clear();
    UNIT_TEST_EXPECT_TRUE(stateData->GetAllProductDependencies(product5.m_productID, products));
    UNIT_TEST_EXPECT_TRUE(products.size() == 1);
    UNIT_TEST_EXPECT_TRUE(ProductsContainProductID(products, product6.m_productID));

    // Product6 -> 
    products.clear();
    UNIT_TEST_EXPECT_FALSE(stateData->GetAllProductDependencies(product6.m_productID, products));
    UNIT_TEST_EXPECT_TRUE(products.size() == 0);

    //Product6 -> Product (This creates a circular dependency)
    productDependency = ProductDependencyDatabaseEntry(product6.m_productID, validSourceGuid1, 1, 0, platform, true);
    UNIT_TEST_EXPECT_TRUE(stateData->SetProductDependency(productDependency));

    /* Circular Dependency Tree
    * v--------------------------------------------------------<
    * |                                                        |
    * Product -> Product2 -> Product3 -> Product5 -> Product 6-^
    *                    \
    *                     -> Product4
    */

    // Product6 -> Product
    products.clear();
    UNIT_TEST_EXPECT_TRUE(stateData->GetDirectProductDependencies(product6.m_productID, products));
    UNIT_TEST_EXPECT_TRUE(products.size() == 1);
    UNIT_TEST_EXPECT_TRUE(ProductsContainProductID(products, product.m_productID));

    // Product3 -> Product5, Product6, Product, Product2, Product4
    products.clear();
    UNIT_TEST_EXPECT_TRUE(stateData->GetAllProductDependencies(product3.m_productID, products));
    UNIT_TEST_EXPECT_TRUE(products.size() == 5);
    UNIT_TEST_EXPECT_TRUE(ProductsContainProductID(products, product5.m_productID));
    UNIT_TEST_EXPECT_TRUE(ProductsContainProductID(products, product6.m_productID));
    UNIT_TEST_EXPECT_TRUE(ProductsContainProductID(products, product.m_productID));
    UNIT_TEST_EXPECT_TRUE(ProductsContainProductID(products, product2.m_productID));
    UNIT_TEST_EXPECT_TRUE(ProductsContainProductID(products, product4.m_productID));

    stateData->RemoveProductDependencyByProductId(product5.m_productID);
    products.clear();
    UNIT_TEST_EXPECT_TRUE(stateData->GetAllProductDependencies(product2.m_productID, products));
    UNIT_TEST_EXPECT_TRUE(products.size() == 3);
    UNIT_TEST_EXPECT_TRUE(ProductsContainProductID(products, product3.m_productID));
    UNIT_TEST_EXPECT_TRUE(ProductsContainProductID(products, product4.m_productID));
    UNIT_TEST_EXPECT_TRUE(ProductsContainProductID(products, product5.m_productID));

    // Teardown
    //The product dependencies should cascade delete
    UNIT_TEST_EXPECT_TRUE(stateData->RemoveSource(source.m_sourceID));
    UNIT_TEST_EXPECT_TRUE(stateData->RemoveSource(source2.m_sourceID));
    UNIT_TEST_EXPECT_TRUE(stateData->RemoveSource(source3.m_sourceID));
    UNIT_TEST_EXPECT_TRUE(stateData->RemoveSource(source4.m_sourceID));
    UNIT_TEST_EXPECT_TRUE(stateData->RemoveSource(source5.m_sourceID));
    UNIT_TEST_EXPECT_TRUE(stateData->RemoveSource(source6.m_sourceID));

    productDependencies.clear();
    products.clear();
    UNIT_TEST_EXPECT_FALSE(stateData->GetProductDependencies(productDependencies));
    UNIT_TEST_EXPECT_FALSE(stateData->GetDirectProductDependencies(product.m_productID, products));
    UNIT_TEST_EXPECT_FALSE(stateData->GetAllProductDependencies(product.m_productID, products));
}

void AssetProcessingStateDataUnitTest::ExistenceTest(AssetProcessor::AssetDatabaseConnection* stateData)
{
    UNIT_TEST_EXPECT_FALSE(stateData->DataExists());
    stateData->ClearData(); // this is expected to initialize a database.
    UNIT_TEST_EXPECT_TRUE(stateData->DataExists());
}

// test is broken out into its own function so as to be more compatible with a future GTEST-like API.
void AssetProcessingStateDataUnitTest::BuilderInfoTest(AssetProcessor::AssetDatabaseConnection* stateData)
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
    
    UNIT_TEST_EXPECT_TRUE(stateData->QueryBuilderInfoTable(resultGatherer));
    UNIT_TEST_EXPECT_TRUE(results.empty());

    BuilderInfoEntryContainer newEntries;

    newEntries.emplace_back(BuilderInfoEntry(AzToolsFramework::AssetDatabase::InvalidEntryId, AZ::Uuid::CreateString("{648B7B06-27A3-42AC-897D-FA4557C28654}"), "Finger_Print"));
    newEntries.emplace_back(BuilderInfoEntry(AzToolsFramework::AssetDatabase::InvalidEntryId, AZ::Uuid::CreateString("{0B657D45-A5B0-485B-BF34-0E8779F9A482}"), "Finger_Print"));

    UNIT_TEST_EXPECT_TRUE(stateData->SetBuilderInfoTable(newEntries));
    // make sure each entry has a number assigned:
    UNIT_TEST_EXPECT_TRUE(newEntries[0].m_builderInfoID != AzToolsFramework::AssetDatabase::InvalidEntryId);
    UNIT_TEST_EXPECT_TRUE(newEntries[1].m_builderInfoID != AzToolsFramework::AssetDatabase::InvalidEntryId);

    UNIT_TEST_EXPECT_TRUE(stateData->QueryBuilderInfoTable(resultGatherer));
    UNIT_TEST_EXPECT_TRUE(results.size() == 2);
    UNIT_TEST_EXPECT_TRUE(results[0].m_builderInfoID != AzToolsFramework::AssetDatabase::InvalidEntryId);
    UNIT_TEST_EXPECT_TRUE(results[1].m_builderInfoID != AzToolsFramework::AssetDatabase::InvalidEntryId);

    // they could be in any order, so fix that first:
    bool isInCorrectOrder = (results[0].m_builderInfoID == newEntries[0].m_builderInfoID) && (results[1].m_builderInfoID == newEntries[1].m_builderInfoID);
    bool isInReverseOrder = (results[1].m_builderInfoID == newEntries[0].m_builderInfoID) && (results[0].m_builderInfoID == newEntries[1].m_builderInfoID);

    UNIT_TEST_EXPECT_TRUE(isInCorrectOrder || isInReverseOrder);

    if (isInReverseOrder)
    {
        BuilderInfoEntry temp = results[0];
        results[0] = results[1];
        results[1] = temp;
    }

    for (size_t idx = 0; idx < 2; ++idx)
    {
        UNIT_TEST_EXPECT_TRUE(results[idx].m_builderUuid == newEntries[idx].m_builderUuid);
        UNIT_TEST_EXPECT_TRUE(results[idx].m_builderInfoID == newEntries[idx].m_builderInfoID);
        UNIT_TEST_EXPECT_TRUE(results[idx].m_analysisFingerprint == newEntries[idx].m_analysisFingerprint);
    }

    // now REPLACE the entries with fewer and make sure it actually chops it down and also replaces the fields.
    newEntries.clear();
    results.clear();
    newEntries.emplace_back(BuilderInfoEntry(AzToolsFramework::AssetDatabase::InvalidEntryId, AZ::Uuid::CreateString("{8863194A-BCB2-4A4C-A7D9-4E90D68814D4}"), "Finger_Print2"));
    UNIT_TEST_EXPECT_TRUE(stateData->SetBuilderInfoTable(newEntries));
    // make sure each entry has a number assigned:
    UNIT_TEST_EXPECT_TRUE(newEntries[0].m_builderInfoID != AzToolsFramework::AssetDatabase::InvalidEntryId);
    UNIT_TEST_EXPECT_TRUE(stateData->QueryBuilderInfoTable(resultGatherer));
    UNIT_TEST_EXPECT_TRUE(results.size() == 1);
    UNIT_TEST_EXPECT_TRUE(results[0].m_builderInfoID != AzToolsFramework::AssetDatabase::InvalidEntryId);
    UNIT_TEST_EXPECT_TRUE(results[0].m_builderUuid == newEntries[0].m_builderUuid);
    UNIT_TEST_EXPECT_TRUE(results[0].m_builderInfoID == newEntries[0].m_builderInfoID);
    UNIT_TEST_EXPECT_TRUE(results[0].m_analysisFingerprint == newEntries[0].m_analysisFingerprint);
}

void AssetProcessingStateDataUnitTest::SourceDependencyTest(AssetProcessor::AssetDatabaseConnection* stateData)
{
    using SourceFileDependencyEntry = AzToolsFramework::AssetDatabase::SourceFileDependencyEntry;
    using SourceFileDependencyEntryContainer = AzToolsFramework::AssetDatabase::SourceFileDependencyEntryContainer;

    //  A depends on B, which depends on both C and D

    SourceFileDependencyEntry newEntry1;  // a depends on B
    newEntry1.m_sourceDependencyID = AzToolsFramework::AssetDatabase::InvalidEntryId;
    newEntry1.m_builderGuid = AZ::Uuid::CreateRandom();
    newEntry1.m_source = "a.txt";
    newEntry1.m_dependsOnSource = "b.txt";
    
    SourceFileDependencyEntry newEntry2; // b depends on C
    newEntry2.m_sourceDependencyID = AzToolsFramework::AssetDatabase::InvalidEntryId;
    newEntry2.m_builderGuid = AZ::Uuid::CreateRandom();
    newEntry2.m_source = "b.txt";
    newEntry2.m_dependsOnSource = "c.txt";

    SourceFileDependencyEntry newEntry3;  // b also depends on D
    newEntry3.m_sourceDependencyID = AzToolsFramework::AssetDatabase::InvalidEntryId;
    newEntry3.m_builderGuid = AZ::Uuid::CreateRandom();
    newEntry3.m_source = "b.txt";
    newEntry3.m_dependsOnSource = "d.txt";

    UNIT_TEST_EXPECT_TRUE(stateData->SetSourceFileDependency(newEntry1));
    UNIT_TEST_EXPECT_TRUE(stateData->SetSourceFileDependency(newEntry2));
    UNIT_TEST_EXPECT_TRUE(stateData->SetSourceFileDependency(newEntry3));
    
    SourceFileDependencyEntryContainer results;

    // what depends on b?  a does.
    UNIT_TEST_EXPECT_TRUE(stateData->GetSourceFileDependenciesByDependsOnSource("b.txt", SourceFileDependencyEntry::DEP_Any, results));
    UNIT_TEST_EXPECT_TRUE(results.size() == 1);
    UNIT_TEST_EXPECT_TRUE(results[0].m_source == "a.txt");
    UNIT_TEST_EXPECT_TRUE(results[0].m_builderGuid == newEntry1.m_builderGuid);
    UNIT_TEST_EXPECT_TRUE(results[0].m_sourceDependencyID == newEntry1.m_sourceDependencyID);

    // what does B depend on?
    results.clear();
    UNIT_TEST_EXPECT_TRUE(stateData->GetDependsOnSourceBySource("b.txt", SourceFileDependencyEntry::DEP_Any, results));
    // b depends on 2 things: c and d
    UNIT_TEST_EXPECT_TRUE(results.size() == 2);
    UNIT_TEST_EXPECT_TRUE(results[0].m_source == "b.txt");  // note that both of these are B, since its B that has the dependency on the others.
    UNIT_TEST_EXPECT_TRUE(results[1].m_source == "b.txt");
    UNIT_TEST_EXPECT_TRUE(results[0].m_dependsOnSource == "c.txt"); 
    UNIT_TEST_EXPECT_TRUE(results[1].m_dependsOnSource == "d.txt");

    // what does b depend on, but filtered to only one builder?
    results.clear();
    UNIT_TEST_EXPECT_TRUE(stateData->GetSourceFileDependenciesByBuilderGUIDAndSource(newEntry2.m_builderGuid, "b.txt", SourceFileDependencyEntry::DEP_SourceToSource, results));
    // b depends on 1 thing from that builder: c
    UNIT_TEST_EXPECT_TRUE(results.size() == 1);
    UNIT_TEST_EXPECT_TRUE(results[0].m_source == "b.txt");
    UNIT_TEST_EXPECT_TRUE(results[0].m_dependsOnSource == "c.txt");

    // make sure that we can look these up by ID (a)
    UNIT_TEST_EXPECT_TRUE(stateData->GetSourceFileDependencyBySourceDependencyId(newEntry1.m_sourceDependencyID, results[0]));
    UNIT_TEST_EXPECT_TRUE(results[0].m_source == "a.txt");
    UNIT_TEST_EXPECT_TRUE(results[0].m_builderGuid == newEntry1.m_builderGuid);
    UNIT_TEST_EXPECT_TRUE(results[0].m_sourceDependencyID == newEntry1.m_sourceDependencyID);

    // remove D, b now should only depend on C
    results.clear();
    UNIT_TEST_EXPECT_TRUE(stateData->RemoveSourceFileDependency(newEntry3.m_sourceDependencyID));
    UNIT_TEST_EXPECT_TRUE(stateData->GetDependsOnSourceBySource("b.txt", SourceFileDependencyEntry::DEP_Any, results));
    UNIT_TEST_EXPECT_TRUE(results.size() == 1);
    UNIT_TEST_EXPECT_TRUE(results[0].m_dependsOnSource == "c.txt");


    // clean up
    UNIT_TEST_EXPECT_TRUE(stateData->RemoveSourceFileDependency(newEntry1.m_sourceDependencyID));
    UNIT_TEST_EXPECT_TRUE(stateData->RemoveSourceFileDependency(newEntry2.m_sourceDependencyID));
}


void AssetProcessingStateDataUnitTest::SourceFingerprintTest(AssetProcessor::AssetDatabaseConnection* stateData)
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

    UNIT_TEST_EXPECT_TRUE(stateData->SetScanFolder(scanFolder));

    SourceDatabaseEntry sourceFile1;
    sourceFile1.m_analysisFingerprint = "12345";
    sourceFile1.m_scanFolderPK = scanFolder.m_scanFolderID;
    sourceFile1.m_sourceGuid = AZ::Uuid::CreateRandom();
    sourceFile1.m_sourceName = "a.txt";
    UNIT_TEST_EXPECT_TRUE(stateData->SetSource(sourceFile1));
    
    SourceDatabaseEntry sourceFile2;
    sourceFile2.m_analysisFingerprint = "54321";
    sourceFile2.m_scanFolderPK = scanFolder.m_scanFolderID;
    sourceFile2.m_sourceGuid = AZ::Uuid::CreateRandom();
    sourceFile2.m_sourceName = "b.txt";

    UNIT_TEST_EXPECT_TRUE(stateData->SetSource(sourceFile2));

    AZStd::string resultString("garbage");
    // its not a database error to ask for a file that does not exist:
    UNIT_TEST_EXPECT_TRUE(stateData->QuerySourceAnalysisFingerprint("does not exist", scanFolder.m_scanFolderID, resultString));
    // but we do expect it to empty the result:
    UNIT_TEST_EXPECT_TRUE(resultString.empty());
    UNIT_TEST_EXPECT_TRUE(stateData->QuerySourceAnalysisFingerprint("a.txt", scanFolder.m_scanFolderID, resultString));
    UNIT_TEST_EXPECT_TRUE(resultString == "12345");
    UNIT_TEST_EXPECT_TRUE(stateData->QuerySourceAnalysisFingerprint("b.txt", scanFolder.m_scanFolderID, resultString));
    UNIT_TEST_EXPECT_TRUE(resultString == "54321");
}

void AssetProcessingStateDataUnitTest::AssetProcessingStateDataTest()
{
    using namespace AssetProcessingStateDataUnitTestInternal;
    using namespace AzToolsFramework::AssetDatabase;

    QDir dirPath;

    // intentional scope to contain QTemporaryDir since it cleans up on destruction!
    { 
        QTemporaryDir tempDir;
        ProductDatabaseEntryContainer products;
        dirPath = QDir(tempDir.path());

        bool testsFailed = false;
        connect(this, &UnitTestRun::UnitTestFailed, this, [&testsFailed]()
        {
            testsFailed = true;
        }, Qt::DirectConnection);

        // now test the SQLite version of the database on its own.
        {
            FakeDatabaseLocationListener listener(dirPath.filePath("statedatabase.sqlite").toUtf8().constData(), "displayString");
            AssetProcessor::AssetDatabaseConnection connection;

            ExistenceTest(&connection);
            if (testsFailed)
            {
                return;
            }

            DataTest(&connection);
            if (testsFailed)
            {
                return;
            }

            BuilderInfoTest(&connection);
            if (testsFailed)
            {
                return;
            }

            SourceFingerprintTest(&connection);
            if (testsFailed)
            {
                return;
            }

            SourceDependencyTest(&connection);
        }
    }
    // scope ending for the QTempDir
    // if this fails it means someone left a handle to the database open.
    UNIT_TEST_EXPECT_FALSE(dirPath.exists());

    Q_EMIT UnitTestPassed();
}

void AssetProcessingStateDataUnitTest::StartTest()
{
    AssetProcessingStateDataTest();
}

REGISTER_UNIT_TEST(AssetProcessingStateDataUnitTest)

