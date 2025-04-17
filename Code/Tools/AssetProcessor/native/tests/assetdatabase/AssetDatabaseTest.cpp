/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzToolsFramework/API/AssetDatabaseBus.h>
#include <AzCore/std/sort.h>

#include <native/tests/AssetProcessorTest.h>
#include <native/tests/MockAssetDatabaseRequestsHandler.h>
#include <native/AssetDatabase/AssetDatabase.h>
#include <AzToolsFramework/AssetDatabase/PathOrUuid.h>

namespace UnitTests
{
    using namespace testing;
    using namespace AssetProcessor;
    using namespace AzToolsFramework::AssetDatabase;

    using AzToolsFramework::AssetDatabase::ProductDatabaseEntry;
    using AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry;
    using AzToolsFramework::AssetDatabase::SourceDatabaseEntry;
    using AzToolsFramework::AssetDatabase::SourceFileDependencyEntry;
    using AzToolsFramework::AssetDatabase::SourceFileDependencyEntryContainer;
    using AzToolsFramework::AssetDatabase::JobDatabaseEntry;
    using AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer;
    using AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry;
    using AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer;
    using AzToolsFramework::AssetDatabase::AssetDatabaseConnection;
    using AzToolsFramework::AssetDatabase::FileDatabaseEntry;
    using AzToolsFramework::AssetDatabase::StatDatabaseEntry;
    using AzToolsFramework::AssetDatabase::StatDatabaseEntryContainer;


    class AssetDatabaseTest : public AssetProcessorTest
    {
    public:
        void SetUp() override
        {
            AssetProcessorTest::SetUp();
            m_data.reset(new StaticData());
            m_data->m_databaseLocationListener.m_assetDatabasePath = ":memory:"; // this special string causes SQLITE to open the database in memory and not touch disk at all.
            // Initialize the database:
            m_data->m_connection.ClearData(); // this is expected to reset/clear/reopen
        }

        void TearDown() override
        {
            m_data.reset();
            AssetProcessorTest::TearDown();
        }

        // COVERAGE TEST
        // For each of these coverage tests we'll start with the same kind of database, one with
        // SCAN FOLDER:          rootportkey
        //        SOURCE:            somefile.tif
        //             JOB:              "some job key"  runkey: 1   "pc"  SUCCEEDED
        //                 Product:         "someproduct1.dds"  subid: 1
        //                 Product:         "someproduct2.dds"  subid: 2
        //        SOURCE:            otherfile.tif
        //             JOB:              "some other job key"   runkey: 2  "osx" FAILED
        //                 Product:         "someproduct3.dds"  subid: 3
        //                 Product:         "someproduct4.dds"  subid: 4
        void CreateCoverageTestData()
        {
            m_data->m_scanFolder = { "c:/O3DE/dev", "dev", "rootportkey" };
            ASSERT_TRUE(m_data->m_connection.SetScanFolder(m_data->m_scanFolder));

            m_data->m_sourceFile1 = { m_data->m_scanFolder.m_scanFolderID, "somefile.tif", AZ::Uuid::CreateRandom(), "AnalysisFingerprint1"};
            m_data->m_sourceFile2 = { m_data->m_scanFolder.m_scanFolderID, "otherfile.tif", AZ::Uuid::CreateRandom(), "AnalysisFingerprint2"};
            ASSERT_TRUE(m_data->m_connection.SetSource(m_data->m_sourceFile1));
            ASSERT_TRUE(m_data->m_connection.SetSource(m_data->m_sourceFile2));

            m_data->m_job1 = { m_data->m_sourceFile1.m_sourceID, "some job key", 123, "pc", AZ::Uuid::CreateRandom(), AzToolsFramework::AssetSystem::JobStatus::Completed, 1 };
            m_data->m_job2 = { m_data->m_sourceFile2.m_sourceID, "some other job key", 345, "osx", AZ::Uuid::CreateRandom(), AzToolsFramework::AssetSystem::JobStatus::Failed, 2 };
            ASSERT_TRUE(m_data->m_connection.SetJob(m_data->m_job1));
            ASSERT_TRUE(m_data->m_connection.SetJob(m_data->m_job2));

            m_data->m_product1 = { m_data->m_job1.m_jobID, 1, "someproduct1.dds", AZ::Data::AssetType::CreateRandom() };
            m_data->m_product2 = { m_data->m_job1.m_jobID, 2, "someproduct2.dds", AZ::Data::AssetType::CreateRandom() };
            m_data->m_product3 = { m_data->m_job2.m_jobID, 3, "someproduct3.dds", AZ::Data::AssetType::CreateRandom() };
            m_data->m_product4 = { m_data->m_job2.m_jobID, 4, "someproduct4.dds", AZ::Data::AssetType::CreateRandom() };

            ASSERT_TRUE(m_data->m_connection.SetProduct(m_data->m_product1));
            ASSERT_TRUE(m_data->m_connection.SetProduct(m_data->m_product2));
            ASSERT_TRUE(m_data->m_connection.SetProduct(m_data->m_product3));
            ASSERT_TRUE(m_data->m_connection.SetProduct(m_data->m_product4));
        }

        /******************************************** Create and insert Stat entry ********************************************/
        //! Returns the first stat entry to be inserted into Stats table. Users can specify the prefix of the StatName
        StatDatabaseEntry GetFirstStatEntry(const AZStd::string& namePrefix = "")
        {
            return StatDatabaseEntry{ namePrefix + "a", 10, 100 };
        }

        //! Step statEntry to the next inserted entry, which increments the name's last character by 1 in ASCII order, increment 20 in StatValue, and increment
        //! 300 in LastLogTime. For example, if statEntry was passed in as (StatName=b, StatValue=30, LastLogTime=400), it will become
        //! (StatName=c, StatValue=50, LastLogTime=700) after the invocation.
        void StepStatEntry(StatDatabaseEntry& statEntry)
        {
            statEntry.m_statName.back()++;
            statEntry.m_statValue = statEntry.m_statValue + 20;
            statEntry.m_lastLogTime = statEntry.m_lastLogTime + 300;
        }

        //! Insert _StatCount_ stat entries into Stats table, starting with first entry given by GetFirstStatEntry().
        void InsertStatsTestData(const unsigned int StatCount, const AZStd::string& namePrefix = "")
        {
            StatDatabaseEntry statEntry = GetFirstStatEntry(namePrefix);
            for (unsigned int i = 0; i < StatCount; ++i)
            {
                ASSERT_TRUE(m_data->m_connection.ReplaceStat(statEntry));
                StepStatEntry(statEntry);
            }
        }

    protected:

        void SetAndCheckMissingDependency(
            AzToolsFramework::AssetDatabase::MissingProductDependencyDatabaseEntry& updatedMissingDependency,
            const AzToolsFramework::AssetDatabase::MissingProductDependencyDatabaseEntry& originalMissingDependency);

        struct StaticData
        {
            // these variables are created during SetUp() and destroyed during TearDown() and thus are always available during tests using this fixture:
            AZStd::string m_databaseLocation;
            AssetProcessor::MockAssetDatabaseRequestsHandler m_databaseLocationListener;
            AssetProcessor::AssetDatabaseConnection m_connection;

            // The following database entry variables are initialized only when you call coverage test data CreateCoverageTestData().
            // Tests which don't need or want a pre-made database should not call CreateCoverageTestData() but note that in that case
            // these entries will be empty and their identifiers will be -1.
            ScanFolderDatabaseEntry m_scanFolder;
            SourceDatabaseEntry m_sourceFile1;
            SourceDatabaseEntry m_sourceFile2;
            JobDatabaseEntry m_job1;
            JobDatabaseEntry m_job2;
            ProductDatabaseEntry m_product1;
            ProductDatabaseEntry m_product2;
            ProductDatabaseEntry m_product3;
            ProductDatabaseEntry m_product4;

        };

        // we store the above data in a unique_ptr so that its memory can be cleared during TearDown() in one call, before we destroy the memory
        // allocator, reducing the chance of missing or forgetting to destroy one in the future.
        AZStd::unique_ptr<StaticData> m_data;
    };

    TEST_F(AssetDatabaseTest, UpdateJob_Succeeds)
    {
        using namespace AzToolsFramework::AssetDatabase;
        CreateCoverageTestData();

        m_data->m_job1.m_warningCount = 11;
        m_data->m_job1.m_errorCount = 22;

        ASSERT_TRUE(m_data->m_connection.SetJob(m_data->m_job1));

        JobDatabaseEntryContainer jobs;
        ASSERT_TRUE(m_data->m_connection.GetJobsBySourceID(m_data->m_job1.m_sourcePK, jobs));
        ASSERT_EQ(jobs.size(), 1);
        ASSERT_EQ(m_data->m_job1, jobs[0]);
    }

    TEST_F(AssetDatabaseTest, GetProducts_WithEmptyDatabase_Fails_ReturnsNoProducts)
    {
        ProductDatabaseEntryContainer products;
        EXPECT_FALSE(m_data->m_connection.GetProducts(products));
        EXPECT_EQ(products.size(), 0);
    }

    TEST_F(AssetDatabaseTest, GetProductByProductID_NotFound_Fails_ReturnsNoProducts)
    {
        AzToolsFramework::AssetDatabase::ProductDatabaseEntry product;
        EXPECT_FALSE(m_data->m_connection.GetProductByProductID(3443, product));
        EXPECT_EQ(product, AzToolsFramework::AssetDatabase::ProductDatabaseEntry());
    }

    TEST_F(AssetDatabaseTest, GetProductsLikeProductName_NotFound_Fails_ReturnsNoProducts)
    {
        ProductDatabaseEntryContainer products;
        EXPECT_FALSE(m_data->m_connection.GetProductsLikeProductName("none", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::Raw, products));
        EXPECT_EQ(products.size(), 0);

        EXPECT_FALSE(m_data->m_connection.GetProductsLikeProductName("none", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::StartsWith, products));
        EXPECT_EQ(products.size(), 0);

        EXPECT_FALSE(m_data->m_connection.GetProductsLikeProductName("none", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::EndsWith, products));
        EXPECT_EQ(products.size(), 0);

        EXPECT_FALSE(m_data->m_connection.GetProductsLikeProductName("none", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::Matches, products));
        EXPECT_EQ(products.size(), 0);
    }

    TEST_F(AssetDatabaseTest, GetProductsBySourceID_NotFound_Fails_ReturnsNoProducts)
    {
        ProductDatabaseEntryContainer products;
        EXPECT_FALSE(m_data->m_connection.GetProductsBySourceID(25654, products));
        EXPECT_EQ(products.size(), 0);
    }

    TEST_F(AssetDatabaseTest, SetProduct_InvalidProductID_Fails)
    {
        // trying to "overwrite" a product that does not exist should fail and emit error.
        AZ::Data::AssetType validAssetType1 = AZ::Data::AssetType::CreateRandom();
        ProductDatabaseEntry product { 123213, 234234, 1, "SomeProduct1.dds", validAssetType1 };

        m_errorAbsorber->Clear();
        EXPECT_FALSE(m_data->m_connection.SetProduct(product));
        EXPECT_GT(m_errorAbsorber->m_numErrorsAbsorbed, 0);
        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this

        // make sure it didn't actually touch the db as a side effect:
        ProductDatabaseEntryContainer products;
        EXPECT_FALSE(m_data->m_connection.GetProducts(products));
        EXPECT_EQ(products.size(), 0);
    }

    TEST_F(AssetDatabaseTest, SetProduct_InvalidJobPK_Fails)
    {
        AZ::Data::AssetType validAssetType1 = AZ::Data::AssetType::CreateRandom();

        // -1 means insert a new product, but the JobPK is an enforced FK constraint, so this should fail since there
        // won't be a Job with the PK of 234234.

        ProductDatabaseEntry product{ AzToolsFramework::AssetDatabase::InvalidEntryId, 234234, 1, "SomeProduct1.dds", validAssetType1 };

        m_errorAbsorber->Clear();
        EXPECT_FALSE(m_data->m_connection.SetProduct(product));
        EXPECT_GT(m_errorAbsorber->m_numErrorsAbsorbed, 0);
        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this

        // make sure it didn't actually touch the db as a side effect:
        ProductDatabaseEntryContainer products;
        EXPECT_FALSE(m_data->m_connection.GetProducts(products));
        EXPECT_EQ(products.size(), 0);
    }

    // if we give it a valid command and a -1 product, we expect it to succeed without assert or warning
    // and we expect it to tell us (by filling in the entry) what the new PK is.
    TEST_F(AssetDatabaseTest, SetProduct_AutoPK_Succeeds)
    {
        AZ::Data::AssetType validAssetType1 = AZ::Data::AssetType::CreateRandom();

        // to add a product legitimately you have to have a full chain of primary keys, chain is:
        // ScanFolder --> Source --> job --> product.
        // we'll create all of those first (except product) before starting the product test.

        //add a scanfolder.  None of this has to exist in real disk, this is a db test only.
        ScanFolderDatabaseEntry scanFolder{ "c:/O3DE/dev", "dev", "rootportkey" };
        EXPECT_TRUE(m_data->m_connection.SetScanFolder(scanFolder));
        ASSERT_NE(scanFolder.m_scanFolderID, AzToolsFramework::AssetDatabase::InvalidEntryId);

        SourceDatabaseEntry sourceEntry {scanFolder.m_scanFolderID, "somefile.tif", AZ::Uuid::CreateRandom(), "fingerprint1"};
        EXPECT_TRUE(m_data->m_connection.SetSource(sourceEntry));
        ASSERT_NE(sourceEntry.m_sourceID, AzToolsFramework::AssetDatabase::InvalidEntryId);

        JobDatabaseEntry jobEntry{ sourceEntry.m_sourceID, "some job key", 123, "pc", AZ::Uuid::CreateRandom(), AzToolsFramework::AssetSystem::JobStatus::Completed, 1 };
        EXPECT_TRUE(m_data->m_connection.SetJob(jobEntry));
        ASSERT_NE(jobEntry.m_jobID, AzToolsFramework::AssetDatabase::InvalidEntryId);

        // --- set up complete --- perform the test!
        AZStd::bitset<64> flags;
        flags.set(static_cast<int>(AssetBuilderSDK::ProductOutputFlags::IntermediateAsset | AssetBuilderSDK::ProductOutputFlags::ProductAsset));

        ProductDatabaseEntry product{ AzToolsFramework::AssetDatabase::InvalidEntryId, jobEntry.m_jobID, 1, "SomeProduct1.dds", validAssetType1,
            AZ::Uuid::CreateNull(), 0, flags};

        m_errorAbsorber->Clear();
        EXPECT_TRUE(m_data->m_connection.SetProduct(product));
        ASSERT_NE(product.m_productID, AzToolsFramework::AssetDatabase::InvalidEntryId);

        EXPECT_EQ(m_errorAbsorber->m_numErrorsAbsorbed, 0);
        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this

        // read it back from the DB and make sure its identical to what was written

        ProductDatabaseEntry productFromDB;
        EXPECT_TRUE(m_data->m_connection.GetProductByProductID(product.m_productID, productFromDB));
        ASSERT_EQ(product, productFromDB);
    }


    // update an existing job by giving it a specific PK of a known existing item.
    TEST_F(AssetDatabaseTest, SetProduct_SpecificPK_Succeeds_DifferentSubID)
    {
        AZ::Data::AssetType validAssetType1 = AZ::Data::AssetType::CreateRandom();

        // to add a product legitimately you have to have a full chain of primary keys, chain is:
        // ScanFolder --> Source --> job --> product.
        // we'll create all of those first (except product) before starting the product test.
        ScanFolderDatabaseEntry scanFolder{ "c:/O3DE/dev", "dev", "rootportkey" };
        ASSERT_TRUE(m_data->m_connection.SetScanFolder(scanFolder));

        SourceDatabaseEntry sourceEntry{ scanFolder.m_scanFolderID, "somefile.tif", AZ::Uuid::CreateRandom(), "fingerprint1" };
        ASSERT_TRUE(m_data->m_connection.SetSource(sourceEntry));

        // two different job entries.
        JobDatabaseEntry jobEntry{ sourceEntry.m_sourceID, "some job key", 123, "pc", AZ::Uuid::CreateRandom(), AzToolsFramework::AssetSystem::JobStatus::Completed, 1 };
        JobDatabaseEntry jobEntry2{ sourceEntry.m_sourceID, "some job key 2", 345, "pc", AZ::Uuid::CreateRandom(), AzToolsFramework::AssetSystem::JobStatus::Completed, 2 };
        ASSERT_TRUE(m_data->m_connection.SetJob(jobEntry));
        ASSERT_TRUE(m_data->m_connection.SetJob(jobEntry2));

        AZStd::bitset<64> flags;
        flags.set(static_cast<int>(AssetBuilderSDK::ProductOutputFlags::ProductAsset));
        ProductDatabaseEntry product{ AzToolsFramework::AssetDatabase::InvalidEntryId, jobEntry.m_jobID, 1, "SomeProduct1.dds", validAssetType1,
            AZ::Uuid::CreateNull(), 0, flags};
        ASSERT_TRUE(m_data->m_connection.SetProduct(product));

        // --- set up complete --- perform the test!
        // update all the fields of that product and then write it to the db.
        ProductDatabaseEntry newProductData = product; // copy first
        // now change all the fields:
        newProductData.m_assetType = AZ::Uuid::CreateRandom();
        newProductData.m_productName = "different name.dds";
        newProductData.m_subID = 2;
        newProductData.m_jobPK = jobEntry2.m_jobID; // move it to the other job, too!
        newProductData.m_flags.set(static_cast<int>(AssetBuilderSDK::ProductOutputFlags::IntermediateAsset));

        // update the product
        EXPECT_TRUE(m_data->m_connection.SetProduct(newProductData));
        ASSERT_NE(newProductData.m_productID, AzToolsFramework::AssetDatabase::InvalidEntryId);

        // it should not have entered a new product but instead overwritten the old one.
        ASSERT_EQ(product.m_productID, newProductData.m_productID);

        // read it back from DB and verify:
        ProductDatabaseEntry productFromDB;
        EXPECT_TRUE(m_data->m_connection.GetProductByProductID(newProductData.m_productID, productFromDB));
        ASSERT_EQ(newProductData, productFromDB);

        ProductDatabaseEntryContainer products;
        EXPECT_TRUE(m_data->m_connection.GetProducts(products));
        EXPECT_EQ(products.size(), 1);
    }

    // update an existing job by giving it a subID and JobID which is enough to uniquely identify a product (since products may not have the same subid from the same job).
    // this is actually a very common case (same job id, same subID)
    TEST_F(AssetDatabaseTest, SetProduct_SpecificPK_Succeeds_SameSubID_SameJobID)
    {
        ScanFolderDatabaseEntry scanFolder{ "c:/O3DE/dev", "dev", "rootportkey"};
        ASSERT_TRUE(m_data->m_connection.SetScanFolder(scanFolder));
        SourceDatabaseEntry sourceEntry{ scanFolder.m_scanFolderID, "somefile.tif", AZ::Uuid::CreateRandom(), "fingerprint1" };
        ASSERT_TRUE(m_data->m_connection.SetSource(sourceEntry));
        JobDatabaseEntry jobEntry{ sourceEntry.m_sourceID, "some job key", 123, "pc", AZ::Uuid::CreateRandom(), AzToolsFramework::AssetSystem::JobStatus::Completed, 1 };
        ASSERT_TRUE(m_data->m_connection.SetJob(jobEntry));
        ProductDatabaseEntry product{ AzToolsFramework::AssetDatabase::InvalidEntryId, jobEntry.m_jobID, 1, "SomeProduct1.dds", AZ::Data::AssetType::CreateRandom() };
        ASSERT_TRUE(m_data->m_connection.SetProduct(product));

        // --- set up complete --- perform the test!
        // update all the fields of that product and then write it to the db.
        ProductDatabaseEntry newProductData = product; // copy first
                                                       // now change all the fields:
        newProductData.m_assetType = AZ::Uuid::CreateRandom();
        newProductData.m_productName = "different name.dds";
        newProductData.m_productID = AzToolsFramework::AssetDatabase::InvalidEntryId; // wipe out the product ID, so that we can make sure it returns it.
        // we don't change the subID here or the job ID.

        // update the product
        EXPECT_TRUE(m_data->m_connection.SetProduct(newProductData));
        ASSERT_NE(newProductData.m_productID, AzToolsFramework::AssetDatabase::InvalidEntryId);

        // it should not have entered a new product but instead overwritten the old one.
        EXPECT_EQ(product.m_productID, newProductData.m_productID);

        // read it back from DB and verify:
        ProductDatabaseEntry productFromDB;
        EXPECT_TRUE(m_data->m_connection.GetProductByProductID(newProductData.m_productID, productFromDB));
        EXPECT_EQ(newProductData, productFromDB);

        ProductDatabaseEntryContainer products;
        EXPECT_TRUE(m_data->m_connection.GetProducts(products));
        EXPECT_EQ(products.size(), 1);
    }

    TEST_F(AssetDatabaseTest, GetProductsByJobID_InvalidID_NotFound_ReturnsFalse)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        EXPECT_FALSE(m_data->m_connection.GetProductsByJobID(-1, resultProducts));
        EXPECT_EQ(resultProducts.size(), 0);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsByJobID_Valid_ReturnsTrue_FindsProducts)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        EXPECT_TRUE(m_data->m_connection.GetProductsByJobID(m_data->m_job1.m_jobID, resultProducts));
        EXPECT_EQ(resultProducts.size(), 2); // should have found the first two products.

        // since there is no ordering, we just have to find both of them:
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductByJobIDSubId_InvalidID_NotFound_ReturnsFalse)
    {
        CreateCoverageTestData();

        ProductDatabaseEntry resultProduct;

        EXPECT_FALSE(m_data->m_connection.GetProductByJobIDSubId(m_data->m_job1.m_jobID, aznumeric_caster(-1), resultProduct));
        EXPECT_FALSE(m_data->m_connection.GetProductByJobIDSubId(-1, m_data->m_product1.m_subID, resultProduct));
        EXPECT_FALSE(m_data->m_connection.GetProductByJobIDSubId(-1, aznumeric_caster(-1), resultProduct));
        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductByJobIDSubId_ValidID_Found_ReturnsTrue)
    {
        CreateCoverageTestData();

        ProductDatabaseEntry resultProduct;

        EXPECT_TRUE(m_data->m_connection.GetProductByJobIDSubId(m_data->m_job1.m_jobID, m_data->m_product1.m_subID, resultProduct));
        EXPECT_EQ(resultProduct, m_data->m_product1);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductBySourceGuidSubId_InvalidInputs_ProductNotFound)
    {
        CreateCoverageTestData();

        ProductDatabaseEntry resultProduct;

        AZ::Uuid invalidGuid = AZ::Uuid::CreateNull();
        AZ::s32 invalidSubId = -1;

        EXPECT_FALSE(m_data->m_connection.GetProductBySourceGuidSubId(invalidGuid, m_data->m_product1.m_subID, resultProduct));
        EXPECT_FALSE(m_data->m_connection.GetProductBySourceGuidSubId(m_data->m_sourceFile1.m_sourceGuid, invalidSubId, resultProduct));
        EXPECT_FALSE(m_data->m_connection.GetProductBySourceGuidSubId(invalidGuid, invalidSubId, resultProduct));

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0);
    }

    TEST_F(AssetDatabaseTest, GetProductBySourceGuidSubId_ValidInputs_ProductFound)
    {
        CreateCoverageTestData();

        ProductDatabaseEntry resultProduct;

        EXPECT_TRUE(m_data->m_connection.GetProductBySourceGuidSubId(m_data->m_sourceFile1.m_sourceGuid, m_data->m_product1.m_subID, resultProduct));
        EXPECT_EQ(resultProduct, m_data->m_product1);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0);
    }

    // --------------------------------------------------------------------------------------------------------------------
    // ------------------------------------------ GetProductsByProductName ------------------------------------------------
    // --------------------------------------------------------------------------------------------------------------------

    TEST_F(AssetDatabaseTest, GetProductsByProductName_EmptyString_NoResults)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;
        EXPECT_FALSE(m_data->m_connection.GetProductsByProductName(QString(), resultProducts));
        EXPECT_EQ(resultProducts.size(), 0);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsByProductName_NotFound_NoResult)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;
        EXPECT_FALSE(m_data->m_connection.GetProductsByProductName("akdsuhuksahdsak", resultProducts));
        EXPECT_EQ(resultProducts.size(), 0);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsByProductName_CorrectName_ReturnsCorrectResult)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;
        EXPECT_TRUE(m_data->m_connection.GetProductsByProductName("someproduct4.dds", resultProducts));
        EXPECT_EQ(resultProducts.size(), 1);
        EXPECT_EQ(resultProducts[0], m_data->m_product4);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    // tests all of the filters (beside name) to make sure they all function as expected.
    TEST_F(AssetDatabaseTest, GetProductsByProductName_FilterTest_BuilderGUID)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        // give it a random builder guid.  This should make it not match any products:
        EXPECT_FALSE(m_data->m_connection.GetProductsByProductName("someproduct4.dds", resultProducts, AZ::Uuid::CreateRandom()));
        EXPECT_EQ(resultProducts.size(), 0);

        // give it a correct builder guid but the wrong builder.  Job2's builder actually built product4.
        EXPECT_FALSE(m_data->m_connection.GetProductsByProductName("someproduct4.dds", resultProducts, m_data->m_job1.m_builderGuid));
        EXPECT_EQ(resultProducts.size(), 0);

        // give it correct data
        EXPECT_TRUE(m_data->m_connection.GetProductsByProductName("someproduct4.dds", resultProducts, m_data->m_job2.m_builderGuid));
        EXPECT_EQ(resultProducts.size(), 1);
        EXPECT_EQ(resultProducts[0], m_data->m_product4);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsByProductName_FilterTest_JobKey)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        // give it a random job key that is not going to match the existing job keys. This should make it not match any products:
        EXPECT_FALSE(m_data->m_connection.GetProductsByProductName("someproduct4.dds", resultProducts, AZ::Uuid::CreateNull(), "no matcher"));
        EXPECT_EQ(resultProducts.size(), 0);

        // give it a correct job key but not one that output that product.
        EXPECT_FALSE(m_data->m_connection.GetProductsByProductName("someproduct4.dds", resultProducts, AZ::Uuid::CreateNull(), QString(m_data->m_job1.m_jobKey.c_str())));
        EXPECT_EQ(resultProducts.size(), 0);

        // give it correct data
        EXPECT_TRUE(m_data->m_connection.GetProductsByProductName("someproduct4.dds", resultProducts, AZ::Uuid::CreateNull(), QString(m_data->m_job2.m_jobKey.c_str())));
        EXPECT_EQ(resultProducts.size(), 1);
        EXPECT_EQ(resultProducts[0], m_data->m_product4);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsByProductName_FilterTest_Platform)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        // give it a platform that is not going to match the existing job platforms.  This should make it not match any products:
        EXPECT_FALSE(m_data->m_connection.GetProductsByProductName("someproduct4.dds", resultProducts, AZ::Uuid::CreateNull(), QString(), "badplatform"));
        EXPECT_EQ(resultProducts.size(), 0);

        // give it a correct platform but not one that output that product.
        EXPECT_FALSE(m_data->m_connection.GetProductsByProductName("someproduct4.dds", resultProducts, AZ::Uuid::CreateNull(), QString(), "pc"));  // its actually osx
        EXPECT_EQ(resultProducts.size(), 0);

        // give it correct data
        EXPECT_TRUE(m_data->m_connection.GetProductsByProductName("someproduct4.dds", resultProducts, AZ::Uuid::CreateNull(), QString(), "osx"));
        EXPECT_EQ(resultProducts.size(), 1);
        EXPECT_EQ(resultProducts[0], m_data->m_product4);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsByProductName_FilterTest_Status)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        // give it a correct status but not one that output that product.
        EXPECT_FALSE(m_data->m_connection.GetProductsByProductName("someproduct4.dds", resultProducts, AZ::Uuid::CreateNull(), QString(), QString(), AzToolsFramework::AssetSystem::JobStatus::Completed));  // its actually failed
        EXPECT_EQ(resultProducts.size(), 0);

        // give it correct data
        EXPECT_TRUE(m_data->m_connection.GetProductsByProductName("someproduct4.dds", resultProducts, AZ::Uuid::CreateNull(), QString(), QString(), AzToolsFramework::AssetSystem::JobStatus::Failed));
        EXPECT_EQ(resultProducts.size(), 1);
        EXPECT_EQ(resultProducts[0], m_data->m_product4);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    // --------------------------------------------------------------------------------------------------------------------
    // ------------------------------------------ GetProductsLikeProductName ----------------------------------------------
    // --------------------------------------------------------------------------------------------------------------------

    TEST_F(AssetDatabaseTest, GetProductsLikeProductName_EmptyString_NoResults)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;
        EXPECT_FALSE(m_data->m_connection.GetProductsLikeProductName(QString(), AssetDatabaseConnection::StartsWith, resultProducts));
        EXPECT_EQ(resultProducts.size(), 0);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsLikeProductName_NotFound_NoResult)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;
        EXPECT_FALSE(m_data->m_connection.GetProductsLikeProductName("akdsuhuksahdsak", AssetDatabaseConnection::StartsWith, resultProducts));
        EXPECT_EQ(resultProducts.size(), 0);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsLikeProductName_CorrectName_StartsWith_ReturnsCorrectResult)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;
        EXPECT_TRUE(m_data->m_connection.GetProductsLikeProductName("someproduct4", AssetDatabaseConnection::StartsWith, resultProducts));
        EXPECT_EQ(resultProducts.size(), 1);
        EXPECT_EQ(resultProducts[0], m_data->m_product4);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsLikeProductName_CorrectName_EndsWith_ReturnsCorrectResult)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;
        EXPECT_TRUE(m_data->m_connection.GetProductsLikeProductName("product4.dds", AssetDatabaseConnection::EndsWith, resultProducts));
        EXPECT_EQ(resultProducts.size(), 1);
        EXPECT_EQ(resultProducts[0], m_data->m_product4);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsLikeProductName_CorrectName_Matches_ReturnsCorrectResult)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;
        EXPECT_TRUE(m_data->m_connection.GetProductsLikeProductName("product4", AssetDatabaseConnection::Matches, resultProducts));
        EXPECT_EQ(resultProducts.size(), 1);
        EXPECT_EQ(resultProducts[0], m_data->m_product4);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsLikeProductName_CorrectName_StartsWith_ReturnsMany)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        // a very broad search that matches all products.
        EXPECT_TRUE(m_data->m_connection.GetProductsLikeProductName("someproduct", AssetDatabaseConnection::StartsWith, resultProducts));
        EXPECT_EQ(resultProducts.size(), 4);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    // tests all of the filters (beside name) to make sure they all function as expected.
    TEST_F(AssetDatabaseTest, GetProductsLikeProductName_FilterTest_BuilderGUID)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        // give it a random builder guid.  This should make it not match any products:
        EXPECT_FALSE(m_data->m_connection.GetProductsLikeProductName("someproduct4", AssetDatabaseConnection::StartsWith, resultProducts, AZ::Uuid::CreateRandom()));
        EXPECT_EQ(resultProducts.size(), 0);

        // give it a correct builder guid but the wrong builder.  Job2's builder actually built product4.
        EXPECT_FALSE(m_data->m_connection.GetProductsLikeProductName("someproduct4", AssetDatabaseConnection::StartsWith, resultProducts, m_data->m_job1.m_builderGuid));
        EXPECT_EQ(resultProducts.size(), 0);

        // give it correct data
        EXPECT_TRUE(m_data->m_connection.GetProductsLikeProductName("someproduct4", AssetDatabaseConnection::StartsWith, resultProducts, m_data->m_job2.m_builderGuid));
        EXPECT_EQ(resultProducts.size(), 1);
        EXPECT_EQ(resultProducts[0], m_data->m_product4);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsLikeProductName_FilterTest_JobKey)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        // give it a random job key that is not going to match the existing job keys. This should make it not match any products:
        EXPECT_FALSE(m_data->m_connection.GetProductsLikeProductName("someproduct4", AssetDatabaseConnection::StartsWith,  resultProducts, AZ::Uuid::CreateNull(), "no matcher"));
        EXPECT_EQ(resultProducts.size(), 0);

        // give it a correct job key but not one that output that product.
        EXPECT_FALSE(m_data->m_connection.GetProductsLikeProductName("someproduct4", AssetDatabaseConnection::StartsWith, resultProducts, AZ::Uuid::CreateNull(), QString(m_data->m_job1.m_jobKey.c_str())));
        EXPECT_EQ(resultProducts.size(), 0);

        // give it correct data
        EXPECT_TRUE(m_data->m_connection.GetProductsLikeProductName("someproduct4", AssetDatabaseConnection::StartsWith, resultProducts, AZ::Uuid::CreateNull(), QString(m_data->m_job2.m_jobKey.c_str())));
        EXPECT_EQ(resultProducts.size(), 1);
        EXPECT_EQ(resultProducts[0], m_data->m_product4);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsLikeProductName_FilterTest_Platform)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        // give it a platform that is not going to match the existing job platforms.  This should make it not match any products:
        EXPECT_FALSE(m_data->m_connection.GetProductsLikeProductName("someproduct4", AssetDatabaseConnection::StartsWith, resultProducts, AZ::Uuid::CreateNull(), QString(), "badplatform"));
        EXPECT_EQ(resultProducts.size(), 0);

        // give it a correct platform but not one that output that product.
        EXPECT_FALSE(m_data->m_connection.GetProductsLikeProductName("someproduct4", AssetDatabaseConnection::StartsWith, resultProducts, AZ::Uuid::CreateNull(), QString(), "pc"));  // its actually osx
        EXPECT_EQ(resultProducts.size(), 0);

        // give it correct data
        EXPECT_TRUE(m_data->m_connection.GetProductsLikeProductName("someproduct4", AssetDatabaseConnection::StartsWith, resultProducts, AZ::Uuid::CreateNull(), QString(), "osx"));
        EXPECT_EQ(resultProducts.size(), 1);
        EXPECT_EQ(resultProducts[0], m_data->m_product4);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsLikeProductName_FilterTest_Status)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        // give it a correct status but not one that output that product.
        EXPECT_FALSE(m_data->m_connection.GetProductsLikeProductName("someproduct4", AssetDatabaseConnection::StartsWith, resultProducts, AZ::Uuid::CreateNull(), QString(), QString(), AzToolsFramework::AssetSystem::JobStatus::Completed));  // its actually failed
        EXPECT_EQ(resultProducts.size(), 0);

        // give it correct data
        EXPECT_TRUE(m_data->m_connection.GetProductsLikeProductName("someproduct4", AssetDatabaseConnection::StartsWith, resultProducts, AZ::Uuid::CreateNull(), QString(), QString(), AzToolsFramework::AssetSystem::JobStatus::Failed));
        EXPECT_EQ(resultProducts.size(), 1);
        EXPECT_EQ(resultProducts[0], m_data->m_product4);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    // --------------------------------------------------------------------------------------------------------------------
    // --------------------------------------------- GetProductsBySourceID ------------------------------------------------
    // --------------------------------------------------------------------------------------------------------------------

    TEST_F(AssetDatabaseTest, GetProductsBySourceID_NotFound_NoResult)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;
        EXPECT_FALSE(m_data->m_connection.GetProductsBySourceID(-1, resultProducts));
        EXPECT_EQ(resultProducts.size(), 0);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsBySourceID_CorrectID_ReturnsCorrectResult)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;
        EXPECT_TRUE(m_data->m_connection.GetProductsBySourceID(m_data->m_sourceFile1.m_sourceID, resultProducts));
        EXPECT_EQ(resultProducts.size(), 2);
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    // tests all of the filters (beside name) to make sure they all function as expected.
    TEST_F(AssetDatabaseTest, GetProductsBySourceID_FilterTest_BuilderGUID)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        // give it a random builder guid.  This should make it not match any products:
        EXPECT_FALSE(m_data->m_connection.GetProductsBySourceID(m_data->m_sourceFile1.m_sourceID, resultProducts, AZ::Uuid::CreateRandom()));
        EXPECT_EQ(resultProducts.size(), 0);

        // give it a valid data, but the wrong one.  Note that job2 was the one that built the other files, not the sourcefile1.
        EXPECT_FALSE(m_data->m_connection.GetProductsBySourceID(m_data->m_sourceFile1.m_sourceID, resultProducts, m_data->m_job2.m_builderGuid));
        EXPECT_EQ(resultProducts.size(), 0);

        // give it correct data
        EXPECT_TRUE(m_data->m_connection.GetProductsBySourceID(m_data->m_sourceFile1.m_sourceID, resultProducts, m_data->m_job1.m_builderGuid));
        EXPECT_EQ(resultProducts.size(), 2);
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsBySourceID_FilterTest_JobKey)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        // give it invalid data that won't match anything
        EXPECT_FALSE(m_data->m_connection.GetProductsBySourceID(m_data->m_sourceFile1.m_sourceID, resultProducts, AZ::Uuid::CreateNull(), "no matcher"));
        EXPECT_EQ(resultProducts.size(), 0);

        // give it a valid data, but the wrong one:
        EXPECT_FALSE(m_data->m_connection.GetProductsBySourceID(m_data->m_sourceFile1.m_sourceID, resultProducts, AZ::Uuid::CreateNull(), QString(m_data->m_job2.m_jobKey.c_str())));
        EXPECT_EQ(resultProducts.size(), 0);

        // give it correct data
        EXPECT_TRUE(m_data->m_connection.GetProductsBySourceID(m_data->m_sourceFile1.m_sourceID, resultProducts, AZ::Uuid::CreateNull(), QString(m_data->m_job1.m_jobKey.c_str())));
        EXPECT_EQ(resultProducts.size(), 2);
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsBySourceID_FilterTest_Platform)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        // give it invalid data that won't match anything
        EXPECT_FALSE(m_data->m_connection.GetProductsBySourceID(m_data->m_sourceFile1.m_sourceID, resultProducts, AZ::Uuid::CreateNull(), QString(), "badplatform"));
        EXPECT_EQ(resultProducts.size(), 0);

        // give it a valid data, but the wrong one:
        EXPECT_FALSE(m_data->m_connection.GetProductsBySourceID(m_data->m_sourceFile1.m_sourceID, resultProducts, AZ::Uuid::CreateNull(), QString(), "osx"));  // its actually pc
        EXPECT_EQ(resultProducts.size(), 0);

        // give it correct data
        EXPECT_TRUE(m_data->m_connection.GetProductsBySourceID(m_data->m_sourceFile1.m_sourceID, resultProducts, AZ::Uuid::CreateNull(), QString(), "pc"));
        EXPECT_EQ(resultProducts.size(), 2);
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsBySourceID_FilterTest_Status)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        // give it a valid data, but the wrong one:
        EXPECT_FALSE(m_data->m_connection.GetProductsBySourceID(m_data->m_sourceFile1.m_sourceID, resultProducts, AZ::Uuid::CreateNull(), QString(), QString(), AzToolsFramework::AssetSystem::JobStatus::Failed));  // its actually completed
        EXPECT_EQ(resultProducts.size(), 0);

        // give it correct data
        EXPECT_TRUE(m_data->m_connection.GetProductsBySourceID(m_data->m_sourceFile1.m_sourceID, resultProducts, AZ::Uuid::CreateNull(), QString(), QString(), AzToolsFramework::AssetSystem::JobStatus::Completed));
        EXPECT_EQ(resultProducts.size(), 2);
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    // --------------------------------------------------------------------------------------------------------------------
    // -------------------------------------------- GetProductsBySourceName -----------------------------------------------
    // --------------------------------------------------------------------------------------------------------------------

    TEST_F(AssetDatabaseTest, GetProductsBySourceName_EmptyString_NoResult)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;
        EXPECT_FALSE(m_data->m_connection.GetProductsBySourceName(QString(), resultProducts));
        EXPECT_EQ(resultProducts.size(), 0);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsBySourceName_NotFound_NoResult)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;
        EXPECT_FALSE(m_data->m_connection.GetProductsBySourceName("blahrga", resultProducts));
        EXPECT_EQ(resultProducts.size(), 0);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsBySourceName_CorrectName_ReturnsCorrectResult)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;
        EXPECT_TRUE(m_data->m_connection.GetProductsBySourceName("somefile.tif", resultProducts));  // this is source1, which results in product1 and product2 via job1
        EXPECT_EQ(resultProducts.size(), 2);
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    // tests all of the filters (beside name) to make sure they all function as expected.
    TEST_F(AssetDatabaseTest, GetProductsBySourceName_FilterTest_BuilderGUID)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        // give it a random builder guid.  This should make it not match any products:
        EXPECT_FALSE(m_data->m_connection.GetProductsBySourceName("somefile.tif", resultProducts, AZ::Uuid::CreateRandom()));
        EXPECT_EQ(resultProducts.size(), 0);

        // give it a valid data, but the wrong one.  Note that job2 was the one that built the other files, not the sourcefile1.
        EXPECT_FALSE(m_data->m_connection.GetProductsBySourceName("somefile.tif", resultProducts, m_data->m_job2.m_builderGuid));
        EXPECT_EQ(resultProducts.size(), 0);

        // give it correct data
        EXPECT_TRUE(m_data->m_connection.GetProductsBySourceName("somefile.tif", resultProducts, m_data->m_job1.m_builderGuid));
        EXPECT_EQ(resultProducts.size(), 2);
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsBySourceName_FilterTest_JobKey)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        // give it invalid data that won't match anything
        EXPECT_FALSE(m_data->m_connection.GetProductsBySourceName("somefile.tif", resultProducts, AZ::Uuid::CreateNull(), "no matcher"));
        EXPECT_EQ(resultProducts.size(), 0);

        // give it a valid data, but the wrong one:
        EXPECT_FALSE(m_data->m_connection.GetProductsBySourceName("somefile.tif", resultProducts, AZ::Uuid::CreateNull(), QString(m_data->m_job2.m_jobKey.c_str())));
        EXPECT_EQ(resultProducts.size(), 0);

        // give it correct data
        EXPECT_TRUE(m_data->m_connection.GetProductsBySourceName("somefile.tif", resultProducts, AZ::Uuid::CreateNull(), QString(m_data->m_job1.m_jobKey.c_str())));
        EXPECT_EQ(resultProducts.size(), 2);
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsBySourceName_FilterTest_Platform)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        // give it invalid data that won't match anything
        EXPECT_FALSE(m_data->m_connection.GetProductsBySourceName("somefile.tif", resultProducts, AZ::Uuid::CreateNull(), QString(), "badplatform"));
        EXPECT_EQ(resultProducts.size(), 0);

        // give it a valid data, but the wrong one:
        EXPECT_FALSE(m_data->m_connection.GetProductsBySourceName("somefile.tif", resultProducts, AZ::Uuid::CreateNull(), QString(), "osx"));  // its actually pc
        EXPECT_EQ(resultProducts.size(), 0);

        // give it correct data
        EXPECT_TRUE(m_data->m_connection.GetProductsBySourceName("somefile.tif", resultProducts, AZ::Uuid::CreateNull(), QString(), "pc"));
        EXPECT_EQ(resultProducts.size(), 2);
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsBySourceName_FilterTest_Status)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        // give it a valid data, but the wrong one:
        EXPECT_FALSE(m_data->m_connection.GetProductsBySourceName("somefile.tif", resultProducts, AZ::Uuid::CreateNull(), QString(), QString(), AzToolsFramework::AssetSystem::JobStatus::Failed));  // its actually completed
        EXPECT_EQ(resultProducts.size(), 0);

        // give it correct data
        EXPECT_TRUE(m_data->m_connection.GetProductsBySourceName("somefile.tif", resultProducts, AZ::Uuid::CreateNull(), QString(), QString(), AzToolsFramework::AssetSystem::JobStatus::Completed));
        EXPECT_EQ(resultProducts.size(), 2);
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    // --------------------------------------------------------------------------------------------------------------------
    // -------------------------------------------- GetProductsLikeSourceName ---------------------------------------------
    // --------------------------------------------------------------------------------------------------------------------

    TEST_F(AssetDatabaseTest, GetProductsLikeSourceName_EmptyString_NoResult)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;
        EXPECT_FALSE(m_data->m_connection.GetProductsLikeSourceName(QString(), AzToolsFramework::AssetDatabase::AssetDatabaseConnection::Raw, resultProducts));
        EXPECT_EQ(resultProducts.size(), 0);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsLikeSourceName_NotFound_NoResult)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;
        EXPECT_FALSE(m_data->m_connection.GetProductsLikeSourceName("blahrga", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::Raw, resultProducts));
        EXPECT_EQ(resultProducts.size(), 0);

        // this matches the end of a legit string, but we are using startswith, so it should NOT MATCH
        EXPECT_FALSE(m_data->m_connection.GetProductsLikeSourceName("file.tif", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::StartsWith, resultProducts));
        EXPECT_EQ(resultProducts.size(), 0);

        // this matches the startswith, but should NOT MATCH, because its asking for things that end with it.
        EXPECT_FALSE(m_data->m_connection.GetProductsLikeSourceName("somefile", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::EndsWith, resultProducts));
        EXPECT_EQ(resultProducts.size(), 0);

        // make sure invalid tokens do not crash it or something
        EXPECT_FALSE(m_data->m_connection.GetProductsLikeSourceName("%%%%%blahrga%%%%%", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::Matches, resultProducts));
        EXPECT_EQ(resultProducts.size(), 0);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsLikeSourceName_StartsWith_CorrectName_ReturnsCorrectResult)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;
        EXPECT_TRUE(m_data->m_connection.GetProductsLikeSourceName("somefile", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::StartsWith, resultProducts));  // this is source1, which results in product1 and product2 via job1
        EXPECT_EQ(resultProducts.size(), 2);
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }


    TEST_F(AssetDatabaseTest, GetProductsLikeSourceName_EndsWith_CorrectName_ReturnsCorrectResult)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;
        EXPECT_TRUE(m_data->m_connection.GetProductsLikeSourceName("omefile.tif", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::EndsWith, resultProducts));  // this is source1, which results in product1 and product2 via job1
        EXPECT_EQ(resultProducts.size(), 2);
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsLikeSourceName_Matches_CorrectName_ReturnsCorrectResult)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;
        EXPECT_TRUE(m_data->m_connection.GetProductsLikeSourceName("omefile", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::Matches, resultProducts));  // this is source1, which results in product1 and product2 via job1
        EXPECT_EQ(resultProducts.size(), 2);
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    // tests all of the filters (beside name) to make sure they all function as expected.
    TEST_F(AssetDatabaseTest, GetProductsLikeSourceName_FilterTest_BuilderGUID)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        // give it a random builder guid.  This should make it not match any products:
        EXPECT_FALSE(m_data->m_connection.GetProductsLikeSourceName("somefile", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::StartsWith, resultProducts, AZ::Uuid::CreateRandom()));
        EXPECT_EQ(resultProducts.size(), 0);

        // give it a valid data, but the wrong one.  Note that job2 was the one that built the other files, not the sourcefile1.
        EXPECT_FALSE(m_data->m_connection.GetProductsLikeSourceName("somefile", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::StartsWith, resultProducts, m_data->m_job2.m_builderGuid));
        EXPECT_EQ(resultProducts.size(), 0);

        // give it correct data
        EXPECT_TRUE(m_data->m_connection.GetProductsLikeSourceName("somefile", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::StartsWith, resultProducts, m_data->m_job1.m_builderGuid));
        EXPECT_EQ(resultProducts.size(), 2);
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsLikeSourceName_FilterTest_JobKey)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        // give it invalid data that won't match anything
        EXPECT_FALSE(m_data->m_connection.GetProductsLikeSourceName("somefile", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::StartsWith, resultProducts, AZ::Uuid::CreateNull(), "no matcher"));
        EXPECT_EQ(resultProducts.size(), 0);

        // give it a valid data, but the wrong one:
        EXPECT_FALSE(m_data->m_connection.GetProductsLikeSourceName("somefile", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::StartsWith, resultProducts, AZ::Uuid::CreateNull(), QString(m_data->m_job2.m_jobKey.c_str())));
        EXPECT_EQ(resultProducts.size(), 0);

        // give it correct data
        EXPECT_TRUE(m_data->m_connection.GetProductsLikeSourceName("somefile", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::StartsWith, resultProducts, AZ::Uuid::CreateNull(), QString(m_data->m_job1.m_jobKey.c_str())));
        EXPECT_EQ(resultProducts.size(), 2);
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsLikeSourceName_FilterTest_Platform)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        // give it invalid data that won't match anything
        EXPECT_FALSE(m_data->m_connection.GetProductsLikeSourceName("somefile", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::StartsWith, resultProducts, AZ::Uuid::CreateNull(), QString(), "badplatform"));
        EXPECT_EQ(resultProducts.size(), 0);

        // give it a valid data, but the wrong one:
        EXPECT_FALSE(m_data->m_connection.GetProductsLikeSourceName("somefile", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::StartsWith, resultProducts, AZ::Uuid::CreateNull(), QString(), "osx"));  // its actually pc
        EXPECT_EQ(resultProducts.size(), 0);

        // give it correct data
        EXPECT_TRUE(m_data->m_connection.GetProductsLikeSourceName("somefile", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::StartsWith, resultProducts, AZ::Uuid::CreateNull(), QString(), "pc"));
        EXPECT_EQ(resultProducts.size(), 2);
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsLikeSourceName_FilterTest_Status)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        // give it a valid data, but the wrong one:
        EXPECT_FALSE(m_data->m_connection.GetProductsLikeSourceName("somefile", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::StartsWith, resultProducts, AZ::Uuid::CreateNull(), QString(), QString(), AzToolsFramework::AssetSystem::JobStatus::Failed));  // its actually completed
        EXPECT_EQ(resultProducts.size(), 0);

        // give it correct data
        EXPECT_TRUE(m_data->m_connection.GetProductsLikeSourceName("somefile", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::StartsWith, resultProducts, AZ::Uuid::CreateNull(), QString(), QString(), AzToolsFramework::AssetSystem::JobStatus::Completed));
        EXPECT_EQ(resultProducts.size(), 2);
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    // --------------------------------------------------------------------------------------------------------------------
    // -------------------------------------------------- SetProducts  ----------------------------------------------------
    // --------------------------------------------------------------------------------------------------------------------
    TEST_F(AssetDatabaseTest, SetProducts_EmptyList_Fails)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer requestProducts;
        EXPECT_FALSE(m_data->m_connection.SetProducts(requestProducts));
        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, SetProducts_UpdatesProductIDs)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer requestProducts;

        ProductDatabaseEntryContainer resultProducts;
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t priorProductCount = resultProducts.size();

        requestProducts.push_back({ m_data->m_job1.m_jobID, 5, "someproduct5.dds", AZ::Data::AssetType::CreateRandom() });
        requestProducts.push_back({ m_data->m_job1.m_jobID, 6, "someproduct6.dds", AZ::Data::AssetType::CreateRandom() });
        EXPECT_TRUE(m_data->m_connection.SetProducts(requestProducts));

        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t newProductCount = resultProducts.size();

        EXPECT_NE(requestProducts[0].m_productID, AzToolsFramework::AssetDatabase::InvalidEntryId);
        EXPECT_NE(requestProducts[1].m_productID, AzToolsFramework::AssetDatabase::InvalidEntryId);

        EXPECT_EQ(newProductCount, priorProductCount + 2);
        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    // --------------------------------------------------------------------------------------------------------------------
    // ---------------------------------------------- RemoveProduct(s)  ---------------------------------------------------
    // --------------------------------------------------------------------------------------------------------------------
    TEST_F(AssetDatabaseTest, RemoveProduct_InvalidID_Fails_DoesNotCorruptDB)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t priorProductCount = resultProducts.size();

        EXPECT_FALSE(m_data->m_connection.RemoveProduct(-1));

        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t newProductCount = resultProducts.size();

        EXPECT_EQ(newProductCount, priorProductCount);
    }

    TEST_F(AssetDatabaseTest, RemoveProducts_EmptyList_Fails_DoesNotCorruptDB)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t priorProductCount = resultProducts.size();

        resultProducts.clear();
        EXPECT_FALSE(m_data->m_connection.RemoveProducts(resultProducts));

        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t newProductCount = resultProducts.size();

        EXPECT_EQ(newProductCount, priorProductCount);
    }

    TEST_F(AssetDatabaseTest, RemoveProducts_InvalidProductIDs_Fails_DoesNotCorruptDB)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t priorProductCount = resultProducts.size();

        resultProducts.clear();
        resultProducts.push_back({ -1, m_data->m_job1.m_jobID, 5, "someproduct5.dds", AZ::Data::AssetType::CreateRandom() });
        resultProducts.push_back({ -2, m_data->m_job1.m_jobID, 6, "someproduct5.dds", AZ::Data::AssetType::CreateRandom() });
        resultProducts.push_back({ -3, m_data->m_job1.m_jobID, 7, "someproduct5.dds", AZ::Data::AssetType::CreateRandom() });

        EXPECT_FALSE(m_data->m_connection.RemoveProducts(resultProducts));

        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t newProductCount = resultProducts.size();

        EXPECT_EQ(newProductCount, priorProductCount);
    }

    TEST_F(AssetDatabaseTest, RemoveProduct_CorrectProduct_OnlyRemovesThatProduct)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t priorProductCount = resultProducts.size();

        EXPECT_TRUE(m_data->m_connection.RemoveProduct(m_data->m_product1.m_productID));

        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t newProductCount = resultProducts.size();

        EXPECT_EQ(newProductCount, priorProductCount - 1);

        // make sure they're all there except that one.
        EXPECT_EQ(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product3), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product4), resultProducts.end());
    }

    TEST_F(AssetDatabaseTest, RemoveProducts_CorrectProduct_OnlyRemovesThoseProducts)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t priorProductCount = resultProducts.size();

        resultProducts.clear();
        resultProducts.push_back(m_data->m_product1);
        resultProducts.push_back(m_data->m_product3);

        EXPECT_TRUE(m_data->m_connection.RemoveProducts(resultProducts));

        // its also supposed to clear their ids:
        EXPECT_EQ(resultProducts[0].m_productID, AzToolsFramework::AssetDatabase::InvalidEntryId);
        EXPECT_EQ(resultProducts[1].m_productID, AzToolsFramework::AssetDatabase::InvalidEntryId);

        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t newProductCount = resultProducts.size();

        EXPECT_EQ(newProductCount, priorProductCount - 2);

        // make sure they're all there except those two - (1 and 3) which should be 'eq' to end().
        EXPECT_EQ(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());
        EXPECT_EQ(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product3), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product4), resultProducts.end());
    }


    // --------------------------------------------------------------------------------------------------------------------
    // ---------------------------------------------- RemoveProductsByJobID  ---------------------------------------------------
    // --------------------------------------------------------------------------------------------------------------------
    TEST_F(AssetDatabaseTest, RemoveProductsByJobID_InvalidID_Fails_DoesNotCorruptDB)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t priorProductCount = resultProducts.size();

        EXPECT_FALSE(m_data->m_connection.RemoveProductsByJobID(-1));

        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t newProductCount = resultProducts.size();

        EXPECT_EQ(newProductCount, priorProductCount);
    }

    TEST_F(AssetDatabaseTest, RemoveProductsByJobID_ValidID_OnlyRemovesTheMatchingProducts)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t priorProductCount = resultProducts.size();

        EXPECT_TRUE(m_data->m_connection.RemoveProductsByJobID(m_data->m_job1.m_jobID));

        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t newProductCount = resultProducts.size();

        EXPECT_EQ(newProductCount, priorProductCount - 2);

        // both products that belong to the first job should be gone.
        EXPECT_EQ(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_EQ(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product3), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product4), resultProducts.end());
    }


    // --------------------------------------------------------------------------------------------------------------------
    // ------------------------------------------ RemoveProductsBySourceID ------------------------------------------------
    // --------------------------------------------------------------------------------------------------------------------

    TEST_F(AssetDatabaseTest, RemoveProductsBySourceID_InvalidSourceID_NoResults)
    {
        CreateCoverageTestData();
        ProductDatabaseEntryContainer resultProducts;

        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t priorProductCount = resultProducts.size();

        EXPECT_FALSE(m_data->m_connection.RemoveProductsBySourceID(-1));

        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t newProductCount = resultProducts.size();
        EXPECT_EQ(newProductCount, priorProductCount);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, RemoveProductsBySourceID_Valid_OnlyRemovesTheCorrectOnes)
    {
        CreateCoverageTestData();
        ProductDatabaseEntryContainer resultProducts;

        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t priorProductCount = resultProducts.size();

        EXPECT_TRUE(m_data->m_connection.RemoveProductsBySourceID(m_data->m_sourceFile1.m_sourceID));

        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t newProductCount = resultProducts.size();
        EXPECT_EQ(newProductCount, priorProductCount - 2);

        // both products that belong to the first source should be gone - but only those!
        EXPECT_EQ(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_EQ(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product3), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product4), resultProducts.end());

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    // tests all of the filters (beside name) to make sure they all function as expected.
    TEST_F(AssetDatabaseTest, RemoveProductsBySourceID_FilterTest_BuilderGUID)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t priorProductCount = resultProducts.size();

        // give it a non-matching builder UUID - it should not delete anything despite the product sourceId being correct.
        EXPECT_FALSE(m_data->m_connection.RemoveProductsBySourceID(m_data->m_sourceFile1.m_sourceID, AZ::Uuid::CreateRandom()));
        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t newProductCount = resultProducts.size();
        EXPECT_EQ(newProductCount, priorProductCount);

        // give it a correct data but the wrong builder (a valid, but wrong one)
        EXPECT_FALSE(m_data->m_connection.RemoveProductsBySourceID(m_data->m_sourceFile1.m_sourceID, m_data->m_job2.m_builderGuid));
        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        newProductCount = resultProducts.size();
        EXPECT_EQ(newProductCount, priorProductCount);

        // give it correct data, it should delete the first two products:
        EXPECT_TRUE(m_data->m_connection.RemoveProductsBySourceID(m_data->m_sourceFile1.m_sourceID, m_data->m_job1.m_builderGuid));
        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        newProductCount = resultProducts.size();
        EXPECT_EQ(newProductCount, priorProductCount - 2);

        // both products that belong to the first source should be gone - but only those!
        EXPECT_EQ(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_EQ(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product3), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product4), resultProducts.end());

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, RemoveProductsBySourceID_FilterTest_JobKey)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t priorProductCount = resultProducts.size();

        // give it a non-matching job key - it should not delete anything despite the product sourceId being correct.
        EXPECT_FALSE(m_data->m_connection.RemoveProductsBySourceID(m_data->m_sourceFile1.m_sourceID, AZ::Uuid::CreateNull(), "random key that wont match"));
        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t newProductCount = resultProducts.size();
        EXPECT_EQ(newProductCount, priorProductCount);

        // give it a correct data but the wrong builder (a valid, but wrong one)
        EXPECT_FALSE(m_data->m_connection.RemoveProductsBySourceID(m_data->m_sourceFile1.m_sourceID, AZ::Uuid::CreateNull(), m_data->m_job2.m_jobKey.c_str())); // job2 is not the one that did sourcefile1
        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        newProductCount = resultProducts.size();
        EXPECT_EQ(newProductCount, priorProductCount);

        // give it correct data, it should delete the first two products:
        EXPECT_TRUE(m_data->m_connection.RemoveProductsBySourceID(m_data->m_sourceFile1.m_sourceID, AZ::Uuid::CreateNull(), m_data->m_job1.m_jobKey.c_str()));
        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        newProductCount = resultProducts.size();
        EXPECT_EQ(newProductCount, priorProductCount - 2);

        // both products that belong to the first source should be gone - but only those!
        EXPECT_EQ(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_EQ(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product3), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product4), resultProducts.end());

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, RemoveProductsBySourceID_FilterTest_Platform)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t priorProductCount = resultProducts.size();

        // give it a non-matching job key - it should not delete anything despite the product sourceId being correct.
        EXPECT_FALSE(m_data->m_connection.RemoveProductsBySourceID(m_data->m_sourceFile1.m_sourceID, AZ::Uuid::CreateNull(), QString(), "no such platform"));
        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t newProductCount = resultProducts.size();
        EXPECT_EQ(newProductCount, priorProductCount);

        // give it a correct data but the wrong builder (a valid, but wrong one)
        EXPECT_FALSE(m_data->m_connection.RemoveProductsBySourceID(m_data->m_sourceFile1.m_sourceID, AZ::Uuid::CreateNull(), QString(), "osx")); // its actually PC
        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        newProductCount = resultProducts.size();
        EXPECT_EQ(newProductCount, priorProductCount);

        // give it correct data, it should delete the first two products:
        EXPECT_TRUE(m_data->m_connection.RemoveProductsBySourceID(m_data->m_sourceFile1.m_sourceID, AZ::Uuid::CreateNull(), QString(), "pc"));
        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        newProductCount = resultProducts.size();
        EXPECT_EQ(newProductCount, priorProductCount - 2);

        // both products that belong to the first source should be gone - but only those!
        EXPECT_EQ(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_EQ(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product3), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product4), resultProducts.end());

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, RemoveProductsBySourceID_FilterTest_Status)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t priorProductCount = resultProducts.size();

        // give it a correct status but not one that output that product.
        EXPECT_FALSE(m_data->m_connection.RemoveProductsBySourceID(m_data->m_sourceFile1.m_sourceID, AZ::Uuid::CreateNull(), QString(), QString(), AzToolsFramework::AssetSystem::JobStatus::Failed));

        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t newProductCount = resultProducts.size();
        EXPECT_EQ(newProductCount, priorProductCount);

        // give it correct data
        EXPECT_TRUE(m_data->m_connection.RemoveProductsBySourceID(m_data->m_sourceFile1.m_sourceID, AZ::Uuid::CreateNull(), QString(), QString(), AzToolsFramework::AssetSystem::JobStatus::Completed));

        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        newProductCount = resultProducts.size();
        EXPECT_EQ(newProductCount, priorProductCount - 2);

        // both products that belong to the first source should be gone - but only those!
        EXPECT_EQ(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_EQ(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product3), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product4), resultProducts.end());

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, SetProductDependencies_CorrectnessTest)
    {
        CreateCoverageTestData();
        ProductDatabaseEntryContainer resultProducts;

        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        EXPECT_GT(resultProducts.size(), 0);

        ProductDependencyDatabaseEntryContainer productDependencies;
        AZStd::bitset<64> dependencyFlags = 0xFAA0FEEE;
        AZStd::string pathDep = "unresolved/dependency.txt";
        AZStd::string platform = "somePlatform";

        productDependencies.reserve(200);

        // make 100 product dependencies on the first productID
        for (AZ::u32 productIndex = 0; productIndex < 100; ++productIndex)
        {
            ProductDependencyDatabaseEntry entry(resultProducts[0].m_productID, m_data->m_sourceFile1.m_sourceGuid, productIndex, dependencyFlags, platform, true, pathDep);
            productDependencies.emplace_back(AZStd::move(entry));
        }

        // make 100 product dependencies on the second productID
        for (AZ::u32 productIndex = 0; productIndex < 100; ++productIndex)
        {
            ProductDependencyDatabaseEntry entry(resultProducts[1].m_productID, m_data->m_sourceFile2.m_sourceGuid, productIndex, dependencyFlags, platform, true, pathDep);
            productDependencies.emplace_back(AZStd::move(entry));
        }

        // do a bulk insert:
        EXPECT_TRUE(m_data->m_connection.SetProductDependencies(productDependencies));

        // now, read all the data back and verify each field:
        productDependencies.clear();

        // searching for the first product should only result in the first 100 results:
        EXPECT_TRUE(m_data->m_connection.GetProductDependenciesByProductID(resultProducts[0].m_productID, productDependencies));
        EXPECT_EQ(productDependencies.size(), 100);

        for (AZ::u32 productIndex = 0; productIndex < 100; ++productIndex)
        {
            EXPECT_NE(productDependencies[productIndex].m_productDependencyID, AzToolsFramework::AssetDatabase::InvalidEntryId);
            EXPECT_EQ(productDependencies[productIndex].m_productPK, resultProducts[0].m_productID);
            EXPECT_EQ(productDependencies[productIndex].m_dependencySourceGuid, m_data->m_sourceFile1.m_sourceGuid);
            EXPECT_EQ(productDependencies[productIndex].m_dependencySubID, productIndex);
            EXPECT_EQ(productDependencies[productIndex].m_dependencyFlags, dependencyFlags);
            EXPECT_EQ(productDependencies[productIndex].m_platform, platform);
            EXPECT_EQ(productDependencies[productIndex].m_unresolvedPath, pathDep);
        }

        productDependencies.clear();

        // searching for the second product should only result in the second 100 results:
        EXPECT_TRUE(m_data->m_connection.GetProductDependenciesByProductID(resultProducts[1].m_productID, productDependencies));
        EXPECT_EQ(productDependencies.size(), 100);

        for (AZ::u32 productIndex = 0; productIndex < 100; ++productIndex)
        {
            EXPECT_NE(productDependencies[productIndex].m_productDependencyID, AzToolsFramework::AssetDatabase::InvalidEntryId);
            EXPECT_EQ(productDependencies[productIndex].m_productPK, resultProducts[1].m_productID);
            EXPECT_EQ(productDependencies[productIndex].m_dependencySourceGuid, m_data->m_sourceFile2.m_sourceGuid);
            EXPECT_EQ(productDependencies[productIndex].m_dependencySubID, productIndex);
            EXPECT_EQ(productDependencies[productIndex].m_dependencyFlags, dependencyFlags);
            EXPECT_EQ(productDependencies[productIndex].m_platform, platform);
            EXPECT_EQ(productDependencies[productIndex].m_unresolvedPath, pathDep);
        }

        // now, we replace the dependencies of the first product with fewer results, with different data:
        productDependencies.clear();
        for (AZ::u32 productIndex = 0; productIndex < 50; ++productIndex)
        {
            ProductDependencyDatabaseEntry entry(resultProducts[0].m_productID, m_data->m_sourceFile2.m_sourceGuid, productIndex, dependencyFlags, platform, true);
            productDependencies.emplace_back(AZStd::move(entry));
        }

        EXPECT_TRUE(m_data->m_connection.SetProductDependencies(productDependencies));

        // searching for the first product should only result in 50 results, which proves that the original 100 were replaced with the new entries:
        productDependencies.clear();
        EXPECT_TRUE(m_data->m_connection.GetProductDependenciesByProductID(resultProducts[0].m_productID, productDependencies));
        EXPECT_EQ(productDependencies.size(), 50);

        for (AZ::u32 productIndex = 0; productIndex < 50; ++productIndex)
        {
            EXPECT_NE(productDependencies[productIndex].m_productDependencyID, AzToolsFramework::AssetDatabase::InvalidEntryId);
            EXPECT_EQ(productDependencies[productIndex].m_productPK, resultProducts[0].m_productID);
            EXPECT_EQ(productDependencies[productIndex].m_dependencySourceGuid, m_data->m_sourceFile2.m_sourceGuid); // here we verify that the field has changed.
            EXPECT_EQ(productDependencies[productIndex].m_dependencySubID, productIndex);
            EXPECT_EQ(productDependencies[productIndex].m_dependencyFlags, dependencyFlags);
            EXPECT_EQ(productDependencies[productIndex].m_platform, platform);
            EXPECT_EQ(productDependencies[productIndex].m_unresolvedPath, ""); // verify that no path is set if it was not specified in the entry

        }

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, AddLargeNumberOfDependencies_PerformanceTest)
    {
        CreateCoverageTestData();
        ProductDatabaseEntryContainer resultProducts;

        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        EXPECT_GT(resultProducts.size(), 0);

        ProductDependencyDatabaseEntryContainer productDependencies;
        AZStd::bitset<64> dependencyFlags;
        AZStd::string platform;

        productDependencies.reserve(20000);

        for (AZ::u32 productIndex = 0; productIndex < 20000; ++productIndex)
        {
            ProductDependencyDatabaseEntry entry(resultProducts[0].m_productID, m_data->m_sourceFile1.m_sourceGuid, productIndex, dependencyFlags, platform, true);
            productDependencies.emplace_back(AZStd::move(entry));
        }
        EXPECT_TRUE(m_data->m_connection.SetProductDependencies(productDependencies));

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, MissingDependencyTable_WriteAndReadMissingDependencyByDependencyId_ResultsMatch)
    {
        CreateCoverageTestData();

        // Use a non-zero sub ID to verify it writes and reads correctly.
        AZ::Data::AssetId assetId(AZ::Uuid::CreateString("{12209A94-AF18-44BB-8A62-96F35291B2E1}"), 3);
        AzToolsFramework::AssetDatabase::MissingProductDependencyDatabaseEntry writeMissingDependency(
            // The product ID is a link to another table, it will fail to write this entry if this is invalid.
            m_data->m_product1.m_productID,
            "Scanner Name",
            "1.0.0",
            "Source File Fingerprint",
            assetId.m_guid,
            assetId.m_subId,
            "Source String",
            "last Scan Time",
            0);
        EXPECT_TRUE(m_data->m_connection.SetMissingProductDependency(writeMissingDependency));

        AzToolsFramework::AssetDatabase::MissingProductDependencyDatabaseEntry readMissingDependency;
        EXPECT_TRUE(m_data->m_connection.GetMissingProductDependencyByMissingProductDependencyId(
            writeMissingDependency.m_missingProductDependencyId,
            readMissingDependency));

        EXPECT_EQ(writeMissingDependency, readMissingDependency);
    }

    TEST_F(AssetDatabaseTest, MissingDependencyTable_UpdateExistingMissingDependencyByDependencyId_ResultsMatch)
    {
        CreateCoverageTestData();

        // Use a non-zero sub ID to verify it writes and reads correctly.
        AZ::Data::AssetId assetId(AZ::Uuid::CreateString("{32C32642-5832-4997-A478-F288C734425D}"), 6);
        AzToolsFramework::AssetDatabase::MissingProductDependencyDatabaseEntry originalMissingDependency(
            // The product ID is a link to another table, it will fail to write this entry if this is invalid.
            m_data->m_product3.m_productID,
            "Scanner Name",
            "1.0.0",
            "Source File Fingerprint",
            assetId.m_guid,
            assetId.m_subId,
            "Source String",
            "last Scan Time",
            0);
        EXPECT_TRUE(m_data->m_connection.SetMissingProductDependency(originalMissingDependency));

        AzToolsFramework::AssetDatabase::MissingProductDependencyDatabaseEntry readMissingDependency;
        EXPECT_TRUE(m_data->m_connection.GetMissingProductDependencyByMissingProductDependencyId(
            originalMissingDependency.m_missingProductDependencyId,
            readMissingDependency));

        EXPECT_EQ(originalMissingDependency, readMissingDependency);

        // Test each field separately.
        AzToolsFramework::AssetDatabase::MissingProductDependencyDatabaseEntry updatedMissingDependency(originalMissingDependency);

        updatedMissingDependency.m_productPK = m_data->m_product1.m_productID;
        SetAndCheckMissingDependency(updatedMissingDependency, originalMissingDependency);

        updatedMissingDependency.m_scannerId = "Different Scanner Name";
        SetAndCheckMissingDependency(updatedMissingDependency, originalMissingDependency);

        updatedMissingDependency.m_scannerVersion = "Different Scanner Version";
        SetAndCheckMissingDependency(updatedMissingDependency, originalMissingDependency);

        updatedMissingDependency.m_sourceFileFingerprint = "Different Fingerprint";
        SetAndCheckMissingDependency(updatedMissingDependency, originalMissingDependency);

        updatedMissingDependency.m_dependencySourceGuid = AZ::Uuid::CreateString("{6C3ED7B4-E6F1-4163-9141-54F5DC1D9C35}");
        SetAndCheckMissingDependency(updatedMissingDependency, originalMissingDependency);

        updatedMissingDependency.m_dependencySubId = 3;
        SetAndCheckMissingDependency(updatedMissingDependency, originalMissingDependency);

        updatedMissingDependency.m_missingDependencyString = "Different Source String";
        SetAndCheckMissingDependency(updatedMissingDependency, originalMissingDependency);

        updatedMissingDependency.m_lastScanTime = "Different Scan Time";
        SetAndCheckMissingDependency(updatedMissingDependency, originalMissingDependency);

        updatedMissingDependency.m_scanTimeSecondsSinceEpoch = 1;
        SetAndCheckMissingDependency(updatedMissingDependency, originalMissingDependency);
    }

    void AssetDatabaseTest::SetAndCheckMissingDependency(
        AzToolsFramework::AssetDatabase::MissingProductDependencyDatabaseEntry& updatedMissingDependency,
        const AzToolsFramework::AssetDatabase::MissingProductDependencyDatabaseEntry& originalMissingDependency)
    {
        EXPECT_TRUE(m_data->m_connection.SetMissingProductDependency(updatedMissingDependency));

        AzToolsFramework::AssetDatabase::MissingProductDependencyDatabaseEntry readMissingDependency;
        EXPECT_TRUE(m_data->m_connection.GetMissingProductDependencyByMissingProductDependencyId(
            originalMissingDependency.m_missingProductDependencyId,
            readMissingDependency));
        EXPECT_EQ(updatedMissingDependency, readMissingDependency);
        // MissingProductDependencyDatabaseEntry doesn't override the != operator
        EXPECT_FALSE(readMissingDependency == originalMissingDependency);
    }

    TEST_F(AssetDatabaseTest, MissingDependencyTable_WriteAndReadMissingDependenciesByProductId_ResultsMatch)
    {
        CreateCoverageTestData();

        AZStd::vector<AZ::Data::AssetId> assetIds;
        assetIds.push_back(AZ::Data::AssetId(AZ::Uuid::CreateString("{FDAC3A8C-26D1-47D9-88B0-647BCED826DB}"), 10));
        assetIds.push_back(AZ::Data::AssetId(AZ::Uuid::CreateString("{261E8996-7309-4D18-986F-EC6EDE910A70}"), 20));
        assetIds.push_back(AZ::Data::AssetId(AZ::Uuid::CreateString("{2FA88E3A-D6E4-4192-B865-4DDD61AE7492}"), 30));
        // The product ID is a link to another table, it will fail to write this entry if this is invalid.
        AZ::s64 productPK = m_data->m_product2.m_productID;

        AzToolsFramework::AssetDatabase::MissingProductDependencyDatabaseEntryContainer writeMissingDependencies;
        writeMissingDependencies.push_back(AzToolsFramework::AssetDatabase::MissingProductDependencyDatabaseEntry(
            productPK,
            "Scanner 0",
            "0.0.0",
            "Fingerprint 0",
            assetIds[0].m_guid,
            assetIds[0].m_subId,
            "Source String 0",
            "last Scan Time 0",
            0));
        writeMissingDependencies.push_back(AzToolsFramework::AssetDatabase::MissingProductDependencyDatabaseEntry(
            productPK,
            "Scanner 1",
            "1.1.1",
            "Fingerprint 1",
            assetIds[1].m_guid,
            assetIds[1].m_subId,
            "Source String 1",
            "last Scan Time 1",
            1));
        writeMissingDependencies.push_back(AzToolsFramework::AssetDatabase::MissingProductDependencyDatabaseEntry(
            productPK,
            "Scanner 2",
            "2.2.2",
            "Fingerprint 2",
            assetIds[2].m_guid,
            assetIds[2].m_subId,
            "Source String 2",
            "last Scan Time 2",
            2));
        EXPECT_TRUE(m_data->m_connection.SetMissingProductDependency(writeMissingDependencies[0]));
        EXPECT_TRUE(m_data->m_connection.SetMissingProductDependency(writeMissingDependencies[1]));
        EXPECT_TRUE(m_data->m_connection.SetMissingProductDependency(writeMissingDependencies[2]));

        AzToolsFramework::AssetDatabase::MissingProductDependencyDatabaseEntryContainer readMissingDependencies;
        EXPECT_TRUE(m_data->m_connection.GetMissingProductDependenciesByProductId(productPK, readMissingDependencies));

        EXPECT_EQ(readMissingDependencies.size(), writeMissingDependencies.size());
        for (int dependencyIndex = 0; dependencyIndex < writeMissingDependencies.size(); ++dependencyIndex)
        {
            EXPECT_EQ(readMissingDependencies[dependencyIndex], writeMissingDependencies[dependencyIndex]);
        }
    }

    TEST_F(AssetDatabaseTest, MissingDependencyTable_WriteAndDeleteMissingDependencyByDependencyId_MissingDependencyRecordDeleted)
    {
        CreateCoverageTestData();

        // Use a non-zero sub ID to verify it writes and reads correctly.
        AZ::Data::AssetId assetId(AZ::Uuid::CreateString("{12209A94-AF18-44BB-8A62-96F35291B2E1}"), 3);
        AzToolsFramework::AssetDatabase::MissingProductDependencyDatabaseEntry writeMissingDependency(
            // The product ID is a link to another table, it will fail to write this entry if this is invalid.
            m_data->m_product1.m_productID,
            "Scanner Name",
            "1.0.0",
            "Source File Fingerprint",
            assetId.m_guid,
            assetId.m_subId,
            "Source String",
            "last Scan Time",
            0);
        EXPECT_TRUE(m_data->m_connection.SetMissingProductDependency(writeMissingDependency));

        AzToolsFramework::AssetDatabase::MissingProductDependencyDatabaseEntry readMissingDependency;
        EXPECT_TRUE(m_data->m_connection.GetMissingProductDependencyByMissingProductDependencyId(
            writeMissingDependency.m_missingProductDependencyId,
            readMissingDependency));

        // Verify that it was written to the DB before erasing it.
        EXPECT_EQ(writeMissingDependency, readMissingDependency);

        EXPECT_TRUE(m_data->m_connection.DeleteMissingProductDependencyByProductId(writeMissingDependency.m_missingProductDependencyId));

        EXPECT_FALSE(m_data->m_connection.GetMissingProductDependencyByMissingProductDependencyId(
            writeMissingDependency.m_missingProductDependencyId,
            readMissingDependency));
    }

    // Verify that clearing missing product dependencies by product ID clears every missing dependency for that product ID.
    TEST_F(AssetDatabaseTest, MissingDependencyTable_DeleteMultipleMissingDependenciesForOneProduct_MissingDependencyRecordsDeleted)
    {
        CreateCoverageTestData();

        // Use a non-zero sub ID to verify it writes and reads correctly.
        AZ::Data::AssetId assetId(AZ::Uuid::CreateString("{12209A94-AF18-44BB-8A62-96F35291B2E1}"), 3);
        AzToolsFramework::AssetDatabase::MissingProductDependencyDatabaseEntry firstMissingDependency(
            // The product ID is a link to another table, it will fail to write this entry if this is invalid.
            m_data->m_product1.m_productID,
            "Scanner Name",
            "1.0.0",
            "Source File Fingerprint",
            assetId.m_guid,
            assetId.m_subId,
            "Source String",
            "last Scan Time",
            0);
        EXPECT_TRUE(m_data->m_connection.SetMissingProductDependency(firstMissingDependency));

        AZ::Data::AssetId secondMissingAssetId(AZ::Uuid::CreateString("{12209A94-FFFF-FFFF-8A62-96F35291B2E1}"), 4);
        AzToolsFramework::AssetDatabase::MissingProductDependencyDatabaseEntry secondMissingDependency(
            // Use the same product ID as the first missing dependency
            m_data->m_product1.m_productID,
            "Scanner Name 2",
            "1.0.0",
            "Source File Fingerprint",
            secondMissingAssetId.m_guid,
            secondMissingAssetId.m_subId,
            "Source String",
            "last Scan Time",
            0);
        EXPECT_TRUE(m_data->m_connection.SetMissingProductDependency(secondMissingDependency));

        AZStd::unordered_set<AZ::s64> expectedMissingDependencies;
        expectedMissingDependencies.insert(firstMissingDependency.m_missingProductDependencyId);
        expectedMissingDependencies.insert(secondMissingDependency.m_missingProductDependencyId);
        // Tests can't be ran inside the lambda, so cache results and check after.
        bool foundUnexpectedDependency = false;
        m_data->m_connection.QueryMissingProductDependencyByProductId(
            m_data->m_product1.m_productID,
            [&](AzToolsFramework::AssetDatabase::MissingProductDependencyDatabaseEntry& entry)
        {
            if (expectedMissingDependencies.find(entry.m_missingProductDependencyId) == expectedMissingDependencies.end())
            {
                foundUnexpectedDependency = true;
            }
            else
            {
                expectedMissingDependencies.erase(entry.m_missingProductDependencyId);
            }
            return true;
        });
        EXPECT_FALSE(foundUnexpectedDependency);
        EXPECT_EQ(expectedMissingDependencies.size(), 0);

        EXPECT_TRUE(m_data->m_connection.DeleteMissingProductDependencyByProductId(m_data->m_product1.m_productID));

        foundUnexpectedDependency = false;
        m_data->m_connection.QueryMissingProductDependencyByProductId(
            m_data->m_product1.m_productID,
            [&](AzToolsFramework::AssetDatabase::MissingProductDependencyDatabaseEntry& /*entry*/)
        {
            // No dependencies should be found for this product.
            foundUnexpectedDependency = true;
            return false;
        });
        EXPECT_FALSE(foundUnexpectedDependency);
    }

    // Verify that clearing missing dependencies for one product ID does not clear it for another product ID.
    TEST_F(AssetDatabaseTest, MissingDependencyTable_DeleteMissingDependenciesForOneProduct_MissingDependenciesNotDeletedForOtherProduct)
    {
        CreateCoverageTestData();

        // Use a non-zero sub ID to verify it writes and reads correctly.
        AZ::Data::AssetId assetId(AZ::Uuid::CreateString("{12209A94-AF18-44BB-8A62-96F35291B2E1}"), 3);
        AzToolsFramework::AssetDatabase::MissingProductDependencyDatabaseEntry firstMissingDependency(
            // The product ID is a link to another table, it will fail to write this entry if this is invalid.
            m_data->m_product1.m_productID,
            "Scanner Name",
            "1.0.0",
            "Source File Fingerprint",
            assetId.m_guid,
            assetId.m_subId,
            "Source String",
            "last Scan Time",
            0);
        EXPECT_TRUE(m_data->m_connection.SetMissingProductDependency(firstMissingDependency));

        AZ::Data::AssetId secondMissingAssetId(AZ::Uuid::CreateString("{12209A94-FFFF-FFFF-8A62-96F35291B2E1}"), 4);
        AzToolsFramework::AssetDatabase::MissingProductDependencyDatabaseEntry secondMissingDependency(
            // Use the same product ID as the first missing dependency
            m_data->m_product3.m_productID,
            "Scanner Name 2",
            "1.0.0",
            "Source File Fingerprint",
            secondMissingAssetId.m_guid,
            secondMissingAssetId.m_subId,
            "Source String",
            "last Scan Time",
            0);
        EXPECT_TRUE(m_data->m_connection.SetMissingProductDependency(secondMissingDependency));

        // Verify both missing dependencies are set.
        size_t dependenciesFound = 0;
        m_data->m_connection.QueryMissingProductDependencyByProductId(
            m_data->m_product1.m_productID,
            [&](AzToolsFramework::AssetDatabase::MissingProductDependencyDatabaseEntry& /*entry*/)
        {
            ++dependenciesFound;
            return true;
        });
        EXPECT_EQ(dependenciesFound, 1);

        dependenciesFound = 0;
        m_data->m_connection.QueryMissingProductDependencyByProductId(
            m_data->m_product3.m_productID,
            [&](AzToolsFramework::AssetDatabase::MissingProductDependencyDatabaseEntry& /*entry*/)
        {
            ++dependenciesFound;
            return true;
        });
        EXPECT_EQ(dependenciesFound, 1);

        // Delete the first product's missing dependencies.
        EXPECT_TRUE(m_data->m_connection.DeleteMissingProductDependencyByProductId(m_data->m_product1.m_productID));

        // Verify the first product's missing dependency is gone.
        dependenciesFound = 0;
        m_data->m_connection.QueryMissingProductDependencyByProductId(
            m_data->m_product1.m_productID,
            [&](AzToolsFramework::AssetDatabase::MissingProductDependencyDatabaseEntry& /*entry*/)
        {
            ++dependenciesFound;
            return false;
        });
        // No dependencies should be found for this product.
        EXPECT_EQ(dependenciesFound, 0);

        // Verify the second product's missing dependency is still there.
        dependenciesFound = 0;
        m_data->m_connection.QueryMissingProductDependencyByProductId(
            m_data->m_product3.m_productID,
            [&](AzToolsFramework::AssetDatabase::MissingProductDependencyDatabaseEntry& /*entry*/)
        {
            ++dependenciesFound;
            return true;
        });
        EXPECT_EQ(dependenciesFound, 1);
    }

    TEST_F(AssetDatabaseTest, AddLargeNumberOfSourceDependencies_PerformanceTest)
    {
        CreateCoverageTestData();
        SourceFileDependencyEntryContainer resultSourceDependencies;

        resultSourceDependencies.reserve(20000);
        AZ::Uuid builderGuid = AZ::Uuid::CreateRandom();

        // emit 20,000 source dependencies for the same origin file:
        AZ::Uuid originUuid{ "{3C1C9062-7246-443A-A6DF-A001D31B941A}" };

        for (AZ::u32 sourceIndex = 0; sourceIndex < 20000; ++sourceIndex)
        {
            AZStd::string dependentFile = AZStd::string::format("otherfile%i.txt", sourceIndex);
            SourceFileDependencyEntry entry(builderGuid, originUuid, PathOrUuid(dependentFile), SourceFileDependencyEntry::DEP_SourceToSource, true, "");
            resultSourceDependencies.emplace_back(AZStd::move(entry));
        }

        EXPECT_TRUE(m_data->m_connection.SetSourceFileDependencies(resultSourceDependencies));

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this

        // read them back
        resultSourceDependencies.clear();
        EXPECT_TRUE(m_data->m_connection.GetSourceFileDependenciesByBuilderGUIDAndSource(builderGuid, originUuid, SourceFileDependencyEntry::DEP_SourceToSource, resultSourceDependencies));
        EXPECT_EQ(resultSourceDependencies.size(), 20000);
    }

    TEST_F(AssetDatabaseTest, SourceFileDependencies_CorrectnessTest)
    {
        CreateCoverageTestData();
        AZ::Uuid builderGuid1 = AZ::Uuid::CreateRandom();
        AZ::Uuid builderGuid2 = AZ::Uuid::CreateRandom();

        AZ::Uuid file1Uuid{ "{5AA73EF6-5E14-41F3-B458-4FA19D495696}" };
        AZ::Uuid file2Uuid{ "{A3FF1BD5-7D6F-4241-8398-1DC6239AD97A}" };
        AZ::Uuid file1DependsOn1Uuid{ "{33338E41-985A-40DF-A1CC-87BDBC17EC7A}" };

        SourceFileDependencyEntryContainer entries;

        // add the two different kinds of dependencies.
        entries.push_back(SourceFileDependencyEntry(builderGuid1, file1Uuid, PathOrUuid("file1dependson1.txt"), SourceFileDependencyEntry::DEP_SourceToSource, true, ""));
        entries.push_back(SourceFileDependencyEntry(builderGuid2, file1Uuid, PathOrUuid("file1dependson2.txt"), SourceFileDependencyEntry::DEP_SourceToSource, true, ""));
        entries.push_back(SourceFileDependencyEntry(builderGuid1, file1Uuid, PathOrUuid("file1dependson1job.txt"), SourceFileDependencyEntry::DEP_JobToJob, true, ""));
        entries.push_back(SourceFileDependencyEntry(builderGuid2, file1Uuid, PathOrUuid("file1dependson2job.txt"), SourceFileDependencyEntry::DEP_JobToJob, true, ""));

        entries.push_back(SourceFileDependencyEntry(builderGuid1, file2Uuid, PathOrUuid("file2dependson1.txt"), SourceFileDependencyEntry::DEP_SourceToSource, true, ""));
        entries.push_back(SourceFileDependencyEntry(builderGuid1, file2Uuid, PathOrUuid("file2dependson1job.txt"), SourceFileDependencyEntry::DEP_JobToJob, true, ""));

        ASSERT_TRUE(m_data->m_connection.SetSourceFileDependencies(entries));

        SourceFileDependencyEntryContainer resultEntries;

        AZStd::string searchFor;
        auto SearchPredicate = [&searchFor](const SourceFileDependencyEntry& element)
        {
            return element.m_dependsOnSource.GetPath() == searchFor;
        };

        AZ::Uuid searchUuid;
        auto SearchPredicateReverse = [&searchUuid](const SourceFileDependencyEntry& element)
        {
            return element.m_sourceGuid == searchUuid;
        };

        // ask for only the source-to-source dependencies of file1.txt for builder1
        EXPECT_TRUE(m_data->m_connection.GetSourceFileDependenciesByBuilderGUIDAndSource(builderGuid1, file1Uuid, SourceFileDependencyEntry::DEP_SourceToSource, resultEntries));
        EXPECT_EQ(resultEntries.size(), 1);
        searchFor = "file1dependson1.txt";
        EXPECT_NE(AZStd::find_if(resultEntries.begin(), resultEntries.end(), SearchPredicate), resultEntries.end());
        resultEntries.clear();

        // ask for only the source-to-source dependencies of file1.txt for builder2
        EXPECT_TRUE(m_data->m_connection.GetSourceFileDependenciesByBuilderGUIDAndSource(builderGuid2, file1Uuid, SourceFileDependencyEntry::DEP_SourceToSource, resultEntries));
        EXPECT_EQ(resultEntries.size(), 1);
        searchFor = "file1dependson2.txt";
        EXPECT_NE(AZStd::find_if(resultEntries.begin(), resultEntries.end(), SearchPredicate), resultEntries.end());
        resultEntries.clear();

        // ask for the source-to-source dependencies of file1.txt for ANY builder, we shiould get both.
        EXPECT_TRUE(m_data->m_connection.GetDependsOnSourceBySource(file1Uuid, SourceFileDependencyEntry::DEP_SourceToSource, resultEntries));
        EXPECT_EQ(resultEntries.size(), 2);
        searchFor = "file1dependson1.txt";
        EXPECT_NE(AZStd::find_if(resultEntries.begin(), resultEntries.end(), SearchPredicate), resultEntries.end());
        searchFor = "file1dependson2.txt";
        EXPECT_NE(AZStd::find_if(resultEntries.begin(), resultEntries.end(), SearchPredicate), resultEntries.end());
        resultEntries.clear();

        // now ask for the job-to-job dependencies for builder 1
        EXPECT_TRUE(m_data->m_connection.GetSourceFileDependenciesByBuilderGUIDAndSource(builderGuid1, file1Uuid, SourceFileDependencyEntry::DEP_JobToJob, resultEntries));
        EXPECT_EQ(resultEntries.size(), 1);
        searchFor = "file1dependson1job.txt";
        EXPECT_NE(AZStd::find_if(resultEntries.begin(), resultEntries.end(), SearchPredicate), resultEntries.end());
        resultEntries.clear();

        // now ask for the job-to-job dependencies for builder 2
        EXPECT_TRUE(m_data->m_connection.GetSourceFileDependenciesByBuilderGUIDAndSource(builderGuid2, file1Uuid, SourceFileDependencyEntry::DEP_JobToJob, resultEntries));
        EXPECT_EQ(resultEntries.size(), 1);
        searchFor = "file1dependson2job.txt";
        EXPECT_NE(AZStd::find_if(resultEntries.begin(), resultEntries.end(), SearchPredicate), resultEntries.end());
        resultEntries.clear();

        // now ask for the job-to-job dependencies for any builder
        EXPECT_TRUE(m_data->m_connection.GetDependsOnSourceBySource(file1Uuid, SourceFileDependencyEntry::DEP_JobToJob, resultEntries));
        EXPECT_EQ(resultEntries.size(), 2);
        searchFor = "file1dependson1job.txt";
        EXPECT_NE(AZStd::find_if(resultEntries.begin(), resultEntries.end(), SearchPredicate), resultEntries.end());
        searchFor = "file1dependson2job.txt";
        EXPECT_NE(AZStd::find_if(resultEntries.begin(), resultEntries.end(), SearchPredicate), resultEntries.end());
        resultEntries.clear();

        // now ask for the reverse dependencies - we should find one source-to-source
        EXPECT_TRUE(m_data->m_connection.GetSourceFileDependenciesByDependsOnSource(file1DependsOn1Uuid, "file1dependson1.txt", "c:/root/file1dependson1.txt", SourceFileDependencyEntry::DEP_SourceToSource, resultEntries));
        EXPECT_EQ(resultEntries.size(), 1);
        searchUuid = file1Uuid;
        EXPECT_NE(AZStd::find_if(resultEntries.begin(), resultEntries.end(), SearchPredicateReverse), resultEntries.end());
        resultEntries.clear();

        // now ask for the reverse dependencies - we should find no job-to-job for this:
        EXPECT_FALSE(m_data->m_connection.GetSourceFileDependenciesByDependsOnSource(file1DependsOn1Uuid, "file1dependson1.txt", "c:/root/file1dependson1.txt", SourceFileDependencyEntry::DEP_JobToJob, resultEntries));
        EXPECT_EQ(resultEntries.size(), 0);
        resultEntries.clear();

        // now ask for the reverse dependencies - we should find one 'any' type.
        EXPECT_TRUE(m_data->m_connection.GetSourceFileDependenciesByDependsOnSource(file1DependsOn1Uuid, "file1dependson1.txt", "c:/root/file1dependson1.txt", SourceFileDependencyEntry::DEP_Any, resultEntries));
        EXPECT_EQ(resultEntries.size(), 1);
        searchUuid = file1Uuid;
        EXPECT_NE(AZStd::find_if(resultEntries.begin(), resultEntries.end(), SearchPredicateReverse), resultEntries.end());
        resultEntries.clear();

        // now try the other file - remember the ID for later
        ASSERT_TRUE(m_data->m_connection.GetSourceFileDependenciesByBuilderGUIDAndSource(builderGuid1, file2Uuid, SourceFileDependencyEntry::DEP_SourceToSource, resultEntries));
        EXPECT_EQ(resultEntries.size(), 1);
        searchFor = "file2dependson1.txt";
        EXPECT_NE(AZStd::find_if(resultEntries.begin(), resultEntries.end(), SearchPredicate), resultEntries.end());
        AZ::s64 entryIdSource = resultEntries[0].m_sourceDependencyID;
        resultEntries.clear();

        // and with Job-to-job dependencies
        EXPECT_TRUE(m_data->m_connection.GetSourceFileDependenciesByBuilderGUIDAndSource(builderGuid1, file2Uuid, SourceFileDependencyEntry::DEP_JobToJob, resultEntries));
        ASSERT_EQ(resultEntries.size(), 1);
        EXPECT_EQ(resultEntries[0].m_builderGuid, builderGuid1);
        EXPECT_EQ(resultEntries[0].m_sourceGuid, file2Uuid);
        EXPECT_NE(resultEntries[0].m_sourceDependencyID, AzToolsFramework::AssetDatabase::InvalidEntryId);
        EXPECT_STREQ(resultEntries[0].m_dependsOnSource.GetPath().c_str(), "file2dependson1job.txt");
        EXPECT_EQ(resultEntries[0].m_typeOfDependency,  SourceFileDependencyEntry::DEP_JobToJob);
        AZ::s64 entryIdJob = resultEntries[0].m_sourceDependencyID;
        resultEntries.clear();

        SourceFileDependencyEntry resultValue;
        EXPECT_TRUE(m_data->m_connection.GetSourceFileDependencyBySourceDependencyId(entryIdSource, resultValue));
        EXPECT_EQ(resultValue.m_sourceDependencyID, entryIdSource);
        EXPECT_EQ(resultValue.m_typeOfDependency, SourceFileDependencyEntry::DEP_SourceToSource);
        EXPECT_EQ(resultValue.m_sourceGuid, file2Uuid);
        EXPECT_STREQ(resultValue.m_dependsOnSource.GetPath().c_str(), "file2dependson1.txt");
        EXPECT_EQ(resultValue.m_builderGuid, builderGuid1);

        EXPECT_TRUE(m_data->m_connection.GetSourceFileDependencyBySourceDependencyId(entryIdJob, resultValue));
        EXPECT_EQ(resultValue.m_sourceDependencyID, entryIdJob);
        EXPECT_EQ(resultValue.m_typeOfDependency, SourceFileDependencyEntry::DEP_JobToJob);
        EXPECT_EQ(resultValue.m_sourceGuid, file2Uuid);
        EXPECT_STREQ(resultValue.m_dependsOnSource.GetPath().c_str(), "file2dependson1job.txt");
        EXPECT_EQ(resultValue.m_builderGuid, builderGuid1);

        // removal of source
        m_data->m_connection.RemoveSourceFileDependency(entryIdSource);
        EXPECT_FALSE(m_data->m_connection.GetSourceFileDependencyBySourceDependencyId(entryIdSource, resultValue));
        EXPECT_TRUE(m_data->m_connection.GetSourceFileDependencyBySourceDependencyId(entryIdJob, resultValue));

        // removeal of job
        m_data->m_connection.RemoveSourceFileDependency(entryIdJob);
        EXPECT_FALSE(m_data->m_connection.GetSourceFileDependencyBySourceDependencyId(entryIdSource, resultValue));
        EXPECT_FALSE(m_data->m_connection.GetSourceFileDependencyBySourceDependencyId(entryIdJob, resultValue));
    }

    TEST_F(AssetDatabaseTest, UpdateNonExistentFile_Fails)
    {
        CreateCoverageTestData();

        ASSERT_FALSE(m_data->m_connection.UpdateFileModTimeAndHashByFileNameAndScanFolderId("testfile.txt", m_data->m_scanFolder.m_scanFolderID, 1234, 1111));

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, UpdateExistingFile_Succeeds)
    {
        CreateCoverageTestData();

        FileDatabaseEntry entry;
        entry.m_fileName = "testfile.txt";
        entry.m_scanFolderPK = m_data->m_scanFolder.m_scanFolderID;

        bool entryAlreadyExists;
        ASSERT_TRUE(m_data->m_connection.InsertFile(entry, entryAlreadyExists));
        ASSERT_FALSE(entryAlreadyExists);
        ASSERT_TRUE(m_data->m_connection.UpdateFileModTimeAndHashByFileNameAndScanFolderId("testfile.txt", m_data->m_scanFolder.m_scanFolderID, 1234, 1111));

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetSourceBySourceName_InvalidInput_SourceNotFound)
    {
        CreateCoverageTestData();

        SourceDatabaseEntry resultSource;

        EXPECT_FALSE(m_data->m_connection.GetSourceBySourceNameScanFolderId("non_existent", m_data->m_scanFolder.m_scanFolderID, resultSource));

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0);
    }

    TEST_F(AssetDatabaseTest, GetSourceBySourceName_ValidInput_SourceFound)
    {
        CreateCoverageTestData();

        SourceDatabaseEntry resultSource;

        EXPECT_TRUE(m_data->m_connection.GetSourceBySourceNameScanFolderId("somefile.tif", m_data->m_scanFolder.m_scanFolderID, resultSource));
        EXPECT_EQ(resultSource.m_sourceGuid, m_data->m_sourceFile1.m_sourceGuid);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0);
    }

    TEST_F(AssetDatabaseTest, GetDirectReverseProductDependenciesBySourceGuidSubID_InvalidInput_ProductsNotFound)
    {
        CreateCoverageTestData();

        ProductDependencyDatabaseEntry productDependency;
        productDependency.m_productPK = m_data->m_product1.m_productID;
        productDependency.m_dependencySourceGuid = m_data->m_sourceFile1.m_sourceGuid;
        productDependency.m_dependencySubID = m_data->m_product1.m_subID;
        ASSERT_TRUE(m_data->m_connection.SetProductDependency(productDependency));

        ProductDatabaseEntryContainer resultProducts;

        AZ::Uuid invalidGuid = AZ::Uuid::CreateNull();
        AZ::s32 invalidSubId = -1;

        EXPECT_FALSE(m_data->m_connection.GetDirectReverseProductDependenciesBySourceGuidSubId(invalidGuid, m_data->m_product1.m_subID, resultProducts));
        EXPECT_FALSE(m_data->m_connection.GetDirectReverseProductDependenciesBySourceGuidSubId(m_data->m_sourceFile1.m_sourceGuid, invalidSubId, resultProducts));
        EXPECT_FALSE(m_data->m_connection.GetDirectReverseProductDependenciesBySourceGuidSubId(invalidGuid, invalidSubId, resultProducts));

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0);
    }

    TEST_F(AssetDatabaseTest, GetDirectReverseProductDependenciesBySourceGuidSubID_ValidInput_ProductsFound)
    {
        CreateCoverageTestData();

        ProductDependencyDatabaseEntry productDependency;
        productDependency.m_productPK = m_data->m_product1.m_productID;
        productDependency.m_dependencySourceGuid = m_data->m_sourceFile1.m_sourceGuid;
        productDependency.m_dependencySubID = m_data->m_product1.m_subID;
        ASSERT_TRUE(m_data->m_connection.SetProductDependency(productDependency));

        ProductDatabaseEntryContainer resultProducts;

        EXPECT_TRUE(m_data->m_connection.GetDirectReverseProductDependenciesBySourceGuidSubId(m_data->m_sourceFile1.m_sourceGuid, m_data->m_product1.m_subID, resultProducts));
        ASSERT_EQ(resultProducts.size(), 1);
        EXPECT_EQ(resultProducts[0], m_data->m_product1);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0);
    }

    TEST_F(AssetDatabaseTest, QueryProductDependenciesUnresolvedAdvanced_HandlesLargeSearch_Success)
    {
        CreateCoverageTestData();

        constexpr int NumTestPaths = 10000;

        AZStd::vector<AZStd::string> searchPaths;

        searchPaths.reserve(NumTestPaths);

        for (int i = 0; i < NumTestPaths; ++i)
        {
            searchPaths.emplace_back(AZStd::string::format("%d.txt", i));
        }

        ProductDependencyDatabaseEntry dependency1(m_data->m_product1.m_productID, AZ::Uuid::CreateNull(), 0, 0, "pc", false, "*.txt");
        ProductDependencyDatabaseEntry dependency2(
            m_data->m_product1.m_productID, AZ::Uuid::CreateNull(), 0, 0, "pc", false, "default.xml");

        m_data->m_connection.SetProductDependency(dependency1);
        m_data->m_connection.SetProductDependency(dependency2);

        AZStd::vector<AZStd::string> matches;
        matches.reserve(NumTestPaths);

        ASSERT_TRUE(m_data->m_connection.QueryProductDependenciesUnresolvedAdvanced(
            searchPaths,
            [&matches](AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry& /*entry*/, const AZStd::string& path)
            {
                matches.push_back(path);
                return true;
            }));

        ASSERT_EQ(matches.size(), searchPaths.size());

        // Check the first few results match
        for (int i = 0; i < 10 && i < NumTestPaths; ++i)
        {
            ASSERT_STREQ(matches[i].c_str(), searchPaths[i].c_str());
        }

        matches.clear();
        searchPaths.clear();
        searchPaths.push_back("default.xml");

        // Run the query again to make sure a) we can b) we don't get any extra results and c) we can query for exact (non wildcard) matches
        ASSERT_TRUE(m_data->m_connection.QueryProductDependenciesUnresolvedAdvanced(
            searchPaths,
            [&matches](AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry& /*entry*/, const AZStd::string& path)
            {
                matches.push_back(path);
                return true;
            }));

        ASSERT_THAT(matches, testing::ElementsAreArray(searchPaths));
    }

    TEST_F(AssetDatabaseTest, QueryCombined_Succeeds)
    {
        // This test specifically checks that the legacy subIds returned by QueryCombined are correctly matched to only the one product that they're associated with
        using namespace AzToolsFramework::AssetDatabase;

        CreateCoverageTestData();

        auto subIds = { 123, 134, 155, 166, 177 };
        AZStd::vector<LegacySubIDsEntry> createdLegacySubIds;

        for(int subId : subIds)
        {
            LegacySubIDsEntry subIdEntry;
            subIdEntry.m_productPK = m_data->m_product1.m_productID;
            subIdEntry.m_subID = subId;

            ASSERT_TRUE(m_data->m_connection.CreateOrUpdateLegacySubID(subIdEntry));

            createdLegacySubIds.push_back(subIdEntry);
        }

        AZStd::vector<CombinedDatabaseEntry> results;

        auto databaseQueryCallback = [&](AzToolsFramework::AssetDatabase::CombinedDatabaseEntry& combined) -> bool
        {
            results.push_back(combined);

            return true;
        };

        ASSERT_TRUE(m_data->m_connection.QueryCombined(databaseQueryCallback, AZ::Uuid::CreateNull(), nullptr, nullptr, AzToolsFramework::AssetSystem::JobStatus::Any, /*includeLegacyIds*/ true));

        bool foundProductWithLegacyIds = false;

        for(const auto& combined : results)
        {
            if (combined.m_productID == m_data->m_product1.m_productID)
            {
                foundProductWithLegacyIds = true;

                ASSERT_THAT(combined.m_legacySubIDs, testing::UnorderedElementsAreArray(createdLegacySubIds));
            }
            else
            {
                ASSERT_EQ(combined.m_legacySubIDs.size(), 0);
            }
        }

        ASSERT_TRUE(foundProductWithLegacyIds);
    }

    TEST_F(AssetDatabaseTest, InsertFile_Existing_ReturnsExisting)
    {
        CreateCoverageTestData();

        using namespace AzToolsFramework::AssetDatabase;

        FileDatabaseEntry fileEntry;
        fileEntry.m_fileName = "blah";
        fileEntry.m_scanFolderPK = m_data->m_scanFolder.m_scanFolderID;
        bool entryAlreadyExists = false;

        ASSERT_TRUE(m_data->m_connection.InsertFile(fileEntry, entryAlreadyExists));
        ASSERT_FALSE(entryAlreadyExists);

        fileEntry.m_fileID = InvalidEntryId; // InsertFile will update the Id, we want to test without a specified Id
        ASSERT_TRUE(m_data->m_connection.InsertFile(fileEntry, entryAlreadyExists));
        ASSERT_TRUE(entryAlreadyExists);

        // Test one more time, with the Id set to a specific entry now
        ASSERT_NE(fileEntry.m_fileID, InvalidEntryId);
        ASSERT_TRUE(m_data->m_connection.InsertFile(fileEntry, entryAlreadyExists));
        ASSERT_TRUE(entryAlreadyExists);
    }

    TEST_F(AssetDatabaseTest, StatDatabaseEntryEquality)
    {
        // two entries are the same if m_statName, m_statValue, and m_lastLogTime are same.
        using namespace AzToolsFramework::AssetDatabase;

        StatDatabaseEntry entry1, entry2;
        entry1.m_statName = "EqTest";
        entry1.m_statValue = 17632;
        entry1.m_lastLogTime = 54689213;
        entry2.m_statName = "EqTest";
        entry2.m_statValue = 17632;
        entry2.m_lastLogTime = 54689213;
        EXPECT_EQ(entry1, entry2);
        entry2.m_statName = "Helloworld";
        EXPECT_NE(entry1, entry2);
        entry2.m_statName = "EqTest";
        entry2.m_statValue = 81245;
        EXPECT_NE(entry1, entry2);
        entry2.m_statValue = 17632;
        entry2.m_lastLogTime = 12345678;
        EXPECT_NE(entry1, entry2);
    }

    TEST_F(AssetDatabaseTest, ReplaceStat_CreateIfNotExist)
    {
        // create entry if StatName is not seen
        CreateCoverageTestData();

        using namespace AzToolsFramework::AssetDatabase;

        StatDatabaseEntry statEntry;
        StatDatabaseEntryContainer statContainer;
        statEntry.m_statName = "testJob_createIfNotExist";
        statEntry.m_statValue = 1853;
        statEntry.m_lastLogTime = m_data->m_job1.m_lastLogTime;

        //! Ensure the Stats table is empty
        size_t entryCount = 0;
        m_data->m_connection.QueryStatsTable(
            [&entryCount]([[maybe_unused]] StatDatabaseEntry& stat)
            {
                ++entryCount;
                return true;
            });
        EXPECT_EQ(entryCount, 0);

        //! Insert a stat and read the stat. Stat read and stat written should be the same.
        EXPECT_TRUE(m_data->m_connection.ReplaceStat(statEntry));
        m_data->m_connection.GetStatByStatName(statEntry.m_statName.c_str(), statContainer);
        EXPECT_EQ(statContainer.size(), 1);
        EXPECT_EQ(statContainer.at(0), statEntry);
        statContainer.clear();

        //! Ensure one element is added.
        entryCount = 0;
        m_data->m_connection.QueryStatsTable(
            [&entryCount]([[maybe_unused]] StatDatabaseEntry& stat)
            {
                ++entryCount;
                return true;
            });
        EXPECT_EQ(entryCount, 1);
    }

    TEST_F(AssetDatabaseTest, ReplaceStat_UpdateIfExist)
    {
        // replace the entry if the StatName is in the asset database
        CreateCoverageTestData();

        using namespace AzToolsFramework::AssetDatabase;

        StatDatabaseEntry statEntry;
        StatDatabaseEntryContainer statContainer;
        statEntry.m_statName = "testJob_updateIfExist";
        statEntry.m_statValue = 8432;
        statEntry.m_lastLogTime = m_data->m_job1.m_lastLogTime;

        //! Ensure the Stats table is empty
        size_t entryCount = 0;
        m_data->m_connection.QueryStatsTable(
            [&entryCount]([[maybe_unused]] StatDatabaseEntry& stat)
            {
                ++entryCount;
                return true;
            });
        EXPECT_EQ(entryCount, 0);

        //! Insert a stat
        EXPECT_TRUE(m_data->m_connection.ReplaceStat(statEntry));

        //! Insert a stat with the same StatName. The old one should be replaced.
        StatDatabaseEntry secondStatEntry;
        secondStatEntry.m_statName = statEntry.m_statName;
        secondStatEntry.m_statValue = 16384;
        secondStatEntry.m_lastLogTime = 23570;
        EXPECT_TRUE(m_data->m_connection.ReplaceStat(secondStatEntry));
        m_data->m_connection.GetStatByStatName(statEntry.m_statName.c_str(), statContainer);
        ASSERT_EQ(statContainer.size(), 1);
        ASSERT_NE(statContainer.at(0), statEntry);
        ASSERT_EQ(statContainer.at(0), secondStatEntry);

        //! Ensure the element is replaced, not added.
        entryCount = 0;
        m_data->m_connection.QueryStatsTable(
            [&entryCount]([[maybe_unused]] StatDatabaseEntry& stat)
            {
                ++entryCount;
                return true;
            });
        ASSERT_EQ(entryCount, 1);
    }

    TEST_F(AssetDatabaseTest, QueryStatsTable)
    {
        const unsigned int StatCount = 10;
        InsertStatsTestData(StatCount);

        StatDatabaseEntryContainer statContainer;
        auto getAllStats = [&statContainer](StatDatabaseEntry& stat)
        {
            statContainer.push_back(stat);
            return true;
        };
        ASSERT_TRUE(m_data->m_connection.QueryStatsTable(getAllStats));
        ASSERT_TRUE(statContainer.size() == StatCount);

        // check the items are identical to what we inserted
        AZStd::sort(
            statContainer.begin(),
            statContainer.end(),
            [](const StatDatabaseEntry& lhs, const StatDatabaseEntry& rhs)
            {
                return lhs.m_statName != rhs.m_statName  ? lhs.m_statName < rhs.m_statName
                    : lhs.m_statValue != rhs.m_statValue ? lhs.m_statValue < rhs.m_statValue
                                                         : lhs.m_lastLogTime < rhs.m_lastLogTime;
            });

        StatDatabaseEntry statEntry = GetFirstStatEntry();
        for (unsigned int i = 0; i < StatCount; ++i)
        {
            EXPECT_EQ(statEntry, statContainer[i]);
            StepStatEntry(statEntry);
        }
    }

    TEST_F(AssetDatabaseTest, GetStatByStatName)
    {
        const unsigned int StatCount = 10;
        InsertStatsTestData(StatCount);

        StatDatabaseEntry statEntry = GetFirstStatEntry();
        for (unsigned int i = 0; i < StatCount; ++i)
        {
            StatDatabaseEntryContainer statContainer;
            ASSERT_TRUE(m_data->m_connection.GetStatByStatName(statEntry.m_statName.c_str(), statContainer));
            ASSERT_EQ(statContainer.size(), 1);
            EXPECT_EQ(statContainer[0], statEntry);
            StepStatEntry(statEntry);
        }
    }

    TEST_F(AssetDatabaseTest, GetStatLikeStatName)
    {
        const unsigned int StatCountPerPrefix = 5;
        AZStd::array<AZStd::string, 4> prefixes{ "Apple_", "Banana_", "Orange_", "Grape_" };
        for (const auto& prefix : prefixes)
        {
            InsertStatsTestData(StatCountPerPrefix, prefix);
        }

        //! Make sure we insert right number of entries
        {
            unsigned int entryCount{ 0 };
            auto countAllStats = [&entryCount]([[maybe_unused]] StatDatabaseEntry& stat)
            {
                ++entryCount;
                return true;
            };
            ASSERT_TRUE(m_data->m_connection.QueryStatsTable(countAllStats));
            ASSERT_EQ(entryCount, StatCountPerPrefix * prefixes.size());
        }

        //! Query StatName like prefixes
        for (const auto& prefix : prefixes)
        {
            StatDatabaseEntryContainer container;
            EXPECT_TRUE(m_data->m_connection.GetStatLikeStatName((prefix + "%").c_str(), container));
            EXPECT_EQ(container.size(), StatCountPerPrefix);
        }

        //! Query StatName like suffixes
        char query[] = "%a";
        for (unsigned int i = 0; i < StatCountPerPrefix; ++i, ++query[1])
        {
            StatDatabaseEntryContainer container;
            EXPECT_TRUE(m_data->m_connection.GetStatLikeStatName(query, container));
            EXPECT_EQ(container.size(), prefixes.size());
        }
    }

    class QueryLoggingTraceHandler : public AZ::Debug::TraceMessageBus::Handler
    {
    public:

        QueryLoggingTraceHandler()
        {
            BusConnect();
        }

        ~QueryLoggingTraceHandler()
        {
            BusDisconnect();
        }

        bool OnPrintf(const char* /*window*/, const char* message) override
        {
            if (m_expectedMessage.compare(message) == 0)
            {
                m_expectedMessageFound = true;
            }
            return false; // Return false so it also prints out to the log.
        }

        AZStd::string m_expectedMessage;
        bool m_expectedMessageFound = false;

    private:
    };

    TEST_F(AssetDatabaseTest, LoggingEnabled_InsertFile_LogMessageMatches)
    {
        using namespace AzToolsFramework::AssetDatabase;
        CreateCoverageTestData();
        QueryLoggingTraceHandler queryLoggingTraceHandler;
        queryLoggingTraceHandler.m_expectedMessage =
            "SELECT * FROM Files WHERE ScanFolderPK = :scanfolderpk AND FileName = :filename; = Params :scanfolderpk = `1`, :filename = `blah`\n";
        m_data->m_connection.SetQueryLogging(true);

        FileDatabaseEntry fileEntry;
        fileEntry.m_fileName = "blah";
        fileEntry.m_scanFolderPK = m_data->m_scanFolder.m_scanFolderID;
        bool entryAlreadyExists = false;
        ASSERT_TRUE(m_data->m_connection.InsertFile(fileEntry, entryAlreadyExists));
        m_data->m_connection.SetQueryLogging(false);
        ASSERT_TRUE(queryLoggingTraceHandler.m_expectedMessageFound);

    }

} // end namespace UnitTests
