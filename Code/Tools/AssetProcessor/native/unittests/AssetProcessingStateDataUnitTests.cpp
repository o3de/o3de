/*
* Copyright (c) Contributors to the Open 3D Engine Project.
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
*
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/
#include <native/tests/AssetProcessorTest.h>

#include <native/unittests/AssetProcessorUnitTests.h>
#include <native/unittests/UnitTestUtils.h> // for the assert absorber.

using namespace AzToolsFramework::AssetDatabase;

namespace AssetProcessor
{
    namespace Internal
    {
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
    }

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

        void AddDefaultScanFolder()
        {
            // this isn't a real directory. It just creates an entry in the database, so any string would work.
            m_defaultScanFolder = ScanFolderDatabaseEntry("c:/O3DE/dev", "dev", "rootportkey");
            EXPECT_TRUE(m_connection.SetScanFolder(m_defaultScanFolder));
            EXPECT_NE(m_defaultScanFolder.m_scanFolderID, AzToolsFramework::AssetDatabase::InvalidEntryId);
        }

        void AddDefaultSource()
        {
            if (m_defaultScanFolder.m_scanFolderID == AzToolsFramework::AssetDatabase::InvalidEntryId)
            {
                AddDefaultScanFolder();
            }

            AZ::Uuid validSourceGuid = AZ::Uuid::CreateRandom();
            m_defaultSource = SourceDatabaseEntry(m_defaultScanFolder.m_scanFolderID, "SomeSource.tif", validSourceGuid, "12345");
            EXPECT_TRUE(m_connection.SetSource(m_defaultSource));
            EXPECT_NE(m_defaultSource.m_sourceID, AzToolsFramework::AssetDatabase::InvalidEntryId);
        }

        void AddDefaultJob()
        {
            if (m_defaultSource.m_sourceID == AzToolsFramework::AssetDatabase::InvalidEntryId)
            {
                AddDefaultSource();
            }

            AZ::u32 validFingerprint = 0;
            AZ::Uuid validBuilderGuid = AZ::Uuid::CreateRandom();
            AzToolsFramework::AssetSystem::JobStatus statusQueued = AzToolsFramework::AssetSystem::JobStatus::Queued;

            m_defaultJob = JobDatabaseEntry(m_defaultSource.m_sourceID, "jobKey", validFingerprint, "pc", validBuilderGuid, statusQueued, 1);
            EXPECT_TRUE(m_connection.SetJob(m_defaultJob));
            EXPECT_NE(m_defaultJob.m_jobID, AzToolsFramework::AssetDatabase::InvalidEntryId);
            EXPECT_EQ(m_defaultJob.m_jobRunKey, 1);
        }

        void AddDefaultProduct()
        {
            if (m_defaultJob.m_jobID == AzToolsFramework::AssetDatabase::InvalidEntryId)
            {
                AddDefaultJob();
            }

            AZ::Data::AssetType validAssetType = AZ::Data::AssetType::CreateRandom();
            m_defaultProduct = ProductDatabaseEntry(m_defaultJob.m_jobID, 0, "SomeProduct.dds", validAssetType);
            EXPECT_TRUE(m_connection.SetProduct(m_defaultProduct));
        }

        void AddDefaultLegacySubId()
        {
            if (m_defaultProduct.m_productID == AzToolsFramework::AssetDatabase::InvalidEntryId)
            {
                AddDefaultProduct();
            }

            m_defaultLegacyEntry = LegacySubIDsEntry(AzToolsFramework::AssetDatabase::InvalidEntryId, m_defaultProduct.m_productID, 0);
            EXPECT_TRUE(m_connection.CreateOrUpdateLegacySubID(m_defaultLegacyEntry));
            EXPECT_NE(m_defaultLegacyEntry.m_subIDsEntryID, AzToolsFramework::AssetDatabase::InvalidEntryId); // it should have also updated the PK
        }

        void CreateProductDependencyTree(SourceDatabaseEntryContainer& sources, ProductDatabaseEntryContainer& products, const AZStd::string& platform = "")
        {
            /* Create the product dependency tree as below
            *
            * products[0] -> products[1] -> products[2] -> products[4] -> products[5]
            *                    \
            *                     -> products[3]
            */
            AddDefaultProduct();

            // Add sources
            sources.emplace_back(m_defaultSource);
            for (int index = 1; index < 6; ++index)
            {
                AZ::Uuid validSourceGuid = AZ::Uuid::CreateRandom();
                sources.emplace_back(SourceDatabaseEntry(m_defaultScanFolder.m_scanFolderID, AZStd::string::format("SomeSource%d.tif", index).c_str(), validSourceGuid, ""));
                EXPECT_TRUE(m_connection.SetSource(sources[index]));
            }

            //Add jobs
            JobDatabaseEntryContainer jobs;
            jobs.emplace_back(m_defaultJob);
            AzToolsFramework::AssetSystem::JobStatus statusCompleted = AzToolsFramework::AssetSystem::JobStatus::Completed;
            for (int index = 1; index < 6; ++index)
            {
                AZ::Uuid validBuilderGuid = AZ::Uuid::CreateRandom();
                AZ::u32 validFingerprint = index;
                jobs.emplace_back(JobDatabaseEntry(sources[index].m_sourceID, AZStd::string::format("jobkey%d", index).c_str(), validFingerprint, "pc", validBuilderGuid, statusCompleted, index + 1));
                EXPECT_TRUE(m_connection.SetJob(jobs[index]));
            }

            //Add products
            products.emplace_back(m_defaultProduct);
            for (int index = 1; index < 6; ++index)
            {
                AZ::Data::AssetType validAssetType = AZ::Data::AssetType::CreateRandom();
                products.emplace_back(ProductDatabaseEntry(jobs[index].m_jobID, index, AZStd::string::format("SomeProduct%d.dds", index).c_str(), validAssetType));
                EXPECT_TRUE(m_connection.SetProduct(products[index]));
            }

            //products[0] -> products[1]
            ProductDependencyDatabaseEntry productDependency = ProductDependencyDatabaseEntry(products[0].m_productID, sources[1].m_sourceGuid, 1, 0, platform, true);
            EXPECT_TRUE(m_connection.SetProductDependency(productDependency));

            //products[1] -> products[2]
            productDependency = ProductDependencyDatabaseEntry(products[1].m_productID, sources[2].m_sourceGuid, 2, 0, platform, true);
            EXPECT_TRUE(m_connection.SetProductDependency(productDependency));

            //products[1] -> products[3]
            productDependency = ProductDependencyDatabaseEntry(products[1].m_productID, sources[3].m_sourceGuid, 3, 0, platform, true);
            EXPECT_TRUE(m_connection.SetProductDependency(productDependency));

            //products[2] -> products[4]
            productDependency = ProductDependencyDatabaseEntry(products[2].m_productID, sources[4].m_sourceGuid, 4, 0, platform, true);
            EXPECT_TRUE(m_connection.SetProductDependency(productDependency));

            //products[4] -> products[5]
            productDependency = ProductDependencyDatabaseEntry(products[4].m_productID, sources[5].m_sourceGuid, 5, 0, platform, true);
            EXPECT_TRUE(m_connection.SetProductDependency(productDependency));
        }

        void CreateProductDependencyTree(ProductDatabaseEntryContainer& products, const AZStd::string& platform = "")
        {
            SourceDatabaseEntryContainer sources;
            CreateProductDependencyTree(sources, products, platform);
        }

        void CreateProductDependencyTree(SourceDatabaseEntryContainer& sources, const AZStd::string& platform = "")
        {
            ProductDatabaseEntryContainer products;
            CreateProductDependencyTree(sources, products, platform);
        }

        AssetProcessor::AssetDatabaseConnection m_connection;

        ScanFolderDatabaseEntry m_defaultScanFolder;
        SourceDatabaseEntry m_defaultSource;
        JobDatabaseEntry m_defaultJob;       
        ProductDatabaseEntry m_defaultProduct;     
        LegacySubIDsEntry m_defaultLegacyEntry;
    };

    TEST_F(AssetProcessingStateDataUnitTest, TestScanFolder_AddScanFolder_Succeeds)
    {
        // There are no scan folders yet so trying to find one should fail
        ScanFolderDatabaseEntry scanFolder;
        ScanFolderDatabaseEntryContainer scanFolders;
        EXPECT_FALSE(m_connection.GetScanFolders(scanFolders));
        EXPECT_FALSE(m_connection.GetScanFolderByScanFolderID(0, scanFolder));
        EXPECT_FALSE(m_connection.GetScanFolderBySourceID(0, scanFolder));
        EXPECT_FALSE(m_connection.GetScanFolderByProductID(0, scanFolder));
        EXPECT_FALSE(m_connection.GetScanFolderByPortableKey("sadfsadfsadfsadfs", scanFolder));
        scanFolders.clear();

        AddDefaultScanFolder();

        // Get all scan folders, there should be one we just added
        scanFolders.clear();
        EXPECT_TRUE(m_connection.GetScanFolders(scanFolders));
        EXPECT_EQ(scanFolders.size(), 1);
        EXPECT_TRUE(Internal::ScanFoldersContainScanPath(scanFolders, "c:/O3DE/dev"));
        EXPECT_TRUE(Internal::ScanFoldersContainScanFolderID(scanFolders, m_defaultScanFolder.m_scanFolderID));
        EXPECT_TRUE(Internal::ScanFoldersContainPortableKey(scanFolders, m_defaultScanFolder.m_portableKey.c_str()));
        EXPECT_TRUE(Internal::ScanFoldersContainPortableKey(scanFolders, "rootportkey"));
    }

    TEST_F(AssetProcessingStateDataUnitTest, TestScanFolder_AddDuplicateScanFolder_GetsSameId)
    {
        AddDefaultScanFolder();

        // Add the same folder again, should not add another because it already exists, so we should get the same id
        // not only that, but the path should update.
        ScanFolderDatabaseEntry dupeScanFolder("c:/O3DE/dev1", "dev", "rootportkey");
        dupeScanFolder.m_scanFolderID = AzToolsFramework::AssetDatabase::InvalidEntryId;
        EXPECT_TRUE(m_connection.SetScanFolder(dupeScanFolder));
        EXPECT_EQ(dupeScanFolder, m_defaultScanFolder);

        EXPECT_EQ(dupeScanFolder.m_portableKey, m_defaultScanFolder.m_portableKey);
        EXPECT_EQ(dupeScanFolder.m_scanFolderID, m_defaultScanFolder.m_scanFolderID);

        // Get all scan folders, there should only the one we added
        ScanFolderDatabaseEntryContainer scanFolders;
        EXPECT_TRUE(m_connection.GetScanFolders(scanFolders));
        EXPECT_EQ(scanFolders.size(), 1);
        EXPECT_TRUE(Internal::ScanFoldersContainScanPath(scanFolders, "c:/O3DE/dev1"));
        EXPECT_TRUE(Internal::ScanFoldersContainScanFolderID(scanFolders, m_defaultScanFolder.m_scanFolderID));
        EXPECT_TRUE(Internal::ScanFoldersContainPortableKey(scanFolders, m_defaultScanFolder.m_portableKey.c_str()));
        EXPECT_TRUE(Internal::ScanFoldersContainPortableKey(scanFolders, "rootportkey"));
    }

    TEST_F(AssetProcessingStateDataUnitTest, TestScanFolder_RetrieveScanFolderById_Succeeds)
    {
        AddDefaultScanFolder();

        // Retrieve the one we just made by id
        ScanFolderDatabaseEntry retrieveScanfolderById;
        EXPECT_TRUE(m_connection.GetScanFolderByScanFolderID(m_defaultScanFolder.m_scanFolderID, retrieveScanfolderById));
        EXPECT_NE(retrieveScanfolderById.m_scanFolderID, AzToolsFramework::AssetDatabase::InvalidEntryId);
        EXPECT_EQ(retrieveScanfolderById.m_scanFolderID, m_defaultScanFolder.m_scanFolderID);
    }

    TEST_F(AssetProcessingStateDataUnitTest, TestScanFolder_RetrieveScanFolderByPortableKey_Succeeds)
    {
        AddDefaultScanFolder();

        // Retrieve the one we just made by portable key
        ScanFolderDatabaseEntry retrieveScanfolderByScanPath;
        EXPECT_TRUE(m_connection.GetScanFolderByPortableKey("rootportkey", retrieveScanfolderByScanPath));
        EXPECT_NE(retrieveScanfolderByScanPath.m_scanFolderID, AzToolsFramework::AssetDatabase::InvalidEntryId);
        EXPECT_EQ(retrieveScanfolderByScanPath.m_scanFolderID, m_defaultScanFolder.m_scanFolderID);
    }

    TEST_F(AssetProcessingStateDataUnitTest, TestScanFolder_RemoveScanFolderById_Succeeds)
    {
        AddDefaultScanFolder();
        // Add another scan folder
        ScanFolderDatabaseEntry gameScanFolderEntry("c:/O3DE/game", "game", "gameportkey");
        EXPECT_TRUE(m_connection.SetScanFolder(gameScanFolderEntry));
        EXPECT_NE(gameScanFolderEntry.m_scanFolderID, AzToolsFramework::AssetDatabase::InvalidEntryId);

        // Get all scan folders, there should be two scan folders we just added
        ScanFolderDatabaseEntryContainer scanFolders;
        EXPECT_TRUE(m_connection.GetScanFolders(scanFolders));
        EXPECT_EQ(scanFolders.size(), 2);
        EXPECT_TRUE(Internal::ScanFoldersContainScanPath(scanFolders, "c:/O3DE/dev"));
        EXPECT_TRUE(Internal::ScanFoldersContainScanPath(scanFolders, "c:/O3DE/game"));
        EXPECT_TRUE(Internal::ScanFoldersContainScanFolderID(scanFolders, m_defaultScanFolder.m_scanFolderID));
        EXPECT_TRUE(Internal::ScanFoldersContainScanFolderID(scanFolders, gameScanFolderEntry.m_scanFolderID));

        // Remove the game scan folder
        EXPECT_TRUE(m_connection.RemoveScanFolder(848475));//should return true even if it doesn't exist, false only means SQL failed
        EXPECT_TRUE(m_connection.RemoveScanFolder(gameScanFolderEntry.m_scanFolderID));

        // Get all scan folders again, there should now only the first we added
        scanFolders.clear();
        EXPECT_TRUE(m_connection.GetScanFolders(scanFolders));
        EXPECT_EQ(scanFolders.size(), 1);
        EXPECT_TRUE(Internal::ScanFoldersContainScanPath(scanFolders, "c:/O3DE/dev"));
        EXPECT_TRUE(Internal::ScanFoldersContainScanFolderID(scanFolders, m_defaultScanFolder.m_scanFolderID));
    }

    TEST_F(AssetProcessingStateDataUnitTest, TestScanFolder_RemoveScanFolderByContainer_Succeeds)
    {
        AddDefaultScanFolder();
        // Add another scan folder
        ScanFolderDatabaseEntry gameScanFolderEntry("c:/O3DE/game", "game", "gameportkey");
        EXPECT_TRUE(m_connection.SetScanFolder(gameScanFolderEntry));
        EXPECT_NE(gameScanFolderEntry.m_scanFolderID, AzToolsFramework::AssetDatabase::InvalidEntryId);

        // Get all scan folders, there should only the two we added
        ScanFolderDatabaseEntryContainer scanFolders;
        EXPECT_TRUE(m_connection.GetScanFolders(scanFolders));
        EXPECT_EQ(scanFolders.size(), 2);
        EXPECT_TRUE(Internal::ScanFoldersContainScanPath(scanFolders, "c:/O3DE/dev"));
        EXPECT_TRUE(Internal::ScanFoldersContainScanPath(scanFolders, "c:/O3DE/game"));
        EXPECT_TRUE(Internal::ScanFoldersContainScanFolderID(scanFolders, m_defaultScanFolder.m_scanFolderID));
        EXPECT_TRUE(Internal::ScanFoldersContainScanFolderID(scanFolders, gameScanFolderEntry.m_scanFolderID));

        // Remove scan folder by using a container
        ScanFolderDatabaseEntryContainer tempScanFolderDatabaseEntryContainer; // note that on clang, its illegal to call a non-const function with a temp variable container as the param
        EXPECT_TRUE(m_connection.RemoveScanFolders(tempScanFolderDatabaseEntryContainer)); // call with empty
        EXPECT_TRUE(m_connection.RemoveScanFolders(scanFolders));
        scanFolders.clear();
        EXPECT_FALSE(m_connection.GetScanFolders(scanFolders));
    }

    TEST_F(AssetProcessingStateDataUnitTest, TestSources_AddSource_Succeeds)
    {
        // There are no sources yet so trying to find one should fail
        SourceDatabaseEntry source;
        SourceDatabaseEntryContainer sources;
        EXPECT_FALSE(m_connection.GetSources(sources));
        EXPECT_FALSE(m_connection.GetSourceBySourceID(3443, source));
        EXPECT_FALSE(m_connection.GetSourceBySourceGuid(AZ::Uuid::Create(), source));
        EXPECT_FALSE(m_connection.GetSourcesLikeSourceName("source", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::Raw, sources));
        EXPECT_FALSE(m_connection.GetSourcesLikeSourceName("source", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::StartsWith, sources));
        EXPECT_FALSE(m_connection.GetSourcesLikeSourceName("source", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::EndsWith, sources));
        EXPECT_FALSE(m_connection.GetSourcesLikeSourceName("source", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::Matches, sources));

        // Trying to add a source without a valid scan folder pk should fail
        AZ::Uuid validSourceGuid1 = AZ::Uuid::CreateRandom();
        source = SourceDatabaseEntry(234234, "SomeSource.tif", validSourceGuid1, "");
        {
            UnitTestUtils::AssertAbsorber absorber;
            EXPECT_FALSE(m_connection.SetSource(source));
            EXPECT_GE(absorber.m_numWarningsAbsorbed, 0);
        }

        AddDefaultSource();

        // Get all sources, there should only the one we added
        sources.clear();
        EXPECT_TRUE(m_connection.GetSources(sources));
        EXPECT_EQ(sources.size(), 1);
        EXPECT_EQ(sources[0].m_analysisFingerprint, "12345");
        EXPECT_TRUE(Internal::SourcesContainSourceName(sources, "SomeSource.tif"));
        EXPECT_TRUE(Internal::SourcesContainSourceID(sources, m_defaultSource.m_sourceID));
        EXPECT_TRUE(Internal::SourcesContainSourceGuid(sources, m_defaultSource.m_sourceGuid));
    }

    TEST_F(AssetProcessingStateDataUnitTest, TestSources_AddDuplicateSource_GetsSameId)
    {
        AddDefaultSource();

        // Add the same source again, should not add another because it already exists, so we should get the same id
        SourceDatabaseEntry dupeSource(m_defaultSource);
        dupeSource.m_sourceID = AzToolsFramework::AssetDatabase::InvalidEntryId;
        EXPECT_TRUE(m_connection.SetSource(dupeSource));
        EXPECT_EQ(dupeSource.m_sourceID, m_defaultSource.m_sourceID);

        // Get all sources, there should still only the one we added
        SourceDatabaseEntryContainer sources;
        EXPECT_TRUE(m_connection.GetSources(sources));
        EXPECT_EQ(sources.size(), 1);
        EXPECT_EQ(sources[0].m_analysisFingerprint, "12345");
        EXPECT_TRUE(Internal::SourcesContainSourceName(sources, "SomeSource.tif"));
        EXPECT_TRUE(Internal::SourcesContainSourceID(sources, m_defaultSource.m_sourceID));
        EXPECT_TRUE(Internal::SourcesContainSourceGuid(sources, m_defaultSource.m_sourceGuid));
    }

    TEST_F(AssetProcessingStateDataUnitTest, TestSources_ChangeSourceField_Succeeds)
    {
        AddDefaultSource();

        // Make sure that changing a field like fingerprint writes the new field to database but does not
        // add a new entry (ie, its just modifying existing data)
        SourceDatabaseEntry sourceWithDifferentFingerprint(m_defaultSource);
        sourceWithDifferentFingerprint.m_analysisFingerprint = "otherFingerprint";
        EXPECT_TRUE(m_connection.SetSource(sourceWithDifferentFingerprint));
        EXPECT_EQ(sourceWithDifferentFingerprint.m_sourceID, m_defaultSource.m_sourceID);
        SourceDatabaseEntryContainer sources;
        EXPECT_TRUE(m_connection.GetSources(sources));
        EXPECT_EQ(sources.size(), 1);
        EXPECT_EQ(sources[0].m_analysisFingerprint, "otherFingerprint");
        EXPECT_TRUE(Internal::SourcesContainSourceName(sources, "SomeSource.tif"));
        EXPECT_TRUE(Internal::SourcesContainSourceID(sources, m_defaultSource.m_sourceID));
        EXPECT_TRUE(Internal::SourcesContainSourceGuid(sources, m_defaultSource.m_sourceGuid));
    }

    TEST_F(AssetProcessingStateDataUnitTest, TestSources_ChangeScanFolder_Succeeds)
    {
        AddDefaultSource();

        // Add the same source again, but change the scan folder.  This should NOT add a new source - even if we don't know what the sourceID is:
        ScanFolderDatabaseEntry scanfolder = ScanFolderDatabaseEntry("c:/O3DE/dev1", "dev1", "devkey");
        EXPECT_TRUE(m_connection.SetScanFolder(scanfolder));

        SourceDatabaseEntry source(m_defaultSource);
        source.m_scanFolderPK = scanfolder.m_scanFolderID;
        source.m_analysisFingerprint = "new different fingerprint";
        source.m_sourceID = AzToolsFramework::AssetDatabase::InvalidEntryId;
        EXPECT_TRUE(m_connection.SetSource(source));
        EXPECT_EQ(source.m_sourceID, m_defaultSource.m_sourceID);

        // Get all sources, there should still only the one we added
        SourceDatabaseEntryContainer sources;
        EXPECT_TRUE(m_connection.GetSources(sources));
        EXPECT_EQ(sources.size(), 1);
        EXPECT_EQ(sources[0].m_analysisFingerprint, "new different fingerprint"); // verify that this column IS updated.
        EXPECT_TRUE(Internal::SourcesContainSourceName(sources, "SomeSource.tif"));
        EXPECT_TRUE(Internal::SourcesContainSourceID(sources, m_defaultSource.m_sourceID));
        EXPECT_TRUE(Internal::SourcesContainSourceGuid(sources, m_defaultSource.m_sourceGuid));

        // Add the same source again, but change the scan folder back. This should NOT add a new source - this time we do know what the source ID is!
        SourceDatabaseEntry dupeSource(m_defaultSource);
        dupeSource.m_scanFolderPK = m_defaultScanFolder.m_scanFolderID; // changing it back here.
        EXPECT_TRUE(m_connection.SetSource(dupeSource));
        EXPECT_EQ(dupeSource.m_sourceID, m_defaultSource.m_sourceID);

        // Get all sources, there should still only the one we added
        sources.clear();
        EXPECT_TRUE(m_connection.GetSources(sources));
        EXPECT_EQ(sources.size(), 1);
        EXPECT_TRUE(Internal::SourcesContainSourceName(sources, "SomeSource.tif"));
        EXPECT_TRUE(Internal::SourcesContainSourceID(sources, m_defaultSource.m_sourceID));
        EXPECT_TRUE(Internal::SourcesContainSourceGuid(sources, m_defaultSource.m_sourceGuid));

        // Remove the extra scan folder, make sure it doesn't drop the source since it should now be bound to the original scan folder agian
        EXPECT_TRUE(m_connection.RemoveScanFolder(scanfolder.m_scanFolderID));
        sources.clear();
        EXPECT_TRUE(m_connection.GetSources(sources));
        EXPECT_EQ(sources.size(), 1);
        EXPECT_TRUE(Internal::SourcesContainSourceName(sources, "SomeSource.tif"));
        EXPECT_TRUE(Internal::SourcesContainSourceID(sources, m_defaultSource.m_sourceID));
        EXPECT_TRUE(Internal::SourcesContainSourceGuid(sources, m_defaultSource.m_sourceGuid));
    }

    TEST_F(AssetProcessingStateDataUnitTest, TestSources_RetrieveSourceBySourceId_Succeeds)
    {
        AddDefaultSource();

        SourceDatabaseEntry retrieveSourceBySourceID;
        EXPECT_TRUE(m_connection.GetSourceBySourceID(m_defaultSource.m_sourceID, retrieveSourceBySourceID));
        EXPECT_NE(retrieveSourceBySourceID.m_sourceID, AzToolsFramework::AssetDatabase::InvalidEntryId);
        EXPECT_EQ(retrieveSourceBySourceID.m_sourceID, m_defaultSource.m_sourceID);
        EXPECT_EQ(retrieveSourceBySourceID.m_scanFolderPK, m_defaultSource.m_scanFolderPK);
        EXPECT_EQ(retrieveSourceBySourceID.m_sourceGuid, m_defaultSource.m_sourceGuid);
        EXPECT_EQ(retrieveSourceBySourceID.m_sourceName, m_defaultSource.m_sourceName);
    }

    TEST_F(AssetProcessingStateDataUnitTest, TestSources_RetrieveSourceBySourceGuid_Succeeds)
    {
        AddDefaultSource();

        // Try retrieving this source by guid
        SourceDatabaseEntry retrieveSourceBySourceGuid;
        EXPECT_TRUE(m_connection.GetSourceBySourceGuid(m_defaultSource.m_sourceGuid, retrieveSourceBySourceGuid));
        EXPECT_NE(retrieveSourceBySourceGuid.m_sourceID, AzToolsFramework::AssetDatabase::InvalidEntryId);
        EXPECT_EQ(retrieveSourceBySourceGuid.m_sourceID, m_defaultSource.m_sourceID);
        EXPECT_EQ(retrieveSourceBySourceGuid.m_scanFolderPK, m_defaultSource.m_scanFolderPK);
        EXPECT_EQ(retrieveSourceBySourceGuid.m_sourceGuid, m_defaultSource.m_sourceGuid);
        EXPECT_EQ(retrieveSourceBySourceGuid.m_sourceName, m_defaultSource.m_sourceName);
    }

    TEST_F(AssetProcessingStateDataUnitTest, TestSources_RetrieveSourceBySourceName_Succeeds)
    {
        AddDefaultSource();

        // Try retrieving this source by source name
        SourceDatabaseEntryContainer sources;
        EXPECT_FALSE(m_connection.GetSourcesLikeSourceName("Source.tif", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::Raw, sources));
        sources.clear();
        EXPECT_FALSE(m_connection.GetSourcesLikeSourceName("_SomeSource_", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::Raw, sources));
        sources.clear();
        EXPECT_TRUE(m_connection.GetSourcesLikeSourceName("SomeSource%", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::Raw, sources));
        EXPECT_EQ(sources.size(), 1);
        EXPECT_TRUE(Internal::SourcesContainSourceName(sources, "SomeSource.tif"));
        EXPECT_TRUE(Internal::SourcesContainSourceID(sources, m_defaultSource.m_sourceID));
        EXPECT_TRUE(Internal::SourcesContainSourceGuid(sources, m_defaultSource.m_sourceGuid));
        sources.clear();
        EXPECT_TRUE(m_connection.GetSourcesLikeSourceName("%SomeSource%", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::Raw, sources));
        EXPECT_EQ(sources.size(), 1);
        EXPECT_TRUE(Internal::SourcesContainSourceName(sources, "SomeSource.tif"));
        EXPECT_TRUE(Internal::SourcesContainSourceID(sources, m_defaultSource.m_sourceID));
        EXPECT_TRUE(Internal::SourcesContainSourceGuid(sources, m_defaultSource.m_sourceGuid));
        sources.clear();
        EXPECT_FALSE(m_connection.GetSourcesLikeSourceName("Source", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::StartsWith, sources));
        sources.clear();
        EXPECT_TRUE(m_connection.GetSourcesLikeSourceName("Some", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::StartsWith, sources));
        EXPECT_EQ(sources.size(), 1);
        EXPECT_TRUE(Internal::SourcesContainSourceName(sources, "SomeSource.tif"));
        EXPECT_TRUE(Internal::SourcesContainSourceID(sources, m_defaultSource.m_sourceID));
        EXPECT_TRUE(Internal::SourcesContainSourceGuid(sources, m_defaultSource.m_sourceGuid));
        sources.clear();
        EXPECT_FALSE(m_connection.GetSourcesLikeSourceName("SomeSource", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::EndsWith, sources));
        sources.clear();
        EXPECT_TRUE(m_connection.GetSourcesLikeSourceName(".tif", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::EndsWith, sources));
        EXPECT_EQ(sources.size(), 1);
        EXPECT_TRUE(Internal::SourcesContainSourceName(sources, "SomeSource.tif"));
        EXPECT_TRUE(Internal::SourcesContainSourceID(sources, m_defaultSource.m_sourceID));
        EXPECT_TRUE(Internal::SourcesContainSourceGuid(sources, m_defaultSource.m_sourceGuid));
        sources.clear();
        EXPECT_FALSE(m_connection.GetSourcesLikeSourceName("blah", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::Matches, sources));
        sources.clear();
        EXPECT_TRUE(m_connection.GetSourcesLikeSourceName("meSour", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::Matches, sources));
        EXPECT_EQ(sources.size(), 1);
        EXPECT_TRUE(Internal::SourcesContainSourceName(sources, "SomeSource.tif"));
        EXPECT_TRUE(Internal::SourcesContainSourceID(sources, m_defaultSource.m_sourceID));
        EXPECT_TRUE(Internal::SourcesContainSourceGuid(sources, m_defaultSource.m_sourceGuid));
    }

    TEST_F(AssetProcessingStateDataUnitTest, TestSources_RemoveSourceById_Succeeds)
    {
        AddDefaultSource();

        // Get all sources, there should still only the one we added
        SourceDatabaseEntryContainer sources;
        EXPECT_TRUE(m_connection.GetSources(sources));
        EXPECT_EQ(sources.size(), 1);
        EXPECT_EQ(sources[0].m_analysisFingerprint, "12345");
        EXPECT_TRUE(Internal::SourcesContainSourceName(sources, "SomeSource.tif"));
        EXPECT_TRUE(Internal::SourcesContainSourceID(sources, m_defaultSource.m_sourceID));
        EXPECT_TRUE(Internal::SourcesContainSourceGuid(sources, m_defaultSource.m_sourceGuid));

        // Remove a source
        EXPECT_TRUE(m_connection.RemoveSource(432234));//should return true even if it doesn't exist, false only if SQL failed
        EXPECT_TRUE(m_connection.RemoveSource(m_defaultSource.m_sourceID));

        // Get all sources, there shouldn't be any
        sources.clear();
        EXPECT_FALSE(m_connection.GetSources(sources));
    }

    TEST_F(AssetProcessingStateDataUnitTest, TestSources_RemoveSourceByContainer_Succeeds)
    {
        AddDefaultSource();
        // Add another source
        AZ::Uuid validSourceGuid = AZ::Uuid::CreateRandom();
        SourceDatabaseEntry source(m_defaultScanFolder.m_scanFolderID, "SomeSource1.tif", validSourceGuid, "");
        EXPECT_TRUE(m_connection.SetSource(source));

        // Get all sources, there should only the two we added
        SourceDatabaseEntryContainer sources;
        EXPECT_TRUE(m_connection.GetSources(sources));
        EXPECT_EQ(sources.size(), 2);
        EXPECT_TRUE(Internal::SourcesContainSourceName(sources, "SomeSource.tif"));
        EXPECT_TRUE(Internal::SourcesContainSourceName(sources, "SomeSource1.tif"));
        EXPECT_TRUE(Internal::SourcesContainSourceID(sources, m_defaultSource.m_sourceID));
        EXPECT_TRUE(Internal::SourcesContainSourceGuid(sources, m_defaultSource.m_sourceGuid));
        EXPECT_TRUE(Internal::SourcesContainSourceID(sources, source.m_sourceID));
        EXPECT_TRUE(Internal::SourcesContainSourceGuid(sources, source.m_sourceGuid));

        // Remove source via container
        SourceDatabaseEntryContainer tempSourceDatabaseEntryContainer;
        EXPECT_TRUE(m_connection.RemoveSources(tempSourceDatabaseEntryContainer)); // try it with an empty one.
        EXPECT_TRUE(m_connection.RemoveSources(sources));

        // Get all sources, there should none
        sources.clear();
        EXPECT_FALSE(m_connection.GetSources(sources));
    }

    TEST_F(AssetProcessingStateDataUnitTest, TestSources_RemoveSourceByScanFolderId_Succeeds)
    {
        AddDefaultSource();
        // Add another source
        AZ::Uuid validSourceGuid = AZ::Uuid::CreateRandom();
        SourceDatabaseEntry source(m_defaultScanFolder.m_scanFolderID, "SomeSource1.tif", validSourceGuid, "");
        EXPECT_TRUE(m_connection.SetSource(source));

        // Get all sources, there should only the two we added
        SourceDatabaseEntryContainer sources;
        EXPECT_TRUE(m_connection.GetSources(sources));
        EXPECT_EQ(sources.size(), 2);
        EXPECT_TRUE(Internal::SourcesContainSourceName(sources, "SomeSource.tif"));
        EXPECT_TRUE(Internal::SourcesContainSourceName(sources, "SomeSource1.tif"));
        EXPECT_TRUE(Internal::SourcesContainSourceID(sources, m_defaultSource.m_sourceID));
        EXPECT_TRUE(Internal::SourcesContainSourceGuid(sources, m_defaultSource.m_sourceGuid));
        EXPECT_TRUE(Internal::SourcesContainSourceID(sources, source.m_sourceID));
        EXPECT_TRUE(Internal::SourcesContainSourceGuid(sources, source.m_sourceGuid));

        // Remove the scan folder for these sources, the sources should cascade delete
        EXPECT_TRUE(m_connection.RemoveScanFolder(m_defaultScanFolder.m_scanFolderID));

        // Get all sources, there should none
        sources.clear();
        EXPECT_FALSE(m_connection.GetSources(sources));
    }

    TEST_F(AssetProcessingStateDataUnitTest, TestJobs_AddJob_Succeeds)
    {
        //there are no jobs yet so trying to find one should fail.
        JobDatabaseEntryContainer jobs;
        JobDatabaseEntry job;
        EXPECT_FALSE(m_connection.GetJobs(jobs));
        EXPECT_FALSE(m_connection.GetJobByJobID(3443, job));
        EXPECT_FALSE(m_connection.GetJobsBySourceID(3234, jobs));
        EXPECT_FALSE(m_connection.GetJobsBySourceName(AssetProcessor::SourceAssetReference("c:/O3DE/dev/none"), jobs));

        AddDefaultSource();

        // Trying to add a job without a valid source pk should fail:
        AZ::u32 validFingerprint1 = 1;
        AZ::Uuid validBuilderGuid1 = AZ::Uuid::CreateRandom();
        AzToolsFramework::AssetSystem::JobStatus statusQueued = AzToolsFramework::AssetSystem::JobStatus::Queued;
        {
            UnitTestUtils::AssertAbsorber absorber;
            job = JobDatabaseEntry(234234, "jobkey", validFingerprint1, "pc", validBuilderGuid1, statusQueued, 1);
            EXPECT_FALSE(m_connection.SetJob(job));
            EXPECT_GE(absorber.m_numWarningsAbsorbed, 0);
        }

        // Trying to add a job with a valid source pk but an invalid job id should fail:
        {
            UnitTestUtils::AssertAbsorber absorber;
            job = JobDatabaseEntry(m_defaultSource.m_sourceID, "jobkey", validFingerprint1, "pc", validBuilderGuid1, statusQueued, 0);
            EXPECT_FALSE(m_connection.SetJob(job));
            EXPECT_GE(absorber.m_numErrorsAbsorbed, 0);
        }

        AddDefaultJob();

        // Get all jobs, there should only the one we added
        jobs.clear();
        EXPECT_TRUE(m_connection.GetJobs(jobs));
        EXPECT_EQ(jobs.size(), 1);
        EXPECT_TRUE(Internal::JobsContainJobID(jobs, m_defaultJob.m_jobID));
        EXPECT_TRUE(Internal::JobsContainJobKey(jobs, m_defaultJob.m_jobKey.c_str()));
        EXPECT_TRUE(Internal::JobsContainFingerprint(jobs, m_defaultJob.m_fingerprint));
        EXPECT_TRUE(Internal::JobsContainPlatform(jobs, m_defaultJob.m_platform.c_str()));
        EXPECT_TRUE(Internal::JobsContainBuilderGuid(jobs, m_defaultJob.m_builderGuid));
        EXPECT_TRUE(Internal::JobsContainStatus(jobs, m_defaultJob.m_status));
        EXPECT_TRUE(Internal::JobsContainRunKey(jobs, m_defaultJob.m_jobRunKey));
    }

    TEST_F(AssetProcessingStateDataUnitTest, TestJobs_AddDuplicateJob_GetsSameId)
    {
        AddDefaultJob();

        // Add the same job again, should not add another because it already exists, so we should get the same id
        JobDatabaseEntry dupeJob(m_defaultJob);
        dupeJob.m_jobID = AzToolsFramework::AssetDatabase::InvalidEntryId;
        EXPECT_TRUE(m_connection.SetJob(dupeJob));
        EXPECT_EQ(dupeJob, m_defaultJob);

        // Get all jobs, there should still only the one we added
        JobDatabaseEntryContainer jobs;
        EXPECT_TRUE(m_connection.GetJobs(jobs));
        EXPECT_EQ(jobs.size(), 1);
        EXPECT_TRUE(Internal::JobsContainJobID(jobs, m_defaultJob.m_jobID));
        EXPECT_TRUE(Internal::JobsContainJobKey(jobs, m_defaultJob.m_jobKey.c_str()));
        EXPECT_TRUE(Internal::JobsContainFingerprint(jobs, m_defaultJob.m_fingerprint));
        EXPECT_TRUE(Internal::JobsContainPlatform(jobs, m_defaultJob.m_platform.c_str()));
        EXPECT_TRUE(Internal::JobsContainBuilderGuid(jobs, m_defaultJob.m_builderGuid));
        EXPECT_TRUE(Internal::JobsContainStatus(jobs, m_defaultJob.m_status));
    }

    TEST_F(AssetProcessingStateDataUnitTest, TestJobs_RetrieveJobByJobId_Succeeds)
    {
        AddDefaultJob();

        // Try retrieving this source by id
        EXPECT_TRUE(m_connection.GetJobByJobID(m_defaultJob.m_jobID, m_defaultJob));
        EXPECT_NE(m_defaultJob.m_jobID, AzToolsFramework::AssetDatabase::InvalidEntryId);

        // Try retrieving jobs by source id
        JobDatabaseEntryContainer jobs;
        EXPECT_TRUE(m_connection.GetJobsBySourceID(m_defaultSource.m_sourceID, jobs));
        EXPECT_EQ(jobs.size(), 1);
        EXPECT_TRUE(Internal::JobsContainJobID(jobs, m_defaultJob.m_jobID));
        EXPECT_TRUE(Internal::JobsContainJobKey(jobs, m_defaultJob.m_jobKey.c_str()));
        EXPECT_TRUE(Internal::JobsContainFingerprint(jobs, m_defaultJob.m_fingerprint));
        EXPECT_TRUE(Internal::JobsContainPlatform(jobs, m_defaultJob.m_platform.c_str()));
        EXPECT_TRUE(Internal::JobsContainBuilderGuid(jobs, m_defaultJob.m_builderGuid));
        EXPECT_TRUE(Internal::JobsContainStatus(jobs, m_defaultJob.m_status));
    }

    TEST_F(AssetProcessingStateDataUnitTest, TestJobs_RetrieveJobBySourceName_Succeeds)
    {
        AddDefaultJob();

        auto& config = m_appManager->m_platformConfig;
        config->AddScanFolder(AssetProcessor::ScanFolderInfo{m_defaultScanFolder.m_scanFolder.c_str(),
            m_defaultScanFolder.m_displayName.c_str(), m_defaultScanFolder.m_portableKey.c_str(),
            false, true, {} , 0, m_defaultScanFolder.m_scanFolderID});

        // Try retrieving jobs by source name
        JobDatabaseEntryContainer jobs;
        EXPECT_TRUE(m_connection.GetJobsBySourceName(AssetProcessor::SourceAssetReference(
            m_defaultSource.m_scanFolderPK, m_defaultSource.m_sourceName.c_str()), jobs));
        EXPECT_EQ(jobs.size(), 1);
        EXPECT_TRUE(Internal::JobsContainJobID(jobs, m_defaultJob.m_jobID));
        EXPECT_TRUE(Internal::JobsContainJobKey(jobs, m_defaultJob.m_jobKey.c_str()));
        EXPECT_TRUE(Internal::JobsContainFingerprint(jobs, m_defaultJob.m_fingerprint));
        EXPECT_TRUE(Internal::JobsContainPlatform(jobs, m_defaultJob.m_platform.c_str()));
        EXPECT_TRUE(Internal::JobsContainBuilderGuid(jobs, m_defaultJob.m_builderGuid));
        EXPECT_TRUE(Internal::JobsContainStatus(jobs, m_defaultJob.m_status));
    }

    TEST_F(AssetProcessingStateDataUnitTest, TestJobs_RemoveJobByJobId_Succeeds)
    {
        AddDefaultJob();

        // Remove a job
        EXPECT_TRUE(m_connection.RemoveJob(432234));
        EXPECT_TRUE(m_connection.RemoveJob(m_defaultJob.m_jobID));

        // Get all jobs, there shouldn't be any
        JobDatabaseEntryContainer jobs;
        EXPECT_FALSE(m_connection.GetJobs(jobs));
    }

    TEST_F(AssetProcessingStateDataUnitTest, TestJobs_RemoveJobByContainer_Succeeds)
    {
        AddDefaultJob();
        // Add another source and job
        AZ::Uuid validSourceGuid = AZ::Uuid::CreateRandom();
        SourceDatabaseEntry source = SourceDatabaseEntry(m_defaultScanFolder.m_scanFolderID, "SomeSource1.tif", validSourceGuid, "");
        EXPECT_TRUE(m_connection.SetSource(source));

        AZ::u32 validFingerprint = 1;
        AZ::u32 validJobRunKey = 1;
        AZ::Uuid validBuilderGuid = AZ::Uuid::CreateRandom();
        AzToolsFramework::AssetSystem::JobStatus statusQueued = AzToolsFramework::AssetSystem::JobStatus::Queued;
        JobDatabaseEntry job(source.m_sourceID, "jobkey1", validFingerprint, "pc", validBuilderGuid, statusQueued, validJobRunKey);
        EXPECT_TRUE(m_connection.SetJob(job));

        // Get all jobs, there should be 2
        JobDatabaseEntryContainer jobs;
        EXPECT_TRUE(m_connection.GetJobs(jobs));
        EXPECT_EQ(jobs.size(), 2);
        EXPECT_TRUE(Internal::JobsContainJobID(jobs, m_defaultJob.m_jobID));
        EXPECT_TRUE(Internal::JobsContainJobKey(jobs, m_defaultJob.m_jobKey.c_str()));
        EXPECT_TRUE(Internal::JobsContainFingerprint(jobs, m_defaultJob.m_fingerprint));
        EXPECT_TRUE(Internal::JobsContainPlatform(jobs, m_defaultJob.m_platform.c_str()));
        EXPECT_TRUE(Internal::JobsContainBuilderGuid(jobs, m_defaultJob.m_builderGuid));
        EXPECT_TRUE(Internal::JobsContainStatus(jobs, m_defaultJob.m_status));
        EXPECT_TRUE(Internal::JobsContainJobID(jobs, job.m_jobID));
        EXPECT_TRUE(Internal::JobsContainJobKey(jobs, job.m_jobKey.c_str()));
        EXPECT_TRUE(Internal::JobsContainFingerprint(jobs, job.m_fingerprint));
        EXPECT_TRUE(Internal::JobsContainPlatform(jobs, job.m_platform.c_str()));
        EXPECT_TRUE(Internal::JobsContainBuilderGuid(jobs, job.m_builderGuid));
        EXPECT_TRUE(Internal::JobsContainStatus(jobs, job.m_status));

        //Remove job via container
        JobDatabaseEntryContainer tempJobDatabaseEntryContainer;
        EXPECT_TRUE(m_connection.RemoveJobs(tempJobDatabaseEntryContainer)); // make sure it works on an empty container.
        EXPECT_TRUE(m_connection.RemoveJobs(jobs));

        //get all jobs, there should none
        jobs.clear();
        EXPECT_FALSE(m_connection.GetJobs(jobs));
    }

    TEST_F(AssetProcessingStateDataUnitTest, TestJobs_RemoveJobBySource_Succeeds)
    {
        AddDefaultJob();
        // Add another job for the same source
        AZ::u32 validFingerprint = 1;
        AZ::u32 validJobRunKey = 1;
        AZ::Uuid validBuilderGuid = AZ::Uuid::CreateRandom();
        AzToolsFramework::AssetSystem::JobStatus statusQueued = AzToolsFramework::AssetSystem::JobStatus::Queued;
        JobDatabaseEntry job(m_defaultSource.m_sourceID, "jobkey1", validFingerprint, "pc", validBuilderGuid, statusQueued, validJobRunKey);
        EXPECT_TRUE(m_connection.SetJob(job));

        // Get all jobs, there should be 2
        JobDatabaseEntryContainer jobs;
        EXPECT_TRUE(m_connection.GetJobs(jobs));
        EXPECT_EQ(jobs.size(), 2);
        EXPECT_TRUE(Internal::JobsContainJobID(jobs, m_defaultJob.m_jobID));
        EXPECT_TRUE(Internal::JobsContainJobKey(jobs, m_defaultJob.m_jobKey.c_str()));
        EXPECT_TRUE(Internal::JobsContainFingerprint(jobs, m_defaultJob.m_fingerprint));
        EXPECT_TRUE(Internal::JobsContainPlatform(jobs, m_defaultJob.m_platform.c_str()));
        EXPECT_TRUE(Internal::JobsContainBuilderGuid(jobs, m_defaultJob.m_builderGuid));
        EXPECT_TRUE(Internal::JobsContainStatus(jobs, m_defaultJob.m_status));
        EXPECT_TRUE(Internal::JobsContainJobID(jobs, job.m_jobID));
        EXPECT_TRUE(Internal::JobsContainJobKey(jobs, job.m_jobKey.c_str()));
        EXPECT_TRUE(Internal::JobsContainFingerprint(jobs, job.m_fingerprint));
        EXPECT_TRUE(Internal::JobsContainPlatform(jobs, job.m_platform.c_str()));
        EXPECT_TRUE(Internal::JobsContainBuilderGuid(jobs, job.m_builderGuid));
        EXPECT_TRUE(Internal::JobsContainStatus(jobs, job.m_status));

        // Remove the source for these jobs, the jobs should cascade delete
        EXPECT_TRUE(m_connection.RemoveSource(m_defaultSource.m_sourceID));

        // Get all jobs, there should none
        jobs.clear();
        EXPECT_FALSE(m_connection.GetJobs(jobs));
    }

    TEST_F(AssetProcessingStateDataUnitTest, TestProducts_AddProduct_Succeeds)
    {
        // There are no scan folders yet so trying to find one should fail
        ProductDatabaseEntry product;
        ProductDatabaseEntryContainer products;
        EXPECT_FALSE(m_connection.GetProducts(products));
        EXPECT_EQ(products.size(), 0);

        AddDefaultProduct();

        // Get all products, there should be one we just added
        products.clear();
        EXPECT_TRUE(m_connection.GetProducts(products));
        EXPECT_EQ(products.size(), 1);
    }

    TEST_F(AssetProcessingStateDataUnitTest, TestProducts_RemoveProduct_Succeeds)
    {
        AddDefaultProduct();

        // Add another product
        AZ::Data::AssetType validAssetType = AZ::Data::AssetType::CreateRandom();
        ProductDatabaseEntry product = ProductDatabaseEntry(m_defaultJob.m_jobID, 1, "SomeProduct1.dds", validAssetType);
        EXPECT_TRUE(m_connection.SetProduct(product));

        // The products should cascade delete
        EXPECT_TRUE(m_connection.RemoveSource(m_defaultSource.m_sourceID));

        // Get all products, there should none
        ProductDatabaseEntryContainer products;
        EXPECT_FALSE(m_connection.GetProducts(products));
    }

    TEST_F(AssetProcessingStateDataUnitTest, TestProducts_AddLegacySubIds_Succeeds)
    {
        AddDefaultProduct();

        // Test invalid insert for non existant legacy subids.
        LegacySubIDsEntry legacyEntry(1, m_defaultProduct.m_productID, 0);
        {
            UnitTestUtils::AssertAbsorber absorber;
            EXPECT_FALSE(m_connection.CreateOrUpdateLegacySubID(legacyEntry));
            EXPECT_GT(absorber.m_numWarningsAbsorbed, 0);
        }

        // Test invalid insert for non-existant legacy product FK constraint
        legacyEntry = LegacySubIDsEntry(AzToolsFramework::AssetDatabase::InvalidEntryId, 9999, 0);
        {
            UnitTestUtils::AssertAbsorber absorber;
            EXPECT_FALSE(m_connection.CreateOrUpdateLegacySubID(legacyEntry));
            EXPECT_GT(absorber.m_numWarningsAbsorbed, 0);
        }

        // Test valid insert for a product
        AddDefaultLegacySubId();
        EXPECT_NE(m_defaultLegacyEntry.m_subIDsEntryID, AzToolsFramework::AssetDatabase::InvalidEntryId); // it should have also updated the PK
    }

    TEST_F(AssetProcessingStateDataUnitTest, TestLegacySubIds_RetrieveLegacySubIds_Succeeds)
    {
        AddDefaultLegacySubId();

        // Insert of another for same product
        LegacySubIDsEntry legacyEntry(AzToolsFramework::AssetDatabase::InvalidEntryId, m_defaultProduct.m_productID, 1);
        EXPECT_TRUE(m_connection.CreateOrUpdateLegacySubID(legacyEntry));
        EXPECT_NE(legacyEntry.m_subIDsEntryID, AzToolsFramework::AssetDatabase::InvalidEntryId); // it should have also updated the PK
        EXPECT_NE(legacyEntry.m_subIDsEntryID, m_defaultLegacyEntry.m_subIDsEntryID); // pk should be unique

        // Insert of another for different product
        ProductDatabaseEntry product(m_defaultJob.m_jobID, 1, "SomeProduct1.dds", m_defaultProduct.m_assetType);
        EXPECT_TRUE(m_connection.SetProduct(product));
        legacyEntry = LegacySubIDsEntry(AzToolsFramework::AssetDatabase::InvalidEntryId, product.m_productID, 2);
        EXPECT_TRUE(m_connection.CreateOrUpdateLegacySubID(legacyEntry));

        // Test that the ones inserted can be retrieved.
        AZStd::vector<LegacySubIDsEntry> entriesReturned;
        auto handler = [&entriesReturned](LegacySubIDsEntry& entry)
        {
            entriesReturned.push_back(entry);
            return true;
        };
        EXPECT_TRUE(m_connection.QueryLegacySubIdsByProductID(m_defaultProduct.m_productID, handler));
        EXPECT_EQ(entriesReturned.size(), 2);

        bool foundSubID3 = false;
        bool foundSubID4 = false;
        for (const LegacySubIDsEntry& entryFound : entriesReturned)
        {
            EXPECT_NE(entryFound.m_subIDsEntryID, AzToolsFramework::AssetDatabase::InvalidEntryId);
            EXPECT_EQ(entryFound.m_productPK, m_defaultProduct.m_productID);
            if (entryFound.m_subID == 0)
            {
                foundSubID3 = true;
            }
            else if (entryFound.m_subID == 1)
            {
                foundSubID4 = true;
            }
        }

        EXPECT_TRUE(foundSubID3);
        EXPECT_TRUE(foundSubID4);

        entriesReturned.clear();

        EXPECT_TRUE(m_connection.QueryLegacySubIdsByProductID(product.m_productID, handler));
        EXPECT_EQ(entriesReturned.size(), 1);
        EXPECT_NE(entriesReturned[0].m_subIDsEntryID, AzToolsFramework::AssetDatabase::InvalidEntryId);
        EXPECT_EQ(entriesReturned[0].m_productPK, product.m_productID);
        EXPECT_EQ(entriesReturned[0].m_subID, 2);
    }

    TEST_F(AssetProcessingStateDataUnitTest, TestLegacySubIds_OverrideLegacySubId_Succeeds)
    {
        AddDefaultLegacySubId();

        // Retrieve the current legacy sub IDs entry
        AZStd::vector<LegacySubIDsEntry> entriesReturned;
        auto handler = [&entriesReturned](LegacySubIDsEntry& entry)
        {
            entriesReturned.push_back(entry);
            return true;
        };

        EXPECT_TRUE(m_connection.QueryLegacySubIdsByProductID(m_defaultProduct.m_productID, handler));
        EXPECT_EQ(entriesReturned.size(), 1);
        EXPECT_NE(entriesReturned[0].m_subIDsEntryID, AzToolsFramework::AssetDatabase::InvalidEntryId);
        EXPECT_EQ(entriesReturned[0].m_productPK, m_defaultProduct.m_productID);
        EXPECT_EQ(entriesReturned[0].m_subID, 0);

        // Test UPDATE -> overwrite existing.
        entriesReturned[0].m_subID = 1;
        EXPECT_TRUE(m_connection.CreateOrUpdateLegacySubID(entriesReturned[0]));

        entriesReturned.clear();
        EXPECT_TRUE(m_connection.QueryLegacySubIdsByProductID(m_defaultProduct.m_productID, handler));
        EXPECT_EQ(entriesReturned.size(), 1);
        EXPECT_NE(entriesReturned[0].m_subIDsEntryID, AzToolsFramework::AssetDatabase::InvalidEntryId);
        EXPECT_EQ(entriesReturned[0].m_productPK, m_defaultProduct.m_productID);
        EXPECT_EQ(entriesReturned[0].m_subID, 1);
    }

    TEST_F(AssetProcessingStateDataUnitTest, TestLegacySubIds_RemoveLegacySubIdByProductId_Succeeds)
    {
        AddDefaultLegacySubId();
        AZStd::vector<LegacySubIDsEntry> entriesReturned;
        auto handler = [&entriesReturned](LegacySubIDsEntry& entry)
        {
            entriesReturned.push_back(entry);
            return true;
        };

        EXPECT_TRUE(m_connection.RemoveLegacySubIDsByProductID(m_defaultProduct.m_productID));
        entriesReturned.clear();

        EXPECT_TRUE(m_connection.QueryLegacySubIdsByProductID(m_defaultProduct.m_productID, handler));
        EXPECT_TRUE(entriesReturned.empty());
    }

    TEST_F(AssetProcessingStateDataUnitTest, TestLegacySubIds_RemoveLegacySubIdByPK_Succeeds)
    {
        AddDefaultLegacySubId();
        // Add a second legacy sub ID
        LegacySubIDsEntry legacyEntry(AzToolsFramework::AssetDatabase::InvalidEntryId, m_defaultProduct.m_productID, 1);
        EXPECT_TRUE(m_connection.CreateOrUpdateLegacySubID(legacyEntry));
        EXPECT_NE(legacyEntry.m_subIDsEntryID, AzToolsFramework::AssetDatabase::InvalidEntryId); // it should have also updated the PK
        EXPECT_NE(legacyEntry.m_subIDsEntryID, m_defaultLegacyEntry.m_subIDsEntryID); // pk should be unique

        AZStd::vector<LegacySubIDsEntry> entriesReturned;
        auto handler = [&entriesReturned](LegacySubIDsEntry& entry)
        {
            entriesReturned.push_back(entry);
            return true;
        };

        EXPECT_TRUE(m_connection.QueryLegacySubIdsByProductID(m_defaultProduct.m_productID, handler));
        EXPECT_EQ(entriesReturned.size(), 2);

        // Test delete by PK.  The prior entries should be here for product1. This also makes sure the above
        // delete statement didn't delete more than it should have.
        AZ::s64 toRemove = entriesReturned[0].m_subIDsEntryID;
        AZ::u32 removingSubID = entriesReturned[0].m_subID;
        EXPECT_TRUE(m_connection.RemoveLegacySubID(toRemove));

        entriesReturned.clear();
        EXPECT_TRUE(m_connection.QueryLegacySubIdsByProductID(m_defaultProduct.m_productID, handler));
        EXPECT_EQ(entriesReturned.size(), 1);
        EXPECT_NE(entriesReturned[0].m_subIDsEntryID, AzToolsFramework::AssetDatabase::InvalidEntryId);
        EXPECT_NE(entriesReturned[0].m_subIDsEntryID, toRemove);
        EXPECT_EQ(entriesReturned[0].m_productPK, m_defaultProduct.m_productID);
        EXPECT_NE(entriesReturned[0].m_subID, removingSubID);
    }

    TEST_F(AssetProcessingStateDataUnitTest, TestProductDependency_AddProductDependency_Succeeds)
    {
        AddDefaultProduct();
        // Add a second product
        AZ::Uuid validSourceGuid = AZ::Uuid::CreateRandom();
        SourceDatabaseEntry source = SourceDatabaseEntry(m_defaultScanFolder.m_scanFolderID, "SomeSource1.tif", validSourceGuid, "");
        EXPECT_TRUE(m_connection.SetSource(source));

        AZ::Uuid validBuilderGuid = AZ::Uuid::CreateRandom();
        AZ::u32 validFingerprint = 2;
        AZ::u32 validJobRunKey = 1;
        AzToolsFramework::AssetSystem::JobStatus statusCompleted = AzToolsFramework::AssetSystem::JobStatus::Completed;
        JobDatabaseEntry job = JobDatabaseEntry(source.m_sourceID, "jobkey1", validFingerprint, "pc", validBuilderGuid, statusCompleted, validJobRunKey);
        EXPECT_TRUE(m_connection.SetJob(job));

        AZ::Data::AssetType validAssetType = AZ::Data::AssetType::CreateRandom();
        ProductDatabaseEntry product = ProductDatabaseEntry(job.m_jobID, 1, "SomeProduct1.dds", validAssetType);
        EXPECT_TRUE(m_connection.SetProduct(product));

        // There are no product dependencies yet so trying to find one should fail
        ProductDependencyDatabaseEntry productDependency;
        ProductDatabaseEntryContainer products;
        ProductDependencyDatabaseEntryContainer productDependencies;
        EXPECT_FALSE(m_connection.GetProductDependencies(productDependencies));
        EXPECT_FALSE(m_connection.GetProductDependencyByProductDependencyID(3443, productDependency));
        EXPECT_FALSE(m_connection.GetProductDependenciesByProductID(3443, productDependencies));
        EXPECT_FALSE(m_connection.GetDirectProductDependencies(3443, products));
        EXPECT_FALSE(m_connection.GetAllProductDependencies(3443, products));

        AZStd::string platform;
        // Trying to add a product dependency without a valid product pk should fail
        productDependency = ProductDependencyDatabaseEntry(234234, m_defaultSource.m_sourceGuid, 1, 0, platform, true);
        {
            UnitTestUtils::AssertAbsorber absorber;
            EXPECT_FALSE(m_connection.SetProductDependency(productDependency));
            EXPECT_GE(absorber.m_numWarningsAbsorbed, 0);
        }

        // Setting a valid product pk should allow it to be added
        // Product -> Product2
        productDependency = ProductDependencyDatabaseEntry(m_defaultProduct.m_productID, validSourceGuid, 2, 0, platform, true);
        EXPECT_TRUE(m_connection.SetProductDependency(productDependency));
        EXPECT_NE(productDependency.m_productDependencyID, AzToolsFramework::AssetDatabase::InvalidEntryId);

        // Get all product dependencies, there should only the one we added
        productDependencies.clear();
        EXPECT_TRUE(m_connection.GetProductDependencies(productDependencies));
        EXPECT_EQ(productDependencies.size(), 1);

        EXPECT_TRUE(Internal::ProductDependenciesContainProductDependencyID(productDependencies, productDependency.m_productDependencyID));
        EXPECT_TRUE(Internal::ProductDependenciesContainProductID(productDependencies, productDependency.m_productPK));
        EXPECT_TRUE(Internal::ProductDependenciesContainDependencySoureGuid(productDependencies, productDependency.m_dependencySourceGuid));
        EXPECT_TRUE(Internal::ProductDependenciesContainDependencySubID(productDependencies, productDependency.m_dependencySubID));
        EXPECT_TRUE(Internal::ProductDependenciesContainDependencyFlags(productDependencies, productDependency.m_dependencyFlags));

        // Add the same product again, should not add another because it already exists, so we should get the same id
        ProductDependencyDatabaseEntry dupeProductDependency(productDependency);
        dupeProductDependency.m_productDependencyID = AzToolsFramework::AssetDatabase::InvalidEntryId;
        EXPECT_TRUE(m_connection.SetProductDependency(dupeProductDependency));
        EXPECT_EQ(dupeProductDependency, dupeProductDependency);
        EXPECT_EQ(dupeProductDependency, dupeProductDependency);

        // Get all product dependencies, there should still only the one we added
        productDependencies.clear();
        EXPECT_TRUE(m_connection.GetProductDependencies(productDependencies));
        EXPECT_EQ(productDependencies.size(), 1);
        EXPECT_TRUE(Internal::ProductDependenciesContainProductDependencyID(productDependencies, productDependency.m_productDependencyID));
        EXPECT_TRUE(Internal::ProductDependenciesContainProductID(productDependencies, productDependency.m_productPK));
        EXPECT_TRUE(Internal::ProductDependenciesContainDependencySoureGuid(productDependencies, productDependency.m_dependencySourceGuid));
        EXPECT_TRUE(Internal::ProductDependenciesContainDependencySubID(productDependencies, productDependency.m_dependencySubID));
        EXPECT_TRUE(Internal::ProductDependenciesContainDependencyFlags(productDependencies, productDependency.m_dependencyFlags));
    }

    TEST_F(AssetProcessingStateDataUnitTest, TestProductDependency_VerifyProductDependency_Succeeds)
    {
        /* Verify the following product dependency tree
        *
        * products[0] -> products[1] -> products[2] -> products[4] -> products[5]
        *                    \
        *                     -> products[3]
        */
        ProductDatabaseEntryContainer products;
        CreateProductDependencyTree(products);

        // Direct Deps
        // products[0] -> products[1]
        ProductDatabaseEntryContainer dependentProducts;
        EXPECT_TRUE(m_connection.GetDirectProductDependencies(products[0].m_productID, dependentProducts));
        EXPECT_EQ(dependentProducts.size(), 1);
        EXPECT_TRUE(Internal::ProductsContainProductID(products, products[1].m_productID));

        // products[1] -> products[2], products[3]
        dependentProducts.clear();
        EXPECT_TRUE(m_connection.GetDirectProductDependencies(products[1].m_productID, dependentProducts));
        EXPECT_EQ(dependentProducts.size(), 2);
        EXPECT_TRUE(Internal::ProductsContainProductID(dependentProducts, products[2].m_productID));
        EXPECT_TRUE(Internal::ProductsContainProductID(dependentProducts, products[3].m_productID));

        // products[2] -> products[4]
        dependentProducts.clear();
        EXPECT_TRUE(m_connection.GetDirectProductDependencies(products[2].m_productID, dependentProducts));
        EXPECT_EQ(dependentProducts.size(), 1);
        EXPECT_TRUE(Internal::ProductsContainProductID(dependentProducts, products[4].m_productID));

        // products[3] ->
        dependentProducts.clear();
        EXPECT_FALSE(m_connection.GetDirectProductDependencies(products[3].m_productID, dependentProducts));
        EXPECT_EQ(dependentProducts.size(), 0);

        // products[4] -> products[5]
        dependentProducts.clear();
        EXPECT_TRUE(m_connection.GetDirectProductDependencies(products[4].m_productID, dependentProducts));
        EXPECT_EQ(dependentProducts.size(), 1);
        EXPECT_TRUE(Internal::ProductsContainProductID(dependentProducts, products[5].m_productID));

        // products[5] ->
        dependentProducts.clear();
        EXPECT_FALSE(m_connection.GetDirectProductDependencies(products[5].m_productID, dependentProducts));
        EXPECT_EQ(dependentProducts.size(), 0);

        // All Deps
        // products[0] -> products[1], products[2], products[3], products[4], products[5]
        dependentProducts.clear();
        EXPECT_TRUE(m_connection.GetAllProductDependencies(products[0].m_productID, dependentProducts));
        EXPECT_EQ(dependentProducts.size(), 5);
        EXPECT_TRUE(Internal::ProductsContainProductID(dependentProducts, products[1].m_productID));
        EXPECT_TRUE(Internal::ProductsContainProductID(dependentProducts, products[2].m_productID));
        EXPECT_TRUE(Internal::ProductsContainProductID(dependentProducts, products[3].m_productID));
        EXPECT_TRUE(Internal::ProductsContainProductID(dependentProducts, products[4].m_productID));
        EXPECT_TRUE(Internal::ProductsContainProductID(dependentProducts, products[5].m_productID));

        // products[1] -> products[2], products[3], products[4], products[5]
        dependentProducts.clear();
        EXPECT_TRUE(m_connection.GetAllProductDependencies(products[1].m_productID, dependentProducts));
        EXPECT_EQ(dependentProducts.size(), 4);
        EXPECT_TRUE(Internal::ProductsContainProductID(dependentProducts, products[2].m_productID));
        EXPECT_TRUE(Internal::ProductsContainProductID(dependentProducts, products[3].m_productID));
        EXPECT_TRUE(Internal::ProductsContainProductID(dependentProducts, products[4].m_productID));
        EXPECT_TRUE(Internal::ProductsContainProductID(dependentProducts, products[5].m_productID));

        // products[2] -> products[4], products[5]
        dependentProducts.clear();
        EXPECT_TRUE(m_connection.GetAllProductDependencies(products[2].m_productID, dependentProducts));
        EXPECT_EQ(dependentProducts.size(), 2);
        EXPECT_TRUE(Internal::ProductsContainProductID(dependentProducts, products[4].m_productID));
        EXPECT_TRUE(Internal::ProductsContainProductID(dependentProducts, products[5].m_productID));

        // products[3] ->
        dependentProducts.clear();
        EXPECT_FALSE(m_connection.GetAllProductDependencies(products[3].m_productID, dependentProducts));
        EXPECT_EQ(dependentProducts.size(), 0);

        // products[4] -> products[5]
        dependentProducts.clear();
        EXPECT_TRUE(m_connection.GetAllProductDependencies(products[4].m_productID, dependentProducts));
        EXPECT_EQ(dependentProducts.size(), 1);
        EXPECT_TRUE(Internal::ProductsContainProductID(dependentProducts, products[5].m_productID));

        // products[5] ->
        dependentProducts.clear();
        EXPECT_FALSE(m_connection.GetAllProductDependencies(products[5].m_productID, dependentProducts));
        EXPECT_EQ(dependentProducts.size(), 0);
    }

    TEST_F(AssetProcessingStateDataUnitTest, TestProductDependency_VerifyCircularProductDependency_Succeeds)
    {
        /* Verify the following circular dependency tree
        * v-----------------------------------------------------------------------<
        * |                                                                       |
        * products[0] -> products[1] -> products[2] -> products[4] -> products[6]-^
        *                    \
        *                     -> products[3]
        */
        SourceDatabaseEntryContainer sources;
        ProductDatabaseEntryContainer products;
        AZStd::string platform;
        CreateProductDependencyTree(sources, products, platform);
        // products[5] -> products[0] (This creates a circular dependency)
        ProductDependencyDatabaseEntry productDependency(products[5].m_productID, sources[0].m_sourceGuid, 0, 0, platform, true);
        EXPECT_TRUE(m_connection.SetProductDependency(productDependency));

        // products[5] -> products[0]
        ProductDatabaseEntryContainer dependentProducts;
        EXPECT_TRUE(m_connection.GetDirectProductDependencies(products[5].m_productID, dependentProducts));
        EXPECT_EQ(dependentProducts.size(), 1);
        EXPECT_TRUE(Internal::ProductsContainProductID(dependentProducts, products[0].m_productID));

        // products[2] -> products[4], products[5], products[0], products[1], products[3]
        dependentProducts.clear();
        EXPECT_TRUE(m_connection.GetAllProductDependencies(products[2].m_productID, dependentProducts));
        EXPECT_EQ(dependentProducts.size(), 5);
        EXPECT_TRUE(Internal::ProductsContainProductID(dependentProducts, products[4].m_productID));
        EXPECT_TRUE(Internal::ProductsContainProductID(dependentProducts, products[5].m_productID));
        EXPECT_TRUE(Internal::ProductsContainProductID(dependentProducts, products[0].m_productID));
        EXPECT_TRUE(Internal::ProductsContainProductID(dependentProducts, products[1].m_productID));
        EXPECT_TRUE(Internal::ProductsContainProductID(dependentProducts, products[3].m_productID));

        m_connection.RemoveProductDependencyByProductId(products[4].m_productID);

        // products[1] -> products[2], products[3], products[4]
        dependentProducts.clear();
        EXPECT_TRUE(m_connection.GetAllProductDependencies(products[1].m_productID, dependentProducts));
        EXPECT_EQ(dependentProducts.size(), 3);
        EXPECT_TRUE(Internal::ProductsContainProductID(dependentProducts, products[2].m_productID));
        EXPECT_TRUE(Internal::ProductsContainProductID(dependentProducts, products[3].m_productID));
        EXPECT_TRUE(Internal::ProductsContainProductID(dependentProducts, products[4].m_productID));
    }

    TEST_F(AssetProcessingStateDataUnitTest, TestProductDependency_RemoveProductDependency_Succeeds)
    {
        SourceDatabaseEntryContainer sources;
        CreateProductDependencyTree(sources);

        // Teardown
        // The product dependencies should cascade delete
        for (int index = 0; index < sources.size(); ++index)
        {
            EXPECT_TRUE(m_connection.RemoveSource(sources[index].m_sourceID));
        }

        ProductDependencyDatabaseEntryContainer productDependencies;
        ProductDatabaseEntryContainer dependentProducts;
        EXPECT_FALSE(m_connection.GetProductDependencies(productDependencies));
        EXPECT_FALSE(m_connection.GetDirectProductDependencies(m_defaultProduct.m_productID, dependentProducts));
        EXPECT_FALSE(m_connection.GetAllProductDependencies(m_defaultProduct.m_productID, dependentProducts));
    }

    TEST_F(AssetProcessingStateDataUnitTest, TestBuilderInfo_SetBuilderInfoTable_Succeeds)
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

    TEST_F(AssetProcessingStateDataUnitTest, TestSourceDependency_VerifySourceDependency_Succeeds)
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

    TEST_F(AssetProcessingStateDataUnitTest, TestSourceFingerprint_QuerySourceAnalysisFingerprint_Succeeds)
    {
        using SourceDatabaseEntry = AzToolsFramework::AssetDatabase::SourceDatabaseEntry;

        AddDefaultSource();
        // Add another source
        AZ::Uuid validSourceGuid = AZ::Uuid::CreateRandom();
        SourceDatabaseEntry source(m_defaultScanFolder.m_scanFolderID, "SomeSource1.tif", validSourceGuid, "54321");
        EXPECT_TRUE(m_connection.SetSource(source));

        AZStd::string resultString("garbage");
        // its not a database error to ask for a file that does not exist:
        EXPECT_TRUE(m_connection.QuerySourceAnalysisFingerprint("does not exist", m_defaultScanFolder.m_scanFolderID, resultString));
        // but we do expect it to empty the result:
        EXPECT_TRUE(resultString.empty());
        EXPECT_TRUE(m_connection.QuerySourceAnalysisFingerprint("SomeSource.tif", m_defaultScanFolder.m_scanFolderID, resultString));
        EXPECT_EQ(resultString, "12345");
        EXPECT_TRUE(m_connection.QuerySourceAnalysisFingerprint("SomeSource1.tif", m_defaultScanFolder.m_scanFolderID, resultString));
        EXPECT_EQ(resultString, "54321");
    }
}
