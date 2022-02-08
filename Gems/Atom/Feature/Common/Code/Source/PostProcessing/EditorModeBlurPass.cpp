/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 
#include <PostProcessing/EditorModeBlurPass.h>
#include <PostProcess/PostProcessFeatureProcessor.h>

#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/Feature/PostProcess/EditorModeFeedback/EditorModeFeedbackInterface.h>

 namespace AZ
{
    namespace Render
    {
        RPI::Ptr<EditorModeBlurPass> EditorModeBlurPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<EditorModeBlurPass> pass = aznew EditorModeBlurPass(descriptor);
            return AZStd::move(pass);
        }
        
        EditorModeBlurPass::EditorModeBlurPass(const RPI::PassDescriptor& descriptor)
            : AZ::RPI::FullscreenTrianglePass(descriptor)
        {
        }
        
        void EditorModeBlurPass::InitializeInternal()
        {
            FullscreenTrianglePass::InitializeInternal();
        }
        
        void EditorModeBlurPass::FrameBeginInternal(FramePrepareParams params)
        {
            FullscreenTrianglePass::FrameBeginInternal(params);
        }

        bool EditorModeBlurPass::IsEnabled() const
        {
            // move this to parent
            if (const auto editorModeFeedback = AZ::Interface<EditorModeFeedbackInterface>::Get())
            {
                return editorModeFeedback->IsEnabled();
            }

            return false;
        }
    }
}
