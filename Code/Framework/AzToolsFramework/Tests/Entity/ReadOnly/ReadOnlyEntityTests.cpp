/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/Entity/ReadOnly/ReadOnlyEntityFixture.h>

namespace AzToolsFramework
{
    TEST_F(ReadOnlyEntityFixture, NoHandlerEntityIsNotReadOnlyByDefault)
    {
        EXPECT_FALSE(m_readOnlyEntityPublicInterface->IsReadOnly(m_entityMap[ChildEntityName]));
    }

    TEST_F(ReadOnlyEntityFixture, SingleHandlerEntityIsReadOnly)
    {
        // Create a handler that sets all entities to read-only.
        ReadOnlyHandlerAlwaysTrue alwaysTrueHandler;

        // All entities should be marked read-only now.
        EXPECT_TRUE(m_readOnlyEntityPublicInterface->IsReadOnly(m_entityMap[RootEntityName]));
        EXPECT_TRUE(m_readOnlyEntityPublicInterface->IsReadOnly(m_entityMap[ChildEntityName]));
        EXPECT_TRUE(m_readOnlyEntityPublicInterface->IsReadOnly(m_entityMap[GrandChild1EntityName]));
        EXPECT_TRUE(m_readOnlyEntityPublicInterface->IsReadOnly(m_entityMap[GrandChild2EntityName]));
    }

    TEST_F(ReadOnlyEntityFixture, SingleHandlerEntityIsNotReadOnly)
    {
        // Create a handler that sets all entities to read-only.
        ReadOnlyHandlerAlwaysFalse alwaysFalseHandler;

        // All entities should not be marked read-only now.
        EXPECT_FALSE(m_readOnlyEntityPublicInterface->IsReadOnly(m_entityMap[RootEntityName]));
        EXPECT_FALSE(m_readOnlyEntityPublicInterface->IsReadOnly(m_entityMap[ChildEntityName]));
        EXPECT_FALSE(m_readOnlyEntityPublicInterface->IsReadOnly(m_entityMap[GrandChild1EntityName]));
        EXPECT_FALSE(m_readOnlyEntityPublicInterface->IsReadOnly(m_entityMap[GrandChild2EntityName]));
    }

    TEST_F(ReadOnlyEntityFixture, SingleHandlerWithLogic)
    {
        // Create a handler that sets just the child entity to read-only.
        ReadOnlyHandlerEntityId entityIdHandler(m_entityMap[ChildEntityName]);

        EXPECT_FALSE(m_readOnlyEntityPublicInterface->IsReadOnly(m_entityMap[RootEntityName]));
        EXPECT_TRUE(m_readOnlyEntityPublicInterface->IsReadOnly(m_entityMap[ChildEntityName]));
        EXPECT_FALSE(m_readOnlyEntityPublicInterface->IsReadOnly(m_entityMap[GrandChild1EntityName]));
        EXPECT_FALSE(m_readOnlyEntityPublicInterface->IsReadOnly(m_entityMap[GrandChild2EntityName]));
    }

    TEST_F(ReadOnlyEntityFixture, TwoHandlersCanOverlap)
    {
        // Create two handlers that set different entities to read-only.
        ReadOnlyHandlerEntityId entityIdHandler1(m_entityMap[ChildEntityName]);
        ReadOnlyHandlerEntityId entityIdHandler2(m_entityMap[GrandChild2EntityName]);

        // Both entities should be marked as read-only, while others aren't.
        EXPECT_FALSE(m_readOnlyEntityPublicInterface->IsReadOnly(m_entityMap[RootEntityName]));
        EXPECT_TRUE(m_readOnlyEntityPublicInterface->IsReadOnly(m_entityMap[ChildEntityName]));
        EXPECT_FALSE(m_readOnlyEntityPublicInterface->IsReadOnly(m_entityMap[GrandChild1EntityName]));
        EXPECT_TRUE(m_readOnlyEntityPublicInterface->IsReadOnly(m_entityMap[GrandChild2EntityName]));
    }

    TEST_F(ReadOnlyEntityFixture, EnsureCacheIsRefreshedCorrectly)
    {
        // Verify the child entity is not marked as read-only
        EXPECT_FALSE(m_readOnlyEntityPublicInterface->IsReadOnly(m_entityMap[ChildEntityName]));

        // Create a handler that sets the child entity to read-only.
        ReadOnlyHandlerEntityId entityIdHandler(m_entityMap[ChildEntityName]);

        // Communicate to the ReadOnlyEntitySystemComponent that the read-only state for the child entity may have changed.
        // Note that this operation would usually be executed by the handler, hence the Query interface call.
        if (auto readOnlyEntityQueryInterface = AZ::Interface<ReadOnlyEntityQueryInterface>::Get())
        {
            readOnlyEntityQueryInterface->RefreshReadOnlyState({ m_entityMap[ChildEntityName] });
        }

        // Verify the child entity is marked as read-only
        EXPECT_TRUE(m_readOnlyEntityPublicInterface->IsReadOnly(m_entityMap[ChildEntityName]));
    }

    TEST_F(ReadOnlyEntityFixture, EnsureCacheIsClearedCorrectly)
    {
        {
            // Create a handler that sets the child entity to read-only.
            ReadOnlyHandlerEntityId entityIdHandler(m_entityMap[ChildEntityName]);

            // Verify the child entity is marked as read-only
            EXPECT_TRUE(m_readOnlyEntityPublicInterface->IsReadOnly(m_entityMap[ChildEntityName]));
        }
        // When the handler goes out of scope, it calls RefreshReadOnlyStateForAllEntities and refreshes the cache.

        // Verify the child entity is no longer marked as read-only
        EXPECT_FALSE(m_readOnlyEntityPublicInterface->IsReadOnly(m_entityMap[ChildEntityName]));
    }

    TEST_F(ReadOnlyEntityFixture, EnsureCacheIsClearedCorrectlyEvenIfUnchanged)
    {
        // Create a handler that sets all entities to read-only.
        ReadOnlyHandlerAlwaysTrue alwaysTrueHandler;

        {
            // Create a handler that sets the child entity to read-only.
            ReadOnlyHandlerEntityId entityIdHandler(m_entityMap[ChildEntityName]);

            // Verify the child entity is marked as read-only
            EXPECT_TRUE(m_readOnlyEntityPublicInterface->IsReadOnly(m_entityMap[ChildEntityName]));
        }
        // When the handler goes out of scope, it calls RefreshReadOnlyStateForAllEntities and refreshes the cache.

        // Verify the child entity is still marked as read-only
        EXPECT_TRUE(m_readOnlyEntityPublicInterface->IsReadOnly(m_entityMap[ChildEntityName]));
    }
}
