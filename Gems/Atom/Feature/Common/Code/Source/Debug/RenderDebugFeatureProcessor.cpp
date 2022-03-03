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

namespace AZ {
    namespace Render {

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

        RenderDebugSettingsInterface* RenderDebugFeatureProcessor::GetOrCreateSettingsInterface()
        {
            if (m_settings == nullptr)
            {
                m_settings = AZStd::make_unique<RenderDebugSettings>(this);
            }
            return m_settings.get();
        }

        void RenderDebugFeatureProcessor::RemoveSettingsInterface()
        {
            m_settings = nullptr;
        }

        void RenderDebugFeatureProcessor::OnPostProcessSettingsChanged()
        {
        }

        void RenderDebugFeatureProcessor::Activate()
        {
            m_sceneSrg = GetParentScene()->GetShaderResourceGroup();

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
        }

        void RenderDebugFeatureProcessor::Render(const RPI::FeatureProcessor::RenderPacket& packet)
        {
            AZ_PROFILE_SCOPE(RPI, "RenderDebugFeatureProcessor: Render");
            AZ_UNUSED(packet);

            if (!m_settings)
            {
                return;
            }

            if (GetParentScene())
            {
                Data::Instance<RPI::ShaderResourceGroup> sceneSrg = GetParentScene()->GetShaderResourceGroup();

                if (sceneSrg)
                {
                    sceneSrg->SetConstant(m_debugOverrideAlbedoIndex, m_settings->GetMaterialAlbedoOverride());
                    sceneSrg->SetConstant(m_debugOverrideRoughnessIndex, m_settings->GetMaterialRoughnessOverride());
                    sceneSrg->SetConstant(m_debugOverrideMetallicIndex, m_settings->GetMaterialMetallicOverride());
                    sceneSrg->SetConstant(m_debugLightingIntensityIndex, m_settings->GetDebugLightingIntensity());

                    float yaw = m_settings->GetDebugLightingAzimuth();
                    float pitch = m_settings->GetDebugLightingElevation();

                    yaw = AZ::DegToRad(yaw);
                    pitch = AZ::DegToRad(pitch);

                    Transform lightRotation = Transform::CreateRotationZ(yaw) * Transform::CreateRotationX(pitch);
                    Vector3 lightDirection = lightRotation.GetBasis(0);
                    sceneSrg->SetConstant(m_debugLightingDirectionIndex, lightDirection);
                }
            }

            for (const RPI::ViewPtr& view : packet.m_views)
            {
                if (view->GetUsageFlags() && RPI::View::UsageFlags::UsageCamera)
                {
                    Data::Instance<RPI::ShaderResourceGroup> viewSrg = view->GetShaderResourceGroup();

                    if (viewSrg)
                    {
                        viewSrg->SetConstant(m_renderDebugOptionsIndex, 0);
                        viewSrg->SetConstant(m_renderDebugViewModeIndex, 0);
                    }
                }

                // m_lightBufferHandler.UpdateSrg(view->GetShaderResourceGroup().get());
            }
        }

    } // namespace Render
} // namespace AZ
