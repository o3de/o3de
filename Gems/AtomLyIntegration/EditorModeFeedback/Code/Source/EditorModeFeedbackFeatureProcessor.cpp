/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EditorModeFeedbackFeatureProcessor.h>
#include <Pass/State/FocusedEntityParentPass.h>
#include <Pass/State/SelectedEntityParentPass.h>

#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>

namespace AZ
{
    namespace Render
    {
        namespace
        {
            const char* const Window = "EditorModeFeedback";
        }

        void EditorModeFeatureProcessor::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<EditorModeFeatureProcessor, RPI::FeatureProcessor>()->Version(0);
            }
        }

        void EditorModeFeatureProcessor::Activate()
        {
            EnableSceneNotification();

            EditorStateParentPassList editorStatePasses;
            editorStatePasses.push_back(AZStd::make_unique<FocusedEntityParentPass>());
            editorStatePasses.push_back(AZStd::make_unique<SelectedEntityParentPass>());
            m_editorStatePassSystem = AZStd::make_unique<EditorStatePassSystem>(AZStd::move(editorStatePasses));
        }

        void EditorModeFeatureProcessor::Deactivate()
        {
            m_editorStatePassSystem.reset();
            DisableSceneNotification();
            m_parentPassRequestAsset.Reset();
        }

        void EditorModeFeatureProcessor::ApplyRenderPipelineChange(RPI::RenderPipeline* renderPipeline)
        {
            m_editorStatePassSystem->AddPassesToRenderPipeline(renderPipeline);
        }
    } // namespace Render
} // namespace AZ
