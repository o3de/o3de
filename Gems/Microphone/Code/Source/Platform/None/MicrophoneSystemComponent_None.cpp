/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <MicrophoneSystemComponent.h>

namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Null Implementation of a Microphone
    class MicrophoneSystemComponentNone : public MicrophoneSystemComponent::Implementation
    {
    public:
        AZ_CLASS_ALLOCATOR(MicrophoneSystemComponentNone, AZ::SystemAllocator);

        bool InitializeDevice() override
        {
            return true;
        }

        void ShutdownDevice() override
        {
        }

        bool StartSession() override
        {
            return true;
        }

        void EndSession() override
        {
        }

        bool IsCapturing() override
        {
            return false;
        }

        SAudioInputConfig GetFormatConfig() const override
        {
            return m_config;
        }

        AZStd::size_t GetData(void**, AZStd::size_t, const SAudioInputConfig&, bool) override
        {
            return 0;
        }

    private:
        SAudioInputConfig m_config;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////
    MicrophoneSystemComponent::Implementation* MicrophoneSystemComponent::Implementation::Create()
    {
        return aznew MicrophoneSystemComponentNone();
    }
} // namespace Audio
