/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Input/Events/InputChannelEventFilter.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    const AZ::Crc32 InputChannelEventFilter::AnyChannelNameCrc32("wildcard_any_input_channel_name");
    const AZ::Crc32 InputChannelEventFilter::AnyDeviceNameCrc32("wildcard_any_input_device_name");

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelEventFilterInclusionList::InputChannelEventFilterInclusionList(
        const AZ::Crc32& channelNameCrc32, // AnyChannelNameCrc32
        const AZ::Crc32& deviceNameCrc32,  // AnyDeviceNameCrc32
        const LocalUserId& localUserId)    // LocalUserIdAny
    : m_channelNameCrc32InclusionList()
    , m_deviceNameCrc32InclusionList()
    , m_localUserIdInclusionList()
    {
        IncludeChannelName(channelNameCrc32);
        IncludeDeviceName(deviceNameCrc32);
        IncludeLocalUserId(localUserId);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputChannelEventFilterInclusionList::DoesPassFilter(const InputChannel& inputChannel) const
    {
        if (!m_channelNameCrc32InclusionList.empty())
        {
            const AZ::Crc32& channelName = inputChannel.GetInputChannelId().GetNameCrc32();
            if (m_channelNameCrc32InclusionList.find(channelName) == m_channelNameCrc32InclusionList.end())
            {
                return false;
            }
        }

        if (!m_deviceNameCrc32InclusionList.empty())
        {
            const AZ::Crc32& deviceName = inputChannel.GetInputDevice().GetInputDeviceId().GetNameCrc32();
            if (m_deviceNameCrc32InclusionList.find(deviceName) == m_deviceNameCrc32InclusionList.end())
            {
                return false;
            }
        }

        if (!m_localUserIdInclusionList.empty())
        {
            const LocalUserId localUserId = inputChannel.GetInputDevice().GetAssignedLocalUserId();
            if (m_localUserIdInclusionList.find(localUserId) == m_localUserIdInclusionList.end())
            {
                return false;
            }
        }

        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelEventFilterInclusionList::IncludeChannelName(const AZ::Crc32& channelNameCrc32)
    {
        if (channelNameCrc32 == AnyChannelNameCrc32)
        {
            m_channelNameCrc32InclusionList.clear();
        }
        else
        {
            m_channelNameCrc32InclusionList.insert(channelNameCrc32);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelEventFilterInclusionList::IncludeDeviceName(const AZ::Crc32& deviceNameCrc32)
    {
        if (deviceNameCrc32 == AnyDeviceNameCrc32)
        {
            m_deviceNameCrc32InclusionList.clear();
        }
        else
        {
            m_deviceNameCrc32InclusionList.insert(deviceNameCrc32);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelEventFilterInclusionList::IncludeLocalUserId(LocalUserId localUserId)
    {
        if (localUserId == LocalUserIdAny)
        {
            m_localUserIdInclusionList.clear();
        }
        else
        {
            m_localUserIdInclusionList.insert(localUserId);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelEventFilterExclusionList::InputChannelEventFilterExclusionList()
        : m_channelNameCrc32ExclusionList()
        , m_deviceNameCrc32ExclusionList()
        , m_localUserIdExclusionList()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputChannelEventFilterExclusionList::DoesPassFilter(const InputChannel& inputChannel) const
    {
        const AZ::Crc32& channelName = inputChannel.GetInputChannelId().GetNameCrc32();
        if (m_channelNameCrc32ExclusionList.find(channelName) != m_channelNameCrc32ExclusionList.end())
        {
            return false;
        }

        const AZ::Crc32& deviceName = inputChannel.GetInputDevice().GetInputDeviceId().GetNameCrc32();
        if (m_deviceNameCrc32ExclusionList.find(deviceName) != m_deviceNameCrc32ExclusionList.end())
        {
            return false;
        }

        const LocalUserId localUserId = inputChannel.GetInputDevice().GetAssignedLocalUserId();
        if (m_localUserIdExclusionList.find(localUserId) != m_localUserIdExclusionList.end())
        {
            return false;
        }

        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelEventFilterExclusionList::ExcludeChannelName(const AZ::Crc32& channelNameCrc32)
    {
        m_channelNameCrc32ExclusionList.insert(channelNameCrc32);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelEventFilterExclusionList::ExcludeDeviceName(const AZ::Crc32& deviceNameCrc32)
    {
        m_deviceNameCrc32ExclusionList.insert(deviceNameCrc32);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelEventFilterExclusionList::ExcludeLocalUserId(LocalUserId localUserId)
    {
        m_localUserIdExclusionList.insert(localUserId);
    }
} // namespace AzFramework
