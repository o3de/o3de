/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Outline/OutlineFeatureProcessor.h>
#include <AzCore/Console/IConsole.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Pass/Pass.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Scene/Scene.h>
#include <AzFramework/Scene/SceneSystemInterface.h>

void OnOutlineActiveChanged(const bool& activate)
{
    AzFramework::EntityContextId entityContextId;
    AzFramework::GameEntityContextRequestBus::BroadcastResult(
        entityContextId, &AzFramework::GameEntityContextRequestBus::Events::GetGameEntityContextId);

    if (auto scene = AZ::RPI::Scene::GetSceneForEntityContextId(entityContextId); scene != nullptr)
    {
        // avoid unnecessary enable/disable to avoid warning log spam
        bool alreadyActive = scene->GetFeatureProcessor<AZ::Render::OutlineFeatureProcessor>() != nullptr;
        if (activate && !alreadyActive)
        {
            scene->EnableFeatureProcessor<AZ::Render::OutlineFeatureProcessor>();
        }
        else if (!activate && alreadyActive)
        {
            scene->DisableFeatureProcessor<AZ::Render::OutlineFeatureProcessor>();
        }
    }
}

AZ_CVAR( bool, r_outline, true, &OnOutlineActiveChanged, AZ::ConsoleFunctorFlags::Null,
    "Controls if the outline rendering feature is active.  0 : Inactive,  1 : Active (default)");

namespace AZ::Render
{
    OutlineFeatureProcessor::OutlineFeatureProcessor() = default;
    OutlineFeatureProcessor::~OutlineFeatureProcessor() = default;

    void OutlineFeatureProcessor::Reflect(ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext
                ->Class<OutlineFeatureProcessor, FeatureProcessor>()
                ->Version(0);
        }
    }
    
    void OutlineFeatureProcessor::Activate()
    {
        EnableSceneNotification();
    }
    
    void OutlineFeatureProcessor::Deactivate()
    {
        DisableSceneNotification();
    }

    void OutlineFeatureProcessor::AddRenderPasses(RPI::RenderPipeline* renderPipeline)
    {
        // Early return if pass is already found in render pipeline or if the pipeline is not the default one.
        if (renderPipeline->GetViewType() != RPI::ViewType::Default)
        {
            return;
        }

        const auto mergeTemplateName = Name("OutlinePassTemplate");
        auto mergePassFilter = AZ::RPI::PassFilter::CreateWithTemplateName(mergeTemplateName, renderPipeline);
        if(auto foundPass = AZ::RPI::PassSystemInterface::Get()->FindFirstPass(mergePassFilter); foundPass)
        {
            return;
        }

        const auto gatherTemplateName = Name("OutlineGatherPassTemplate");
        auto gatherPassFilter = AZ::RPI::PassFilter::CreateWithTemplateName(gatherTemplateName, renderPipeline);
        if(auto foundPass = AZ::RPI::PassSystemInterface::Get()->FindFirstPass(gatherPassFilter); foundPass)
        {
            return;
        }

        Name postProcessPassName = Name("PostProcessPass");
        if (renderPipeline->FindFirstPass(postProcessPassName) == nullptr)
        {
            AZ_Warning("OutlineFeatureProcessor", false,
                "Can't find %s in the render pipeline.", postProcessPassName.GetCStr());
            return;
        }

        Name forwardProcessPassName = Name("Forward");
        if (renderPipeline->FindFirstPass(forwardProcessPassName) == nullptr)
        {
            AZ_Warning("OutlineFeatureProcessor", false,
                "Can't find %s in the render pipeline.", forwardProcessPassName.GetCStr());
            return;
        }

        // Gather
        RPI::PassRequest gatherPassRequest;
        gatherPassRequest.m_passName = Name("OutlineGatherPass");
        gatherPassRequest.m_templateName = gatherTemplateName;
        gatherPassRequest.AddInputConnection( RPI::PassConnection{
            Name("Input"),
            RPI::PassAttachmentRef{
                forwardProcessPassName, Name("DiffuseOutput")
            }
        });
        gatherPassRequest.AddInputConnection( RPI::PassConnection{
            Name("DepthStencilInputOutput"),
            RPI::PassAttachmentRef{
                forwardProcessPassName, Name("DepthStencilInputOutput")
            }
        });

        if (auto pass = RPI::PassSystemInterface::Get()->CreatePassFromRequest(&gatherPassRequest); pass != nullptr)
        {
            renderPipeline->AddPassAfter(pass, forwardProcessPassName);
        }

        // Merge
        RPI::PassRequest mergePassRequest;
        mergePassRequest.m_passName = Name("OutlinePass");
        mergePassRequest.m_templateName = mergeTemplateName;
        mergePassRequest.AddInputConnection( RPI::PassConnection{
            Name("InputOutput"),
            RPI::PassAttachmentRef{
                postProcessPassName, Name("Output")
            }
        });

        if (auto pass = RPI::PassSystemInterface::Get()->CreatePassFromRequest(&mergePassRequest); pass != nullptr)
        {
            renderPipeline->AddPassAfter(pass, postProcessPassName);
        }
    }
}

