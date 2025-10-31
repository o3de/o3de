/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Pass/State/EditorStateParentPass.h>
#include <Pass/State/EditorStateParentPassData.h>

#include <Atom/RPI.Public/Pass/PassUtils.h>

namespace AZ::Render
{
    RPI::Ptr<EditorStateParentPass> EditorStateParentPass::Create(const RPI::PassDescriptor& descriptor)
    {
        RPI::Ptr<EditorStateParentPass> pass = aznew EditorStateParentPass(descriptor);
        return AZStd::move(pass);
    }

    EditorStateParentPass::EditorStateParentPass(const RPI::PassDescriptor& descriptor)
        : AZ::RPI::ParentPass(descriptor)
        , m_passDescriptor(descriptor)
    {
    }

    bool EditorStateParentPass::IsEnabled() const
    {
        const RPI::EditorStateParentPassData* passData =
            RPI::PassUtils::GetPassData<RPI::EditorStateParentPassData>(m_passDescriptor);
        if (passData == nullptr)
        {
                AZ_Error(
                    "EditorStateParentPass", false, "[EditorStateParentPassData '%s']: Trying to construct without valid EditorStateParentPassData!", GetPathName().GetCStr());
                return false;
        }
        return passData->editorStatePass->IsEnabled();
    }
} // namespace AZ::Render
