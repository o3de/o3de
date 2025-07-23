/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/AzToolsFrameworkAPI.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityMapperInterface.h>
#include <AzToolsFramework/Prefab/Undo/PrefabUndoBase.h>

//for link undo
#include <AzToolsFramework/Prefab/Link/Link.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        //! handles the addition and removal of entities from instances
        class AZTF_API PrefabUndoInstance
            : public PrefabUndoBase
        {
        public:
            explicit PrefabUndoInstance(const AZStd::string& undoOperationName);

            void Capture(
                const PrefabDom& initialState,
                const PrefabDom& endState,
                TemplateId templateId);
        };

        //! Undo class for handling addition of entity DOMs to a prefab template.
        class AZTF_API PrefabUndoAddEntityDoms : public PrefabUndoBase
        {
        public:
            explicit PrefabUndoAddEntityDoms(const AZStd::string& undoOperationName);

            void Capture(const AZStd::vector<const AZ::Entity*>& entityList, TemplateId templateId);
        };

        //! Undo class for handling removal of entity DOMs to a prefab template.
        class AZTF_API PrefabUndoRemoveEntityDoms
            : public PrefabUndoBase
        {
        public:
            explicit PrefabUndoRemoveEntityDoms(const AZStd::string& undoOperationName);

            void Capture(const AZStd::vector<AZStd::pair<const PrefabDomValue*, AZStd::string>>& entityDomAndPathList,
                TemplateId templateId);
        };

        //! Undo class for handling update of an entity to a prefab template.
        class AZTF_API PrefabUndoEntityUpdate
            : public PrefabUndoBase
        {
        public:
            AZ_RTTI(PrefabUndoEntityUpdate, "{6D60C5A6-9535-45B3-8897-E5F6382FDC93}", PrefabUndoBase);
            AZ_CLASS_ALLOCATOR(PrefabUndoEntityUpdate, AZ::SystemAllocator);

            explicit PrefabUndoEntityUpdate(const AZStd::string& undoOperationName);

            void Capture(const PrefabDomValue& initialState, const PrefabDomValue& endState, AZ::EntityId entity, bool updateCache = true);

            void Undo() override;
            void Redo() override;
            void Redo(InstanceOptionalConstReference instanceToExclude) override;

        private:
            InstanceEntityMapperInterface* m_instanceEntityMapperInterface = nullptr;
            EntityAlias m_entityAlias;
        };

        //! handles link changes on instances
        class AZTF_API PrefabUndoInstanceLink
            : public PrefabUndoBase
        {
        public:
            AZ_CLASS_ALLOCATOR(PrefabUndoInstanceLink, AZ::SystemAllocator)
            enum class LinkStatus
            {
                ADD,
                REMOVE,
                LINKSTATUS
            };

            explicit PrefabUndoInstanceLink(const AZStd::string& undoOperationName);

            //capture for add/remove
            void Capture(
                TemplateId targetId,
                TemplateId sourceId,
                const InstanceAlias& instanceAlias,
                PrefabDom linkPatches = PrefabDom(),
                const LinkId linkId = InvalidLinkId);

            void Undo() override;
            void Redo() override;
            void Redo(InstanceOptionalConstReference instanceToExclude) override;

            LinkId GetLinkId();

        private:
            //used for special cases of add/delete
            void AddLink();

            void RemoveLink();

            TemplateId m_targetId;
            TemplateId m_sourceId;
            InstanceAlias m_instanceAlias;

            LinkId m_linkId;
            PrefabDom m_linkPatches;  //data for delete/update
            LinkStatus m_linkStatus;
        };
    }
}
