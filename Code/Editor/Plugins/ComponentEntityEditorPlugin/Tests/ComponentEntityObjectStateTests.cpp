/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/TransformBus.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>

#include <Plugins/ComponentEntityEditorPlugin/Objects/ComponentEntityObject.h>

using namespace AzToolsFramework;

namespace UnitTest
{
    class ComponentEntityObjectVisibilityFixture
        : public ToolsApplicationFixture<>
        , private EditorEntityVisibilityNotificationBus::Router
        , private EditorEntityInfoNotificationBus::Handler
    {
    public:
        void SetUpEditorFixtureImpl() override
        {
            EditorEntityVisibilityNotificationBus::Router::BusRouterConnect();
            EditorEntityInfoNotificationBus::Handler::BusConnect();
        }

        void TearDownEditorFixtureImpl() override
        {
            EditorEntityInfoNotificationBus::Handler::BusDisconnect();
            EditorEntityVisibilityNotificationBus::Router::BusRouterDisconnect();
        }

        TestEditorActions m_editorActions;
        AZ::EntityId m_layerId;

    private:
        // EditorEntityVisibilityNotificationBus ...
        void OnEntityVisibilityChanged(bool /*visibility*/) override
        {
        }

        // EditorEntityInfoNotificationBus ...
        void OnEntityInfoUpdatedVisibility(AZ::EntityId /*entityId*/, bool /*visible*/) override
        {
        }
    };

    TEST_F(ComponentEntityObjectVisibilityFixture, ViewportComponentEntityObjectRespectsLayerVisibility)
    {
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        AZ::Entity* entityA = nullptr;
        const AZ::EntityId a = CreateDefaultEditorEntity("A", &entityA);
        AZ::Entity* entityB = nullptr;
        const AZ::EntityId b = CreateDefaultEditorEntity("B", &entityB);
        AZ::Entity* entityC = nullptr;
        const AZ::EntityId c = CreateDefaultEditorEntity("C", &entityC);

        m_layerId = CreateEditorLayerEntity("Layer");

        entityA->Deactivate();
        entityB->Deactivate();
        entityC->Deactivate();

        CComponentEntityObject componentEntityObjectA;
        componentEntityObjectA.AssignEntity(entityA);

        CComponentEntityObject componentEntityObjectB;
        componentEntityObjectB.AssignEntity(entityB);

        CComponentEntityObject componentEntityObjectC;
        componentEntityObjectC.AssignEntity(entityC);

        entityC->Activate();
        entityB->Activate();
        entityA->Activate();

        AZ::TransformBus::Event(a, &AZ::TransformBus::Events::SetParent, m_layerId);
        AZ::TransformBus::Event(b, &AZ::TransformBus::Events::SetParent, m_layerId);
        AZ::TransformBus::Event(c, &AZ::TransformBus::Events::SetParent, m_layerId);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        SetEntityVisibility(a, false);
        SetEntityVisibility(b, false);
        SetEntityVisibility(c, false);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        EXPECT_TRUE(componentEntityObjectA.IsHidden());
        EXPECT_TRUE(componentEntityObjectB.IsHidden());
        EXPECT_TRUE(componentEntityObjectC.IsHidden());
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        SetEntityVisibility(a, true);
        SetEntityVisibility(b, true);
        SetEntityVisibility(c, true);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        EXPECT_FALSE(componentEntityObjectA.IsHidden());
        EXPECT_FALSE(componentEntityObjectB.IsHidden());
        EXPECT_FALSE(componentEntityObjectC.IsHidden());
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        SetEntityVisibility(m_layerId, false);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        EXPECT_TRUE(componentEntityObjectA.IsHidden());
        EXPECT_TRUE(componentEntityObjectB.IsHidden());
        EXPECT_TRUE(componentEntityObjectC.IsHidden());
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }

    TEST_F(ComponentEntityObjectVisibilityFixture, ComponentEntityObjectDoesNotOverrideVisibility)
    {
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        AZ::Entity* entityA = nullptr;
        const AZ::EntityId a = CreateDefaultEditorEntity("A", &entityA);
        AZ::Entity* entityB = nullptr;
        const AZ::EntityId b = CreateDefaultEditorEntity("B", &entityB);
        AZ::Entity* entityC = nullptr;
        const AZ::EntityId c = CreateDefaultEditorEntity("C", &entityC);

        m_layerId = CreateEditorLayerEntity("Layer");

        entityA->Deactivate();
        entityB->Deactivate();
        entityC->Deactivate();

        CComponentEntityObject componentEntityObjectA;
        componentEntityObjectA.AssignEntity(entityA);

        CComponentEntityObject componentEntityObjectB;
        componentEntityObjectB.AssignEntity(entityB);

        CComponentEntityObject componentEntityObjectC;
        componentEntityObjectC.AssignEntity(entityC);

        entityC->Activate();
        entityB->Activate();
        entityA->Activate();

        AZ::TransformBus::Event(a, &AZ::TransformBus::Events::SetParent, m_layerId);
        AZ::TransformBus::Event(b, &AZ::TransformBus::Events::SetParent, m_layerId);
        AZ::TransformBus::Event(c, &AZ::TransformBus::Events::SetParent, m_layerId);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        SetEntityVisibility(m_layerId, false);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        EXPECT_TRUE(componentEntityObjectA.IsHidden());
        EXPECT_TRUE(componentEntityObjectB.IsHidden());
        EXPECT_TRUE(componentEntityObjectC.IsHidden());

        EXPECT_TRUE(IsEntitySetToBeVisible(a));
        EXPECT_FALSE(IsEntityVisible(a));

        EXPECT_TRUE(IsEntitySetToBeVisible(a));
        EXPECT_FALSE(IsEntityVisible(b));

        EXPECT_TRUE(IsEntitySetToBeVisible(a));
        EXPECT_FALSE(IsEntityVisible(c));
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }

    TEST_F(ComponentEntityObjectVisibilityFixture, ViewportComponentEntityObjectRespectsLayerLock)
    {
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        AZ::Entity* entityA = nullptr;
        const AZ::EntityId a = CreateDefaultEditorEntity("A", &entityA);
        AZ::Entity* entityB = nullptr;
        const AZ::EntityId b = CreateDefaultEditorEntity("B", &entityB);
        AZ::Entity* entityC = nullptr;
        const AZ::EntityId c = CreateDefaultEditorEntity("C", &entityC);

        m_layerId = CreateEditorLayerEntity("Layer");

        entityA->Deactivate();
        entityB->Deactivate();
        entityC->Deactivate();

        CComponentEntityObject componentEntityObjectA;
        componentEntityObjectA.AssignEntity(entityA);

        CComponentEntityObject componentEntityObjectB;
        componentEntityObjectB.AssignEntity(entityB);

        CComponentEntityObject componentEntityObjectC;
        componentEntityObjectC.AssignEntity(entityC);

        entityC->Activate();
        entityB->Activate();
        entityA->Activate();

        AZ::TransformBus::Event(a, &AZ::TransformBus::Events::SetParent, m_layerId);
        AZ::TransformBus::Event(b, &AZ::TransformBus::Events::SetParent, m_layerId);
        AZ::TransformBus::Event(c, &AZ::TransformBus::Events::SetParent, m_layerId);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        SetEntityLockState(a, true);
        SetEntityLockState(b, true);
        SetEntityLockState(c, true);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        EXPECT_TRUE(componentEntityObjectA.IsFrozen());
        EXPECT_TRUE(componentEntityObjectB.IsFrozen());
        EXPECT_TRUE(componentEntityObjectC.IsFrozen());
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        SetEntityLockState(a, false);
        SetEntityLockState(b, false);
        SetEntityLockState(c, false);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        EXPECT_FALSE(componentEntityObjectA.IsFrozen());
        EXPECT_FALSE(componentEntityObjectB.IsFrozen());
        EXPECT_FALSE(componentEntityObjectC.IsFrozen());
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        SetEntityLockState(m_layerId, true);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        EXPECT_TRUE(componentEntityObjectA.IsFrozen());
        EXPECT_TRUE(componentEntityObjectB.IsFrozen());
        EXPECT_TRUE(componentEntityObjectC.IsFrozen());
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }
} // namespace UnitTest
