/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "WhiteBoxComponent.h"

#include <AzCore/Component/TransformBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Rendering/WhiteBoxMaterial.h>
#include <Rendering/WhiteBoxRenderData.h>
#include <Rendering/WhiteBoxRenderDataUtil.h>
#include <Rendering/WhiteBoxRenderMeshInterface.h>
#include <WhiteBox/WhiteBoxBus.h>
#include <WhiteBox/WhiteBoxColliderBus.h>

namespace WhiteBox
{
    void WhiteBoxComponent::Reflect(AZ::ReflectContext* context)
    {
        WhiteBoxRenderData::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<WhiteBoxComponent, AZ::Component>()
                ->Version(2)
                ->Field("WhiteBoxRenderData", &WhiteBoxComponent::m_whiteBoxRenderData)
                ->Field("BooleanRenderData", &WhiteBoxComponent::m_booleanRenderData)
                ->Field("HasBooleanMesh", &WhiteBoxComponent::m_hasBooleanMesh)
                ->Field("LiveBoolean", &WhiteBoxComponent::m_liveBoolean);
        }

        // Reflected here (Common scope), not in the Editor-only reflection, so runtime Lua can use it.
        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<WhiteBoxComponentRequestBus>("WhiteBoxComponentRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "whitebox.request.bus")
                ->Event("WhiteBoxIsVisible", &WhiteBoxComponentRequestBus::Events::WhiteBoxIsVisible)
                ->Event("SetLiveBoolean", &WhiteBoxComponentRequestBus::Events::SetLiveBoolean)
                ->Event("GetLiveBoolean", &WhiteBoxComponentRequestBus::Events::GetLiveBoolean)
                ->Event("BakeWhiteBox", &WhiteBoxComponentRequestBus::Events::BakeWhiteBox);
        }
    }

    WhiteBoxComponent::WhiteBoxComponent() = default;
    WhiteBoxComponent::~WhiteBoxComponent() = default;

    void WhiteBoxComponent::Activate()
    {
        WhiteBoxRequestBus::BroadcastResult(m_renderMesh, &WhiteBoxRequests::CreateRenderMeshInterface, GetEntityId());

        const AZ::EntityId entityId = GetEntityId();

        // generate the mesh from the currently selected (base or boolean) render data
        RebuildRenderMesh();

        AzFramework::VisibleGeometryRequestBus::Handler::BusConnect(entityId);
        AZ::TransformNotificationBus::Handler::BusConnect(entityId);
        WhiteBoxComponentRequestBus::Handler::BusConnect(entityId);
    }

    void WhiteBoxComponent::Deactivate()
    {
        WhiteBoxComponentRequestBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        AzFramework::VisibleGeometryRequestBus::Handler::BusDisconnect();

        m_renderMesh.reset();
    }

    const WhiteBoxRenderData& WhiteBoxComponent::ActiveRenderData() const
    {
        return (m_liveBoolean && m_hasBooleanMesh) ? m_booleanRenderData : m_whiteBoxRenderData;
    }

    const WhiteBoxRenderData& WhiteBoxComponent::GetActiveRenderData() const
    {
        return ActiveRenderData();
    }

    void WhiteBoxComponent::RebuildRenderMesh()
    {
        if (!m_renderMesh)
        {
            return;
        }

        AZ::Transform worldFromLocal = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(worldFromLocal, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

        const WhiteBoxRenderData& renderData = ActiveRenderData();
        m_renderMesh->BuildMesh(renderData, worldFromLocal);
        m_renderMesh->UpdateMaterial(renderData.m_material);
        m_renderMesh->SetVisiblity(renderData.m_material.m_visible);
    }

    void WhiteBoxComponent::GenerateWhiteBoxMesh(const WhiteBoxRenderData& whiteBoxRenderData)
    {
        m_whiteBoxRenderData = whiteBoxRenderData;
    }

    void WhiteBoxComponent::SetBooleanRenderData(const WhiteBoxRenderData& booleanRenderData)
    {
        m_booleanRenderData = booleanRenderData;
    }

    void WhiteBoxComponent::SetLiveBooleanState(const bool hasBoolean, const bool live)
    {
        m_hasBooleanMesh = hasBoolean;
        m_liveBoolean = hasBoolean && live;
    }

    void WhiteBoxComponent::SetLiveBoolean(const bool enabled)
    {
        // only allow the boolean variant to be selected if one was baked
        m_liveBoolean = enabled && m_hasBooleanMesh;
    }

    bool WhiteBoxComponent::GetLiveBoolean() const
    {
        return m_liveBoolean;
    }

    void WhiteBoxComponent::BakeWhiteBox()
    {
        // apply the current selection to the render mesh ...
        RebuildRenderMesh();

        // ... and to the physics collider (no-op if this entity has no White Box collider
        // or no boolean collider variant was baked).
        WhiteBoxColliderRequestBus::Event(
            GetEntityId(), &WhiteBoxColliderRequests::BakeCollider, m_liveBoolean);
    }

    void WhiteBoxComponent::OnTransformChanged([[maybe_unused]] const AZ::Transform& local, const AZ::Transform& world)
    {
        m_renderMesh->UpdateTransform(world);
    }

    void WhiteBoxComponent::BuildVisibleGeometry(
        [[maybe_unused]] const AZ::Aabb& bounds, AzFramework::VisibleGeometryContainer& geometryContainer) const
    {
        // Convert the active white box render data into visible geometry data
        const AzFramework::VisibleGeometry geometry =
            BuildVisibleGeometryFromWhiteBoxRenderData(GetEntityId(), ActiveRenderData());

        if (!geometry.m_indices.empty() && !geometry.m_vertices.empty())
        {
            geometryContainer.push_back(geometry);
        }
    }

    bool WhiteBoxComponent::WhiteBoxIsVisible() const
    {
        return m_renderMesh->IsVisible();
    }
} // namespace WhiteBox
