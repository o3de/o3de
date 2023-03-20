/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/ComponentBus.h>

#include <IAudioSystem.h>

namespace EMotionFX
{
    namespace Integration
    {
        /**
         * EMotionFX Anim Audio Component Request Bus
         * Used for making requests to the EMotionFX Anim Audio Components.
         */
        class AnimAudioComponentRequests
            : public AZ::ComponentBus
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

            /// Adds audio support to when an anim event is fired
            virtual void AddTriggerEvent(const AZStd::string& eventName, const AZStd::string& triggerName, const AZStd::string& jointName) = 0;

            /// Clears all audio support for anim events
            virtual void ClearTriggerEvents() = 0;

            /// Removes audio support from an anim event
            virtual void RemoveTriggerEvent(const AZStd::string& eventName) = 0;

            /// Execute a single ATL source trigger on a joint proxy.
            virtual bool ExecuteSourceTrigger(
                const Audio::TAudioControlID triggerID,
                const Audio::TAudioControlID& sourceId,
                const AZStd::string& jointName) = 0;

            /// Execute a single ATL trigger on a joint proxy.
            virtual bool ExecuteTrigger(
                const Audio::TAudioControlID triggerID,
                const AZStd::string& jointName) = 0;

            /// Kill a single or all ATL triggers on a joint proxy. If a joint name is provided, only kill on provided joint.  Otherwise, kill all joints' audio triggers.
            virtual void KillTrigger(const Audio::TAudioControlID triggerID, const AZStd::string* jointName = nullptr) = 0;
            virtual void KillAllTriggers(const AZStd::string* jointName = nullptr) = 0;

            /// Set an Rtpc on a joint proxy. If a joint name is provided, only set on provided joint.  Otherwise, set on all joints.
            virtual void SetRtpcValue(const Audio::TAudioControlID rtpcID, float value, const AZStd::string* jointName = nullptr) = 0;

            /// Set a Switch State on a joint proxy. If a joint name is provided, only set on provided joint.  Otherwise, set on all joints.
            virtual void SetSwitchState(const Audio::TAudioControlID switchID, const Audio::TAudioSwitchStateID stateID, const AZStd::string* jointName = nullptr) = 0;

            /// Set an Environment amount on a joint proxy. If a joint name is provided, only set on provided joint.  Otherwise, set on all joints.
            virtual void SetEnvironmentAmount(const Audio::TAudioEnvironmentID environmentID, float amount, const AZStd::string* jointName = nullptr) = 0;
        };

        using AnimAudioComponentRequestBus = AZ::EBus<AnimAudioComponentRequests>;

    } // namespace Integration
} // namespace EMotionFX
