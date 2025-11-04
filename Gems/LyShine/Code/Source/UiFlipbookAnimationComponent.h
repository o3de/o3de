/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Slice/SliceAsset.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <LyShine/Bus/UiCanvasUpdateNotificationBus.h>

#include <LyShine/Bus/UiFlipbookAnimationBus.h>
#include <LyShine/Bus/UiInitializationBus.h>
#include <LyShine/Bus/Sprite/UiSpriteBus.h>
#include <LyShine/UiComponentTypes.h>

#include "EditorPropertyTypes.h"

//////////////////////////////////////////////////////////////////////////
//! FlipbookAnimationComponent
//! FlipbookAnimationComponent provides a way to create an animated sprite for a UI canvas
//! using sprite-sheets (via the image component)
class UiFlipbookAnimationComponent
    : public AZ::Component
    , public UiCanvasUpdateNotificationBus::Handler
    , public UiFlipbookAnimationBus::Handler
    , public UiInitializationBus::Handler
    , public UiSpriteSourceNotificationBus::Handler
{
public: // static functions

    static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("UiFlipbookService"));
    }

    static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("UiFlipbookService"));
    }

    static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("UiIndexableImageService"));
    }

public: // functions

    AZ_COMPONENT(UiFlipbookAnimationComponent, LyShine::UiFlipbookAnimationComponentUuid);

    UiFlipbookAnimationComponent() = default;
    ~UiFlipbookAnimationComponent() override = default;

    //////////////////////////////////////////////////////////////////////////
    // AZ::Component
    void Activate() override;
    void Deactivate() override;

    //////////////////////////////////////////////////////////////////////////
    // UiCanvasUpdateNotificationBus::Handler
    void Update(float deltaTime) override;

    //////////////////////////////////////////////////////////////////////////
    // UiInitializationBus::Handler
    void InGamePostActivate() override;

    //////////////////////////////////////////////////////////////////////////
    // UiFlipbookAnimationBus::Handler
    void Start() override;
    void Stop() override;
    bool IsPlaying() override { return m_isPlaying; }
    AZ::u32 GetStartFrame() override { return m_startFrame; }
    void SetStartFrame(AZ::u32 startFrame) override;
    AZ::u32 GetEndFrame() override { return m_endFrame; }
    void SetEndFrame(AZ::u32 endFrame) override;
    AZ::u32 GetCurrentFrame() override { return m_currentFrame; }
    void SetCurrentFrame(AZ::u32 currentFrame) override;
    AZ::u32 GetLoopStartFrame() override { return m_loopStartFrame; }
    void SetLoopStartFrame(AZ::u32 loopStartFrame) override;
    UiFlipbookAnimationInterface::LoopType GetLoopType() override { return m_loopType; }
    void SetLoopType(UiFlipbookAnimationInterface::LoopType loopType) override;
    float GetFramerate() override { return m_framerate; }
    void SetFramerate(float framerate) override { m_framerate = AZ::GetMax<float>(0.0f, framerate); }
    FramerateUnits GetFramerateUnit() override { return m_framerateUnit; }
    void SetFramerateUnit(FramerateUnits framerateUnit) override { m_framerateUnit = framerateUnit; }
    float GetStartDelay() override { return m_startDelay; }
    void SetStartDelay(float startDelay) override { m_startDelay = AZ::GetMax<float>(0.0f, startDelay); }
    float GetLoopDelay() override { return m_loopDelay; }
    void SetLoopDelay(float loopDelay) override { m_loopDelay = AZ::GetMax<float>(0.0f, loopDelay); }
    float GetReverseDelay() override { return m_reverseDelay; }
    void SetReverseDelay(float reverseDelay) override { m_reverseDelay = AZ::GetMax<float>(0.0f, reverseDelay); }
    bool GetIsAutoPlay() override { return m_isAutoPlay; }
    void SetIsAutoPlay(bool isAutoPlay) override { m_isAutoPlay = isAutoPlay; }

    //////////////////////////////////////////////////////////////////////////
    // UiSpriteSourceNotificationBus::Handler
    void OnSpriteSourceChanged() override;

protected: // static functions

    //////////////////////////////////////////////////////////////////////////
    // Component descriptor
    static void Reflect(AZ::ReflectContext* context);
    //////////////////////////////////////////////////////////////////////////

protected: // functions

    //! Returns a string representation of the indices used to index sprite-sheet types.
    LyShine::AZu32ComboBoxVec PopulateIndexStringList() const;

    //! Populates a list of enumerated frame values between the start and end frame range
    LyShine::AZu32ComboBoxVec PopulateConstrainedIndexStringList() const;

    //! Total number of cells within sprite-sheet
    AZ::u32  GetMaxFrame() const;

    //! Ensures that the given frame value is valid for the associated sprite-sheet
    bool FrameWithinRange(AZ::u32 frameValue);

    //! Updates correlated frame values when the start frame value changes.
    void OnStartFrameChange();

    //! Updates correlated frame values when the end frame value changes.
    void OnEndFrameChange();

    //! Converts the frame-rate value based on the new framerate unit.
    void OnFramerateUnitChange();

    //! Returns true if this is a PingPong loop type, false otherwise.
    bool IsPingPongLoopType() const;

    //! Returns true if animation will loop, false otherwise.
    bool IsLoopingType() const;

    //! Returns the amount to delay the current (forward or reverse) loop sequence
    float CalculateLoopDelay() const;

    //! Returns the framerate in seconds-per-frame.
    float CalculateFramerateAsSecondsPerFrame() const { return m_framerateUnit == FramerateUnits::FPS && m_framerate != 0.0f ? 1.0f / m_framerate : m_framerate; }

protected: // variables

    // Serialized members
    AZ::u32 m_startFrame = 0;       //!< Start frame of animation. Can be different from "loop start" frame
                                    //!< to allow animations to have an "intro" sequence.

    AZ::u32 m_endFrame = 0;         //!< Last frame of animation.

    AZ::u32 m_loopStartFrame = 0;   //!< Start frame for looped animations

    UiFlipbookAnimationInterface::LoopType m_loopType = UiFlipbookAnimationInterface::LoopType::None;

    float   m_framerate = 0.0f;     //!< Transition speed between frames (either in FPS or seconds to wait per frame).

    float   m_startDelay = 0.0f;    //!< Number of seconds to wait before playing the flipbook (applied only once during playback).

    float   m_loopDelay = 0.0f;     //!< Number of seconds to wait before playing the loop sequence
    
    float   m_reverseDelay = 0.0f;  //!< Number of seconds to wait before playing the reverse loop sequence (PingPong loop types)

    bool    m_isAutoPlay = true;    //!< Whether the animation should automatically start playing.

    FramerateUnits m_framerateUnit = FramerateUnits::FPS; //! Units to use when defining framerate

    // Non-serialized members
    AZ::u32 m_currentFrame = 0;         //!< Current sprite-sheet frame/index displayed

    AZ::u32 m_prevFrame = 0;            //!< Previous frame displayed - used to detect intro sequences in a loop

    float   m_elapsedTime = 0.0f;       //!< Used to determine passage of time for handling frame delay

    bool    m_isPlaying = false;        //!< True if the animation is playing, false otherwise

    AZ::s32 m_currentLoopDirection = 1; //!< Used for PingPong loop direction (positive/negative)

    bool    m_useStartDelay = false;    //!< Keeps track of whether start delay applies or not.
};
