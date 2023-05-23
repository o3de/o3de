/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Prefab/Instance/InstanceToTemplateInterface.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/Prefab/Undo/PrefabUndoComponentPropertyEdit.h>
#include <AzToolsFramework/Prefab/Undo/PrefabUndoUtils.h>

namespace AzToolsFramework::Prefab
{
    PrefabUndoComponentPropertyEdit::PrefabUndoComponentPropertyEdit(const AZStd::string& undoOperationName)
        : PrefabUndoBase(undoOperationName)
        , m_changed(false)
    {
    }

    void PrefabUndoComponentPropertyEdit::Capture(
        Instance& owningInstance,
        const AZStd::string& pathToPropertyFromOwningPrefab,
        const PrefabDomValue& afterStateOfComponentProperty,
        bool updateCache)
    {
        m_templateId = owningInstance.GetTemplateId();

        // Get the current state of property value from the owning prefab template (which is also the focused template).
        const PrefabDom& owningTemplateDom = m_prefabSystemComponentInterface->FindTemplateDom(m_templateId);
        PrefabDomPath domPathToPropertyFromOwningPrefab(pathToPropertyFromOwningPrefab.c_str());
        const PrefabDomValue* currentPropertyDomValue = domPathToPropertyFromOwningPrefab.Get(owningTemplateDom);

        if (currentPropertyDomValue)
        {
            PrefabDom changePatches;
            m_instanceToTemplateInterface->GeneratePatch(changePatches, *currentPropertyDomValue, afterStateOfComponentProperty);
            m_changed = !(changePatches.GetArray().Empty());

            if (m_changed)
            {
                // Adds 'replace' patches to replace the property value.
                PrefabUndoUtils::AppendUpdateValuePatch(
                    m_redoPatch, afterStateOfComponentProperty, pathToPropertyFromOwningPrefab, PatchType::Edit);
                PrefabUndoUtils::AppendUpdateValuePatch(
                    m_undoPatch, *currentPropertyDomValue, pathToPropertyFromOwningPrefab, PatchType::Edit);
            }
        }
        else
        {
            // If the DOM value from owning template is not present, it means the value is new to the template.
            m_changed = true;

            // Adds 'add' and 'remove' patches for the property value.
            PrefabUndoUtils::AppendUpdateValuePatch(
                m_redoPatch, afterStateOfComponentProperty, pathToPropertyFromOwningPrefab, PatchType::Add);
            PrefabUndoUtils::AppendRemovePatch(m_undoPatch, pathToPropertyFromOwningPrefab);
        }

        // Preemptively updates the cached DOM to prevent reloading instance DOM.
        if (m_changed && updateCache)
        {
            PrefabDomReference cachedOwningInstanceDom = owningInstance.GetCachedInstanceDom();
            if (cachedOwningInstanceDom.has_value())
            {
                PrefabUndoUtils::UpdateValueInPrefabDom(
                    cachedOwningInstanceDom, afterStateOfComponentProperty, pathToPropertyFromOwningPrefab);
            }
        }
    }

    bool PrefabUndoComponentPropertyEdit::Changed() const
    {
        return m_changed;
    }

    void PrefabUndoComponentPropertyEdit::Undo()
    {
        if (m_changed)
        {
            m_instanceToTemplateInterface->PatchTemplate(m_undoPatch, m_templateId);
        }
    }

    void PrefabUndoComponentPropertyEdit::Redo()
    {
        Redo(AZStd::nullopt);
    }

    void PrefabUndoComponentPropertyEdit::Redo(InstanceOptionalConstReference instance)
    {
        if (m_changed)
        {
            m_instanceToTemplateInterface->PatchTemplate(m_redoPatch, m_templateId, instance);
        }
    }
}
