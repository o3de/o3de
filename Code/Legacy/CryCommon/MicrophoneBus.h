/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <IAudioInterfacesCommonData.h>

namespace Audio
{
    /**
     * Interface for connecting with a hardware Microphone device.
     * The pattern that should be used is as follows:
     * 
     * InitializeDevice
     *  StartSession
     *   (Capturing Mic Data...)
     *  EndSession
     *  ...
     *  (optional: additional StartSession/EndSession pairs)
     *  ...
     * ShutdownDevice
     */
    class MicrophoneRequests
        : public AZ::EBusTraits
    {
    public:
        /**
         * EBus Traits
         */
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        using MutexType = AZStd::recursive_mutex;

        /**
         * Initialize a hardware Microphone input device with the OS.
         * @return True if the Microphone was initialized without errors, false otherwise.
         */
        virtual bool InitializeDevice() = 0;

        /**
         * Shutdown the connection to the Microphone device.
         */
        virtual void ShutdownDevice() = 0;

        /**
         * Start capturing Microphone data.
         * @return True if the Microphone capturing session was started without errors, false otherwise.
         */
        virtual bool StartSession() = 0;

        /**
         * Stop capturing Microphone data.
         */
        virtual void EndSession() = 0;

        /**
         * Check if the Microphone is actively capturing data.
         * @return True if a capturing session has started and is running, false otherwise.
         */
        virtual bool IsCapturing() = 0;

        /**
         * Obtain the format set up for the mic capture session.
         * @return Configuration of the Microphone.
         */
        virtual SAudioInputConfig GetFormatConfig() const = 0;

        /**
         * Consume a number of sample frames from the captured data.
         * @param outputData Where the data will be copied to.
         * @param numFrames The number of sample frames requested.
         * @param targetConfig The configuration of the data sink (sample rate, channels, sample type, etc).
         * @param shouldDeinterleave Ask for a deinterleaved copy when in stereo: [LRLRLRLR] --> [LLLL, RRRR]
         * @return The number of sample frames obtained.
         */
        virtual AZStd::size_t GetData(void** outputData, AZStd::size_t numFrames, const SAudioInputConfig& targetConfig, bool shouldDeinterleave) = 0;
    };

    using MicrophoneRequestBus = AZ::EBus<MicrophoneRequests>;

} // namespace Audio
