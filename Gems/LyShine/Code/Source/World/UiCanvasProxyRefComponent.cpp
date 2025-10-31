/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiCanvasProxyRefComponent.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <LyShine/Bus/UiCanvasBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiCanvasProxyRefComponent::UiCanvasProxyRefComponent()
{}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiCanvasProxyRefComponent::GetCanvas()
{
    AZ::EntityId uiCanvasEntityId;

    if (m_canvasAssetRefEntityId.IsValid())
    {
        UiCanvasRefBus::EventResult(uiCanvasEntityId, m_canvasAssetRefEntityId, &UiCanvasRefBus::Events::GetCanvas);
    }

    return uiCanvasEntityId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasProxyRefComponent::SetCanvasRefEntity(AZ::EntityId canvasAssetRefEntity)
{
    m_canvasAssetRefEntityId = canvasAssetRefEntity;

    AZ::EntityId uiCanvasEntityId = GetCanvas();

    UiCanvasRefNotificationBus::Event(
        GetEntityId(), &UiCanvasRefNotificationBus::Events::OnCanvasRefChanged, GetEntityId(), uiCanvasEntityId);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasProxyRefComponent::OnCanvasRefChanged([[maybe_unused]] AZ::EntityId uiCanvasRefEntity, AZ::EntityId uiCanvasEntity)
{
    UiCanvasRefNotificationBus::Event(
        GetEntityId(), &UiCanvasRefNotificationBus::Events::OnCanvasRefChanged, GetEntityId(), uiCanvasEntity);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasProxyRefComponent::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

    if (serializeContext)
    {
        serializeContext->Class<UiCanvasProxyRefComponent, AZ::Component>()
            ->Version(1)
            ->Field("CanvasAssetRefEntity", &UiCanvasProxyRefComponent::m_canvasAssetRefEntityId);

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (editContext)
        {
            auto editInfo = editContext->Class<UiCanvasProxyRefComponent>("UI Canvas Proxy Ref", "The UI Canvas Proxy Ref component allows you to associate an entity with another entity that is managing a UI Canvas");

            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Category, "UI")
                ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/UiCanvasProxyRef.svg")
                ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/UiCanvasProxyRef.svg")
                ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/ui/canvas-proxy-ref/")
                ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"));

            editInfo->DataElement(0, &UiCanvasProxyRefComponent::m_canvasAssetRefEntityId,
                "Canvas Asset Ref entity", "The entity that holds the UI Canvas Asset Ref component.");
        }
    }

    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
    if (behaviorContext)
    {
        behaviorContext->EBus<UiCanvasProxyRefBus>("UiCanvasProxyRefBus")
            ->Event("SetCanvasRefEntity", &UiCanvasProxyRefBus::Events::SetCanvasRefEntity);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasProxyRefComponent::Activate()
{
    UiCanvasRefBus::Handler::BusConnect(GetEntityId());
    UiCanvasProxyRefBus::Handler::BusConnect(GetEntityId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasProxyRefComponent::Deactivate()
{
    UiCanvasProxyRefBus::Handler::BusDisconnect();
    UiCanvasRefBus::Handler::BusDisconnect();
}
