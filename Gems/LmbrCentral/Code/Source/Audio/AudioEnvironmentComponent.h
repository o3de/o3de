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

#include <LmbrCentral/Audio/AudioEnvironmentComponentBus.h>

#include <IAudioInterfacesCommonData.h>

namespace LmbrCentral
{
    /*!
     * AudioEnvironmentComponent
     *  Allows "sending" an amount of sound signal through effects.
     *  Typically this is done via auxillary effects bus sends.
     *  The signal goes through the bus and comes out 'wet' and is
     *  mixed into the original 'dry' sound.
     *  Only 1 AudioEnvironmentComponent is allowed on an Entity,
     *  but the api supports multiple Environment sends.
     */
    class AudioEnvironmentComponent
        : public AZ::Component
        , public AudioEnvironmentComponentRequestBus::Handler
    {
    public:
        /*!
         * AZ::Component
         */
        AZ_COMPONENT(AudioEnvironmentComponent, "{D5085D04-2522-4585-9E65-D337C5BBB8A7}");
        void Activate() override;
        void Deactivate() override;

        AudioEnvironmentComponent() = default;
        AudioEnvironmentComponent(const AZStd::string& environmentName);

        /*!
         * AudioEnvironmentComponentRequestBus::Handler Interface
         */
        void SetAmount(float amount) override;
        void SetEnvironmentAmount(const char* environmentName, float amount) override;

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("AudioEnvironmentService"));
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            dependent.push_back(AZ_CRC_CE("AudioPreloadService"));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("AudioProxyService"));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("AudioEnvironmentService"));
        }

        static void Reflect(AZ::ReflectContext* context);

    private:
        //! Editor callbacks
        void OnDefaultEnvironmentChanged();

        //! Transient data
        Audio::TAudioEnvironmentID m_defaultEnvironmentID;

        //! Serialized data
        AZStd::string m_defaultEnvironmentName;
    };

} // namespace LmbrCentral
