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
        void AddOrRemoveDebugPass();

        AZStd::unique_ptr<RayTracingDebugSettings> m_settings;
        Data::Instance<RPI::ShaderResourceGroup> m_sceneSrg;
        RPI::RenderPipeline* m_pipeline{ nullptr };
        RPI::Ptr<RPI::Pass> m_rayTracingPass;
        int m_debugComponentCount{ 0 };
        RHI::ShaderInputNameIndex m_debugOptionsIndex{ "m_debugViewMode" };
    };
} // namespace AZ::Render
