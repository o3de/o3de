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
#include <Atom/RPI.Reflect/Pass/RasterPassData.h>

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
