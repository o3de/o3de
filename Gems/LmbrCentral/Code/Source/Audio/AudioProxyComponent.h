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

#include <AzCore/std/containers/map.h>
#include <AzCore/Math/Transform.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <LmbrCentral/Audio/AudioProxyComponentBus.h>

namespace LmbrCentral
{
    /*!
     * AudioProxyComponent
     *  A component wrapper for an AudioProxy,
     *  which acts as a proxy for a logical Audio Object.
     *  ATL components 'share' the AudioProxyComponent on an Entity.
     *  There is only 1 AudioProxyComponent allowed on an Entity.
     */
    class AudioProxyComponent
        : public AZ::Component
        , public AZ::TransformNotificationBus::Handler
        , public AudioProxyComponentRequestBus::Handler
    {
    public:
        /*!
         * AZ::Component
         */
        AZ_COMPONENT(AudioProxyComponent, "{0EE6EE0F-7939-4AB8-B0E3-F9B3925D61EE}");
        void Activate() override;
        void Deactivate() override;

        /*!
         * AZ::TransformNotificationBus::Handler
         * Required Interface
         */
        void OnTransformChanged(const AZ::Transform&, const AZ::Transform&) override;


        /*!
         * AudioProxyComponentRequestBus::Handler
         * Required Interface
         */
        bool ExecuteSourceTrigger(
            const Audio::TAudioControlID triggerID,
            const Audio::SAudioCallBackInfos& callbackInfo,
            const Audio::SAudioSourceInfo& sourceInfo
        ) override;

        bool ExecuteTrigger(
            const Audio::TAudioControlID triggerID,
            const Audio::SAudioCallBackInfos& callbackInfo
        ) override;

        void KillTrigger(const Audio::TAudioControlID triggerID) override;
        void KillAllTriggers() override;

        void SetRtpcValue(const Audio::TAudioControlID rtpcID, float value) override;

        void SetSwitchState(const Audio::TAudioControlID switchID, const Audio::TAudioSwitchStateID stateID) override;

        void SetEnvironmentAmount(const Audio::TAudioEnvironmentID environmentID, float amount) override;

        void SetMovesWithEntity(bool shouldTrackEntity) override
        {
            m_tracksEntityPosition = shouldTrackEntity;
        }

        void SetObstructionCalcType(const Audio::EAudioObjectObstructionCalcType type) override;

        void SetPosition(const Audio::SATLWorldPosition& position) override;

        void SetMultiplePositions(const Audio::MultiPositionParams& params) override;

    protected:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("AudioProxyService", 0x7da4c79c));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("AudioProxyService", 0x7da4c79c));
        }

        static void Reflect(AZ::ReflectContext* context);

    private:
        //! Transient data
        Audio::IAudioProxy* m_audioProxy = nullptr;
        bool m_tracksEntityPosition = true;
        AZ::Transform m_transform;

        //! Serialized data
        // None!  The AudioProxyComponent, by design, is completely transient.
    };

} // namespace LmbrCentral
