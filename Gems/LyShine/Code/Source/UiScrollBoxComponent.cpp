/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiScrollBoxComponent.h"
#include "Sprite.h"

#include <AzCore/Component/TickBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/sort.h>

#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiTransform2dBus.h>
#include <LyShine/Bus/UiTransformBus.h>
#include <LyShine/Bus/UiVisualBus.h>

#include <LyShine/ISprite.h>
#include <LyShine/UiSerializeHelpers.h>
#include "UiSerialize.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
//! UiScrollBoxNotificationBus Behavior context handler class
class BehaviorUiScrollBoxNotificationBusHandler
    : public UiScrollBoxNotificationBus::Handler
    , public AZ::BehaviorEBusHandler
{
public:
    AZ_EBUS_BEHAVIOR_BINDER(BehaviorUiScrollBoxNotificationBusHandler, "{15CA0E45-F673-4E18-922F-D9DB1272CFEA}", AZ::SystemAllocator,
        OnScrollOffsetChanging, OnScrollOffsetChanged);

    void OnScrollOffsetChanging(AZ::Vector2 value) override
    {
        Call(FN_OnScrollOffsetChanging, value);
    }

    void OnScrollOffsetChanged(AZ::Vector2 value) override
    {
        Call(FN_OnScrollOffsetChanged, value);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
//! UiScrollableNotificationBus Behavior context handler class
class BehaviorUiScrollableNotificationBusHandler
    : public UiScrollableNotificationBus::Handler
    , public AZ::BehaviorEBusHandler
{
public:
    AZ_EBUS_BEHAVIOR_BINDER(BehaviorUiScrollableNotificationBusHandler, "{7F130E59-778C-4951-BB62-B2E57E530BC0}", AZ::SystemAllocator,
        OnScrollableValueChanging, OnScrollableValueChanged);

    void OnScrollableValueChanging(AZ::Vector2 value) override
    {
        Call(FN_OnScrollableValueChanging, value);
    }

    void OnScrollableValueChanged(AZ::Vector2 value) override
    {
        Call(FN_OnScrollableValueChanged, value);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiScrollBoxComponent::UiScrollBoxComponent()
    : m_scrollOffset(0.0f, 0.0f)
    , m_isHorizontalScrollingEnabled(true)
    , m_isVerticalScrollingEnabled(false)
    , m_isScrollingConstrained(true)
    , m_snapMode(SnapMode::None)
    , m_snapGrid(10.0f, 10.0f)
    , m_hScrollBarVisibility(ScrollBarVisibility::AlwaysShow)
    , m_vScrollBarVisibility(ScrollBarVisibility::AlwaysShow)
    , m_contentEntity()
    , m_hScrollBarEntity()
    , m_vScrollBarEntity()
    , m_onScrollOffsetChanged()
    , m_onScrollOffsetChanging()
    , m_scrollOffsetChangedActionName()
    , m_scrollOffsetChangingActionName()
    , m_isDragging(false)
    , m_isActive(false)
    , m_pressedScrollOffset(0.0f, 0.0f)
    , m_lastDragPoint(0.0f, 0.0f)
    , m_scrollSensitivity(1.0f, 1.0f)
    , m_lastOffsetChange(0.0f, 0.0f)
    , m_offsetChangeAccumulator(0.0f, 0.0f)
    , m_stoppingTimeAccumulator(0.0f)
    , m_draggingTimeAccumulator(0.0f)
    , m_momentumIsActive(false)
    , m_momentumDuration(0.0f)
    , m_momentumTimeAccumulator(0.0f)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiScrollBoxComponent::~UiScrollBoxComponent()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Vector2 UiScrollBoxComponent::GetScrollOffset()
{
    return m_scrollOffset;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBoxComponent::SetScrollOffset(AZ::Vector2 scrollOffset)
{
    if (m_isScrollingConstrained)
    {
        AZ::Entity* contentParentEntity = nullptr;
        UiElementBus::EventResult(contentParentEntity, m_contentEntity, &UiElementBus::Events::GetParent);

        if (contentParentEntity)
        {
            scrollOffset = ConstrainOffset(scrollOffset, contentParentEntity);
        }
    }

    if (scrollOffset != m_scrollOffset)
    {
        DoSetScrollOffset(scrollOffset);

        // Reset drag info
        if (m_isDragging)
        {
            m_pressedScrollOffset = m_scrollOffset;
            m_pressedPoint = m_lastDragPoint;
        }

        NotifyScrollersOnValueChanged();

        DoChangedActions();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Vector2 UiScrollBoxComponent::GetNormalizedScrollValue()
{
    AZ::Vector2 normalizedScrollValueOut(0.0f, 0.0f);
    ScrollOffsetToNormalizedScrollValue(m_scrollOffset, normalizedScrollValueOut);

    return normalizedScrollValueOut;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBoxComponent::ChangeContentSizeAndScrollOffset(AZ::Vector2 contentSize, AZ::Vector2 scrollOffset)
{
    if (m_contentEntity.IsValid())
    {
        AZ::Vector2 prevScrollOffset = m_scrollOffset;

        // Get current content size
        AZ::Vector2 prevContentSize(0.0f, 0.0f);
        UiTransformBus::EventResult(prevContentSize, m_contentEntity, &UiTransformBus::Events::GetCanvasSpaceSizeNoScaleRotate);

        // Resize content element
        if (prevContentSize != contentSize)
        {
            UiTransform2dInterface::Offsets offsets;
            UiTransform2dBus::EventResult(offsets, m_contentEntity, &UiTransform2dBus::Events::GetOffsets);

            AZ::Vector2 pivot;
            UiTransformBus::EventResult(pivot, m_contentEntity, &UiTransformBus::Events::GetPivot);

            AZ::Vector2 sizeDiff = contentSize - prevContentSize;

            if (sizeDiff.GetX() != 0.0f)
            {
                offsets.m_left -= sizeDiff.GetX() * pivot.GetX();
                offsets.m_right += sizeDiff.GetX() * (1.0f - pivot.GetX());
            }
            if (sizeDiff.GetY() != 0.0f)
            {
                offsets.m_top -= sizeDiff.GetY() * pivot.GetY();
                offsets.m_bottom += sizeDiff.GetY() * (1.0f - pivot.GetY());
            }

            UiTransform2dBus::Event(m_contentEntity, &UiTransform2dBus::Events::SetOffsets, offsets);
        }

        // Adjust scroll offset
        if (m_scrollOffset != scrollOffset)
        {
            DoSetScrollOffset(scrollOffset);
        }

        // Reset drag info
        if (m_isDragging)
        {
            m_pressedScrollOffset = m_scrollOffset;
            m_pressedPoint = m_lastDragPoint;
        }

        // Handle content size change which also handles snapping/constraining
        if (prevContentSize != contentSize)
        {
            ContentOrParentSizeChanged();
        }
        else
        {
            if (prevScrollOffset != m_scrollOffset)
            {
                NotifyScrollersOnValueChanged();
            }

            if (DoSnap())
            {
                // Reset drag info
                if (m_isDragging)
                {
                    m_pressedScrollOffset = m_scrollOffset;
                    m_pressedPoint = m_lastDragPoint;
                }

                NotifyScrollersOnValueChanged();

                DoChangedActions();
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiScrollBoxComponent::HasHorizontalContentToScroll()
{
    bool hasContentToScroll = false;

    if (!m_isHorizontalScrollingEnabled)
    {
        hasContentToScroll = false;
    }
    else if (!m_isScrollingConstrained)
    {
        hasContentToScroll = true;
    }
    else
    {
        if (m_hScrollBarEntity.IsValid() && (m_hScrollBarVisibility != ScrollBarVisibility::AlwaysShow))
        {
            bool isEnabled = false;
            UiElementBus::EventResult(isEnabled, m_hScrollBarEntity, &UiElementBus::Events::IsEnabled);
            hasContentToScroll = isEnabled;
        }
        else
        {
            AZ::Entity* contentParentEntity = nullptr;
            UiElementBus::EventResult(contentParentEntity, m_contentEntity, &UiElementBus::Events::GetParent);
            if (contentParentEntity)
            {
                // Get content parent's size
                AZ::Vector2 parentSize;
                UiTransformBus::EventResult(
                    parentSize, contentParentEntity->GetId(), &UiTransformBus::Events::GetCanvasSpaceSizeNoScaleRotate);

                // Get content size
                UiTransformInterface::Rect contentRect = GetAxisAlignedContentRect();
                AZ::Vector2 contentSize = contentRect.GetSize();

                hasContentToScroll = contentSize.GetX() > parentSize.GetX();
            }
        }
    }

    return hasContentToScroll;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiScrollBoxComponent::HasVerticalContentToScroll()
{
    bool hasContentToScroll = false;

    if (!m_isVerticalScrollingEnabled)
    {
        hasContentToScroll = false;
    }
    else if (!m_isScrollingConstrained)
    {
        hasContentToScroll = true;
    }
    else
    {
        if (m_vScrollBarEntity.IsValid() && (m_vScrollBarVisibility != ScrollBarVisibility::AlwaysShow))
        {
            bool isEnabled = false;
            UiElementBus::EventResult(isEnabled, m_vScrollBarEntity, &UiElementBus::Events::IsEnabled);
            hasContentToScroll = isEnabled;
        }
        else
        {
            AZ::Entity* contentParentEntity = nullptr;
            UiElementBus::EventResult(contentParentEntity, m_contentEntity, &UiElementBus::Events::GetParent);
            if (contentParentEntity)
            {
                // Get content parent's size
                AZ::Vector2 parentSize;
                UiTransformBus::EventResult(
                    parentSize, contentParentEntity->GetId(), &UiTransformBus::Events::GetCanvasSpaceSizeNoScaleRotate);

                // Get content size
                UiTransformInterface::Rect contentRect = GetAxisAlignedContentRect();
                AZ::Vector2 contentSize = contentRect.GetSize();

                hasContentToScroll = contentSize.GetY() > parentSize.GetY();
            }
        }
    }

    return hasContentToScroll;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiScrollBoxComponent::GetIsHorizontalScrollingEnabled()
{
    return m_isHorizontalScrollingEnabled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBoxComponent::SetIsHorizontalScrollingEnabled(bool isEnabled)
{
    m_isHorizontalScrollingEnabled = isEnabled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiScrollBoxComponent::GetIsVerticalScrollingEnabled()
{
    return m_isVerticalScrollingEnabled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBoxComponent::SetIsVerticalScrollingEnabled(bool isEnabled)
{
    m_isVerticalScrollingEnabled = isEnabled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiScrollBoxComponent::GetIsScrollingConstrained()
{
    return m_isScrollingConstrained;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBoxComponent::SetIsScrollingConstrained(bool isConstrained)
{
    m_isScrollingConstrained = isConstrained;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiScrollBoxInterface::SnapMode UiScrollBoxComponent::GetSnapMode()
{
    return m_snapMode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBoxComponent::SetSnapMode(SnapMode snapMode)
{
    m_snapMode = snapMode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Vector2 UiScrollBoxComponent::GetSnapGrid()
{
    return m_snapGrid;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBoxComponent::SetSnapGrid(AZ::Vector2 snapGrid)
{
    m_snapGrid = snapGrid;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiScrollBoxInterface::ScrollBarVisibility UiScrollBoxComponent::GetHorizontalScrollBarVisibility()
{
    return m_hScrollBarVisibility;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBoxComponent::SetHorizontalScrollBarVisibility(ScrollBarVisibility visibility)
{
    m_hScrollBarVisibility = visibility;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiScrollBoxInterface::ScrollBarVisibility UiScrollBoxComponent::GetVerticalScrollBarVisibility()
{
    return m_vScrollBarVisibility;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBoxComponent::SetVerticalScrollBarVisibility(ScrollBarVisibility visibility)
{
    m_vScrollBarVisibility = visibility;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Vector2 UiScrollBoxComponent::GetScrollSensitivity()
{
    return m_scrollSensitivity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBoxComponent::SetScrollSensitivity(AZ::Vector2 scrollSensitivity)
{
    m_scrollSensitivity = scrollSensitivity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiScrollBoxComponent::GetMomentumDuration()
{
    return m_momentumDuration;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBoxComponent::SetMomentumDuration(float scrollMomentumDuration)
{
    m_momentumDuration = scrollMomentumDuration;
}

void UiScrollBoxComponent::SetMomentumActive(bool activate)
{
    m_momentumIsActive = activate;

    if (m_momentumIsActive)
    {
        m_momentumTimeAccumulator = 0.0f;
    }
    else
    {
        m_offsetChangeAccumulator.Set(0.0f, 0.0f);
        m_draggingTimeAccumulator = 0.0f;
        m_stoppingTimeAccumulator = 0.0f;
    }
}

void UiScrollBoxComponent::StopMomentum()
{
    m_offsetChangeAccumulator.Set(0.0f, 0.0f);
    m_draggingTimeAccumulator = 0.0f;
    m_stoppingTimeAccumulator = 0.0f;
    m_momentumTimeAccumulator = 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiScrollBoxComponent::ScrollOffsetChangeCallback UiScrollBoxComponent::GetScrollOffsetChangingCallback()
{
    return m_onScrollOffsetChanging;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBoxComponent::SetScrollOffsetChangingCallback(ScrollOffsetChangeCallback onChange)
{
    m_onScrollOffsetChanging = onChange;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const LyShine::ActionName& UiScrollBoxComponent::GetScrollOffsetChangingActionName()
{
    return m_scrollOffsetChangingActionName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBoxComponent::SetScrollOffsetChangingActionName(const LyShine::ActionName& actionName)
{
    m_scrollOffsetChangingActionName = actionName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiScrollBoxComponent::ScrollOffsetChangeCallback UiScrollBoxComponent::GetScrollOffsetChangedCallback()
{
    return m_onScrollOffsetChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBoxComponent::SetScrollOffsetChangedCallback(ScrollOffsetChangeCallback onChange)
{
    m_onScrollOffsetChanged = onChange;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const LyShine::ActionName& UiScrollBoxComponent::GetScrollOffsetChangedActionName()
{
    return m_scrollOffsetChangedActionName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBoxComponent::SetScrollOffsetChangedActionName(const LyShine::ActionName& actionName)
{
    m_scrollOffsetChangedActionName = actionName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBoxComponent::SetContentEntity(AZ::EntityId entityId)
{
    m_contentEntity = entityId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiScrollBoxComponent::GetContentEntity()
{
    return m_contentEntity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBoxComponent::SetHorizontalScrollBarEntity(AZ::EntityId entityId)
{
    m_hScrollBarEntity = entityId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiScrollBoxComponent::GetHorizontalScrollBarEntity()
{
    return m_hScrollBarEntity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBoxComponent::SetVerticalScrollBarEntity(AZ::EntityId entityId)
{
    m_vScrollBarEntity = entityId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiScrollBoxComponent::GetVerticalScrollBarEntity()
{
    return m_vScrollBarEntity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiScrollBoxComponent::FindClosestContentChildElement()
{
    // if no content entity return an invalid entity id
    if (!m_contentEntity.IsValid())
    {
        return AZ::EntityId();
    }

    // Iterate over the children of the content element and find the one that has the smallest
    // offset from the content elements anchors to the child's pivot.
    // E.g. if the anchors are the center of the content (the default) and the chilren's pivots
    // are in their centers (the default) then we will find the child whose center is closest
    // to the center of the content element's parent (usually the mask element)
    LyShine::EntityArray children;
    UiElementBus::EventResult(children, m_contentEntity, &UiElementBus::Events::GetChildElements);

    float closestDistSq = FLT_MAX;
    AZ::EntityId closestChild;

    for (auto child : children)
    {
        AZ::Vector2 scrollOffsetToChild = ComputeCurrentOffsetToChild(child->GetId());

        float distSq = scrollOffsetToChild.GetLengthSq();
        if (distSq < closestDistSq)
        {
            closestChild = child->GetId();
            closestDistSq = distSq;
        }
    }

    return closestChild;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiScrollBoxComponent::FindNextContentChildElement(UiNavigationHelpers::Command command)
{
    // if no content entity return an invalid entity id
    if (!m_contentEntity.IsValid())
    {
        return AZ::EntityId();
    }

    // Iterate over the children of the content element and find the one who's pivot is closest to
    // the content element's anchors in the specified direction.
    LyShine::EntityArray children;
    UiElementBus::EventResult(children, m_contentEntity, &UiElementBus::Events::GetChildElements);

    float shortestDist = FLT_MAX;
    float shortestPerpendicularDist = FLT_MAX;
    AZ::EntityId closestChild;

    for (auto child : children)
    {
        AZ::Vector2 scrollOffsetToChild = ComputeCurrentOffsetToChild(child->GetId());

        float dist = 0.0f;
        const float epsilon = 0.01f;
        if (command == UiNavigationHelpers::Command::Up)
        {
            dist = scrollOffsetToChild.GetY() < -epsilon ? -scrollOffsetToChild.GetY() : 0.0f;
        }
        else if (command == UiNavigationHelpers::Command::Down)
        {
            dist = scrollOffsetToChild.GetY() > epsilon ? scrollOffsetToChild.GetY() : 0.0f;
        }
        else if (command == UiNavigationHelpers::Command::Left)
        {
            dist = scrollOffsetToChild.GetX() < -epsilon ? -scrollOffsetToChild.GetX() : 0.0f;
        }
        else if (command == UiNavigationHelpers::Command::Right)
        {
            dist = scrollOffsetToChild.GetX() > epsilon ? scrollOffsetToChild.GetX() : 0.0f;
        }

        if (dist > 0.0f)
        {
            if (dist < shortestDist)
            {
                shortestDist = dist;
                shortestPerpendicularDist = fabs((command == UiNavigationHelpers::Command::Up || command == UiNavigationHelpers::Command::Down) ? scrollOffsetToChild.GetX() : scrollOffsetToChild.GetY());
                closestChild = child->GetId();
            }
            else if (dist == shortestDist)
            {
                float perpDist = fabs((command == UiNavigationHelpers::Command::Up || command == UiNavigationHelpers::Command::Down) ? scrollOffsetToChild.GetX() : scrollOffsetToChild.GetY());
                if (perpDist < shortestPerpendicularDist)
                {
                    shortestPerpendicularDist = perpDist;
                    closestChild = child->GetId();
                }
            }
        }
    }

    return closestChild;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiScrollBoxComponent::GetScrollableParentToContentRatio(AZ::Vector2& ratioOut)
{
    AZ::Entity* contentParentEntity = nullptr;
    UiElementBus::EventResult(contentParentEntity, m_contentEntity, &UiElementBus::Events::GetParent);
    if (contentParentEntity)
    {
        AZ::Vector2 parentSize;
        UiTransformBus::EventResult(parentSize, contentParentEntity->GetId(), &UiTransformBus::Events::GetCanvasSpaceSizeNoScaleRotate);

        UiTransformInterface::Rect contentRect = GetAxisAlignedContentRect();
        AZ::Vector2 contentSize = contentRect.GetSize();

        ratioOut.SetX(contentSize.GetX() != 0.0f ? parentSize.GetX() / contentSize.GetX() : 1.0f);
        ratioOut.SetY(contentSize.GetY() != 0.0f ? parentSize.GetY() / contentSize.GetY() : 1.0f);

        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBoxComponent::OnValueChangingByScroller(float value)
{
    AZ::EntityId scroller = *UiScrollerToScrollableNotificationBus::GetCurrentBusId();

    AZ::Vector2 newScrollOffsetOut;
    bool result = ScrollerValueToScrollOffsets(scroller, value, newScrollOffsetOut);

    if (result && m_scrollOffset != newScrollOffsetOut)
    {
        DoSetScrollOffset(newScrollOffsetOut);
        DoChangingActions();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBoxComponent::OnValueChangedByScroller(float value)
{
    AZ::EntityId scroller = *UiScrollerToScrollableNotificationBus::GetCurrentBusId();

    AZ::Vector2 newScrollOffsetOut;
    bool result = ScrollerValueToScrollOffsets(scroller, value, newScrollOffsetOut);

    if (result)
    {
        AZ::Vector2 prevScrollOffset = m_scrollOffset;

        if (m_scrollOffset != newScrollOffsetOut)
        {
            DoSetScrollOffset(newScrollOffsetOut);
        }

        if (DoSnap())
        {
            // Snapping/constraining caused the scroll offsets to change, so notify scrollers
            NotifyScrollersOnValueChanged();
        }

        if (m_scrollOffset != prevScrollOffset)
        {
            DoChangedActions();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBoxComponent::InGamePostActivate()
{
    if (m_hScrollBarEntity.IsValid() && m_isHorizontalScrollingEnabled)
    {
        // Set this entity as the scrollable entity of the scroller
        UiScrollerBus::Event(m_hScrollBarEntity, &UiScrollerBus::Events::SetScrollableEntity, GetEntityId());

        UiScrollerToScrollableNotificationBus::MultiHandler::BusConnect(m_hScrollBarEntity);
    }

    if (m_vScrollBarEntity.IsValid() && m_isVerticalScrollingEnabled)
    {
        // Set this entity as the scrollable entity of the scroller
        UiScrollerBus::Event(m_vScrollBarEntity, &UiScrollerBus::Events::SetScrollableEntity, GetEntityId());

        UiScrollerToScrollableNotificationBus::MultiHandler::BusConnect(m_vScrollBarEntity);
    }

    DoSetScrollOffset(m_scrollOffset);

    // Setup based on the size of the content and its parent
    ContentOrParentSizeChanged();

    // Listen for canvas space rect changes from the content entity
    if (m_contentEntity.IsValid())
    {
        UiTransformChangeNotificationBus::MultiHandler::BusConnect(m_contentEntity);

        // Listen for canvas space rect changes from the content entity's parent
        AZ::Entity* contentParentEntity = nullptr;
        UiElementBus::EventResult(contentParentEntity, m_contentEntity, &UiElementBus::Events::GetParent);
        if (contentParentEntity)
        {
            UiTransformChangeNotificationBus::MultiHandler::BusConnect(contentParentEntity->GetId());
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiScrollBoxComponent::HandlePressed(AZ::Vector2 point, bool& shouldStayActive)
{
    bool handled = UiInteractableComponent::HandlePressed(point, shouldStayActive);

    if (handled)
    {
        // clear the dragging flag, we are not dragging until we detect a drag
        m_isDragging = false;

        // record the scroll offset at the time of the press
        m_pressedScrollOffset = m_scrollOffset;
    }

    // Stop momentum if the user pressed the screen, when handled directly
    SetMomentumActive(false);
        
    return handled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiScrollBoxComponent::HandleReleased([[maybe_unused]] AZ::Vector2 point)
{
    if (m_isHandlingEvents)
    {
        // handle snapping
        DoSnap();

        UiInteractableComponent::TriggerReleasedAction();

        NotifyScrollersOnValueChanged();

        // NOTE: when we have inertia/rubber-banding these actions should occur when snap is finished
        DoChangedActions();
    }

    m_isPressed = false;
    m_isDragging = false;

    //Start momentum if released the screen
    SetMomentumActive(true);

    return m_isHandlingEvents;
}

/////////////////////////////////////////////////////////////////
bool UiScrollBoxComponent::HandleEnterPressed(bool& shouldStayActive)
{
    bool handled = UiInteractableComponent::HandleEnterPressed(shouldStayActive);

    if (handled)
    {
        // the scrollbox will stay active after released
        shouldStayActive = true;
        m_isActive = true;
    }

    return handled;
}

/////////////////////////////////////////////////////////////////
bool UiScrollBoxComponent::HandleAutoActivation()
{
    if (!m_isHandlingEvents)
    {
        return false;
    }

    m_isActive = true;
    return true;
}

/////////////////////////////////////////////////////////////////
bool UiScrollBoxComponent::HandleKeyInputBegan(const AzFramework::InputChannel::Snapshot& inputSnapshot, AzFramework::ModifierKeyMask activeModifierKeys)
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
    if (m_isHorizontalScrollingEnabled && ((command == UiNavigationHelpers::Command::Left || command == UiNavigationHelpers::Command::Right))
        || (m_isVerticalScrollingEnabled && (command == UiNavigationHelpers::Command::Up || command == UiNavigationHelpers::Command::Down)))
    {
        AZ::Vector2 newScrollOffset = m_scrollOffset;
        if (m_snapMode == UiScrollBoxInterface::SnapMode::Children)
        {
            AZ::EntityId closestChild = FindNextContentChildElement(command);
            if (closestChild.IsValid())
            {
                // want elastic animation eventually
                AZ::Vector2 deltaToSubtract = ComputeCurrentOffsetToChild(closestChild);

                // snapping should only move the content in the directions it is allowed to scroll
                if (!m_isHorizontalScrollingEnabled)
                {
                    deltaToSubtract.SetX(0.0f);
                }
                else if (!m_isVerticalScrollingEnabled)
                {
                    deltaToSubtract.SetY(0.0f);
                }

                newScrollOffset -= deltaToSubtract;

                // do constraining
                if (m_isScrollingConstrained)
                {
                    AZ::Entity* contentParentEntity = nullptr;
                    UiElementBus::EventResult(contentParentEntity, m_contentEntity, &UiElementBus::Events::GetParent);

                    newScrollOffset = ConstrainOffset(newScrollOffset, contentParentEntity);
                }
            }
        }
        else if (m_snapMode == UiScrollBoxInterface::SnapMode::Grid)
        {
            if (command == UiNavigationHelpers::Command::Up)
            {
                newScrollOffset.SetY(newScrollOffset.GetY() + m_snapGrid.GetY());
            }
            else if (command == UiNavigationHelpers::Command::Down)
            {
                newScrollOffset.SetY(newScrollOffset.GetY() - m_snapGrid.GetY());
            }
            else if (command == UiNavigationHelpers::Command::Left)
            {
                newScrollOffset.SetX(newScrollOffset.GetX() + m_snapGrid.GetX());
            }
            else if (command == UiNavigationHelpers::Command::Right)
            {
                newScrollOffset.SetX(newScrollOffset.GetX() - m_snapGrid.GetX());
            }

            if (m_isScrollingConstrained)
            {
                AZ::Entity* contentParentEntity = nullptr;
                UiElementBus::EventResult(contentParentEntity, m_contentEntity, &UiElementBus::Events::GetParent);

                // Only scroll if constraining doesn't change the offset
                AZ::Vector2 constrainedScrollOffset = ConstrainOffset(newScrollOffset, contentParentEntity);
                if (constrainedScrollOffset != newScrollOffset)
                {
                    newScrollOffset = m_scrollOffset;
                }
            }
        }
        else
        {
            // get content parent's rect in canvas space
            AZ::Entity* contentParentEntity = nullptr;
            UiElementBus::EventResult(contentParentEntity, m_contentEntity, &UiElementBus::Events::GetParent);

            if (contentParentEntity)
            {
                UiTransformInterface::Rect parentRect;
                UiTransformBus::Event(contentParentEntity->GetId(), &UiTransformBus::Events::GetCanvasSpaceRectNoScaleRotate, parentRect);

                const float keySteps = 10.0f;

                if (command == UiNavigationHelpers::Command::Left || command == UiNavigationHelpers::Command::Right)
                {
                    float xStep = parentRect.GetSize().GetX() / keySteps;
                    newScrollOffset.SetX(newScrollOffset.GetX() + (command == UiNavigationHelpers::Command::Left ? xStep : -xStep));
                }
                else
                {
                    float yStep = parentRect.GetSize().GetY() / keySteps;
                    newScrollOffset.SetY(newScrollOffset.GetY() + (command == UiNavigationHelpers::Command::Up ? yStep : -yStep));
                }

                // do constraining
                if (m_isScrollingConstrained)
                {
                    newScrollOffset = ConstrainOffset(newScrollOffset, contentParentEntity);
                }
            }
        }

        if (newScrollOffset != m_scrollOffset)
        {
            DoSetScrollOffset(newScrollOffset);

            NotifyScrollersOnValueChanged();

            DoChangingActions();

            DoChangedActions();
        }

        result = true;
    }

    return result;
}

/////////////////////////////////////////////////////////////////
void UiScrollBoxComponent::InputPositionUpdate(AZ::Vector2 point)
{
    if (m_isPressed && m_contentEntity.IsValid())
    {
        m_lastOffsetChange = AZ::Vector2(0.0f, 0.0f);
        if (!m_isDragging)
        {
            CheckForDragOrHandOffToParent(point);
        }

        if (m_isDragging)
        {
            AZ::Vector2 dragVector = point - m_pressedPoint;
            dragVector *= m_scrollSensitivity;
            
            AZ::Entity* contentParentEntity = nullptr;
            UiElementBus::EventResult(contentParentEntity, m_contentEntity, &UiElementBus::Events::GetParent);

            AZ::Matrix4x4 transform;
            if (contentParentEntity)
            {
                UiTransformBus::Event(contentParentEntity->GetId(), &UiTransformBus::Events::GetTransformFromViewport, transform);
            }
            else
            {
                transform = AZ::Matrix4x4::CreateIdentity();
            }

            // Transform the draw vector from viewport space to the local space of the parent of the content element
            // This means we can do all calculations in unrotated/unscaled space.
            AZ::Vector3 dragVector3(dragVector.GetX(), dragVector.GetY(), 0.0f);
            dragVector3 = transform.Multiply3x3(dragVector3);
            AZ::Vector2 dragVectorInParentSpace(dragVector3.GetX(), dragVector3.GetY());

            if (!m_isHorizontalScrollingEnabled)
            {
                dragVectorInParentSpace.SetX(0.0f);
            }

            if (!m_isVerticalScrollingEnabled)
            {
                dragVectorInParentSpace.SetY(0.0f);
            }

            AZ::Vector2 newScrollOffset = m_pressedScrollOffset + dragVectorInParentSpace;

            // do constraining
            if (m_isScrollingConstrained)
            {
                newScrollOffset = ConstrainOffset(newScrollOffset, contentParentEntity);
            }

            m_lastDragPoint = point;

            if (newScrollOffset != m_scrollOffset)
            {
                m_lastOffsetChange = newScrollOffset - m_scrollOffset;
                m_offsetChangeAccumulator += m_lastOffsetChange;
                DoSetScrollOffset(newScrollOffset);

                NotifyScrollersOnValueChanging();

                DoChangingActions();
            }
            
            //Reset offset and time accumulators if change scrolling direction
            if (m_lastOffsetChange.Dot(m_offsetChangeAccumulator) < 0.0f)
            {
                SetMomentumActive(false);
            }
        }
    }
}

/////////////////////////////////////////////////////////////////
bool UiScrollBoxComponent::DoesSupportDragHandOff(AZ::Vector2 startPoint)
{
    // this component does support hand-off, so long as the start point is in its bounds
    bool isPointInRect = false;
    UiTransformBus::EventResult(isPointInRect, GetEntityId(), &UiTransformBus::Events::IsPointInRect, startPoint);
    return isPointInRect;
}

/////////////////////////////////////////////////////////////////
bool UiScrollBoxComponent::OfferDragHandOff(AZ::EntityId currentActiveInteractable, AZ::Vector2 startPoint, AZ::Vector2 currentPoint, float dragThreshold)
{
    bool result = false;

    // This only gets called if this is not already the active interactable, check preconditions
    AZ_Assert(!m_isPressed && !m_isDragging, "ScrollBox is already active");

    // get transform of content entity
    AZ::Entity* contentParentEntity = nullptr;
    UiElementBus::EventResult(contentParentEntity, m_contentEntity, &UiElementBus::Events::GetParent);

    AZ::Matrix4x4 transform;
    if (contentParentEntity)
    {
        UiTransformBus::Event(contentParentEntity->GetId(), &UiTransformBus::Events::GetTransformFromViewport, transform);
    }
    else
    {
        transform = AZ::Matrix4x4::CreateIdentity();
    }

    float validDragDistance = GetValidDragDistanceInPixels(startPoint, currentPoint);
    if (validDragDistance > dragThreshold)
    {
        // share this common code?
        m_isDragging = true;
        m_isPressed = true;
        m_pressedPoint = startPoint;
        m_pressedScrollOffset = m_scrollOffset;
        m_lastDragPoint = m_pressedPoint;
        
        // Stop momentum if the user pressed the screen, when handled indirectly
        SetMomentumActive(false);

        // tell the canvas that this is now the active interacatable
        UiInteractableActiveNotificationBus::Event(
            currentActiveInteractable, &UiInteractableActiveNotificationBus::Events::ActiveChanged, GetEntityId(), false);
        result = true;
    }
    else
    {
        // The current drag movement is not over the threshhold to be dragging this interactable

        // look for a parent interactable that the start point of the drag is inside
        AZ::EntityId interactableContainer;
        UiElementBus::EventResult(
            interactableContainer, GetEntityId(), &UiElementBus::Events::FindParentInteractableSupportingDrag, startPoint);

        // if there was a parent interactable offer them the opportunity to become the active interactable
        UiInteractableBus::EventResult(
            result,
            interactableContainer,
            &UiInteractableBus::Events::OfferDragHandOff,
            currentActiveInteractable,
            startPoint,
            currentPoint,
            dragThreshold);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBoxComponent::LostActiveStatus()
{
    UiInteractableComponent::LostActiveStatus();

    if (m_isDragging)
    {
        if (m_isHandlingEvents)
        {
            // handle snapping
            DoSnap();

            NotifyScrollersOnValueChanged();

            // NOTE: when we have inertia/rubber-banding these actions should occur when snap is finished
            DoChangedActions();
        }

        m_isDragging = false;
    }

    m_isActive = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBoxComponent::HandleDescendantReceivedHoverByNavigation(AZ::EntityId descendantEntityId)
{
    // Check if the content element is an ancestor of the descendant element
    bool isAncestor = false;
    if (m_contentEntity.IsValid())
    {
        UiElementBus::EventResult(isAncestor, descendantEntityId, &UiElementBus::Events::IsAncestor, m_contentEntity);
    }

    if (isAncestor)
    {
        AZ::Vector2 newScrollOffset = m_scrollOffset;

        if (m_snapMode == UiScrollBoxInterface::SnapMode::Children)
        {
            // Find the descendant's ancestor that's a direct child of the content entity
            AZ::EntityId parent;
            UiElementBus::EventResult(parent, descendantEntityId, &UiElementBus::Events::GetParentEntityId);
            while (parent.IsValid())
            {
                if (parent == m_contentEntity)
                {
                    break;
                }

                descendantEntityId = parent;
                parent.SetInvalid();
                UiElementBus::EventResult(parent, descendantEntityId, &UiElementBus::Events::GetParentEntityId);
            }

            if (descendantEntityId.IsValid())
            {
                AZ::Vector2 offset = ComputeCurrentOffsetToChild(descendantEntityId);

                if (!m_isHorizontalScrollingEnabled)
                {
                    offset.SetX(0.0f);
                }
                if (!m_isVerticalScrollingEnabled)
                {
                    offset.SetY(0.0f);
                }

                newScrollOffset = m_scrollOffset - offset;
            }
        }
        else
        {
            // Check if the descendant element is visible in the viewport area
            AZ::EntityId contentParent;
            UiElementBus::EventResult(contentParent, m_contentEntity, &UiElementBus::Events::GetParentEntityId);
            if (contentParent.IsValid())
            {
                UiTransformInterface::Rect contentParentRect;
                AZ::Matrix4x4 transformFromViewport;
                UiTransformBus::Event(contentParent, &UiTransformBus::Events::GetCanvasSpaceRectNoScaleRotate, contentParentRect);
                UiTransformBus::Event(contentParent, &UiTransformBus::Events::GetTransformFromViewport, transformFromViewport);

                UiTransformInterface::RectPoints descendantPoints;
                UiTransformBus::Event(descendantEntityId, &UiTransformBus::Events::GetViewportSpacePoints, descendantPoints);
                descendantPoints = descendantPoints.Transform(transformFromViewport);

                UiTransformInterface::Rect descendantRect;
                descendantRect.left = descendantPoints.GetAxisAlignedTopLeft().GetX();
                descendantRect.right = descendantPoints.GetAxisAlignedBottomRight().GetX();
                descendantRect.top = descendantPoints.GetAxisAlignedTopLeft().GetY();
                descendantRect.bottom = descendantPoints.GetAxisAlignedBottomRight().GetY();

                bool descendantInsideH = (descendantRect.left >= contentParentRect.left &&
                                          descendantRect.right <= contentParentRect.right);
                bool descendantInsideV = (descendantRect.top >= contentParentRect.top &&
                                          descendantRect.bottom <= contentParentRect.bottom);

                if (!descendantInsideH || !descendantInsideV)
                {
                    AZ::Vector2 offset(0.0f, 0.0f);

                    // Scroll to make the descendant visible in the viewport area
                    if (!descendantInsideH && m_isHorizontalScrollingEnabled)
                    {
                        float leftOffset = descendantRect.left - contentParentRect.left;
                        float rightOffset = descendantRect.right - contentParentRect.right;
                        bool shouldOffsetFromLeft = fabs(leftOffset) < fabs(rightOffset);
                        offset.SetX(shouldOffsetFromLeft ? leftOffset : rightOffset);
                    }
                    if (!descendantInsideV && m_isVerticalScrollingEnabled)
                    {
                        float topOffset = descendantRect.top - contentParentRect.top;
                        float bottomOffset = descendantRect.bottom - contentParentRect.bottom;
                        bool shouldOffsetFromTop = fabs(topOffset) < fabs(bottomOffset);
                        offset.SetY(shouldOffsetFromTop ? topOffset : bottomOffset);
                    }

                    newScrollOffset = m_scrollOffset - offset;

                    if (m_snapMode == UiScrollBoxInterface::SnapMode::Grid)
                    {
                        // Make sure new offset is on the grid
                        const float gridEpsilon = 0.00001f;

                        if (m_snapGrid.GetX() >= gridEpsilon && m_isHorizontalScrollingEnabled)
                        {
                            float gridSteps = newScrollOffset.GetX() / m_snapGrid.GetX();
                            float roundedGridSteps = (offset.GetX() < 0.0f) ? ceil(gridSteps) : floor(gridSteps);
                            newScrollOffset.SetX(roundedGridSteps * m_snapGrid.GetX());
                        }

                        if (m_snapGrid.GetY() >= gridEpsilon && m_isVerticalScrollingEnabled)
                        {
                            float gridSteps = newScrollOffset.GetY() / m_snapGrid.GetY();
                            float roundedGridSteps = (offset.GetY() < 0.0f) ? ceil(gridSteps) : floor(gridSteps);
                            newScrollOffset.SetY(roundedGridSteps * m_snapGrid.GetY());
                        }
                    }
                }
            }
        }

        if (newScrollOffset != m_scrollOffset)
        {
            SetScrollOffset(newScrollOffset);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBoxComponent::OnCanvasSpaceRectChanged([[maybe_unused]] AZ::EntityId entityId, const UiTransformInterface::Rect& oldRect, const UiTransformInterface::Rect& newRect)
{
    // If old rect equals new rect, size changed due to initialization
    bool sizeChanged = (oldRect == newRect) || (!oldRect.GetSize().IsClose(newRect.GetSize(), 0.05f));

    if (sizeChanged)
    {
        ContentOrParentSizeChanged();
    }
}

void UiScrollBoxComponent::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
{
    if (!m_momentumIsActive && m_isDragging)
    {
        m_draggingTimeAccumulator += deltaTime;
        // Detect if stopped by checking if immediate offset change falls below threshold
        if ((m_lastOffsetChange / m_scrollSensitivity).GetLength() < MIN_OFFSET_THRESHOLD)
        {
            m_stoppingTimeAccumulator += deltaTime;
        }
        else
        {
            m_stoppingTimeAccumulator = 0.0f;
        }
    }
    
    // Stop momentum if off or already ran the full momentum duration
    if (!m_momentumIsActive ||
        m_momentumDuration < m_momentumTimeAccumulator)
    {
        return;
    }
    
    // Stop momentum if no dragging accumulator, or not enough drag, or if stopped for long enough
    if (m_draggingTimeAccumulator == 0.0f ||
        (m_offsetChangeAccumulator / m_scrollSensitivity).GetLength() < MIN_OFFSET_THRESHOLD ||
        m_stoppingTimeAccumulator > MAX_STOPPING_DELAY)
    {
        return;
    }
    
    m_momentumTimeAccumulator += deltaTime;

    float momentumRatio = AZ::GetClamp(m_momentumTimeAccumulator / m_momentumDuration, 0.0f, 1.0f);
    float momentumEasing = 1.0f + (momentumRatio - 1.0f) * (momentumRatio - 1.0f) * (momentumRatio - 1.0f); // Ease Out Cubic decrease

    // m_offsetChangeAccumulator is the aggregated unidirectional scrolling, inverse easing for deceleration
    AZ::Vector2 momentumOffsetChange = m_offsetChangeAccumulator * ( deltaTime / m_draggingTimeAccumulator) * (1.0f - momentumEasing);
    AZ::Vector2 newScrollOffset = m_scrollOffset + momentumOffsetChange;

    // do constraining
    if (m_isScrollingConstrained)
    {
        AZ::Entity* contentParentEntity = nullptr;
        EBUS_EVENT_ID_RESULT(contentParentEntity, m_contentEntity, UiElementBus, GetParent);
        newScrollOffset = ConstrainOffset(newScrollOffset, contentParentEntity);
    }

    if (newScrollOffset != m_scrollOffset)
    {
        DoSetScrollOffset(newScrollOffset);

        NotifyScrollersOnValueChanging();

        DoChangingActions();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBoxComponent::Activate()
{
    UiInteractableComponent::Activate();
    UiScrollBoxBus::Handler::BusConnect(GetEntityId());
    UiScrollableBus::Handler::BusConnect(GetEntityId());
    UiInitializationBus::Handler::BusConnect(GetEntityId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBoxComponent::Deactivate()
{
    UiInteractableComponent::Deactivate();
    UiScrollBoxBus::Handler::BusDisconnect(GetEntityId());
    UiScrollableBus::Handler::BusDisconnect(GetEntityId());
    UiInitializationBus::Handler::BusDisconnect(GetEntityId());
    UiTransformChangeNotificationBus::MultiHandler::BusDisconnect();

    if (m_hScrollBarEntity.IsValid() && m_isHorizontalScrollingEnabled)
    {
        UiScrollerToScrollableNotificationBus::MultiHandler::BusDisconnect(m_hScrollBarEntity);
    }

    if (m_vScrollBarEntity.IsValid() && m_isVerticalScrollingEnabled)
    {
        UiScrollerToScrollableNotificationBus::MultiHandler::BusDisconnect(m_vScrollBarEntity);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiScrollBoxComponent::IsAutoActivationSupported()
{
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiInteractableStatesInterface::State UiScrollBoxComponent::ComputeInteractableState()
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

void UiScrollBoxComponent::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

    if (serializeContext)
    {
        serializeContext->Class<UiScrollBoxComponent, UiInteractableComponent>()
            ->Version(4, &VersionConverter)
        // Content group
            ->Field("ContentEntity", &UiScrollBoxComponent::m_contentEntity)
            ->Field("ScrollOffset", &UiScrollBoxComponent::m_scrollOffset)
            ->Field("ConstrainScrolling", &UiScrollBoxComponent::m_isScrollingConstrained)
            ->Field("SnapMode", &UiScrollBoxComponent::m_snapMode)
            ->Field("SnapGrid", &UiScrollBoxComponent::m_snapGrid)
        // Horizontal scrolling group
            ->Field("AllowHorizSrolling", &UiScrollBoxComponent::m_isHorizontalScrollingEnabled)
            ->Field("HScrollBarEntity", &UiScrollBoxComponent::m_hScrollBarEntity)
            ->Field("HScrollBarVisibility", &UiScrollBoxComponent::m_hScrollBarVisibility)
        // Vertical scrolling group
            ->Field("AllowVertScrolling", &UiScrollBoxComponent::m_isVerticalScrollingEnabled)
            ->Field("VScrollBarEntity", &UiScrollBoxComponent::m_vScrollBarEntity)
            ->Field("VScrollBarVisibility", &UiScrollBoxComponent::m_vScrollBarVisibility)
        // Actions group
            ->Field("ScrollOffsetChangingActionName", &UiScrollBoxComponent::m_scrollOffsetChangingActionName)
            ->Field("ScrollOffsetChangedActionName", &UiScrollBoxComponent::m_scrollOffsetChangedActionName);

        AZ::EditContext* ec = serializeContext->GetEditContext();
        if (ec)
        {
            auto editInfo = ec->Class<UiScrollBoxComponent>("ScrollBox", "An interactable component for scrolling a child element.");

            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Category, "UI")
                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/UiScrollBox.png")
                ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/UiScrollBox.png")
                ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("UI"))
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            // Content group
            {
                editInfo->ClassElement(AZ::Edit::ClassElements::Group, "Content")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiScrollBoxComponent::m_contentEntity, "Content element", "The child element that is the scrollable content.")
                    ->Attribute(AZ::Edit::Attributes::EnumValues, &UiScrollBoxComponent::PopulateChildEntityList);
                editInfo->DataElement(0, &UiScrollBoxComponent::m_scrollOffset, "Initial scroll offset", "The initial offset of the scroll box content.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show); // needed because sub-elements are hidden
                editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiScrollBoxComponent::m_isScrollingConstrained, "Constrain scrolling",
                    "Check this box to prevent the content from being scrolled beyond its edges.");
                editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiScrollBoxComponent::m_snapMode, "Snap",
                    "Sets the snapping behavior when the control is released.")
                    ->EnumAttribute(UiScrollBoxInterface::SnapMode::None, "None")
                    ->EnumAttribute(UiScrollBoxInterface::SnapMode::Children, "To children")
                    ->EnumAttribute(UiScrollBoxInterface::SnapMode::Grid, "To grid")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC_CE("RefreshEntireTree"));
                editInfo->DataElement(0, &UiScrollBoxComponent::m_snapGrid, "Grid spacing",
                    "The scroll offset will be snapped to multiples of these values.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &UiScrollBoxComponent::IsSnapToGrid);
            }

            // Horizontal scrolling group
            {
                editInfo->ClassElement(AZ::Edit::ClassElements::Group, "Horizontal scrolling")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiScrollBoxComponent::m_isHorizontalScrollingEnabled, "Enabled",
                    "Check this box to allow the scroll box to be scrolled horizontally.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC_CE("RefreshEntireTree"));

                editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiScrollBoxComponent::m_hScrollBarEntity, "Scrollbar element",
                    "The element that is the horizontal scrollbar.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &UiScrollBoxComponent::m_isHorizontalScrollingEnabled)
                    ->Attribute(AZ::Edit::Attributes::EnumValues, &UiScrollBoxComponent::PopulateHScrollBarEntityList);

                editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiScrollBoxComponent::m_hScrollBarVisibility, "Scrollbar visibility",
                    "Sets visibility behavior of the horizontal scrollbar.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &UiScrollBoxComponent::m_isHorizontalScrollingEnabled)
                    ->EnumAttribute(UiScrollBoxInterface::ScrollBarVisibility::AlwaysShow, "Always visible")
                    ->EnumAttribute(UiScrollBoxInterface::ScrollBarVisibility::AutoHide, "Auto hide")
                    ->EnumAttribute(UiScrollBoxInterface::ScrollBarVisibility::AutoHideAndResizeViewport, "Auto hide and resize view area");
            }

            // Vertical scrolling group
            {
                editInfo->ClassElement(AZ::Edit::ClassElements::Group, "Vertical scrolling")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiScrollBoxComponent::m_isVerticalScrollingEnabled, "Enabled",
                    "Check this box to allow the scroll box to be scrolled vertically.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC_CE("RefreshEntireTree"));

                editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiScrollBoxComponent::m_vScrollBarEntity, "Scrollbar element",
                    "The element that is the vertical scrollbar.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &UiScrollBoxComponent::m_isVerticalScrollingEnabled)
                    ->Attribute(AZ::Edit::Attributes::EnumValues, &UiScrollBoxComponent::PopulateVScrollBarEntityList);

                editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiScrollBoxComponent::m_vScrollBarVisibility, "Scrollbar visibility",
                    "Sets visibility behavior of the vertical scrollbar.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &UiScrollBoxComponent::m_isVerticalScrollingEnabled)
                    ->EnumAttribute(UiScrollBoxInterface::ScrollBarVisibility::AlwaysShow, "Always visible")
                    ->EnumAttribute(UiScrollBoxInterface::ScrollBarVisibility::AutoHide, "Auto hide")
                    ->EnumAttribute(UiScrollBoxInterface::ScrollBarVisibility::AutoHideAndResizeViewport, "Auto hide and resize view area");
            }

            // Actions group
            {
                editInfo->ClassElement(AZ::Edit::ClassElements::Group, "Actions")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                editInfo->DataElement(0, &UiScrollBoxComponent::m_scrollOffsetChangingActionName, "Change", "The action triggered while the offset is changing.");
                editInfo->DataElement(0, &UiScrollBoxComponent::m_scrollOffsetChangedActionName, "End change", "The action triggered when the offset is done changing.");
            }
        }
    }

    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
    if (behaviorContext)
    {
        behaviorContext->EBus<UiScrollBoxBus>("UiScrollBoxBus")
            ->Event("GetScrollOffset", &UiScrollBoxBus::Events::GetScrollOffset)
            ->Event("SetScrollOffset", &UiScrollBoxBus::Events::SetScrollOffset)
            ->Event("GetNormalizedScrollValue", &UiScrollBoxBus::Events::GetNormalizedScrollValue)
            ->Event("HasHorizontalContentToScroll", &UiScrollBoxBus::Events::HasHorizontalContentToScroll)
            ->Event("HasVerticalContentToScroll", &UiScrollBoxBus::Events::HasVerticalContentToScroll)
            ->Event("GetIsHorizontalScrollingEnabled", &UiScrollBoxBus::Events::GetIsHorizontalScrollingEnabled)
            ->Event("SetIsHorizontalScrollingEnabled", &UiScrollBoxBus::Events::SetIsHorizontalScrollingEnabled)
            ->Event("GetIsVerticalScrollingEnabled", &UiScrollBoxBus::Events::GetIsVerticalScrollingEnabled)
            ->Event("SetIsVerticalScrollingEnabled", &UiScrollBoxBus::Events::SetIsVerticalScrollingEnabled)
            ->Event("GetIsScrollingConstrained", &UiScrollBoxBus::Events::GetIsScrollingConstrained)
            ->Event("SetIsScrollingConstrained", &UiScrollBoxBus::Events::SetIsScrollingConstrained)
            ->Event("GetSnapMode", &UiScrollBoxBus::Events::GetSnapMode)
            ->Event("SetSnapMode", &UiScrollBoxBus::Events::SetSnapMode)
            ->Event("GetSnapGrid", &UiScrollBoxBus::Events::GetSnapGrid)
            ->Event("SetSnapGrid", &UiScrollBoxBus::Events::SetSnapGrid)
            ->Event("GetHorizontalScrollBarVisibility", &UiScrollBoxBus::Events::GetHorizontalScrollBarVisibility)
            ->Event("SetHorizontalScrollBarVisibility", &UiScrollBoxBus::Events::SetHorizontalScrollBarVisibility)
            ->Event("GetVerticalScrollBarVisibility", &UiScrollBoxBus::Events::GetVerticalScrollBarVisibility)
            ->Event("SetVerticalScrollBarVisibility", &UiScrollBoxBus::Events::SetVerticalScrollBarVisibility)
            ->Event("GetScrollSensitivity", &UiScrollBoxBus::Events::GetScrollSensitivity)
            ->Event("SetScrollSensitivity", &UiScrollBoxBus::Events::SetScrollSensitivity)
            ->Event("GetMomentumDuration", &UiScrollBoxBus::Events::GetMomentumDuration)
            ->Event("SetMomentumDuration", &UiScrollBoxBus::Events::SetMomentumDuration)
            ->Event("SetMomentumActive", &UiScrollBoxBus::Events::SetMomentumActive)
            ->Event("StopMomentum", &UiScrollBoxBus::Events::StopMomentum)
            ->Event("GetScrollOffsetChangingActionName", &UiScrollBoxBus::Events::GetScrollOffsetChangingActionName)
            ->Event("SetScrollOffsetChangingActionName", &UiScrollBoxBus::Events::SetScrollOffsetChangingActionName)
            ->Event("GetScrollOffsetChangedActionName", &UiScrollBoxBus::Events::GetScrollOffsetChangedActionName)
            ->Event("SetScrollOffsetChangedActionName", &UiScrollBoxBus::Events::SetScrollOffsetChangedActionName)
            ->Event("GetContentEntity", &UiScrollBoxBus::Events::GetContentEntity)
            ->Event("SetContentEntity", &UiScrollBoxBus::Events::SetContentEntity)
            ->Event("GetHorizontalScrollBarEntity", &UiScrollBoxBus::Events::GetHorizontalScrollBarEntity)
            ->Event("SetHorizontalScrollBarEntity", &UiScrollBoxBus::Events::SetHorizontalScrollBarEntity)
            ->Event("GetVerticalScrollBarEntity", &UiScrollBoxBus::Events::GetVerticalScrollBarEntity)
            ->Event("SetVerticalScrollBarEntity", &UiScrollBoxBus::Events::SetVerticalScrollBarEntity)
            ->Event("FindClosestContentChildElement", &UiScrollBoxBus::Events::FindClosestContentChildElement);

        behaviorContext->Enum<(int)UiScrollBoxInterface::SnapMode::None>("eUiScrollBoxSnapMode_None")
            ->Enum<(int)UiScrollBoxInterface::SnapMode::Children>("eUiScrollBoxSnapMode_Children")
            ->Enum<(int)UiScrollBoxInterface::SnapMode::Grid>("eUiScrollBoxSnapMode_Grid")
            ->Enum<(int)UiScrollBoxInterface::ScrollBarVisibility::AlwaysShow>("eUiScrollBoxScrollBarVisibility_AlwaysShow")
            ->Enum<(int)UiScrollBoxInterface::ScrollBarVisibility::AutoHide>("eUiScrollBoxScrollBarVisibility_AutoHide")
            ->Enum<(int)UiScrollBoxInterface::ScrollBarVisibility::AutoHideAndResizeViewport>("eUiScrollBoxScrollBarVisibility_AutoHideAndResizeViewport");

        behaviorContext->EBus<UiScrollBoxNotificationBus>("UiScrollBoxNotificationBus")
            ->Handler<BehaviorUiScrollBoxNotificationBusHandler>();

        behaviorContext->EBus<UiScrollableNotificationBus>("UiScrollableNotificationBus")
            ->Handler<BehaviorUiScrollableNotificationBusHandler>();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiScrollBoxComponent::EntityComboBoxVec UiScrollBoxComponent::PopulateChildEntityList()
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
UiScrollBoxComponent::EntityComboBoxVec UiScrollBoxComponent::PopulateHScrollBarEntityList()
{
    return PopulateScrollBarEntityList(UiScrollerInterface::Orientation::Horizontal);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiScrollBoxComponent::EntityComboBoxVec UiScrollBoxComponent::PopulateVScrollBarEntityList()
{
    return PopulateScrollBarEntityList(UiScrollerInterface::Orientation::Vertical);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiScrollBoxComponent::EntityComboBoxVec UiScrollBoxComponent::PopulateScrollBarEntityList(UiScrollerInterface::Orientation orientation)
{
    EntityComboBoxVec result;

    // Add a first entry for "None"
    result.push_back(AZStd::make_pair(AZ::EntityId(), "<None>"));

    // Get a list of all scrollbar elements
    AZ::EntityId canvasEntityId;
    UiElementBus::EventResult(canvasEntityId, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);
    LyShine::EntityArray scrollBarElements;
    UiCanvasBus::Event(
        canvasEntityId,
        &UiCanvasBus::Events::FindElements,
        [this, orientation](const AZ::Entity* entity)
        {
            bool isScroller = false;
            if (entity->GetId() != GetEntityId())
            {
                if (UiScrollerBus::FindFirstHandler(entity->GetId()))
                {
                    // Check scrollbar's orientation
                    UiScrollerInterface::Orientation entityOrientation;
                    UiScrollerBus::EventResult(entityOrientation, entity->GetId(), &UiScrollerBus::Events::GetOrientation);
                    isScroller = (entityOrientation == orientation);
                }
            }
            return isScroller;
        },
        scrollBarElements);

    // Sort the elements by name
    AZStd::sort(scrollBarElements.begin(), scrollBarElements.end(),
        [](const AZ::Entity* e1, const AZ::Entity* e2) { return e1->GetName() < e2->GetName(); });

    // Add their names to the StringList and their IDs to the id list
    for (auto scrollBarEntity : scrollBarElements)
    {
        result.push_back(AZStd::make_pair(scrollBarEntity->GetId(), scrollBarEntity->GetName()));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiScrollBoxComponent::IsSnapToGrid() const
{
    return m_snapMode == SnapMode::Grid;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Vector2 UiScrollBoxComponent::ConstrainOffset(AZ::Vector2 proposedOffset, AZ::Entity* contentParentEntity)
{
    AZ::Vector2 newScrollOffset = proposedOffset;

    if (contentParentEntity)
    {
        // get content parent's rect in canvas space
        UiTransformInterface::Rect parentRect;
        UiTransformBus::Event(contentParentEntity->GetId(), &UiTransformBus::Events::GetCanvasSpaceRectNoScaleRotate, parentRect);

        // get content's rect in canvas space
        UiTransformInterface::Rect contentRect = GetAxisAlignedContentRect();

        AZ::Vector2 latestOffsetDelta = newScrollOffset - m_scrollOffset;

        // add the requested scroll offset to the content rect to get the proposed position
        // The content has already need moved by the requested offset all but latestOffsetDelta
        contentRect.MoveBy(latestOffsetDelta);

        if (contentRect.GetWidth() <= parentRect.GetWidth())
        {
            newScrollOffset.SetX(0.0f);
        }
        else if (contentRect.left > parentRect.left)
        {
            newScrollOffset.SetX(newScrollOffset.GetX() - (contentRect.left - parentRect.left));
        }
        else if (contentRect.right < parentRect.right)
        {
            newScrollOffset.SetX(newScrollOffset.GetX() + (parentRect.right - contentRect.right));
        }

        if (contentRect.GetHeight() <= parentRect.GetHeight())
        {
            newScrollOffset.SetY(0.0f);
        }
        else if (contentRect.top > parentRect.top)
        {
            newScrollOffset.SetY(newScrollOffset.GetY() - (contentRect.top - parentRect.top));
        }
        else if (contentRect.bottom < parentRect.bottom)
        {
            newScrollOffset.SetY(newScrollOffset.GetY() + (parentRect.bottom - contentRect.bottom));
        }
    }

    return newScrollOffset;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiScrollBoxComponent::DoSnap()
{
    AZ::Vector2 newScrollOffset = m_scrollOffset;
    AZ::Vector2 deltaToSubtract(0.0f, 0.0f);

    if (m_snapMode == UiScrollBoxInterface::SnapMode::Children)
    {
        AZ::EntityId closestChild = FindClosestContentChildElement();

        if (closestChild.IsValid())
        {
            // want elastic animation eventually
            deltaToSubtract = ComputeCurrentOffsetToChild(closestChild);
        }
    }
    else if (m_snapMode == UiScrollBoxInterface::SnapMode::Grid)
    {
        deltaToSubtract = ComputeCurrentOffsetFromGrid();
    }

    // snapping should only move the content in the directions it is allowed to scroll
    if (!m_isHorizontalScrollingEnabled)
    {
        deltaToSubtract.SetX(0.0f);
    }
    if (!m_isVerticalScrollingEnabled)
    {
        deltaToSubtract.SetY(0.0f);
    }

    newScrollOffset = m_scrollOffset - deltaToSubtract;

    if (m_isScrollingConstrained)
    {
        AZ::Entity* contentParentEntity = nullptr;
        UiElementBus::EventResult(contentParentEntity, m_contentEntity, &UiElementBus::Events::GetParent);
        newScrollOffset = ConstrainOffset(newScrollOffset, contentParentEntity);
    }

    if (newScrollOffset != m_scrollOffset)
    {
        DoSetScrollOffset(newScrollOffset);
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Vector2 UiScrollBoxComponent::ComputeCurrentOffsetToChild(AZ::EntityId child)
{
    // Get the position of the child element's pivot in canvas space
    AZ::Vector2 childPivotPosition;
    UiTransformBus::EventResult(childPivotPosition, child, &UiTransformBus::Events::GetCanvasSpacePivot);

    AZ::Vector2 anchorCenter = ComputeContentAnchorCenterInCanvasSpace();

    // offset is the distance from the content anchors to the current child pivot position
    // (given the current scroll offset)
    AZ::Vector2 offsetToChild = childPivotPosition - anchorCenter;

    AZ::Entity* contentParentEntity = nullptr;
    UiElementBus::EventResult(contentParentEntity, m_contentEntity, &UiElementBus::Events::GetParent);

    AZ::Matrix4x4 transform;
    if (contentParentEntity)
    {
        UiTransformBus::Event(contentParentEntity->GetId(), &UiTransformBus::Events::GetTransformFromCanvasSpace, transform);
    }
    else
    {
        transform = AZ::Matrix4x4::CreateIdentity();
    }

    // Transform the offset from canvas space to the local space of the parent of the content element
    AZ::Vector3 offsetToChild3(offsetToChild.GetX(), offsetToChild.GetY(), 0.0f);
    offsetToChild3 = transform.Multiply3x3(offsetToChild3);
    offsetToChild = AZ::Vector2(offsetToChild3.GetX(), offsetToChild3.GetY());

    return offsetToChild;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Vector2 UiScrollBoxComponent::ComputeCurrentOffsetFromGrid()
{
    // offset is the delta to add to subtract from m_scrollOffset to put it on the grid
    AZ::Vector2 offsetToGrid;
    offsetToGrid.SetX(ComputeOffsetOfValueFromGrid(m_scrollOffset.GetX(), m_snapGrid.GetX()));
    offsetToGrid.SetY(ComputeOffsetOfValueFromGrid(m_scrollOffset.GetY(), m_snapGrid.GetY()));
    return offsetToGrid;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Vector2 UiScrollBoxComponent::ComputeContentAnchorCenterInCanvasSpace() const
{
    // Get the position of the content elements anchors in canvas space
    AZ::Entity* contentParentEntity = nullptr;
    UiElementBus::EventResult(contentParentEntity, m_contentEntity, &UiElementBus::Events::GetParent);

    if (!contentParentEntity)
    {
        return AZ::Vector2(0.0f, 0.0f);
    }

    // get content parent's rect in canvas space
    UiTransformInterface::Rect parentRect;
    UiTransformBus::Event(contentParentEntity->GetId(), &UiTransformBus::Events::GetCanvasSpaceRectNoScaleRotate, parentRect);

    // Get the content anchor center in canvas space
    UiTransform2dInterface::Anchors anchors;
    UiTransform2dBus::EventResult(anchors, m_contentEntity, &UiTransform2dBus::Events::GetAnchors);

    UiTransformInterface::Rect anchorRect;
    anchorRect.left = parentRect.left + anchors.m_left * parentRect.GetWidth();
    anchorRect.right = parentRect.left + anchors.m_right * parentRect.GetWidth();
    anchorRect.top = parentRect.top + anchors.m_top * parentRect.GetHeight();
    anchorRect.bottom = parentRect.top + anchors.m_bottom * parentRect.GetHeight();

    AZ::Vector2 anchorCenter =  anchorRect.GetCenter();

    AZ::Matrix4x4 transformToCanvasSpace;
    UiTransformBus::Event(contentParentEntity->GetId(), &UiTransformBus::Events::GetTransformToCanvasSpace, transformToCanvasSpace);
    AZ::Vector3 anchorCenter3(anchorCenter.GetX(), anchorCenter.GetY(), 0.0f);
    anchorCenter3 = transformToCanvasSpace * anchorCenter3;
    anchorCenter = AZ::Vector2(anchorCenter3.GetX(), anchorCenter3.GetY());

    return anchorCenter;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiScrollBoxComponent::ComputeOffsetOfValueFromGrid(float value, float gridStep)
{
    const float gridEpsilon = 0.00001f;

    // compute offset to round to nearest point on grid
    float offsetFromGrid = 0.0f;
    if (gridStep >= gridEpsilon)
    {
        float roundedGridStep = roundf(value / gridStep);
        float targetValue = roundedGridStep * gridStep;
        offsetFromGrid = value - targetValue;
    }

    return offsetFromGrid;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// calculate how much we have dragged along the dragable axes of the ScrollBox
float UiScrollBoxComponent::GetValidDragDistanceInPixels(AZ::Vector2 startPoint, AZ::Vector2 endPoint)
{
    const float validDragRatio = 0.5f;

    AZ::Entity* contentParentEntity = nullptr;
    UiElementBus::EventResult(contentParentEntity, m_contentEntity, &UiElementBus::Events::GetParent);

    if (!contentParentEntity)
    {
        return 0.0f;
    }

    // convert the drag vector to local space
    AZ::Matrix4x4 transformFromViewport;
    UiTransformBus::Event(contentParentEntity->GetId(), &UiTransformBus::Events::GetTransformFromViewport, transformFromViewport);
    AZ::Vector2 dragVec = endPoint - startPoint;
    AZ::Vector3 dragVec3(dragVec.GetX(), dragVec.GetY(), 0.0f);
    AZ::Vector3 localDragVec = transformFromViewport.Multiply3x3(dragVec3);

    // constrain to the allowed movement directions
    if (!m_isHorizontalScrollingEnabled)
    {
        localDragVec.SetX(0.0f);
    }
    if (!m_isVerticalScrollingEnabled)
    {
        localDragVec.SetY(0.0f);
    }

    // convert back to viewport space
    AZ::Matrix4x4 transformToViewport;
    UiTransformBus::Event(contentParentEntity->GetId(), &UiTransformBus::Events::GetTransformToViewport, transformToViewport);
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
void UiScrollBoxComponent::CheckForDragOrHandOffToParent(AZ::Vector2 point)
{
    AZ::EntityId parentDraggable;
    UiElementBus::EventResult(parentDraggable, GetEntityId(), &UiElementBus::Events::FindParentInteractableSupportingDrag, point);

    // if this interactable is inside another interactable that supports drag then we use
    // a threshold value before starting a drag on this interactable
    const float normalDragThreshold = 0.0f;
    const float containedDragThreshold = 5.0f;

    float dragThreshold = normalDragThreshold;
    if (parentDraggable.IsValid())
    {
        dragThreshold = containedDragThreshold;
    }

    // calculate how much we have dragged in a valid direction
    float validDragDistance = GetValidDragDistanceInPixels(m_pressedPoint, point);
    if (validDragDistance > dragThreshold)
    {
        // we dragged above the threshold value along axis of slider
        m_isDragging = true;
    }
    else if (parentDraggable.IsValid())
    {
        // offer the parent draggable the chance to become the active interactable
        bool handOff = false;
        UiInteractableBus::EventResult(
            handOff,
            parentDraggable,
            &UiInteractableBus::Events::OfferDragHandOff,
            GetEntityId(),
            m_pressedPoint,
            point,
            containedDragThreshold);

        if (handOff)
        {
            // interaction has been handed off to a container entity
            m_isPressed = false;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBoxComponent::DoSetScrollOffset(AZ::Vector2 scrollOffset)
{
    m_scrollOffset = scrollOffset;

    if (m_contentEntity.IsValid())
    {
        // The scrollOffset is the distance from the content element's anchors to its pivot
        // Given the scrollOffset we adjust the offsets to make this so.
        UiTransform2dInterface::Offsets offsets;
        UiTransform2dBus::EventResult(offsets, m_contentEntity, &UiTransform2dBus::Events::GetOffsets);

        AZ::Vector2 pivot;
        UiTransformBus::EventResult(pivot, m_contentEntity, &UiTransformBus::Events::GetPivot);

        float width = offsets.m_right - offsets.m_left;
        float height = offsets.m_bottom - offsets.m_top;

        offsets.m_left = scrollOffset.GetX() - width * pivot.GetX();
        offsets.m_right = offsets.m_left + width;
        offsets.m_top = scrollOffset.GetY() - height * pivot.GetY();
        offsets.m_bottom = offsets.m_top + height;

        UiTransform2dBus::Event(m_contentEntity, &UiTransform2dBus::Events::SetOffsets, offsets);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBoxComponent::DoChangedActions()
{
    if (m_onScrollOffsetChanged)
    {
        m_onScrollOffsetChanged(GetEntityId(), m_scrollOffset);
    }

    // Tell any action listeners about the event
    if (!m_scrollOffsetChangedActionName.empty())
    {
        AZ::EntityId canvasEntityId;
        UiElementBus::EventResult(canvasEntityId, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);
        UiCanvasNotificationBus::Event(
            canvasEntityId, &UiCanvasNotificationBus::Events::OnAction, GetEntityId(), m_scrollOffsetChangedActionName);
    }

    NotifyListenersOnScrollOffsetChanged();

    NotifyListenersOnScrollValueChanged();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBoxComponent::DoChangingActions()
{
    if (m_onScrollOffsetChanging)
    {
        m_onScrollOffsetChanging(GetEntityId(), m_scrollOffset);
    }

    // Tell any action listeners about the event
    if (!m_scrollOffsetChangingActionName.empty())
    {
        AZ::EntityId canvasEntityId;
        UiElementBus::EventResult(canvasEntityId, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);
        UiCanvasNotificationBus::Event(
            canvasEntityId, &UiCanvasNotificationBus::Events::OnAction, GetEntityId(), m_scrollOffsetChangingActionName);
    }

    NotifyListenersOnScrollOffsetChanging();

    NotifyListenersOnScrollValueChanging();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBoxComponent::NotifyScrollersOnValueChanged()
{
    AZ::Vector2 normalizedScrollValueOut;
    bool result = ScrollOffsetToNormalizedScrollValue(m_scrollOffset, normalizedScrollValueOut);

    if (result)
    {
        UiScrollableToScrollerNotificationBus::Event(
            GetEntityId(), &UiScrollableToScrollerNotificationBus::Events::OnValueChangedByScrollable, normalizedScrollValueOut);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBoxComponent::NotifyScrollersOnValueChanging()
{
    AZ::Vector2 normalizedScrollValueOut;
    bool result = ScrollOffsetToNormalizedScrollValue(m_scrollOffset, normalizedScrollValueOut);

    if (result)
    {
        UiScrollableToScrollerNotificationBus::Event(
            GetEntityId(), &UiScrollableToScrollerNotificationBus::Events::OnValueChangingByScrollable, normalizedScrollValueOut);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBoxComponent::NotifyListenersOnScrollValueChanged()
{
    AZ::Vector2 normalizedScrollValueOut;
    bool result = ScrollOffsetToNormalizedScrollValue(m_scrollOffset, normalizedScrollValueOut);

    if (result)
    {
        UiScrollableNotificationBus::Event(
            GetEntityId(), &UiScrollableNotificationBus::Events::OnScrollableValueChanged, normalizedScrollValueOut);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBoxComponent::NotifyListenersOnScrollValueChanging()
{
    AZ::Vector2 normalizedScrollValueOut;
    bool result = ScrollOffsetToNormalizedScrollValue(m_scrollOffset, normalizedScrollValueOut);

    if (result)
    {
        UiScrollableNotificationBus::Event(
            GetEntityId(), &UiScrollableNotificationBus::Events::OnScrollableValueChanging, normalizedScrollValueOut);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBoxComponent::NotifyListenersOnScrollOffsetChanged()
{
    UiScrollBoxNotificationBus::Event(GetEntityId(), &UiScrollBoxNotificationBus::Events::OnScrollOffsetChanged, m_scrollOffset);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBoxComponent::NotifyListenersOnScrollOffsetChanging()
{
    UiScrollBoxNotificationBus::Event(GetEntityId(), &UiScrollBoxNotificationBus::Events::OnScrollOffsetChanging, m_scrollOffset);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiTransformInterface::Rect UiScrollBoxComponent::GetAxisAlignedContentRect()
{
    UiTransformInterface::RectPoints points;
    UiTransformBus::Event(m_contentEntity, &UiTransformBus::Events::GetCanvasSpacePointsNoScaleRotate, points);

    AZ::Matrix4x4 transform;
    UiTransformBus::Event(m_contentEntity, &UiTransformBus::Events::GetLocalTransform, transform);

    points = points.Transform(transform);

    UiTransformInterface::Rect rect;
    rect.left = points.GetAxisAlignedTopLeft().GetX();
    rect.right = points.GetAxisAlignedBottomRight().GetX();
    rect.top = points.GetAxisAlignedTopLeft().GetY();
    rect.bottom = points.GetAxisAlignedBottomRight().GetY();

    return rect;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiScrollBoxComponent::ScrollOffsetToNormalizedScrollValue(AZ::Vector2 scrollOffset, AZ::Vector2& normalizedScrollValueOut)
{
    AZ::Entity* contentParentEntity = nullptr;
    UiElementBus::EventResult(contentParentEntity, m_contentEntity, &UiElementBus::Events::GetParent);
    if (contentParentEntity)
    {
        UiTransformInterface::Rect parentRect;
        UiTransformBus::Event(contentParentEntity->GetId(), &UiTransformBus::Events::GetCanvasSpaceRectNoScaleRotate, parentRect);

        UiTransformInterface::Rect contentRect = GetAxisAlignedContentRect();

        if (contentRect.GetWidth() <= parentRect.GetWidth())
        {
            normalizedScrollValueOut.SetX(0.0f);
        }
        else
        {
            float minScrollOffset = scrollOffset.GetX() - (contentRect.left - parentRect.left);
            float maxScrollOffset = scrollOffset.GetX() - (contentRect.right - parentRect.right);
            normalizedScrollValueOut.SetX((scrollOffset.GetX() - minScrollOffset) / (maxScrollOffset - minScrollOffset));
        }

        if (contentRect.GetHeight() <= parentRect.GetHeight())
        {
            normalizedScrollValueOut.SetY(0.0f);
        }
        else
        {
            float minScrollOffset = scrollOffset.GetY() - (contentRect.top - parentRect.top);
            float maxScrollOffset = scrollOffset.GetY() - (contentRect.bottom - parentRect.bottom);
            normalizedScrollValueOut.SetY((scrollOffset.GetY() - minScrollOffset) / (maxScrollOffset - minScrollOffset));
        }

        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiScrollBoxComponent::NormalizedScrollValueToScrollOffset(UiScrollerInterface::Orientation orientation, float normalizedScrollValue, float& scrollOffsetOut)
{
    if (orientation == UiScrollerInterface::Orientation::Horizontal || orientation == UiScrollerInterface::Orientation::Vertical)
    {
        AZ::Entity* contentParentEntity = nullptr;
        UiElementBus::EventResult(contentParentEntity, m_contentEntity, &UiElementBus::Events::GetParent);
        if (contentParentEntity)
        {
            UiTransformInterface::Rect parentRect;
            UiTransformBus::Event(contentParentEntity->GetId(), &UiTransformBus::Events::GetCanvasSpaceRectNoScaleRotate, parentRect);

            UiTransformInterface::Rect contentRect = GetAxisAlignedContentRect();

            if (orientation == UiScrollerInterface::Orientation::Horizontal)
            {
                if (contentRect.GetWidth() <= parentRect.GetWidth())
                {
                    if (m_isScrollingConstrained)
                    {
                        scrollOffsetOut = 0.0f;
                    }
                    else
                    {
                        scrollOffsetOut = m_scrollOffset.GetX();
                    }
                }
                else
                {
                    float minScrollOffset = m_scrollOffset.GetX() - (contentRect.left - parentRect.left);
                    float maxScrollOffset = m_scrollOffset.GetX() - (contentRect.right - parentRect.right);
                    scrollOffsetOut = minScrollOffset + (maxScrollOffset - minScrollOffset) * normalizedScrollValue;
                }
            }
            else // orientation == UiScrollerInterface::Orientation::Vertical
            {
                if (contentRect.GetHeight() <= parentRect.GetHeight())
                {
                    if (m_isScrollingConstrained)
                    {
                        scrollOffsetOut = 0.0f;
                    }
                    else
                    {
                        scrollOffsetOut = m_scrollOffset.GetY();
                    }
                }
                else
                {
                    float minScrollOffset = m_scrollOffset.GetY() - (contentRect.top - parentRect.top);
                    float maxScrollOffset = m_scrollOffset.GetY() - (contentRect.bottom - parentRect.bottom);
                    scrollOffsetOut = minScrollOffset + (maxScrollOffset - minScrollOffset) * normalizedScrollValue;
                }
            }

            return true;
        }
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiScrollBoxComponent::ScrollerValueToScrollOffsets(AZ::EntityId scroller, float scrollerValue, AZ::Vector2& scrollOffsetsOut)
{
    if (((scroller == m_hScrollBarEntity) && m_isHorizontalScrollingEnabled)
        || ((scroller == m_vScrollBarEntity) && m_isVerticalScrollingEnabled))
    {
        float scrollOffsetOut;
        UiScrollerInterface::Orientation orientation = (scroller == m_hScrollBarEntity) ? UiScrollerInterface::Orientation::Horizontal : UiScrollerInterface::Orientation::Vertical;
        bool result = NormalizedScrollValueToScrollOffset(orientation, scrollerValue, scrollOffsetOut);

        if (result)
        {
            scrollOffsetsOut = m_scrollOffset;
            if (orientation == UiScrollerInterface::Orientation::Horizontal)
            {
                scrollOffsetsOut.SetX(scrollOffsetOut);
            }
            else // orientation == UiScrollerInterface::Orientation::Vertical
            {
                scrollOffsetsOut.SetY(scrollOffsetOut);
            }

            return true;
        }
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiScrollBoxComponent::IsVerticalScrollBarOnRight()
{
    // Check if vertical scrollbar is on the right of the content's parent

    AZ::Entity* contentParentEntity = nullptr;
    UiElementBus::EventResult(contentParentEntity, m_contentEntity, &UiElementBus::Events::GetParent);
    if (contentParentEntity)
    {
        // Get content parent rect in canvas space
        UiTransformInterface::Rect parentRect;
        UiTransformBus::Event(contentParentEntity->GetId(), &UiTransformBus::Events::GetCanvasSpaceRectNoScaleRotate, parentRect);

        // Get vertical scrollbar rect in canvas space
        UiTransformInterface::Rect vScrollBarRect;
        UiTransformBus::Event(m_vScrollBarEntity, &UiTransformBus::Events::GetCanvasSpaceRectNoScaleRotate, vScrollBarRect);

        return (vScrollBarRect.GetCenter().GetX() > parentRect.GetCenter().GetX());
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiScrollBoxComponent::IsHorizontalScrollBarOnBottom()
{
    // Check if horizontal scrollbar is on the bottom of the content's parent

    AZ::Entity* contentParentEntity = nullptr;
    UiElementBus::EventResult(contentParentEntity, m_contentEntity, &UiElementBus::Events::GetParent);
    if (contentParentEntity)
    {
        // Get content parent rect in canvas space
        UiTransformInterface::Rect parentRect;
        UiTransformBus::Event(contentParentEntity->GetId(), &UiTransformBus::Events::GetCanvasSpaceRectNoScaleRotate, parentRect);

        // Get horizontal scrollbar rect in canvas space
        UiTransformInterface::Rect hScrollBarRect;
        UiTransformBus::Event(m_hScrollBarEntity, &UiTransformBus::Events::GetCanvasSpaceRectNoScaleRotate, hScrollBarRect);

        return (hScrollBarRect.GetCenter().GetY() > parentRect.GetCenter().GetY());
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBoxComponent::UpdateScrollBarVisiblity()
{
    bool updateHorizontalScrollBar = m_hScrollBarEntity.IsValid() && m_isHorizontalScrollingEnabled && (m_hScrollBarVisibility != ScrollBarVisibility::AlwaysShow);
    bool updateVerticalScrollBar = m_vScrollBarEntity.IsValid() && m_isVerticalScrollingEnabled && (m_vScrollBarVisibility != ScrollBarVisibility::AlwaysShow);

    if (updateHorizontalScrollBar || updateVerticalScrollBar)
    {
        // Set scrollbar visibility based on whether there is scrollable content

        AZ::Entity* contentParentEntity = nullptr;
        UiElementBus::EventResult(contentParentEntity, m_contentEntity, &UiElementBus::Events::GetParent);
        if (contentParentEntity)
        {
            bool showHScrollBar = true;
            bool showVScrollBar = true;

            // Get content parent's size
            AZ::Vector2 parentSize;
            UiTransformBus::EventResult(parentSize, contentParentEntity->GetId(), &UiTransformBus::Events::GetCanvasSpaceSizeNoScaleRotate);

            // Get content size
            UiTransformInterface::Rect contentRect = GetAxisAlignedContentRect();
            AZ::Vector2 contentSize = contentRect.GetSize();

            // First check if none of the hideable scrollbars are needed
            bool needHScrollBar = false;
            bool needVScrollBar = false;
            if (updateHorizontalScrollBar)
            {
                needHScrollBar = (contentSize.GetX() > parentSize.GetX());
            }
            if (updateVerticalScrollBar)
            {
                needVScrollBar = (contentSize.GetY() > parentSize.GetY());
            }

            if (!needHScrollBar && !needVScrollBar)
            {
                showHScrollBar = false;
                showVScrollBar = false;
            }
            else
            {
                // Next, check if only a horizontal scrollbar is needed
                AZ::Vector2 supposedParentSize = parentSize;

                if (updateHorizontalScrollBar && (m_hScrollBarVisibility == ScrollBarVisibility::AutoHideAndResizeViewport))
                {
                    // Get height of horizontal scrollbar
                    AZ::Vector2 hScrollBarSize;
                    UiTransformBus::EventResult(
                        hScrollBarSize, m_hScrollBarEntity, &UiTransformBus::Events::GetCanvasSpaceSizeNoScaleRotate);
                    float hScrollBarHeight = hScrollBarSize.GetY();

                    supposedParentSize.SetY(supposedParentSize.GetY() - hScrollBarHeight);
                }

                if (contentSize.GetY() <= supposedParentSize.GetY() && contentSize.GetX() > supposedParentSize.GetX())
                {
                    showHScrollBar = true;
                    showVScrollBar = false;
                }
                else
                {
                    // Next, check if only a vertical scrollbar is needed
                    supposedParentSize = parentSize;

                    if (updateVerticalScrollBar && (m_vScrollBarVisibility == ScrollBarVisibility::AutoHideAndResizeViewport))
                    {
                        // Get width of vertical scrollbar
                        AZ::Vector2 vScrollBarSize;
                        UiTransformBus::EventResult(
                            vScrollBarSize, m_vScrollBarEntity, &UiTransformBus::Events::GetCanvasSpaceSizeNoScaleRotate);
                        float vScrollBarWidth = vScrollBarSize.GetX();

                        supposedParentSize.SetX(supposedParentSize.GetX() - vScrollBarWidth);
                    }

                    if (contentSize.GetX() <= supposedParentSize.GetX() && contentSize.GetY() > supposedParentSize.GetY())
                    {
                        showHScrollBar = false;
                        showVScrollBar = true;
                    }
                    else
                    {
                        // Both scrollbars are needed
                        showHScrollBar = true;
                        showVScrollBar = true;
                    }
                }
            }

            // Set enabled property on the scrollbars
            if (updateHorizontalScrollBar)
            {
                UiElementBus::Event(m_hScrollBarEntity, &UiElementBus::Events::SetIsEnabled, showHScrollBar);
            }
            if (updateVerticalScrollBar)
            {
                UiElementBus::Event(m_vScrollBarEntity, &UiElementBus::Events::SetIsEnabled, showVScrollBar);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBoxComponent::UpdateScrollBarAnchorsAndOffsets()
{
    // Set scrollbar anchors and offsets based on the other scrollbar's visibility

    if (m_hScrollBarEntity.IsValid() && m_isHorizontalScrollingEnabled && (m_hScrollBarVisibility != ScrollBarVisibility::AlwaysShow))
    {
        // Set anchors
        UiTransform2dInterface::Anchors anchors;
        UiTransform2dBus::EventResult(anchors, m_hScrollBarEntity, &UiTransform2dBus::Events::GetAnchors);
        anchors.m_left = 0.0f;
        anchors.m_right = 1.0f;
        UiTransform2dBus::Event(m_hScrollBarEntity, &UiTransform2dBus::Events::SetAnchors, anchors, false, false);

        // Set offsets

        UiTransform2dInterface::Offsets offsets;
        UiTransform2dBus::EventResult(offsets, m_hScrollBarEntity, &UiTransform2dBus::Events::GetOffsets);

        bool isVScrollBarEnabled = false;
        if (m_vScrollBarEntity.IsValid() && m_isVerticalScrollingEnabled)
        {
            UiElementBus::EventResult(isVScrollBarEnabled, m_vScrollBarEntity, &UiElementBus::Events::IsEnabled);
        }

        if (isVScrollBarEnabled)
        {
            // Get width of vertical scrollbar
            AZ::Vector2 vScrollBarSize;
            UiTransformBus::EventResult(vScrollBarSize, m_vScrollBarEntity, &UiTransformBus::Events::GetCanvasSpaceSizeNoScaleRotate);

            if (IsVerticalScrollBarOnRight())
            {
                offsets.m_left = 0.0f;
                offsets.m_right = -vScrollBarSize.GetX();
            }
            else
            {
                offsets.m_left = vScrollBarSize.GetX();
                offsets.m_right = 0.0f;
            }
        }
        else
        {
            offsets.m_left = 0.0f;
            offsets.m_right = 0.0f;
        }

        UiTransform2dBus::Event(m_hScrollBarEntity, &UiTransform2dBus::Events::SetOffsets, offsets);
    }

    if (m_vScrollBarEntity.IsValid() && m_isVerticalScrollingEnabled && (m_vScrollBarVisibility != ScrollBarVisibility::AlwaysShow))
    {
        // Set anchors
        UiTransform2dInterface::Anchors anchors;
        UiTransform2dBus::EventResult(anchors, m_vScrollBarEntity, &UiTransform2dBus::Events::GetAnchors);
        anchors.m_top = 0.0f;
        anchors.m_bottom = 1.0f;
        UiTransform2dBus::Event(m_vScrollBarEntity, &UiTransform2dBus::Events::SetAnchors, anchors, false, false);

        // Set offsets

        UiTransform2dInterface::Offsets offsets;
        UiTransform2dBus::EventResult(offsets, m_vScrollBarEntity, &UiTransform2dBus::Events::GetOffsets);

        bool isHScrollBarEnabled = false;
        if (m_hScrollBarEntity.IsValid() && m_isHorizontalScrollingEnabled)
        {
            UiElementBus::EventResult(isHScrollBarEnabled, m_hScrollBarEntity, &UiElementBus::Events::IsEnabled);
        }

        if (isHScrollBarEnabled)
        {
            // Get height of horizontal scrollbar
            AZ::Vector2 hScrollBarSize;
            UiTransformBus::EventResult(hScrollBarSize, m_hScrollBarEntity, &UiTransformBus::Events::GetCanvasSpaceSizeNoScaleRotate);

            if (IsHorizontalScrollBarOnBottom())
            {
                offsets.m_top = 0.0f;
                offsets.m_bottom = -hScrollBarSize.GetY();
            }
            else
            {
                offsets.m_top = hScrollBarSize.GetY();
                offsets.m_bottom = 0.0f;
            }
        }
        else
        {
            offsets.m_top = 0.0f;
            offsets.m_bottom = 0.0f;
        }

        UiTransform2dBus::Event(m_vScrollBarEntity, &UiTransform2dBus::Events::SetOffsets, offsets);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBoxComponent::UpdateContentParentOffsets(bool checkScrollBarVisibility)
{
    if ((m_hScrollBarEntity.IsValid() && m_isHorizontalScrollingEnabled && (m_hScrollBarVisibility == ScrollBarVisibility::AutoHideAndResizeViewport))
        || (m_vScrollBarEntity.IsValid() && m_isVerticalScrollingEnabled && (m_vScrollBarVisibility == ScrollBarVisibility::AutoHideAndResizeViewport)))
    {
        // Set content parent offsets based on scrollbar visibility

        AZ::Entity* contentParentEntity = nullptr;
        UiElementBus::EventResult(contentParentEntity, m_contentEntity, &UiElementBus::Events::GetParent);
        if (contentParentEntity)
        {
            UiTransform2dInterface::Offsets offsets;
            UiTransform2dBus::EventResult(offsets, contentParentEntity->GetId(), &UiTransform2dBus::Events::GetOffsets);

            if (m_hScrollBarEntity.IsValid() && m_isHorizontalScrollingEnabled && (m_hScrollBarVisibility == ScrollBarVisibility::AutoHideAndResizeViewport))
            {
                bool isHScrollBarEnabled = false;
                if (checkScrollBarVisibility)
                {
                    UiElementBus::EventResult(isHScrollBarEnabled, m_hScrollBarEntity, &UiElementBus::Events::IsEnabled);
                }

                if (isHScrollBarEnabled)
                {
                    // Get height of horizontal scrollbar
                    AZ::Vector2 hScrollBarSize;
                    UiTransformBus::EventResult(
                        hScrollBarSize, m_hScrollBarEntity, &UiTransformBus::Events::GetCanvasSpaceSizeNoScaleRotate);

                    if (IsHorizontalScrollBarOnBottom())
                    {
                        offsets.m_top = 0.0f;
                        offsets.m_bottom = -hScrollBarSize.GetY();
                    }
                    else
                    {
                        offsets.m_top = hScrollBarSize.GetY();
                        offsets.m_bottom = 0.0f;
                    }
                }
                else
                {
                    offsets.m_top = 0.0f;
                    offsets.m_bottom = 0.0f;
                }
            }

            if (m_vScrollBarEntity.IsValid() && m_isVerticalScrollingEnabled && (m_vScrollBarVisibility == ScrollBarVisibility::AutoHideAndResizeViewport))
            {
                bool isVScrollBarEnabled = false;
                if (checkScrollBarVisibility)
                {
                    UiElementBus::EventResult(isVScrollBarEnabled, m_vScrollBarEntity, &UiElementBus::Events::IsEnabled);
                }

                if (isVScrollBarEnabled)
                {
                    // Get width of vertical scrollbar
                    AZ::Vector2 vScrollBarSize;
                    UiTransformBus::EventResult(
                        vScrollBarSize, m_vScrollBarEntity, &UiTransformBus::Events::GetCanvasSpaceSizeNoScaleRotate);

                    if (IsVerticalScrollBarOnRight())
                    {
                        offsets.m_left = 0.0f;
                        offsets.m_right = -vScrollBarSize.GetX();
                    }
                    else
                    {
                        offsets.m_left = vScrollBarSize.GetX();
                        offsets.m_right = 0.0f;
                    }
                }
                else
                {
                    offsets.m_left = 0.0f;
                    offsets.m_right = 0.0f;
                }
            }

            UiTransform2dBus::Event(contentParentEntity->GetId(), &UiTransform2dBus::Events::SetOffsets, offsets);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiScrollBoxComponent::ContentOrParentSizeChanged()
{
    // Initialize content parent offsets if they are being controlled by scrollbar visibility behavior.
    // Offsets are initialized as if scrollbars are not visible
    UpdateContentParentOffsets(false);

    // Set whether scrollbars are visible based on scrollbar visibility behavior, content size and the size of its parent
    UpdateScrollBarVisiblity();

    // Set scrollbar anchors and offsets based on scrollbar visibility behavior and whether the other scrollbar is visible
    UpdateScrollBarAnchorsAndOffsets();

    // Set content parent offsets based on scrollbar visibility behavior and whether scrollbars are visible
    UpdateContentParentOffsets(true);

    // Notify listeners of ratio change between content size and the size of its parent
    AZ::Vector2 parentToContentRatio;
    bool result = GetScrollableParentToContentRatio(parentToContentRatio);
    if (result)
    {
        UiScrollableToScrollerNotificationBus::Event(
            GetEntityId(), &UiScrollableToScrollerNotificationBus::Events::OnScrollableParentToContentRatioChanged, parentToContentRatio);
    }

    if (DoSnap())
    {
        // Reset drag info
        if (m_isDragging)
        {
            m_pressedScrollOffset = m_scrollOffset;
            m_pressedPoint = m_lastDragPoint;
        }

        NotifyScrollersOnValueChanged();

        DoChangedActions();
    }
    else
    {
        NotifyScrollersOnValueChanged();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiScrollBoxComponent::VersionConverter(AZ::SerializeContext& context,
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

    // Conversion from version 3 to 4:
    // - Need to convert Vec2 to AZ::Vector2
    if (classElement.GetVersion() < 4)
    {
        if (!LyShine::ConvertSubElementFromVec2ToVector2(context, classElement, "ScrollOffset"))
        {
            return false;
        }

        if (!LyShine::ConvertSubElementFromVec2ToVector2(context, classElement, "SnapGrid"))
        {
            return false;
        }
    }

    return true;
}
