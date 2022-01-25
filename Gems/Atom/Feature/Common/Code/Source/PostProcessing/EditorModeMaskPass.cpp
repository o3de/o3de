/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 
#include <PostProcessing/EditorModeMaskPass.h>
#include <PostProcess/PostProcessFeatureProcessor.h>

#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>

 namespace AZ
{
    namespace Render
    {
        RPI::Ptr<EditorModeMaskPass> EditorModeMaskPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<EditorModeMaskPass> pass = aznew EditorModeMaskPass(descriptor);
            return AZStd::move(pass);
        }
        
        EditorModeMaskPass::EditorModeMaskPass(const RPI::PassDescriptor& descriptor)
            : AZ::RPI::RasterPass(descriptor)
        {
        }
        
        void EditorModeMaskPass::InitializeInternal()
        {
            RasterPass::InitializeInternal();
        }
        
        void EditorModeMaskPass::FrameBeginInternal(FramePrepareParams params)
        {
            RasterPass::FrameBeginInternal(params);
        }

        bool EditorModeMaskPass::IsEnabled() const
        {
            return true;
        }
    }
}
