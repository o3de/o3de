/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
                const TemplateId& templateId);

            void Undo() override;
            void Redo() override;
        };

        //! handles entity updates, such as when the values on an entity change
        class PrefabUndoEntityUpdate
            : public PrefabUndoBase
        {
        public:
            AZ_RTTI(PrefabUndoEntityUpdate, "{6D60C5A6-9535-45B3-8897-E5F6382FDC93}", PrefabUndoBase);
            AZ_CLASS_ALLOCATOR(PrefabUndoEntityUpdate, AZ::SystemAllocator, 0);

            explicit PrefabUndoEntityUpdate(const AZStd::string& undoOperationName);

            void Capture(
                PrefabDom& initialState,
                PrefabDom& endState,
                const AZ::EntityId& entity);

            void Undo() override;
            void Redo() override;

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
                const TemplateId& targetId,
                const TemplateId& sourceId,
                const InstanceAlias& instanceAlias,
                const PrefabDomReference linkDom = PrefabDomReference(),
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
            PrefabDom m_linkDom;  //data for delete/update
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

        private:
            void UpdateLink(PrefabDom& linkDom);

            LinkId m_linkId;
            PrefabDom m_linkDomNext;  //data for delete/update
            PrefabDom m_linkDomPrevious; //stores the data for undo

            PrefabSystemComponentInterface* m_prefabSystemComponentInterface = nullptr;
        };

    }
}
