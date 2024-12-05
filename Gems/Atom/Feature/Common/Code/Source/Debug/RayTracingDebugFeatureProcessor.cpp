/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Debug/RayTracingDebugFeatureProcessor.h>
#include <RayTracing/RayTracingFeatureProcessor.h>

namespace AZ::Render
{
    void RayTracingDebugFeatureProcessor::Reflect(ReflectContext* context)
    {
        if (auto* serializeContext{ azrtti_cast<SerializeContext*>(context) })
        {
            // clang-format off
            serializeContext->Class<RayTracingDebugFeatureProcessor, RPI::FeatureProcessor>()
                ->Version(0)
            ;
            // clang-format on
        }
    }

    RayTracingDebugSettingsInterface* RayTracingDebugFeatureProcessor::GetSettingsInterface()
    {
        return m_settings.get();
    }

    void RayTracingDebugFeatureProcessor::OnRayTracingDebugComponentAdded()
    {
        if (m_debugComponentCount == 0)
        {
            AddDebugPass();
        }
        m_debugComponentCount++;
    }

    void RayTracingDebugFeatureProcessor::OnRayTracingDebugComponentRemoved()
    {
        m_debugComponentCount--;
        if (m_debugComponentCount == 0)
        {
            RemoveDebugPass();
        }
    }

    void RayTracingDebugFeatureProcessor::Activate()
    {
        m_settings = AZStd::make_unique<RayTracingDebugSettings>();

        FeatureProcessor::Activate();
        EnableSceneNotification();
    }

    void RayTracingDebugFeatureProcessor::Deactivate()
    {
        DisableSceneNotification();
        FeatureProcessor::Deactivate();

        m_sceneSrg = nullptr;
        m_settings.reset();
    }

    void RayTracingDebugFeatureProcessor::AddRenderPasses(RPI::RenderPipeline* pipeline)
    {
        m_pipeline = pipeline;
        FeatureProcessor::AddRenderPasses(pipeline);
    }

    void RayTracingDebugFeatureProcessor::Render(const RPI::FeatureProcessor::RenderPacket& packet)
    {
        if (!m_rayTracingPass)
        {
            return;
        }

        if (m_rayTracingPass->IsEnabled() != m_settings->GetEnabled())
        {
            m_rayTracingPass->SetEnabled(m_settings->GetEnabled());
        }

        if (!m_sceneSrg && m_settings->GetEnabled())
        {
            m_sceneSrg = GetParentScene()->GetFeatureProcessor<RayTracingFeatureProcessor>()->GetRayTracingSceneSrg();
        }

        if (m_sceneSrg && m_settings->GetEnabled())
        {
            if (!m_sceneSrg->SetConstant(m_debugOptionsIndex, m_settings->GetDebugViewMode()))
            {
                AZ_ErrorOnce(
                    "RayTracingDebugFeatureProcessor",
                    m_debugOptionsIndex.IsValid(),
                    "Failed to find shader input index for '%s' in the ray tracing scene SRG.",
                    m_debugOptionsIndex.GetNameForDebug().GetCStr());
            }
        }

        FeatureProcessor::Render(packet);
    }

    void RayTracingDebugFeatureProcessor::AddDebugPass()
    {
        m_rayTracingPass =
            RPI::PassSystemInterface::Get()->CreatePassFromTemplate(Name{ "DebugRayTracingPassTemplate" }, Name{ "DebugRayTracingPass" });
        if (!m_rayTracingPass)
        {
            AZ_Assert(false, "Failed to create DebugRayTracingPass");
            return;
        }
        m_pipeline->AddPassAfter(m_rayTracingPass, Name{ "AuxGeomPass" });
    }

    void RayTracingDebugFeatureProcessor::RemoveDebugPass()
    {
        if (m_rayTracingPass)
        {
            m_rayTracingPass->QueueForRemoval();
            m_rayTracingPass.reset();
        }
    }
} // namespace AZ::Render
