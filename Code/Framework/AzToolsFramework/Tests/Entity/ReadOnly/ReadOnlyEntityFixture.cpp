/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/Entity/ReadOnly/ReadOnlyEntityFixture.h>

#include <AzToolsFramework/Entity/EditorEntityHelpers.h>

namespace AzToolsFramework
{
    void ReadOnlyEntityFixture::SetUpEditorFixtureImpl()
    {
        // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
        // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash 
        // in the unit tests.
        AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

        m_readOnlyEntityInterface = AZ::Interface<ReadOnlyEntityInterface>::Get();
        ASSERT_TRUE(m_readOnlyEntityInterface != nullptr);

        GenerateTestHierarchy();
    }

    void ReadOnlyEntityFixture::TearDownEditorFixtureImpl()
    {
    }

    void ReadOnlyEntityFixture::GenerateTestHierarchy() 
    {
        /*
        *   Root
        *   |_  Child
        *       |_  GrandChild1
        *       |_  GrandChild2
        */

        m_entityMap[RootEntityName] =           CreateEditorEntity(RootEntityName, AZ::EntityId());
        m_entityMap[ChildEntityName] =          CreateEditorEntity(ChildEntityName, m_entityMap[RootEntityName]);
        m_entityMap[GrandChild1EntityName] =    CreateEditorEntity(GrandChild1EntityName, m_entityMap[ChildEntityName]);
        m_entityMap[GrandChild2EntityName] =    CreateEditorEntity(GrandChild2EntityName, m_entityMap[ChildEntityName]);
    }

    AZ::EntityId ReadOnlyEntityFixture::CreateEditorEntity(const char* name, AZ::EntityId parentId)
    {
        AZ::Entity* entity = nullptr;
        UnitTest::CreateDefaultEditorEntity(name, &entity);

        // Parent
        AZ::TransformBus::Event(entity->GetId(), &AZ::TransformInterface::SetParent, parentId);

        return entity->GetId();
    }
}
