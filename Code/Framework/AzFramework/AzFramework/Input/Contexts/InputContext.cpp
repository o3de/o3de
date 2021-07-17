/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Input/Contexts/InputContext.h>
#include <AzFramework/Input/Mappings/InputMapping.h>

#include <AzCore/Debug/Trace.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputContext::InputContext(const char* name)
        : InputDevice(InputDeviceId(name))
        , InputChannelEventListener()
        , m_inputChannelsById()
        , m_inputMappingsById()
        , m_consumesProcessedInput(false)
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputContext::InputContext(const char* name, const InitData& initData)
        : InputDevice(InputDeviceId(name))
        , InputChannelEventListener(initData.filter, initData.priority, initData.autoActivate)
        , m_inputChannelsById()
        , m_inputMappingsById()
        , m_consumesProcessedInput(initData.consumesProcessedInput)
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputContext::~InputContext()
    {
        // We have to reset the state of all the input mappings owned by this input context.
        for (auto& inputMappingById : m_inputMappingsById)
        {
            inputMappingById.second->ResetState();
        }
        Deactivate();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputContext::Activate()
    {
        InputChannelEventListener::Connect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputContext::Deactivate()
    {
        InputChannelEventListener::Disconnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputContext::AddInputMapping(AZStd::shared_ptr<InputMapping> inputMapping)
    {
        if (!inputMapping)
        {
            AZ_Warning("InputContext", false, "Cannot add a null input mapping");
            return false;
        }
        
        const InputChannelId& inputMappingId = inputMapping->GetInputChannelId();
        if (&inputMapping->GetInputDevice() != this)
        {
            AZ_Warning("InputContext", false,
                       "Input context (%s) is not the parent of input mapping with id: %s, cannot add",
                       GetInputDeviceId().GetName(), inputMappingId.GetName());
            return false;
        }

        const auto it = m_inputMappingsById.find(inputMappingId);
        if (it != m_inputMappingsById.end())
        {
            AZ_Warning("InputContext", false,
                       "Input context (%s) already contains an input mapping with id: %s, cannot add",
                       GetInputDeviceId().GetName(), inputMappingId.GetName());
            return false;
        }

        m_inputMappingsById[inputMappingId] = AZStd::move(inputMapping);
        m_inputChannelsById[inputMappingId] = inputMapping.get();
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputContext::RemoveInputMapping(const InputChannelId& inputMappingId)
    {
        const auto mappingIt = m_inputMappingsById.find(inputMappingId);
        if (mappingIt == m_inputMappingsById.end())
        {
            AZ_Warning("InputContext", false,
                       "Input context (%s) does not contain an input mapping with id: %s, cannot remove",
                       GetInputDeviceId().GetName(), inputMappingId.GetName());
            return false;
        }

        const auto channelIt = m_inputChannelsById.find(inputMappingId);
        AZ_Assert(channelIt != m_inputChannelsById.end(),
                  "InputContext (%s) contains an InputMapping (%s) that was found in m_inputMappingsById but not m_inputChannelsById",
                  GetInputDeviceId().GetName(), inputMappingId.GetName());
        m_inputChannelsById.erase(channelIt);
        m_inputMappingsById.erase(mappingIt);
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const InputDevice::InputChannelByIdMap& InputContext::GetInputChannelsById() const
    {
        return m_inputChannelsById;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputContext::IsSupported() const
    {
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputContext::IsConnected() const
    {
        return InputChannelEventListener::BusIsConnected();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputContext::TickInputDevice()
    {
        for (auto& inputMappingById : m_inputMappingsById)
        {
            inputMappingById.second->OnTick();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputContext::OnInputChannelEventFiltered(const InputChannel& inputChannel)
    {
        for (auto& inputMappingById : m_inputMappingsById)
        {
            if (inputMappingById.second->ProcessPotentialSourceInputEvent(inputChannel))
            {
                return m_consumesProcessedInput;
            }
        }

        return false;
    }
} // namespace AzFramework
