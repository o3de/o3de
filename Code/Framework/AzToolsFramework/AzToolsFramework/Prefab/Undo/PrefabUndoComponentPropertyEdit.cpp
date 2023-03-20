/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Prefab/Instance/InstanceEntityMapperInterface.h>
#include <AzToolsFramework/Prefab/Instance/InstanceToTemplateInterface.h>
#include <AzToolsFramework/Prefab/Undo/PrefabUndoUtils.h>
#include <AzToolsFramework/Prefab/Undo/PrefabUndoComponentPropertyEdit.h>

namespace AzToolsFramework::Prefab
{
    PrefabUndoComponentPropertyEdit::PrefabUndoComponentPropertyEdit(const AZStd::string& undoOperationName)
        : PrefabUndoBase(undoOperationName)
    {
        m_instanceEntityMapperInterface = AZ::Interface<InstanceEntityMapperInterface>::Get();
        AZ_Assert(m_instanceEntityMapperInterface, "Failed to grab instance entity mapper interface");
    }

    void PrefabUndoComponentPropertyEdit::Capture(
        const PrefabDomValue& initialState,
        const PrefabDomValue& endState,
        AZ::EntityId entityId,
        AZStd::string_view pathToComponentProperty,
        bool updateCache)
    {
        // get the entity alias for future undo/redo
        auto instanceReference = m_instanceEntityMapperInterface->FindOwningInstance(entityId);
        AZ_Error(
            "Prefab", instanceReference, "Failed to find an owning instance for the entity with id %llu.", static_cast<AZ::u64>(entityId));
        Instance& instance = instanceReference->get();
        m_templateId = instance.GetTemplateId();


        // generate undo/redo patches
        PrefabUndoUtils::GenerateUpdateEntityPatch(m_redoPatch, initialState, endState, pathToComponentProperty);
        PrefabUndoUtils::GenerateUpdateEntityPatch(m_undoPatch, endState, initialState, pathToComponentProperty);

        // Preemptively updates the cached DOM to prevent reloading instance DOM.
        if (updateCache)
        {
            PrefabDomReference cachedOwningInstanceDom = instance.GetCachedInstanceDom();
            if (cachedOwningInstanceDom.has_value())
            {
                PrefabUndoUtils::UpdateEntityInInstanceDom(cachedOwningInstanceDom, endState, pathToComponentProperty);
            }
        }
    }

    void PrefabUndoComponentPropertyEdit::Undo()
    {
        m_instanceToTemplateInterface->PatchTemplate(m_undoPatch, m_templateId);
    }

    void PrefabUndoComponentPropertyEdit::Redo()
    {
        Redo(AZStd::nullopt);
    }

    void PrefabUndoComponentPropertyEdit::Redo(InstanceOptionalConstReference instance)
    {
        m_instanceToTemplateInterface->PatchTemplate(m_redoPatch, m_templateId, instance);
    }
}
