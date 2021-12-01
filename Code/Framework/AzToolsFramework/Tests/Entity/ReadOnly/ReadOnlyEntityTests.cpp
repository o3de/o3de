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
    TEST_F(ReadOnlyEntityFixture, EntityIsNotReadOnlyByDefault)
    {
        EXPECT_FALSE(m_readOnlyEntityInterface->IsReadOnly(m_entityMap[ChildEntityName]));
    }

    TEST_F(ReadOnlyEntityFixture, TestRegisterEntityAsReadOnly)
    {
        // Registering an entity is successful.
        m_readOnlyEntityInterface->RegisterEntityAsReadOnly(m_entityMap[ChildEntityName]);
        EXPECT_TRUE(m_readOnlyEntityInterface->IsReadOnly(m_entityMap[ChildEntityName]));

        // Restore default state for other tests.
        m_readOnlyEntityInterface->UnregisterEntityAsReadOnly(m_entityMap[ChildEntityName]);
    }

    TEST_F(ReadOnlyEntityFixture, TestUnregisterEntityAsReadOnly)
    {
        // Unregistering an entity is successful.
        m_readOnlyEntityInterface->RegisterEntityAsReadOnly(m_entityMap[ChildEntityName]);
        m_readOnlyEntityInterface->UnregisterEntityAsReadOnly(m_entityMap[ChildEntityName]);
        EXPECT_FALSE(m_readOnlyEntityInterface->IsReadOnly(m_entityMap[ChildEntityName]));
    }

    TEST_F(ReadOnlyEntityFixture, TestRegisterEntitiesAsReadOnly)
    {
        // Registering multiple entities is successful.
        m_readOnlyEntityInterface->RegisterEntitiesAsReadOnly({ m_entityMap[GrandChild1EntityName], m_entityMap[GrandChild2EntityName] });
        EXPECT_TRUE(m_readOnlyEntityInterface->IsReadOnly(m_entityMap[GrandChild1EntityName]));
        EXPECT_TRUE(m_readOnlyEntityInterface->IsReadOnly(m_entityMap[GrandChild2EntityName]));

        // Restore default state for other tests.
        m_readOnlyEntityInterface->UnregisterEntitiesAsReadOnly({ m_entityMap[GrandChild1EntityName], m_entityMap[GrandChild2EntityName] });
    }

    TEST_F(ReadOnlyEntityFixture, TestUnregisterEntitiesAsReadOnly)
    {
        // Unregistering multiple entities is successful.
        m_readOnlyEntityInterface->RegisterEntitiesAsReadOnly({ m_entityMap[GrandChild1EntityName], m_entityMap[GrandChild2EntityName] });
        m_readOnlyEntityInterface->UnregisterEntitiesAsReadOnly({ m_entityMap[GrandChild1EntityName], m_entityMap[GrandChild2EntityName] });
        EXPECT_FALSE(m_readOnlyEntityInterface->IsReadOnly(m_entityMap[GrandChild1EntityName]));
        EXPECT_FALSE(m_readOnlyEntityInterface->IsReadOnly(m_entityMap[GrandChild2EntityName]));
    }

}
