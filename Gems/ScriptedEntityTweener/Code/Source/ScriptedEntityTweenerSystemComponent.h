/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TickBus.h>

#include <AzCore/Component/Component.h>
#include <AzCore/std/containers/set.h>

#include <ScriptedEntityTweener/ScriptedEntityTweenerBus.h>
#include "ScriptedEntityTweenerTask.h"

namespace ScriptedEntityTweener
{
    class ScriptedEntityTweenerSystemComponent
        : public AZ::Component
        , protected ScriptedEntityTweenerBus::Handler
        , protected AZ::TickBus::Handler
    {
    public:
        AZ_COMPONENT(ScriptedEntityTweenerSystemComponent, "{6AAC4396-2FAB-4273-BA80-2D25DC91A116}", AZ::Component);

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // ScriptedEntityTweenerManagerBus interface implementation
        void AnimateEntity(const AZ::EntityId& entityId, const AnimationParameters& params) override;

        //! Sets optional animation parameters to be used on next AnimateEntityScript call, needed as lua implementation doesn't support > 13 args in non-debug builds
        void SetOptionalParams(float timeIntoAnimation,
            float duration,
            int easingMethod,
            int easingType,
            float delayTime,
            int timesToPlay,
            bool isFrom,
            bool isPlayingBackward,
            const AZ::Uuid& animationId,
            int timelineId,
            int onCompleteCallbackId,
            int onUpdateCallbackId,
            int onLoopCallbackId) override;

        //! Script exposed version of the AnimateEntity call
        void AnimateEntityScript(const AZ::EntityId& entityId,
            const AZStd::string& componentName,
            const AZStd::string& virtualPropertyName,
            const AZStd::any& paramTarget) override;

        void Stop(int timelineId, const AZ::EntityId& entityId) override;
        void Pause(int timelineId, const AZ::EntityId& entityId, const AZStd::string& componentName, const AZStd::string& virtualPropertyName) override;
        void Resume(int timelineId, const AZ::EntityId& entityId, const AZStd::string& componentName, const AZStd::string& virtualPropertyName) override;
        void SetPlayDirectionReversed(int timelineId, const AZ::EntityId& entityId, const AZStd::string& componentName, const AZStd::string& virtualPropertyName, bool rewind) override;
        void SetSpeed(int timelineId, const AZ::EntityId& entityId, const AZStd::string& componentName, const AZStd::string& virtualPropertyName, float speed) override;
        void SetInitialValue(const AZ::Uuid& timelineId, const AZ::EntityId& entityId, const AZStd::string& componentName, const AZStd::string& virtualPropertyName, const AZStd::any& initialValue) override;
        AZStd::any GetVirtualPropertyValue(const AZ::EntityId& entityId, const AZStd::string& componentName, const AZStd::string& virtualPropertyName) override;

        void Reset() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::TickBus
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        int GetTickOrder() override;
        ////////////////////////////////////////////////////////////////////////

    private:
        AZStd::set<ScriptedEntityTweenerTask> m_animationTasks;

        //! Used by AnimateEntityScript, setup by SetOptionalParams
        AnimationParameters m_tempParams;
    };
}
