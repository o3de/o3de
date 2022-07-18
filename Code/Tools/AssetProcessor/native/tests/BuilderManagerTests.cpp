/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>
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

        // Request a builder for processing, this will create another builder (and hold on to the reference to prevent it from being re-used)
        auto processJobBuilder = bm.GetBuilder(AssetProcessor::BuilderPurpose::ProcessJob);

        // There should now be 2 builders, because the first one is reserved for CreateJobs
        ASSERT_EQ(bm.GetBuilderCreationCount(), 2);

        // As an extra test, request a builder for CreateJobs, which should not create a new builder
        bm.GetBuilder(AssetProcessor::BuilderPurpose::CreateJobs);

        // There should still be 2 builders
        ASSERT_EQ(bm.GetBuilderCreationCount(), 2);
    }

    AZ::Outcome<void, AZStd::string> TestBuilder::Start(AssetProcessor::BuilderPurpose /*purpose*/)
    {
        return AZ::Success();
    }

    TestBuilderManager::TestBuilderManager(ConnectionManager* connectionManager): BuilderManager(connectionManager)
    {
        AddNewBuilder();
    }

    int TestBuilderManager::GetBuilderCreationCount() const
    {
        return m_connectionCounter;
    }

    AZStd::shared_ptr<AssetProcessor::Builder> TestBuilderManager::AddNewBuilder()
    {
        auto uuid = AZ::Uuid::CreateRandom();

        m_builders[uuid] = AZStd::make_shared<TestBuilder>(m_quitListener, uuid, ++m_connectionCounter);

        return m_builders[uuid];
    }
}
