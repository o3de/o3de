/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/Utils/GpuBufferHandler.h>
#include <Atom/Feature/Utils/MultiIndexedDataVector.h>
#include <Atom/Feature/Debug/RenderDebugFeatureProcessorInterface.h>
#include <Atom/RPI.Reflect/Image/ImageAsset.h>
#include "Atom/RPI.Public/Image/StreamingImage.h"
#include <AtomCore/Instance/Instance.h>

namespace AZ::Render
{
    class RenderDebugSettings;

    class RenderDebugFeatureProcessor final
        : public AZ::Render::RenderDebugFeatureProcessorInterface
    {
    public:
        AZ_RTTI(AZ::Render::RenderDebugFeatureProcessor, "{1F14912D-43E1-4992-9822-BE8967E59EA3}", AZ::Render::RenderDebugFeatureProcessorInterface);

        static void Reflect(AZ::ReflectContext* context);

        RenderDebugFeatureProcessor();
        virtual ~RenderDebugFeatureProcessor() = default;

        //! RenderDebugFeatureProcessorInterface overrides...
        RenderDebugSettingsInterface* GetSettingsInterface() override;
        void OnRenderDebugComponentAdded() override;
        void OnRenderDebugComponentRemoved() override;

        // FeatureProcessor overrides ...
        void Activate() override;
        void Deactivate() override;
        void Simulate(const RPI::FeatureProcessor::SimulatePacket& packet) override;
        void Render(const RPI::FeatureProcessor::RenderPacket& packet) override;

    private:

        RenderDebugFeatureProcessor(const RenderDebugFeatureProcessor&) = delete;

        static constexpr const char* FeatureProcessorName = "RenderDebugFeatureProcessor";


        AZStd::unique_ptr<RenderDebugSettings> m_settings = nullptr;

        // Scene and View SRG
        Data::Instance<RPI::ShaderResourceGroup> m_sceneSrg = nullptr;
        Data::Instance<RPI::ShaderResourceGroup> m_viewSrg = nullptr;

        // View SRG members
        RHI::ShaderInputNameIndex m_renderDebugOptionsIndex = "m_renderDebugOptions";
        RHI::ShaderInputNameIndex m_renderDebugViewModeIndex = "m_renderDebugViewMode";

        // Scene SRG members
        RHI::ShaderInputNameIndex m_debuggingEnabledIndex = "m_debuggingEnabled";
        RHI::ShaderInputNameIndex m_debugOverrideBaseColorIndex = "m_debugOverrideBaseColor";
        RHI::ShaderInputNameIndex m_debugOverrideRoughnessIndex = "m_debugOverrideRoughness";
        RHI::ShaderInputNameIndex m_debugOverrideMetallicIndex = "m_debugOverrideMetallic";
        RHI::ShaderInputNameIndex m_debugLightingDirectionIndex = "m_debugLightingDirection";
        RHI::ShaderInputNameIndex m_debugLightingIntensityIndex = "m_debugLightingIntensity";

        Name m_shaderDebugEnableOptionName = Name("o_shader_debugging_enabled");

        u32 m_debugComponentCount = 0;
    };

} // namespace AZ::Render
