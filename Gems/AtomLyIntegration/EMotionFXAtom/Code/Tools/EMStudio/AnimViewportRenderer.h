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

#include <Integration/Assets/ActorAsset.h>
#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/Feature/SkyBox/SkyBoxFeatureProcessorInterface.h>
#include <Atom/Feature/CoreLights/DirectionalLightFeatureProcessorInterface.h>
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
    class AnimViewportRenderer
    {
    public:
        AZ_CLASS_ALLOCATOR(AnimViewportRenderer, AZ::SystemAllocator, 0);

        AnimViewportRenderer(AZ::RPI::ViewportContextPtr viewportContext);
        ~AnimViewportRenderer();

        void Reinit();

        //! Return the center position of the existing objects.
        AZ::Vector3 GetCharacterCenter() const;

        void UpdateActorRenderFlag(EMotionFX::ActorRenderFlagBitset renderFlags);

    private:

        // This function resets the light, camera and other environment settings.
        void ResetEnvironment();

        // This function creates in-editor entities for all actor assets stored in the actor manager,
        // and deletes all the actor entities that no longer has an actor asset in the actor manager.
        // Those entities are used in atom render viewport to visualize actors in animation editor.
        void ReinitActorEntities();

        AZ::Entity* CreateActorEntity(AZ::Data::Asset<EMotionFX::Integration::ActorAsset> actorAsset);

        AZ::Entity* FindActorEntity(AZ::Data::Asset<EMotionFX::Integration::ActorAsset> actorAsset) const;
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
        AZ::Entity* m_gridEntity = nullptr;
        AZStd::vector<AZ::Entity*> m_actorEntities;

        AZStd::vector<AZ::Render::DirectionalLightFeatureProcessorInterface::LightHandle> m_lightHandles;
    };
}
