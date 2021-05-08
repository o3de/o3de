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
#ifndef AZFRAMEWORK_GAMEENTITYCONTEXTCOMPONENT_H
#define AZFRAMEWORK_GAMEENTITYCONTEXTCOMPONENT_H

#include <AzCore/Math/Uuid.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/Component/Component.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Entity/SliceGameEntityOwnershipService.h>

#include "EntityContext.h"

namespace AzFramework
{
    /**
     * System component responsible for owning the game entity context.
     *
     * The game entity context owns entities in the game runtime, as well as during play-in-editor.
     * These entities typically own game/runtime components, *not* inheriting from EditorComponentBase.
     */
    class GameEntityContextComponent
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
        void DeactivateGameEntity(const AZ::EntityId&) override;
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
        void OnContextEntitiesAdded(const EntityList& entities);
        void OnContextReset() override;
        bool ValidateEntitiesAreValidForContext(const EntityList& entities) override;
        //////////////////////////////////////////////////////////////////////////

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("GameEntityContextService", 0xa6f2c885));
        }
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("GameEntityContextService", 0xa6f2c885));
        }
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("SliceSystemService", 0x1a5b7aad));
        }
    };
} // namespace AzFramework

#endif // AZFRAMEWORK_GAMEENTITYCONTEXTCOMPONENT_H
