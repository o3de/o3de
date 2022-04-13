/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EditorModeFeedbackFeatureProcessor.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>

// Temporary measure for configuring editor mode feedback effects at runtime until GHI 3455 is implemented
#include <AzCore/Console/IConsole.h>
#define AZ_EDITOR_MODE_PASS_CVAR(TYPE, NAMESPACE, NAME, INITIAL_VALUE)                                                                     \
    AZ_CVAR(TYPE, NAMESPACE##_##NAME, INITIAL_VALUE, nullptr, AZ::ConsoleFunctorFlags::Null, "");

// Temporary measure for configuring editor mode depth transitions at runtime until GHI 3455 is implemented
#define AZ_EDITOR_MODE_PASS_TRANSITION_CVARS(NAMESPACE, MIN_VALUE, START, DURATION, FINAL_BLEND)                                           \
    AZ_EDITOR_MODE_PASS_CVAR(float, NAMESPACE, MinDepthTransitionValue, MIN_VALUE);                                                        \
    AZ_EDITOR_MODE_PASS_CVAR(float, NAMESPACE, DepthTransitionStart, START);                                                               \
    AZ_EDITOR_MODE_PASS_CVAR(float, NAMESPACE, DepthTransitionDuration, DURATION);                                                         \
    AZ_EDITOR_MODE_PASS_CVAR(float, NAMESPACE, FinalBlendAmount, FINAL_BLEND);

// Temporary measure for setting the color tint pass shader parameters at runtime until GHI 3455 is implemented
AZ_EDITOR_MODE_PASS_TRANSITION_CVARS(cl_editorModeTintPass, 0.0f, 0.0f, 0.0f, 1.0f);
AZ_EDITOR_MODE_PASS_CVAR(float, cl_editorModeTintPass, TintAmount, 0.5f);
AZ_EDITOR_MODE_PASS_CVAR(AZ::Color, cl_editorModeTintPass, TintColor, AZ::Color(0.0f, 0.0f, 0.0f, 0.0f));

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

        void EditorModeFeatureProcessor::Render([[maybe_unused]] const FeatureProcessor::RenderPacket& packet)
        {
            //AZ_Printf("EditorModeFeatureProcessor", "Render");
        }

        void EditorModeFeatureProcessor::OnRenderPipelineAdded(RPI::RenderPipelinePtr pipeline)
        {
            InitPasses(pipeline.get());
        }

        void EditorModeFeatureProcessor::OnRenderPipelinePassesChanged([[maybe_unused]] RPI::RenderPipeline* renderPipeline)
        {
            InitPasses(renderPipeline);
        }

        void EditorModeFeatureProcessor::OnBeginPrepareRender()
        {
            
        }

        void EditorModeFeatureProcessor::InitPasses([[maybe_unused]] RPI::RenderPipeline* renderPipeline)
        {
            RPI::PassFilter tintPassFilter = RPI::PassFilter::CreateWithPassName(AZ::Name{ "TintPass" }, renderPipeline);
            RPI::Ptr<RPI::Pass> tintPass = RPI::PassSystemInterface::Get()->FindFirstPass(tintPassFilter);
            
            if (tintPass)
            {
                m_tintFullscreenTrianglePass = azdynamic_cast<RPI::FullscreenTrianglePass*>(tintPass.get());
            }
        }

        void EditorModeFeatureProcessor::Simulate([[maybe_unused]] const SimulatePacket& packet)
        {
            if (m_tintFullscreenTrianglePass)
            {
                RHI::ShaderInputNameIndex minDepthTransitionValueIndex = "m_minDepthTransitionValue";
                RHI::ShaderInputNameIndex depthTransitionStartIndex = "m_depthTransitionStart";
                RHI::ShaderInputNameIndex depthTransitionDurationIndex = "m_depthTransitionDuration";
                RHI::ShaderInputNameIndex finalBlendAmountIndex = "m_finalBlendAmount";
                RHI::ShaderInputNameIndex tintAmountIndex = "m_tintAmount";
                RHI::ShaderInputNameIndex tintColorIndex = "m_tintColor";

                auto srg = m_tintFullscreenTrianglePass->GetShaderResourceGroup();
                srg->SetConstant(minDepthTransitionValueIndex, (float)cl_editorModeTintPass_MinDepthTransitionValue);
                srg->SetConstant(depthTransitionStartIndex, (float)cl_editorModeTintPass_DepthTransitionStart);
                srg->SetConstant(depthTransitionDurationIndex, (float)cl_editorModeTintPass_DepthTransitionDuration);
                srg->SetConstant(finalBlendAmountIndex, (float)cl_editorModeTintPass_FinalBlendAmount);
                srg->SetConstant(tintAmountIndex, (float)cl_editorModeTintPass_TintAmount);
                srg->SetConstant(tintColorIndex, (AZ::Color)cl_editorModeTintPass_TintColor);
            }
        }
    } // namespace Render
} // namespace AZ
