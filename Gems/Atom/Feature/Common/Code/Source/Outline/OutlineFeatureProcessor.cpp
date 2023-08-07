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
        const auto templateName = Name("OutlinePassTemplate");

        // Early return if pass is already found in render pipeline or if the pipeline is not the default one.
        auto passFilter = AZ::RPI::PassFilter::CreateWithTemplateName(templateName, renderPipeline);
        auto foundPass = AZ::RPI::PassSystemInterface::Get()->FindFirstPass(passFilter);
        if (foundPass || renderPipeline->GetViewType() != RPI::ViewType::Default)
        {
            return;
        }

        // check if the reference pass of insert position exist
        Name postProcessPassName = Name("PostProcessPass");
        if (renderPipeline->FindFirstPass(postProcessPassName) == nullptr)
        {
            AZ_Warning("OutlineFeatureProcessor", false,
                "Can't find %s in the render pipeline.", postProcessPassName.GetCStr());
            return;
        }

        RPI::PassRequest passRequest;
        passRequest.m_passName = Name("OutlinePass");
        passRequest.m_templateName = templateName;
        passRequest.AddInputConnection( RPI::PassConnection{
            Name("InputOutput"),
            RPI::PassAttachmentRef{
                postProcessPassName, Name("Output")
            }
        });

        if (auto pass = RPI::PassSystemInterface::Get()->CreatePassFromRequest(&passRequest); pass != nullptr)
        {
            renderPipeline->AddPassAfter(pass, postProcessPassName);
        }
    }
}

