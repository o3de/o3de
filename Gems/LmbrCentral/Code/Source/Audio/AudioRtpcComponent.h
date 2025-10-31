/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/Component/Component.h>
#include <LmbrCentral/Audio/AudioRtpcComponentBus.h>

#include <IAudioInterfacesCommonData.h>

namespace LmbrCentral
{
    /*!
    * AudioRtpcComponent
    *  Allows settings values on ATL Rtpcs (Real-Time Parameter Controls).
    *  An Rtpc name can be serialized with the component, or it can be manually specified
    *  at runtime for use in scripting.
    */
    class AudioRtpcComponent
        : public AZ::Component
        , public AudioRtpcComponentRequestBus::Handler
    {
    public:
        /*!
         * AZ::Component
         */
        AZ_COMPONENT(AudioRtpcComponent, "{C54C7AE6-08AA-49E0-B6CD-E1BBB4950DAF}");
        void Activate() override;
        void Deactivate() override;

        AudioRtpcComponent() = default;
        AudioRtpcComponent(const AZStd::string& rtpcName);

        /*!
         * AudioRtpcComponentRequestBus::Handler Required Interface
         */
        void SetValue(float value) override;
        void SetRtpcValue(const char* rtpcName, float value) override;

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("AudioRtpcService"));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("AudioProxyService"));
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            dependent.push_back(AZ_CRC_CE("AudioPreloadService"));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("AudioRtpcService"));
        }

        static void Reflect(AZ::ReflectContext* context);

    private:
        //! Editor callback
        void OnRtpcNameChanged();

        //! Transient data
        Audio::TAudioControlID m_defaultRtpcID = INVALID_AUDIO_CONTROL_ID;

        //! Serialized data
        AZStd::string m_defaultRtpcName;
    };

} // namespace LmbrCentral
