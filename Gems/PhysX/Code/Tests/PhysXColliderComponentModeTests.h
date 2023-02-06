/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "TestColliderComponent.h"

#include <AZTestShared/Math/MathTestHelpers.h>
#include <AzManipulatorTestFramework/AzManipulatorTestFrameworkTestHelpers.h>
#include <AzManipulatorTestFramework/IndirectManipulatorViewportInteraction.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/ToolsComponents/EditorNonUniformScaleComponent.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzToolsFramework/Viewport/ViewportSettings.h>
#include <AzToolsFramework/ViewportSelection/EditorDefaultSelection.h>
#include <AzToolsFramework/ViewportSelection/EditorInteractionSystemViewportSelectionRequestBus.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <AzToolsFramework/ViewportUi/ViewportUiManager.h>
#include <EditorColliderComponent.h>
#include <Tests/Viewport/ViewportUiManagerTests.h>

namespace UnitTest
{
    class PhysXColliderComponentModeTest : public ToolsApplicationFixture<false>
    {
    protected:
        AZ::Entity* CreateColliderComponent()
        {
            AZ::Entity* entity = nullptr;
            AZ::EntityId entityId = CreateDefaultEditorEntity("ComponentModeEntity", &entity);

            entity->Deactivate();

            // Add placeholder component which implements component mode.
            auto colliderComponent = entity->CreateComponent<TestColliderComponentMode>();

            m_colliderComponentId = colliderComponent->GetId();

            entity->Activate();

            AzToolsFramework::SelectEntity(entityId);

            return entity;
        }

        void SetUpEditorFixtureImpl() override
        {
            m_viewportManagerWrapper.Create();
        }
        void TearDownEditorFixtureImpl() override
        {
            m_viewportManagerWrapper.Destroy();
        }

        // Needed to support ViewportUi request calls.
        ViewportManagerWrapper m_viewportManagerWrapper;
        AZ::ComponentId m_colliderComponentId;
    };

    using PhysXColliderComponentModeManipulatorTest =
        UnitTest::IndirectCallManipulatorViewportInteractionFixtureMixin<PhysXColliderComponentModeTest>;

    class PhysXEditorColliderComponentFixture : public UnitTest::ToolsApplicationFixture<false>
    {
    public:
        void SetUpEditorFixtureImpl() override;
        void TearDownEditorFixtureImpl() override;
        void SetupTransform(const AZ::Quaternion& rotation, const AZ::Vector3& translation, float uniformScale);
        void SetupCollider(
            const Physics::ShapeConfiguration& shapeConfiguration,
            const AZ::Quaternion& colliderRotation,
            const AZ::Vector3& colliderOffset);
        void SetupNonUniformScale(const AZ::Vector3& nonUniformScale);
        void EnterColliderSubMode(PhysX::ColliderComponentModeRequests::SubMode subMode);

    protected:
        AZ::Entity* m_entity = nullptr;
        AZ::EntityComponentIdPair m_idPair;
    };

    using PhysXEditorColliderComponentManipulatorFixture =
        UnitTest::IndirectCallManipulatorViewportInteractionFixtureMixin<PhysXEditorColliderComponentFixture>;
} // namespace UnitTest
