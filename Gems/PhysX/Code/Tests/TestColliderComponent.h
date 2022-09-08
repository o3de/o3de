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
    class TestColliderComponentMode
        : public AzToolsFramework::Components::EditorComponentBase
        , public PhysX::EditorColliderComponentRequestBus::Handler
    {
    public:
        AZ_EDITOR_COMPONENT(TestColliderComponentMode, "{D4EEA05C-4620-4A63-8816-2D0380158DF9}");

        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<TestColliderComponentMode>()
                    ->Version(0)
                    ->Field("ComponentMode", &TestColliderComponentMode::m_componentModeDelegate);
            }
        }

        void Activate() override
        {
            EditorComponentBase::Activate();
            PhysX::EditorColliderComponentRequestBus::Handler::BusConnect(AZ::EntityComponentIdPair(GetEntityId(), GetId()));
            m_componentModeDelegate.ConnectWithSingleComponentMode<
                TestColliderComponentMode, PhysX::ColliderComponentMode>(
                    AZ::EntityComponentIdPair(GetEntityId(), GetId()), nullptr);
        }

        void Deactivate() override
        {
            PhysX::EditorColliderComponentRequestBus::Handler::BusDisconnect();
            EditorComponentBase::Deactivate();
            m_componentModeDelegate.Disconnect();
        }

        void SetColliderOffset(const AZ::Vector3& offset) override { m_offset = offset; }
        AZ::Vector3 GetColliderOffset() const override { return m_offset; }
        void SetColliderRotation(const AZ::Quaternion& rotation) override { m_rotation = rotation; }
        AZ::Quaternion GetColliderRotation() const override { return m_rotation; }
        AZ::Transform GetColliderWorldTransform() const override { return m_transform; }
        void SetShapeType(Physics::ShapeType shapeType) override { m_shapeType = shapeType; }
        Physics::ShapeType GetShapeType() const override { return m_shapeType; }
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
        void SetAssetScale(const AZ::Vector3& scale) override { m_assetScale = scale; }
        AZ::Vector3 GetAssetScale() const override { return m_assetScale; }

    private:
        AzToolsFramework::ComponentModeFramework::ComponentModeDelegate m_componentModeDelegate;
        AZ::Vector3 m_offset = AZ::Vector3::CreateZero();
        AZ::Quaternion m_rotation = AZ::Quaternion::CreateIdentity();
        AZ::Transform m_transform = AZ::Transform::CreateIdentity();
        Physics::ShapeType m_shapeType = Physics::ShapeType::PhysicsAsset;
        float m_sphereRadius = 0.5f;
        float m_capsuleHeight = 1.0f;
        float m_capsuleRadius = 0.25f;
        float m_cylinderHeight = 1.0f;
        float m_cylinderRadius = 0.25f;
        AZ::u8 m_subdivisionCount = 16;
        AZ::Vector3 m_assetScale = AZ::Vector3::CreateOne();
    };
} // namespace UnitTest
