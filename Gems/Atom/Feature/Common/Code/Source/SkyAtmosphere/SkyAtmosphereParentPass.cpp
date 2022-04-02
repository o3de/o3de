/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SkyAtmosphere/SkyAtmosphereParentPass.h>

namespace AZ::Render
{
    SkyAtmosphereParentPass::SkyAtmosphereParentPass(const RPI::PassDescriptor& descriptor)
        : Base(descriptor)
    {
    }

    RPI::Ptr<SkyAtmosphereParentPass> SkyAtmosphereParentPass::Create(const RPI::PassDescriptor& descriptor)
    {
        return aznew SkyAtmosphereParentPass(descriptor);
    }

    void SkyAtmosphereParentPass::CreateAtmospherePass(SkyAtmosphereFeatureProcessorInterface::AtmosphereId id)
    {
        // make sure a pass doesn't already exist for this id
        auto pass = GetPass(id);
        if (pass)
        {
            return;
        }

        pass = SkyAtmospherePass::CreateWithPassRequest(id);
        if (pass)
        {
            pass->QueueForBuildAndInitialization();
            AddChild(pass);
        }
    }

    void SkyAtmosphereParentPass::ReleaseAtmospherePass(SkyAtmosphereFeatureProcessorInterface::AtmosphereId id)
    {
        if (auto pass = GetPass(id))
        {
            RemoveChild(pass);
        }
    }

    void SkyAtmosphereParentPass::UpdateAtmospherePassSRG(SkyAtmosphereFeatureProcessorInterface::AtmosphereId id, const SkyAtmospherePass::AtmosphereParams& params)
    {
        if (auto pass = GetPass(id))
        {
            pass->UpdateRenderPassSRG(params);
        }
    }

    RPI::Ptr<SkyAtmospherePass> SkyAtmosphereParentPass::GetPass(SkyAtmosphereFeatureProcessorInterface::AtmosphereId id) const
    {
        for (const RPI::Ptr<Pass>& child : m_children)
        {
            SkyAtmospherePass* pass = azrtti_cast<SkyAtmospherePass*>(child.get());
            if (pass && pass->GetAtmosphereId() == id)
            {
                return pass;
            }
        }
        return nullptr;
    }

} // namespace AZ::Render

