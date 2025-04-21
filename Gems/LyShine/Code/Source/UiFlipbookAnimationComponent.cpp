/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiFlipbookAnimationComponent.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <LyShine/Bus/UiGameEntityContextBus.h>
#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiImageBus.h>
#include <LyShine/Bus/UiIndexableImageBus.h>
#include <LyShine/UiSerializeHelpers.h>

namespace
{
    const char* notConfiguredMessage = "<Spritesheet/image index unavailable>";

    //! Renames the float field "Frame Delay" to "Framerate" (as of V3).
    bool ConvertFrameDelayToFramerate(
        AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement)
    {
        int index = classElement.FindElement(AZ_CRC_CE("Frame Delay"));
        if (index != -1)
        {
            AZ::SerializeContext::DataElementNode& frameDelayNode = classElement.GetSubElement(index);
            float frameDelayValue = 0;

            if (!frameDelayNode.GetData<float>(frameDelayValue))
            {
                AZ_Error("Serialization", false, "Element Frame Delay is not a float.");
                return false;
            }

            // remove the FrameDelay node
            classElement.RemoveElement(index);

            // If Framerate doesn't exist yet, add it
            index = classElement.FindElement(AZ_CRC_CE("Framerate"));
            if (index == -1)
            {
                index = classElement.AddElement<float>(context, "Framerate");

                if (index == -1)
                {
                    // Error adding the new sub element
                    AZ_Error("Serialization", false, "Failed to create Framerate node");
                    return false;
                }
            }

            // Finally, set the framerate to be the same value as the frame delay
            AZ::SerializeContext::DataElementNode& framerateNode = classElement.GetSubElement(index);
            if (!framerateNode.SetData<float>(context, frameDelayValue))
            {
                AZ_Error("Serialization", false, "Unable to set Framerate to legacy Frame Delay value (%.2f).", frameDelayValue);
                return false;
            }
        }

        return true;
    }

    //! Convert legacy components to use seconds-per-frame as default time unit for playback.
    //!
    //! Prior to V3, default unit of time for playback was seconds-per-frame.
    bool ConvertFramerateUnitToSeconds(
        AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement)
    {
        // If Framerate Unit doesn't exist yet, add it
        int index = classElement.FindElement(AZ_CRC_CE("Framerate Unit"));
        if (index == -1)
        {
            index = classElement.AddElement<int>(context, "Framerate Unit");

            if (index == -1)
            {
                // Error adding the new sub element
                AZ_Error("Serialization", false, "Failed to create Framerate Unit node");
                return false;
            }
        }

        // Set the framerate unit to seconds for legacy reasons (FPS is default for newer versions of this component)
        AZ::SerializeContext::DataElementNode& framerateUnitNode = classElement.GetSubElement(index);
        const int secondsEnumVal = static_cast<int>(UiFlipbookAnimationInterface::FramerateUnits::SecondsPerFrame);
        if (!framerateUnitNode.SetData<int>(context, secondsEnumVal))
        {
            AZ_Error("Serialization", false, "Unable to set Framerate Unit to seconds (%d).", secondsEnumVal);
            return false;
        }

        return true;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Forwards events to Lua for UiFlipbookAnimationNotificationsBus
class UiFlipbookAnimationNotificationsBusBehaviorHandler
    : public UiFlipbookAnimationNotificationsBus::Handler
    , public AZ::BehaviorEBusHandler
{
public:
    AZ_EBUS_BEHAVIOR_BINDER(UiFlipbookAnimationNotificationsBusBehaviorHandler, "{0A92A44E-0C32-4AD6-9C49-222A484B54FF}", AZ::SystemAllocator,
        OnAnimationStarted, OnAnimationStopped, OnLoopSequenceCompleted);

    void OnAnimationStarted() override
    {
        Call(FN_OnAnimationStarted);
    }

    void OnAnimationStopped() override
    {
        Call(FN_OnAnimationStopped);
    }

    void OnLoopSequenceCompleted() override
    {
        Call(FN_OnLoopSequenceCompleted);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
static bool UiFlipbookAnimationComponentVersionConverter(AZ::SerializeContext& context,
    AZ::SerializeContext::DataElementNode& classElement)
{
    // conversion from version 2:
    // - Rename "frame delay" to "framerate"
    // - Set "framerate unit" to seconds (default moving forward is FPS, but we use seconds for legacy compatibility)
    if (classElement.GetVersion() <= 2)
    {
        if (!ConvertFrameDelayToFramerate(context, classElement))
        {
            return false;
        }

        if (!ConvertFramerateUnitToSeconds(context, classElement))
        {
            return false;
        }
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void UiFlipbookAnimationComponent::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
    if (serializeContext)
    {
        serializeContext->Class<UiFlipbookAnimationComponent, AZ::Component>()
            ->Version(3, &UiFlipbookAnimationComponentVersionConverter)
            ->Field("Start Frame", &UiFlipbookAnimationComponent::m_startFrame)
            ->Field("End Frame", &UiFlipbookAnimationComponent::m_endFrame)
            ->Field("Loop Start Frame", &UiFlipbookAnimationComponent::m_loopStartFrame)
            ->Field("Loop Type", &UiFlipbookAnimationComponent::m_loopType)
            ->Field("Framerate Unit", &UiFlipbookAnimationComponent::m_framerateUnit)
            ->Field("Framerate", &UiFlipbookAnimationComponent::m_framerate)
            ->Field("Start Delay", &UiFlipbookAnimationComponent::m_startDelay)
            ->Field("Loop Delay", &UiFlipbookAnimationComponent::m_loopDelay)
            ->Field("Reverse Delay", &UiFlipbookAnimationComponent::m_reverseDelay)
            ->Field("Auto Play", &UiFlipbookAnimationComponent::m_isAutoPlay)
        ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (editContext)
        {
            auto editInfo = editContext->Class<UiFlipbookAnimationComponent>("FlipbookAnimation",
                    "Animates image sequences or images configured as sprite sheets.");

            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Category, "UI")
                ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Flipbook.png")
                ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Flipbook.svg")
                ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("UI"))
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
            ;

            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiFlipbookAnimationComponent::m_startFrame, "Start frame", "Frame to start at")
                ->Attribute("EnumValues", &UiFlipbookAnimationComponent::PopulateIndexStringList)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiFlipbookAnimationComponent::OnStartFrameChange)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC_CE("RefreshEntireTree"));
            ;

            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiFlipbookAnimationComponent::m_endFrame, "End frame", "Frame to end at")
                ->Attribute("EnumValues", &UiFlipbookAnimationComponent::PopulateIndexStringList)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiFlipbookAnimationComponent::OnEndFrameChange)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC_CE("RefreshEntireTree"));
            ;

            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiFlipbookAnimationComponent::m_loopStartFrame, "Loop start frame", "Frame to start looping from")
                ->Attribute("EnumValues", &UiFlipbookAnimationComponent::PopulateConstrainedIndexStringList)
            ;

            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiFlipbookAnimationComponent::m_loopType, "Loop type", "Go from start to end continuously or start to end and back to start")
                ->EnumAttribute(UiFlipbookAnimationInterface::LoopType::None, "None")
                ->EnumAttribute(UiFlipbookAnimationInterface::LoopType::Linear, "Linear")
                ->EnumAttribute(UiFlipbookAnimationInterface::LoopType::PingPong, "PingPong")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC_CE("RefreshEntireTree"))
            ;

            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiFlipbookAnimationComponent::m_framerateUnit, "Framerate unit", "Unit of measurement for framerate")
                ->EnumAttribute(UiFlipbookAnimationInterface::FramerateUnits::FPS, "FPS")
                ->EnumAttribute(UiFlipbookAnimationInterface::FramerateUnits::SecondsPerFrame, "Seconds Per Frame")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiFlipbookAnimationComponent::OnFramerateUnitChange)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC_CE("RefreshEntireTree"))
                ;

            editInfo->DataElement(0, &UiFlipbookAnimationComponent::m_framerate, "Framerate", "Determines transition speed between frames")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Max, AZ::Constants::FloatMax)
                ;

            editInfo->DataElement(0, &UiFlipbookAnimationComponent::m_startDelay, "Start delay", "Number of seconds to wait before playing the flipbook (applied only once).")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Max, AZ::Constants::FloatMax)
                ;

            editInfo->DataElement(0, &UiFlipbookAnimationComponent::m_loopDelay, "Loop delay", "Number of seconds to delay until the loop sequence plays")
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiFlipbookAnimationComponent::IsLoopingType)
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Max, AZ::Constants::FloatMax)
                ;

            editInfo->DataElement(0, &UiFlipbookAnimationComponent::m_reverseDelay, "Reverse delay", "Number of seconds to delay until the reverse sequence plays (PingPong loop types only)")
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiFlipbookAnimationComponent::IsPingPongLoopType)
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Max, AZ::Constants::FloatMax)
                ;

            editInfo->DataElement(0, &UiFlipbookAnimationComponent::m_isAutoPlay, "Auto Play", "Automatically starts playing the animation")
            ;
        }
    }

    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
    if (behaviorContext)
    {
        behaviorContext->EBus<UiFlipbookAnimationBus>("UiFlipbookAnimationBus")
            ->Event("Start", &UiFlipbookAnimationBus::Events::Start)
            ->Event("Stop", &UiFlipbookAnimationBus::Events::Stop)
            ->Event("IsPlaying", &UiFlipbookAnimationBus::Events::IsPlaying)
            ->Event("GetStartFrame", &UiFlipbookAnimationBus::Events::GetStartFrame)
            ->Event("SetStartFrame", &UiFlipbookAnimationBus::Events::SetStartFrame)
            ->Event("GetEndFrame", &UiFlipbookAnimationBus::Events::GetEndFrame)
            ->Event("SetEndFrame", &UiFlipbookAnimationBus::Events::SetEndFrame)
            ->Event("GetCurrentFrame", &UiFlipbookAnimationBus::Events::GetCurrentFrame)
            ->Event("SetCurrentFrame", &UiFlipbookAnimationBus::Events::SetCurrentFrame)
            ->Event("GetLoopStartFrame", &UiFlipbookAnimationBus::Events::GetLoopStartFrame)
            ->Event("SetLoopStartFrame", &UiFlipbookAnimationBus::Events::SetLoopStartFrame)
            ->Event("GetLoopType", &UiFlipbookAnimationBus::Events::GetLoopType)
            ->Event("SetLoopType", &UiFlipbookAnimationBus::Events::SetLoopType)
            ->Event("GetFramerate", &UiFlipbookAnimationBus::Events::GetFramerate)
            ->Event("SetFramerate", &UiFlipbookAnimationBus::Events::SetFramerate)
            ->Event("GetFramerateUnit", &UiFlipbookAnimationBus::Events::GetFramerateUnit)
            ->Event("SetFramerateUnit", &UiFlipbookAnimationBus::Events::SetFramerateUnit)
            ->Event("GetStartDelay", &UiFlipbookAnimationBus::Events::GetStartDelay)
            ->Event("SetStartDelay", &UiFlipbookAnimationBus::Events::SetStartDelay)
            ->Event("GetLoopDelay", &UiFlipbookAnimationBus::Events::GetLoopDelay)
            ->Event("SetLoopDelay", &UiFlipbookAnimationBus::Events::SetLoopDelay)
            ->Event("GetReverseDelay", &UiFlipbookAnimationBus::Events::GetReverseDelay)
            ->Event("SetReverseDelay", &UiFlipbookAnimationBus::Events::SetReverseDelay)
            ->Event("GetIsAutoPlay", &UiFlipbookAnimationBus::Events::GetIsAutoPlay)
            ->Event("SetIsAutoPlay", &UiFlipbookAnimationBus::Events::SetIsAutoPlay)
        ;

        behaviorContext->EBus<UiFlipbookAnimationNotificationsBus>("UiFlipbookAnimationNotificationsBus")
            ->Handler<UiFlipbookAnimationNotificationsBusBehaviorHandler>()
            ;

        behaviorContext->Enum<(int)UiFlipbookAnimationInterface::LoopType::None>("eUiFlipbookAnimationLoopType_None")
            ->Enum<(int)UiFlipbookAnimationInterface::LoopType::Linear>("eUiFlipbookAnimationLoopType_Linear")
            ->Enum<(int)UiFlipbookAnimationInterface::LoopType::PingPong>("eUiFlipbookAnimationLoopType_PingPong")
            ;

        behaviorContext->Enum<(int)UiFlipbookAnimationInterface::FramerateUnits::FPS>("eUiFlipbookAnimationFramerateUnits_FPS")
            ->Enum<(int)UiFlipbookAnimationInterface::FramerateUnits::SecondsPerFrame>("eUiFlipbookAnimationFramerateUnits_SecondsPerFrame")
            ;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::u32 UiFlipbookAnimationComponent::GetMaxFrame() const
{
    AZ::u32 numImageIndices = 0;

    UiIndexableImageBus::EventResult(numImageIndices, GetEntityId(), &UiIndexableImageBus::Events::GetImageIndexCount);

    return numImageIndices;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiFlipbookAnimationComponent::FrameWithinRange(AZ::u32 frameValue)
{
    AZ::u32 maxFrame = GetMaxFrame();
    return maxFrame > 0 && frameValue < maxFrame;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
LyShine::AZu32ComboBoxVec UiFlipbookAnimationComponent::PopulateIndexStringList() const
{
    AZ::u32 numFrames = GetMaxFrame();
    if (numFrames > 0)
    {
        return LyShine::GetEnumSpriteIndexList(GetEntityId(), 0, numFrames - 1);
    }
    
    // Add an empty element to prevent an AzToolsFramework warning that fires
    // when an empty container is encountered.
    LyShine::AZu32ComboBoxVec comboBoxVec;
    comboBoxVec.push_back(AZStd::make_pair(0, notConfiguredMessage));

    return comboBoxVec;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
LyShine::AZu32ComboBoxVec UiFlipbookAnimationComponent::PopulateConstrainedIndexStringList() const
{
    const char* errorMessage = notConfiguredMessage;

    AZ::u32 indexCount = GetMaxFrame();
    const bool isIndexedImage = indexCount > 1;
    if (isIndexedImage)
    {
        errorMessage = "<Invalid loop range>";
    }

    return LyShine::GetEnumSpriteIndexList(GetEntityId(), m_startFrame, m_endFrame, errorMessage);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFlipbookAnimationComponent::OnStartFrameChange()
{
    m_endFrame = AZ::GetMax<AZ::u32>(m_startFrame, m_endFrame);
    m_currentFrame = AZ::GetClamp<AZ::u32>(m_currentFrame, m_startFrame, m_endFrame);
    m_loopStartFrame = AZ::GetClamp<AZ::u32>(m_loopStartFrame, m_startFrame, m_endFrame);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFlipbookAnimationComponent::OnEndFrameChange()
{
    m_startFrame = AZ::GetMin<AZ::u32>(m_startFrame, m_endFrame);
    m_currentFrame = AZ::GetClamp<AZ::u32>(m_currentFrame, m_startFrame, m_endFrame);
    m_loopStartFrame = AZ::GetClamp<AZ::u32>(m_loopStartFrame, m_startFrame, m_endFrame);
}

void UiFlipbookAnimationComponent::OnFramerateUnitChange()
{
    AZ_Assert(m_framerateUnit == FramerateUnits::FPS || m_framerateUnit == FramerateUnits::SecondsPerFrame,
        "New framerate unit added for flipbooks - please update this function accordingly!");
    m_framerate = m_framerate != 0.0f ? 1.0f / m_framerate : 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiFlipbookAnimationComponent::IsPingPongLoopType() const
{
    return m_loopType == LoopType::PingPong;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiFlipbookAnimationComponent::IsLoopingType() const
{
    return m_loopType != LoopType::None;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiFlipbookAnimationComponent::CalculateLoopDelay() const
{
    float loopDelay = 0.0f;
    if (IsLoopingType())
    {
        const bool isStartFrame = m_currentFrame == m_loopStartFrame;
        const bool playingIntro = m_prevFrame < m_currentFrame && m_startFrame != m_loopStartFrame;
        const bool shouldApplyStartLoopDelay = isStartFrame && !playingIntro;

        if (shouldApplyStartLoopDelay)
        {
            loopDelay = m_loopDelay;
        }
        else if (m_loopType == LoopType::PingPong)
        {
            const bool isEndFrame = m_currentFrame == m_endFrame;
            const bool isPlayingReverse = m_currentLoopDirection < 0;
            const bool shouldApplyReverseDelay = isEndFrame && isPlayingReverse;

            if (shouldApplyReverseDelay)
            {
                loopDelay = m_reverseDelay;
            }
        }
    }

    return loopDelay;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFlipbookAnimationComponent::Activate()
{
    UiFlipbookAnimationBus::Handler::BusConnect(GetEntityId());
    UiInitializationBus::Handler::BusConnect(GetEntityId());
    UiSpriteSourceNotificationBus::Handler::BusConnect(GetEntityId());

    if (m_isPlaying)
    {
        // this is unlikely but possible. To get here a client would have to start the flipbook
        // playing and then deactivate and reactivate (e.g. add a component).
        AZ::EntityId canvasEntityId;
        UiElementBus::EventResult(canvasEntityId, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);
        if (canvasEntityId.IsValid())
        {
            UiCanvasUpdateNotificationBus::Handler::BusConnect(canvasEntityId);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFlipbookAnimationComponent::Deactivate()
{
    UiFlipbookAnimationBus::Handler::BusDisconnect();
    UiInitializationBus::Handler::BusDisconnect();
    UiCanvasUpdateNotificationBus::Handler::BusDisconnect();
    UiSpriteSourceNotificationBus::Handler::BusDisconnect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFlipbookAnimationComponent::Update(float deltaTime)
{
    if (m_isPlaying)
    {
        m_elapsedTime += deltaTime;

        if (m_useStartDelay)
        {
            if (m_elapsedTime >= m_startDelay)
            {
                m_useStartDelay = false;
                m_elapsedTime = 0.0f;
                UiIndexableImageBus::Event(GetEntityId(), &UiIndexableImageBus::Events::SetImageIndex, m_currentFrame);
            }
            return;
        }

        const float loopDelay = CalculateLoopDelay();

        // Calculate the frame delay (time to transition to next frame) based on framerate.
        // If framerate is in FPS we convert to seconds-per-frame to test against elapsedTime.
        const float frameDelay = CalculateFramerateAsSecondsPerFrame();

        if (m_elapsedTime >= (frameDelay + loopDelay))
        {
            // Determine the number of frames that has elapsed and adjust 
            // "elapsed time" to account for any additional time that has
            // passed given the current delta.
            const float elapsedTimeAfterDelayFrame = m_elapsedTime - (frameDelay + loopDelay);
            const AZ::s32 numFramesElapsed = static_cast<AZ::s32>(1 + (elapsedTimeAfterDelayFrame / frameDelay));
            m_elapsedTime = m_elapsedTime - ((numFramesElapsed * frameDelay) + loopDelay);

            // In case the loop direction is negative, we don't want to
            // subtract from the current frame if its zero.
            m_prevFrame = m_currentFrame;
            const AZ::s32 nextFrameNum = AZ::GetMax<AZ::s32>(0, static_cast<AZ::s32>(m_currentFrame) + numFramesElapsed * m_currentLoopDirection);
            m_currentFrame = static_cast<AZ::u32>(nextFrameNum);

            switch (m_loopType)
            {
            case LoopType::None:
                if (m_currentFrame > m_endFrame)
                {
                    m_currentFrame = m_endFrame;
                    Stop();
                }
                break;
            case LoopType::Linear:
                if (m_currentFrame > m_endFrame)
                {
                    m_currentFrame = m_loopStartFrame;
                    UiFlipbookAnimationNotificationsBus::Event(
                        GetEntityId(), &UiFlipbookAnimationNotificationsBus::Events::OnLoopSequenceCompleted);
                }
                break;
            case LoopType::PingPong:
                if (m_currentLoopDirection > 0 && m_currentFrame >= m_endFrame)
                {
                    m_currentLoopDirection = -1;
                    m_currentFrame = m_endFrame;
                    UiFlipbookAnimationNotificationsBus::Event(
                        GetEntityId(), &UiFlipbookAnimationNotificationsBus::Events::OnLoopSequenceCompleted);
                }
                else if (m_currentLoopDirection < 0 && m_currentFrame <= m_loopStartFrame)
                {
                    m_currentLoopDirection = 1;
                    m_currentFrame = m_loopStartFrame;
                    UiFlipbookAnimationNotificationsBus::Event(
                        GetEntityId(), &UiFlipbookAnimationNotificationsBus::Events::OnLoopSequenceCompleted);
                }
                break;
            default:
                break;
            }

            // Show current frame
            UiIndexableImageBus::Event(GetEntityId(), &UiIndexableImageBus::Events::SetImageIndex, m_currentFrame);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFlipbookAnimationComponent::InGamePostActivate()
{
    if (m_isPlaying)
    {
        // Could get here if Start was called from Lua in the OnActivate function
        if (!UiCanvasUpdateNotificationBus::Handler::BusIsConnected())
        {
            AZ::EntityId canvasEntityId;
            UiElementBus::EventResult(canvasEntityId, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);
            if (canvasEntityId.IsValid())
            {
                UiCanvasUpdateNotificationBus::Handler::BusConnect(canvasEntityId);
            }
        }
    }
    else if (m_isAutoPlay)
    {
        Start();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFlipbookAnimationComponent::Start()
{
    m_currentFrame = m_startFrame;
    m_currentLoopDirection = 1;
    m_isPlaying = true;
    m_elapsedTime = 0.0f;
    m_useStartDelay = m_startDelay > 0.0f ? true : false;

    // Show current frame
    if (!m_useStartDelay)
    {
        UiIndexableImageBus::Event(GetEntityId(), &UiIndexableImageBus::Events::SetImageIndex, m_currentFrame);
    }

    // Start the update loop
    if (!UiCanvasUpdateNotificationBus::Handler::BusIsConnected())
    {
        AZ::EntityId canvasEntityId;
        UiElementBus::EventResult(canvasEntityId, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);
        // if this element has not been fixed up yet then canvasEntityId will be invalid. We handle this
        // in InGamePostActivate
        if (canvasEntityId.IsValid())
        {
            UiCanvasUpdateNotificationBus::Handler::BusConnect(canvasEntityId);
        }
    }

    // Let listeners know that we started playing
    UiFlipbookAnimationNotificationsBus::Event(GetEntityId(), &UiFlipbookAnimationNotificationsBus::Events::OnAnimationStarted);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFlipbookAnimationComponent::Stop()
{
    m_isPlaying = false;
    UiCanvasUpdateNotificationBus::Handler::BusDisconnect();

    UiFlipbookAnimationNotificationsBus::Event(GetEntityId(), &UiFlipbookAnimationNotificationsBus::Events::OnAnimationStopped);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFlipbookAnimationComponent::SetStartFrame(AZ::u32 startFrame)
{
    if (!FrameWithinRange(startFrame))
    {
        AZ_Warning("UI", false, "Invalid frame value given: %u", startFrame);
        return;
    }

    m_startFrame = startFrame;
    OnStartFrameChange();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFlipbookAnimationComponent::SetEndFrame(AZ::u32 endFrame)
{
    if (!FrameWithinRange(endFrame))
    {
        AZ_Warning("UI", false, "Invalid frame value given: %u", endFrame);
        return;
    }

    m_endFrame = endFrame;
    OnEndFrameChange();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFlipbookAnimationComponent::SetCurrentFrame(AZ::u32 currentFrame)
{
    // The current frame needs to stay between the start and end frames
    const bool validFrameValue = currentFrame >= m_startFrame && currentFrame <= m_endFrame;
    if (!validFrameValue)
    {
        AZ_Warning("UI", false, "Invalid frame value given: %u", currentFrame);
        return;
    }

    m_currentFrame = currentFrame;
    UiIndexableImageBus::Event(GetEntityId(), &UiIndexableImageBus::Events::SetImageIndex, m_currentFrame);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFlipbookAnimationComponent::SetLoopStartFrame(AZ::u32 loopStartFrame)
{
    // Ensure that loop start frame exists within start and end frame range
    const bool validFrameValue = loopStartFrame >= m_startFrame && loopStartFrame <= m_endFrame;
    if (!validFrameValue)
    {
        AZ_Warning("UI", false, "Invalid frame value given: %u", loopStartFrame);
        return;
    }

    m_loopStartFrame = loopStartFrame;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFlipbookAnimationComponent::SetLoopType(UiFlipbookAnimationInterface::LoopType loopType)
{
    m_loopType = loopType;

    // PingPong is currently the only loop type that supports a negative loop
    // direction.
    if (m_loopType != LoopType::PingPong)
    {
        m_currentLoopDirection = 1;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFlipbookAnimationComponent::OnSpriteSourceChanged()
{
    AZ::u32 indexCount = GetMaxFrame();

    const AZ::u32 newStartFrame = AZ::GetClamp<AZ::u32>(m_startFrame, 0, indexCount - 1);
    const AZ::u32 newEndFrame = AZ::GetClamp<AZ::u32>(m_endFrame, 0, indexCount - 1);
    const bool frameRangesChanged = newStartFrame != m_startFrame || newEndFrame != m_endFrame;
    if (frameRangesChanged)
    {
        m_startFrame = newStartFrame;
        m_endFrame = newEndFrame;
        OnStartFrameChange();
        OnEndFrameChange();
    }
}
