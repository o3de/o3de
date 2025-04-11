/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiTooltipDisplayComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/sort.h>
#include <AzCore/Time/ITime.h>

#include <LyShine/Bus/UiTransform2dBus.h>
#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiTextBus.h>
#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/Bus/UiTooltipDataPopulatorBus.h>
#include <LyShine/UiSerializeHelpers.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiTooltipDisplayComponent::UiTooltipDisplayComponent()
    : m_triggerMode(TriggerMode::OnHover)
    , m_autoPosition(true)
    , m_autoPositionMode(AutoPositionMode::OffsetFromMouse)
    , m_offset(0.0f, -10.0f)
    , m_autoSize(true)
    , m_delayTime(0.5f)
    , m_displayTime(5.0f)
    , m_state(State::Hidden)
    , m_stateStartTime(-1.0f)
    , m_curDelayTime(-1.0f)
    , m_timeSinceLastShown(-1.0f)
    , m_maxWrapTextWidth(-1.0f)
    , m_showSequence(nullptr)
    , m_hideSequence(nullptr)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiTooltipDisplayComponent::~UiTooltipDisplayComponent()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiTooltipDisplayInterface::TriggerMode UiTooltipDisplayComponent::GetTriggerMode()
{
    return m_triggerMode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTooltipDisplayComponent::SetTriggerMode(TriggerMode triggerMode)
{
    m_triggerMode = triggerMode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiTooltipDisplayComponent::GetAutoPosition()
{
    return m_autoPosition;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTooltipDisplayComponent::SetAutoPosition(bool autoPosition)
{
    m_autoPosition = autoPosition;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiTooltipDisplayInterface::AutoPositionMode UiTooltipDisplayComponent::GetAutoPositionMode()
{
    return m_autoPositionMode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTooltipDisplayComponent::SetAutoPositionMode(AutoPositionMode autoPositionMode)
{
    m_autoPositionMode = autoPositionMode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const AZ::Vector2& UiTooltipDisplayComponent::GetOffset()
{
    return m_offset;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTooltipDisplayComponent::SetOffset(const AZ::Vector2& offset)
{
    m_offset = offset;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiTooltipDisplayComponent::GetAutoSize()
{
    return m_autoSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTooltipDisplayComponent::SetAutoSize(bool autoSize)
{
    m_autoSize = autoSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiTooltipDisplayComponent::GetTextEntity()
{
    return m_textEntity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTooltipDisplayComponent::SetTextEntity(AZ::EntityId textEntity)
{
    m_textEntity = textEntity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiTooltipDisplayComponent::GetDelayTime()
{
    return m_delayTime;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTooltipDisplayComponent::SetDelayTime(float delayTime)
{
    m_delayTime = delayTime;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiTooltipDisplayComponent::GetDisplayTime()
{
    return m_displayTime;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTooltipDisplayComponent::SetDisplayTime(float displayTime)
{
    m_displayTime = displayTime;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTooltipDisplayComponent::PrepareToShow(AZ::EntityId tooltipElement)
{
    m_tooltipElement = tooltipElement;

    // We should be in the hidden state
    AZ_Assert(((m_state == State::Hiding) || (m_state == State::Hidden)), "State is not hidden when attempting to show tooltip");
    if (m_state != State::Hidden)
    {
        EndTransitionState();
        SetState(State::Hidden);
    }

    m_curDelayTime = m_delayTime;
    

    SetState(State::DelayBeforeShow);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTooltipDisplayComponent::Hide()
{
    switch (m_state)
    {
    case State::Showing:
    {
        // Since sequences can't have keys that represent current values,
        // only play the hide animation if the show animation has completed.
        const AZ::TimeMs realTimeMs = AZ::GetRealElapsedTimeMs();
        m_timeSinceLastShown = AZ::TimeMsToSeconds(realTimeMs);

        EndTransitionState();

        SetState(State::Hidden);
    }
    break;

    case State::Shown:
    {
        const AZ::TimeMs realTimeMs = AZ::GetRealElapsedTimeMs();
        m_timeSinceLastShown = AZ::TimeMsToSeconds(realTimeMs);

        // Check if there is a hide animation to play
        IUiAnimationSystem* animSystem = nullptr;
        PrepareSequenceForPlay(m_hideSequenceName, m_hideSequence, animSystem);

        if (m_hideSequence)
        {
            SetState(State::Hiding);

            animSystem->PlaySequence(m_hideSequence, nullptr, false, false);
        }
        else
        {
            SetState(State::Hidden);
        }
    }
    break;

    case State::DelayBeforeShow:
    {
        SetState(State::Hidden);
    }
    break;

    default:
        break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTooltipDisplayComponent::Update()
{
    if (m_state == State::DelayBeforeShow)
    {
        // Check if it's time to show the tooltip
        const AZ::TimeMs realTimeMs = AZ::GetRealElapsedTimeMs();
        const float currentTime = AZ::TimeMsToSeconds(realTimeMs);
        if ((currentTime - m_stateStartTime) >= m_curDelayTime)
        {
            // Make sure nothing has changed with the hover interactable
            if (m_tooltipElement.IsValid() && UiTooltipDataPopulatorBus::FindFirstHandler(m_tooltipElement))
            {
                Show();
            }
            else
            {
                Hide();
            }
        }
    }
    else if (m_state == State::Shown)
    {
        // Check if it's time to hide the tooltip
        if (m_displayTime >= 0.0f)
        {
            const AZ::TimeMs realTimeMs = AZ::GetRealElapsedTimeMs();
            const float currentTime = AZ::TimeMsToSeconds(realTimeMs);
            if ((currentTime - m_stateStartTime) >= m_displayTime)
            {
                // Hide tooltip
                Hide();
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTooltipDisplayComponent::InGamePostActivate()
{
    SetState(State::Hidden);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTooltipDisplayComponent::OnUiAnimationEvent(EUiAnimationEvent uiAnimationEvent, IUiAnimSequence* animSequence)
{
    if (m_state == State::Showing && animSequence == m_showSequence)
    {
        if ((uiAnimationEvent == eUiAnimationEvent_Stopped) || (uiAnimationEvent == eUiAnimationEvent_Aborted))
        {
            SetState(State::Shown);
        }
    }
    else if (m_state == State::Hiding && animSequence == m_hideSequence)
    {
        if ((uiAnimationEvent == eUiAnimationEvent_Stopped) || (uiAnimationEvent == eUiAnimationEvent_Aborted))
        {
            SetState(State::Hidden);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiTooltipDisplayComponent::State UiTooltipDisplayComponent::GetState()
{
    return m_state;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

void UiTooltipDisplayComponent::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

    if (serializeContext)
    {
        serializeContext->Class<UiTooltipDisplayComponent, AZ::Component>()
            ->Version(2, &VersionConverter)
            ->Field("TriggerMode", &UiTooltipDisplayComponent::m_triggerMode)
            ->Field("AutoPosition", &UiTooltipDisplayComponent::m_autoPosition)
            ->Field("AutoPositionMode", &UiTooltipDisplayComponent::m_autoPositionMode)
            ->Field("Offset", &UiTooltipDisplayComponent::m_offset)
            ->Field("AutoSize", &UiTooltipDisplayComponent::m_autoSize)
            ->Field("Text", &UiTooltipDisplayComponent::m_textEntity)
            ->Field("DelayTime", &UiTooltipDisplayComponent::m_delayTime)
            ->Field("DisplayTime", &UiTooltipDisplayComponent::m_displayTime)
            ->Field("ShowSequence", &UiTooltipDisplayComponent::m_showSequenceName)
            ->Field("HideSequence", &UiTooltipDisplayComponent::m_hideSequenceName);

        AZ::EditContext* ec = serializeContext->GetEditContext();
        if (ec)
        {
            auto editInfo = ec->Class<UiTooltipDisplayComponent>("TooltipDisplay", "A component that handles how the tooltip element is to be displayed.");

            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Category, "UI")
                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/UiTooltipDisplay.png")
                ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/UiTooltipDisplay.png")
                ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("UI"))
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiTooltipDisplayComponent::m_triggerMode, "Trigger Mode",
                "Sets the way the tooltip is triggered to display.")
                ->EnumAttribute(UiTooltipDisplayInterface::TriggerMode::OnHover, "On Hover")
                ->EnumAttribute(UiTooltipDisplayInterface::TriggerMode::OnPress, "On Press")
                ->EnumAttribute(UiTooltipDisplayInterface::TriggerMode::OnClick, "On Click");
            editInfo->DataElement(0, &UiTooltipDisplayComponent::m_autoPosition, "Auto position",
                "Whether the element will automatically be positioned.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC_CE("RefreshEntireTree"));
            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiTooltipDisplayComponent::m_autoPositionMode, "Positioning",
                "Sets the positioning behavior of the element.")
                ->EnumAttribute(UiTooltipDisplayInterface::AutoPositionMode::OffsetFromMouse, "Offset from mouse")
                ->EnumAttribute(UiTooltipDisplayInterface::AutoPositionMode::OffsetFromElement, "Offset from element")
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiTooltipDisplayComponent::m_autoPosition);
            editInfo->DataElement(0, &UiTooltipDisplayComponent::m_offset, "Offset",
                "The offset to use when positioning the element.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiTooltipDisplayComponent::m_autoPosition);
            editInfo->DataElement(0, &UiTooltipDisplayComponent::m_autoSize, "Auto size",
                "Whether the element will automatically be sized so that the text element's size is the same as the size of the tooltip string.\n"
                "If auto size is on, the text element's anchors should be apart.");
            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiTooltipDisplayComponent::m_textEntity, "Text",
                "The UI element to hold the main tooltip text. Also used for auto sizing.")
                ->Attribute(AZ::Edit::Attributes::EnumValues, &UiTooltipDisplayComponent::PopulateTextEntityList);
            editInfo->DataElement(0, &UiTooltipDisplayComponent::m_delayTime, "Delay Time",
                "The amount of time to wait before displaying the element.");
            editInfo->DataElement(0, &UiTooltipDisplayComponent::m_displayTime, "Display Time",
                "The amount of time the element is to be displayed.");
            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiTooltipDisplayComponent::m_showSequenceName, "Show Sequence",
                "The sequence to be played when the element is about to show.")
                ->Attribute(AZ::Edit::Attributes::StringList, &UiTooltipDisplayComponent::PopulateSequenceList);
            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiTooltipDisplayComponent::m_hideSequenceName, "Hide Sequence",
                "The sequence to be played when the element is about to hide.")
                ->Attribute(AZ::Edit::Attributes::StringList, &UiTooltipDisplayComponent::PopulateSequenceList);
        }
    }

    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
    if (behaviorContext)
    {
        behaviorContext->Enum<(int)UiTooltipDisplayInterface::AutoPositionMode::OffsetFromMouse>("eUiTooltipDisplayAutoPositionMode_OffsetFromMouse")
            ->Enum<(int)UiTooltipDisplayInterface::AutoPositionMode::OffsetFromElement>("eUiTooltipDisplayAutoPositionMode_OffsetFromElement");

        behaviorContext->Enum<(int)UiTooltipDisplayInterface::TriggerMode::OnHover>("eUiTooltipDisplayTriggerMode_OnHover")
            ->Enum<(int)UiTooltipDisplayInterface::TriggerMode::OnPress>("eUiTooltipDisplayTriggerMode_OnPress")
            ->Enum<(int)UiTooltipDisplayInterface::TriggerMode::OnPress>("eUiTooltipDisplayTriggerMode_OnClick");

        behaviorContext->EBus<UiTooltipDisplayBus>("UiTooltipDisplayBus")
            ->Event("GetTriggerMode", &UiTooltipDisplayBus::Events::GetTriggerMode)
            ->Event("SetTriggerMode", &UiTooltipDisplayBus::Events::SetTriggerMode)
            ->Event("GetAutoPosition", &UiTooltipDisplayBus::Events::GetAutoPosition)
            ->Event("SetAutoPosition", &UiTooltipDisplayBus::Events::SetAutoPosition)
            ->Event("GetAutoPositionMode", &UiTooltipDisplayBus::Events::GetAutoPositionMode)
            ->Event("SetAutoPositionMode", &UiTooltipDisplayBus::Events::SetAutoPositionMode)
            ->Event("GetOffset", &UiTooltipDisplayBus::Events::GetOffset)
            ->Event("SetOffset", &UiTooltipDisplayBus::Events::SetOffset)
            ->Event("GetAutoSize", &UiTooltipDisplayBus::Events::GetAutoSize)
            ->Event("SetAutoSize", &UiTooltipDisplayBus::Events::SetAutoSize)
            ->Event("GetTextEntity", &UiTooltipDisplayBus::Events::GetTextEntity)
            ->Event("SetTextEntity", &UiTooltipDisplayBus::Events::SetTextEntity)
            ->Event("GetDelayTime", &UiTooltipDisplayBus::Events::GetDelayTime)
            ->Event("SetDelayTime", &UiTooltipDisplayBus::Events::SetDelayTime)
            ->Event("GetDisplayTime", &UiTooltipDisplayBus::Events::GetDisplayTime)
            ->Event("SetDisplayTime", &UiTooltipDisplayBus::Events::SetDisplayTime)
            ->VirtualProperty("DelayTime", "GetDelayTime", "SetDelayTime")
            ->VirtualProperty("DisplayTime", "GetDisplayTime", "SetDisplayTime");

        behaviorContext->Class<UiTooltipDisplayComponent>()->RequestBus("UiTooltipDisplayBus");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTooltipDisplayComponent::Activate()
{
    UiTooltipDisplayBus::Handler::BusConnect(GetEntityId());
    UiInitializationBus::Handler::BusConnect(GetEntityId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTooltipDisplayComponent::Deactivate()
{
    UiTooltipDisplayBus::Handler::BusDisconnect(GetEntityId());
    UiInitializationBus::Handler::BusDisconnect(GetEntityId());

    // Stop listening for animation actions. The sequences may have been deleted at this point,
    // so first check if they still exist.
    IUiAnimationSystem* animSystem = nullptr;
    IUiAnimSequence* sequence;

    if (m_listeningForAnimationEvents)
    {
        m_listeningForAnimationEvents = false;
        sequence = GetSequence(m_showSequenceName, animSystem);
        if (sequence)
        {
            animSystem->RemoveUiAnimationListener(sequence, this);
        }

        sequence = GetSequence(m_hideSequenceName, animSystem);
        if (sequence)
        {
            animSystem->RemoveUiAnimationListener(sequence, this);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTooltipDisplayComponent::SetState(State state)
{
    m_state = state;
    const AZ::TimeMs realTimeMs = AZ::GetRealElapsedTimeMs();
    m_stateStartTime = AZ::TimeMsToSeconds(realTimeMs);

    switch (m_state)
    {
    case State::Hiding:
        UiTooltipDisplayNotificationBus::Event(m_tooltipElement, &UiTooltipDisplayNotificationBus::Events::OnHiding);
        break;

    case State::Hidden:
        UiTooltipDisplayNotificationBus::Event(m_tooltipElement, &UiTooltipDisplayNotificationBus::Events::OnHidden);
        UiElementBus::Event(GetEntityId(), &UiElementBus::Events::SetIsEnabled, false);
        break;

    case State::Showing:
    case State::Shown:
        UiElementBus::Event(GetEntityId(), &UiElementBus::Events::SetIsEnabled, true);
        if (m_state == State::Showing)
        {
            UiTooltipDisplayNotificationBus::Event(m_tooltipElement, &UiTooltipDisplayNotificationBus::Events::OnShowing);
        }
        else
        {
            UiTooltipDisplayNotificationBus::Event(m_tooltipElement, &UiTooltipDisplayNotificationBus::Events::OnShown);
        }
        break;

    default:
        break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTooltipDisplayComponent::EndTransitionState()
{
    if (m_state == State::Showing)
    {
        if (m_showSequence)
        {
            IUiAnimationSystem* animSystem = GetAnimationSystem();
            if (animSystem)
            {
                animSystem->StopSequence(m_showSequence);
            }
        }

        SetState(State::Shown);
    }
    else if (m_state == State::Hiding)
    {
        if (m_hideSequence)
        {
            IUiAnimationSystem* animSystem = GetAnimationSystem();
            if (animSystem)
            {
                animSystem->StopSequence(m_hideSequence);
            }
        }

        SetState(State::Hidden);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTooltipDisplayComponent::Show()
{
    // It is assumed that current state is DelayBeforeShow

    if (m_autoSize && m_textEntity.IsValid())
    {
        // Initialize max wrap text width
        if (m_maxWrapTextWidth < 0.0f)
        {
            UiTransformInterface::Rect textRect;
            UiTransformBus::Event(m_textEntity, &UiTransformBus::Events::GetCanvasSpaceRectNoScaleRotate, textRect);

            m_maxWrapTextWidth = textRect.GetWidth();
        }

        // If wrapping is on, reset display element to its original width
        UiTextInterface::WrapTextSetting wrapSetting = UiTextInterface::WrapTextSetting::NoWrap;
        UiTextBus::EventResult(wrapSetting, m_textEntity, &UiTextBus::Events::GetWrapText);
        if (wrapSetting != UiTextInterface::WrapTextSetting::NoWrap)
        {
            UiTransformInterface::Rect textRect;
            UiTransformBus::Event(m_textEntity, &UiTransformBus::Events::GetCanvasSpaceRectNoScaleRotate, textRect);

            AZ::Vector2 delta(m_maxWrapTextWidth - textRect.GetWidth(), 0.0f);

            ResizeElementByDelta(GetEntityId(), delta);
        }
    }

    // Assign tooltip data to the tooltip display element
    UiTooltipDataPopulatorBus::Event(m_tooltipElement, &UiTooltipDataPopulatorBus::Events::PushDataToDisplayElement, GetEntityId());

    // Auto-resize display element so that the text element is the same size as the size of its text string
    if (m_autoSize && m_textEntity.IsValid())
    {
        AutoResize();
    }

    // Auto-position display element
    if (m_autoPosition)
    {
        AutoPosition();
    }

    // Check if there is a show animation to play
    IUiAnimationSystem* animSystem = nullptr;
    PrepareSequenceForPlay(m_showSequenceName, m_showSequence, animSystem);

    if (m_showSequence)
    {
        SetState(State::Showing);

        animSystem->PlaySequence(m_showSequence, nullptr, false, false);
    }
    else
    {
        SetState(State::Shown);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTooltipDisplayComponent::AutoResize()
{
    // Get the text string size
    AZ::Vector2 stringSize(-1.0f, -1.0f);
    UiTextBus::EventResult(stringSize, m_textEntity, &UiTextBus::Events::GetTextSize);

    if (stringSize.GetX() < 0.0f || stringSize.GetY() < 0.0f)
    {
        return;
    }

    // Get the text element's rect
    UiTransformInterface::Rect textRect;
    UiTransformBus::Event(m_textEntity, &UiTransformBus::Events::GetCanvasSpaceRectNoScaleRotate, textRect);

    // Get the difference between the text element's size and the string size
    AZ::Vector2 delta(stringSize.GetX() - textRect.GetWidth(), stringSize.GetY() - textRect.GetHeight());

    UiTextInterface::WrapTextSetting wrapSetting;
    UiTextBus::EventResult(wrapSetting, m_textEntity, &UiTextBus::Events::GetWrapText);
    if (wrapSetting != UiTextInterface::WrapTextSetting::NoWrap)
    {
        // In order for the wrapping to remain the same after the resize, the
        // text element width would need to match the string width exactly. To accommodate
        // for slight variation in size, add a small value to ensure that the string will fit
        // inside the text element's bounds. The downside to this is there may be extra space
        // at the bottom, but this is unlikely.
        const float epsilon = 0.01f;
        delta.SetX(delta.GetX() + epsilon);
    }

    // Resize the display element by the difference
    ResizeElementByDelta(GetEntityId(), delta);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTooltipDisplayComponent::ResizeElementByDelta(AZ::EntityId entityId, const AZ::Vector2& delta)
{
    if ((delta.GetX() != 0.0f) || (delta.GetY() != 0.0f))
    {
        // Resize the element based on the difference
        UiTransform2dInterface::Offsets offsets;
        UiTransform2dBus::EventResult(offsets, entityId, &UiTransform2dBus::Events::GetOffsets);

        AZ::Vector2 pivot;

        UiTransformBus::EventResult(pivot, entityId, &UiTransformBus::Events::GetPivot);

        if (delta.GetX() != 0.0f)
        {
            float leftOffsetDelta = delta.GetX() * pivot.GetX();
            offsets.m_left -= leftOffsetDelta;
            offsets.m_right += delta.GetX() - leftOffsetDelta;
        }
        if (delta.GetY() != 0.0f)
        {
            float topOffsetDelta = delta.GetY() * pivot.GetY();
            offsets.m_top -= topOffsetDelta;
            offsets.m_bottom += delta.GetY() - topOffsetDelta;
        }

        UiTransform2dBus::Event(entityId, &UiTransform2dBus::Events::SetOffsets, offsets);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTooltipDisplayComponent::AutoPosition()
{
    bool havePosition = false;
    AZ::Vector2 position(-1.0f, -1.0f);
    if (m_autoPositionMode == UiTooltipDisplayInterface::AutoPositionMode::OffsetFromMouse)
    {
        // Get current mouse position
        AZ::EntityId canvasId;
        UiElementBus::EventResult(canvasId, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);

        UiCanvasBus::EventResult(position, canvasId, &UiCanvasBus::Events::GetMousePosition);

        if (position.GetX() >= 0.0f && position.GetY() >= 0.0f)
        {
            // Check if the mouse is hovering over the tooltip element
            UiTransformBus::EventResult(havePosition, m_tooltipElement, &UiTransformBus::Events::IsPointInRect, position);
        }
    }

    if (!havePosition)
    {
        // Get the pivot position of the tooltip element
        UiTransformBus::EventResult(position, m_tooltipElement, &UiTransformBus::Events::GetViewportSpacePivot);
    }

    MoveToPosition(position, m_offset);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTooltipDisplayComponent::MoveToPosition(const AZ::Vector2& point, const AZ::Vector2& offsetFromPoint)
{
    // Move the display element so that its pivot is a certain distance from the point

    UiTransform2dInterface::Offsets offsets;
    UiTransform2dBus::EventResult(offsets, GetEntityId(), &UiTransform2dBus::Events::GetOffsets);

    AZ::Vector2 pivot;
    UiTransformBus::EventResult(pivot, GetEntityId(), &UiTransformBus::Events::GetViewportSpacePivot);

    AZ::Vector2 moveDist = (point + offsetFromPoint) - pivot;
    offsets += moveDist;

    UiTransform2dBus::Event(GetEntityId(), &UiTransform2dBus::Events::SetOffsets, offsets);

    // Make sure that the display element stays within the bounds of its parent

    // Get parent rect
    UiTransformInterface::Rect parentRect;

    AZ::Entity* parentEntity = nullptr;
    UiElementBus::EventResult(parentEntity, GetEntityId(), &UiElementBus::Events::GetParent);

    if (parentEntity)
    {
        UiTransformBus::Event(parentEntity->GetId(), &UiTransformBus::Events::GetCanvasSpaceRectNoScaleRotate, parentRect);
    }
    else
    {
        AZ::EntityId canvasEntityId;
        UiElementBus::EventResult(canvasEntityId, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);

        AZ::Vector2 size;
        UiCanvasBus::EventResult(size, canvasEntityId, &UiCanvasBus::Events::GetCanvasSize);

        parentRect.Set(0.0f, size.GetX(), 0.0f, size.GetY());
    }

    // If the display element exceeds the top/bottom bounds of its parent, the offset
    // is flipped and applied to the top/bottom of the display element.
    // If positioning is to be relative to the tooltip element though, this step is skipped
    // to preserve the original position as much as possible.
    if (m_autoPositionMode != UiTooltipDisplayInterface::AutoPositionMode::OffsetFromElement)
    {
        CheckBoundsAndChangeYPosition(parentRect, point.GetY(), fabs(m_offset.GetY()));
    }

    ConstrainToBounds(parentRect);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTooltipDisplayComponent::CheckBoundsAndChangeYPosition(const UiTransformInterface::Rect& boundsRect, float yPoint, float yOffsetFromPoint)
{
    // Get display element rect
    UiTransformInterface::Rect rect = GetAxisAlignedRect();

    float yMoveDist = 0.0f;

    // Check upper and lower bounds
    if (rect.top < boundsRect.top)
    {
        // Move top of display element to an offset below the point
        yMoveDist = (yPoint + yOffsetFromPoint) - rect.top;
    }
    else if (rect.bottom > boundsRect.bottom)
    {
        // Move bottom of display element to an offset above the point
        yMoveDist = (yPoint - yOffsetFromPoint) - rect.bottom;
    }

    if (yMoveDist != 0.0f)
    {
        UiTransform2dInterface::Offsets offsets;
        UiTransform2dBus::EventResult(offsets, GetEntityId(), &UiTransform2dBus::Events::GetOffsets);

        offsets.m_top += yMoveDist;
        offsets.m_bottom += yMoveDist;

        UiTransform2dBus::Event(GetEntityId(), &UiTransform2dBus::Events::SetOffsets, offsets);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTooltipDisplayComponent::ConstrainToBounds(const UiTransformInterface::Rect& boundsRect)
{
    // Get display element rect
    UiTransformInterface::Rect rect = GetAxisAlignedRect();

    AZ::Vector2 moveDist(0.0f, 0.0f);

    // Check left and right bounds
    if (rect.left < boundsRect.left)
    {
        moveDist.SetX(boundsRect.left - rect.left);
    }
    else if (rect.right > boundsRect.right)
    {
        moveDist.SetX(boundsRect.right - rect.right);
    }

    // Check upper and lower bounds
    if (rect.top < boundsRect.top)
    {
        moveDist.SetY(boundsRect.top - rect.top);
    }
    else if (rect.bottom > boundsRect.bottom)
    {
        moveDist.SetY(boundsRect.bottom - rect.bottom);
    }

    if (moveDist.GetX() != 0.0f || moveDist.GetY() != 0.0f)
    {
        UiTransform2dInterface::Offsets offsets;
        UiTransform2dBus::EventResult(offsets, GetEntityId(), &UiTransform2dBus::Events::GetOffsets);

        offsets += moveDist;

        UiTransform2dBus::Event(GetEntityId(), &UiTransform2dBus::Events::SetOffsets, offsets);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiTransformInterface::Rect UiTooltipDisplayComponent::GetAxisAlignedRect()
{
    UiTransformInterface::RectPoints points;
    UiTransformBus::Event(GetEntityId(), &UiTransformBus::Events::GetCanvasSpacePointsNoScaleRotate, points);

    AZ::Matrix4x4 transform;
    UiTransformBus::Event(GetEntityId(), &UiTransformBus::Events::GetLocalTransform, transform);

    points = points.Transform(transform);

    UiTransformInterface::Rect rect;
    rect.left = points.GetAxisAlignedTopLeft().GetX();
    rect.right = points.GetAxisAlignedBottomRight().GetX();
    rect.top = points.GetAxisAlignedTopLeft().GetY();
    rect.bottom = points.GetAxisAlignedBottomRight().GetY();

    return rect;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
IUiAnimationSystem* UiTooltipDisplayComponent::GetAnimationSystem()
{
    AZ::EntityId canvasId;
    UiElementBus::EventResult(canvasId, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);

    IUiAnimationSystem* animSystem = nullptr;
    UiCanvasBus::EventResult(animSystem, canvasId, &UiCanvasBus::Events::GetAnimationSystem);

    return animSystem;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
IUiAnimSequence* UiTooltipDisplayComponent::GetSequence(const AZStd::string& sequenceName, IUiAnimationSystem*& animSystemOut)
{
    IUiAnimSequence* sequence = nullptr;

    if (!sequenceName.empty() && (sequenceName != "<None>"))
    {
        IUiAnimationSystem* animSystem = GetAnimationSystem();
        if (animSystem)
        {
            sequence = animSystem->FindSequence(sequenceName.c_str());

            animSystemOut = animSystem;
        }
    }

    return sequence;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTooltipDisplayComponent::PrepareSequenceForPlay(const AZStd::string& sequenceName, IUiAnimSequence*& sequence, IUiAnimationSystem*& animSystemOut)
{
    IUiAnimationSystem* animSystem = nullptr;

    if (!sequence)
    {
        sequence = GetSequence(sequenceName, animSystem);

        if (sequence)
        {
            m_listeningForAnimationEvents = true;
            animSystem->AddUiAnimationListener(sequence, this);
        }
    }
    else
    {
        animSystem = GetAnimationSystem();
        if (!animSystem)
        {
            sequence = nullptr;
        }
    }

    if (sequence)
    {
        animSystemOut = animSystem;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiTooltipDisplayComponent::EntityComboBoxVec UiTooltipDisplayComponent::PopulateTextEntityList()
{
    EntityComboBoxVec result;

    // add a first entry for "None"
    result.push_back(AZStd::make_pair(AZ::EntityId(AZ::EntityId()), "<None>"));

    // allow the destination to be the same entity as the source by
    // adding this entity (if it has a text component)
    if (UiTextBus::FindFirstHandler(GetEntityId()))
    {
        result.push_back(AZStd::make_pair(AZ::EntityId(GetEntityId()), GetEntity()->GetName()));
    }

    // Get a list of all descendant elements that support the UiTextBus
    LyShine::EntityArray matchingElements;
    UiElementBus::Event(
        GetEntityId(),
        &UiElementBus::Events::FindDescendantElements,
        [](const AZ::Entity* entity)
        {
            return UiTextBus::FindFirstHandler(entity->GetId()) != nullptr;
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
UiTooltipDisplayComponent::SequenceComboBoxVec UiTooltipDisplayComponent::PopulateSequenceList()
{
    SequenceComboBoxVec result;

    // add a first entry for "None"
    result.push_back("<None>");

    IUiAnimationSystem* animSystem = GetAnimationSystem();
    if (animSystem)
    {
        for (int i = 0; i < animSystem->GetNumSequences(); i++)
        {
            IUiAnimSequence* sequence = animSystem->GetSequence(i);
            if (sequence)
            {
                result.push_back(sequence->GetName());
            }
        }
    }

    // Sort the sequence names
    SequenceComboBoxVec::iterator iterBegin = result.begin();
    ++iterBegin;
    if (iterBegin < result.end())
    {
        AZStd::sort(iterBegin, result.end(),
            [](const AZStd::string s1, const AZStd::string s2) { return s1 < s2; });
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiTooltipDisplayComponent::VersionConverter(AZ::SerializeContext& context,
    AZ::SerializeContext::DataElementNode& classElement)
{
    // conversion from version 1:
    // - Need to convert Vec2 to AZ::Vector2
    if (classElement.GetVersion() <= 1)
    {
        if (!LyShine::ConvertSubElementFromVec2ToVector2(context, classElement, "Offset"))
        {
            return false;
        }
    }

    return true;
}
