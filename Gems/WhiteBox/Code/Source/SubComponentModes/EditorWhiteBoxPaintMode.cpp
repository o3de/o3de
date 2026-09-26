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
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <cmath>
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

    Api::FaceHandle PaintMode::PickFace(
        EditorWhiteBoxComponent& component, const ModeMouseInteraction& mouse, AZ::Vector3* hitPoint, AZ::Vector3* hitNormal) const
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
                if (hitNormal != nullptr)
                {
                    *hitNormal = (b - a).Cross(c - a).GetNormalizedSafe();
                }
            }
        }
        if (hit.IsValid() && hitPoint != nullptr)
        {
            *hitPoint = pick.m_rayOrigin + pick.m_rayDirection * 100000.0f * nearest;
        }
        return hit;
    }

    void PaintMode::PaintBlend(EditorWhiteBoxComponent& component, const AZ::Vector3& worldPoint)
    {
        if (component.GetWhiteBoxMesh() != m_strokeMesh)
        {
            return;
        }
        const float radius = AZ::GetMax(m_settings.m_brushRadius, 1e-3f);
        const float strength = AZ::GetClamp(m_settings.m_brushStrength, 0.0f, 1.0f);
        const float hardness = AZ::GetClamp(m_settings.m_brushHardness, 0.0f, 0.99f);
        for (const Api::VertexHandle vertex : Api::MeshVertexHandles(*m_strokeMesh))
        {
            const float distance = (WorldPoint(component, Api::VertexPosition(*m_strokeMesh, vertex)) - worldPoint).GetLength();
            if (distance >= radius)
            {
                continue;
            }
            // Full strength inside the hard core, then a smooth fall to nothing at the rim.
            const float t = AZ::GetClamp((distance / radius - hardness) / (1.0f - hardness), 0.0f, 1.0f);
            const float weight = strength * (1.0f - t * t) * (1.0f - t * t);
            if (weight <= 0.0f)
            {
                continue;
            }
            AZ::Vector3 blend = Api::VertexBlend(*m_strokeMesh, vertex);
            const AZ::Vector3 target = m_settings.m_operation == FacePaintOperation::BlendLayer2 ? AZ::Vector3(1.0f, 0.0f, 0.0f)
                : m_settings.m_operation == FacePaintOperation::BlendLayer3                     ? AZ::Vector3(blend.GetX(), 1.0f, 0.0f)
                                                                                                 : AZ::Vector3::CreateZero();
            blend = blend.Lerp(target, weight);
            Api::SetVertexBlend(*m_strokeMesh, vertex, blend);
            m_changed = true;
        }
        if (m_changed)
        {
            NotifyMeshChanged();
        }
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
        m_hover = PickFace(*component, mouse, &m_hoverPoint, &m_hoverNormal);
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
            if (IsBlendOperation(m_settings.m_operation))
            {
                Api::BakeVertexBlend(*m_strokeMesh); // after the snapshot, so undo drops the bake with the stroke
            }
        }
        if (m_snapshot && m_hover.IsValid() && IsBlendOperation(m_settings.m_operation))
        {
            PaintBlend(*component, m_hoverPoint);
        }
        else if (m_snapshot && m_hover.IsValid())
        {
            if (m_settings.m_wholePolygon)
            {
                // The hovered triangle is one of the polygon's; paint the whole face so a quad does not
                // end up half-coloured. Paint() skips faces already done this stroke, so overlap is free.
                const Api::PolygonHandle polygon = Api::FacePolygonHandle(*m_strokeMesh, m_hover);
                for (const Api::FaceHandle face : polygon.m_faceHandles)
                {
                    Paint(*component, face);
                }
            }
            else
            {
                Paint(*component, m_hover);
            }
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
        // Outline what the next click affects (the whole polygon with Whole Polygon on). Read live so
        // toggling updates the hover at once.
        AZStd::vector<AZ::Vector3> outline;
        const FacePaintSettings& live = component->GetFacePaintSettings();
        if (IsBlendOperation(live.m_operation))
        {
            // The brush: a circle of the brush radius lying on the surface, and a smaller one for the hard core.
            const AZ::Vector3 normal = m_hoverNormal.GetNormalizedSafe();
            const AZ::Vector3 side = (AZStd::abs(normal.GetZ()) < 0.9f ? AZ::Vector3::CreateAxisZ() : AZ::Vector3::CreateAxisX()).Cross(normal).GetNormalizedSafe();
            const AZ::Vector3 up = normal.Cross(side);
            const AZ::Vector3 centre = m_hoverPoint + normal * 0.005f;
            for (const float ring : { live.m_brushRadius, live.m_brushRadius * AZ::GetClamp(live.m_brushHardness, 0.0f, 1.0f) })
            {
                if (ring <= 1e-3f)
                {
                    continue;
                }
                constexpr int Segments = 48;
                for (int i = 0; i < Segments; ++i)
                {
                    const float a0 = AZ::Constants::TwoPi * float(i) / float(Segments);
                    const float a1 = AZ::Constants::TwoPi * float(i + 1) / float(Segments);
                    outline.push_back(centre + (side * std::cos(a0) + up * std::sin(a0)) * ring);
                    outline.push_back(centre + (side * std::cos(a1) + up * std::sin(a1)) * ring);
                }
            }
        }
        else if (live.m_wholePolygon)
        {
            const Api::PolygonHandle polygon = Api::FacePolygonHandle(*mesh, m_hover);
            for (const Api::EdgeHandle edgeHandle : Api::PolygonBorderEdgeHandlesFlattened(*mesh, polygon))
            {
                const auto edgeVertices = Api::EdgeVertexPositions(*mesh, edgeHandle);
                outline.push_back(WorldPoint(*component, edgeVertices[0]));
                outline.push_back(WorldPoint(*component, edgeVertices[1]));
            }
        }
        else if (const auto vertices = Api::FaceVertexPositions(*mesh, m_hover); vertices.size() == 3)
        {
            for (size_t edge = 0; edge < 3; ++edge)
            {
                outline.push_back(WorldPoint(*component, vertices[edge]));
                outline.push_back(WorldPoint(*component, vertices[(edge + 1) % 3]));
            }
        }

        if (!outline.empty())
        {
            const AZ::Color highlight(1.0f, 0.75f, 0.1f, 1.0f);
            debugDisplay.DepthTestOff();
            debugDisplay.DepthWriteOff();
            debugDisplay.SetDrawInFrontMode(true);
            debugDisplay.SetColor(highlight);
            debugDisplay.SetLineWidth(2.0f);
            debugDisplay.DrawLines(outline, highlight);
            debugDisplay.DepthTestOn();
        }
    }
}
