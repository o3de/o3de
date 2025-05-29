/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiButtonComponent.h"
#include "Sprite.h"

#include <AzCore/Math/Crc.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiTransformBus.h>
#include <LyShine/Bus/UiVisualBus.h>
#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/ISprite.h>

#include <LyShine/IDraw2d.h>
#include <LyShine/UiSerializeHelpers.h>

#include "UiSerialize.h"
#include "Sprite.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
//! UiButtonNotificationBus Behavior context handler class
class UiButtonNotificationBusBehaviorHandler
    : public UiButtonNotificationBus::Handler
    , public AZ::BehaviorEBusHandler
{
public:
    AZ_EBUS_BEHAVIOR_BINDER(UiButtonNotificationBusBehaviorHandler, "{8CB61B57-8A99-46AE-ABAC-23384FA5BC96}", AZ::SystemAllocator,
        OnButtonClick);

    void OnButtonClick() override
    {
        Call(FN_OnButtonClick);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiButtonComponent::UiButtonComponent()
    : m_onClick(nullptr)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiButtonComponent::~UiButtonComponent()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiButtonComponent::HandleReleased(AZ::Vector2 point)
{
    bool isInRect = false;
    UiTransformBus::EventResult(isInRect, GetEntityId(), &UiTransformBus::Events::IsPointInRect, point);
    if (isInRect)
    {
        return HandleReleasedCommon(point);
    }
    else
    {
        if (m_isHandlingEvents)
        {
            UiInteractableComponent::TriggerReleasedAction(true);
        }

        m_isPressed = false;

        return m_isHandlingEvents;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiButtonComponent::HandleEnterReleased()
{
    AZ::Vector2 point(-1.0f, -1.0f);
    return HandleReleasedCommon(point);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiButtonComponent::OnClickCallback UiButtonComponent::GetOnClickCallback()
{
    return m_onClick;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiButtonComponent::SetOnClickCallback(OnClickCallback onClick)
{
    m_onClick = onClick;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const LyShine::ActionName& UiButtonComponent::GetOnClickActionName()
{
    return m_actionName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiButtonComponent::SetOnClickActionName(const LyShine::ActionName& actionName)
{
    m_actionName = actionName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiButtonComponent::Activate()
{
    UiInteractableComponent::Activate();
    UiButtonBus::Handler::BusConnect(m_entity->GetId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiButtonComponent::Deactivate()
{
    UiInteractableComponent::Deactivate();
    UiButtonBus::Handler::BusDisconnect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiButtonComponent::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

    if (serializeContext)
    {
        serializeContext->Class<UiButtonComponent, UiInteractableComponent>()
            ->Version(5, &VersionConverter)
            ->Field("ActionName", &UiButtonComponent::m_actionName);

        AZ::EditContext* ec = serializeContext->GetEditContext();
        if (ec)
        {
            auto editInfo = ec->Class<UiButtonComponent>("Button", "An interactable component for button behavior");

            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Category, "UI")
                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/UiButton.png")
                ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/UiButton.png")
                ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("UI"))
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            // Actions group
            {
                editInfo->ClassElement(AZ::Edit::ClassElements::Group, "Actions")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                editInfo->DataElement(0, &UiButtonComponent::m_actionName, "Click", "The action name triggered when the button is released");
            }
        }
    }

    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
    if (behaviorContext)
    {
        behaviorContext->EBus<UiButtonBus>("UiButtonBus")
            ->Event("GetOnClickActionName", &UiButtonBus::Events::GetOnClickActionName)
            ->Event("SetOnClickActionName", &UiButtonBus::Events::SetOnClickActionName);

        behaviorContext->EBus<UiButtonNotificationBus>("UiButtonNotificationBus")
            ->Handler<UiButtonNotificationBusBehaviorHandler>();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiButtonComponent::HandleReleasedCommon(const AZ::Vector2& point)
{
    if (m_isHandlingEvents)
    {
        // if a C++ callback is registered for OnClick then call it
        if (m_onClick)
        {
            // NOTE: The signature for the callback will probably change. We currently pass the point at which
            // mouse/touch was when released - may not be useful.
            m_onClick(GetEntityId(), point);
        }

        UiInteractableComponent::TriggerReleasedAction();

        // Tell any action listeners about the event
        if (!m_actionName.empty())
        {
            AZ::EntityId canvasEntityId;
            UiElementBus::EventResult(canvasEntityId, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);
            // Queue the event to prevent deletions during the input event
            UiCanvasNotificationBus::QueueEvent(canvasEntityId, &UiCanvasNotificationBus::Events::OnAction, GetEntityId(), m_actionName);
        }

        // Queue the event to prevent deletions during the input event
        UiButtonNotificationBus::QueueEvent(GetEntityId(), &UiButtonNotificationBus::Events::OnButtonClick);
    }

    m_isPressed = false;

    return m_isHandlingEvents;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiButtonComponent::VersionConverter(AZ::SerializeContext& context,
    AZ::SerializeContext::DataElementNode& classElement)
{
    // conversion from version 1 to 2:
    // - Need to convert CryString elements to AZStd::string
    // - Need to convert Color to Color and Alpha
    // conversion from version 2 to 3:
    // - Need to convert CryString ActionName elements to AZStd::string
    AZ_Assert(classElement.GetVersion() >= 3, "Unsupported UiButtonComponent version: %d", classElement.GetVersion());

    // conversion from version 3 to 4:
    // - Need to convert AZStd::string sprites to AzFramework::SimpleAssetReference<LmbrCentral::TextureAsset>
    if (classElement.GetVersion() < 4)
    {
        if (!LyShine::ConvertSubElementFromAzStringToAssetRef<LmbrCentral::TextureAsset>(context, classElement, "SelectedSprite"))
        {
            return false;
        }

        if (!LyShine::ConvertSubElementFromAzStringToAssetRef<LmbrCentral::TextureAsset>(context, classElement, "PressedSprite"))
        {
            return false;
        }

        if (!LyShine::ConvertSubElementFromAzStringToAssetRef<LmbrCentral::TextureAsset>(context, classElement, "DisabledSprite"))
        {
            return false;
        }
    }

    // Conversion from version 4 to 5:
    if (classElement.GetVersion() < 5)
    {
        // find the base class (AZ::Component)
        // NOTE: in very old versions there may not be a base class because the base class was not serialized
        int componentBaseClassIndex = classElement.FindElement(AZ_CRC_CE("BaseClass1"));

        // If there was a base class, make a copy and remove it
        AZ::SerializeContext::DataElementNode componentBaseClassNode;
        if (componentBaseClassIndex != -1)
        {
            // make a local copy of the component base class node
            componentBaseClassNode = classElement.GetSubElement(componentBaseClassIndex);

            // remove the component base class from the button
            classElement.RemoveElement(componentBaseClassIndex);
        }

        // Add a new base class (UiInteractableComponent)
        int interactableBaseClassIndex = classElement.AddElement<UiInteractableComponent>(context, "BaseClass1");
        AZ::SerializeContext::DataElementNode& interactableBaseClassNode = classElement.GetSubElement(interactableBaseClassIndex);

        // if there was previously a base class...
        if (componentBaseClassIndex != -1)
        {
            // copy the component base class into the new interactable base class
            // Since AZ::Component is now the base class of UiInteractableComponent
            interactableBaseClassNode.AddElement(componentBaseClassNode);
        }

        // Move the selected/hover state to the base class
        if (!UiSerialize::MoveToInteractableStateActions(context, classElement, "HoverStateActions",
                "SelectedColor", "SelectedAlpha", "SelectedSprite"))
        {
            return false;
        }

        // Move the pressed state to the base class
        if (!UiSerialize::MoveToInteractableStateActions(context, classElement, "PressedStateActions",
                "PressedColor", "PressedAlpha", "PressedSprite"))
        {
            return false;
        }

        // Move the disabled state to the base class
        if (!UiSerialize::MoveToInteractableStateActions(context, classElement, "DisabledStateActions",
                "DisabledColor", "DisabledAlpha", "DisabledSprite"))
        {
            return false;
        }
    }

    return true;
}
