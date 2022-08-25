/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        //! Execute a single ATL trigger on a proxy
        virtual bool ExecuteTrigger(
            const Audio::TAudioControlID triggerID) = 0;

        //! Execute a single ATL source trigger on a proxy
        virtual bool ExecuteSourceTrigger(
            const Audio::TAudioControlID triggerID,
            const Audio::SAudioSourceInfo& sourceInfo) = 0;

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
        virtual void SetObstructionCalcType(const Audio::ObstructionType type) = 0;

        //! Set the position of the audio proxy directly.
        virtual void SetPosition(const Audio::SATLWorldPosition& position) = 0;

        //! Set multiple positions of the audio proxy.
        virtual void SetMultiplePositions(const Audio::MultiPositionParams& params) = 0;
    };

    using AudioProxyComponentRequestBus = AZ::EBus<AudioProxyComponentRequests>;

} // namespace LmbrCentral
