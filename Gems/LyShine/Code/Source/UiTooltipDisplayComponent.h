/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <LyShine/Bus/UiTooltipDisplayBus.h>
#include <LyShine/Bus/UiInitializationBus.h>
#include <LyShine/Bus/UiTransformBus.h>
#include <LyShine/Animation/IUiAnimation.h>
#include <LyShine/UiComponentTypes.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Serialization/SerializeContext.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiTooltipDisplayComponent
    : public AZ::Component
    , public UiTooltipDisplayBus::Handler
    , public UiInitializationBus::Handler
    , public IUiAnimationListener
{
public: //types

    using EntityComboBoxVec = AZStd::vector<AZStd::pair<AZ::EntityId, AZStd::string> >;
    using SequenceComboBoxVec = AZStd::vector<AZStd::string>;
    enum class State
    {
        Hiding,
        Hidden,
        DelayBeforeShow,
        Showing,
        Shown
    };

public: // member functions

    AZ_COMPONENT(UiTooltipDisplayComponent, LyShine::UiTooltipDisplayComponentUuid, AZ::Component);

    UiTooltipDisplayComponent();
    ~UiTooltipDisplayComponent() override;

    // UiTooltipDisplayInterface
    TriggerMode GetTriggerMode() override;
    void SetTriggerMode(TriggerMode triggerMode) override;
    bool GetAutoPosition() override;
    void SetAutoPosition(bool autoPosition) override;
    AutoPositionMode GetAutoPositionMode() override;
    void SetAutoPositionMode(AutoPositionMode autoPositionMode) override;
    const AZ::Vector2& GetOffset() override;
    void SetOffset(const AZ::Vector2& offset) override;
    bool GetAutoSize() override;
    void SetAutoSize(bool autoSize) override;
    AZ::EntityId GetTextEntity() override;
    void SetTextEntity(AZ::EntityId textEntity) override;
    float GetDelayTime() override;
    void SetDelayTime(float delayTime) override;
    float GetDisplayTime() override;
    void SetDisplayTime(float displayTime) override;
    void PrepareToShow(AZ::EntityId tooltipElement) override;
    void Hide() override;
    void Update() override;
    // ~UiTooltipDisplayInterface

    // UiInitializationInterface
    void InGamePostActivate() override;
    // ~UiInitializationInterface

    //! IUiAnimationListener
    void OnUiAnimationEvent(EUiAnimationEvent uiAnimationEvent, IUiAnimSequence* pAnimSequence) override;
    // ~IUiAnimationListener

    State GetState();

public:  // static member functions

    static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("UiTooltipDisplayService"));
    }

    static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("UiTooltipService"));
        incompatible.push_back(AZ_CRC_CE("UiTooltipDisplayService"));
    }

    static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("UiElementService"));
        required.push_back(AZ_CRC_CE("UiTransformService"));
    }

    static void Reflect(AZ::ReflectContext* context);

protected: // member functions

    // AZ::Component
    void Activate() override;
    void Deactivate() override;
    // ~AZ::Component

    AZ_DISABLE_COPY_MOVE(UiTooltipDisplayComponent);

    //! Set the current state of the tooltip
    void SetState(State state);

    //! Skip to the next state if the current state is a transitional one
    void EndTransitionState();

    //! Show the display element
    void Show();

    //! Resize the display element so that its child text element is the same size as the
    //! text string. The text element's anchors are assumed to be set up so that the text
    //! element grows/shrinks with its parent.
    void AutoResize();

    //! Resize an element by a specified delta
    void ResizeElementByDelta(AZ::EntityId entityId, const AZ::Vector2& delta);

    //! Position the display element according to the positioning mode
    void AutoPosition();

    //! Move the display element to a specified position
    void MoveToPosition(const AZ::Vector2& point, const AZ::Vector2& offsetFromPoint);

    //! Change the vertical position of the display element if it exceeds a bounding rect.
    //! If the element exceeds the top of the rect, move it so that its top is a certain
    //! distance below the specified point. If the element exceeds the bottom of the rect,
    //! move it so that its bottom is a certain distance above the specified point.
    void CheckBoundsAndChangeYPosition(const UiTransformInterface::Rect& boundsRect, float yPoint, float yOffsetFromPoint);

    //! Constrain the display element to a bounding rect
    void ConstrainToBounds(const UiTransformInterface::Rect& boundsRect);

    //! Get the axis aligned rect of the display element
    UiTransformInterface::Rect GetAxisAlignedRect();

    //! Get the canvas's animation system
    IUiAnimationSystem* GetAnimationSystem();

    //! Get a sequence that is owned by the canvas's animation system
    IUiAnimSequence* GetSequence(const AZStd::string& sequenceName, IUiAnimationSystem*& animSystemOut);

    //! Initialize the sequence before playing
    void PrepareSequenceForPlay(const AZStd::string& sequenceName, IUiAnimSequence*& sequence, IUiAnimationSystem*& animSystemOut);

    EntityComboBoxVec PopulateTextEntityList();
    SequenceComboBoxVec PopulateSequenceList();

private: // static member functions

    static bool VersionConverter(AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement);

protected: // data

    TriggerMode m_triggerMode;

    bool m_autoPosition;
    AutoPositionMode m_autoPositionMode;
    AZ::Vector2 m_offset;

    AZ::EntityId m_textEntity;
    bool m_autoSize;

    float m_delayTime;
    float m_displayTime;

    AZStd::string m_showSequenceName;
    AZStd::string m_hideSequenceName;

    State m_state;
    float m_stateStartTime;
    float m_curDelayTime;
    float m_timeSinceLastShown;
    AZ::EntityId m_tooltipElement;
    float m_maxWrapTextWidth;

    IUiAnimSequence* m_showSequence;
    IUiAnimSequence* m_hideSequence;

    bool m_listeningForAnimationEvents = false;
};
