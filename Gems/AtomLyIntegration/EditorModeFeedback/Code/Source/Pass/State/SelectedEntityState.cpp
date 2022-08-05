/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Pass/State/SelectedEntityState.h>
#include <Pass/Child/EditorModeOutlinePass.h>

#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>

namespace AZ::Render
{
    // Name of the mask for selected entities
    static constexpr const char* const SelectedEntityMaskName = "editormodeselectedmask";

    // Indices of child passes (a better way to do this will be to specifiy a pass template name/effect name pair for the
    // PassNameList so we can lookup child passes by their effect name rather than having to maintain parity between
    // their ordering in the PassNameList and in this enum but that is a problem that will be addressed in the next
    // version where we remove the CVARs and have the editor state effects configured via menus and registry, see the note
    // in UpdatePassData below)
    enum class SelectedEntityChildPass
    {
        EntityOutlinePass
    };

    // Helper function to construct the pass descriptor list for this editor state effect.
    static PassNameList CreateSelectedEntityChildPasses()
    {
        return PassNameList
        {
            // Outline effect for the entities in the selected entity mask
            AZ::Name("EditorModeOutlineTemplate")
        };
    }

    SelectedEntityState::SelectedEntityState()
        : EditorStateBase(EditorState::EntitySelection, "EntitySelection", CreateSelectedEntityChildPasses(), SelectedEntityMaskName)
    {
    }

    void SelectedEntityState::UpdatePassData([[maybe_unused]] RPI::ParentPass* parentPass)
    {
        // Note: this is an example of how the state passes configure their child passes to tailor the effects in response to
        // settigns menus etc. Right now they can't be set here as the temporary CVARS are hogging the pass configuration
        //auto* entityOutlinePass =
        //    FindChildPass<EditorModeOutlinePass>(parentPass, static_cast<std::size_t>(SelectedEntityChildPass::EntityOutlinePass));
        //
        //if (entityOutlinePass)
        //{
        //    entityOutlinePass->SetLineColor(AZ::Color::CreateFromRgba(0, 0, 255, 255));
        //}
    }

    AzToolsFramework::EntityIdList SelectedEntityState::GetMaskedEntities() const
    {
        AzToolsFramework::EntityIdList initialSelectedEntityList, selectedEntityList;
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
            initialSelectedEntityList, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);

        // Drill down any entity hierarchies to select all children of the currently selected entities 
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
