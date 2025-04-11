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
    //! Mock EditorMeshColliderComponent for testing component mode.
    class TestMeshColliderComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , public PhysX::EditorColliderComponentRequestBus::Handler
        , public PhysX::EditorMeshColliderComponentRequestBus::Handler
    {
    public:
        AZ_EDITOR_COMPONENT(TestMeshColliderComponent, "{D2A6AD2D-8020-4312-9EE4-FF6CEBA02C21}");

        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<TestMeshColliderComponent>()
                    ->Version(0)
                    ->Field("ComponentMode", &TestMeshColliderComponent::m_componentModeDelegate);
            }
        }

        void Activate() override
        {
            EditorComponentBase::Activate();
            PhysX::EditorColliderComponentRequestBus::Handler::BusConnect(AZ::EntityComponentIdPair(GetEntityId(), GetId()));
            PhysX::EditorMeshColliderComponentRequestBus::Handler::BusConnect(AZ::EntityComponentIdPair(GetEntityId(), GetId()));
            m_componentModeDelegate.ConnectWithSingleComponentMode<
                TestMeshColliderComponent, PhysX::ColliderComponentMode>(
                    AZ::EntityComponentIdPair(GetEntityId(), GetId()), nullptr);
        }

        void Deactivate() override
        {
            PhysX::EditorMeshColliderComponentRequestBus::Handler::BusDisconnect();
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
        Physics::ShapeType GetShapeType() const override { return Physics::ShapeType::PhysicsAsset; }
        
        // EditorMeshColliderComponentRequests overrides ...
        void SetAssetScale(const AZ::Vector3& scale) override { m_assetScale = scale; }
        AZ::Vector3 GetAssetScale() const override { return m_assetScale; }

    private:
        AzToolsFramework::ComponentModeFramework::ComponentModeDelegate m_componentModeDelegate;
        AZ::Vector3 m_offset = AZ::Vector3::CreateZero();
        AZ::Quaternion m_rotation = AZ::Quaternion::CreateIdentity();
        AZ::Transform m_transform = AZ::Transform::CreateIdentity();
        AZ::Vector3 m_assetScale = AZ::Vector3::CreateOne();
    };
} // namespace UnitTest
