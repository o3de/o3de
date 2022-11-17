/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#if !defined(Q_MOC_RUN)
#include <AzCore/UnitTest/TestTypes.h>
#endif
#include "BuilderManagerTests.h"
#include <AzCore/std/smart_ptr/make_shared.h>
#include <native/connection/connectionManager.h>

namespace UnitTests
{
    class BuilderManagerTest : public UnitTest::ScopedAllocatorSetupFixture
    {

    };

    TEST_F(BuilderManagerTest, GetBuilder_ReservesFirstBuilderForCreateJobs)
    {
        ConnectionManager cm{nullptr};
        TestBuilderManager bm(&cm);

        // We start off with 1 builder pre-created
        ASSERT_EQ(bm.GetBuilderCreationCount(), 1);

        // Save off the uuid of the CreateJobs builder for later
        auto createJobsBuilderUuid = bm.GetBuilder(AssetProcessor::BuilderPurpose::CreateJobs)->GetUuid();

        constexpr int NumberOfBuilders = 15; // Start up several builders, more builders increases the chances of exposing a bug
        AZStd::vector<AssetProcessor::BuilderRef> builders;

        for (int i = 0; i < NumberOfBuilders; ++i)
        {
            builders.push_back(bm.GetBuilder(AssetProcessor::BuilderPurpose::ProcessJob));
        }

        // There should now be NumberOfBuilders + 1 builders, because the first one is reserved for CreateJobs
        ASSERT_EQ(bm.GetBuilderCreationCount(), NumberOfBuilders + 1);

        // Now if we request a CreateJob builder, we should get the same builder again
        ASSERT_EQ(bm.GetBuilder(AssetProcessor::BuilderPurpose::CreateJobs)->GetUuid(), createJobsBuilderUuid);

        // And the number of builders should remain the same
        ASSERT_EQ(bm.GetBuilderCreationCount(), NumberOfBuilders + 1);

        // Release the builders and check that we still get the same builder for CreateJobs
        builders = {};

        ASSERT_EQ(bm.GetBuilder(AssetProcessor::BuilderPurpose::CreateJobs)->GetUuid(), createJobsBuilderUuid);
        ASSERT_EQ(bm.GetBuilderCreationCount(), NumberOfBuilders + 1);
    }

    AZ::Outcome<void, AZStd::string> TestBuilder::Start(AssetProcessor::BuilderPurpose /*purpose*/)
    {
        return AZ::Success();
    }

    TestBuilderManager::TestBuilderManager(ConnectionManager* connectionManager): BuilderManager(connectionManager)
    {
        TestBuilderManager::AddNewBuilder(AssetProcessor::BuilderPurpose::CreateJobs);
    }

    int TestBuilderManager::GetBuilderCreationCount() const
    {
        return m_connectionCounter;
    }

    AZStd::shared_ptr<AssetProcessor::Builder> TestBuilderManager::AddNewBuilder(AssetProcessor::BuilderPurpose purpose)
    {
        auto uuid = AZ::Uuid::CreateRandom();
        auto builder = AZStd::make_shared<TestBuilder>(m_quitListener, uuid, ++m_connectionCounter);

        m_builderList.AddBuilder(builder, purpose);

        return builder;
    }
}
