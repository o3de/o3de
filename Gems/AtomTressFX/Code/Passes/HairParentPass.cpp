/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/DrawListTagRegistry.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/RenderPipeline.h>

namespace AZ
{
    namespace Render
    {
        HairParentPass::HairParentPass(const RPI::PassDescriptor& descriptor)
            : Base(descriptor)
        {
        }

        HairParentPass::~HairParentPass()
        {
        }

        RPI::Ptr<HairParentPass> HairParentPass::Create(const RPI::PassDescriptor& descriptor)
        {
            return aznew HairParentPass(descriptor);
        }

        // The following two methods are here as the mean to allow usage of different hair passes in
        // the active pipeline due to different hair options activations.
        // For example - one might want to use short resolve render method vs' the complete full buffers
        // that is used currently (but cost much memory), or to enable/disable collisions by removing
        // the collision passes.
        // [To Do] - The parent pass class can be removed if this is not done. 
        void HairParentPass::UpdateChildren()
        {
            if (!m_updateChildren)
            {
                return;
            }
            m_updateChildren = false;
        }

        void HairParentPass::BuildAttachmentsInternal()
        {
            UpdateChildren();

            Base::BuildAttachmentsInternal();
        }

    } // namespace Render
} // namespace AZ
