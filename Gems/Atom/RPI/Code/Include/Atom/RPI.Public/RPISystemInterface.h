/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/FrameSchedulerEnums.h>
#include <Atom/RPI.Reflect/RPISystemDescriptor.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>

#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/Configuration.h>

#include <AzCore/Name/Name.h>
#include <AzFramework/Windowing/WindowBus.h>
#include <Atom/RPI.Public/XR/XRRenderingInterface.h>

namespace AZ
{
    namespace RPI
    {
        //! Interface of RPISystem, which is the main entry point for the Atom renderer.
        class ATOM_RPI_PUBLIC_API RPISystemInterface
        {
        public:
            AZ_RTTI(RPISystemInterface, "{62E72C4F-A985-4001-9004-DE53029DBF11}");
            
            // Note that you have to delete these for safety reasons, you will trip a static_assert if you do not
            AZ_DISABLE_COPY_MOVE(RPISystemInterface);

            static RPISystemInterface* Get();

            RPISystemInterface() = default;
            virtual ~RPISystemInterface() = default;

            //! Pre-load some system assets. This should be called once Critical Asset have compiled ready and before create any RPI instances.
            virtual void InitializeSystemAssets() = 0;

            //! Was the RPI system initialized properly
            virtual bool IsInitialized() const = 0;

            //! If the renderer is null.
            //! It's controlled by renderer=null command line option
            virtual bool IsNullRenderer() const = 0;

            //! Register a created scene to RPISystem. Registered scene will be simulated and rendered in RPISystem ticks
            virtual void RegisterScene(ScenePtr scene) = 0;

            //! Unregister a scene from RPISystem. The scene won't be simulated or rendered.
            virtual void UnregisterScene(ScenePtr scene) = 0;

            //! Deprecated. Use GetSceneByName(name), GetSceneForEntityContextId(entityContextId) or Scene::GetSceneForEntityId(AZ::EntityId entityId) instead
            AZ_DEPRECATED(virtual ScenePtr GetDefaultScene() const = 0;, "This method has been deprecated. Please use GetSceneByName(name), GetSceneForEntityContextId(entityContextId) or Scene::GetSceneForEntityId(AZ::EntityId entityId) instead.");
            
            //! Get scene by using scene id.
            virtual Scene* GetScene(const SceneId& sceneId) const = 0;

            //! Get scene by using scene name.
            virtual Scene* GetSceneByName(const AZ::Name& name) const = 0;

            //! Return the number of registered scenes
            virtual uint32_t GetNumScenes() const = 0;

            //! Get the render pipeline created for a window
            virtual RenderPipelinePtr GetRenderPipelineForWindow(AzFramework::NativeWindowHandle windowHandle) = 0;

            //! Returns the shader asset that is being used as the source for the SceneSrg and ViewSrg layouts.
            virtual Data::Asset<ShaderAsset> GetCommonShaderAssetForSrgs() const = 0;

            virtual RHI::Ptr<RHI::ShaderResourceGroupLayout> GetSceneSrgLayout() const = 0;

            virtual RHI::Ptr<RHI::ShaderResourceGroupLayout> GetViewSrgLayout() const = 0;
            
            //! Tick for graphics simulation that runs on the CPU. 
            //! This will drive FeatureProcessor simulation activity. It should be called once per game-tick.
            virtual void SimulationTick() = 0;

            //! Tick for rendering one frame.
            virtual void RenderTick() = 0;

            //! Job policy for FeatureProcessor simulation. 
            //! When parallel jobs are enabled, this will usually spawn one job per FeatureProcessor per Scene.
            virtual void SetSimulationJobPolicy(RHI::JobPolicy jobPolicy) = 0;
            virtual RHI::JobPolicy GetSimulationJobPolicy() const = 0;

            //! Job policy for FeatureProcessor render prepare. 
            //! When parallel jobs are enabled, this will usually spawn one job per FeatureProcessor per Scene.
            virtual void SetRenderPrepareJobPolicy(RHI::JobPolicy jobPolicy) = 0;
            virtual RHI::JobPolicy GetRenderPrepareJobPolicy() const = 0;

            //! Get RPI system descriptor
            virtual const RPISystemDescriptor& GetDescriptor() const = 0;

            //! Return the name of the RHI back-end API (i.e. "dx12", "vulkan", etc.)
            virtual Name GetRenderApiName() const = 0;

            //! Get the index of current render tick
            virtual uint64_t GetCurrentTick() const = 0;

            //! Application multisample state
            virtual void SetApplicationMultisampleState(const RHI::MultisampleState& multisampleState) = 0;
            virtual const RHI::MultisampleState& GetApplicationMultisampleState() const = 0;

            //! Get the XR system for XR specific functionality
            virtual XRRenderingInterface* GetXRSystem() const = 0;
        };

    } // namespace RPI
} // namespace AZ
