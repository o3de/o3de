/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Prefab/PrefabDomUtils.h>
#include <Prefab/PrefabUndoBase.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class PrefabSystemComponentInterface;
            
        //! Undo class for handling addition of an entity to a prefab template.
        class PrefabUndoAddEntity
            : public PrefabUndoBase
        {
        public:
            AZ_RTTI(PrefabUndoAddEntity, "{67EC7123-7F42-4BDD-9543-43349E2EA605}", PrefabUndoBase);
            AZ_CLASS_ALLOCATOR(PrefabUndoAddEntity, AZ::SystemAllocator, 0);

            explicit PrefabUndoAddEntity(const AZStd::string& undoOperationName);

            struct ParentEntityInfo
            {
                explicit ParentEntityInfo(
                    AZ::EntityId parentEntityId,
                    const PrefabDomValue& parentEntityDomBeforeAddingEntity,
                    const PrefabDomValue& parentEntityDomAfterAddingEntity);
                    
                AZ::EntityId m_parentEntityId;
                const PrefabDomValue& m_parentEntityDomBeforeAddingEntity;
                const PrefabDomValue& m_parentEntityDomAfterAddingEntity;
            };

            struct NewEntityInfo
            {
                explicit NewEntityInfo(
                    AZ::EntityId newEntityId,
                    const PrefabDomValue& newEntityDom);

                AZ::EntityId m_newEntityId;
                const PrefabDomValue& m_newEntityDom;
            };

            void Capture(
                ParentEntityInfo parentEntityInfo,
                NewEntityInfo newEntityInfo,
                PrefabDomReference cachedInstanceDom,
                TemplateId templateId,
                AZStd::string entityAliasPathPrefix = "");

            void Undo() override;
            void Redo() override;

        private:
            PrefabSystemComponentInterface* m_prefabSystemComponentInterface = nullptr;
        };
    }
}
