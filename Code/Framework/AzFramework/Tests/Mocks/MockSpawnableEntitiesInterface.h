/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/UnitTest/UnitTest.h>
#include <gmock/gmock.h>
#include <AzFramework/Spawnable/SpawnableEntitiesInterface.h>
#include <AzFramework/Spawnable/SpawnableEntitiesManager.h>

namespace AzFramework
{
    class MockSpawnableEntitiesInterface;
    using NiceSpawnableEntitiesInterfaceMock = ::testing::NiceMock<MockSpawnableEntitiesInterface>;

    class MockSpawnableEntitiesInterface : public SpawnableEntitiesDefinition
    {
    public:
        AZ_RTTI(MockSpawnableEntitiesInterface, "{2A20FF73-C445-4F32-ABB9-5CF0A5778404}", SpawnableEntitiesDefinition);

        MockSpawnableEntitiesInterface()
        {
            AZ::Interface<SpawnableEntitiesDefinition>::Register(this);
        }

        virtual ~MockSpawnableEntitiesInterface()
        {
            AZ::Interface<SpawnableEntitiesDefinition>::Unregister(this);
        }

        MOCK_METHOD2(SpawnAllEntities, void(EntitySpawnTicket& ticket, SpawnAllEntitiesOptionalArgs optionalArgs));

        MOCK_METHOD3(
            SpawnEntities,
            void(EntitySpawnTicket& ticket, AZStd::vector<size_t> entityIndices, SpawnEntitiesOptionalArgs optionalArgs));

        MOCK_METHOD2(DespawnAllEntities, void(EntitySpawnTicket& ticket, DespawnAllEntitiesOptionalArgs optionalArgs));

        MOCK_METHOD3(DespawnEntity, void(AZ::EntityId entityId, EntitySpawnTicket& ticket, DespawnEntityOptionalArgs optionalArgs));

        MOCK_METHOD2(
            RetrieveEntitySpawnTicket, void(EntitySpawnTicket::Id entitySpawnTicketId, RetrieveEntitySpawnTicketCallback callback));

        MOCK_METHOD3(
            ReloadSpawnable,
            void(EntitySpawnTicket& ticket, AZ::Data::Asset<Spawnable> spawnable, ReloadSpawnableOptionalArgs optionalArgs));

        MOCK_METHOD3(
            ListEntities, void(EntitySpawnTicket& ticket, ListEntitiesCallback listCallback, ListEntitiesOptionalArgs optionalArgs));

        MOCK_METHOD3(
            ListIndicesAndEntities,
            void(EntitySpawnTicket& ticket, ListIndicesEntitiesCallback listCallback, ListEntitiesOptionalArgs optionalArgs));

        MOCK_METHOD3(
            ClaimEntities,
            void(EntitySpawnTicket& ticket, ClaimEntitiesCallback listCallback, ClaimEntitiesOptionalArgs optionalArgs));

        MOCK_METHOD3(Barrier, void(EntitySpawnTicket& ticket, BarrierCallback completionCallback, BarrierOptionalArgs optionalArgs));

        MOCK_METHOD1(CreateTicket, AZStd::pair<EntitySpawnTicket::Id, void*>(AZ::Data::Asset<Spawnable>&& spawnable));
        MOCK_METHOD1(DestroyTicket, void(void* ticket));

        /** Installs some default result values for the above functions.
         *   Note that you can always override these in scope of your test by adding additional ON_CALL / EXPECT_CALL
         *   statements in the body of your test or after calling this function, and yours will take precedence.
         **/
        static void InstallDefaultReturns(NiceSpawnableEntitiesInterfaceMock& target)
        {
            using namespace ::testing;

            // The ID and pointer are completely arbitrary, they just need to both be non-zero to look like a valid ticket.
            constexpr EntitySpawnTicket::Id ticketId(1);
            static int ticketPayload = 0;
            ON_CALL(target, CreateTicket(_)).WillByDefault(
                Return(AZStd::make_pair<AzFramework::EntitySpawnTicket::Id, void*>(ticketId, &ticketPayload)));
        }

    };

} // namespace AzFramework
