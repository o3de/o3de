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
#include <AzFramework/Entity/EntityContextBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Bus that defines the interface for flipbook animations.
//!
//! A flipbook animation component exists on an entity that has an image component
//! and interacts with the image bus to achieve its functionality (such as by
//! manipulating sprite-sheet indices).
class UiFlipbookAnimationInterface
    : public AZ::ComponentBus
{
public: // Types

    //! Defines the looping behavior when playing back a flipbook animation.
    enum class LoopType
    {
        None,       //!< No looping behavior
        
        Linear,     //!< When end frame is reached, next frame will be the loop start frame

        PingPong    //!< When end frame is reached, next frame will be the previous frame, 
                    //!< continuing in reverse until the start frame is reached.
    };

    //! Units of speed for framerate
    enum class FramerateUnits
    {
        FPS,                //!< Framerate of animation

        SecondsPerFrame,    //!< Number of seconds to wait before playing next frame
    };

public:
    virtual ~UiFlipbookAnimationInterface() {}

    //! Start the animation sequence, beginning at the start frame.
    //!
    //! If a LoopType other than None has been set, the animation won't stop
    //! unless explicitly done so (or the image is unloaded/destroyed).
    virtual void Start() = 0;

    //! Stops animation playback.
    virtual void Stop() = 0;

    //! \return True if the flipbook animation is currently playing, false otherwise.
    virtual bool IsPlaying() = 0;

    //! \return The starting frame of the animation.
    virtual AZ::u32 GetStartFrame() = 0;

    //! Sets the starting frame of the animation.
    virtual void SetStartFrame(AZ::u32 startFrame) = 0;

    //! \return End frame of the animation.
    virtual AZ::u32 GetEndFrame() = 0;

    //! Sets the ending frame of the animation.
    virtual void SetEndFrame(AZ::u32 endFrame) = 0;

    //! \return The current frame of the animation that's being rendered.
    virtual AZ::u32 GetCurrentFrame() = 0;

    //! Sets the current frame of the animation to render.
    //!
    //! If the animation is currently playing, this will effectively "skip"
    //! to the given frame.
    virtual void SetCurrentFrame(AZ::u32 currentFrame) = 0;

    //! This frame is distinct from the start frame and allows a "lead in"
    //! seqence of frames to play leading up to the looping animation. The
    //! frames that occur prior to the loop start frame will only play once.
    //!
    //! \return The frame to start the loop from.
    virtual AZ::u32 GetLoopStartFrame() = 0;

    //! Sets the starting frame for looping sequences.
    virtual void SetLoopStartFrame(AZ::u32 loopStartFrame) = 0;

    //! \return The LoopType of the flipbook animation.
    virtual LoopType GetLoopType() = 0;

    //! Sets the LoopType of the flipbook animation.
    virtual void SetLoopType(LoopType loopType) = 0;
    
    //! Gets the speed used to determine when to transition to the next frame.
    //!
    //! Framerate is defined relative to unit of time, specified by FramerateUnits.
    //!
    //! See GetFramerateUnit, SetFramerateUnit.
    virtual float GetFramerate() = 0;

    //! Sets the speed used to determine when to transition to the next frame.
    //!
    //! Framerate is defined relative to unit of time, specified by FramerateUnits.
    //!
    //! See GetFramerateUnit, SetFramerateUnit.
    virtual void SetFramerate(float framerate) = 0;

    //! Gets the framerate unit.
    virtual FramerateUnits GetFramerateUnit() = 0;

    //! Sets the framerate unit.
    virtual void SetFramerateUnit(FramerateUnits framerateUnit) = 0;

    //! Gets the delay (in seconds) before playing the flipbook (applied only once during playback).
    virtual float GetStartDelay() = 0;

    //! Sets the delay (in seconds) before playing the flipbook (applied only once during playback).
    virtual void SetStartDelay(float startDelay) = 0;
    
    //! Gets the delay (in seconds) before playing the loop sequence.
    virtual float GetLoopDelay() = 0;

    //! Sets the delay (in seconds) before playing the loop sequence.
    virtual void SetLoopDelay(float loopDelay) = 0;

    //! Gets the delay (in seconds) before playing the reverse loop sequence (PingPong loop types only).
    virtual float GetReverseDelay() = 0;

    //! Sets the delay (in seconds) before playing the reverse loop sequence (PingPong loop types only).
    virtual void SetReverseDelay(float reverseDelay) = 0;

    //! Returns true if the animation will begin playing when the component activates, false otherwise.
    virtual bool GetIsAutoPlay() = 0;

    //! Sets whether the animation will automatically begin playing.
    //!
    //! This flag is ignored after the component has activated.
    virtual void SetIsAutoPlay(bool isAutoPlay) = 0;
};

using UiFlipbookAnimationBus = AZ::EBus<UiFlipbookAnimationInterface>;

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Allows listeners to be aware of events, like loop completion, occurring.
class UiFlipbookAnimationNotifications
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiFlipbookAnimationNotifications() {}

    //! Notify listeners when the animation starts
    virtual void OnAnimationStarted() {}

    //! Notify listeners when the animation stops
    virtual void OnAnimationStopped() {}

    //! Notify listeners when the current loop sequence has completed
    //!
    //! This will only trigger for LoopType sequences other than None.
    //!
    //! For Linear LoopType, this will trigger on the last frame of the last
    //! frame of the loop.
    //!
    //! For PingPong LoopType, this will trigger on the last frame of the
    //! loop sequence before reversing the loop direction.
    virtual void OnLoopSequenceCompleted() {}
};

typedef AZ::EBus<UiFlipbookAnimationNotifications> UiFlipbookAnimationNotificationsBus;
