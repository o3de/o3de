/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Input/Mappings/InputMappingOr.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputMappingOr::InputMappingOr(const InputChannelId& inputChannelId, const InputContext& inputContext)
        : InputMapping(inputChannelId, inputContext)
        , m_sourceInputChannelIds()
        , m_currentlyActiveSource(EmptySnapshot())
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputMappingOr::AddSourceInput(const InputChannelId& inputSourceId)
    {
        const auto it = m_sourceInputChannelIds.find(inputSourceId);
        if (it != m_sourceInputChannelIds.end())
        {
            AZ_Warning("InputMappingOr", false,
                       "Input mapping (%s) already contains an input source with id: %s, cannot add",
                       GetInputChannelId().GetName(), inputSourceId.GetName());
            return false;
        }

        m_sourceInputChannelIds.insert(inputSourceId);
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputMappingOr::RemoveSourceInput(const InputChannelId& inputSourceId)
    {
        const auto it = m_sourceInputChannelIds.find(inputSourceId);
        if (it == m_sourceInputChannelIds.end())
        {
            AZ_Warning("InputMappingOr", false,
                       "Input mapping (%s) does not contain an input source with id: %s, cannot remove",
                       GetInputChannelId().GetName(), inputSourceId.GetName());
            return false;
        }

        m_sourceInputChannelIds.erase(it);
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    float InputMappingOr::GetValue() const
    {
        return m_currentlyActiveSource.m_value;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    float InputMappingOr::GetDelta() const
    {
        return m_currentlyActiveSource.m_delta;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputMappingOr::ResetState()
    {
        m_currentlyActiveSource = EmptySnapshot();
        InputChannel::ResetState();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputMappingOr::IsSourceInput(const InputChannel& inputChannel)
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
    bool InputMappingOr::OnSourceInputEvent(const InputChannel& inputChannel)
    {
        // Capture the source input even when in the 'Ended' state in order to capture the delta.
        m_currentlyActiveSource = inputChannel.IsStateIdle() ?
                                  EmptySnapshot() :
                                  InputChannel::Snapshot(inputChannel);
        UpdateState(inputChannel.IsActive());
        return true;
    }
} // namespace AzFramework
