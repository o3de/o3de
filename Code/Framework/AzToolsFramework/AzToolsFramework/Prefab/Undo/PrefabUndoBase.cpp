/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Interface/Interface.h>
#include <AzToolsFramework/Prefab/Instance/InstanceDomGeneratorInterface.h>
#include <AzToolsFramework/Prefab/Instance/InstanceToTemplateInterface.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/Prefab/Undo/PrefabUndoBase.h>

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

            m_instanceDomGeneratorInterface = AZ::Interface<InstanceDomGeneratorInterface>::Get();
            AZ_Assert(m_instanceDomGeneratorInterface, "Failed to grab instance DOM generator interface");

            m_prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();
            AZ_Assert(m_prefabSystemComponentInterface, "Failed to grab prefab system component interface");
        }

        bool PrefabUndoBase::Changed() const
        {
            return m_changed;
        }

        void PrefabUndoBase::Undo()
        {
            [[maybe_unused]] bool isPatchApplicationSuccessful =
                m_instanceToTemplateInterface->PatchTemplate(m_undoPatch, m_templateId);

            AZ_Error(
                "Prefab", isPatchApplicationSuccessful,
                "Applying the undo patch to template with id '%llu' was unsuccessful", m_templateId);
        }

        void PrefabUndoBase::Redo()
        {
            Redo(AZStd::nullopt);
        }

        void PrefabUndoBase::Redo(InstanceOptionalConstReference instanceToExclude)
        {
            [[maybe_unused]] bool isPatchApplicationSuccessful =
                m_instanceToTemplateInterface->PatchTemplate(m_redoPatch, m_templateId, instanceToExclude);

            AZ_Error(
                "Prefab", isPatchApplicationSuccessful,
                "Applying the redo patch to template with id '%llu' was unsuccessful", m_templateId);
        }
    }
}
