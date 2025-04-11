/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzFramework/Spawnable/SpawnableMetaData.h>

namespace AZ
{
    class ReflectContext;
}

namespace AzFramework
{
    class Spawnable final
        : public AZ::Data::AssetData
    {
    public:
        AZ_CLASS_ALLOCATOR(Spawnable, AZ::SystemAllocator);
        AZ_RTTI(AzFramework::Spawnable, "{855E3021-D305-4845-B284-20C3F7FDF16B}", AZ::Data::AssetData);

        // The order is important for sorting in the SpawnableAssetHandler.
        enum class EntityAliasType : uint8_t
        {
            Original,   //!< The original entity is spawned.
            Disable,    //!< No entity will be spawned.
            Replace,    //!< The entity alias is spawned instead of the original.
            Additional, //!< The original entity is spawned as well as the alias. The alias will get a new entity id.
            Merge       //!< The original entity is spawned and the components of the alias are added. The caller is responsible for
                        //!< maintaining a valid component list.
        };

        enum ShareState : int32_t
        {
            Read = -1,
            NotShared = 0,
            ReadWrite = 1
        };

        //! An entity alias redirects the spawning of an entity to another entity, possibly in another spawnable.
        struct EntityAlias
        {
            AZ_CLASS_ALLOCATOR(EntityAlias, AZ::SystemAllocator);
            AZ_TYPE_INFO(AzFramework::Spawnable::EntityAlias, "{C8D0C5BC-1F0B-4572-98C1-73B2CA8C9356}");

            bool HasLowerIndex(const EntityAlias& other) const;

            AZ::Data::Asset<Spawnable> m_spawnable; //!< The spawnable containing the target entity to spawn.
            uint32_t m_tag{ 0 }; //!< A unique tag to identify this alias with.
            uint32_t m_sourceIndex{ 0 }; //!< The index of the entity in the original spawnable that will be replaced.
            uint32_t m_targetIndex{ 0 }; //!< The index of the entity in the target spawnable that will be used to replace the original.
            EntityAliasType m_aliasType{ EntityAliasType::Original }; //!< The kind of replacement.
            bool m_queueLoad{ false }; //!< Whether or not to automatically queue the spawnable for loading.

            static void Reflect(AZ::ReflectContext* context);
        };

        using EntityList = AZStd::vector<AZStd::unique_ptr<AZ::Entity>>;
        using EntityAliasList = AZStd::vector<EntityAlias>;

        class EntityAliasConstVisitor
        {
        protected:
            using ListTargetSpawanblesCallback = AZStd::function<void(const AZ::Data::Asset<Spawnable>& targetSpawnable)>;

        public:
            EntityAliasConstVisitor(const Spawnable& owner, const EntityAliasList* entityAliasList);
            ~EntityAliasConstVisitor();

            EntityAliasConstVisitor(EntityAliasConstVisitor&& rhs);
            EntityAliasConstVisitor& operator=(EntityAliasConstVisitor&& rhs);

            EntityAliasConstVisitor(const EntityAliasConstVisitor& rhs) = delete;
            EntityAliasConstVisitor& operator=(const EntityAliasConstVisitor& rhs) = delete;

            //! Checks if the visitor was able to retrieve data. This needs to be checked before calling any other functions.
            bool IsValid() const;

            bool HasAliases() const;
            bool AreAllSpawnablesReady() const;

            // Modification of aliases is limited to specific changes that can only be done through the available modification functions.
            // For this reason access through iterators is limited to unmodifiable constant iterators.
            EntityAliasList::const_iterator begin() const;
            EntityAliasList::const_iterator end() const;
            EntityAliasList::const_iterator cbegin() const;
            EntityAliasList::const_iterator cend() const;

            void ListTargetSpawnables(const ListTargetSpawanblesCallback& callback) const;
            void ListTargetSpawnables(AZ::Crc32 tag, const ListTargetSpawanblesCallback& callback) const;

        protected:
            const Spawnable& m_owner;
            const EntityAliasList* m_entityAliasList{};
        };

        class EntityAliasVisitor final : public EntityAliasConstVisitor
        {
        public:
            EntityAliasVisitor(Spawnable& owner, EntityAliasList* m_entityAliasList);
            ~EntityAliasVisitor();

            EntityAliasVisitor(EntityAliasVisitor&& rhs);
            EntityAliasVisitor& operator=(EntityAliasVisitor&& rhs);

            EntityAliasVisitor(const EntityAliasVisitor& rhs) = delete;
            EntityAliasVisitor& operator=(const EntityAliasVisitor& rhs) = delete;

            void AddAlias(
                AZ::Data::Asset<Spawnable> targetSpawnable,
                AZ::Crc32 tag,
                uint32_t sourceIndex,
                uint32_t targetIndex,
                Spawnable::EntityAliasType aliasType,
                bool queueLoad);

            using ListSpawnablesRequiringLoadCallback = AZStd::function<void(AZ::Data::Asset<Spawnable>& spawnablePendingLoad)>;
            void ListSpawnablesRequiringLoad(const ListSpawnablesRequiringLoadCallback& callback);

            using UpdateCallback = AZStd::function<void(
                Spawnable::EntityAliasType& aliasType,
                bool& queueLoad,
                const AZ::Data::Asset<Spawnable>& aliasedSpawnable,
                const AZ::Crc32 tag,
                const uint32_t sourceIndex,
                const uint32_t targetIndex)>;
            void UpdateAliases(const UpdateCallback& callback);
            void UpdateAliases(AZ::Crc32 tag, const UpdateCallback& callback);
            void UpdateAliasType(uint32_t index, Spawnable::EntityAliasType newType);

            void Optimize();

        private:
            EntityAliasList* GetEntityAliasList() const;

            bool m_dirty{ false };
        };

        inline static constexpr const char* DefaultMainSpawnableName = "Root";
        inline static constexpr const char* FileExtension = "spawnable";
        inline static constexpr const char* DotFileExtension = ".spawnable";

        Spawnable() = default;
        explicit Spawnable(const AZ::Data::AssetId& id, AssetStatus status = AssetStatus::NotLoaded);
        Spawnable(const Spawnable& rhs) = delete;
        Spawnable(Spawnable&& other) = delete;
        ~Spawnable() override = default;

        Spawnable& operator=(const Spawnable& rhs) = delete;
        Spawnable& operator=(Spawnable&& other) = delete;

        const EntityList& GetEntities() const;
        EntityList& GetEntities();
        EntityAliasConstVisitor TryGetAliasesConst() const;
        EntityAliasConstVisitor TryGetAliases() const;
        EntityAliasVisitor TryGetAliases();
        bool IsEmpty() const;

        SpawnableMetaData& GetMetaData();
        const SpawnableMetaData& GetMetaData() const;

        static void Reflect(AZ::ReflectContext* context);

    private:
        SpawnableMetaData m_metaData;

        // Aliases that optionally replace the ones stored in this spawnable.
        EntityAliasList m_entityAliases;
        // Container for keeping all entities of the prefab the Spawnable was created from.
        // Includes both direct and nested entities of the prefab.
        EntityList m_entities;

        mutable AZStd::atomic<int32_t> m_shareState{ ShareState::NotShared };
    };

    using SpawnableAsset = AZ::Data::Asset<AzFramework::Spawnable>;
    using SpawnableAssetVector = AZStd::vector<AZ::Data::Asset<AzFramework::Spawnable>>;
} // namespace AzFramework
