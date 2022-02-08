/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 
#include <PostProcessing/EditorModeOutlinePass.h>
#include <PostProcess/PostProcessFeatureProcessor.h>

#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/Feature/PostProcess/EditorModeFeedback/EditorModeFeedbackInterface.h>

 namespace AZ
{
    namespace Render
    {
        RPI::Ptr<EditorModeOutlinePass> EditorModeOutlinePass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<EditorModeOutlinePass> pass = aznew EditorModeOutlinePass(descriptor);
            return AZStd::move(pass);
        }
        
        EditorModeOutlinePass::EditorModeOutlinePass(const RPI::PassDescriptor& descriptor)
            : AZ::RPI::FullscreenTrianglePass(descriptor)
        {
        }
        
        void EditorModeOutlinePass::InitializeInternal()
        {
            FullscreenTrianglePass::InitializeInternal();
        }
        
        void EditorModeOutlinePass::FrameBeginInternal(FramePrepareParams params)
        {
            FullscreenTrianglePass::FrameBeginInternal(params);
        }

        bool EditorModeOutlinePass::IsEnabled() const
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
