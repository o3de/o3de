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
#include "LyShine_precompiled.h"
#include "UiDropTargetComponent.h"

#include <AzCore/Math/Crc.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiDraggableBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
//! UiDropTargetNotificationBus Behavior context handler class
class UiDropTargetNotificationBusBehaviorHandler
    : public UiDropTargetNotificationBus::Handler
    , public AZ::BehaviorEBusHandler
{
public:
    AZ_EBUS_BEHAVIOR_BINDER(UiDropTargetNotificationBusBehaviorHandler, "{B01A3FB5-52E1-4FF4-A627-088DA37A1304}", AZ::SystemAllocator,
        OnDropHoverStart, OnDropHoverEnd, OnDrop);

    void OnDropHoverStart(AZ::EntityId draggable) override
    {
        Call(FN_OnDropHoverStart, draggable);
    }

    void OnDropHoverEnd(AZ::EntityId draggable) override
    {
        Call(FN_OnDropHoverEnd, draggable);
    }

    void OnDrop(AZ::EntityId draggable) override
    {
        Call(FN_OnDrop, draggable);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiDropTargetComponent::UiDropTargetComponent()
{
    // Must be called in the same order as the states defined in UiDropTargetInterface
    m_stateActionManager.AddState(nullptr); // normal state has no state actions
    m_stateActionManager.AddState(&m_dropValidStateActions);
    m_stateActionManager.AddState(&m_dropInvalidStateActions);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiDropTargetComponent::~UiDropTargetComponent()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const LyShine::ActionName& UiDropTargetComponent::GetOnDropActionName()
{
    return m_onDropActionName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDropTargetComponent::SetOnDropActionName(const LyShine::ActionName& actionName)
{
    m_onDropActionName = actionName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDropTargetComponent::HandleDropHoverStart(AZ::EntityId draggable)
{
    EBUS_QUEUE_EVENT_ID(GetEntityId(), UiDropTargetNotificationBus, OnDropHoverStart, draggable);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDropTargetComponent::HandleDropHoverEnd(AZ::EntityId draggable)
{
    EBUS_QUEUE_EVENT_ID(GetEntityId(), UiDropTargetNotificationBus, OnDropHoverEnd, draggable);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDropTargetComponent::HandleDrop(AZ::EntityId draggable)
{
    EBUS_QUEUE_EVENT_ID(GetEntityId(), UiDropTargetNotificationBus, OnDrop, draggable);

    // Check to see if the draggable is a proxy
    bool isProxy = false;
    EBUS_EVENT_ID_RESULT(isProxy, draggable, UiDraggableBus, IsProxy);

    // Tell any action listeners about the event
    // Don't do this for proxy draggables though. A proxy draggable always calls HandleDrop on the original
    // and we don't want this action triggered twice.
    if (!m_onDropActionName.empty() && !isProxy)
    {
        AZ::EntityId canvasEntityId;
        EBUS_EVENT_ID_RESULT(canvasEntityId, GetEntityId(), UiElementBus, GetCanvasEntityId);
        EBUS_EVENT_ID(canvasEntityId, UiCanvasNotificationBus, OnAction, GetEntityId(), m_onDropActionName);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiDropTargetInterface::DropState UiDropTargetComponent::GetDropState()
{
    return m_dropState;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDropTargetComponent::SetDropState(DropState dropState)
{
    if (dropState != m_dropState)
    {
        m_stateActionManager.ResetAllOverrides();
        m_stateActionManager.ApplyStateActions(dropState);
        m_dropState = dropState;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDropTargetComponent::Init()
{
    m_stateActionManager.Init(GetEntityId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDropTargetComponent::Activate()
{
    m_stateActionManager.Activate();
    m_navigationSettings.Activate(m_entity->GetId(), GetNavigableDropTargets);
    UiDropTargetBus::Handler::BusConnect(m_entity->GetId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDropTargetComponent::Deactivate()
{
    m_stateActionManager.Deactivate();
    m_navigationSettings.Deactivate();
    UiDropTargetBus::Handler::BusDisconnect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDropTargetComponent::OnDropValidStateActionsChanged()
{
    m_stateActionManager.InitInteractableEntityForStateActions(m_dropValidStateActions);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDropTargetComponent::OnDropInvalidStateActionsChanged()
{
    m_stateActionManager.InitInteractableEntityForStateActions(m_dropInvalidStateActions);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDropTargetComponent::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

    if (serializeContext)
    {
        serializeContext->Class<UiDropTargetComponent, AZ::Component>()
            ->Version(1)
            ->Field("DropValidStateActions", &UiDropTargetComponent::m_dropValidStateActions)
            ->Field("DropInvalidStateActions", &UiDropTargetComponent::m_dropInvalidStateActions)
            ->Field("NavigationSettings", &UiDropTargetComponent::m_navigationSettings)
            ->Field("OnDropActionName", &UiDropTargetComponent::m_onDropActionName);

        AZ::EditContext* ec = serializeContext->GetEditContext();
        if (ec)
        {
            auto editInfo = ec->Class<UiDropTargetComponent>("DropTarget", "A target component for drag and drop behavior");

            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Category, "UI")
                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/UiDropTarget.png")
                ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/UiDropTarget.png")
                ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("UI", 0x27ff46b0))
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            // Navigation settings
            editInfo->DataElement(0, &UiDropTargetComponent::m_navigationSettings, "Navigation",
                "How to navigate from this drop target to the next drop target");

            // Drop states group
            {
                editInfo->ClassElement(AZ::Edit::ClassElements::Group, "Drop States")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                editInfo->DataElement(0, &UiDropTargetComponent::m_dropValidStateActions, "Valid", "The valid drop state actions")
                    ->Attribute(AZ::Edit::Attributes::AddNotify, &UiDropTargetComponent::OnDropValidStateActionsChanged);

                editInfo->DataElement(0, &UiDropTargetComponent::m_dropInvalidStateActions, "Invalid", "The invalid drop state actions")
                    ->Attribute(AZ::Edit::Attributes::AddNotify, &UiDropTargetComponent::OnDropInvalidStateActionsChanged);
            }

            // Actions group
            {
                editInfo->ClassElement(AZ::Edit::ClassElements::Group, "Actions")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                editInfo->DataElement(0, &UiDropTargetComponent::m_onDropActionName, "OnDrop",
                    "The action name triggered when a draggable is dropped on the drop target");
            }
        }
    }

    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
    if (behaviorContext)
    {
        behaviorContext->Enum<(int)DropStateNormal>("eUiDropState_Normal")
            ->Enum<(int)DropStateValid>("eUiDropState_Valid")
            ->Enum<(int)DropStateInvalid>("eUiDropState_Invalid");

        behaviorContext->EBus<UiDropTargetBus>("UiDropTargetBus")
            ->Event("GetOnDropActionName", &UiDropTargetBus::Events::GetOnDropActionName)
            ->Event("SetOnDropActionName", &UiDropTargetBus::Events::SetOnDropActionName)
            ->Event("GetDropState", &UiDropTargetBus::Events::GetDropState)
            ->Event("SetDropState", &UiDropTargetBus::Events::SetDropState);

        behaviorContext->EBus<UiDropTargetNotificationBus>("UiDropTargetNotificationBus")
            ->Handler<UiDropTargetNotificationBusBehaviorHandler>();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
LyShine::EntityArray UiDropTargetComponent::GetNavigableDropTargets(AZ::EntityId entityId)
{
    // Get a list of all navigable elements
    AZ::EntityId canvasEntityId;
    EBUS_EVENT_ID_RESULT(canvasEntityId, entityId, UiElementBus, GetCanvasEntityId);
    LyShine::EntityArray navigableElements;
    EBUS_EVENT_ID(canvasEntityId, UiCanvasBus, FindElements,
        [entityId](const AZ::Entity* entity)
        {
            bool navigable = false;
            if (entity->GetId() != entityId)
            {
                if (UiDropTargetBus::FindFirstHandler(entity->GetId()))
                {
                    UiNavigationInterface::NavigationMode navigationMode = UiNavigationInterface::NavigationMode::None;
                    EBUS_EVENT_ID_RESULT(navigationMode, entity->GetId(), UiNavigationBus, GetNavigationMode);
                    navigable = (navigationMode != UiNavigationInterface::NavigationMode::None);
                }
            }
            return navigable;
        },
        navigableElements);

    return navigableElements;
}
