/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/UnitTest/TestTypes.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AZTestShared/Utils/Utils.h>

#include <System/PhysXSystem.h>

namespace AzPhysics
{
    struct TriggerEvent;
}

namespace PhysXEditorTests
{
    using EntityPtr = AZStd::unique_ptr<AZ::Entity>;

    //! Creates a default editor entity in an inactive state.
    EntityPtr CreateInactiveEditorEntity(const char* entityName);

    //! Creates and activates a game entity from an editor entity.
    EntityPtr CreateActiveGameEntityFromEditorEntity(AZ::Entity* editorEntity);

    //! Creates an active editor entity with shape collider and box shape components.
    EntityPtr CreateBoxShapeColliderEditorEntity(
        const AZ::Transform& transform,
        const AZ::Vector3& nonUniformScale,
        const AZ::Vector3& boxDimensions,
        const AZ::Vector3& translationOffset);

    //! Creates an active editor entity with shape collider and capsule shape components.
    EntityPtr CreateCapsuleShapeColliderEditorEntity(
        const AZ::Transform& transform, float radius, float height, const AZ::Vector3& translationOffset);

    //! Creates an active editor entity with shape collider and sphere shape components.
    EntityPtr CreateSphereShapeColliderEditorEntity(const AZ::Transform& transform, float radius, const AZ::Vector3& translationOffset);

    //! Gets the AABB for the simulated body on the entity with the given ID, or returns a null AABB if no body is found.
    AZ::Aabb GetSimulatedBodyAabb(AZ::EntityId entityId);

    //! Class used for loading system components from this gem.
    class PhysXEditorSystemComponentEntity
        : public AZ::Entity
    {
        friend class PhysXEditorFixture;
    };

    //! Test fixture which creates a tools application, loads the PhysX runtime gem and creates a default physics world.
    //! The application is created for the whole test case, rather than individually for each test, due to a known
    //! problem with buses when repeatedly loading and unloading gems. A new default world is created for each test.
    class PhysXEditorFixture
        : public testing::Test
        , public Physics::DefaultWorldBus::Handler
        , public UnitTest::RegistryTestHelper
    {
    public:
        void SetUp() override;
        void TearDown() override;

        void ConnectToPVD();
        void DisconnectFromPVD();

        // DefaultWorldBus
        AzPhysics::SceneHandle GetDefaultSceneHandle() const override;
       
        AzPhysics::SceneHandle m_defaultSceneHandle = AzPhysics::InvalidSceneHandle;
        AzPhysics::Scene* m_defaultScene = nullptr;

        // workaround for parameterized tests causing issues with this (and any derived) fixture
        void ValidateInvalidEditorShapeColliderComponentParams(float radius, float height);
    };
} // namespace PhysXEditorTests
