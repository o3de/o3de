/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiScrollBarComponent.h"

#include "UiNavigationHelpers.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiTransform2dBus.h>
#include <LyShine/Bus/UiTransformBus.h>

#include <AzCore/Time/ITime.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
//! UiScrollerNotificationBus Behavior context handler class
class BehaviorUiScrollerNotificationBusHandler
    : public UiScrollerNotificationBus::Handler
    , public AZ::BehaviorEBusHandler
{
public:
    AZ_EBUS_BEHAVIOR_BINDER(BehaviorUiScrollerNotificationBusHandler, "{77A20EE4-EB8D-431A-B4B1-798805801C4D}", AZ::SystemAllocator,
        OnScrollerValueChanging, OnScrollerValueChanged);

    void OnScrollerValueChanging(float value) override
    {
        Call(FN_OnScrollerValueChanging, value);
    }

    void OnScrollerValueChanged(float value) override
    {
        Call(FN_OnScrollerValueChanged, value);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiScrollBarComponent::UiScrollBarComponent()
    : m_value(0.0f)
    , m_handleSize(0.1f)
    , m_minHandlePixelSize(20.0f)
    , m_orientation(Orientation::Horizontal)
    , m_isDragging(false)
    , m_isActive(false)
    , m_lastDragPoint(0.0f, 0.0f)
    , m_onValueChanged()
    , m_onValueChanging()
    , m_valueChangedActionName()
    , m_valueChangingActionName()
    , m_handleEntity()
    , m_scrollableEntity()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiScrollBarComponent::~UiScrollBarComponent()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBarComponent::Update(float deltaTime)
{
    UiInteractableComponent::Update(deltaTime);

    if (m_isAutoFadeEnabled)
    {
        // count down secondsBeforeFade
        if (!m_isFading && m_secondsRemainingBeforeFade > 0)
        {
            m_secondsRemainingBeforeFade -= deltaTime;

            if (m_secondsRemainingBeforeFade <= 0)
            {
                m_isFading = true;
                // if m_secondsBeforeFade is below 0, use any leftover time for fading
                deltaTime = abs(m_secondsRemainingBeforeFade);
            }
        }

        // calculate fade and set alpha for image components
        if (m_isFading && m_isAutoFadeEnabled && m_currFade > 0)
        {
            float deltaFade = deltaTime * m_fadeSpeed;
            m_currFade -= deltaFade;
            if (m_currFade < 0)
            {
                m_currFade = 0;
            }
            SetImageComponentsAlpha(m_currFade);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiScrollBarComponent::GetValue()
{
    return m_value;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBarComponent::SetValue(float value)
{
    if (m_value != value)
    {
        DoSetValue(value);

        // Reset drag info
        if (m_isDragging)
        {
            ResetDragInfo();
        }

        DoChangedActions();

        NotifyScrollableOnValueChanged();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiScrollBarComponent::GetHandleSize()
{
    return m_handleSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBarComponent::SetHandleSize(float size)
{
    m_handleSize = AZ::GetClamp(size, 0.0f, 1.0f);

    // Make sure handle size is at least the minimum size
    float handleParentLength = GetHandleParentLength();
    float minHandleSize = (handleParentLength > 0.0f) ? m_minHandlePixelSize / handleParentLength : 0.0f;
    minHandleSize = AZ::GetClamp(minHandleSize, 0.0f, 1.0f);

    m_displayedHandleSize = AZ::GetClamp(m_handleSize, minHandleSize, 1.0f);

    if (m_handleEntity.IsValid())
    {
        // Change handle's anchor
        UiTransform2dInterface::Anchors anchors;
        EBUS_EVENT_ID_RESULT(anchors, m_handleEntity, UiTransform2dBus, GetAnchors);

        if (m_orientation == Orientation::Horizontal)
        {
            anchors.m_right = anchors.m_left + m_displayedHandleSize;
        }
        else if (m_orientation == Orientation::Vertical)
        {
            anchors.m_bottom = anchors.m_top + m_displayedHandleSize;
        }
        else
        {
            AZ_Assert(false, "unhandled scrollbar orientation");
        }

        EBUS_EVENT_ID(m_handleEntity, UiTransform2dBus, SetAnchors, anchors, false, false);

        // Position handle at the correct location
        DoSetValue(m_value);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiScrollBarComponent::GetMinHandlePixelSize()
{
    return m_minHandlePixelSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBarComponent::SetMinHandlePixelSize(float size)
{
    m_minHandlePixelSize = size;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiScrollBarComponent::IsAutoFadeEnabled()
{
    return m_isAutoFadeEnabled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBarComponent::SetAutoFadeEnabled(bool isAutoFadeEnabled)
{
    m_isAutoFadeEnabled = isAutoFadeEnabled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiScrollBarComponent::GetAutoFadeDelay()
{
    return m_inactiveSecondsBeforeFade;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBarComponent::SetAutoFadeDelay(float delay)
{
    m_inactiveSecondsBeforeFade = delay;
    ResetFade();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiScrollBarComponent::GetAutoFadeSpeed()
{
    return m_fadeSpeed;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBarComponent::SetAutoFadeSpeed(float speed)
{
    m_fadeSpeed = speed;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiScrollerInterface::Orientation UiScrollBarComponent::GetOrientation()
{
    return m_orientation;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBarComponent::SetOrientation(Orientation orientation)
{
    m_orientation = orientation;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiScrollBarComponent::GetHandleEntity()
{
    return m_handleEntity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBarComponent::SetHandleEntity(AZ::EntityId entityId)
{
    m_handleEntity = entityId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiScrollBarComponent::GetScrollableEntity()
{
    return m_scrollableEntity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBarComponent::SetScrollableEntity(AZ::EntityId entityId)
{
    m_scrollableEntity = entityId;

    if (m_scrollableEntity.IsValid())
    {
        UiScrollableToScrollerNotificationBus::Handler::BusConnect(m_scrollableEntity);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiScrollBarComponent::ValueChangeCallback UiScrollBarComponent::GetValueChangingCallback()
{
    return m_onValueChanging;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBarComponent::SetValueChangingCallback(ValueChangeCallback onChange)
{
    m_onValueChanging = onChange;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const LyShine::ActionName& UiScrollBarComponent::GetValueChangingActionName()
{
    return m_valueChangingActionName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBarComponent::SetValueChangingActionName(const LyShine::ActionName& actionName)
{
    m_valueChangingActionName = actionName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiScrollBarComponent::ValueChangeCallback UiScrollBarComponent::GetValueChangedCallback()
{
    return m_onValueChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBarComponent::SetValueChangedCallback(ValueChangeCallback onChange)
{
    m_onValueChanged = onChange;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const LyShine::ActionName& UiScrollBarComponent::GetValueChangedActionName()
{
    return m_valueChangedActionName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBarComponent::SetValueChangedActionName(const LyShine::ActionName& actionName)
{
    m_valueChangedActionName = actionName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBarComponent::OnValueChangingByScrollable(AZ::Vector2 value)
{
    float axisValue = 0.0f;

    if (m_orientation == Orientation::Horizontal)
    {
        axisValue = value.GetX();
    }
    else if (m_orientation == Orientation::Vertical)
    {
        axisValue = value.GetY();
    }
    else
    {
        AZ_Assert(false, "unhandled scrollbar orientation");
    }

    if (m_value != axisValue)
    {
        DoSetValue(axisValue);

        // Reset drag info
        if (m_isDragging)
        {
            ResetDragInfo();
        }

        DoChangingActions();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBarComponent::OnValueChangedByScrollable(AZ::Vector2 value)
{
    float axisValue = 0.0f;

    if (m_orientation == Orientation::Horizontal)
    {
        axisValue = value.GetX();
    }
    else if (m_orientation == Orientation::Vertical)
    {
        axisValue = value.GetY();
    }
    else
    {
        AZ_Assert(false, "unhandled scrollbar orientation");
    }

    if (m_value != axisValue)
    {
        DoSetValue(axisValue);

        // Reset drag info
        if (m_isDragging)
        {
            ResetDragInfo();
        }

        DoChangedActions();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBarComponent::OnScrollableParentToContentRatioChanged(AZ::Vector2 parentToContentRatio)
{
    float axisParentToContentRatio = 1.0f;

    if (m_orientation == Orientation::Horizontal)
    {
        axisParentToContentRatio = parentToContentRatio.GetX();
    }
    else if (m_orientation == Orientation::Vertical)
    {
        axisParentToContentRatio = parentToContentRatio.GetY();
    }
    else
    {
        AZ_Assert(false, "unhandled scrollbar orientation");
    }

    SetHandleSize(axisParentToContentRatio);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBarComponent::InGamePostActivate()
{
    SetHandleSize(m_handleSize);
    ResetFade();
    UiImageBus::EventResult(m_initialScrollBarAlpha, GetEntityId(), &UiImageBus::Events::GetAlpha);
    UiImageBus::EventResult(m_initialHandleAlpha, m_handleEntity, &UiImageBus::Events::GetAlpha);

    // Listen for canvas space rect changes
    UiTransformChangeNotificationBus::Handler::BusConnect(GetEntityId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiScrollBarComponent::HandlePressed(AZ::Vector2 point, bool& shouldStayActive)
{
    bool handled = UiInteractableComponent::HandlePressed(point, shouldStayActive);

    if (handled)
    {
        m_isDragging = false;
        m_pressedOnHandle = false;

        AZ::Entity* handleParentEntity = nullptr;
        EBUS_EVENT_ID_RESULT(handleParentEntity, m_handleEntity, UiElementBus, GetParent);

        if (handleParentEntity)
        {
            // Check where point is relative to handle
            LocRelativeToHandle pointLoc = GetLocationRelativeToHandle(point);
            m_pressedOnHandle = (pointLoc == LocRelativeToHandle::OnHandle);

            if (m_pressedOnHandle)
            {
                // Store value when press occurred
                m_pressedValue = m_value;

                // Store pressed position
                m_pressedPosAlongAxis = GetPosAlongAxis(point);
            }
            else
            {
                // Move handle
                const AZ::TimeMs realTimeMs = AZ::GetRealElapsedTimeMs();
                m_lastMoveTime = AZ::TimeMsToSeconds(realTimeMs);
                m_moveDelayTime = 0.45f;

                MoveHandle(pointLoc);
            }
        }
    }

    return handled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiScrollBarComponent::HandleReleased([[maybe_unused]] AZ::Vector2 point)
{
    if (m_isPressed && m_isHandlingEvents)
    {
        UiInteractableComponent::TriggerReleasedAction();

        DoChangedActions();

        NotifyScrollableOnValueChanged();
    }

    m_isPressed = false;
    m_isDragging = false;
    m_pressedPoint = AZ::Vector2(0.0f, 0.0f);

    return m_isHandlingEvents;
}

/////////////////////////////////////////////////////////////////
bool UiScrollBarComponent::HandleEnterPressed(bool& shouldStayActive)
{
    bool handled = UiInteractableComponent::HandleEnterPressed(shouldStayActive);

    if (handled)
    {
        // the scrollbar will stay active after released
        shouldStayActive = true;
        m_isActive = true;
    }

    return handled;
}

/////////////////////////////////////////////////////////////////
bool UiScrollBarComponent::HandleAutoActivation()
{
    if (!m_isHandlingEvents)
    {
        return false;
    }

    m_isActive = true;
    return true;
}

/////////////////////////////////////////////////////////////////
bool UiScrollBarComponent::HandleKeyInputBegan(const AzFramework::InputChannel::Snapshot& inputSnapshot, AzFramework::ModifierKeyMask activeModifierKeys)
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
    bool moved = false;

    const UiNavigationHelpers::Command command = UiNavigationHelpers::MapInputChannelIdToUiNavigationCommand(inputSnapshot.m_channelId, activeModifierKeys);
    if (command == UiNavigationHelpers::Command::Up ||
        command == UiNavigationHelpers::Command::Down ||
        command == UiNavigationHelpers::Command::Left ||
        command == UiNavigationHelpers::Command::Right)
    {
        if ((m_orientation == Orientation::Horizontal) && (command == UiNavigationHelpers::Command::Left || command == UiNavigationHelpers::Command::Right))
        {
            moved = MoveHandle(command == UiNavigationHelpers::Command::Left ? LocRelativeToHandle::BeforeHandle : LocRelativeToHandle::AfterHandle);
            result = true;
        }
        else if ((m_orientation == Orientation::Vertical) && (command == UiNavigationHelpers::Command::Up || command == UiNavigationHelpers::Command::Down))
        {
            moved = MoveHandle(command == UiNavigationHelpers::Command::Up ? LocRelativeToHandle::BeforeHandle : LocRelativeToHandle::AfterHandle);
            result = true;
        }
    }

    if (moved)
    {
        DoChangedActions();

        NotifyScrollableOnValueChanged();
    }

    return result;
}

/////////////////////////////////////////////////////////////////
void UiScrollBarComponent::InputPositionUpdate(AZ::Vector2 point)
{
    if (m_isPressed)
    {
        if (m_pressedOnHandle)
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
                        // the drag was handed off to a parent, this scrollbar is no longer active
                        m_isPressed = false;
                    }
                    else
                    {
                        // the drag was valid for this scrollbar, we are now dragging
                        m_isDragging = true;

                        m_pressedValue = m_value;
                        m_pressedPoint = point;
                        m_pressedPosAlongAxis = GetPosAlongAxis(m_pressedPoint);
                    }
                }
            }

            // if we are now in the dragging state do the drag of the scrollbar handle
            if (m_isDragging)
            {
                float newValue = m_value;

                // Check handle size to see if there is space to scroll
                if (m_displayedHandleSize < 1.0f)
                {
                    float handleParentLength = GetHandleParentLength();
                    if (handleParentLength > 0.0f)
                    {
                        float newPosAlongAxis = GetPosAlongAxis(point);

                        // Calculate drag distance relative to max distance
                        float dragDistAlongAxis = newPosAlongAxis - m_pressedPosAlongAxis;
                        float maxDragDistAlongAxis = handleParentLength - (m_displayedHandleSize * handleParentLength);
                        float valueOffset = dragDistAlongAxis / maxDragDistAlongAxis;

                        // Update value
                        newValue = AZ::GetClamp(m_pressedValue + valueOffset, 0.0f, 1.0f);
                    }
                }

                m_lastDragPoint = point;

                if (newValue != m_value)
                {
                    DoSetValue(newValue);
                    DoChangingActions();

                    NotifyScrollableOnValueChanging();
                }
            }
        }
        else
        {
            if (m_handleEntity.IsValid())
            {
                // Only do something if we're over the interactable
                bool isPointInRect = false;
                EBUS_EVENT_ID_RESULT(isPointInRect, GetEntityId(), UiTransformBus, IsPointInRect, point);
                if (isPointInRect)
                {
                    // Only do something if we're on either side of the handle
                    LocRelativeToHandle pointLoc = GetLocationRelativeToHandle(point);
                    if (pointLoc != LocRelativeToHandle::OnHandle)
                    {
                        const AZ::TimeMs realTimeMs = AZ::GetRealElapsedTimeMs();
                        const float currentTime = AZ::TimeMsToSeconds(realTimeMs);
                        if (currentTime - m_lastMoveTime > m_moveDelayTime)
                        {
                            m_lastMoveTime = currentTime;
                            m_moveDelayTime = 0.05f;

                            MoveHandle(pointLoc);
                        }
                    }
                }
            }
        }
    }
}

/////////////////////////////////////////////////////////////////
bool UiScrollBarComponent::DoesSupportDragHandOff(AZ::Vector2 startPoint)
{
    // this component does support hand-off, so long as the start point is in its bounds
    bool isPointInRect = false;
    EBUS_EVENT_ID_RESULT(isPointInRect, GetEntityId(), UiTransformBus, IsPointInRect, startPoint);
    return isPointInRect;
}

/////////////////////////////////////////////////////////////////
bool UiScrollBarComponent::OfferDragHandOff(AZ::EntityId currentActiveInteractable, AZ::Vector2 startPoint, AZ::Vector2 currentPoint, float dragThreshold)
{
    bool handedOffToParent = false;
    bool dragDetected = CheckForDragOrHandOffToParent(currentActiveInteractable, startPoint, currentPoint, dragThreshold, handedOffToParent);

    if (dragDetected)
    {
        if (!handedOffToParent)
        {
            // a drag was detected and it was not handed off to a parent, so this scrollbar is now taking the handoff
            m_isPressed = true;
            m_pressedValue = m_value;
            m_pressedPoint = startPoint;
            m_pressedPosAlongAxis = GetPosAlongAxis(m_pressedPoint);
            m_isDragging = true;
            m_lastDragPoint = m_pressedPoint;

            // tell the canvas that this is now the active interactable
            EBUS_EVENT_ID(currentActiveInteractable, UiInteractableActiveNotificationBus, ActiveChanged, GetEntityId(), false);
        }
    }

    return dragDetected;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBarComponent::LostActiveStatus()
{
    UiInteractableComponent::LostActiveStatus();

    if (m_isDragging)
    {
        if (m_isHandlingEvents)
        {
            DoChangedActions();

            NotifyScrollableOnValueChanged();
        }

        m_isDragging = false;
    }

    m_isActive = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBarComponent::OnCanvasSpaceRectChanged([[maybe_unused]] AZ::EntityId entityId, const UiTransformInterface::Rect& oldRect, const UiTransformInterface::Rect& newRect)
{
    // If old rect equals new rect, size changed due to initialization
    bool sizeChanged = (oldRect == newRect) || (!oldRect.GetSize().IsClose(newRect.GetSize(), 0.05f));

    if (sizeChanged)
    {
        SetHandleSize(m_handleSize);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBarComponent::Activate()
{
    UiInteractableComponent::Activate();
    UiScrollBarBus::Handler::BusConnect(GetEntityId());
    UiScrollerBus::Handler::BusConnect(GetEntityId());
    UiInitializationBus::Handler::BusConnect(GetEntityId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBarComponent::Deactivate()
{
    UiInteractableComponent::Deactivate();
    UiScrollBarBus::Handler::BusDisconnect(GetEntityId());
    UiScrollerBus::Handler::BusDisconnect(GetEntityId());
    UiInitializationBus::Handler::BusDisconnect(GetEntityId());
    UiTransformChangeNotificationBus::Handler::BusDisconnect();

    if (m_scrollableEntity.IsValid())
    {
        UiScrollableToScrollerNotificationBus::Handler::BusDisconnect(m_scrollableEntity);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiScrollBarComponent::IsAutoActivationSupported()
{
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiInteractableStatesInterface::State UiScrollBarComponent::ComputeInteractableState()
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
void UiScrollBarComponent::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

    if (serializeContext)
    {
        serializeContext->Class<UiScrollBarComponent, UiInteractableComponent>()
            ->Version(1)
            // Elements group
            ->Field("HandleEntity", &UiScrollBarComponent::m_handleEntity)
            // Values group
            ->Field("Orientation", &UiScrollBarComponent::m_orientation)
            ->Field("Value", &UiScrollBarComponent::m_value)
            ->Field("HandleSize", &UiScrollBarComponent::m_handleSize)
            ->Field("MinHandlePixelSize", &UiScrollBarComponent::m_minHandlePixelSize)
            // Actions group
            ->Field("ValueChangingActionName", &UiScrollBarComponent::m_valueChangingActionName)
            ->Field("ValueChangedActionName", &UiScrollBarComponent::m_valueChangedActionName)
            // Visibility group
            ->Field("IsAutoFadeEnabled", &UiScrollBarComponent::m_isAutoFadeEnabled)
            ->Field("FadeDelay", &UiScrollBarComponent::m_inactiveSecondsBeforeFade)
            ->Field("FadeSpeed", &UiScrollBarComponent::m_fadeSpeed);

        AZ::EditContext* ec = serializeContext->GetEditContext();
        if (ec)
        {
            auto editInfo = ec->Class<UiScrollBarComponent>("ScrollBar", "An interactable component for scrolling content that is larger than its viewing area.");

            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Category, "UI")
                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/UiScrollBar.png")
                ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/UiScrollBar.png")
                ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("UI", 0x27ff46b0))
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            // Elements group
            {
                editInfo->ClassElement(AZ::Edit::ClassElements::Group, "Elements")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiScrollBarComponent::m_handleEntity, "Handle", "The child element that is the sliding handle.")
                    ->Attribute(AZ::Edit::Attributes::EnumValues, &UiScrollBarComponent::PopulateChildEntityList);
            }

            // Values group
            {
                editInfo->ClassElement(AZ::Edit::ClassElements::Group, "Values")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiScrollBarComponent::m_orientation, "Orientation", "The way the scrollbar should be oriented.")
                    ->EnumAttribute(Orientation::Horizontal, "Horizontal")
                    ->EnumAttribute(Orientation::Vertical, "Vertical");

                editInfo->DataElement(0, &UiScrollBarComponent::m_value, "Value", "The initial value of the scrollbar.")
                    ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f);

                editInfo->DataElement(0, &UiScrollBarComponent::m_handleSize, "Handle size", "The size of the handle relative to the scrollbar.")
                    ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f);

                editInfo->DataElement(0, &UiScrollBarComponent::m_minHandlePixelSize, "Min handle size", "The minimum size of the handle in pixels.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f);
            }

            // Actions group
            {
                editInfo->ClassElement(AZ::Edit::ClassElements::Group, "Actions")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                editInfo->DataElement(0, &UiScrollBarComponent::m_valueChangingActionName, "Change", "The action triggered while the value is changing.");
                editInfo->DataElement(0, &UiScrollBarComponent::m_valueChangedActionName, "End change", "The action triggered when the value is done changing.");
            }

            // Visibility group
            {
                editInfo->ClassElement(AZ::Edit::ClassElements::Group, "Fade")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                editInfo->DataElement(0, &UiScrollBarComponent::m_isAutoFadeEnabled, "Auto Fade When Not In Use", "The scrollbar will automatically fade away when not in use.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshEntireTree", 0xefbc823c));
                editInfo->DataElement(0, &UiScrollBarComponent::m_inactiveSecondsBeforeFade, "Fade Delay", "The delay in seconds before the scrollbar will begin to fade.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &UiScrollBarComponent::m_isAutoFadeEnabled);
                editInfo->DataElement(0, &UiScrollBarComponent::m_fadeSpeed, "Fade Speed", "The speed in seconds at which the scrollbar will fade away.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &UiScrollBarComponent::m_isAutoFadeEnabled);
            }
        }
    }

    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
    if (behaviorContext)
    {
        behaviorContext->EBus<UiScrollBarBus>("UiScrollBarBus")
            ->Event("GetHandleSize", &UiScrollBarBus::Events::GetHandleSize)
            ->Event("SetHandleSize", &UiScrollBarBus::Events::SetHandleSize)
            ->Event("GetMinHandlePixelSize", &UiScrollBarBus::Events::GetMinHandlePixelSize)
            ->Event("SetMinHandlePixelSize", &UiScrollBarBus::Events::SetMinHandlePixelSize)
            ->Event("GetHandleEntity", &UiScrollBarBus::Events::GetHandleEntity)
            ->Event("SetHandleEntity", &UiScrollBarBus::Events::SetHandleEntity)
            ->VirtualProperty("HandleSize", "GetHandleSize", "SetHandleSize")
            ->VirtualProperty("MinHandlePixelSize", "GetMinHandlePixelSize", "SetMinHandlePixelSize")
            ->Event("IsAutoFadeEnabled", &UiScrollBarBus::Events::IsAutoFadeEnabled)
            ->Event("SetAutoFadeEnabled", &UiScrollBarBus::Events::SetAutoFadeEnabled)
            ->Event("GetAutoFadeDelay", &UiScrollBarBus::Events::GetAutoFadeDelay)
            ->Event("SetAutoFadeDelay", &UiScrollBarBus::Events::SetAutoFadeDelay)
            ->Event("GetAutoFadeSpeed", &UiScrollBarBus::Events::GetAutoFadeSpeed)
            ->Event("SetAutoFadeSpeed", &UiScrollBarBus::Events::SetAutoFadeSpeed)
            ->VirtualProperty("AutoFadeEnabled", "IsAutoFadeEnabled", "SetAutoFadeEnabled")
            ->VirtualProperty("AutoFadeDelay", "GetAutoFadeDelay", "SetAutoFadeDelay")
            ->VirtualProperty("AutoFadeSpeed", "GetAutoFadeSpeed", "SetAutoFadeSpeed");


        behaviorContext->Enum<(int)UiScrollerInterface::Orientation::Horizontal>("eUiScrollerOrientation_Horizontal")
            ->Enum<(int)UiScrollerInterface::Orientation::Vertical>("eUiScrollerOrientation_Vertical");

        behaviorContext->EBus<UiScrollerBus>("UiScrollerBus")
            ->Event("GetValue", &UiScrollerBus::Events::GetValue)
            ->Event("SetValue", &UiScrollerBus::Events::SetValue)
            ->Event("GetOrientation", &UiScrollerBus::Events::GetOrientation)
            ->Event("SetOrientation", &UiScrollerBus::Events::SetOrientation)
            ->Event("GetValueChangingActionName", &UiScrollerBus::Events::GetValueChangingActionName)
            ->Event("SetValueChangingActionName", &UiScrollerBus::Events::SetValueChangingActionName)
            ->Event("GetValueChangedActionName", &UiScrollerBus::Events::GetValueChangedActionName)
            ->Event("SetValueChangedActionName", &UiScrollerBus::Events::SetValueChangedActionName)
            ->VirtualProperty("Value", "GetValue", "SetValue");

        behaviorContext->EBus<UiScrollerNotificationBus>("UiScrollerNotificationBus")
            ->Handler<BehaviorUiScrollerNotificationBusHandler>();

        behaviorContext->Class<UiScrollBarComponent>()
            ->RequestBus("UiScrollBarBus")
            ->RequestBus("UiScrollerBus");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiScrollBarComponent::EntityComboBoxVec UiScrollBarComponent::PopulateChildEntityList()
{
    EntityComboBoxVec result;

    // add a first entry for "None"
    result.push_back(AZStd::make_pair(AZ::EntityId(AZ::EntityId()), "<None>"));

    // Get a list of all child elements
    LyShine::EntityArray matchingElements;
    EBUS_EVENT_ID(GetEntityId(), UiElementBus, FindDescendantElements,
        []([[maybe_unused]] const AZ::Entity* entity) { return true; },
        matchingElements);

    // add their names to the StringList and their IDs to the id list
    for (auto childEntity : matchingElements)
    {
        result.push_back(AZStd::make_pair(AZ::EntityId(childEntity->GetId()), childEntity->GetName()));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// calculate how much we have dragged along the axis of the scrollbar
float UiScrollBarComponent::GetValidDragDistanceInPixels(AZ::Vector2 startPoint, AZ::Vector2 endPoint)
{
    const float validDragRatio = 0.5f;

    // convert the drag vector to local space
    AZ::Matrix4x4 transformFromViewport;
    EBUS_EVENT_ID(m_entity->GetId(), UiTransformBus, GetTransformFromViewport, transformFromViewport);
    AZ::Vector2 dragVec = endPoint - startPoint;
    AZ::Vector3 dragVec3(dragVec.GetX(), dragVec.GetY(), 0.0f);
    AZ::Vector3 localDragVec = transformFromViewport.Multiply3x3(dragVec3);

    // constrain to the allowed movement directions
    if (m_orientation == Orientation::Horizontal)
    {
        localDragVec.SetY(0.0f);
    }
    else if (m_orientation == Orientation::Vertical)
    {
        localDragVec.SetX(0.0f);
    }

    // convert back to viewport space
    AZ::Matrix4x4 transformToViewport;
    EBUS_EVENT_ID(m_entity->GetId(), UiTransformBus, GetTransformToViewport, transformToViewport);
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
bool UiScrollBarComponent::CheckForDragOrHandOffToParent(AZ::EntityId currentActiveInteractable, AZ::Vector2 startPoint, AZ::Vector2 currentPoint, float childDragThreshold, bool& handOffDone)
{
    bool result = false;

    AZ::EntityId parentDraggable;
    EBUS_EVENT_ID_RESULT(parentDraggable, GetEntityId(), UiElementBus, FindParentInteractableSupportingDrag, startPoint);

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

    // calculate how much we have dragged along the axis of the scrollbar
    float validDragDistance = GetValidDragDistanceInPixels(startPoint, currentPoint);
    if (validDragDistance > dragThreshold)
    {
        // we dragged above the threshold value along axis of scrollbar
        result = true;
    }
    else if (parentDraggable.IsValid())
    {
        // offer the parent draggable the chance to become the active interactable
        EBUS_EVENT_ID_RESULT(handOffDone, parentDraggable, UiInteractableBus,
            OfferDragHandOff, currentActiveInteractable, startPoint, currentPoint, containedDragThreshold);

        if (handOffDone)
        {
            // interaction has been handed off to a container entity
            result = true;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBarComponent::DoSetValue(float value)
{
    m_value = AZ::GetClamp(value, 0.0f, 1.0f);

    if (m_handleEntity.IsValid())
    {
        // Move handle's anchors
        UiTransform2dInterface::Anchors anchors;
        EBUS_EVENT_ID_RESULT(anchors, m_handleEntity, UiTransform2dBus, GetAnchors);

        if (m_orientation == Orientation::Horizontal)
        {
            anchors.m_left = (1.0f - m_displayedHandleSize) * m_value;
            anchors.m_right = anchors.m_left + m_displayedHandleSize;
        }
        else if (m_orientation == Orientation::Vertical)
        {
            anchors.m_top = (1.0f - m_displayedHandleSize) * m_value;
            anchors.m_bottom = anchors.m_top + m_displayedHandleSize;
        }
        else
        {
            AZ_Assert(false, "unhandled scrollbar orientation");
        }

        EBUS_EVENT_ID(m_handleEntity, UiTransform2dBus, SetAnchors, anchors, false, false);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBarComponent::DoChangedActions()
{
    if (m_onValueChanged)
    {
        m_onValueChanged(GetEntityId(), m_value);
    }

    // Tell any action listeners about the event
    if (!m_valueChangedActionName.empty())
    {
        AZ::EntityId canvasEntityId;
        EBUS_EVENT_ID_RESULT(canvasEntityId, GetEntityId(), UiElementBus, GetCanvasEntityId);
        EBUS_EVENT_ID(canvasEntityId, UiCanvasNotificationBus, OnAction, GetEntityId(), m_valueChangedActionName);
    }

    NotifyListenersOnValueChanged();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBarComponent::DoChangingActions()
{
    if (m_onValueChanging)
    {
        m_onValueChanging(GetEntityId(), m_value);
    }

    // Tell any action listeners about the event
    if (!m_valueChangingActionName.empty())
    {
        AZ::EntityId canvasEntityId;
        EBUS_EVENT_ID_RESULT(canvasEntityId, GetEntityId(), UiElementBus, GetCanvasEntityId);
        EBUS_EVENT_ID(canvasEntityId, UiCanvasNotificationBus, OnAction, GetEntityId(), m_valueChangingActionName);
    }

    ResetFade();
    NotifyListenersOnValueChanging();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBarComponent::NotifyListenersOnValueChanged()
{
    EBUS_EVENT_ID(GetEntityId(), UiScrollerNotificationBus, OnScrollerValueChanged, m_value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBarComponent::NotifyListenersOnValueChanging()
{
    EBUS_EVENT_ID(GetEntityId(), UiScrollerNotificationBus, OnScrollerValueChanging, m_value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBarComponent::NotifyScrollableOnValueChanged()
{
    EBUS_EVENT_ID(GetEntityId(), UiScrollerToScrollableNotificationBus, OnValueChangedByScroller, m_value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBarComponent::NotifyScrollableOnValueChanging()
{
    EBUS_EVENT_ID(GetEntityId(), UiScrollerToScrollableNotificationBus, OnValueChangingByScroller, m_value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiScrollBarComponent::LocRelativeToHandle UiScrollBarComponent::GetLocationRelativeToHandle(const AZ::Vector2& point)
{
    // get point in the no scale/rotate canvas space
    AZ::Matrix4x4 transform;
    EBUS_EVENT_ID(m_handleEntity, UiTransformBus, GetTransformFromViewport, transform);
    AZ::Vector3 point3(point.GetX(), point.GetY(), 0.0f);
    point3 = transform * point3;

    // get the rect for this element in the same space
    UiTransformInterface::Rect rect;
    EBUS_EVENT_ID(m_handleEntity, UiTransformBus, GetCanvasSpaceRectNoScaleRotate, rect);

    LocRelativeToHandle pointLoc;

    if (m_orientation == Orientation::Horizontal)
    {
        float left = rect.left;
        float right = rect.right;

        // allow for "flipped" rects
        if (left > right)
        {
            std::swap(left, right);
        }

        if (point3.GetX() >= left && point3.GetX() <= right)
        {
            pointLoc = LocRelativeToHandle::OnHandle;
        }
        else if (point3.GetX() < left)
        {
            pointLoc = LocRelativeToHandle::BeforeHandle;
        }
        else // (point3.GetX() > right)
        {
            pointLoc = LocRelativeToHandle::AfterHandle;
        }
    }
    else if (m_orientation == Orientation::Vertical)
    {
        float top = rect.top;
        float bottom = rect.bottom;

        // allow for "flipped" rects
        if (top > bottom)
        {
            std::swap(top, bottom);
        }

        if (point3.GetY() >= top && point3.GetY() <= bottom)
        {
            pointLoc = LocRelativeToHandle::OnHandle;
        }
        else if (point3.GetY() < top)
        {
            pointLoc = LocRelativeToHandle::BeforeHandle;
        }
        else // (point3.GetY() > bottom)
        {
            pointLoc = LocRelativeToHandle::AfterHandle;
        }
    }
    else
    {
        AZ_Assert(false, "unhandled scrollbar orientation");
        pointLoc = LocRelativeToHandle::OnHandle;
    }

    return pointLoc;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiScrollBarComponent::GetHandleParentLength()
{
    float handleParentLength = 0.0f;

    AZ::Entity* handleParentEntity = nullptr;
    EBUS_EVENT_ID_RESULT(handleParentEntity, m_handleEntity, UiElementBus, GetParent);

    if (handleParentEntity)
    {
        AZ::Vector2 size;
        EBUS_EVENT_ID_RESULT(size, handleParentEntity->GetId(), UiTransformBus, GetCanvasSpaceSizeNoScaleRotate);

        if (m_orientation == Orientation::Horizontal)
        {
            handleParentLength = size.GetX();
        }
        else if (m_orientation == Orientation::Vertical)
        {
            handleParentLength = size.GetY();
        }
        else
        {
            AZ_Assert(false, "unhandled scrollbar orientation");
        }
    }

    return handleParentLength;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiScrollBarComponent::GetPosAlongAxis(AZ::Vector2 point)
{
    float posAlongAxis = 0.0f;

    AZ::Entity* handleParentEntity = nullptr;
    EBUS_EVENT_ID_RESULT(handleParentEntity, m_handleEntity, UiElementBus, GetParent);

    if (handleParentEntity)
    {
        AZ::Matrix4x4 transform;
        EBUS_EVENT_ID(handleParentEntity->GetId(), UiTransformBus, GetTransformFromViewport, transform);

        AZ::Vector3 point3(point.GetX(), point.GetY(), 0.0f);
        point3 = transform * point3;

        if (m_orientation == Orientation::Horizontal)
        {
            posAlongAxis = point3.GetX();
        }
        else if (m_orientation == Orientation::Vertical)
        {
            posAlongAxis = point3.GetY();
        }
        else
        {
            AZ_Assert(false, "unhandled scrollbar orientation");
        }
    }

    return posAlongAxis;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiScrollBarComponent::MoveHandle(LocRelativeToHandle pointLoc)
{
    float valueStep = (m_displayedHandleSize < 1.0f) ? m_displayedHandleSize / (1.0f - m_displayedHandleSize) : 0.0f;

    float newValue = m_value;
    if (pointLoc == LocRelativeToHandle::BeforeHandle)
    {
        newValue -= valueStep;
    }
    else if (pointLoc == LocRelativeToHandle::AfterHandle)
    {
        newValue += valueStep;
    }

    newValue = AZ::GetClamp(newValue, 0.0f, 1.0f);

    if (newValue != m_value)
    {
        DoSetValue(newValue);
        DoChangingActions();

        NotifyScrollableOnValueChanging();

        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBarComponent::ResetDragInfo()
{
    m_pressedValue = m_value;
    m_pressedPoint = m_lastDragPoint;
    m_pressedPosAlongAxis = GetPosAlongAxis(m_pressedPoint);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBarComponent::SetImageComponentsAlpha(float fade)
{
    UiImageBus::Event(GetEntityId(), &UiImageBus::Events::SetAlpha, m_initialScrollBarAlpha * fade);
    UiImageBus::Event(m_handleEntity, &UiImageBus::Events::SetAlpha, m_initialHandleAlpha * fade);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBarComponent::ResetFade()
{
    m_currFade = 1;
    m_secondsRemainingBeforeFade = m_inactiveSecondsBeforeFade;
    m_isFading = false;
    SetImageComponentsAlpha(m_currFade);
}
