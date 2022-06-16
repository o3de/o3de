/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcess/PostProcessFeatureProcessor.h>
#include <Pass/EditorModeFeedbackParentPass.h>

#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <EditorModeFeedback/EditorModeFeedbackInterface.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<EditorModeFeedbackParentPass> EditorModeFeedbackParentPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<EditorModeFeedbackParentPass> pass = aznew EditorModeFeedbackParentPass(descriptor);
            return AZStd::move(pass);
        }

        EditorModeFeedbackParentPass::EditorModeFeedbackParentPass(const RPI::PassDescriptor& descriptor)
            : AZ::RPI::ParentPass(descriptor)
        {
        }

        bool EditorModeFeedbackParentPass::IsEnabled() const
        {
            if (const auto editorModeFeedback = AZ::Interface<EditorModeFeedbackInterface>::Get())
            {
                return ParentPass::IsEnabled() && editorModeFeedback->IsEnabled();
            }

            return false;
        }
    } // namespace Render
} // namespace AZ
