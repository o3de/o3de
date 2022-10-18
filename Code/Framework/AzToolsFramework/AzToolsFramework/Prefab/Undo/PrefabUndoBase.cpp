/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Interface/Interface.h>
#include <Prefab/Undo/PrefabUndo.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        PrefabUndoBase::PrefabUndoBase(const AZStd::string& undoOperationName)
            : UndoSystem::URSequencePoint(undoOperationName)
            , m_changed(true)
            , m_templateId(InvalidTemplateId)
        {
            m_instanceToTemplateInterface = AZ::Interface<InstanceToTemplateInterface>::Get();
            AZ_Assert(m_instanceToTemplateInterface, "Failed to grab instance to template interface");
        }

        bool PrefabUndoBase::Changed() const
        {
            return m_changed;
        }

        void PrefabUndoBase::Undo()
        {
            m_instanceToTemplateInterface->PatchTemplate(m_undoPatch, m_templateId);
        }

        void PrefabUndoBase::Redo()
        {
            m_instanceToTemplateInterface->PatchTemplate(m_redoPatch, m_templateId);
        }
    }
}
