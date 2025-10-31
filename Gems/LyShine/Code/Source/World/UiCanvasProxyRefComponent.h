/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <LyShine/Bus/World/UiCanvasRefBus.h>
#include <LyShine/UiAssetTypes.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiCanvasProxyRefComponent
    : public AZ::Component
    , public UiCanvasRefBus::Handler
    , public UiCanvasProxyRefBus::Handler
    , public UiCanvasRefNotificationBus::Handler
{
public: // member functions

    AZ_COMPONENT(UiCanvasProxyRefComponent, "{D89FD4F1-77C6-4977-A292-6DBA783F1A9A}");

    UiCanvasProxyRefComponent();

    // UiCanvasRefInterface
    AZ::EntityId GetCanvas() override;
    // ~UiCanvasRefInterface

    // UiCanvasProxyRefInterface
    void SetCanvasRefEntity(AZ::EntityId canvasAssetRefEntity) override;
    // ~UiCanvasProxyRefInterface

    // UiCanvasRefListener
    void OnCanvasRefChanged(AZ::EntityId uiCanvasRefEntity, AZ::EntityId uiCanvasEntity) override;
    // ~UiCanvasRefListener

public: // static member functions

    static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("UiCanvasRefService"));
    }

    static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("UiCanvasRefService"));
    }

    static void GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    static void Reflect(AZ::ReflectContext* context);

protected: // member functions

    // AZ::Component
    void Activate() override;
    void Deactivate() override;
    // ~AZ::Component

    AZ_DISABLE_COPY_MOVE(UiCanvasProxyRefComponent);

protected: // data

    //! Persistent properties

    //! The entity that holds the canvas asset ref that we are a proxy for
    AZ::EntityId m_canvasAssetRefEntityId;
};
