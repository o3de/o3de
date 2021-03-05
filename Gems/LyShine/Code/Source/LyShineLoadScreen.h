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

#include <LoadScreenBus.h>

#if AZ_LOADSCREENCOMPONENT_ENABLED

#include <AzCore/Component/Component.h>

namespace LyShine
{
    class LyShineLoadScreenComponent
        : public AZ::Component
        , public LoadScreenNotificationBus::Handler
        , public LoadScreenUpdateNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(LyShineLoadScreenComponent, "{AE8DA868-1069-48FF-8ED7-AC28829366BB}");

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

    public:
        LyShineLoadScreenComponent() = default;
        ~LyShineLoadScreenComponent() = default;

        // AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        // ~AZ::Component

        // LoadScreenNotificationBus::Handler
        bool NotifyGameLoadStart(bool usingLoadingThread) override;
        bool NotifyLevelLoadStart(bool usingLoadingThread) override;
        void NotifyLoadEnd() override;
        // ~LoadScreenNotificationBus::Handler

        // LoadScreenUpdateNotificationBus::Handler
        void UpdateAndRender(float deltaTimeInSeconds) override;
        void LoadThreadUpdate(float deltaTimeInSeconds) override;
        void LoadThreadRender() override;
        // ~LoadScreenUpdateNotificationBus::Handler

    protected:
        void Reset();
        AZ::EntityId loadFromCfg(const char* pathVarName, const char* autoPlayVarName);

        bool m_isPlaying{ false };

        AZ::EntityId m_gameCanvasEntityId{};
        AZ::EntityId m_levelCanvasEntityId{};
    };
} // namespace LyShine

#endif // AZ_LOADSCREENCOMPONENT_ENABLED
