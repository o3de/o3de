/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/Buffer/BufferSystem.h>
#include <Atom/RPI.Public/Configuration.h>
#include <Atom/RPI.Public/FeatureProcessorFactory.h>
#include <Atom/RPI.Public/DynamicDraw/DynamicDrawSystem.h>
#include <Atom/RPI.Public/Image/ImageSystem.h>
#include <Atom/RPI.Public/Material/MaterialSystem.h>
#include <Atom/RPI.Public/Model/ModelSystem.h>
#include <Atom/RPI.Public/Pass/PassSystem.h>
#include <Atom/RPI.Public/XR/XRRenderingInterface.h>
#include <Atom/RHI/RHISystem.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Shader/ShaderSystem.h>
#include <Atom/RPI.Public/GpuQuery/GpuQuerySystem.h>
#include <Atom/RPI.Public/ViewportContextManager.h>

#include <Atom/RPI.Reflect/RPISystemDescriptor.h>

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/vector.h>

namespace UnitTest
{
    class RPITestFixture;
}

namespace AZ
{
    class ReflectContext;


    namespace RPI
    {
        class FeatureProcessor;

        AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
        class ATOM_RPI_PUBLIC_API RPISystem final
            : public RPISystemInterface
            , public AZ::SystemTickBus::Handler
            , public AZ::Debug::TraceMessageBus::Handler
        {
            AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
            friend class UnitTest::RPITestFixture;

        public:
            AZ_TYPE_INFO(RPISystem, "{D248ED01-1D68-4F76-9DD8-1332B11F452A}");
            AZ_CLASS_ALLOCATOR(RPISystem, AZ::SystemAllocator);

            static void Reflect(ReflectContext* context);

            RPISystem() = default;
            ~RPISystem() = default;

            void Initialize(const RPISystemDescriptor& descriptor);
            void Shutdown();

            // RPISystemInterface overrides...
            bool IsInitialized() const override;
            void InitializeSystemAssets() override;
            bool IsNullRenderer() const override;
            void RegisterScene(ScenePtr scene) override;
            void UnregisterScene(ScenePtr scene) override;
            Scene* GetScene(const SceneId& sceneId) const override;
            Scene* GetSceneByName(const AZ::Name& name) const override;
            ScenePtr GetDefaultScene() const override;
            uint32_t GetNumScenes() const override;
            RenderPipelinePtr GetRenderPipelineForWindow(AzFramework::NativeWindowHandle windowHandle) override;
            Data::Asset<ShaderAsset> GetCommonShaderAssetForSrgs() const override;
            RHI::Ptr<RHI::ShaderResourceGroupLayout> GetSceneSrgLayout() const override;
            RHI::Ptr<RHI::ShaderResourceGroupLayout> GetViewSrgLayout() const override;
            void SimulationTick() override;
            void RenderTick() override;
            void SetSimulationJobPolicy(RHI::JobPolicy jobPolicy) override;
            RHI::JobPolicy GetSimulationJobPolicy() const override;
            void SetRenderPrepareJobPolicy(RHI::JobPolicy jobPolicy) override;
            RHI::JobPolicy GetRenderPrepareJobPolicy() const override;
            const RPISystemDescriptor& GetDescriptor() const override;
            Name GetRenderApiName() const override;
            uint64_t GetCurrentTick() const override;
            void SetApplicationMultisampleState(const RHI::MultisampleState& multisampleState) override;
            const RHI::MultisampleState& GetApplicationMultisampleState() const override;

            // AZ::Debug::TraceMessageBus::Handler overrides...
            bool OnPreAssert(const char* fileName, int line, const char* func, const char* message) override;

            // Register/Unregister xr system with RPI and RHI
            void RegisterXRSystem(XRRenderingInterface* xrSystemInterface);
            void UnregisterXRSystem();

            // Get the Xr system
            XRRenderingInterface* GetXRSystem() const override;
        private:
            // Initializes the system assets for tests. Should only be called from tests
            void InitializeSystemAssetsForTests();

            // SystemTickBus::OnTick
            void OnSystemTick() override;

            float GetCurrentTime() const;

            // Initializes XR resources (session, device, swapchain, etc).
            void InitXRSystem();

            // The set of core asset handlers registered by the system.
            AZStd::vector<AZStd::unique_ptr<Data::AssetHandler>> m_assetHandlers;

            RHI::RHISystem m_rhiSystem;
            MaterialSystem m_materialSystem;
            ModelSystem m_modelSystem;
            ShaderSystem m_shaderSystem;
            BufferSystem m_bufferSystem;
            ImageSystem m_imageSystem;
            PassSystem m_passSystem;
            DynamicDrawSystem m_dynamicDraw;
            FeatureProcessorFactory m_featureProcessorFactory;
            GpuQuerySystem m_querySystem;
            ViewportContextManager m_viewportContextManager;

            AZStd::vector<ScenePtr> m_scenes;

            // The job policy used for feature processor's simulation
            RHI::JobPolicy m_simulationJobPolicy = RHI::JobPolicy::Parallel;

            // The job policy used for feature processor's rendering prepare
            RHI::JobPolicy m_prepareRenderJobPolicy = RHI::JobPolicy::Parallel;

            float m_currentSimulationTime = 0.0f;

            RPISystemDescriptor m_descriptor;

            // Reference to the shader asset that is used
            // to get the layout for SceneSrg (@m_sceneSrgLayout) and ViewSrg (@m_viewSrgLayout).
            Data::Asset<ShaderAsset> m_commonShaderAssetForSrgs;
            RHI::Ptr<RHI::ShaderResourceGroupLayout> m_sceneSrgLayout;
            RHI::Ptr<RHI::ShaderResourceGroupLayout> m_viewSrgLayout;

            bool m_systemAssetsInitialized = false;

            uint64_t m_renderTick = 0;

            // Application multisample state
            RHI::MultisampleState m_multisampleState;

            //XR System
            XRRenderingInterface* m_xrSystem = nullptr;
        };

    } // namespace RPI
} // namespace AZ
