/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <LyShine/Bus/World/UiCanvasRefBus.h>
#include <LyShine/Bus/World/UiCanvasOnMeshBus.h>
#include <LyShine/Bus/UiCanvasManagerBus.h>
#include <AzCore/Math/Vector3.h>

struct IPhysicalEntity;

namespace AzFramework
{
    namespace RenderGeometry
    {
        struct RayResult;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiCanvasOnMeshComponent
    : public AZ::Component
    , public UiCanvasOnMeshBus::Handler
    , public UiCanvasAssetRefNotificationBus::Handler
    , public UiCanvasManagerNotificationBus::Handler
{
public: // member functions

    AZ_COMPONENT(UiCanvasOnMeshComponent, "{0C1B2542-6813-451A-BD11-42F92DD48E36}");

    UiCanvasOnMeshComponent();

    // UiCanvasOnMeshInterface
    bool ProcessHitInputEvent(const AzFramework::InputChannel::Snapshot& inputSnapshot, const AzFramework::RenderGeometry::RayResult& rayResult) override;
    // ~UiCanvasOnMeshInterface


    // UiCanvasAssetRefListener
    void OnCanvasLoadedIntoEntity(AZ::EntityId uiCanvasEntity) override;
    // ~UiCanvasAssetRefListener

    // UiCanvasManagerNotification
    void OnCanvasReloaded(AZ::EntityId canvasEntityId) override;
    // ~UiCanvasManagerNotification

public: // static member functions

    static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("UiCanvasOnMeshService", 0xd2539f92));
    }

    static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("UiCanvasOnMeshService", 0xd2539f92));
    }

    static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("MeshService", 0x71d8a455));
        required.push_back(AZ_CRC("UiCanvasRefService", 0xb4cb5ef4));
    }

    static void Reflect(AZ::ReflectContext* context);

protected: // member functions

    // AZ::Component
    void Activate() override;
    void Deactivate() override;
    // ~AZ::Component

    bool ProcessCollisionInputEventInternal(const AzFramework::InputChannel::Snapshot& inputSnapshot, const AzFramework::RenderGeometry::RayResult& rayResult);

    AZ::EntityId GetCanvas();

    AZ_DISABLE_COPY_MOVE(UiCanvasOnMeshComponent);

protected: // data

    //! Render target name to use (overrides the render target name in the UI canvas)
    AZStd::string m_renderTargetOverride;
};
