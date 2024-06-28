/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/RenderPipeline.h>
#include <Debug/RayTracingDebugFeatureProcessor.h>

#include <Atom/RPI.Public/Pass/PassFilter.h>

namespace AZ::Render
{
    void RayTracingDebugFeatureProcessor::Reflect(ReflectContext* context)
    {
        if (auto serializeContext{ azrtti_cast<SerializeContext*>(context) })
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
        m_debugComponentCount++;
        AddOrRemoveDebugPass();
    }

    void RayTracingDebugFeatureProcessor::OnRayTracingDebugComponentRemoved()
    {
        m_debugComponentCount--;
        AddOrRemoveDebugPass();
    }

    void RayTracingDebugFeatureProcessor::Activate()
    {
        m_settings = AZStd::make_unique<RayTracingDebugSettings>();

        FeatureProcessor::Activate();
        EnableSceneNotification();
    }

    void RayTracingDebugFeatureProcessor::Deactivate()
    {
        EnableSceneNotification();
        FeatureProcessor::Deactivate();

        m_settings.reset();
    }

    void RayTracingDebugFeatureProcessor::AddRenderPasses(RPI::RenderPipeline* pipeline)
    {
        m_pipeline = pipeline;
        FeatureProcessor::AddRenderPasses(pipeline);
    }

    void RayTracingDebugFeatureProcessor::AddOrRemoveDebugPass()
    {
        bool shouldAddPass{ !m_rayTracingPass && m_debugComponentCount > 0 };
        bool shouldRemovePass{ m_rayTracingPass && m_debugComponentCount == 0 };

        if (shouldAddPass)
        {
            m_rayTracingPass = RPI::PassSystemInterface::Get()->CreatePassFromTemplate(
                Name{ "DebugRayTracingPassTemplate" }, Name{ "DebugRayTracingPass" });
            if (!m_rayTracingPass)
            {
                AZ_Assert(false, "Failed to create DebugRayTracingPass");
                return;
            }
            // m_pipeline->AddPassBefore(m_rayTracingPass, Name{ "AuxGeomPass" });
            m_pipeline->AddPassAfter(m_rayTracingPass, Name{ "PostProcessPass" });
        }

        if (shouldRemovePass)
        {
            m_rayTracingPass->QueueForRemoval();
            m_rayTracingPass.reset();
        }
    }
} // namespace AZ::Render
