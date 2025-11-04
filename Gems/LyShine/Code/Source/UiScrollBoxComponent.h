/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "UiInteractableComponent.h"
#include <LyShine/Bus/UiScrollBoxBus.h>
#include <LyShine/Bus/UiScrollableBus.h>
#include <LyShine/Bus/UiInitializationBus.h>
#include <LyShine/Bus/UiInteractableBus.h>
#include <LyShine/Bus/UiScrollerBus.h>
#include <LyShine/Bus/UiTransform2dBus.h>
#include <LyShine/Bus/UiTransformBus.h>
#include <LyShine/UiComponentTypes.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Math/Vector3.h>

#include <LmbrCentral/Rendering/TextureAsset.h>
#include "UiNavigationHelpers.h"

#include <AzCore/Component/TickBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiScrollBoxComponent
    : public UiInteractableComponent
    , public UiScrollBoxBus::Handler
    , public UiScrollableBus::Handler
    , public UiInitializationBus::Handler
    , public UiScrollerToScrollableNotificationBus::MultiHandler
    , public UiTransformChangeNotificationBus::MultiHandler
    , public AZ::TickBus::Handler
{
public: // member functions

    AZ_COMPONENT(UiScrollBoxComponent, LyShine::UiScrollBoxComponentUuid, AZ::Component);

    UiScrollBoxComponent();
    ~UiScrollBoxComponent() override;

    // UiScrollBoxInterface
    AZ::Vector2 GetScrollOffset() override;
    void SetScrollOffset(AZ::Vector2 scrollOffset) override;

    AZ::Vector2 GetNormalizedScrollValue() override;

    void ChangeContentSizeAndScrollOffset(AZ::Vector2 contentSize, AZ::Vector2 scrollOffset) override;

    bool HasHorizontalContentToScroll() override;
    bool HasVerticalContentToScroll() override;

    virtual bool GetIsHorizontalScrollingEnabled() override;
    virtual void SetIsHorizontalScrollingEnabled(bool isEnabled) override;
    virtual bool GetIsVerticalScrollingEnabled() override;
    virtual void SetIsVerticalScrollingEnabled(bool isEnabled) override;
    virtual bool GetIsScrollingConstrained() override;
    virtual void SetIsScrollingConstrained(bool isConstrained) override;
    virtual SnapMode GetSnapMode() override;
    virtual void SetSnapMode(SnapMode snapMode) override;
    virtual AZ::Vector2 GetSnapGrid() override;
    virtual void SetSnapGrid(AZ::Vector2 snapGrid) override;
    virtual ScrollBarVisibility GetHorizontalScrollBarVisibility() override;
    virtual void SetHorizontalScrollBarVisibility(ScrollBarVisibility visibility) override;
    virtual ScrollBarVisibility GetVerticalScrollBarVisibility() override;
    virtual void SetVerticalScrollBarVisibility(ScrollBarVisibility visibility) override;
    virtual AZ::Vector2 GetScrollSensitivity() override;
    virtual void SetScrollSensitivity(AZ::Vector2 scrollSensitivity) override;
    virtual float GetMomentumDuration() override;
    virtual void SetMomentumDuration(float scrollMomentumDuration) override;
    virtual void SetMomentumActive(bool active) override;
    virtual void StopMomentum() override;

    ScrollOffsetChangeCallback GetScrollOffsetChangingCallback() override;
    void SetScrollOffsetChangingCallback(ScrollOffsetChangeCallback onChange) override;
    const LyShine::ActionName& GetScrollOffsetChangingActionName() override;
    void SetScrollOffsetChangingActionName(const LyShine::ActionName& actionName) override;
    ScrollOffsetChangeCallback GetScrollOffsetChangedCallback() override;
    void SetScrollOffsetChangedCallback(ScrollOffsetChangeCallback onChange) override;
    const LyShine::ActionName& GetScrollOffsetChangedActionName() override;
    void SetScrollOffsetChangedActionName(const LyShine::ActionName& actionName) override;

    void SetContentEntity(AZ::EntityId entityId) override;
    AZ::EntityId GetContentEntity() override;
    void SetHorizontalScrollBarEntity(AZ::EntityId entityId) override;
    AZ::EntityId GetHorizontalScrollBarEntity() override;
    void SetVerticalScrollBarEntity(AZ::EntityId entityId) override;
    AZ::EntityId GetVerticalScrollBarEntity() override;

    AZ::EntityId FindClosestContentChildElement() override;
    AZ::EntityId FindNextContentChildElement(UiNavigationHelpers::Command command);
    // ~UiScrollBoxInterface

    // UiScrollableInterface
    bool GetScrollableParentToContentRatio(AZ::Vector2& ratioOut) override;
    // ~UiScrollableInterface

    // UiScrollerToScrollableNotifications
    void OnValueChangingByScroller(float value) override;
    void OnValueChangedByScroller(float value) override;
    // ~UiScrollerToScrollableNotifications

    // UiInitializationInterface
    void InGamePostActivate() override;
    // ~UiInitializationInterface

    // UiInteractableInterface
    bool HandlePressed(AZ::Vector2 point, bool& shouldStayActive) override;
    bool HandleReleased(AZ::Vector2 point) override;
    bool HandleEnterPressed(bool& shouldStayActive) override;
    bool HandleAutoActivation() override;
    bool HandleKeyInputBegan(const AzFramework::InputChannel::Snapshot& inputSnapshot, AzFramework::ModifierKeyMask activeModifierKeys) override;
    void InputPositionUpdate(AZ::Vector2 point) override;
    bool DoesSupportDragHandOff(AZ::Vector2 startPoint) override;
    bool OfferDragHandOff(AZ::EntityId currentActiveInteractable, AZ::Vector2 startPoint, AZ::Vector2 currentPoint, float dragThreshold) override;
    void LostActiveStatus() override;
    void HandleDescendantReceivedHoverByNavigation(AZ::EntityId descendantEntityId) override;
    // ~UiInteractableInterface

    // UiTransformChangeNotification
    void OnCanvasSpaceRectChanged(AZ::EntityId entityId, const UiTransformInterface::Rect& oldRect, const UiTransformInterface::Rect& newRect) override;
    // ~UiTransformChangeNotification

    // TickBus
    void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
    // ~TickBus

protected: // member functions

    // AZ::Component
    void Activate() override;
    void Deactivate() override;
    // ~AZ::Component

    // UiInteractableComponent
    bool IsAutoActivationSupported() override;
    // ~UiInteractableComponent

    UiInteractableStatesInterface::State ComputeInteractableState() override;

    static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("UiInteractableService"));
        provided.push_back(AZ_CRC_CE("UiNavigationService"));
        provided.push_back(AZ_CRC_CE("UiStateActionsService"));
        provided.push_back(AZ_CRC_CE("UiScrollBoxService"));
    }

    static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("UiInteractableService"));
        incompatible.push_back(AZ_CRC_CE("UiNavigationService"));
        incompatible.push_back(AZ_CRC_CE("UiStateActionsService"));
    }

    static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("UiElementService"));
        required.push_back(AZ_CRC_CE("UiTransformService"));
    }

    static void Reflect(AZ::ReflectContext* context);

private: // types

    using EntityComboBoxVec = AZStd::vector< AZStd::pair< AZ::EntityId, AZStd::string > >;

private: // member functions

    AZ_DISABLE_COPY_MOVE(UiScrollBoxComponent);

    //! Methods used for controlling the Edit Context (the properties pane)
    EntityComboBoxVec PopulateChildEntityList();
    EntityComboBoxVec PopulateHScrollBarEntityList();
    EntityComboBoxVec PopulateVScrollBarEntityList();
    EntityComboBoxVec PopulateScrollBarEntityList(UiScrollerInterface::Orientation orientation);
    bool IsSnapToGrid() const;

    //! Given a proposed scroll offset, adjust it so that the area outside
    //! the content rectangle cannot be seen in its parent rectangle.
    //! I.e. prevent scrolling beyond the edges of the content
    AZ::Vector2 ConstrainOffset(AZ::Vector2 proposedOffset, AZ::Entity* contentParentEntity);

    //! Snap m_scrollOffset according to the snap mode
    bool DoSnap();

    //! Compute the offset from the content anchors to the child's pivot
    //! using the current scroll offset
    AZ::Vector2 ComputeCurrentOffsetToChild(AZ::EntityId child);

    //! Compute the offset of the current scroll offset from the closet
    //! snap grid point
    AZ::Vector2 ComputeCurrentOffsetFromGrid();

    //! Helper function to return the position of the content element's
    //! anchors in canvas space. The scroll offset is always relative to this
    //! point.
    AZ::Vector2 ComputeContentAnchorCenterInCanvasSpace() const;

    //! Helper function to calculate how far a float value is from a grid
    float ComputeOffsetOfValueFromGrid(float value, float gridStep);

    //! Get the drag distance along valid axes
    float GetValidDragDistanceInPixels(AZ::Vector2 startPoint, AZ::Vector2 endPoint);

    //! Given the latest input point, potentially initiate a drag or hand one off to a parent
    void CheckForDragOrHandOffToParent(AZ::Vector2 point);

    //! Set scroll offset value and update content's offsets
    void DoSetScrollOffset(AZ::Vector2 scrollOffset);

    //! Notify listeners, callbacks and canvas actions of a scroll offset and value change
    void DoChangedActions();
    void DoChangingActions();

    //! Notify scrollers that the scrollable caused the scroll value to change. Sends value (0 - 1)
    void NotifyScrollersOnValueChanged();
    void NotifyScrollersOnValueChanging();

    //! Notify listeners of a scroll value change. Sends value (0 - 1)
    void NotifyListenersOnScrollValueChanged();
    void NotifyListenersOnScrollValueChanging();

    //! Notify listeners of a scroll offset change. Sends new scroll offset
    void NotifyListenersOnScrollOffsetChanged();
    void NotifyListenersOnScrollOffsetChanging();

    //! Get the axis aligned rect of the content element
    UiTransformInterface::Rect GetAxisAlignedContentRect();

    //! Helper functions to convert between scroller and scrollable values
    bool ScrollOffsetToNormalizedScrollValue(AZ::Vector2 scrollOffset, AZ::Vector2& normalizedScrollValueOut);
    bool NormalizedScrollValueToScrollOffset(UiScrollerInterface::Orientation orientation, float normalizedScrollValue, float& scrollOffsetOut);
    bool ScrollerValueToScrollOffsets(AZ::EntityId scroller, float scrollerValue, AZ::Vector2& scrollOffsetsOut);

    //! Check which side the scrollbar is on
    bool IsVerticalScrollBarOnRight();
    bool IsHorizontalScrollBarOnBottom();

    // Set scrollbar visibility based on whether there is scrollable content
    void UpdateScrollBarVisiblity();

    // Set scrollbar anchors and offsets based on the other scrollbar's visibility
    void UpdateScrollBarAnchorsAndOffsets();

    // Set content parent (viewport) offsets based on scrollbar visibility. The content's parent (the viewport)
    // is shrunk when scrollbars are visible and expanded when scrollbars are not visible.
    // If checkScrollBarVisibility is false, the offsets are set as if the scrollbars are not visible
    void UpdateContentParentOffsets(bool checkScrollBarVisibility);

    // Setup based on the size of the content and its parent
    void ContentOrParentSizeChanged();

private: // static member functions

    static bool VersionConverter(AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement);

private: // data

    //! The scroll offset is the distance from the content elements anchors to its pivot
    //! It is initialized from a property but changes at runtime as the element is scrolled
    AZ::Vector2 m_scrollOffset;

    //! Property values
    bool m_isHorizontalScrollingEnabled;
    bool m_isVerticalScrollingEnabled;
    bool m_isScrollingConstrained;
    SnapMode m_snapMode;
    AZ::Vector2 m_snapGrid;

    AZ::EntityId m_contentEntity;
    AZ::EntityId m_hScrollBarEntity;
    AZ::EntityId m_vScrollBarEntity;
    ScrollBarVisibility m_hScrollBarVisibility;
    ScrollBarVisibility m_vScrollBarVisibility;

    ScrollOffsetChangeCallback m_onScrollOffsetChanged;
    ScrollOffsetChangeCallback m_onScrollOffsetChanging;

    LyShine::ActionName m_scrollOffsetChangedActionName;
    LyShine::ActionName m_scrollOffsetChangingActionName;

    //! Interactable state
    bool m_isDragging;
    bool m_isActive; // true when interactable can be manipulated by key input

    AZ::Vector2 m_pressedScrollOffset; // the original value of scrollOffset when the press occurred

    AZ::Vector2 m_lastDragPoint; // the point of the last drag

    AZ::Vector2 m_scrollSensitivity = AZ::Vector2::CreateZero(); // Vector2(horizontal, vertical) factor applied to the dragging vector to adjust scroll speed
    AZ::Vector2 m_lastOffsetChange = AZ::Vector2::CreateZero(); // Last instant offset change
    AZ::Vector2 m_offsetChangeAccumulator = AZ::Vector2::CreateZero();
    float m_stoppingTimeAccumulator = 0.f;
    float m_draggingTimeAccumulator = 0.f;
    bool m_momentumIsActive = false;
    float m_momentumDuration = 0.f; // Time in seconds for which we keep scrolling after release
    float m_momentumTimeAccumulator = 0.f;
    const float MIN_OFFSET_THRESHOLD = 10.0f;
    const float MAX_STOPPING_DELAY = 0.12f;
};
