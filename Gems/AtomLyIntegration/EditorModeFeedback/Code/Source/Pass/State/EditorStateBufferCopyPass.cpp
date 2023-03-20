/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Pass/State/EditorStateBufferCopyPass.h>
#include <Pass/State/EditorStateBufferCopyPassData.h>

#include <Atom/RPI.Public/Pass/PassUtils.h>

namespace AZ::Render
{
    RPI::Ptr<EditorStateBufferCopyPass> EditorStateBufferCopyPass::Create(const RPI::PassDescriptor& descriptor)
    {
        RPI::Ptr<EditorStateBufferCopyPass> pass = aznew EditorStateBufferCopyPass(descriptor);
        return AZStd::move(pass);
    }
    
    EditorStateBufferCopyPass::EditorStateBufferCopyPass(const RPI::PassDescriptor& descriptor)
        : AZ::RPI::FullscreenTrianglePass(descriptor)
        , m_passDescriptor(descriptor)
    {
    }

    bool EditorStateBufferCopyPass::IsEnabled() const
    {
        const RPI::EditorStateBufferCopyPassData* passData =
            RPI::PassUtils::GetPassData<RPI::EditorStateBufferCopyPassData>(m_passDescriptor);
            
        if (passData == nullptr)
        {
            AZ_Error(
                "EditorStateBufferCopyPass", false, "[EditorStateBufferCopyPassData '%s']: Trying to construct without valid EditorStateBufferCopyPassData!", GetPathName().GetCStr());
            return false;
        }
        return passData->editorStatePass->IsEnabled();
    }
} // namespace AZ::Render
