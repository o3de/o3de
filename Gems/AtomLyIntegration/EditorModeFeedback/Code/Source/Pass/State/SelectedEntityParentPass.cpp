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
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>

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
        : EditorStateParentPassBase(EditorState::EntitySelection, "EntitySelection", CreateSelectedEntityChildPasses(), SelectedEntityMaskName)
    {
        SetEnabled(true);
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
        AzToolsFramework::EntityIdList initialSelectedEntityList, selectedEntityList;
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
            initialSelectedEntityList, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);


        // iterate over selected entities and check if any are container (IsContainer) 
        // c&p FocusModeSystemComponent::RefreshFocusedEntityIdList() code to get all descendants the container
        //
        // UNLESS all heirachies will be outlined accordingly for children
        // just do for every entity and get all descendants (iterate over all children entities recursivley as per FocusModeSystemComponent::RefreshFocusedEntityIdList())

        for (const auto& selectedEntityId : initialSelectedEntityList)
        {
            AZStd::queue<AZ::EntityId> entityIdQueue;
            entityIdQueue.push(selectedEntityId);

            while (!entityIdQueue.empty())
            {
                AZ::EntityId entityId = entityIdQueue.front();
                entityIdQueue.pop();

                if (entityId.IsValid())
                {
                    selectedEntityList.push_back(entityId);
                }

                AzToolsFramework::EntityIdList children;
                AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
                    children, entityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetChildren);

                for (AZ::EntityId childEntityId : children)
                {
                    entityIdQueue.push(childEntityId);
                }
            }
        }

        return selectedEntityList;
    }
} // namespace AZ::Render
