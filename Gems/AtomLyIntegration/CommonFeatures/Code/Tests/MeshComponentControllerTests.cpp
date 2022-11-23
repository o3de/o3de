/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>

#include <Mesh/EditorMeshComponent.h>
#include <Mesh/MeshComponentController.h>

namespace UnitTest
{
    class IntersectionNotificationDetector : public AzFramework::RenderGeometry::IntersectionNotificationBus::Handler
    {
    public:
        void Connect(const AzFramework::EntityContextId& entityContextId);
        void Disconnect();

        // IntersectionNotificationBus overrides ...
        void OnEntityConnected(AZ::EntityId entityId) override;
        void OnEntityDisconnected(AZ::EntityId entityId) override;
        void OnGeometryChanged(AZ::EntityId entityId) override;

        AZ::EntityId m_lastEntityIdChanged;
    };

    void IntersectionNotificationDetector::Connect(const AzFramework::EntityContextId& entityContextId)
    {
        AzFramework::RenderGeometry::IntersectionNotificationBus::Handler::BusConnect(entityContextId);
    }

    void IntersectionNotificationDetector::Disconnect()
    {
        AzFramework::RenderGeometry::IntersectionNotificationBus::Handler::BusDisconnect();
    }

    void IntersectionNotificationDetector::OnEntityConnected([[maybe_unused]] AZ::EntityId entityId)
    {
    }

    void IntersectionNotificationDetector::OnEntityDisconnected([[maybe_unused]] AZ::EntityId entityId)
    {
    }

    void IntersectionNotificationDetector::OnGeometryChanged(AZ::EntityId entityId)
    {
        m_lastEntityIdChanged = entityId;
    }

    class MeshComponentControllerFixture : public ToolsApplicationFixture<false>
    {
    public:
        void SetUpEditorFixtureImpl() override
        {
            m_meshComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(AZ::Render::MeshComponent::CreateDescriptor());
            m_meshComponentDescriptor->Reflect(GetApplication()->GetSerializeContext());

            m_editorMeshComponentDescriptor =
                AZStd::unique_ptr<AZ::ComponentDescriptor>(AZ::Render::EditorMeshComponent::CreateDescriptor());
            m_editorMeshComponentDescriptor->Reflect(GetApplication()->GetSerializeContext());

            m_entity = AZStd::make_unique<AZ::Entity>();
            m_entity->Init();
            m_entity->CreateComponent<AzToolsFramework::Components::TransformComponent>();
            m_entity->Activate();

            AzFramework::EntityContextId contextId(AZStd::string_view{"123456"});
            m_intersectionNotificationDetector.Connect(contextId);
        }

        void TearDownEditorFixtureImpl() override
        {
            m_entity.reset();

            m_intersectionNotificationDetector.Disconnect();

            m_meshComponentDescriptor.reset();
            m_editorMeshComponentDescriptor.reset();
        }

        AZStd::unique_ptr<AZ::Entity> m_entity;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_meshComponentDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_editorMeshComponentDescriptor;
        IntersectionNotificationDetector m_intersectionNotificationDetector;
    };

    TEST_F(MeshComponentControllerFixture, IntersectionNotificationBusIsNotifiedWhenMeshComponentControllerTransformIsModified)
    {
        // suppress warning when feature process is not created in test environment
        UnitTest::ErrorHandler featureProcessorNotFound("Unable to find a MeshFeatureProcessorInterface on the entityId.");

        m_entity->Deactivate();
        m_entity->CreateComponent<AZ::Render::EditorMeshComponent>();
        m_entity->Activate();

        AZ::TransformBus::Event(
            m_entity->GetId(), &AZ::TransformBus::Events::SetWorldTM, AZ::Transform::CreateTranslation(AZ::Vector3(1.0f, 2.0f, 3.0f)));

        using ::testing::Eq;
        EXPECT_THAT(m_entity->GetId(), Eq(m_intersectionNotificationDetector.m_lastEntityIdChanged));
    }
} // namespace UnitTest
