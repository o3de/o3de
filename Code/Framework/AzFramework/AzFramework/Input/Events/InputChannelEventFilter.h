/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzFramework/Input/Channels/InputChannel.h>
#include <AzFramework/Input/Devices/InputDevice.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Class that filters input events by channel name, device name, local user id, or
    //! any combination of the three.
    class InputChannelEventFilter
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Wildcard representing any input channel name
        static const AZ::Crc32 AnyChannelNameCrc32;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Wildcard representing any input device name
        static const AZ::Crc32 AnyDeviceNameCrc32;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default constructor
        InputChannelEventFilter() = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Default copying
        AZ_DEFAULT_COPY(InputChannelEventFilter);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        virtual ~InputChannelEventFilter() = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Check whether an input channel should pass through the filter
        //! \param[in] inputChannel The input channel to be filtered
        //! \return True if the input channel passes the filter, false otherwise
        virtual bool DoesPassFilter(const InputChannel& inputChannel) const = 0;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Class that filters input channel events based on included input channels and devices.
    class InputChannelEventFilterInclusionList : public InputChannelEventFilter
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor. By default the filter will be constructed to include all input events.
        //! \param[in] channelNameCrc32 The input channel name (Crc32) to include (default any)
        //! \param[in] deviceNameCrc32 The input device name (Crc32) to include (default any)
        //! \param[in] localUserId The local user id to include (default any)
        explicit InputChannelEventFilterInclusionList(const AZ::Crc32& channelNameCrc32 = AnyChannelNameCrc32,
                                                      const AZ::Crc32& deviceNameCrc32 = AnyDeviceNameCrc32,
                                                      const LocalUserId& localUserId = LocalUserIdAny);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Default copying
        AZ_DEFAULT_COPY(InputChannelEventFilterInclusionList);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        ~InputChannelEventFilterInclusionList() override = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputChannelEventFilter::DoesPassFilter
        bool DoesPassFilter(const InputChannel& inputChannel) const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Add an input channel name to the inclusion list
        //! \param[in] channelNameCrc32 The input channel name (Crc32) to add to the inclusion list
        void IncludeChannelName(const AZ::Crc32& channelNameCrc32);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Add an input device name to the inclusion list
        //! \param[in] channelNameCrc32 The input device name (Crc32) to add to the inclusion list
        void IncludeDeviceName(const AZ::Crc32& deviceNameCrc32);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Add a local user id to the inclusion list
        //! \param[in] localUserId The local user id to add to the inclusion list
        void IncludeLocalUserId(LocalUserId localUserId);

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        AZStd::unordered_set<AZ::Crc32>   m_channelNameCrc32InclusionList; //!< Channel name inclusion list
        AZStd::unordered_set<AZ::Crc32>   m_deviceNameCrc32InclusionList;  //!< Device name inclusion list
        AZStd::unordered_set<LocalUserId> m_localUserIdInclusionList;      //!< Local user inclusion list
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Class that filters input channel events based on excluded input channels and devices.
    class InputChannelEventFilterExclusionList : public InputChannelEventFilter
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor. By default the filter will be constructed to exclude no input events.
        InputChannelEventFilterExclusionList();

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Default copying
        AZ_DEFAULT_COPY(InputChannelEventFilterExclusionList);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        ~InputChannelEventFilterExclusionList() override = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputChannelEventFilter::DoesPassFilter
        bool DoesPassFilter(const InputChannel& inputChannel) const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Add an input channel name to the exclusion list
        //! \param[in] channelNameCrc32 The input channel name (Crc32) to add to the exclusion list
        void ExcludeChannelName(const AZ::Crc32& channelNameCrc32);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Add an input device name to the exclusion list
        //! \param[in] channelNameCrc32 The input device name (Crc32) to add to the exclusion list
        void ExcludeDeviceName(const AZ::Crc32& deviceNameCrc32);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Add a local user id to the exclusion list
        //! \param[in] localUserId The local user id to to add to the exclusion list
        void ExcludeLocalUserId(LocalUserId localUserId);

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        AZStd::unordered_set<AZ::Crc32>   m_channelNameCrc32ExclusionList; //!< Channel name exclusion list
        AZStd::unordered_set<AZ::Crc32>   m_deviceNameCrc32ExclusionList;  //!< Device name exclusion list
        AZStd::unordered_set<LocalUserId> m_localUserIdExclusionList;      //!< Local user exclusion list
    };
} // namespace AzFramework
