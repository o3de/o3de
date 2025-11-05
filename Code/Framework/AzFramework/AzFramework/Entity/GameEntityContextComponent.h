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
        // Expanded Entity State Handling to Introduce Hierarchichal Entity Activation Handling

        //! Utility method that processes adding the input entity by pointer to the 'm_childrenByParentTree' and 'm_parentOf' trees.
        //! \param entity The target entity to process into the tree.
        void AddEntityToParentChildTree(AZ::Entity* entity);
        //! Utility method that processes removing the input entity by entityId from the 'm_childrenByParentTree' and 'm_parentOf' trees.
        //! \param entityId The target entityId to process out of the tree
        void RemoveEntityFromParentChildTree(const AZ::EntityId& entityId);
        //! Utility method that updates the 'm_childrenByParentTree' and 'm_parentOf' trees with the new arrangement of children and parents.
        //! \param childId The child entity ID that's being reparented.
        //! \param oldParentId The old parent's entity ID the child is currently registered to in the tree.
        //! \param newParentId The new parent's entity ID to move the registration of the child to in the tree.
        void UpdateParentChildHierarchy(const AZ::EntityId& childId, const AZ::EntityId& oldParentId, const AZ::EntityId& newParentId) override;
        //! Utility method that handles evaluating the state the entity should be in relative to it's parent. (Used after a parent change update.)
        //! \param movedChildEntity The entity that needs to be evaluated for parent relative active state.
        void RecomputeEffectiveActivationForEntity(AZ::Entity* movedChildEntity);

        //! Utility method that parses the 'm_childrenByParentTree' in order to return a hierarchichal list of entities from top to bottom.
        //! \param entityId The root entity ID to gather the tree from.
        //! \param out A reference to a vector of EntityIds used in the origin that this methid is being called from.
        void GetEntityTreeFromRootEntity(const AZ::EntityId& entityId, AZStd::vector<AZ::EntityId>& out);
        //! Utility method that parses a tree in order to remove a branch of entities starting at the root entity. This is informed by an index which identifies where in the vector we're starting from.
        //! Handles the entity list in a way that preserves a forward moving loop, allowing the system to ignore processing branches when they are unchanged.
        //! \param root The root entity ID to start the pruning from.
        //! \param list The list generated from GetEntityTreeFromRootEntity "out", to prune the entities from.
        //! \param fromIndex The i index from the loop to assure the list will not go out of bounds while it processes.
        void PruneDescendantsFromTreeInPlace(const AZ::EntityId& root, AZStd::vector<AZ::EntityId>& list, size_t fromIndex) const;

        //! Local stored hierarchy tree to enable hierarchy handling without the TransformBus (which is only functional while active.)
        //! The tree is populated at AddGameEntity, and DestroyGameEntity.
        //! The tree is updated by UpdateParentChildHierarchy.
        AZStd::unordered_map<AZ::EntityId, AZStd::vector<AZ::EntityId>, AZStd::hash<AZ::EntityId>> m_childrenByParentTree; // Parent -> [Children..]
        AZStd::unordered_map<AZ::EntityId, AZ::EntityId, AZStd::hash<AZ::EntityId>> m_parentOf; // Child -> Parent

        //! Local reference to what active type index position the "Parent" type is for Entity Activation Handling.
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
