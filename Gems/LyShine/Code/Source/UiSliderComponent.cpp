/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiSliderComponent.h"
#include "Sprite.h"

#include <AzCore/Component/TickBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiTransform2dBus.h>
#include <LyShine/Bus/UiTransformBus.h>
#include <LyShine/Bus/UiVisualBus.h>

#include <LyShine/ISprite.h>
#include <LyShine/UiSerializeHelpers.h>
#include "UiSerialize.h"
#include "UiNavigationHelpers.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
//! UiSliderNotificationBus Behavior context handler class
class UiSliderNotificationBusBehaviorHandler
    : public UiSliderNotificationBus::Handler
    , public AZ::BehaviorEBusHandler
{
public:
    AZ_EBUS_BEHAVIOR_BINDER(UiSliderNotificationBusBehaviorHandler, "{13540E5E-5987-4BD9-AC7A-F771F8AD0206}", AZ::SystemAllocator,
        OnSliderValueChanging, OnSliderValueChanged);

    void OnSliderValueChanging(float value) override
    {
        Call(FN_OnSliderValueChanging, value);
    }

    void OnSliderValueChanged(float value) override
    {
        Call(FN_OnSliderValueChanged, value);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiSliderComponent::UiSliderComponent()
    : m_value(0.0f)
    , m_minValue(0.0f)
    , m_maxValue(100.0f)
    , m_stepValue(0.0f)
    , m_isDragging(false)
    , m_isActive(false)
    , m_onValueChanged()
    , m_onValueChanging()
    , m_valueChangedActionName()
    , m_valueChangingActionName()
    , m_trackEntity()
    , m_fillEntity()
    , m_manipulatorEntity()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiSliderComponent::~UiSliderComponent()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiSliderComponent::GetValue()
{
    return m_value;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSliderComponent::SetValue(float value)
{
    if (m_minValue < m_maxValue)
    {
        m_value = AZ::GetClamp(value, m_minValue, m_maxValue);
    }
    else // ( m_minValue >= m_maxValue )
    {
        m_value = AZ::GetClamp(value, m_maxValue, m_minValue);
    }

    if (m_stepValue)
    {
        m_value += (m_stepValue / 2.0f);    ///< Add half the step size to the value before the fmodf so that we round to nearest step rather than flooring to previous step
        m_value = m_value - fmodf(m_value, m_stepValue);
    }

    float valueRange = fabsf(m_maxValue - m_minValue);
    float unitValue = valueRange > 0.0f ? (fabsf(m_value - m_minValue) / valueRange) : 0.0f;

    if (m_fillEntity.IsValid())
    {
        // Offsets.
        {
            UiTransform2dInterface::Offsets offsets;
            UiTransform2dBus::EventResult(offsets, m_fillEntity, &UiTransform2dBus::Events::GetOffsets);

            offsets.m_left = offsets.m_right = 0.0f;

            UiTransform2dBus::Event(m_fillEntity, &UiTransform2dBus::Events::SetOffsets, offsets);
        }

        // Anchors.
        {
            UiTransform2dInterface::Anchors anchors;
            UiTransform2dBus::EventResult(anchors, m_fillEntity, &UiTransform2dBus::Events::GetAnchors);

            anchors.m_left = 0.0f;
            anchors.m_right = unitValue;

            UiTransform2dBus::Event(m_fillEntity, &UiTransform2dBus::Events::SetAnchors, anchors, false, true);
        }
    }

    if (m_manipulatorEntity.IsValid())
    {
        // Anchors.
        {
            UiTransform2dInterface::Anchors anchors;
            UiTransform2dBus::EventResult(anchors, m_manipulatorEntity, &UiTransform2dBus::Events::GetAnchors);

            anchors.m_left = anchors.m_right = unitValue;

            UiTransform2dBus::Event(m_manipulatorEntity, &UiTransform2dBus::Events::SetAnchors, anchors, false, true);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiSliderComponent::GetMinValue()
{
    return m_minValue;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSliderComponent::SetMinValue(float value)
{
    m_minValue = value;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiSliderComponent::GetMaxValue()
{
    return m_maxValue;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSliderComponent::SetMaxValue(float value)
{
    m_maxValue = value;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiSliderComponent::GetStepValue()
{
    return m_stepValue;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSliderComponent::SetStepValue(float step)
{
    m_stepValue = step;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiSliderComponent::ValueChangeCallback UiSliderComponent::GetValueChangingCallback()
{
    return m_onValueChanging;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSliderComponent::SetValueChangingCallback(ValueChangeCallback onChange)
{
    m_onValueChanging = onChange;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const LyShine::ActionName& UiSliderComponent::GetValueChangingActionName()
{
    return m_valueChangingActionName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSliderComponent::SetValueChangingActionName(const LyShine::ActionName& actionName)
{
    m_valueChangingActionName = actionName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiSliderComponent::ValueChangeCallback UiSliderComponent::GetValueChangedCallback()
{
    return m_onValueChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSliderComponent::SetValueChangedCallback(ValueChangeCallback onChange)
{
    m_onValueChanged = onChange;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const LyShine::ActionName& UiSliderComponent::GetValueChangedActionName()
{
    return m_valueChangedActionName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSliderComponent::SetValueChangedActionName(const LyShine::ActionName& actionName)
{
    m_valueChangedActionName = actionName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSliderComponent::SetTrackEntity(AZ::EntityId entityId)
{
    m_trackEntity = entityId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiSliderComponent::GetTrackEntity()
{
    return m_trackEntity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSliderComponent::SetFillEntity(AZ::EntityId entityId)
{
    m_fillEntity = entityId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiSliderComponent::GetFillEntity()
{
    return m_fillEntity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSliderComponent::SetManipulatorEntity(AZ::EntityId entityId)
{
    m_manipulatorEntity = entityId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiSliderComponent::GetManipulatorEntity()
{
    return m_manipulatorEntity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSliderComponent::InGamePostActivate()
{
    SetValue(m_value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiSliderComponent::HandlePressed(AZ::Vector2 point, bool& shouldStayActive)
{
    bool handled = UiInteractableComponent::HandlePressed(point, shouldStayActive);

    if (handled)
    {
        m_isDragging = false;
    }

    return handled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiSliderComponent::HandleReleased(AZ::Vector2 point)
{
    if (m_isPressed && m_isHandlingEvents)
    {
        float value = GetValueFromPoint(point);
        SetValue(value);

        UiInteractableComponent::TriggerReleasedAction();

        DoChangedActions();
    }

    m_isPressed = false;
    m_isDragging = false;
    m_pressedPoint = AZ::Vector2(0.0f, 0.0f);

    return m_isHandlingEvents;
}

/////////////////////////////////////////////////////////////////
bool UiSliderComponent::HandleEnterPressed(bool& shouldStayActive)
{
    bool handled = UiInteractableComponent::HandleEnterPressed(shouldStayActive);

    if (handled)
    {
        // the slider will stay active after released
        shouldStayActive = true;
        m_isActive = true;
    }

    return handled;
}

/////////////////////////////////////////////////////////////////
bool UiSliderComponent::HandleAutoActivation()
{
    if (!m_isHandlingEvents)
    {
        return false;
    }

    m_isActive = true;
    return true;
}

/////////////////////////////////////////////////////////////////
bool UiSliderComponent::HandleKeyInputBegan(const AzFramework::InputChannel::Snapshot& inputSnapshot, AzFramework::ModifierKeyMask activeModifierKeys)
{
    if (!m_isHandlingEvents)
    {
        return false;
    }

    // don't accept key input while in pressed state
    if (m_isPressed)
    {
        return false;
    }

    bool result = false;

    const UiNavigationHelpers::Command command = UiNavigationHelpers::MapInputChannelIdToUiNavigationCommand(inputSnapshot.m_channelId, activeModifierKeys);
    if (command == UiNavigationHelpers::Command::Up ||
        command == UiNavigationHelpers::Command::Down ||
        command == UiNavigationHelpers::Command::Left ||
        command == UiNavigationHelpers::Command::Right)
    {
        const float keySteps = 10.0f;
        float delta = (m_stepValue != 0.0f) ? m_stepValue : (m_maxValue - m_minValue) / keySteps;

        UiTransformInterface::RectPoints points;
        UiTransformBus::Event(GetEntityId(), &UiTransformBus::Events::GetViewportSpacePoints, points);

        AZ::Vector2 dir = points.TopRight() - points.TopLeft();
        bool isHorizontal = fabs(dir.GetX()) >= fabs(dir.GetY());
        bool isVertical = fabs(dir.GetX()) <= fabs(dir.GetY());

        float newValue = m_value;
        if (isHorizontal && (command == UiNavigationHelpers::Command::Left || command == UiNavigationHelpers::Command::Right))
        {
            newValue += (command == UiNavigationHelpers::Command::Left ? -delta : delta);
            result = true;
        }
        else if (isVertical && (command == UiNavigationHelpers::Command::Up || command == UiNavigationHelpers::Command::Down))
        {
            newValue += (command == UiNavigationHelpers::Command::Down ? -delta : delta);
            result = true;
        }

        if (m_minValue < m_maxValue)
        {
            newValue = AZ::GetClamp(newValue, m_minValue, m_maxValue);
        }
        else // ( m_minValue >= m_maxValue )
        {
            newValue = AZ::GetClamp(newValue, m_maxValue, m_minValue);
        }

        if (newValue != m_value)
        {
            SetValue(newValue);

            AZ::Vector2 point(-1.0f, 1.0f);
            DoChangingActions();
            DoChangedActions();
        }
    }

    return result;
}

/////////////////////////////////////////////////////////////////
void UiSliderComponent::InputPositionUpdate(AZ::Vector2 point)
{
    if (m_isPressed && m_trackEntity.IsValid())
    {
        // if we are not yet in the dragging state do some tests to see if we should be
        if (!m_isDragging)
        {
            bool handOffDone = false;
            bool dragDetected = CheckForDragOrHandOffToParent(GetEntityId(), m_pressedPoint, point, 0.0f, handOffDone);

            if (dragDetected)
            {
                if (handOffDone)
                {
                    // the drag was handed off to a parent, this slider is no longer active
                    m_isPressed = false;
                }
                else
                {
                    // the drag was valid for this slider, we are now dragging
                    m_isDragging = true;
                }
            }
        }

        // if we are now in the dragging state do the drag of the slider
        if (m_isDragging)
        {
            float value = GetValueFromPoint(point);

            SetValue(value);

            DoChangingActions();
        }
    }
}

/////////////////////////////////////////////////////////////////
bool UiSliderComponent::DoesSupportDragHandOff(AZ::Vector2 startPoint)
{
    // this component does support hand-off, so long as the start point is in its bounds
    bool isPointInRect = false;
    UiTransformBus::EventResult(isPointInRect, GetEntityId(), &UiTransformBus::Events::IsPointInRect, startPoint);
    return isPointInRect;
}

/////////////////////////////////////////////////////////////////
bool UiSliderComponent::OfferDragHandOff(AZ::EntityId currentActiveInteractable, AZ::Vector2 startPoint, AZ::Vector2 currentPoint, float dragThreshold)
{
    bool handedOffToParent = false;
    bool dragDetected = CheckForDragOrHandOffToParent(currentActiveInteractable, startPoint, currentPoint, dragThreshold, handedOffToParent);

    if (dragDetected)
    {
        if (!handedOffToParent)
        {
            // a drag was detected and it was not handed off to a parent, so this slider is now taking the handoff
            m_isPressed = true;
            m_pressedPoint = startPoint;
            m_isDragging = true;

            // tell the canvas that this is now the active interactable
            UiInteractableActiveNotificationBus::Event(
                currentActiveInteractable, &UiInteractableActiveNotificationBus::Events::ActiveChanged, GetEntityId(), false);
        }
    }

    return dragDetected;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSliderComponent::LostActiveStatus()
{
    UiInteractableComponent::LostActiveStatus();

    if (m_isDragging)
    {
        if (m_isHandlingEvents)
        {
            DoChangedActions();
        }

        m_isDragging = false;
    }

    m_isActive = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSliderComponent::Activate()
{
    UiInteractableComponent::Activate();
    UiSliderBus::Handler::BusConnect(GetEntityId());
    UiInitializationBus::Handler::BusConnect(GetEntityId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSliderComponent::Deactivate()
{
    UiInteractableComponent::Deactivate();
    UiSliderBus::Handler::BusDisconnect(GetEntityId());
    UiInitializationBus::Handler::BusDisconnect(GetEntityId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiSliderComponent::IsAutoActivationSupported()
{
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiInteractableStatesInterface::State UiSliderComponent::ComputeInteractableState()
{
    UiInteractableStatesInterface::State state = UiInteractableStatesInterface::StateNormal;

    if (!m_isHandlingEvents)
    {
        state = UiInteractableStatesInterface::StateDisabled;
    }
    else if (m_isPressed || m_isActive)
    {
        // Use pressed state regardless of mouse position
        state = UiInteractableStatesInterface::StatePressed;
    }
    else if (m_isHover)
    {
        state = UiInteractableStatesInterface::StateHover;
    }

    return state;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSliderComponent::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

    if (serializeContext)
    {
        serializeContext->Class<UiSliderComponent, UiInteractableComponent>()
            ->Version(3, &VersionConverter)
        // Elements group
            ->Field("TrackEntity", &UiSliderComponent::m_trackEntity)
            ->Field("FillEntity", &UiSliderComponent::m_fillEntity)
            ->Field("ManipulatorEntity", &UiSliderComponent::m_manipulatorEntity)
        // Value group
            ->Field("Value", &UiSliderComponent::m_value)
            ->Field("MinValue", &UiSliderComponent::m_minValue)
            ->Field("MaxValue", &UiSliderComponent::m_maxValue)
            ->Field("StepValue", &UiSliderComponent::m_stepValue)
        // Actions group
            ->Field("ValueChangingActionName", &UiSliderComponent::m_valueChangingActionName)
            ->Field("ValueChangedActionName", &UiSliderComponent::m_valueChangedActionName);

        AZ::EditContext* ec = serializeContext->GetEditContext();
        if (ec)
        {
            auto editInfo = ec->Class<UiSliderComponent>("Slider", "An interactable component for modifying a floating point value with a slider.");

            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Category, "UI")
                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/UiSlider.png")
                ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/UiSlider.png")
                ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("UI"))
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            // Elements group
            {
                editInfo->ClassElement(AZ::Edit::ClassElements::Group, "Elements")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiSliderComponent::m_trackEntity, "Track", "The child element used to define the range of movement.")
                    ->Attribute(AZ::Edit::Attributes::EnumValues, &UiSliderComponent::PopulateChildEntityList);

                editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiSliderComponent::m_fillEntity, "Fill", "The child element used to show the filled part of the range.")
                    ->Attribute(AZ::Edit::Attributes::EnumValues, &UiSliderComponent::PopulateChildEntityList);

                editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiSliderComponent::m_manipulatorEntity, "Manipulator", "The child element used as a handle.")
                    ->Attribute(AZ::Edit::Attributes::EnumValues, &UiSliderComponent::PopulateChildEntityList);
            }

            // Value group
            {
                editInfo->ClassElement(AZ::Edit::ClassElements::Group, "Value")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                editInfo->DataElement(0, &UiSliderComponent::m_value, "Value", "The initial value of the slider.");
                editInfo->DataElement(0, &UiSliderComponent::m_minValue, "Min", "The minimum slider value.");
                editInfo->DataElement(0, &UiSliderComponent::m_maxValue, "Max", "The maximum slider value.");
                editInfo->DataElement(0, &UiSliderComponent::m_stepValue, "Stepping", "The smallest increment allowed between values. Use zero for no restriction.");
            }

            // Actions group
            {
                editInfo->ClassElement(AZ::Edit::ClassElements::Group, "Actions")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                editInfo->DataElement(0, &UiSliderComponent::m_valueChangingActionName, "Change", "The action triggered while the value is changing.");
                editInfo->DataElement(0, &UiSliderComponent::m_valueChangedActionName, "End change", "The action triggered when the value is done changing.");
            }
        }
    }

    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
    if (behaviorContext)
    {
        behaviorContext->EBus<UiSliderBus>("UiSliderBus")
            ->Event("GetValue", &UiSliderBus::Events::GetValue)
            ->Event("SetValue", &UiSliderBus::Events::SetValue)
            ->Event("GetMinValue", &UiSliderBus::Events::GetMinValue)
            ->Event("SetMinValue", &UiSliderBus::Events::SetMinValue)
            ->Event("GetMaxValue", &UiSliderBus::Events::GetMaxValue)
            ->Event("SetMaxValue", &UiSliderBus::Events::SetMaxValue)
            ->Event("GetStepValue", &UiSliderBus::Events::GetStepValue)
            ->Event("SetStepValue", &UiSliderBus::Events::SetStepValue)
            ->Event("GetTrackEntity", &UiSliderBus::Events::GetTrackEntity)
            ->Event("SetTrackEntity", &UiSliderBus::Events::SetTrackEntity)
            ->Event("GetFillEntity", &UiSliderBus::Events::GetFillEntity)
            ->Event("SetFillEntity", &UiSliderBus::Events::SetFillEntity)
            ->Event("GetManipulatorEntity", &UiSliderBus::Events::GetManipulatorEntity)
            ->Event("SetManipulatorEntity", &UiSliderBus::Events::SetManipulatorEntity)
            ->Event("GetValueChangingActionName", &UiSliderBus::Events::GetValueChangingActionName)
            ->Event("SetValueChangingActionName", &UiSliderBus::Events::SetValueChangingActionName)
            ->Event("GetValueChangedActionName", &UiSliderBus::Events::GetValueChangedActionName)
            ->Event("SetValueChangedActionName", &UiSliderBus::Events::SetValueChangedActionName)
            ->VirtualProperty("Value", "GetValue", "SetValue")
            ->VirtualProperty("MinValue", "GetMinValue", "SetMinValue")
            ->VirtualProperty("MaxValue", "GetMaxValue", "SetMaxValue")
            ->VirtualProperty("StepValue", "GetStepValue", "SetStepValue");

        behaviorContext->Class<UiSliderComponent>()->RequestBus("UiSliderBus");

        behaviorContext->EBus<UiSliderNotificationBus>("UiSliderNotificationBus")
            ->Handler<UiSliderNotificationBusBehaviorHandler>();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiSliderComponent::EntityComboBoxVec UiSliderComponent::PopulateChildEntityList()
{
    EntityComboBoxVec result;

    // add a first entry for "None"
    result.push_back(AZStd::make_pair(AZ::EntityId(AZ::EntityId()), "<None>"));

    // Get a list of all child elements
    LyShine::EntityArray matchingElements;
    UiElementBus::Event(
        GetEntityId(),
        &UiElementBus::Events::FindDescendantElements,
        []([[maybe_unused]] const AZ::Entity* entity)
        {
            return true;
        },
        matchingElements);

    // add their names to the StringList and their IDs to the id list
    for (auto childEntity : matchingElements)
    {
        result.push_back(AZStd::make_pair(AZ::EntityId(childEntity->GetId()), childEntity->GetName()));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiSliderComponent::GetValueFromPoint(AZ::Vector2 point)
{
    // Get m_value from point.
    float value = 0.0f;

    UiTransformInterface::RectPoints points;
    UiTransformBus::Event(m_trackEntity, &UiTransformBus::Events::GetCanvasSpacePointsNoScaleRotate, points);

    // apply scale and rotation to points
    UiTransformBus::Event(m_trackEntity, &UiTransformBus::Events::RotateAndScalePoints, points);

    const AZ::Vector2& tl = points.TopLeft();
    const AZ::Vector2& tr = points.TopRight();

    AZ::Vector2 A = tr - tl;
    float magA = A.GetLength();
    AZ::Vector2 normA = A.GetNormalized();

    AZ::Vector2 B = point - tl;
    float magBalongA = B.Dot(normA);

    float range = fabsf(m_maxValue - m_minValue);
    float unitValue = (magBalongA / magA);

    if (m_minValue < m_maxValue)
    {
        value = m_minValue + (range * unitValue);
    }
    else // ( m_minValue >= m_maxValue )
    {
        value = m_minValue - (range * unitValue);
    }

    return value;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// calculate how much we have dragged along the axis of the slider
float UiSliderComponent::GetValidDragDistanceInPixels(AZ::Vector2 startPoint, AZ::Vector2 endPoint)
{
    const float validDragRatio = 0.5f;

    // convert the drag vector to local space
    AZ::Matrix4x4 transformFromViewport;
    UiTransformBus::Event(m_trackEntity, &UiTransformBus::Events::GetTransformFromViewport, transformFromViewport);
    AZ::Vector2 dragVec = endPoint - startPoint;
    AZ::Vector3 dragVec3(dragVec.GetX(), dragVec.GetY(), 0.0f);
    AZ::Vector3 localDragVec = transformFromViewport.Multiply3x3(dragVec3);

    // the slider component only supports drag along the x axis so zero the y axis
    localDragVec.SetY(0.0f);

    // convert back to viewport space
    AZ::Matrix4x4 transformToViewport;
    UiTransformBus::Event(m_trackEntity, &UiTransformBus::Events::GetTransformToViewport, transformToViewport);
    AZ::Vector3 validDragVec = transformToViewport.Multiply3x3(localDragVec);

    float validDistance = validDragVec.GetLengthSq();
    float totalDistance = dragVec.GetLengthSq();

    // if they are not dragging mostly in a valid direction then ignore the drag
    if (validDistance / totalDistance < validDragRatio)
    {
        validDistance = 0.0f;
    }

    // return the valid drag distance
    return validDistance;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiSliderComponent::CheckForDragOrHandOffToParent(AZ::EntityId currentActiveInteractable, AZ::Vector2 startPoint, AZ::Vector2 currentPoint, float childDragThreshold, bool& handOffDone)
{
    bool result = false;

    AZ::EntityId parentDraggable;
    UiElementBus::EventResult(parentDraggable, GetEntityId(), &UiElementBus::Events::FindParentInteractableSupportingDrag, startPoint);

    // if this interactable is inside another interactable that supports drag then we use
    // a threshold value before starting a drag on this interactable
    const float normalDragThreshold = 0.0f;
    const float containedDragThreshold = 5.0f;

    float dragThreshold = normalDragThreshold;
    if (childDragThreshold > 0.0f)
    {
        dragThreshold = childDragThreshold;
    }
    else if (parentDraggable.IsValid())
    {
        dragThreshold = containedDragThreshold;
    }

    // calculate how much we have dragged along the axis of the slider
    float validDragDistance = GetValidDragDistanceInPixels(startPoint, currentPoint);
    if (validDragDistance > dragThreshold)
    {
        // we dragged above the threshold value along axis of slider
        result = true;
    }
    else if (parentDraggable.IsValid())
    {
        // offer the parent draggable the chance to become the active interactable
        UiInteractableBus::EventResult(
            handOffDone,
            parentDraggable,
            &UiInteractableBus::Events::OfferDragHandOff,
            currentActiveInteractable,
            startPoint,
            currentPoint,
            containedDragThreshold);

        if (handOffDone)
        {
            // interaction has been handed off to a container entity
            result = true;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSliderComponent::DoChangedActions()
{
    if (m_onValueChanged)
    {
        m_onValueChanged(GetEntityId(), m_value);
    }

    // Tell any action listeners about the event
    if (!m_valueChangedActionName.empty())
    {
        AZ::EntityId canvasEntityId;
        UiElementBus::EventResult(canvasEntityId, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);
        UiCanvasNotificationBus::Event(canvasEntityId, &UiCanvasNotificationBus::Events::OnAction, GetEntityId(), m_valueChangedActionName);
    }

    UiSliderNotificationBus::Event(GetEntityId(), &UiSliderNotificationBus::Events::OnSliderValueChanged, m_value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSliderComponent::DoChangingActions()
{
    if (m_onValueChanging)
    {
        m_onValueChanging(GetEntityId(), m_value);
    }

    // Tell any action listeners about the event
    if (!m_valueChangingActionName.empty())
    {
        AZ::EntityId canvasEntityId;
        UiElementBus::EventResult(canvasEntityId, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);
        UiCanvasNotificationBus::Event(canvasEntityId, &UiCanvasNotificationBus::Events::OnAction, GetEntityId(), m_valueChangingActionName);
    }

    UiSliderNotificationBus::Event(GetEntityId(), &UiSliderNotificationBus::Events::OnSliderValueChanging, m_value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiSliderComponent::VersionConverter(AZ::SerializeContext& context,
    AZ::SerializeContext::DataElementNode& classElement)
{
    // conversion from version 1 to 2:
    // - Need to convert AZStd::string sprites to AzFramework::SimpleAssetReference<LmbrCentral::TextureAsset>
    if (classElement.GetVersion() < 2)
    {
        if (!LyShine::ConvertSubElementFromAzStringToAssetRef<LmbrCentral::TextureAsset>(context, classElement, "SelectedSprite"))
        {
            return false;
        }

        if (!LyShine::ConvertSubElementFromAzStringToAssetRef<LmbrCentral::TextureAsset>(context, classElement, "DisabledSprite"))
        {
            return false;
        }
    }

    // Conversion from version 2 to 3:
    if (classElement.GetVersion() < 3)
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

        // Move the disabled state to the base class
        if (!UiSerialize::MoveToInteractableStateActions(context, classElement, "DisabledStateActions",
                "DisabledColor", "DisabledAlpha", "DisabledSprite"))
        {
            return false;
        }
    }

    return true;
}
