/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Entity.h>
#include <AzCore/std/containers/set.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzFramework/Spawnable/Spawnable.h>

namespace UnitTest
{
    class SpawnableSortEntitiesTestFixture
        : public ToolsApplicationFixture<>
    {
    protected:
        AZStd::vector<AZ::Entity*> m_unsorted;
        AZStd::vector<AZ::Entity*> m_sorted;
        AZStd::multiset<AZ::Entity*> m_expectedEntities;
        AZStd::multiset<AZ::Entity*> m_actualEntities;
        AzFramework::Spawnable m_spawnable;

        // Entity IDs to use in tests
        AZ::EntityId E1 = AZ::EntityId(1);
        AZ::EntityId E2 = AZ::EntityId(2);
        AZ::EntityId E3 = AZ::EntityId(3);
        AZ::EntityId E4 = AZ::EntityId(4);
        AZ::EntityId E5 = AZ::EntityId(5);
        AZ::EntityId E6 = AZ::EntityId(6);
        AZ::EntityId MissingNo = AZ::EntityId(999);

        void TearDownEditorFixtureImpl() override;

        // add entity to m_unsorted
        void AddEntity(AZ::EntityId id, AZ::EntityId parentId = AZ::EntityId(), bool expectedInSorted = true);
        void AddEntity(AZ::Entity* entity, bool expectedInSorted = true);

        void ConvertEntitiesToSpawnable(const AZStd::vector<AZ::Entity*>& rawEntities);
        void ConvertoSpawnableToEntities(AZStd::vector<AZ::Entity*>& rawEntities);

        void SortAndSanityCheck();

        bool IsChildAfterParent(AZ::EntityId childId, AZ::EntityId parentId);
    };

}
