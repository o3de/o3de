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

#include <AzFramework/Entity/PrefabEntityOwnershipService.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class Instance;
        class PrefabSystemComponentInterface;
    }

    class PrefabEditorEntityOwnershipService
        : public AzFramework::PrefabEntityOwnershipService
    {
    public:
        using EntityList = AzFramework::EntityList;
        using OnEntitiesAddedCallback = AzFramework::OnEntitiesAddedCallback;
        using OnEntitiesRemovedCallback = AzFramework::OnEntitiesRemovedCallback;
        using ValidateEntitiesCallback = AzFramework::ValidateEntitiesCallback;

        //! Initializes all assets/entities/components required for managing entities.
        void Initialize() override;

        //! Returns true if the entity ownership service is initialized.
        bool IsInitialized() override;

        //! Destroys all the assets/entities/components created for managing entities.
        void Destroy() override;

        //! Resets the assets/entities/components without fully destroying them for managing entities.
        void Reset() override;

        void AddEntity(AZ::Entity* entity) override;

        void AddEntities(const EntityList& entities) override;

        bool DestroyEntity(AZ::Entity* entity) override;

        bool DestroyEntityById(const AZ::EntityId entityId) override;

        /**
        * Gets the entities in entity ownership service that do not belong to a prefab.
        *
        * \param entityList The entity list to add the entities to.
        */
        void GetNonPrefabEntities(EntityList& entities) override;

        /**
        * Gets all entities, including those that are owned by prefabs in the entity ownership service.
        *
        * \param entityList The entity list to add the entities to.
        * \return bool whether fetching entities was successful.
        */
        bool GetAllEntities(EntityList& entities) override;

        /**
        * Instantiates all the prefabs that are in the entity ownership service.
        */
        void InstantiateAllPrefabs() override;

        void HandleEntitiesAdded(const EntityList& entities) override;

        // To be removed in the future
        bool LoadFromStream(AZ::IO::GenericStream& stream, bool remapIds,
            EntityIdToEntityIdMap* idRemapTable = nullptr,
            const AZ::ObjectStream::FilterDescriptor& filterDesc = AZ::ObjectStream::FilterDescriptor());

        void SetEntitiesAddedCallback(OnEntitiesAddedCallback onEntitiesAddedCallback) override;
        void SetEntitiesRemovedCallback(OnEntitiesRemovedCallback onEntitiesRemovedCallback) override;
        void SetValidateEntitiesCallback(ValidateEntitiesCallback validateEntitiesCallback) override;

    protected:
        OnEntitiesAddedCallback m_entitiesAddedCallback;
        OnEntitiesRemovedCallback m_entitiesRemovedCallback;
        ValidateEntitiesCallback m_validateEntitiesCallback;

        AZStd::unique_ptr<Prefab::Instance> m_rootInstance;
        Prefab::PrefabSystemComponentInterface* m_prefabSystemComponent;
    };
}
