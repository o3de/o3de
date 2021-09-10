/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TickBus.h>
#include <AzFramework/Windowing/NativeWindow.h>
#include <AzFramework/Scene/Scene.h>

#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/WindowContext.h>
#include <Atom/Feature/SkyBox/SkyBoxFeatureProcessorInterface.h>
#include <Atom/Feature/Utils/LightingPreset.h>

namespace AZ
{
    class Entity;
    class Component;

    namespace Render
    {
        class DisplayMapperFeatureProcessorInterface;
        class DirectionalLightFeatureProcessorInterface;
        class MeshFeatureProcessorInterface;
    }

    namespace RPI
    {
        class WindowContext;
    }
}

namespace EMStudio
{
    //!
    //!
    class AnimViewportRenderer
        : public AZ::TickBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(AnimViewportRenderer, AZ::SystemAllocator, 0);

        AnimViewportRenderer(AZStd::shared_ptr<AZ::RPI::WindowContext> windowContext);
        ~AnimViewportRenderer();

    private:
        // AZ::TickBus::Handler interface overrides...
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        void Reset();
        void SetLightingPreset(const AZ::Render::LightingPreset* preset);

        AZStd::shared_ptr<AZ::RPI::WindowContext> m_windowContext;
        AZStd::unique_ptr<AzFramework::EntityContext> m_entityContext;
        AZStd::shared_ptr<AzFramework::Scene> m_frameworkScene;
        AZ::RPI::ScenePtr m_scene;
        AZ::RPI::RenderPipelinePtr m_renderPipeline;
        AZ::Render::DirectionalLightFeatureProcessorInterface* m_directionalLightFeatureProcessor = nullptr;
        AZ::Render::DisplayMapperFeatureProcessorInterface* m_displayMapperFeatureProcessor = nullptr;
        AZ::Render::SkyBoxFeatureProcessorInterface* m_skyboxFeatureProcessor = nullptr;
        AZ::Render::MeshFeatureProcessorInterface* m_meshFeatureProcessor = nullptr;

        AZ::Entity* m_postProcessEntity = nullptr;
        AZ::Entity* m_iblEntity = nullptr;

        AZ::Entity* m_cameraEntity = nullptr;
        AZ::Component* m_cameraComponent = nullptr;

        AZ::Entity* m_modelEntity = nullptr;
        AZ::Data::AssetId m_modelAssetId;
        
        AZ::Entity* m_gridEntity = nullptr;
    };
}
