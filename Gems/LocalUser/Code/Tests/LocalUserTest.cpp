/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <LocalUserSystemComponent.h>

#include <AzTest/AzTest.h>

#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/UnitTest/TestTypes.h>

class LocalUserTest
    : public UnitTest::LeakDetectionFixture
{
protected:
    void SetUp() override
    {
        LeakDetectionFixture::SetUp();
        m_localUserSystemComponent = AZStd::make_unique<LocalUser::LocalUserSystemComponent>();
        m_localUserSystemComponent->LocalUser::LocalUserRequestBus::Handler::BusConnect();
    }

    void TearDown() override
    {
        m_localUserSystemComponent->LocalUser::LocalUserRequestBus::Handler::BusDisconnect();
        m_localUserSystemComponent.reset();
        LeakDetectionFixture::TearDown();
    }

private:
    AZStd::unique_ptr<LocalUser::LocalUserSystemComponent> m_localUserSystemComponent;
};

AzFramework::LocalUserId testUserIds[5] = {9, 99, 12345, 98765, 99999999};

TEST_F(LocalUserTest, GetPrimaryLocalUserIdBeforeSet)
{
    // No local user assigned to a local player slot yet
    ASSERT_TRUE(LocalUser::LocalUserRequests::GetPrimaryLocalUserId() == AzFramework::LocalUserIdNone);
}

TEST_F(LocalUserTest, SetThenGetPrimaryLocalUserId)
{
    // Assign a local user id to local player slot 0
    AZ::u32 assignedSlot = LocalUser::LocalPlayerSlotNone;
    LocalUser::LocalUserRequestBus::BroadcastResult(assignedSlot,
                                                    &LocalUser::LocalUserRequests::AssignLocalUserIdToLocalPlayerSlot,
                                                    testUserIds[0],
                                                    LocalUser::LocalPlayerSlotPrimary);
    ASSERT_TRUE(assignedSlot == LocalUser::LocalPlayerSlotPrimary);
    ASSERT_TRUE(LocalUser::LocalUserRequests::GetPrimaryLocalUserId() == testUserIds[0]);
}

TEST_F(LocalUserTest, AssignLocalUserIdToLocalPlayerSlotAny)
{
    // Assign a local user id to local player slot 0
    AZ::u32 assignedSlot = LocalUser::LocalPlayerSlotNone;
    LocalUser::LocalUserRequestBus::BroadcastResult(assignedSlot,
                                                    &LocalUser::LocalUserRequests::AssignLocalUserIdToLocalPlayerSlot,
                                                    testUserIds[0],
                                                    LocalUser::LocalPlayerSlotAny);
    ASSERT_TRUE(assignedSlot == LocalUser::LocalPlayerSlotPrimary);

    // Assign a local user id to local player slot 1
    LocalUser::LocalUserRequestBus::BroadcastResult(assignedSlot,
                                                    &LocalUser::LocalUserRequests::AssignLocalUserIdToLocalPlayerSlot,
                                                    testUserIds[1],
                                                    LocalUser::LocalPlayerSlotAny);
    ASSERT_TRUE(assignedSlot == LocalUser::LocalPlayerSlotPrimary + 1);

    // Assign a local user id to local player slot 2
    LocalUser::LocalUserRequestBus::BroadcastResult(assignedSlot,
                                                    &LocalUser::LocalUserRequests::AssignLocalUserIdToLocalPlayerSlot,
                                                    testUserIds[2],
                                                    LocalUser::LocalPlayerSlotAny);
    ASSERT_TRUE(assignedSlot == LocalUser::LocalPlayerSlotPrimary + 2);
}

TEST_F(LocalUserTest, AssignLocalUserIdToSpecificLocalPlayerSlot)
{
    // Assign a local user id to local player slot 0
    AZ::u32 assignedSlot = LocalUser::LocalPlayerSlotNone;
    LocalUser::LocalUserRequestBus::BroadcastResult(assignedSlot,
                                                    &LocalUser::LocalUserRequests::AssignLocalUserIdToLocalPlayerSlot,
                                                    testUserIds[0],
                                                    LocalUser::LocalPlayerSlotPrimary);
    ASSERT_TRUE(assignedSlot == LocalUser::LocalPlayerSlotPrimary);

    // Assign a local user id to local player slot 1
    LocalUser::LocalUserRequestBus::BroadcastResult(assignedSlot,
                                                    &LocalUser::LocalUserRequests::AssignLocalUserIdToLocalPlayerSlot,
                                                    testUserIds[1],
                                                    1);
    ASSERT_TRUE(assignedSlot == LocalUser::LocalPlayerSlotPrimary + 1);

    // Assign a local user id to local player slot 3
    LocalUser::LocalUserRequestBus::BroadcastResult(assignedSlot,
                                                    &LocalUser::LocalUserRequests::AssignLocalUserIdToLocalPlayerSlot,
                                                    testUserIds[2],
                                                    3);
    ASSERT_TRUE(assignedSlot == LocalUser::LocalPlayerSlotPrimary + 3);

    // Try to assign another local user id to local player slot 1 which is occupied
    LocalUser::LocalUserRequestBus::BroadcastResult(assignedSlot,
                                                    &LocalUser::LocalUserRequests::AssignLocalUserIdToLocalPlayerSlot,
                                                    testUserIds[4],
                                                    1);
    ASSERT_TRUE(assignedSlot == LocalUser::LocalPlayerSlotNone);

    // Try to assign a local user id to local player slot max
    LocalUser::LocalUserRequestBus::BroadcastResult(assignedSlot,
                                                    &LocalUser::LocalUserRequests::AssignLocalUserIdToLocalPlayerSlot,
                                                    testUserIds[4],
                                                    LocalUser::LocalPlayerSlotMax);
    ASSERT_TRUE(assignedSlot == LocalUser::LocalPlayerSlotNone);

    // Try to assign a local user id to local player slot none
    LocalUser::LocalUserRequestBus::BroadcastResult(assignedSlot,
                                                    &LocalUser::LocalUserRequests::AssignLocalUserIdToLocalPlayerSlot,
                                                    testUserIds[4],
                                                    LocalUser::LocalPlayerSlotNone);
    ASSERT_TRUE(assignedSlot == LocalUser::LocalPlayerSlotNone);

    // Assign a local user id to local player slot 2 which is still empty
    LocalUser::LocalUserRequestBus::BroadcastResult(assignedSlot,
                                                    &LocalUser::LocalUserRequests::AssignLocalUserIdToLocalPlayerSlot,
                                                    testUserIds[4],
                                                    2);
    ASSERT_TRUE(assignedSlot == LocalUser::LocalPlayerSlotPrimary + 2);
}

TEST_F(LocalUserTest, AssignMoreLocalUserIdsThanLocalPlayerSlots)
{
    // Assign local user ids to local player slots 0 - 3
    for (int i = 0; i < LocalUser::LocalPlayerSlotMax; ++i)
    {
        LocalUser::LocalUserRequestBus::Broadcast(&LocalUser::LocalUserRequests::AssignLocalUserIdToLocalPlayerSlot,
                                                  testUserIds[i],
                                                  LocalUser::LocalPlayerSlotAny);
    }

    // Try and assign another local user id now that all the slots are occupied
    AZ::u32 assignedSlot = LocalUser::LocalPlayerSlotNone;
    LocalUser::LocalUserRequestBus::BroadcastResult(assignedSlot,
                                                    &LocalUser::LocalUserRequests::AssignLocalUserIdToLocalPlayerSlot,
                                                    testUserIds[4],
                                                    LocalUser::LocalPlayerSlotAny);
    ASSERT_TRUE(assignedSlot == LocalUser::LocalPlayerSlotNone);
}

TEST_F(LocalUserTest, RemoveLocalUserIdFromLocalPlayerSlot)
{
    // Assign local user ids to local player slots 0 - 3
    for (int i = 0; i < LocalUser::LocalPlayerSlotMax; ++i)
    {
        LocalUser::LocalUserRequestBus::Broadcast(&LocalUser::LocalUserRequests::AssignLocalUserIdToLocalPlayerSlot,
                                                  testUserIds[i],
                                                  LocalUser::LocalPlayerSlotAny);
    }

    // Remove the local user id that was assigned to local player slot 0
    AZ::u32 removedFromSlot = LocalUser::LocalPlayerSlotNone;
    LocalUser::LocalUserRequestBus::BroadcastResult(removedFromSlot,
                                                    &LocalUser::LocalUserRequests::RemoveLocalUserIdFromLocalPlayerSlot,
                                                    testUserIds[0]);
    ASSERT_TRUE(removedFromSlot == LocalUser::LocalPlayerSlotPrimary);

    // Remove a local user id that was not assigned to any local player slot
    LocalUser::LocalUserRequestBus::BroadcastResult(removedFromSlot,
                                                    &LocalUser::LocalUserRequests::RemoveLocalUserIdFromLocalPlayerSlot,
                                                    testUserIds[4]);
    ASSERT_TRUE(removedFromSlot == LocalUser::LocalPlayerSlotNone);

    // Remove the local user id that was assigned to local player slot 3
    LocalUser::LocalUserRequestBus::BroadcastResult(removedFromSlot,
                                                    &LocalUser::LocalUserRequests::RemoveLocalUserIdFromLocalPlayerSlot,
                                                    testUserIds[3]);
    ASSERT_TRUE(removedFromSlot == LocalUser::LocalPlayerSlotPrimary + 3);
}

TEST_F(LocalUserTest, GetLocalUserIdAssignedToLocalPlayerSlot)
{
    // Assign local user ids to local player slots 0 - 3
    for (int i = 0; i < LocalUser::LocalPlayerSlotMax; ++i)
    {
        LocalUser::LocalUserRequestBus::Broadcast(&LocalUser::LocalUserRequests::AssignLocalUserIdToLocalPlayerSlot,
                                                  testUserIds[i],
                                                  LocalUser::LocalPlayerSlotAny);
    }

    // Get the local user id that was assigned to local player slot 0
    AzFramework::LocalUserId localUserId = AzFramework::LocalUserIdNone;
    LocalUser::LocalUserRequestBus::BroadcastResult(localUserId,
                                                    &LocalUser::LocalUserRequests::GetLocalUserIdAssignedToLocalPlayerSlot,
                                                    LocalUser::LocalPlayerSlotPrimary);
    ASSERT_TRUE(localUserId == testUserIds[0]);
    
    // Get the local user id that was assigned to local player slot max
    LocalUser::LocalUserRequestBus::BroadcastResult(localUserId,
                                                    &LocalUser::LocalUserRequests::RemoveLocalUserIdFromLocalPlayerSlot,
                                                    LocalUser::LocalPlayerSlotMax);
    ASSERT_TRUE(localUserId == AzFramework::LocalUserIdNone);
    
    // Get the local user id that was assigned to local player slot none
    LocalUser::LocalUserRequestBus::BroadcastResult(localUserId,
                                                    &LocalUser::LocalUserRequests::RemoveLocalUserIdFromLocalPlayerSlot,
                                                    LocalUser::LocalPlayerSlotNone);
    ASSERT_TRUE(localUserId == AzFramework::LocalUserIdNone);

    // Get the local user id that was assigned to local player slot 3
    LocalUser::LocalUserRequestBus::BroadcastResult(localUserId,
                                                    &LocalUser::LocalUserRequests::GetLocalUserIdAssignedToLocalPlayerSlot,
                                                    3);
    ASSERT_TRUE(localUserId == testUserIds[3]);
}

TEST_F(LocalUserTest, GetLocalPlayerSlotOccupiedByLocalUserId)
{
    // Assign local user ids to local player slots 0 - 3
    for (int i = 0; i < LocalUser::LocalPlayerSlotMax; ++i)
    {
        LocalUser::LocalUserRequestBus::Broadcast(&LocalUser::LocalUserRequests::AssignLocalUserIdToLocalPlayerSlot,
                                                  testUserIds[i],
                                                  LocalUser::LocalPlayerSlotAny);
    }

    // Get the local player slot occupied by the user id assigned first above
    AZ::u32 assignedSlot = LocalUser::LocalPlayerSlotNone;
    LocalUser::LocalUserRequestBus::BroadcastResult(assignedSlot,
                                                    &LocalUser::LocalUserRequests::GetLocalPlayerSlotOccupiedByLocalUserId,
                                                    testUserIds[0]);
    ASSERT_TRUE(assignedSlot == LocalUser::LocalPlayerSlotPrimary);

    // Get the local player slot occupied by LocalUserIdNone
    LocalUser::LocalUserRequestBus::BroadcastResult(assignedSlot,
                                                    &LocalUser::LocalUserRequests::GetLocalPlayerSlotOccupiedByLocalUserId,
                                                    AzFramework::LocalUserIdNone);
    ASSERT_TRUE(assignedSlot == LocalUser::LocalPlayerSlotNone);

    // Get the local player slot occupied by the user id assigned last above
    LocalUser::LocalUserRequestBus::BroadcastResult(assignedSlot,
                                                    &LocalUser::LocalUserRequests::GetLocalPlayerSlotOccupiedByLocalUserId,
                                                    testUserIds[3]);
    ASSERT_TRUE(assignedSlot == LocalUser::LocalPlayerSlotPrimary + 3);
}

TEST_F(LocalUserTest, ClearAllLocalUserIdToLocalPlayerSlotAssignments)
{
    // Assign local user ids to local player slots 0 - 3
    for (int i = 0; i < LocalUser::LocalPlayerSlotMax; ++i)
    {
        LocalUser::LocalUserRequestBus::Broadcast(&LocalUser::LocalUserRequests::AssignLocalUserIdToLocalPlayerSlot,
                                                  testUserIds[i],
                                                  LocalUser::LocalPlayerSlotAny);
    }

    // Clear all local player slot assignments
    LocalUser::LocalUserRequestBus::Broadcast(&LocalUser::LocalUserRequests::ClearAllLocalUserIdToLocalPlayerSlotAssignments);

    // Get the local player slot occupied by the user id assigned first above
    AZ::u32 assignedSlot = LocalUser::LocalPlayerSlotNone;
    LocalUser::LocalUserRequestBus::BroadcastResult(assignedSlot,
                                                    &LocalUser::LocalUserRequests::GetLocalPlayerSlotOccupiedByLocalUserId,
                                                    testUserIds[0]);
    ASSERT_TRUE(assignedSlot == LocalUser::LocalPlayerSlotNone);

    // Get the local player slot occupied a local user id that was not assigned above
    LocalUser::LocalUserRequestBus::BroadcastResult(assignedSlot,
                                                    &LocalUser::LocalUserRequests::GetLocalPlayerSlotOccupiedByLocalUserId,
                                                    testUserIds[4]);
    ASSERT_TRUE(assignedSlot == LocalUser::LocalPlayerSlotNone);

    // Get the local player slot occupied by the user id assigned last above
    LocalUser::LocalUserRequestBus::BroadcastResult(assignedSlot,
                                                    &LocalUser::LocalUserRequests::GetLocalPlayerSlotOccupiedByLocalUserId,
                                                    testUserIds[3]);
    ASSERT_TRUE(assignedSlot == LocalUser::LocalPlayerSlotNone);
}

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
