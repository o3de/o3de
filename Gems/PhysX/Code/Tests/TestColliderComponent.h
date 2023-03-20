/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ComponentMode/ComponentModeDelegate.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <PhysX/EditorColliderComponentRequestBus.h>
#include <Editor/ColliderComponentMode.h>

namespace UnitTest
{
    /// Mock EditorColliderComponent for testing component mode.
    class TestColliderComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , public PhysX::EditorColliderComponentRequestBus::Handler
        , public PhysX::EditorPrimitiveColliderComponentRequestBus::Handler
    {
    public:
        AZ_EDITOR_COMPONENT(TestColliderComponent, "{D4EEA05C-4620-4A63-8816-2D0380158DF9}");

        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<TestColliderComponent>()
                    ->Version(0)
                    ->Field("ComponentMode", &TestColliderComponent::m_componentModeDelegate);
            }
        }

        void Activate() override
        {
            EditorComponentBase::Activate();
            PhysX::EditorColliderComponentRequestBus::Handler::BusConnect(AZ::EntityComponentIdPair(GetEntityId(), GetId()));
            PhysX::EditorPrimitiveColliderComponentRequestBus::Handler::BusConnect(AZ::EntityComponentIdPair(GetEntityId(), GetId()));
            m_componentModeDelegate.ConnectWithSingleComponentMode<
                TestColliderComponent, PhysX::ColliderComponentMode>(
                    AZ::EntityComponentIdPair(GetEntityId(), GetId()), nullptr);
        }

        void Deactivate() override
        {
            PhysX::EditorPrimitiveColliderComponentRequestBus::Handler::BusDisconnect();
            PhysX::EditorColliderComponentRequestBus::Handler::BusDisconnect();
            EditorComponentBase::Deactivate();
            m_componentModeDelegate.Disconnect();
        }

        // EditorColliderComponentRequests overrides ...
        void SetColliderOffset(const AZ::Vector3& offset) override { m_offset = offset; }
        AZ::Vector3 GetColliderOffset() const override { return m_offset; }
        void SetColliderRotation(const AZ::Quaternion& rotation) override { m_rotation = rotation; }
        AZ::Quaternion GetColliderRotation() const override { return m_rotation; }
        AZ::Transform GetColliderWorldTransform() const override { return m_transform; }
        Physics::ShapeType GetShapeType() const override { return m_shapeType; }
        
        // EditorPrimitiveColliderComponentRequests overrides ...
        void SetShapeType(Physics::ShapeType shapeType) override { m_shapeType = shapeType; }
        void SetBoxDimensions(const AZ::Vector3& dimensions) override { m_boxDimensions = dimensions; }
        AZ::Vector3 GetBoxDimensions() const override { return m_boxDimensions; }
        void SetSphereRadius(float radius) override { m_sphereRadius = radius; }
        float GetSphereRadius() const override { return m_sphereRadius; }
        void SetCapsuleRadius(float radius) override { m_capsuleRadius = radius; }
        float GetCapsuleRadius() const override { return m_capsuleRadius; }
        void SetCapsuleHeight(float height) override { m_capsuleHeight = height; }
        float GetCapsuleHeight() const override { return m_capsuleHeight; }
        void SetCylinderRadius(float radius) override { m_cylinderRadius = radius; }
        float GetCylinderRadius() const override { return m_cylinderRadius; }
        void SetCylinderHeight(float height) override { m_cylinderHeight = height; }
        float GetCylinderHeight() const override { return m_cylinderHeight; }
        void SetCylinderSubdivisionCount(AZ::u8 subdivisionCount) override { m_subdivisionCount = subdivisionCount; }
        AZ::u8 GetCylinderSubdivisionCount() const override { return m_subdivisionCount; }

    private:
        AzToolsFramework::ComponentModeFramework::ComponentModeDelegate m_componentModeDelegate;
        AZ::Vector3 m_offset = AZ::Vector3::CreateZero();
        AZ::Quaternion m_rotation = AZ::Quaternion::CreateIdentity();
        AZ::Transform m_transform = AZ::Transform::CreateIdentity();
        Physics::ShapeType m_shapeType = Physics::ShapeType::Box;
        AZ::Vector3 m_boxDimensions = Physics::ShapeConstants::DefaultBoxDimensions;
        float m_sphereRadius = Physics::ShapeConstants::DefaultSphereRadius;
        float m_capsuleHeight = Physics::ShapeConstants::DefaultCapsuleHeight;
        float m_capsuleRadius = Physics::ShapeConstants::DefaultCapsuleRadius;
        float m_cylinderHeight = Physics::ShapeConstants::DefaultCylinderHeight;
        float m_cylinderRadius = Physics::ShapeConstants::DefaultCylinderRadius;
        AZ::u8 m_subdivisionCount = Physics::ShapeConstants::DefaultCylinderSubdivisionCount;
    };
} // namespace UnitTest
