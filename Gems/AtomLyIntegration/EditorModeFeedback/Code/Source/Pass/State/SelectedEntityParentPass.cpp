/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Pass/State/SelectedEntityParentPass.h>
#include <Pass/Child/EditorModeOutlinePass.h>

#include <AzToolsFramework/Entity/EditorEntityHelpers.h>

namespace AZ::Render
{
    // Name of the mask for selected entities
    static constexpr const char* const SelectedEntityMaskName = "editormodeselectedmask";

    //
    enum class SelectedEntityChildPass
    {
        EntityOutlinePass
    };

    //
    static PassDescriptorList CreateSelectedEntityChildPasses()
    {
        return PassDescriptorList
        {
            // Outline effect for the entities in the selected entity mask
            AZ::Name("EditorModeOutlineTemplate")
        };
    }
    

    SelectedEntityParentPass::SelectedEntityParentPass()
        : EditorStateParentPassBase("EntitySelection", CreateSelectedEntityChildPasses(), SelectedEntityMaskName)
    {
    }

    void SelectedEntityParentPass::InitPassData([[maybe_unused]] RPI::ParentPass* parentPass)
    {
        //auto* entityOutlinePass =
        //    FindChildPass<EditorModeOutlinePass>(parentPass, static_cast<std::size_t>(SelectedEntityChildPass::EntityOutlinePass));
        //
        //entityOutlinePass->SetLineColor()
    }

    AzToolsFramework::EntityIdList SelectedEntityParentPass::GetMaskedEntities() const
    {
        AzToolsFramework::EntityIdList selectedEntityList;
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
            selectedEntityList, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);

        return selectedEntityList;
    }
} // namespace AZ::Render
