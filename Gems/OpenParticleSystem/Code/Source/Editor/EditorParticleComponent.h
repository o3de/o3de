/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 
#pragma once

#include <OpenParticleSystem/ParticleComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>
#include <OpenParticleSystem/ParticleComponentConfig.h>

#include <Editor/EditorParticleRequestBus.h>

namespace OpenParticle
{
    class EditorParticleComponent
        : public AzToolsFramework::Components::
              EditorComponentAdapter<ParticleComponentController, ParticleComponent, ParticleComponentConfig>
        , private AzToolsFramework::EditorVisibilityNotificationBus::Handler
        , private EditorParticleRequestBus::Handler
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

        void SetParticleAsset(AZ::Data::Asset<ParticleAsset> particleAsset, bool inParticleEditor) override;
        void SetMaterialDiffuseMap(AZ::u32 emitterIndex, AZStd::string mapPath) override;

    protected:
        AZ::u32 OnConfigurationChanged() override;
        static void OpenParticleEditor(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType& assetType);

    private:
        // AzToolsFramework::EditorEntityVisibilityNotificationBus::Handler overrides
        void OnEntityVisibilityChanged(bool visibility) override;
    };
} // namespace OpenParticle
