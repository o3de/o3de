/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/Debug/RayTracingDebugFeatureProcessorInterface.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Debug/RayTracingDebugSettings.h>

namespace AZ::Render
{
    //! FeatureProcessor for handling debug ray tracing information, such as adding/removing the ray tracing debug pass to the render
    //! pipeline and uploading the debug configuration data to the GPU
    class RayTracingDebugFeatureProcessor : public RayTracingDebugFeatureProcessorInterface
    {
    public:
        AZ_CLASS_ALLOCATOR(RayTracingDebugFeatureProcessor, SystemAllocator)
        AZ_RTTI(RayTracingDebugFeatureProcessor, "{283382D4-536D-4770-931D-84F033E18636}", RayTracingDebugFeatureProcessorInterface);

        static void Reflect(ReflectContext* context);

        // RayTracingDebugFeatureProcessorInterface overrides
        RayTracingDebugSettingsInterface* GetSettingsInterface() override;
        void OnRayTracingDebugComponentAdded() override;
        void OnRayTracingDebugComponentRemoved() override;

        // FeatureProcessor overrides
        void Activate() override;
        void Deactivate() override;
        void AddRenderPasses(RPI::RenderPipeline* pipeline) override;
        void Render(const RenderPacket& packet) override;

    private:
        // Adds the debug ray tracing pass to the pipeline (The debug ray tracing pass is not part of the main pipeline and is only added if
        // a "Debug Ray Tracing" level component is added to the scene).
        void AddDebugPass();

        // Removes the debug ray tracing pass from the pipeline.
        void RemoveDebugPass();

        AZStd::unique_ptr<RayTracingDebugSettings> m_settings;
        Data::Instance<RPI::ShaderResourceGroup> m_sceneSrg; // The ray tracing scene SRG (RayTracingSceneSrg)
        RPI::RenderPipeline* m_pipeline{ nullptr };
        RPI::Ptr<RPI::Pass> m_rayTracingPass;
        int m_debugComponentCount{ 0 };
        RHI::ShaderInputNameIndex m_debugOptionsIndex{ "m_debugViewMode" };
    };
} // namespace AZ::Render
