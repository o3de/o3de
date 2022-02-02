/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 
#include <PostProcessing/EditorModeFeedbackPass.h>
#include <PostProcess/PostProcessFeatureProcessor.h>

#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/Feature/PostProcess/EditorModeFeedback/EditorModeFeedbackInterface.h>

 namespace AZ
{
    namespace Render
    {
        RPI::Ptr<EditorModeFeedbackPass> EditorModeFeedbackPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<EditorModeFeedbackPass> pass = aznew EditorModeFeedbackPass(descriptor);
            return AZStd::move(pass);
        }
        
        EditorModeFeedbackPass::EditorModeFeedbackPass(const RPI::PassDescriptor& descriptor)
            : AZ::RPI::FullscreenTrianglePass(descriptor)
        {
        }
        
        void EditorModeFeedbackPass::InitializeInternal()
        {
            FullscreenTrianglePass::InitializeInternal();

            m_desaturationAmountIndex.Reset();
        }
        
        void EditorModeFeedbackPass::FrameBeginInternal(FramePrepareParams params)
        {
            SetSrgConstants();

            FullscreenTrianglePass::FrameBeginInternal(params);
        }

        bool EditorModeFeedbackPass::IsEnabled() const
        {
            // move this to parent
            if (const auto editorModeFeedback = AZ::Interface<EditorModeFeedbackInterface>::Get())
            {
                return editorModeFeedback->Enabled();
            }

            return false;
        }

        void EditorModeFeedbackPass::SetSrgConstants()
        {
           m_shaderResourceGroup->SetConstant(m_desaturationAmountIndex, 1.0f);
        }
    }
}
