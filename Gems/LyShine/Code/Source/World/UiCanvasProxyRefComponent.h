/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
        provided.push_back(AZ_CRC("UiCanvasRefService", 0xb4cb5ef4));
    }

    static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("UiCanvasRefService", 0xb4cb5ef4));
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
