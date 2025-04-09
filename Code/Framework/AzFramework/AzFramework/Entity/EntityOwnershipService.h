/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Debug/Budget.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzFramework/Entity/EntityOwnershipServiceBus.h>

namespace AZ
{
    class Entity;
}

AZ_DECLARE_BUDGET(AzFramework);

namespace AzFramework
{
    // Types
    using EntityList = AZStd::vector<AZ::Entity*>;
    using EntityIdList = AZStd::vector<AZ::EntityId>;
    using EntityContextId = AZ::Uuid;

    // Callbacks
    using OnEntitiesAddedCallback = AZStd::function<void(const EntityList&)>;
    using OnEntitiesRemovedCallback = AZStd::function<void(const EntityIdList&)>;
    using ValidateEntitiesCallback = AZStd::function<bool(const EntityList&)>;

    class EntityOwnershipService
        : public AZ::Data::AssetBus::MultiHandler
    {   
    public:
        using EntityIdToEntityIdMap = AZStd::unordered_map<AZ::EntityId, AZ::EntityId>;

        virtual ~EntityOwnershipService() = default;

        //! Initializes all assets/entities/components required for managing entities.
        virtual void Initialize() = 0;

        //! Returns true if the entity ownership service is initialized.
        virtual bool IsInitialized() = 0;

        //! Destroys all the assets/entities/components created for managing entities.
        virtual void Destroy() = 0;

        //! Resets the assets/entities/components without fully destroying them for managing entities.
        virtual void Reset() = 0;

        virtual void AddEntity(AZ::Entity* entity) = 0;

        virtual void AddEntities(const EntityList& entities) = 0;

        virtual bool DestroyEntity(AZ::Entity* entity) = 0;

        virtual bool DestroyEntityById(AZ::EntityId entityId) = 0;

        /**
         * Gets the entities in entity ownership service that do not belong to a prefab.
         * 
         * \param entityList The entity list to add the entities to.
         */
        virtual void GetNonPrefabEntities(EntityList& entityList) = 0;

        /**
         * Gets all entities, including those that are owned by prefabs in the entity ownership service.
         * 
         * \param entityList The entity list to add the entities to.
         * \return bool whether fetching entities was successful.
         */
        virtual bool GetAllEntities(EntityList& entityList) = 0;

        /**
         * Instantiates all the prefabs that are in the entity ownership service.
         * 
         */
        virtual void InstantiateAllPrefabs() = 0;

        virtual void HandleEntitiesAdded(const EntityList& entities) = 0;

        virtual bool LoadFromStream(AZ::IO::GenericStream& stream, bool remapIds,
            EntityIdToEntityIdMap* idRemapTable = nullptr,
            const AZ::ObjectStream::FilterDescriptor& filterDesc = AZ::ObjectStream::FilterDescriptor()) = 0;
        
        virtual void SetEntitiesAddedCallback(OnEntitiesAddedCallback onEntitiesAddedCallback) = 0;
        virtual void SetEntitiesRemovedCallback(OnEntitiesRemovedCallback onEntitiesRemovedCallback) = 0;
        virtual void SetValidateEntitiesCallback(ValidateEntitiesCallback validateEntitiesCallback) = 0;

        /**
         * Called when an entity is in the process of being destroyed
         *
         * \param entityId The entity Id of the entity being destroyed.
         */
        virtual void HandleEntityBeingDestroyed(const AZ::EntityId& entityId) = 0;

        bool m_shouldAssertForLegacySlicesUsage = false;

    protected:
        OnEntitiesAddedCallback m_entitiesAddedCallback;
        OnEntitiesRemovedCallback m_entitiesRemovedCallback;
        ValidateEntitiesCallback m_validateEntitiesCallback;
    };
}
