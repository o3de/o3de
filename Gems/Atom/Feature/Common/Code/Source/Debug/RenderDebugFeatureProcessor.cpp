/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Debug/RenderDebugFeatureProcessor.h>
#include <Debug/RenderDebugSettings.h>

#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/Shader/ShaderSystemInterface.h>

namespace AZ::Render
{
    RenderDebugFeatureProcessor::RenderDebugFeatureProcessor()
    {
    }

    void RenderDebugFeatureProcessor::Reflect(ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext
                ->Class<RenderDebugFeatureProcessor, RPI::FeatureProcessor>()
                ->Version(0);
        }
    }

    //! RenderDebugFeatureProcessorInterface overrides...
    RenderDebugSettingsInterface* RenderDebugFeatureProcessor::GetSettingsInterface()
    {
        return m_settings.get();
    }

    void RenderDebugFeatureProcessor::OnRenderDebugComponentAdded()
    {
        ++m_debugComponentCount;
    }

    void RenderDebugFeatureProcessor::OnRenderDebugComponentRemoved()
    {
        --m_debugComponentCount;
    }

    void RenderDebugFeatureProcessor::Activate()
    {
        m_sceneSrg = GetParentScene()->GetShaderResourceGroup();
        m_settings = AZStd::make_unique<RenderDebugSettings>(this);
    }

    void RenderDebugFeatureProcessor::Deactivate()
    {
        m_sceneSrg = nullptr;
        m_settings = nullptr;
    }

    void RenderDebugFeatureProcessor::Simulate(const RPI::FeatureProcessor::SimulatePacket& packet)
    {
        AZ_PROFILE_SCOPE(RPI, "RenderDebugFeatureProcessor: Simulate");
        AZ_UNUSED(packet);

        if (m_settings)
        {
            m_settings->Simulate();
        }
    }

    void RenderDebugFeatureProcessor::Render(const RPI::FeatureProcessor::RenderPacket& packet)
    {
        AZ_PROFILE_SCOPE(RPI, "RenderDebugFeatureProcessor: Render");
        AZ_UNUSED(packet);

        // Disable debugging if no render debug level component is active
        bool debugEnabled = (m_debugComponentCount > 0) && m_settings->GetEnabled();

        RPI::ShaderSystemInterface::Get()->SetGlobalShaderOption(m_shaderDebugEnableOptionName, AZ::RPI::ShaderOptionValue{ debugEnabled });

        if (m_sceneSrg)
        {
            m_sceneSrg->SetConstant(m_debuggingEnabledIndex, debugEnabled);

            // Material overrides...
            m_sceneSrg->SetConstant(m_debugOverrideBaseColorIndex, m_settings->GetMaterialBaseColorOverride());
            m_sceneSrg->SetConstant(m_debugOverrideRoughnessIndex, m_settings->GetMaterialRoughnessOverride());
            m_sceneSrg->SetConstant(m_debugOverrideMetallicIndex, m_settings->GetMaterialMetallicOverride());

            // Debug Light...
            Vector3 debugLightIntensity = m_settings->GetDebugLightingColor() * m_settings->GetDebugLightingIntensity();
            m_sceneSrg->SetConstant(m_debugLightingIntensityIndex, debugLightIntensity);

            float yaw = m_settings->GetDebugLightingAzimuth();
            float pitch = m_settings->GetDebugLightingElevation();

            yaw = AZ::DegToRad(yaw);
            pitch = AZ::DegToRad(pitch);

            Transform lightRotation = Transform::CreateRotationZ(yaw) * Transform::CreateRotationX(pitch);
            Vector3 lightDirection = lightRotation.GetBasis(1);
            m_sceneSrg->SetConstant(m_debugLightingDirectionIndex, lightDirection);
        }

        for (const RPI::ViewPtr& view : packet.m_views)
        {
            if (view->GetUsageFlags() & (RPI::View::UsageFlags::UsageCamera | RPI::View::UsageFlags::UsageReflectiveCubeMap))
            {
                Data::Instance<RPI::ShaderResourceGroup> viewSrg = view->GetShaderResourceGroup();

                if (viewSrg)
                {
                    viewSrg->SetConstant(m_renderDebugOptionsIndex, m_settings->GetRenderDebugOptions());
                    viewSrg->SetConstant(m_renderDebugViewModeIndex, m_settings->GetRenderDebugViewMode());
                }
            }
        }
    }

}
