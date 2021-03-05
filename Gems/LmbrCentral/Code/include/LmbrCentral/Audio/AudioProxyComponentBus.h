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

#include <IAudioSystem.h>
#include <AzCore/Component/ComponentBus.h>

namespace LmbrCentral
{
    /*!
     * AudioProxyComponentRequests EBus Interface
     * Messages serviced by AudioProxyComponents.
     */
    class AudioProxyComponentRequests
        : public AZ::ComponentBus
    {
    public:
        //! Execute a single ATL source trigger on a proxy
        virtual bool ExecuteSourceTrigger(
            const Audio::TAudioControlID triggerID,
            const Audio::SAudioCallBackInfos& callbackInfo,
            const Audio::SAudioSourceInfo& sourceInfo) = 0;

        //! Execute a single ATL trigger on a proxy
        virtual bool ExecuteTrigger(
            const Audio::TAudioControlID triggerID,
            const Audio::SAudioCallBackInfos& callbackInfo) = 0;

        //! Kill a single or all ATL triggers on a proxy
        virtual void KillTrigger(const Audio::TAudioControlID triggerID) = 0;
        virtual void KillAllTriggers() = 0;

        //! Set an Rtpc on a proxy
        virtual void SetRtpcValue(const Audio::TAudioControlID rtpcID, float value) = 0;

        //! Set a Switch State on a proxy
        virtual void SetSwitchState(const Audio::TAudioControlID switchID, const Audio::TAudioSwitchStateID stateID) = 0;

        //! Set an Environment amount on a proxy
        virtual void SetEnvironmentAmount(const Audio::TAudioEnvironmentID environmentID, float amount) = 0;

        //! Specify whether the proxy should update position with the Entity
        virtual void SetMovesWithEntity(bool shouldTrackEntity) = 0;

        //! Set the Occlusion/Obstruction calculation type
        virtual void SetObstructionCalcType(const Audio::EAudioObjectObstructionCalcType type) = 0;

        //! Set the position of the audio proxy directly.
        virtual void SetPosition(const Audio::SATLWorldPosition& position) = 0;

        //! Set multiple positions of the audio proxy.
        virtual void SetMultiplePositions(const Audio::MultiPositionParams& params) = 0;
    };

    using AudioProxyComponentRequestBus = AZ::EBus<AudioProxyComponentRequests>;

} // namespace LmbrCentral
