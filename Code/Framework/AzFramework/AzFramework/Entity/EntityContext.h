/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZFRAMEWORK_ENTITYCONTEXT_H
#define AZFRAMEWORK_ENTITYCONTEXT_H

#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/IdUtils.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/Entity/SliceEntityOwnershipService.h>

namespace AZ
{
    class ReflectContext;
}

namespace AzFramework
{
    class Scene;

    /**
     * Provides services for a group of entities under the umbrella of a given context.
     *
     * e.g. Edit-time entities and runtime entities would belong to separate contexts.
     *
     * A context owns a root entity, which can be serialized in or out. Interfaces are
     * provided for creating entities owned by the context.
     *
     * Entity contexts are not required to use entities, but provide a package for managing
     * independent prefab hierarchies (i.e. a level, a world, etc).
     */
    class EntityContext
        : public EntityIdContextQueryBus::MultiHandler
        , public AZ::EntityBus::MultiHandler 
        , public EntityContextRequestBus::Handler
        , public EntityOwnershipServiceNotificationBus::Handler
    {
    public:
        AZ_TYPE_INFO(EntityContext, "{4F98A6B9-C7B5-450E-8A8A-30EEFC411EF5}");

        /// The type used to store entity in AzFramework::Scene.
        using SceneStorageType = EntityContext*;

        EntityContext(AZ::SerializeContext* serializeContext = nullptr);
        EntityContext(const EntityContextId& contextId, AZ::SerializeContext* serializeContext = nullptr);
        EntityContext(const EntityContextId& contextId, AZStd::unique_ptr<EntityOwnershipService> entityOwnershipService,
            AZ::SerializeContext* serializeContext = nullptr);
        virtual ~EntityContext();

        void InitContext();
        void DestroyContext();

        /// \return the context's Id, which is used to listen on a given context's request or event bus.
        const EntityContextId& GetContextId() const { return m_contextId; }

        //////////////////////////////////////////////////////////////////////////
        // EntityContextRequestBus
        AZ::Entity* CreateEntity(const char* name) override;
        void AddEntity(AZ::Entity* entity) override;
        void ActivateEntity(AZ::EntityId entityId) override;
        void DeactivateEntity(AZ::EntityId entityId) override;
        bool DestroyEntity(AZ::Entity* entity) override;
        bool DestroyEntityById(AZ::EntityId entityId) override;
        AZ::Entity* CloneEntity(const AZ::Entity& sourceEntity) override;
        void ResetContext() override;
        //////////////////////////////////////////////////////////////////////////

        static void Reflect(AZ::ReflectContext* context);
        static AZStd::shared_ptr<Scene> FindContainingScene(const EntityContextId& contextId);

    protected:

        //////////////////////////////////////////////////////////////////////////
        // EntityIdContextQueryBus
        EntityContextId GetOwningContextId() override { return m_contextId; }
        //////////////////////////////////////////////////////////////////////////
        
        //////////////////////////////////////////////////////////////////////////
        // EntityOwnershipServiceNotificationBus
        void PrepareForEntityOwnershipServiceReset() override;
        void OnEntityOwnershipServiceReset() override;
        void OnEntitiesReloadedFromStream(const EntityList& entities) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // EntityBus
        void OnEntityDestruction(const AZ::EntityId& entityId) override;
        //////////////////////////////////////////////////////////////////////////

        void HandleEntitiesAdded(const EntityList& entities);
        void HandleEntitiesRemoved(const EntityIdList& entityIds);

        AZ::SerializeContext* GetSerializeContext() const;

        /// Entity context derived implementations can conduct specialized actions when internal events occur, such as adds/removals/resets.
        virtual void OnContextEntitiesAdded(const EntityList& /*entities*/) {}
        virtual void OnContextEntityRemoved(const AZ::EntityId& /*id*/) {}
        virtual void OnRootEntityReloaded() {}
        virtual void PrepareForContextReset() { m_contextIsResetting = true; }
        virtual void OnContextReset() { m_contextIsResetting = false; }

        /// Used to validate that the given list of entities are valid for this context
        /// For example they could be non-UI entities being instantiated in a UI context
        virtual bool ValidateEntitiesAreValidForContext(const EntityList& entities);

        /// Determine if the entity with the given ID is owned by this Entity Context
        /// \param entityId An entity ID to check
        /// \return true if this context owns the entity with the given id.
        bool IsOwnedByThisContext(const AZ::EntityId& entityId);

        AZ::SerializeContext*                       m_serializeContext;

        //! Id of the context, used to address bus messages
        EntityContextId                             m_contextId;

        //! Pre-bound event bus for the context.
        EntityContextEventBus::BusPtr               m_eventBusPtr;

        //! EntityOwnershipService is responsible for the management of entities used by this context. Such as loading, creation, etc.
        AZStd::unique_ptr<EntityOwnershipService>   m_entityOwnershipService;

        // Tracks if the context is currently being reset.
        // This allows systems to skip steps during teardown that will be handled in bulk by the reset.
        bool m_contextIsResetting = false;
    };
} // namespace AzFramework

#endif // AZFRAMEWORK_ENTITYCONTEXT_H
