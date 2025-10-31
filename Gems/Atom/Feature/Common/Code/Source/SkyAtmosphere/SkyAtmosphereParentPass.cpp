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
        if (m_atmosphereIds.contains(id))
        {
            return;
        }

        m_atmosphereIds.insert(id);

        m_flags.m_createChildren = true;

        QueueForBuildAndInitialization();
    }

    void SkyAtmosphereParentPass::ReleaseAtmospherePass(SkyAtmosphereFeatureProcessorInterface::AtmosphereId id)
    {
        if (m_atmosphereIds.contains(id))
        {
            m_atmosphereIds.erase(id);

            if (auto pass = GetPass(id))
            {
                pass->QueueForRemoval();
            }
        }
    }

    void SkyAtmosphereParentPass::UpdateAtmospherePassSRG(SkyAtmosphereFeatureProcessorInterface::AtmosphereId id, const SkyAtmosphereParams& params)
    {
        // child passes should already be built because UpdateAtmospherePassSRG is called from Render()
        // which is run after CreateChildPassesInternal() 
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


    void SkyAtmosphereParentPass::CreateChildPassesInternal()
    {
        for(const auto& id : m_atmosphereIds)
        {
            auto pass = GetPass(id);
            if (pass)
            {
                AZ_Error("SkyAtmosphereParentPass", false, "Trying to create a SkyAtmospherePass that already exists");
                continue;
            }

            pass = SkyAtmospherePass::CreateWithPassRequest(id);
            if (pass)
            {
                AddChild(pass);
            }
        }
    }

} // namespace AZ::Render

