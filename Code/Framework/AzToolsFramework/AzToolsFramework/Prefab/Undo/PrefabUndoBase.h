/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzToolsFramework/Undo/UndoSystem.h>
#include <AzToolsFramework/Prefab/PrefabIdTypes.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class InstanceToTemplateInterface;
        class InstanceDomGeneratorInterface;
        class PrefabSystemComponentInterface;

        class PrefabUndoBase
            : public UndoSystem::URSequencePoint
        {
        public:
            AZ_RTTI(PrefabUndoBase, "{C86BFA2B-E042-40FC-B5C6-1838030B34BF}", UndoSystem::URSequencePoint);
            AZ_CLASS_ALLOCATOR(PrefabUndoBase, AZ::SystemAllocator, 0);

            explicit PrefabUndoBase(const AZStd::string& undoOperationName);

            bool Changed() const override;

            void Undo() override;
            void Redo() override;

            //! Overload to allow to apply the change, but prevent instanceToExclude from being refreshed.
            void virtual Redo(InstanceOptionalConstReference instanceToExclude);

        protected:
            TemplateId m_templateId;

            PrefabDom m_redoPatch;
            PrefabDom m_undoPatch;

            InstanceToTemplateInterface* m_instanceToTemplateInterface = nullptr;
            InstanceDomGeneratorInterface* m_instanceDomGeneratorInterface = nullptr;
            PrefabSystemComponentInterface* m_prefabSystemComponentInterface = nullptr;

            bool m_changed;
        };
    }
}
