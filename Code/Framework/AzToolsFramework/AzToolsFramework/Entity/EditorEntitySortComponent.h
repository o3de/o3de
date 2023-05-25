/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "EditorEntitySortBus.h"
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Prefab/PrefabPublicNotificationBus.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AzToolsFramework
{
    namespace Components
    {
        class EditorEntitySortComponent
            : public AzToolsFramework::Components::EditorComponentBase
            , public EditorEntitySortRequestBus::Handler
            , public EditorEntityContextNotificationBus::Handler
            , public AzToolsFramework::Prefab::PrefabPublicNotificationBus::Handler
        {
            friend class JsonEditorEntitySortComponentSerializer;

        public:
            AZ_COMPONENT(EditorEntitySortComponent, "{6EA1E03D-68B2-466D-97F7-83998C8C27F0}", EditorComponentBase);

            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
            static bool SerializationConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);

            ~EditorEntitySortComponent();

            ////////////////////////////////////////
            // EditorEntitySortRequestBus::Handler
            ////////////////////////////////////////
            EntityOrderArray GetChildEntityOrderArray() override;
            bool SetChildEntityOrderArray(const EntityOrderArray& entityOrderArray) override;
            bool AddChildEntity(const AZ::EntityId& entityId, bool addToBack) override;
            bool AddChildEntityAtPosition(const AZ::EntityId& entityId, const AZ::EntityId& beforeEntity) override;
            bool RemoveChildEntity(const AZ::EntityId& entityId) override;
            AZ::u64 GetChildEntityIndex(const AZ::EntityId& entityId) override;
            bool CanMoveChildEntityUp(const AZ::EntityId& entityId) override;
            void MoveChildEntityUp(const AZ::EntityId& entityId) override;
            bool CanMoveChildEntityDown(const AZ::EntityId& entityId) override;
            void MoveChildEntityDown(const AZ::EntityId& entityId) override;

            //////////////////////////////////////////////////////////////////////////
            // EditorEntityContextNotificationBus::Handler
            void OnEntityStreamLoadSuccess() override;
            //////////////////////////////////////////////////////////////////////////

            void OnPrefabInstancePropagationBegin() override;
            void OnPrefabInstancePropagationEnd() override;
        private:
            void MarkDirtyAndSendChangedEvent();
            bool AddChildEntityInternal(const AZ::EntityId& entityId, bool addToBack, EntityOrderArray::iterator insertPosition);

            ///////////////
            // AZ::Entity
            ///////////////
            void Init() override;
            void Activate() override;

            // Serialization events
            void PrepareSave();
            void PostLoad();

            void SanitizeOrderEntryArray();

            class EntitySortSerializationEvents
                : public AZ::SerializeContext::IEventHandler
            {
                /**
                * Called right before we start reading from the instance pointed by classPtr.
                */
                void OnReadBegin(void* classPtr) override
                {
                    EditorEntitySortComponent* component = reinterpret_cast<EditorEntitySortComponent*>(classPtr);
                    component->PrepareSave();
                }

                /**
                * Called right after we finish writing data to the instance pointed at by classPtr.
                */
                void OnWriteEnd(void* classPtr) override
                {
                    EditorEntitySortComponent* component = reinterpret_cast<EditorEntitySortComponent*>(classPtr);
                    component->PostLoad();
                }
            };

            /**
             * EntityOrderEntry stores an entity id and the sort index 
             * The sort index is the absolute index compared to the other entries, 0 is the first, 1 is the second, so on
             * We serialize out the order data in this fashion because the slice data patching system will traditionally use the vector index to know what data goes where
             * In the case of this data, it does not make sense to data patch by vector index since the underlying data may have changed and the data patch will create duplicate or incorrect data.
             * The slice data patch system has the concept of a "Persistent ID" which can be used instead such that data patches will try to match persistent ids which can be identified regardless
             * of vector index. In this way, our vector order no longer matters and the Entity Id is now the identifier which the data patcher will use to update the sort index.
             */
            struct EntityOrderEntry
            {
                AZ_TYPE_INFO(EntityOrderEntry, "{08980128-8D93-48AC-BF4A-1E75F39C1A29}");
                AZ::EntityId m_entityId;
                AZ::u64 m_sortIndex;
            };
            using EntityOrderEntryArray = AZStd::vector<EntityOrderEntry>;
            EntityOrderEntryArray m_childEntityOrderEntryArray; ///< The serialized order array which uses the persistent id mechanism as described above*/

            EntityOrderArray m_childEntityOrderArray; ///< The simple vector of entity id is what is used by the entity sort order ebus and is generated from the serialized data

            void RebuildEntityOrderCache();
            EntityOrderArray::iterator GetFirstSelectedEntityPosition();
            using EntityOrderCache = AZStd::unordered_map<AZ::EntityId, AZ::u64>;
            EntityOrderCache m_childEntityOrderCache; ///< The map of entity id to index for quick look up

            bool m_entityOrderIsDirty = true; ///< This flag indicates our stored serialization order data is out of date and must be rebuilt before serialization occurs
            bool m_ignoreIncomingOrderChanges = false; ///< This is set when prefab propagation occurs so that non-authored order changes can be ignored
            bool m_shouldSanityCheckStateAfterPropagation = false; //< This is set after activation, to queue a cleanup of any invalid state after the next prefab propagation.
        };
    }
} // namespace AzToolsFramework
