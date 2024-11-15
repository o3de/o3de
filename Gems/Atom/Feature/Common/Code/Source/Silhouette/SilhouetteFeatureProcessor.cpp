/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Pass/Pass.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/Pass/RasterPass.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/DynamicDraw/DynamicDrawInterface.h>
#include <AzCore/Console/IConsole.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Scene/Scene.h>
#include <AzFramework/Scene/SceneSystemInterface.h>
#include <Silhouette/SilhouetteFeatureProcessor.h>

AZ_CVAR(
    bool,
    r_silhouette,
    false,
    nullptr,
    AZ::ConsoleFunctorFlags::Null,
    "Controls if the silhouette rendering feature is active.  0 : Inactive,  1 : Active (default)");

namespace AZ::Render
{
    SilhouetteFeatureProcessor::SilhouetteFeatureProcessor() = default;
    SilhouetteFeatureProcessor::~SilhouetteFeatureProcessor() = default;

    void SilhouetteFeatureProcessor::Reflect(ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<SilhouetteFeatureProcessor, FeatureProcessor>()->Version(0);
        }
    }

    void SilhouetteFeatureProcessor::Activate()
    {
        EnableSceneNotification();
    }

    void SilhouetteFeatureProcessor::Deactivate()
    {
        DisableSceneNotification();

        m_rasterPass = nullptr;
        m_compositePass = nullptr;
    }

    void SilhouetteFeatureProcessor::SetPassesEnabled(bool enabled)
    {
        if (m_compositePass && m_rasterPass)
        {
            m_compositePass->SetEnabled(enabled);
            m_rasterPass->SetEnabled(enabled);
        }
    }

    void SilhouetteFeatureProcessor::OnRenderEnd()
    {
        SetPassesEnabled(r_silhouette);
    }

    void SilhouetteFeatureProcessor::AddRenderPasses(RPI::RenderPipeline* renderPipeline)
    {
        // Early return if pass is already found in render pipeline or if the pipeline is not the default one.
        if (renderPipeline->GetViewType() != RPI::ViewType::Default)
        {
            return;
        }

        // the silhouette passes are already added in another render pipeline
        if (m_renderPipeline && m_renderPipeline != renderPipeline)
        {
            return;
        }

        UpdatePasses(renderPipeline);

        // if we already have valid render pass members then we don't need to dynamically
        // create them and add them to the pipeline
        if (m_compositePass && m_rasterPass)
        {
            return;
        }

        // unset the render pipeline until we add the passes successfully 
        m_renderPipeline = nullptr;

        const auto mergeTemplateName = Name("SilhouettePassTemplate");
        const auto gatherTemplateName = Name("SilhouetteGatherPassTemplate");

        Name postProcessPassName = Name("PostProcessPass");
        if (renderPipeline->FindFirstPass(postProcessPassName) == nullptr)
        {
            AZ_Warning("SilhouetteFeatureProcessor", false, "Can't find %s in the render pipeline.", postProcessPassName.GetCStr());
            return;
        }

        Name forwardProcessPassName = Name("Forward");
        if (renderPipeline->FindFirstPass(forwardProcessPassName) == nullptr)
        {
            AZ_Warning("SilhouetteFeatureProcessor", false, "Can't find %s in the render pipeline.", forwardProcessPassName.GetCStr());
            return;
        }

        // Add the gather pass which draws all the silhouette objects into a render target
        // using depth and stencil to determine where to draw
        RPI::PassRequest gatherPassRequest;
        gatherPassRequest.m_passName = Name("SilhouetteGatherPass");
        gatherPassRequest.m_templateName = gatherTemplateName;
        gatherPassRequest.m_passEnabled = r_silhouette;
        gatherPassRequest.AddInputConnection(RPI::PassConnection{
            Name("DepthStencilInputOutput"), RPI::PassAttachmentRef{ forwardProcessPassName, Name("DepthStencilInputOutput") } });

        if (auto pass = RPI::PassSystemInterface::Get()->CreatePassFromRequest(&gatherPassRequest); pass != nullptr)
        {
            m_rasterPass = static_cast<AZ::RPI::RasterPass*>(pass.get());
            renderPipeline->AddPassAfter(pass, forwardProcessPassName);
        }

        // Add the full screen silhouette pass which merges the silhouettes render target with
        // the framebuffer diffuse, and adds outlines to the silhouette shapes
        RPI::PassRequest compositePassRequest;
        compositePassRequest.m_passName = Name("SilhouettePass");
        compositePassRequest.m_templateName = mergeTemplateName;
        compositePassRequest.m_passEnabled = r_silhouette;
        compositePassRequest.AddInputConnection(
            RPI::PassConnection{ Name("InputOutput"), RPI::PassAttachmentRef{ postProcessPassName, Name("Output") } });

        if (auto pass = RPI::PassSystemInterface::Get()->CreatePassFromRequest(&compositePassRequest); pass != nullptr)
        {
            m_compositePass = pass.get();
            renderPipeline->AddPassAfter(pass, postProcessPassName);
        }

        // remember which render pipeline we added our passes to
        m_renderPipeline = renderPipeline;
    }

    void SilhouetteFeatureProcessor::OnRenderPipelineChanged(AZ::RPI::RenderPipeline* pipeline, AZ::RPI::SceneNotification::RenderPipelineChangeType changeType)
    {
        // Only need to recache silhouette passes if the pipeline had them
        if (pipeline == m_renderPipeline)
        {
            if (changeType == AZ::RPI::SceneNotification::RenderPipelineChangeType::Removed)
            {
                UpdatePasses(nullptr);
            }
            else
            {
                UpdatePasses(pipeline);
            }
        }
    }

    bool SilhouetteFeatureProcessor::NeedsCompositePass() const
    {
        if (auto scene = GetParentScene(); scene != nullptr)
        {
            if (m_rasterPass && m_rasterPass->GetRenderPipeline())
            {
                // Get DrawList from the dynamic draw interface and view
                AZStd::vector<RHI::DrawListView> drawLists = AZ::RPI::DynamicDrawInterface::Get()->GetDrawListsForPass(m_rasterPass);
                const AZStd::vector<AZ::RPI::ViewPtr>& views =
                    m_rasterPass->GetRenderPipeline()->GetViews(m_rasterPass->GetPipelineViewTag());
                RHI::DrawListView viewDrawList;
                if (!views.empty())
                {
                    const AZ::RPI::ViewPtr& view = views.front();
                    viewDrawList = view->GetDrawList(m_rasterPass->GetDrawListTag());
                }

                return !(drawLists.empty() && viewDrawList.empty());
            }
        }
        return false;
    }

    void SilhouetteFeatureProcessor::UpdatePasses(AZ::RPI::RenderPipeline* renderPipeline)
    {
        m_compositePass = nullptr;
        m_rasterPass = nullptr;

        if (renderPipeline == nullptr)
        {
            m_renderPipeline = nullptr;
            return;
        }

        const auto mergeTemplateName = Name("SilhouettePassTemplate");
        auto compositePassFilter = AZ::RPI::PassFilter::CreateWithTemplateName(mergeTemplateName, renderPipeline);
        if (auto foundPass = AZ::RPI::PassSystemInterface::Get()->FindFirstPass(compositePassFilter); foundPass)
        {
            m_compositePass = foundPass;
        }

        const auto gatherTemplateName = Name("SilhouetteGatherPassTemplate");
        auto gatherPassFilter = AZ::RPI::PassFilter::CreateWithTemplateName(gatherTemplateName, renderPipeline);
        if (auto foundPass = AZ::RPI::PassSystemInterface::Get()->FindFirstPass(gatherPassFilter); foundPass)
        {
            m_rasterPass = static_cast<AZ::RPI::RasterPass*>(foundPass);
        }

        // remember which render pipeline we found our passes on
        m_renderPipeline = (m_compositePass && m_rasterPass) ? renderPipeline : nullptr;
    }
} // namespace AZ::Render
