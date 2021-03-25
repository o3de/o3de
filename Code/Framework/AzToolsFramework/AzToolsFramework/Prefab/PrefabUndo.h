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
                PrefabDom& initialState,
                PrefabDom& endState,
                const TemplateId& templateId);

            void Undo() override;
            void Redo() override;
        };

        //! handles entity updates, such as when the values on an entity change
        class PrefabUndoEntityUpdate
            : public PrefabUndoBase
        {
        public:
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
                UPDATE,
                LINKSTATUS
            };

            explicit PrefabUndoInstanceLink(const AZStd::string& undoOperationName);

            //capture for add/remove
            void Capture(
                const TemplateId& targetId,
                const TemplateId& sourceId,
                const InstanceAlias& instanceAlias,
                const LinkId& linkId = InvalidLinkId,
                const Link& link = Link());

            void Undo() override;
            void Redo() override;

        private:
            //used for special cases of add/delete
            void AddLink();

            void RemoveLink();

            TemplateId m_targetId;
            TemplateId m_sourceId;
            InstanceAlias m_instanceAlias;

            LinkId m_linkId;
            Link m_link;  //data for delete/update
            LinkStatus m_linkStatus;

            PrefabSystemComponentInterface* m_prefabSystemComponentInterface = nullptr;
        };
    }
}
