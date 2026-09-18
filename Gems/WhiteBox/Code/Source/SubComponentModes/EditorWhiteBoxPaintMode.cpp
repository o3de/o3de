/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#include "SubComponentModes/EditorWhiteBoxPaintMode.h"
#include "EditorWhiteBoxComponent.h"
#include "Util/WhiteBoxMeshUtil.h"

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/NonUniformScaleBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/IntersectSegment.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Viewport/ActionBus.h>

namespace WhiteBox
{
    PaintMode::PaintMode(const AZ::EntityComponentIdPair& entityComponentIdPair)
        : m_entityComponentIdPair(entityComponentIdPair)
    {
    }

    PaintMode::~PaintMode()
    {
        FinishStroke(true);
    }

    EditorWhiteBoxComponent* PaintMode::Component() const
    {
        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(
            entity, &AZ::ComponentApplicationRequests::FindEntity, m_entityComponentIdPair.GetEntityId());
        return entity ? azrtti_cast<EditorWhiteBoxComponent*>(entity->FindComponent(m_entityComponentIdPair.GetComponentId())) : nullptr;
    }

    AZ::Vector3 PaintMode::WorldPoint(EditorWhiteBoxComponent& component, const AZ::Vector3& point) const
    {
        AZ::Vector3 result = point;
        if (component.GetLayerCount() > 0)
        {
            const auto layer = component.GetLayerMeta(component.GetActiveLayerIndex());
            result = layer.m_position + AZ::Quaternion::CreateFromEulerAnglesDegrees(layer.m_rotation)
                .TransformVector(result * layer.m_scale);
        }
        AZ::Vector3 scale = AZ::Vector3::CreateOne();
        AZ::NonUniformScaleRequestBus::EventResult(
            scale, component.GetEntityId(), &AZ::NonUniformScaleRequests::GetScale);
        AZ::Transform world = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(world, component.GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
        return world.TransformPoint(result * scale);
    }

    Api::FaceHandle PaintMode::PickFace(EditorWhiteBoxComponent& component, const ModeMouseInteraction& mouse) const
    {
        WhiteBoxMesh* mesh = component.GetWhiteBoxMesh();
        if (!mesh || (component.GetLayerCount() > 0 &&
            !component.GetLayerMeta(component.GetActiveLayerIndex()).m_visible))
        {
            return {};
        }
        const auto& pick = mouse.m_mouseInteraction.m_mouseInteraction.m_mousePick;
        // Resolve layer/entity transforms once per pick, rather than issuing buses for every vertex.
        const auto layer = component.GetLayerMeta(component.GetActiveLayerIndex());
        const auto rotation = AZ::Quaternion::CreateFromEulerAnglesDegrees(layer.m_rotation);
        AZ::Vector3 scale = AZ::Vector3::CreateOne();
        AZ::NonUniformScaleRequestBus::EventResult(
            scale, component.GetEntityId(), &AZ::NonUniformScaleRequests::GetScale);
        AZ::Transform world = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(world, component.GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
        const auto worldPoint = [&](const AZ::Vector3& point)
        {
            return world.TransformPoint((layer.m_position + rotation.TransformVector(point * layer.m_scale)) * scale);
        };
        AZ::Intersect::SegmentTriangleHitTester hitTester(pick.m_rayOrigin, pick.m_rayOrigin + pick.m_rayDirection * 100000.0f);
        float nearest = 1.0f;
        Api::FaceHandle hit;
        for (const Api::FaceHandle face : Api::MeshFaceHandles(*mesh))
        {
            const auto vertices = Api::FaceVertexPositions(*mesh, face);
            if (vertices.size() != 3)
            {
                continue;
            }
            float distance = 0.0f;
            AZ::Vector3 normal;
            const auto a = worldPoint(vertices[0]);
            const auto b = worldPoint(vertices[1]);
            const auto c = worldPoint(vertices[2]);
            // Both windings are editable, including inverted room layers.
            if (hitTester.IntersectSegmentTriangle(a, b, c, normal, distance) && distance < nearest)
            {
                nearest = distance;
                hit = face;
            }
        }
        return hit;
    }

    bool PaintMode::HandleMouseInteraction(const ModeMouseInteraction& mouse)
    {
        namespace Viewport = AzToolsFramework::ViewportInteraction;
        const auto& event = mouse.m_mouseInteraction;
        if (event.m_mouseEvent == Viewport::MouseEvent::Down && event.m_mouseInteraction.m_mouseButtons.Right())
        {
            return CancelActiveDrag();
        }
        if (m_snapshot && (event.m_mouseEvent == Viewport::MouseEvent::Up ||
            (event.m_mouseEvent == Viewport::MouseEvent::Move && !event.m_mouseInteraction.m_mouseButtons.Left())))
        {
            FinishStroke(true);
            return true;
        }
        auto* component = Component();
        if (!component)
        {
            return false;
        }
        m_hover = PickFace(*component, mouse);
        if (event.m_mouseInteraction.m_keyboardModifiers.Alt())
        {
            return false; // viewport camera navigation
        }
        if (event.m_mouseEvent == Viewport::MouseEvent::Down && event.m_mouseInteraction.m_mouseButtons.Left() &&
            m_hover.IsValid() && !m_snapshot)
        {
            m_settings = component->GetFacePaintSettings();
            if (m_settings.m_operation == FacePaintOperation::Material && !m_settings.m_material.IsValid())
            {
                return true;
            }
            m_strokeMesh = component->GetWhiteBoxMesh();
            m_snapshot = Api::CloneMesh(*m_strokeMesh);
            if (!m_snapshot)
            {
                m_strokeMesh = nullptr;
                return true;
            }
            m_undo = AZStd::make_unique<AzToolsFramework::ScopedUndoBatch>("White Box Vertex Paint");
        }
        if (m_snapshot && m_hover.IsValid())
        {
            Paint(*component, m_hover);
        }
        return m_snapshot != nullptr;
    }

    void PaintMode::Paint(EditorWhiteBoxComponent& component, const Api::FaceHandle face)
    {
        if (component.GetWhiteBoxMesh() != m_strokeMesh ||
            AZStd::find(m_paintedFaces.begin(), m_paintedFaces.end(), face) != m_paintedFaces.end())
        {
            return;
        }
        m_paintedFaces.push_back(face);
        if (m_settings.m_operation == FacePaintOperation::Material || m_settings.m_operation == FacePaintOperation::ResetMaterial)
        {
            const AZ::Data::AssetId material = m_settings.m_operation == FacePaintOperation::Material
                ? m_settings.m_material : AZ::Data::AssetId{};
            if (Api::FaceMaterial(*m_strokeMesh, face) == material)
            {
                return;
            }
            Api::SetFaceMaterial(*m_strokeMesh, face, material);
        }
        else
        {
            const AZ::u32 color = m_settings.m_operation == FacePaintOperation::Color ? m_settings.m_color : 0;
            if (Api::FacePaintColor(*m_strokeMesh, face) == color)
            {
                return;
            }
            Api::SetFacePaintColor(*m_strokeMesh, face, color);
        }
        m_changed = true;
        NotifyMeshChanged();
    }

    void PaintMode::NotifyMeshChanged() const
    {
        EditorWhiteBoxComponentNotificationBus::Event(
            m_entityComponentIdPair, &EditorWhiteBoxComponentNotifications::OnWhiteBoxMeshModified);
    }

    void PaintMode::FinishStroke(const bool commit)
    {
        if (!m_snapshot)
        {
            return;
        }
        auto* component = Component();
        if (component && component->GetWhiteBoxMesh() == m_strokeMesh && m_changed)
        {
            if (commit)
            {
                component->BakeParametricLayer(component->GetActiveLayerIndex());
                component->SerializeWhiteBox();
                NotifyMeshChanged();
                m_undo->MarkEntityDirty(component->GetEntityId());
            }
            else if (RestoreMeshFromSnapshot(*m_strokeMesh, *m_snapshot))
            {
                NotifyMeshChanged();
            }
        }
        m_snapshot.reset();
        m_strokeMesh = nullptr;
        m_paintedFaces.clear();
        m_changed = false;
        m_undo.reset();
    }

    bool PaintMode::CancelActiveDrag()
    {
        const bool active = m_snapshot != nullptr;
        FinishStroke(false);
        return active;
    }

    void PaintMode::Refresh()
    {
        CancelActiveDrag();
        m_hover = {};
    }

    AZStd::vector<AzToolsFramework::ActionOverride> PaintMode::PopulateActions(const AZ::EntityComponentIdPair&)
    {
        return {};
    }

    void PaintMode::Display(
        const AZ::EntityComponentIdPair&, const AZ::Transform&, const IntersectionAndRenderData&,
        const AzFramework::ViewportInfo&, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        auto* component = Component();
        if (!component || !m_hover.IsValid())
        {
            return;
        }
        WhiteBoxMesh* mesh = component->GetWhiteBoxMesh();
        if (!mesh)
        {
            return;
        }
        const auto faces = Api::MeshFaceHandles(*mesh);
        if (AZStd::find(faces.begin(), faces.end(), m_hover) == faces.end())
        {
            return;
        }
        const auto vertices = Api::FaceVertexPositions(*mesh, m_hover);
        if (vertices.size() == 3)
        {
            debugDisplay.DepthTestOff();
            debugDisplay.SetColor(AZ::Color(1.0f, 0.75f, 0.1f, 1.0f));
            debugDisplay.SetLineWidth(2.0f);
            for (size_t edge = 0; edge < 3; ++edge)
            {
                debugDisplay.DrawLine(WorldPoint(*component, vertices[edge]), WorldPoint(*component, vertices[(edge + 1) % 3]));
            }
            debugDisplay.DepthTestOn();
        }
    }
}
