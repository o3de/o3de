/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Uuid.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/Component/Component.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Entity/SliceGameEntityOwnershipService.h>
#include <AzFramework/Visibility/EntityVisibilityBoundsUnionSystem.h>
#include <AzFramework/AzFrameworkAPI.h>

#include "EntityContext.h"

namespace AzFramework
{
    /**
     * System component responsible for owning the game entity context.
     *
     * The game entity context owns entities in the game runtime, as well as during play-in-editor.
     * These entities typically own game/runtime components, *not* inheriting from EditorComponentBase.
     */
    class AZF_API GameEntityContextComponent
        : public AZ::Component
        , public EntityContext
        , private GameEntityContextRequestBus::Handler
        , public AZ::TransformNotificationBus::MultiHandler
    {
    public:

        AZ_COMPONENT(GameEntityContextComponent, "{DA235454-DD9C-468C-AE70-404E415BAA6C}");

        GameEntityContextComponent();
        ~GameEntityContextComponent() override;

        //////////////////////////////////////////////////////////////////////////
        // Component overrides
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // GameEntityContextRequestBus
        AZ::Uuid GetGameEntityContextId() override { return GetContextId(); }
        EntityContext* GetGameEntityContextInstance() override { return this; }
        void ResetGameContext() override;
        AZ::Entity* CreateGameEntity(const char* name) override;
        BehaviorEntity CreateGameEntityForBehaviorContext(const char* name) override;
        void AddGameEntity(AZ::Entity* entity) override;
        void DestroyGameEntity(const AZ::EntityId&) override;
        void DestroyGameEntityAndDescendants(const AZ::EntityId&) override;
        void ActivateGameEntity(const AZ::EntityId&) override;
        void ActivateGameEntityAndDescendants(AZ::EntityId rootEntityId, bool updateRoot = true) override; //Expanded Entity State Handling
        void DeactivateGameEntity(const AZ::EntityId&) override;
        void DeactivateGameEntityAndDescendants(AZ::EntityId rootEntityId, bool updateRoot = true) override; //Expanded Entity State Handling
        void SetGameEntityParent(const AZ::EntityId& /*targetEntityId*/, const AZ::EntityId& /*parentEntityId*/) override;
        bool LoadFromStream(AZ::IO::GenericStream& stream, bool remapIds) override;
        AZStd::string GetEntityName(const AZ::EntityId& id) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        void DestroyGameEntityInternal(const AZ::EntityId&, bool destroyChildren);
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // EntityContext
        AZ::Entity* CreateEntity(const char* name) override;
        void OnRootEntityReloaded() override;
        void OnContextEntitiesAdded(const EntityList& entities) override;
        void OnContextReset() override;
        bool ValidateEntitiesAreValidForContext(const EntityList& entities) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // TransformNotificationBus
        void OnParentChanged(AZ::EntityId oldParentId, AZ::EntityId newParentId) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // Expanded Entity State Handling to Introduce Hierarchichal Entity Activation Handling

        //! Utility method that parses the 'childrenByParentTree' in order to return a hierarchichal list of entities from top to bottom.
        //! \param entityId The root entity ID to gather the tree from.
        void AddEntityToParentChildTree(AZ::Entity* entity);
        //! Utility method that parses the 'childrenByParentTree' in order to return a hierarchichal list of entities from top to bottom.
        //! \param entityId The root entity ID to gather the tree from.
        void RemoveEntityFromParentChildTree(AZ::Entity* entity);
        //! Utility method that parses the 'childrenByParentTree' in order to return a hierarchichal list of entities from top to bottom.
        //! \param entityId The root entity ID to gather the tree from.
        void RemoveEntityFromParentChildTreeById(const AZ::EntityId& entityId);
        //! Utility method that parses the 'childrenByParentTree' in order to return a hierarchichal list of entities from top to bottom.
        //! \param entityId The root entity ID to gather the tree from.
        //! \param entityId The root entity ID to gather the tree from.
        void UpdateParentChildMaps(AZ::EntityId child, AZ::EntityId oldParent, AZ::EntityId newParent);
        //! Utility method that parses the 'childrenByParentTree' in order to return a hierarchichal list of entities from top to bottom.
        //! \param entityId The root entity ID to gather the tree from.
        void RecomputeEffectiveActivationForEntity(AZ::Entity* movedChildEntity);

        //! Utility method that parses the 'childrenByParentTree' in order to return a hierarchichal list of entities from top to bottom.
        //! \param entityId The root entity ID to gather the tree from.
        //! \param entityId The root entity ID to gather the tree from.
        void GetEntityTreeFromRootEntity(const AZ::EntityId& entityId, AZStd::vector<AZ::EntityId>& out);
        //! Utility method that parses the 'childrenByParentTree' in order to return a hierarchichal list of entities from top to bottom.
        //! \param entityId The root entity ID to gather the tree from.
        //! \param entityId The root entity ID to gather the tree from.
        //! \param entityId The root entity ID to gather the tree from.
        void PruneDescendantsFromTreeInPlace(const AZ::EntityId& root, AZStd::vector<AZ::EntityId>& list, size_t fromIndex) const;

        //! Local stored hierarchy tree to enable hierarchy handling without the TransformBus (which is only functional while active.)
        //! Tree is updated at
        AZStd::unordered_map<AZ::EntityId, AZStd::vector<AZ::EntityId>, AZStd::hash<AZ::EntityId>> childrenByParentTree; // Parent -> [Children..]
        AZStd::unordered_map<AZ::EntityId, AZ::EntityId, AZStd::hash<AZ::EntityId>> parentOf; // Child -> Parent

        size_t parentActiveTypeIndex = std::numeric_limits<size_t>::max();
        //////////////////////////////////////////////////////////////////////////

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("GameEntityContextService"));
        }
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("GameEntityContextService"));
        }
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("SliceSystemService"));
        }

    private:

        AzFramework::EntityVisibilityBoundsUnionSystem m_entityVisibilityBoundsUnionSystem;
    };
} // namespace AzFramework
