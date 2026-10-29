/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <SceneColorView/Configuration.h>

namespace SceneColorView
{
    // Called from the editor and game components. No global settings are used.
    bool ApplySettingsForEntity(AZ::EntityId entityId, const Configuration& configuration);
    void ReleaseSettingsForEntity(AZ::EntityId entityId);

    class SceneColorViewComponent final : public AZ::Component, private AZ::TickBus::Handler
    {
    public:
        AZ_COMPONENT(SceneColorViewComponent, "{A7ED261C-8B54-4860-B186-F149C19858B0}");
        static void Reflect(AZ::ReflectContext* context);
        void SetConfiguration(const Configuration& configuration) { m_configuration = configuration; }
        void Activate() override;
        void Deactivate() override;
    private:
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        int GetTickOrder() override { return AZ::TICK_PRE_RENDER; }
        Configuration m_configuration;
        bool m_warned = false;
    };
}
