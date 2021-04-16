/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
