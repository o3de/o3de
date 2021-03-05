/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
            provided.push_back(AZ_CRC("AudioRtpcService", 0x8342ff68));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("AudioProxyService", 0x7da4c79c));
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            dependent.push_back(AZ_CRC("AudioPreloadService", 0x20c917d8));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("AudioRtpcService", 0x8342ff68));
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
