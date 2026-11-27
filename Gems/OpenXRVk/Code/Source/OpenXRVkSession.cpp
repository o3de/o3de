/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/Trace.h>
#include <AzCore/Casting/numeric_cast.h>

#include <XR/XRBase.h>

#include <OpenXRVk/OpenXRVkDevice.h>
#include <OpenXRVk/OpenXRVkInput.h>
#include <OpenXRVk/OpenXRVkInstance.h>
#include <OpenXRVk/OpenXRVkSpace.h>
#include <OpenXRVk/OpenXRVkUtils.h>
#include <OpenXRVk/OpenXRVkSession.h>

#include <Atom/RHI.Reflect/Vulkan/XRVkDescriptors.h>
#include <Atom/RPI.Public/XR/XRSpaceNotificationBus.h>

#include "OpenXRVkReferenceSpacesManager.h"
#include "OpenXRVkActionsManager.h"


namespace OpenXRVk
{
    XR::Ptr<Session> Session::Create()
    {
        return aznew Session;
    }

    AZ::RHI::ResultCode Session::InitInternal([[maybe_unused]] AZ::RHI::XRSessionDescriptor* descriptor)
    {
        Instance* xrVkInstance = static_cast<Instance*>(GetDescriptor().m_instance.get());
        Device* xrVkDevice = static_cast<Device*>(GetDescriptor().m_device.get());
        auto graphicBinding = xrVkDevice->GetGraphicsBinding(AZ::RHI::HardwareQueueClass::Graphics);

        m_xrInstance = xrVkInstance->GetXRInstance();
        AZ_Printf("OpenXRVk", "Creating session...\n");
        m_graphicsBinding.instance = xrVkInstance->GetNativeInstance();
        m_graphicsBinding.physicalDevice = xrVkDevice->GetNativePhysicalDevice();
        m_graphicsBinding.device = xrVkDevice->GetNativeDevice();
        m_graphicsBinding.queueFamilyIndex = graphicBinding.m_queueFamilyIndex;
        m_graphicsBinding.queueIndex = graphicBinding.m_queueIndex;

        AZ_Assert(m_xrInstance != XR_NULL_HANDLE, "XR instance is null.");
        AZ_Assert(m_session == XR_NULL_HANDLE, "XR session is already initialized.");

        XrSessionCreateInfo createInfo{ XR_TYPE_SESSION_CREATE_INFO };
        createInfo.next = reinterpret_cast<const XrBaseInStructure*>(&m_graphicsBinding);
        createInfo.systemId = xrVkInstance->GetXRSystemId();
        XrResult result = xrCreateSession(m_xrInstance, &createInfo, &m_session);
        ASSERT_IF_UNSUCCESSFUL(result);

        m_referenceSpacesMgr = AZStd::make_unique<ReferenceSpacesManager>();
        bool success = m_referenceSpacesMgr->Init(m_xrInstance, m_session, xrVkInstance->GetViewConfigType(), xrVkInstance->GetViewCount());
        AZ_Error("OpenXRVk::Session", success, "Failed to instantiate the visualized Spaces manager.");
        
        m_actionsMgr = AZStd::make_unique<ActionsManager>();
        success = m_actionsMgr->Init(m_xrInstance, m_session);
        AZ_Error("OpenXRVk::Session", success, "Failed to instantiate the actions manager");

        LogReferenceSpaces();

        return ConvertResult(result);
    }

    void Session::LogReferenceSpaces()
    {
        if (GetDescriptor().m_validationMode == AZ::RHI::ValidationMode::Enabled)
        {
            AZ_Warning("OpenXrVK", m_session != XR_NULL_HANDLE, "Session is not initialized");

            if (m_session == XR_NULL_HANDLE)
            {
                return;
            }

            uint32_t spaceCount = 0;
            XrResult result = xrEnumerateReferenceSpaces(m_session, 0, &spaceCount, nullptr);
            WARN_IF_UNSUCCESSFUL(result);
            AZStd::vector<XrReferenceSpaceType> spaces(spaceCount);
            result = xrEnumerateReferenceSpaces(m_session, spaceCount, &spaceCount, spaces.data());

            AZ_Printf("OpenXRVk", "Available reference spaces: %d\n", spaceCount);
            for (XrReferenceSpaceType space : spaces)
            {
                AZ_Printf("OpenXRVk", "  Name: %s\n", to_string(space));
            }
        }
    }

    void Session::HandleSessionStateChangedEvent(const XrEventDataSessionStateChanged& stateChangedEvent)
    {
        Instance* xrVkInstance = static_cast<Instance*>(GetDescriptor().m_instance.get());
        const XrSessionState oldState = m_sessionState;
        m_sessionState = stateChangedEvent.state;

        if (GetDescriptor().m_validationMode == AZ::RHI::ValidationMode::Enabled)
        {
            AZ_Printf(
                "OpenXRVk",
                "XrEventDataSessionStateChanged: state %s->%s session=%lld time=%lld\n", to_string(oldState), to_string(m_sessionState),
                    stateChangedEvent.session, stateChangedEvent.time);
        }

        if ((stateChangedEvent.session != XR_NULL_HANDLE) && (stateChangedEvent.session != m_session))
        {
            AZ_Printf("OpenXRVk", "XrEventDataSessionStateChanged for unknown session\n");
            return;
        }

        switch (m_sessionState)
        {
            case XR_SESSION_STATE_READY:
            {
                AZ_Assert(m_session != XR_NULL_HANDLE, "Session is null");
                XrSessionBeginInfo sessionBeginInfo{ XR_TYPE_SESSION_BEGIN_INFO };
                sessionBeginInfo.primaryViewConfigurationType = xrVkInstance->GetViewConfigType();
                XrResult result = xrBeginSession(m_session, &sessionBeginInfo);
                WARN_IF_UNSUCCESSFUL(result);
                m_sessionRunning = true;
                m_referenceSpacesMgr->OnSessionReady();
                break;
            }
            case XR_SESSION_STATE_STOPPING:
            {
                AZ_Assert(m_session != XR_NULL_HANDLE, "Session is null");
                m_sessionRunning = false;
                XrResult result = xrEndSession(m_session);
                WARN_IF_UNSUCCESSFUL(result);
                break;
            }
            case XR_SESSION_STATE_EXITING:
            {
                m_exitRenderLoop = true;
                // Do not attempt to restart because user closed this session.
                m_requestRestart = false;
                break;
            }
            case XR_SESSION_STATE_LOSS_PENDING:
            {
                m_exitRenderLoop = true;
                // Poll for a new instance.
                m_requestRestart = true;
                break;
            }
            default:
            {
                break;
            }
        }
    }

    
    const XrEventDataBaseHeader* Session::TryReadNextEvent()
    {
        XrEventDataBaseHeader* baseHeader = reinterpret_cast<XrEventDataBaseHeader*>(&m_eventDataBuffer);
        *baseHeader = { XR_TYPE_EVENT_DATA_BUFFER };
        const XrResult result = xrPollEvent(m_xrInstance, &m_eventDataBuffer);
        if (result == XR_SUCCESS)
        {
            if (baseHeader->type == XR_TYPE_EVENT_DATA_EVENTS_LOST)
            {
                [[maybe_unused]] const XrEventDataEventsLost* const eventsLost = reinterpret_cast<const XrEventDataEventsLost*>(baseHeader);
                if (GetDescriptor().m_validationMode == AZ::RHI::ValidationMode::Enabled)
                {
                    AZ_Printf("OpenXrVK", "%d events lost\n", eventsLost->lostEventCount);
                }
            }
            return baseHeader;
        }
        if (result == XR_EVENT_UNAVAILABLE)
        {
            return nullptr;
        }
        WARN_IF_UNSUCCESSFUL(result);
        return nullptr;
    }

    void Session::PollEvents()
    {
        m_exitRenderLoop = m_requestRestart = false;

        // Process all pending messages.
        while (const XrEventDataBaseHeader* event = TryReadNextEvent())
        {
            switch (event->type)
            {
                case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
                {
                    if (GetDescriptor().m_validationMode == AZ::RHI::ValidationMode::Enabled)
                    {
                        [[maybe_unused]] const auto& instanceLossPending = *reinterpret_cast<const XrEventDataInstanceLossPending*>(event);
                        AZ_Printf("OpenXRVk", "XrEventDataInstanceLossPending by %lld\n", instanceLossPending.lossTime);
                    }
                    m_exitRenderLoop = true;
                    m_requestRestart = true;
                    return;
                }
                case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
                {
                    auto sessionStateChangedEvent = *reinterpret_cast<const XrEventDataSessionStateChanged*>(event);
                    HandleSessionStateChangedEvent(sessionStateChangedEvent);
                    break;
                }
                case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
                {
                    m_actionsMgr->OnInteractionProfileChanged();
                    break;
                }
                case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING:
                    [[fallthrough]];
                default:
                {
                    if (GetDescriptor().m_validationMode == AZ::RHI::ValidationMode::Enabled)
                    {
                        AZ_Printf("OpenXRVk", "Ignoring event type %d\n", event->type);
                    }
                    break;
                }
            }
        }
    }

    void Session::SetBaseSpaceTypeForVisualization(SpaceType spaceType)
    {
        m_baseSpaceTypeForVisualization = spaceType;
    }

    void Session::SetBaseSpaceTypeForControllers(SpaceType spaceType)
    {
        m_baseSpaceTypeForControllers = spaceType;
    }

    SpaceType Session::GetBaseSpaceTypeForVisualization() const
    {
        return m_baseSpaceTypeForVisualization;
    }

    SpaceType Session::GetBaseSpaceTypeForControllers() const
    {
        return m_baseSpaceTypeForControllers;
    }

    void Session::LogActionSourceName(XrAction action, const AZStd::string_view actionName) const
    {
        XrBoundSourcesForActionEnumerateInfo getInfo = { XR_TYPE_BOUND_SOURCES_FOR_ACTION_ENUMERATE_INFO };
        getInfo.action = action;
        uint32_t pathCount = 0;
        XrResult result = xrEnumerateBoundSourcesForAction(m_session, &getInfo, 0, &pathCount, nullptr);
        WARN_IF_UNSUCCESSFUL(result);
        AZStd::vector<XrPath> paths(pathCount);
        result = xrEnumerateBoundSourcesForAction(m_session, &getInfo, aznumeric_cast<uint32_t>(paths.size()), &pathCount, paths.data());
        WARN_IF_UNSUCCESSFUL(result);

        AZStd::string sourceName;
        for (uint32_t i = 0; i < pathCount; ++i)
        {
            constexpr XrInputSourceLocalizedNameFlags all = XR_INPUT_SOURCE_LOCALIZED_NAME_USER_PATH_BIT |
                XR_INPUT_SOURCE_LOCALIZED_NAME_INTERACTION_PROFILE_BIT | XR_INPUT_SOURCE_LOCALIZED_NAME_COMPONENT_BIT;

            XrInputSourceLocalizedNameGetInfo nameInfo = { XR_TYPE_INPUT_SOURCE_LOCALIZED_NAME_GET_INFO };
            nameInfo.sourcePath = paths[i];
            nameInfo.whichComponents = all;

            uint32_t size = 0;
            result = xrGetInputSourceLocalizedName(m_session, &nameInfo, 0, &size, nullptr);
            WARN_IF_UNSUCCESSFUL(result);
            if (size < 1)
            {
                continue;
            }
            AZStd::vector<char> grabSource(size);
            result = xrGetInputSourceLocalizedName(m_session, &nameInfo, uint32_t(grabSource.size()), &size, grabSource.data());
            WARN_IF_UNSUCCESSFUL(result);
            if (!sourceName.empty())
            {
                sourceName += " and ";
            }
            sourceName += "'";
            sourceName += AZStd::string(grabSource.data(), size - 1);
            sourceName += "'";
        }
        
        AZ_Printf("OpenXrVK",
            "%s action is bound to %s\n", actionName.data(), ((!sourceName.empty()) ? sourceName.c_str() : "nothing"));
    }

    void Session::LocateControllerSpace(AZ::u32 handIndex)
    {
        Input* xrInput = GetNativeInput();
        Device* device = static_cast<Device*>(GetDescriptor().m_device.get());
        Space* space = static_cast<Space*>(GetSpace());
        xrInput->LocateControllerSpace(device->GetPredictedDisplayTime(), space->GetXrSpace(OpenXRVk::SpaceType::View), handIndex);
    }

    AZ::RHI::ResultCode Session::GetControllerPose(AZ::u32 handIndex, AZ::RPI::PoseData& outPoseData) const
    {
        return GetNativeInput()->GetControllerPose(handIndex, outPoseData);
    }

    AZ::RHI::ResultCode Session::GetControllerTransform(AZ::u32 handIndex, AZ::Transform& outTransform) const
    {
        return GetNativeInput()->GetControllerTransform(handIndex, outTransform);
    }
    
    AZ::RHI::ResultCode Session::GetControllerStagePose(AZ::u32 handIndex, AZ::RPI::PoseData& outPoseData) const
    {
        Input* xrInput = GetNativeInput();
        return handIndex == 0 ? xrInput->GetVisualizedSpacePose(OpenXRVk::SpaceType::StageLeft, outPoseData) :
            xrInput->GetVisualizedSpacePose(OpenXRVk::SpaceType::StageRight, outPoseData);
    }

    AZ::RHI::ResultCode Session::GetViewFrontPose(AZ::RPI::PoseData& outPoseData) const
    {
        return GetNativeInput()->GetVisualizedSpacePose(OpenXRVk::SpaceType::ViewFront, outPoseData);
    }

    AZ::RHI::ResultCode Session::GetViewLocalPose(AZ::RPI::PoseData& outPoseData) const
    {
        return GetNativeInput()->GetVisualizedSpacePose(OpenXRVk::SpaceType::View, outPoseData);
    }

    float Session::GetControllerScale(AZ::u32 handIndex) const
    {
        return GetNativeInput()->GetControllerScale(handIndex);
    }

    float Session::GetSqueezeState(AZ::u32 handIndex) const
    {
        return GetNativeInput()->GetSqueezeState(handIndex);
    }

    float Session::GetTriggerState(AZ::u32 handIndex) const
    {
        return GetNativeInput()->GetTriggerState(handIndex);
    }

    float Session::GetXButtonState() const
    {
        return (GetNativeInput()->GetXButtonState() ? 1.f : 0.f);
    }

    float Session::GetYButtonState() const
    {
        return (GetNativeInput()->GetYButtonState() ? 1.f : 0.f);
    }

    float Session::GetAButtonState() const
    {
        return (GetNativeInput()->GetAButtonState() ? 1.f : 0.f);
    }

    float Session::GetBButtonState() const
    {
        return (GetNativeInput()->GetBButtonState() ? 1.f : 0.f);
    }

    float Session::GetXJoyStickState(AZ::u32 handIndex) const
    {
        return GetNativeInput()->GetXJoyStickState(handIndex);
    }
    
    float Session::GetYJoyStickState(AZ::u32 handIndex) const
    {
        return GetNativeInput()->GetYJoyStickState(handIndex);
    }

    XrSession Session::GetXrSession() const
    {
        return m_session;
    }

    XrSpace Session::GetXrSpace(SpaceType spaceType) const
    {
        Space* space = static_cast<Space*>(GetSpace());
        return space->GetXrSpace(spaceType);
    }

    const AZStd::vector<XrView>& Session::GetXrViews() const
    {
        return m_referenceSpacesMgr->GetXrViews();
    }

    XrSpace Session::GetViewSpaceXrSpace() const
    {
        return m_referenceSpacesMgr->GetViewSpaceXrSpace();
    }

    bool Session::IsSessionRunning() const
    {
        return m_sessionRunning;
    }

    bool Session::IsSessionFocused() const
    {
        return m_sessionState == XR_SESSION_STATE_FOCUSED;
    }

    bool Session::IsRestartRequested() const
    {
        return m_requestRestart;
    }

    bool Session::IsExitRenderLoopRequested() const
    {
        return m_exitRenderLoop;
    }

    void Session::ShutdownInternal()
    {
        if (m_session != XR_NULL_HANDLE) 
        {
            xrDestroySession(m_session);
        }
    }

    Input* Session::GetNativeInput() const
    {
        return static_cast<Input*>(GetInput());
    }

    void Session::OnBeginFrame([[maybe_unused]] XrTime predictedDisplayTime)
    {
        if (IsSessionFocused())
        {
            // Syncing actions only works if the session is in focused state
            m_actionsMgr->SyncActions(predictedDisplayTime);
        }

        m_referenceSpacesMgr->SyncViews(predictedDisplayTime);
        
        //Notify the rest of the engine.
        const auto& viewPoses = m_referenceSpacesMgr->GetViewPoses();
        AZ::RPI::XRSpaceNotificationBus::Broadcast(&AZ::RPI::XRSpaceNotifications::OnXRSpaceLocationsChanged,
            m_referenceSpacesMgr->GetViewSpacePose(),
            viewPoses[0], viewPoses[1]);
    }
}
