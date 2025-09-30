/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 
#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>
#include <OpenParticleSystem/ParticleComponent.h>
#include <OpenParticleSystem/ParticleComponentConfig.h>

namespace OpenParticle
{
    class EditorParticleComponent
        : public AzToolsFramework::Components::
              EditorComponentAdapter<ParticleComponentController, ParticleComponent, ParticleComponentConfig>
        , private AzToolsFramework::EditorVisibilityNotificationBus::Handler
    {
    public:
        using BaseClass =
            AzToolsFramework::Components::EditorComponentAdapter<ParticleComponentController, ParticleComponent, ParticleComponentConfig>;

        AZ_EDITOR_COMPONENT(EditorParticleComponent, EditorParticleComponentTypeId, BaseClass);
        static constexpr const char* const EditorParticleComponentTypeId = "{d0b6b04d-12b0-40b2-80cf-4f9b264b5074}";

        static void Reflect(AZ::ReflectContext* context);

        EditorParticleComponent() = default;
        explicit EditorParticleComponent(const ParticleComponentConfig& config)
            : BaseClass(config)
        {
        }

        ~EditorParticleComponent() = default;

        void Activate() override;
        void Deactivate() override;

    protected:
        AZ::u32 OnConfigurationChanged() override;

    private:
        // AzToolsFramework::EditorEntityVisibilityNotificationBus::Handler overrides
        void OnEntityVisibilityChanged(bool visibility) override;
    };
} // namespace OpenParticle
