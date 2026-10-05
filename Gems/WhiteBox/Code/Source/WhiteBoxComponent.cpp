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
                ->Version(3)
                ->Field("WhiteBoxRenderData", &WhiteBoxComponent::m_whiteBoxRenderData)
                ->Field("BooleanRenderData", &WhiteBoxComponent::m_booleanRenderData)
                ->Field("HasBooleanMesh", &WhiteBoxComponent::m_hasBooleanMesh)
                ->Field("LiveBoolean", &WhiteBoxComponent::m_liveBoolean)
                ->Field("PhysicsRenderData", &WhiteBoxComponent::m_physicsRenderData)
                ->Field("BooleanPhysicsRenderData", &WhiteBoxComponent::m_booleanPhysicsRenderData)
                ->Field("HasPhysicsData", &WhiteBoxComponent::m_hasPhysicsData);
        }

        // Reflected here (Common scope), not in the Editor-only reflection, so runtime Lua can use it.
        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<WhiteBoxComponentRequestBus>("WhiteBoxComponentRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "whitebox.request.bus")
                ->Event("WhiteBoxIsVisible", &WhiteBoxComponentRequestBus::Events::WhiteBoxIsVisible)
                ->Event("SetWhiteBoxVisible", &WhiteBoxComponentRequestBus::Events::SetWhiteBoxVisible)
                ->Event("SetLiveBoolean", &WhiteBoxComponentRequestBus::Events::SetLiveBoolean)
                ->Event("GetLiveBoolean", &WhiteBoxComponentRequestBus::Events::GetLiveBoolean)
                ->Event(
                    "BakeWhiteBox", &WhiteBoxComponentRequestBus::Events::BakeWhiteBox,
                    {{AZ::BehaviorParameterOverrides(
                        "rebakeCollider",
                        "When true, rebuild the physics collider from the entity's current size/scale",
                        behaviorContext->MakeDefaultValue(false))}});
        }
    }

    void WhiteBoxComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        // The game Material component requires a consumer, as the editor one does.
        provided.push_back(AZ_CRC_CE("MaterialConsumerService"));
    }

    void WhiteBoxComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("MaterialConsumerService"));
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

        // Re-apply the transform (which pulls in the current non-uniform scale) when the entity's
        // Non-Uniform Scale component changes - no transform notification is raised for it.
        m_nonUniformScaleChangedHandler = AZ::NonUniformScaleChangedEvent::Handler(
            [this]([[maybe_unused]] const AZ::Vector3& scale)
            {
                if (m_renderMesh)
                {
                    AZ::Transform worldFromLocal = AZ::Transform::CreateIdentity();
                    AZ::TransformBus::EventResult(worldFromLocal, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
                    m_renderMesh->UpdateTransform(worldFromLocal);
                }
            });
        AZ::NonUniformScaleRequestBus::Event(
            entityId, &AZ::NonUniformScaleRequests::RegisterScaleChangedEvent, m_nonUniformScaleChangedHandler);
    }

    void WhiteBoxComponent::Deactivate()
    {
        m_nonUniformScaleChangedHandler.Disconnect();
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

    void WhiteBoxComponent::BakeWhiteBox(const bool rebakeCollider)
    {
        // apply the current selection to the render mesh ...
        RebuildRenderMesh();

        // ... and the collider (forced when rebakeCollider, to pick up the current scale).
        WhiteBoxColliderRequestBus::Event(
            GetEntityId(), &WhiteBoxColliderRequests::BakeCollider, m_liveBoolean, rebakeCollider);
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

    void WhiteBoxComponent::SetWhiteBoxVisible(const bool visible)
    {
        if (m_renderMesh)
        {
            m_renderMesh->SetVisiblity(visible);
        }
    }

    // Implement the setters:
    void WhiteBoxComponent::SetPhysicsGeometryData(const WhiteBoxRenderData& physicsData)
    {
        m_physicsRenderData = physicsData;
        // Mark that authoritative physics data was supplied. Even an empty result is meaningful
        // (collision disabled for every layer) and must not fall back to the visual geometry.
        m_hasPhysicsData = true;
    }

    void WhiteBoxComponent::SetBooleanPhysicsGeometryData(const WhiteBoxRenderData& physicsData)
    {
        m_booleanPhysicsRenderData = physicsData;
    }

    // Implement the getter for the physics collider:
    const WhiteBoxRenderData& WhiteBoxComponent::GetPhysicsRenderData() const
    {
        // If the live boolean is toggled ON and we have valid boolean physics data, use it.
        if (m_liveBoolean && m_hasBooleanMesh && !m_booleanPhysicsRenderData.m_faces.empty())
        {
            return m_booleanPhysicsRenderData;
        }

        // Baked physics data is authoritative even when empty (collision off).
        if (m_hasPhysicsData)
        {
            return m_physicsRenderData;
        }

        // Failsafe: only legacy data (saved before physics geometry was baked) reaches here -
        // fall back to the visual geometry. ActiveRenderData() handles base vs boolean automatically.
        return ActiveRenderData();
    }


} // namespace WhiteBox
