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

#include <LmbrCentral/Audio/AudioSwitchComponentBus.h>

#include <IAudioInterfacesCommonData.h>

namespace LmbrCentral
{
    /*!
     * AudioSwitchComponent
     * A 'Switch' is something that can be in one 'State' at a time, but "switched" at run-time.
     * For example, a Switch called 'SurfaceMaterial' might have States such as 'Grass', 'Snow', 'Metal', 'Wood'.
     * But a Footstep sound would only be in one of those States at a time.
     */
    class AudioSwitchComponent
        : public AZ::Component
        , public AudioSwitchComponentRequestBus::Handler
    {
    public:
        /*!
         * AZ::Component
         */
        AZ_COMPONENT(AudioSwitchComponent, "{85FD9037-A5EA-4783-B49A-7959BBB34011}");
        void Activate() override;
        void Deactivate() override;

        AudioSwitchComponent() = default;
        AudioSwitchComponent(const AZStd::string& switchName, const AZStd::string& stateName);

        /*!
         * AudioSwitchComponentRequestBus::Handler Interface
         */
        void SetState(const char* stateName) override;
        void SetSwitchState(const char* switchName, const char* stateName) override;

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("AudioSwitchService", 0x652b8dfd));
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
            incompatible.push_back(AZ_CRC("AudioSwitchService", 0x652b8dfd));
        }

        static void Reflect(AZ::ReflectContext* context);

    private:
        //! Editor callbacks
        void OnDefaultSwitchChanged();
        void OnDefaultStateChanged();

        //! Transient data
        Audio::TAudioControlID m_defaultSwitchID;
        Audio::TAudioControlID m_defaultStateID;

        //! Serialized data
        AZStd::string m_defaultSwitchName;
        AZStd::string m_defaultStateName;
    };
}
