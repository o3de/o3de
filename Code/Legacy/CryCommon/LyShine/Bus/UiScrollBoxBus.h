/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Vector2.h>
#include <LyShine/UiBase.h>
#include <functional>

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiScrollBoxInterface
    : public AZ::ComponentBus
{
public: // types

    //! Callback for when box is scrolled
    typedef AZStd::function<void(AZ::EntityId sendingEntityId, AZ::Vector2 newScrollOffset)> ScrollOffsetChangeCallback;

    enum class SnapMode
    {
        None,
        Children,
        Grid
    };

    enum class ScrollBarVisibility
    {
        AlwaysShow,
        AutoHide,
        AutoHideAndResizeViewport
    };

public: // member functions

    virtual ~UiScrollBoxInterface() {}

    //! Query the current scroll offset of the scroll box
    //! \return     The current scroll offset for the scroll box.
    //!
    //! The scroll offset is the offset from the content element's anchor point to
    //! the content element's pivot.
    //! If the content element's anchor points are split it is the offset from
    //! their center to the pivot.
    //! So if the mouse is dragged left, the content moves to the left, so the offset
    //! X has the X drag amount subtracted from it.
    //! Using the offset from the anchor to the pivot allows the offset to be center
    //! to center or top-left to top-left, or any combination of the possible
    //! anchor and pivot positions
    virtual AZ::Vector2 GetScrollOffset() = 0;

    //! Set the scroll offset of the scroll box
    //! \param isOn     The new desired scroll offset of the scroll box.
    virtual void SetScrollOffset(AZ::Vector2 scrollOffset) = 0;

    //! Get the scroll value from 0 - 1
    virtual AZ::Vector2 GetNormalizedScrollValue() = 0;

    //! Change content size and scroll offset, and handle the changes
    virtual void ChangeContentSizeAndScrollOffset(AZ::Vector2 contentSize, AZ::Vector2 scrollOffset) = 0;

    //! Get whether there is content to scroll horizontally
    virtual bool HasHorizontalContentToScroll() = 0;

    //! Get whether there is content to scroll vertically
    virtual bool HasVerticalContentToScroll() = 0;

    //! Get whether horizontal scrolling interaction is enabled
    virtual bool GetIsHorizontalScrollingEnabled() = 0;

    //! Set whether horizontal scrolling interaction is enabled
    virtual void SetIsHorizontalScrollingEnabled(bool isEnabled) = 0;

    //! Get whether vertical scrolling interaction is enabled
    virtual bool GetIsVerticalScrollingEnabled() = 0;

    //! Set whether vertical scrolling interaction is enabled
    virtual void SetIsVerticalScrollingEnabled(bool isEnabled) = 0;

    //! Get whether scrolling interaction is constrained to the content area
    virtual bool GetIsScrollingConstrained() = 0;

    //! Set whether vertical scrolling interaction is constrained to the content area
    virtual void SetIsScrollingConstrained(bool isConstrained) = 0;

    //! Get snap mode
    virtual SnapMode GetSnapMode() = 0;

    //! Set snap mode
    virtual void SetSnapMode(SnapMode snapMode) = 0;

    //! Get snap grid
    virtual AZ::Vector2 GetSnapGrid() = 0;

    //! Set snap grid
    virtual void SetSnapGrid(AZ::Vector2 snapGrid) = 0;

    //! Get horizontal scrollbar visibility behavior
    virtual ScrollBarVisibility GetHorizontalScrollBarVisibility() = 0;

    //! Set horizontal scrollbar visibility behavior
    virtual void SetHorizontalScrollBarVisibility(ScrollBarVisibility visibility) = 0;

    //! Get vertical scrollbar visibility behavior
    virtual ScrollBarVisibility GetVerticalScrollBarVisibility() = 0;

    //! Set vertical scrollbar visibility behavior
    virtual void SetVerticalScrollBarVisibility(ScrollBarVisibility visibility) = 0;

    //! Get the callback invoked while the scroll offset is changing
    virtual ScrollOffsetChangeCallback GetScrollOffsetChangingCallback() = 0;

    //! Set the callback invoked while the scroll offset is changing
    virtual void SetScrollOffsetChangingCallback(ScrollOffsetChangeCallback onChange) = 0;

    //! Get the action triggered while the scroll offset is changing
    virtual const LyShine::ActionName& GetScrollOffsetChangingActionName() = 0;

    //! Set the action triggered while the scroll offset is changing
    virtual void SetScrollOffsetChangingActionName(const LyShine::ActionName& actionName) = 0;

    //! Get the callback invoked when the scroll offset is done changing
    virtual ScrollOffsetChangeCallback GetScrollOffsetChangedCallback() = 0;

    //! Set the callback invoked when the scroll offset is done changing
    virtual void SetScrollOffsetChangedCallback(ScrollOffsetChangeCallback onChange) = 0;

    //! Get the action triggered when the scroll offset is done changing
    virtual const LyShine::ActionName& GetScrollOffsetChangedActionName() = 0;

    //! Set the action triggered when the scroll offset is done changing
    virtual void SetScrollOffsetChangedActionName(const LyShine::ActionName& actionName) = 0;

    //! Set the optional content entity, if none is specified then nothing gets scrolled
    virtual void SetContentEntity(AZ::EntityId entityId) = 0;

    //! Get the optional content entity
    virtual AZ::EntityId GetContentEntity() = 0;

    //! Set the optional horizontal scrollbar entity
    virtual void SetHorizontalScrollBarEntity(AZ::EntityId entityId) = 0;

    //! Get the optional horizontal scrollbar entity
    virtual AZ::EntityId GetHorizontalScrollBarEntity() = 0;

    //! Set the optional vertical scrollbar entity
    virtual void SetVerticalScrollBarEntity(AZ::EntityId entityId) = 0;

    //! Get the optional vertical scrollbar entity
    virtual AZ::EntityId GetVerticalScrollBarEntity() = 0;

    //! Find the child of the content element that is closest to the content anchors
    //! at the current scroll offset. i.e. what is the currently "selected" child
    virtual AZ::EntityId FindClosestContentChildElement() = 0;

public: // static member data

    //! Only one component on a entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
};

typedef AZ::EBus<UiScrollBoxInterface> UiScrollBoxBus;

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Interface class that listeners need to implement to receive scroll box change notifications
class UiScrollBoxNotifications
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiScrollBoxNotifications(){}

    //! Called when the scroll offset is changing
    virtual void OnScrollOffsetChanging(AZ::Vector2 newScrollOffset) = 0;

    //! Called when the scroll offset is done changing
    virtual void OnScrollOffsetChanged(AZ::Vector2 newScrollOffset) = 0;
};

typedef AZ::EBus<UiScrollBoxNotifications> UiScrollBoxNotificationBus;
