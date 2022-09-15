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
#include <AzToolsFramework/Prefab/Instance/InstanceToTemplateInterface.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityMapperInterface.h>

//for link undo
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/Prefab/Link/Link.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class PrefabUndoBase
            : public UndoSystem::URSequencePoint
        {
        public:
            explicit PrefabUndoBase(const AZStd::string& undoOperationName);

            bool Changed() const override { return m_changed; }

        protected:
            TemplateId m_templateId;

            PrefabDom m_redoPatch;
            PrefabDom m_undoPatch;

            InstanceToTemplateInterface* m_instanceToTemplateInterface = nullptr;

            bool m_changed;
        };

        //! handles the addition and removal of entities from instances
        class PrefabUndoInstance
            : public PrefabUndoBase
        {
        public:
            explicit PrefabUndoInstance(const AZStd::string& undoOperationName);

            void Capture(
                const PrefabDom& initialState,
                const PrefabDom& endState,
                TemplateId templateId);

            void Undo() override;
            void Redo() override;
            void Redo(InstanceOptionalConstReference instance);
        };

        //! Undo class for handling addition of an entity to a prefab template.
        class PrefabUndoAddEntity
            : public PrefabUndoBase
        {
        public:
            explicit PrefabUndoAddEntity(const AZStd::string& undoOperationName);

            void Capture(const PrefabDomValue& entityDom, AZ::EntityId entityId, TemplateId templateId);

            void Undo() override;
            void Redo() override;
        };

        //! Undo class for handling deletion of entities to a prefab template.
        class PrefabUndoRemoveEntities
            : public PrefabUndoBase
        {
        public:
            explicit PrefabUndoRemoveEntities(const AZStd::string& undoOperationName);

            void Capture(const AZStd::vector<AZStd::pair<const PrefabDomValue*, AZStd::string>>& entityDomAndPathList,
                TemplateId templateId);

            void Undo() override;
            void Redo() override;
        };

        //! Undo class for handling update of an entity to a prefab template.
        class PrefabUndoEntityUpdate
            : public PrefabUndoBase
        {
        public:
            AZ_RTTI(PrefabUndoEntityUpdate, "{6D60C5A6-9535-45B3-8897-E5F6382FDC93}", PrefabUndoBase);
            AZ_CLASS_ALLOCATOR(PrefabUndoEntityUpdate, AZ::SystemAllocator, 0);

            explicit PrefabUndoEntityUpdate(const AZStd::string& undoOperationName);

            void Capture(const PrefabDomValue& initialState, const PrefabDomValue& endState, AZ::EntityId entity);

            void Undo() override;
            void Redo() override;
            //! Overload to allow to apply the change, but prevent instanceToExclude from being refreshed.
            void Redo(InstanceOptionalConstReference instanceToExclude);

        private:
            InstanceEntityMapperInterface* m_instanceEntityMapperInterface = nullptr;
            EntityAlias m_entityAlias;
        };

        //! handles link changes on instances
        class PrefabUndoInstanceLink
            : public PrefabUndoBase
        {
        public:
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

            PrefabSystemComponentInterface* m_prefabSystemComponentInterface = nullptr;
        };

        class PrefabUndoLinkUpdate
            : public PrefabUndoBase
        {
        public:
            explicit PrefabUndoLinkUpdate(const AZStd::string& undoOperationName);

            //capture for add/remove
            void Capture(
                const PrefabDom& patch,
                const LinkId linkId = InvalidLinkId);

            void Undo() override;
            void Redo() override;
            //! Overload to allow to apply the change, but prevent instanceToExclude from being refreshed.
            void Redo(InstanceOptionalConstReference instanceToExclude);

        private:
            void UpdateLink(PrefabDom& linkDom, InstanceOptionalConstReference instanceToExclude = AZStd::nullopt);

            LinkId m_linkId;
            PrefabDom m_linkDomNext;  //data for delete/update
            PrefabDom m_linkDomPrevious; //stores the data for undo

            PrefabSystemComponentInterface* m_prefabSystemComponentInterface = nullptr;
        };

    }
}
