/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>

#include <Mesh/EditorMeshComponent.h>
#include <Mesh/MeshComponentController.h>

namespace UnitTest
{
    static AzFramework::EntityContextId FindOwningContextId(const AZ::EntityId entityId)
    {
        AzFramework::EntityContextId contextId = AzFramework::EntityContextId::CreateNull();
        AzFramework::EntityIdContextQueryBus::EventResult(contextId, entityId, &AzFramework::EntityIdContextQueries::GetOwningContextId);
        return contextId;
    }

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

    class MeshComponentControllerFixture : public ToolsApplicationFixture
    {
    public:
        void SetUpEditorFixtureImpl() override
        {
            m_meshComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(AZ::Render::MeshComponent::CreateDescriptor());
            m_meshComponentDescriptor->Reflect(GetApplication()->GetSerializeContext());

            m_editorMeshComponentDescriptor =
                AZStd::unique_ptr<AZ::ComponentDescriptor>(AZ::Render::EditorMeshComponent::CreateDescriptor());
            m_editorMeshComponentDescriptor->Reflect(GetApplication()->GetSerializeContext());

            m_entityId1 = CreateDefaultEditorEntity("Entity1");
            m_entityIds.push_back(m_entityId1);

            m_intersectionNotificationDetector.Connect(FindOwningContextId(m_entityId1));
        }

        void TearDownEditorFixtureImpl() override
        {
            bool entityDestroyed = false;
            AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
                entityDestroyed, &AzToolsFramework::EditorEntityContextRequestBus::Events::DestroyEditorEntity, m_entityId1);

            m_intersectionNotificationDetector.Disconnect();

            m_meshComponentDescriptor.reset();
            m_editorMeshComponentDescriptor.reset();
        }

        AZ::EntityId m_entityId1;
        AzToolsFramework::EntityIdList m_entityIds;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_meshComponentDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_editorMeshComponentDescriptor;
        IntersectionNotificationDetector m_intersectionNotificationDetector;
    };

    TEST_F(MeshComponentControllerFixture, IntersectionNotificationBusIsNotifiedWhenMeshComponentControllerTransformIsModified)
    {
        auto* entity1 = AzToolsFramework::GetEntityById(m_entityId1);
        entity1->Deactivate();
        entity1->CreateComponent<AZ::Render::EditorMeshComponent>();
        // note: RPI::Scene::GetFeatureProcessorForEntity<MeshFeatureProcessorInterface>(...) returns nullptr
        // and so m_meshFeatureProcessor is null
        AZ_TEST_START_TRACE_SUPPRESSION;
        entity1->Activate();
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);

        AZ::TransformBus::Event(
            m_entityId1, &AZ::TransformBus::Events::SetWorldTM, AZ::Transform::CreateTranslation(AZ::Vector3(1.0f, 2.0f, 3.0f)));

        using ::testing::Eq;
        EXPECT_THAT(m_entityId1, Eq(m_intersectionNotificationDetector.m_lastEntityIdChanged));
    }
} // namespace UnitTest
