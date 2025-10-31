/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Spawnable/SpawnableEntitiesInterface.h>
#include <Prefab/Spawnable/SpawnableTestFixture.h>

namespace UnitTest
{
    class SpawnableTicketTests : public SpawnableTestFixture
    {
    };

    TEST_F(SpawnableTicketTests, CreateAndDestroyEmptyTicket)
    {
        AzFramework::EntitySpawnTicket ticket;
    }

    TEST_F(SpawnableTicketTests, CreateAndDestroyAssignedTicket)
    {
        AzFramework::EntitySpawnTicket ticket(CreateSpawnableAsset(32));
    }

    TEST_F(SpawnableTicketTests, IsValidTicketWithEmtpyTicket)
    {
        AzFramework::EntitySpawnTicket ticket;
        EXPECT_FALSE(ticket.IsValid());
    }

    TEST_F(SpawnableTicketTests, IsValidTicketWithAssignedTicket)
    {
        AzFramework::EntitySpawnTicket ticket(CreateSpawnableAsset(32));
        EXPECT_TRUE(ticket.IsValid());
    }

    TEST_F(SpawnableTicketTests, RetrieveTicketId)
    {
        AzFramework::EntitySpawnTicket ticket(CreateSpawnableAsset(32));
        EXPECT_NE(0, ticket.GetId());
    }

    TEST_F(SpawnableTicketTests, RetrieveSpawnable)
    {
        AzFramework::EntitySpawnTicket ticket(CreateSpawnableAsset(32));
        const AZ::Data::Asset<AzFramework::Spawnable>* spawnable = ticket.GetSpawnable();
        ASSERT_NE(nullptr, spawnable);
        EXPECT_NE(nullptr, spawnable->GetData());
    }

    TEST_F(SpawnableTicketTests, MoveConstructedTicket)
    {
        AzFramework::EntitySpawnTicket ticket(CreateSpawnableAsset(32));
        AzFramework::EntitySpawnTicket movedToTicket(AZStd::move(ticket));

        const AZ::Data::Asset<AzFramework::Spawnable>* spawnable = movedToTicket.GetSpawnable();
        ASSERT_NE(nullptr, spawnable);
        EXPECT_EQ(32, spawnable->Get()->GetEntities().size());
    }

    TEST_F(SpawnableTicketTests, MoveAssignedTicket)
    {
        AzFramework::EntitySpawnTicket ticket(CreateSpawnableAsset(32));
        AzFramework::EntitySpawnTicket movedToTicket;
        movedToTicket = AZStd::move(ticket);

        const AZ::Data::Asset<AzFramework::Spawnable>* spawnable = movedToTicket.GetSpawnable();
        ASSERT_NE(nullptr, spawnable);
        EXPECT_EQ(32, spawnable->Get()->GetEntities().size());
    }

    TEST_F(SpawnableTicketTests, MoveAssignedTicketToAlreadyAssignedTicket)
    {
        AzFramework::EntitySpawnTicket ticket(CreateSpawnableAsset(32));
        AzFramework::EntitySpawnTicket movedToTicket(CreateSpawnableAsset(16));
        movedToTicket = AZStd::move(ticket);

        const AZ::Data::Asset<AzFramework::Spawnable>* spawnable = movedToTicket.GetSpawnable();
        ASSERT_NE(nullptr, spawnable);
        EXPECT_EQ(32, spawnable->Get()->GetEntities().size());
    }

    TEST_F(SpawnableTicketTests, CopyConstructedTicket)
    {
        AzFramework::EntitySpawnTicket ticket(CreateSpawnableAsset(32));
        AzFramework::EntitySpawnTicket copiedToTicket(ticket);

        const AZ::Data::Asset<AzFramework::Spawnable>* spawnable = copiedToTicket.GetSpawnable();
        ASSERT_NE(nullptr, spawnable);
        EXPECT_EQ(32, spawnable->Get()->GetEntities().size());
    }

    TEST_F(SpawnableTicketTests, CopyAssignedTicket)
    {
        AzFramework::EntitySpawnTicket ticket(CreateSpawnableAsset(32));
        AzFramework::EntitySpawnTicket copiedToTicket;
        copiedToTicket = ticket;

        const AZ::Data::Asset<AzFramework::Spawnable>* spawnable = copiedToTicket.GetSpawnable();
        ASSERT_NE(nullptr, spawnable);
        EXPECT_EQ(32, spawnable->Get()->GetEntities().size());
    }

    TEST_F(SpawnableTicketTests, CopyAssignedTicketToAlreadyAssignedTicket)
    {
        AzFramework::EntitySpawnTicket ticket(CreateSpawnableAsset(32));
        AzFramework::EntitySpawnTicket copiedToTicket(CreateSpawnableAsset(16));
        copiedToTicket = ticket;

        const AZ::Data::Asset<AzFramework::Spawnable>* spawnable = copiedToTicket.GetSpawnable();
        ASSERT_NE(nullptr, spawnable);
        EXPECT_EQ(32, spawnable->Get()->GetEntities().size());
    }
} // namespace UnitTest
