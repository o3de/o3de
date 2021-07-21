/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Input/Mappings/InputMappingAnd.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputMappingAnd::InputMappingAnd(const InputChannelId& inputChannelId, const InputContext& inputContext)
        : InputMapping(inputChannelId, inputContext)
        , m_sourceInputChannelIds()
        , m_activeInputChannelIds()
        , m_averageValue(0.0f)
        , m_averageDelta(0.0f)
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputMappingAnd::AddSourceInput(const InputChannelId& inputSourceId)
    {
        const auto it = m_sourceInputChannelIds.find(inputSourceId);
        if (it != m_sourceInputChannelIds.end())
        {
            AZ_Warning("InputMappingAnd", false,
                       "Input mapping (%s) already contains an input source with id: %s, cannot add",
                       GetInputChannelId().GetName(), inputSourceId.GetName());
            return false;
        }

        m_sourceInputChannelIds.insert(inputSourceId);
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputMappingAnd::RemoveSourceInput(const InputChannelId& inputSourceId)
    {
        const auto it = m_sourceInputChannelIds.find(inputSourceId);
        if (it == m_sourceInputChannelIds.end())
        {
            AZ_Warning("InputMappingAnd", false,
                       "Input mapping (%s) does not contain an input source with id: %s, cannot remove",
                       GetInputChannelId().GetName(), inputSourceId.GetName());
            return false;
        }

        m_sourceInputChannelIds.erase(it);
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    float InputMappingAnd::GetValue() const
    {
        return m_averageValue;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    float InputMappingAnd::GetDelta() const
    {
        return m_averageDelta;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputMappingAnd::ResetState()
    {
        m_averageValue = 0.0f;
        m_averageDelta = 0.0f;
        m_activeInputChannelIds.clear();
        InputChannel::ResetState();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputMappingAnd::IsSourceInput(const InputChannel& inputChannel)
    {
        const InputChannelId& inputChannelId = inputChannel.GetInputChannelId();
        for (const InputChannelId& sourceInputChannelId : m_sourceInputChannelIds)
        {
            if (inputChannelId == sourceInputChannelId)
            {
                return true;
            }
        }

        return false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputMappingAnd::OnSourceInputEvent(const InputChannel& inputChannel)
    {
        // It's possible for multiple device instances of the same type to exist at the same time,
        // so we need to store the unique device id (which accounts for the device index) for all
        // active sources, and ignore input from any device of the same type but different index.
        const InputDeviceId& inputDeviceId = inputChannel.GetInputDevice().GetInputDeviceId();
        for (const auto& activeInputChannelId : m_activeInputChannelIds)
        {
            const InputDeviceId& activeInputDeviceId = activeInputChannelId.second;
            if (inputDeviceId.GetNameCrc32() == activeInputDeviceId.GetNameCrc32() &&
                inputDeviceId.GetIndex() != activeInputDeviceId.GetIndex())
            {
                return false;
            }
        }

        // Add or remove the input source from the container tracking active input sources.
        if (inputChannel.IsActive())
        {
            // This entry might already exist in the container, in which case this does nothing.
            m_activeInputChannelIds.insert({ inputChannel.GetInputChannelId(), inputDeviceId });
        }
        else
        {
            m_activeInputChannelIds.erase(inputChannel.GetInputChannelId());
        }

        // Determine whether all sources are active, then store the average value and average delta.
        const bool isActive = m_activeInputChannelIds.size() == m_sourceInputChannelIds.size();
        const float newAverageValue = isActive ? CalculateAverageValue() : 0.0f;
        m_averageDelta = newAverageValue - m_averageValue;
        m_averageValue = newAverageValue;

        // Update the state of this mapping/channel and return.
        UpdateState(isActive);
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    float InputMappingAnd::CalculateAverageValue() const
    {
        float sumOfValues = 0.0f;
        for (const auto& activeInputChannelId : m_activeInputChannelIds)
        {
            const InputChannel* activeInputChannel = InputChannelRequests::FindInputChannel(activeInputChannelId.first,
                                                                                            activeInputChannelId.second.GetIndex());
            if (activeInputChannel)
            {
                sumOfValues += activeInputChannel->GetValue();
            }
        }

        const float numValues = aznumeric_cast<float>(m_activeInputChannelIds.size());
        return sumOfValues / numValues;
    }
} // namespace AzFramework
