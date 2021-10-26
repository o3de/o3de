/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorPolygonPrismShapeComponentMode.h"

#include <AzToolsFramework/Manipulators/LineHoverSelection.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>

#include <AzCore/Component/NonUniformScaleBus.h>

namespace LmbrCentral
{
    AZ_CLASS_ALLOCATOR_IMPL(EditorPolygonPrismShapeComponentMode, AZ::SystemAllocator, 0)

    /// Util to calculate central position of prism (to draw the height manipulator)
    static AZ::Vector3 CalculateHeightManipulatorPosition(const AZ::PolygonPrism& polygonPrism)
    {
        const float height = polygonPrism.GetHeight();
        const AZ::VertexContainer<AZ::Vector2>& vertexContainer = polygonPrism.m_vertexContainer;

        AzToolsFramework::MidpointCalculator midpointCalculator;
        for (const AZ::Vector2& vertex : vertexContainer.GetVertices())
        {
            midpointCalculator.AddPosition(AZ::Vector2ToVector3(vertex, height));
        }

        return midpointCalculator.CalculateMidpoint();
    }

    EditorPolygonPrismShapeComponentMode::EditorPolygonPrismShapeComponentMode(
        const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Uuid componentType)
        : EditorBaseComponentMode(entityComponentIdPair, componentType)
        , m_nonUniformScaleChangedHandler([this](const AZ::Vector3& scale){OnNonUniformScaleChanged(scale);})
    {
        m_currentTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(
            m_currentTransform, entityComponentIdPair.GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

        m_currentNonUniformScale = AZ::Vector3::CreateOne();
        AZ::NonUniformScaleRequestBus::EventResult(m_currentNonUniformScale, GetEntityId(), &AZ::NonUniformScaleRequests::GetScale);

        AZ::NonUniformScaleRequestBus::Event(entityComponentIdPair.GetEntityId(),
            &AZ::NonUniformScaleRequests::RegisterScaleChangedEvent, m_nonUniformScaleChangedHandler);

        AZ::TransformNotificationBus::Handler::BusConnect(entityComponentIdPair.GetEntityId());
        PolygonPrismShapeComponentNotificationBus::Handler::BusConnect(entityComponentIdPair.GetEntityId());
        ShapeComponentNotificationsBus::Handler::BusConnect(entityComponentIdPair.GetEntityId());

        CreateManipulators();
    }

    EditorPolygonPrismShapeComponentMode::~EditorPolygonPrismShapeComponentMode()
    {
        ShapeComponentNotificationsBus::Handler::BusDisconnect();
        PolygonPrismShapeComponentNotificationBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        m_nonUniformScaleChangedHandler.Disconnect();

        DestroyManipulators();
    }

    void EditorPolygonPrismShapeComponentMode::Refresh()
    {
        ContainerChanged();
    }

    AZStd::vector<AzToolsFramework::ActionOverride> EditorPolygonPrismShapeComponentMode::PopulateActionsImpl()
    {
        return m_vertexSelection.ActionOverrides();
    }

    bool EditorPolygonPrismShapeComponentMode::HandleMouseInteraction(
        const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        return m_vertexSelection.HandleMouse(mouseInteraction);
    }

    void EditorPolygonPrismShapeComponentMode::CreateManipulators()
    {
        using namespace AzToolsFramework;

        AZ::PolygonPrismPtr polygonPrism = nullptr;
        PolygonPrismShapeComponentRequestBus::EventResult(
            polygonPrism, GetEntityId(), &PolygonPrismShapeComponentRequests::GetPolygonPrism);

        // if we have no vertices, do not attempt to create any manipulators
        if (polygonPrism->m_vertexContainer.Empty())
        {
            return;
        }

        m_vertexSelection.Create(
            GetEntityComponentIdPair(), g_mainManipulatorManagerId,
            AZStd::make_unique<LineSegmentHoverSelection<AZ::Vector2>>(
                GetEntityComponentIdPair(), g_mainManipulatorManagerId),
            TranslationManipulators::Dimensions::Two,
            ConfigureTranslationManipulatorAppearance2d);

        // callback after vertices in the selection have moved
        m_vertexSelection.SetVertexPositionsUpdatedCallback([this, polygonPrism]()
        {
            // ensure we refresh the height manipulator after vertices are moved to ensure it stays central to the prism
            m_heightManipulator->SetLocalTransform(
                AZ::Transform::CreateTranslation(CalculateHeightManipulatorPosition(*polygonPrism)));
            m_heightManipulator->SetBoundsDirty();
        });

        // initialize height manipulator
        m_heightManipulator = LinearManipulator::MakeShared(m_currentTransform);
        m_heightManipulator->AddEntityComponentIdPair(GetEntityComponentIdPair());
        m_heightManipulator->SetSpace(AzToolsFramework::TransformUniformScale(m_currentTransform));
        m_heightManipulator->SetNonUniformScale(m_currentNonUniformScale);
        m_heightManipulator->SetLocalTransform(
            AZ::Transform::CreateTranslation(CalculateHeightManipulatorPosition(*polygonPrism)));
        m_heightManipulator->SetAxis(AZ::Vector3::CreateAxisZ());

        const float lineLength = 0.5f;
        const float coneLength = 0.28f;
        const float coneRadius = 0.07f;
        ManipulatorViews views;
        views.emplace_back(CreateManipulatorViewLine(
            *m_heightManipulator, AZ::Color(0.0f, 0.0f, 1.0f, 1.0f),
            lineLength, AzToolsFramework::ManipulatorLineBoundWidth()));
        views.emplace_back(CreateManipulatorViewCone(*m_heightManipulator,
            AZ::Color(0.0f, 0.0f, 1.0f, 1.0f), m_heightManipulator->GetAxis() *
            (lineLength - coneLength), coneLength, coneRadius));
        m_heightManipulator->SetViews(AZStd::move(views));

        // height manipulator callbacks
        m_heightManipulator->InstallMouseMoveCallback([this, polygonPrism](
            const LinearManipulator::Action& action)
        {
            polygonPrism->SetHeight(AZ::GetMax<float>(action.LocalPosition().GetZ(), 0.0f));
            m_heightManipulator->SetLocalTransform(
                AZ::Transform::CreateTranslation(Vector2ToVector3(Vector3ToVector2(
                    action.LocalPosition()), AZ::GetMax<float>(0.0f, action.LocalPosition().GetZ()))));
            m_heightManipulator->SetBoundsDirty();
        });

        m_heightManipulator->Register(g_mainManipulatorManagerId);
    }

    void EditorPolygonPrismShapeComponentMode::DestroyManipulators()
    {
        // clear all manipulators when deselected
        if (m_heightManipulator)
        {
            m_heightManipulator->Unregister();
            m_heightManipulator.reset();
        }

        m_vertexSelection.Destroy();
    }

    void EditorPolygonPrismShapeComponentMode::ContainerChanged()
    {
        DestroyManipulators();
        CreateManipulators();
    }

    void EditorPolygonPrismShapeComponentMode::RefreshManipulators()
    {
        m_vertexSelection.RefreshLocal();

        if (m_heightManipulator)
        {
            AZ::PolygonPrismPtr polygonPrism = nullptr;
            PolygonPrismShapeComponentRequestBus::EventResult(
                polygonPrism, GetEntityId(), &PolygonPrismShapeComponentRequests::GetPolygonPrism);

            m_heightManipulator->SetLocalTransform(
                AZ::Transform::CreateTranslation(CalculateHeightManipulatorPosition(*polygonPrism)));
            m_heightManipulator->SetBoundsDirty();
        }
    }

    void EditorPolygonPrismShapeComponentMode::OnTransformChanged(
        const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_currentTransform = world;

        // update the space manipulators are in after the entity has moved
        m_vertexSelection.RefreshSpace(world, m_currentNonUniformScale);

        if (m_heightManipulator)
        {
            m_heightManipulator->SetSpace(world);
        }
    }

    void EditorPolygonPrismShapeComponentMode::OnNonUniformScaleChanged(const AZ::Vector3& scale)
    {
        m_currentNonUniformScale = scale;
        m_vertexSelection.RefreshSpace(m_currentTransform, scale);
        if (m_heightManipulator)
        {
            m_heightManipulator->SetNonUniformScale(scale);
        }
    }

    void EditorPolygonPrismShapeComponentMode::OnShapeChanged(ShapeChangeReasons /*changeReason*/)
    {
        RefreshManipulators();
    }

    void EditorPolygonPrismShapeComponentMode::OnVertexAdded(size_t index)
    {
        ContainerChanged();

        AZ::PolygonPrismPtr polygonPrism = nullptr;
        PolygonPrismShapeComponentRequestBus::EventResult(
            polygonPrism, GetEntityId(), &PolygonPrismShapeComponentRequests::GetPolygonPrism);

        m_vertexSelection.CreateTranslationManipulator(
            GetEntityComponentIdPair(), AzToolsFramework::g_mainManipulatorManagerId,
            polygonPrism->m_vertexContainer.GetVertices()[index], index);
    }

    void EditorPolygonPrismShapeComponentMode::OnVertexRemoved([[maybe_unused]] size_t index)
    {
        ContainerChanged();
    }

    void EditorPolygonPrismShapeComponentMode::OnVerticesSet(const AZStd::vector<AZ::Vector2>& /*vertices*/)
    {
        ContainerChanged();
    }

    void EditorPolygonPrismShapeComponentMode::OnVerticesCleared()
    {
        ContainerChanged();
    }
} // namespace LmbrCentral
