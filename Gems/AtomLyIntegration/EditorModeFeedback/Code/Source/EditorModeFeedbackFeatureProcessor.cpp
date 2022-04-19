/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EditorModeFeedbackFeatureProcessor.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>

namespace AZ
{
    namespace Render
    {
        void EditorModeFeatureProcessor::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<EditorModeFeatureProcessor, RPI::FeatureProcessor>()->Version(0);
            }
        }

        void EditorModeFeatureProcessor::Activate()
        {
            //AZ_Printf("EditorModeFeatureProcessor", "Activate");
            EnableSceneNotification();
        }

        void EditorModeFeatureProcessor::Deactivate()
        {
            //AZ_Printf("EditorModeFeatureProcessor", "Deactivate");
            DisableSceneNotification();
            m_parentPassRequestAsset.Reset();
        }

        void EditorModeFeatureProcessor::ApplyRenderPipelineChange([[maybe_unused]]RPI::RenderPipeline* renderPipeline)
        {
            //AZ_Printf("EditorModeFeatureProcessor", "ApplyRenderPipelineChange");
            const char* passRequestAssetFilePath = "Passes/EditorModeFeedback_PassRequest.azasset";
            m_parentPassRequestAsset = AZ::RPI::AssetUtils::LoadAssetByProductPath<AZ::RPI::AnyAsset>(
                passRequestAssetFilePath, AZ::RPI::AssetUtils::TraceLevel::Warning);
            const AZ::RPI::PassRequest* passRequest = nullptr;
            if (m_parentPassRequestAsset->IsReady())
            {
                passRequest = m_parentPassRequestAsset->GetDataAs<AZ::RPI::PassRequest>();
            }
            if (!passRequest)
            {
                AZ_Error("EditorModeFeedback", false, "Failed to add editor mode feedback parent pass. Can't load PassRequest from %s", passRequestAssetFilePath);
                //return false;
            }

            // Create the pass
            RPI::Ptr<RPI::Pass> parentPass = RPI::PassSystemInterface::Get()->CreatePassFromRequest(passRequest);

            if (!parentPass)
            {
                AZ_Error("EditorModeFeedback", false, "Create parent pass from pass request failed", renderPipeline->GetId().GetCStr());
                //return false;
            }

            // Add the pass to render pipeline
            bool success = renderPipeline->AddPassAfter(parentPass, Name("PostProcessPass"));
            // only create pass resources if it was success
            if (success)
            {
                //CreatePerPassResources();
                AZ_Printf("Foo", "Awesomse!");
            }
            else
            {
                AZ_Error(
                    "EditorModeFeedback", false, "Add the parent pass to render pipeline [%s] failed", renderPipeline->GetId().GetCStr());
            }


            // return success;
        }
    } // namespace Render
} // namespace AZ
