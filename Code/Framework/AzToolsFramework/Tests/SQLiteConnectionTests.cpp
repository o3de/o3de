/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Uuid.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <AzToolsFramework/SQLite/SQLiteConnection.h>

namespace UnitTest
{
    using namespace AzToolsFramework;

    static int s_numTablesToCreate = 100;
    // we'll do about as much as we can get away with for about a second with most modern CPU
    static int s_numTrialsToPerform = 10500;
   
    class SQLiteTest
        : public LeakDetectionFixture
    {
    public:
        SQLiteTest()
            : LeakDetectionFixture()
        {
        }

        ~SQLiteTest() override = default;

        void SetUp() override
        {
            LeakDetectionFixture::SetUp();
            m_database.reset(aznew SQLite::Connection()); 
            m_randomDatabaseFileName = AZStd::string::format("%s_temp.sqlite", AZ::Uuid::CreateRandom().ToString<AZStd::string>().c_str());
            m_database->Open(m_randomDatabaseFileName.c_str(), false);
        }

        void TearDown() override
        {
            m_database->Close();
            m_database.reset();
            AZ::IO::SystemFile::Delete(m_randomDatabaseFileName.c_str());
            m_randomDatabaseFileName.set_capacity(0);
            LeakDetectionFixture::TearDown();
        }

        AZStd::string m_randomDatabaseFileName;
        AZStd::unique_ptr<SQLite::Connection> m_database;
    };


    TEST_F(SQLiteTest, DoesTableExist_BadInputs_ShouldAssert)
    {
        ASSERT_TRUE(m_database->IsOpen());

        // basic tests, bad input:
        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(m_database->DoesTableExist(""));
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);

        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(m_database->DoesTableExist(nullptr));
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    // DoesTableExist had an off-by-one error in its string.  It would not always crash.
    // This just stress tests that function (which also tests statement creation and destruction) to ensure
    // that if there is a problem with failing creation of functions, we don't crash.
    TEST_F(SQLiteTest, DoesTableExist_BasicFuzzTest_BadTableNames_ShouldNotAssert_ShouldReturnFalse)
    {
        ASSERT_TRUE(m_database->IsOpen());

        // now make up some random table names and try them out - none should exist.
        AZStd::string randomJunkTableName;
        randomJunkTableName.resize(16, '\0');
        for (int trialNumber = 0; trialNumber < s_numTrialsToPerform; ++trialNumber)
        {
            for (int randomChar = 0; randomChar < 16; ++randomChar)
            {
                // note that this also puts characters AFTER the null, if a null appears in the mddle.
                // so that if there are off by one errors they could include cruft afterwards.
                randomJunkTableName[randomChar] = (char)(rand() % 256); // this will trigger invalid UTF8 decoding too
            }
            randomJunkTableName[0] = 'a'; // just to make sure we don't retry the null case.
            EXPECT_FALSE(m_database->DoesTableExist(randomJunkTableName.c_str()));
        }
    }
    
    // this makes sure that repeated calls to DoesTableExist does not cause some crazy assertion or failure
    // if code is incorrect, it might, because DoesTableExists tends to create and destroy temporary statments.
    // as a coincidence, this also serves as somewhat of a stress test for all the other parts of the database
    // since this tests both creation of statements, execution of them, and retiring / cleaning the memory / freeing them
    TEST_F(SQLiteTest, DoesTableExist_BasicStressTest_GoodTableNames_ShouldNotAssert_ShouldReturnTrue)
    {
        // --- SETUP PHASE ----
        ASSERT_TRUE(m_database->IsOpen());
        
        // outside scope to improve reuse memory performance.
        AZStd::string randomValidTableName;
        AZStd::string createDatabaseTableStatement;

        for (int tableToCreate = 0; tableToCreate < s_numTablesToCreate; ++tableToCreate)
        {
            randomValidTableName = AZStd::string::format("testtable_%i", tableToCreate);
            createDatabaseTableStatement = AZStd::string::format(
                "CREATE TABLE IF NOT EXISTS %s( "
                "    rowID   INTEGER PRIMARY KEY, "
                "    version INTEGER NOT NULL);", randomValidTableName.c_str());

            m_database->AddStatement(randomValidTableName, createDatabaseTableStatement);
            EXPECT_TRUE(m_database->ExecuteOneOffStatement(randomValidTableName.c_str()));
            m_database->RemoveStatement(randomValidTableName.c_str());
        }

        // --- TEST PHASE ----
        for (int trialNumber = 0; trialNumber < s_numTrialsToPerform; ++trialNumber)
        {
            randomValidTableName = AZStd::string::format("testtable_%i", rand() % s_numTablesToCreate);
            EXPECT_TRUE(m_database->DoesTableExist(randomValidTableName.c_str()));
        }
    }

}
