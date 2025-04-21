/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/Slice/SliceAsset.h>
#include <AzCore/Serialization/DataPatch.h>
#include <AzCore/Serialization/IdUtils.h>

namespace AZ
{
    /**
     * Slice component manages entities on the project graph (Slice is a node). SliceComponent with nested dependencies is an edit
     * only concept. We have a runtime version with dependencies so that we can accelerate production by live tweaking
     * of game objects. Otherwise all slices should be exported as list of entities (flat structure). There is an exception with the
     * "dynamic" slices, which should still be a flat list of entities that you "clone" for dynamic reuse.
     */
    class SliceComponent
        : public Component
        , public Data::AssetBus::MultiHandler
    {
        friend class SliceComponentSerializationEvents;
        friend class SliceAssetHandler;
    public:
        AZ_COMPONENT(SliceComponent, "{AFD304E4-1773-47C8-855A-8B622398934F}", Data::AssetEvents);

        class SliceReference;
        class SliceInstance;

        using SliceInstancePtrSet = AZStd::unordered_set<SliceInstance*>;
        using SliceReferenceToInstancePtrs = AZStd::unordered_map<SliceReference*, SliceInstancePtrSet>;
        using SliceAssetToSliceInstancePtrs = AZStd::unordered_map<Data::Asset<SliceAsset>, SliceInstancePtrSet>;
        /**
         * Points to a specific instantiation of a nested slice.
         */
        class SliceInstanceAddress
        {
        public:
            AZ_TYPE_INFO(SliceInstanceAddress, "{94142EA2-1319-44D5-82C8-A6D9D34A63BC}");

            SliceInstanceAddress();
            SliceInstanceAddress(SliceReference* reference, SliceInstance* instance);
            SliceInstanceAddress(const SliceInstanceAddress& RHS);
            SliceInstanceAddress(const SliceInstanceAddress&& RHS);

            bool operator==(const SliceInstanceAddress& rhs) const;
            bool operator!=(const SliceInstanceAddress& rhs) const;

            SliceInstanceAddress& operator=(const SliceInstanceAddress& RHS);

            bool IsValid() const;
            explicit operator bool() const;

            const SliceReference* GetReference() const;
            SliceReference* GetReference();
            void SetReference(SliceReference* reference);

            const SliceInstance* GetInstance() const;
            SliceInstance* GetInstance();
            void SetInstance(SliceInstance* instance);

            // DEPRECATED
            // SlinceInstanceAddress used to be an AZStd::pair, so 'first' and 'second' are provided to maintain backwards compatibility.
            // Use GetReference() and GetInstance() instead.
            SliceReference*& first;
            SliceInstance*& second;

        private:
            SliceReference* m_reference = nullptr; ///< The SliceReference to which this SliceInstance belongs
            SliceInstance* m_instance = nullptr; ///< The specific instantiation of the nested slice
        };

        using SliceInstanceAddressSet = AZStd::unordered_set<SliceInstanceAddress>;
        using EntityList = AZStd::vector<Entity*>;
        using EntityIdToEntityIdMap = AZStd::unordered_map<EntityId, EntityId>;
        using SliceInstanceEntityIdRemapList = AZStd::vector<AZStd::pair<SliceInstanceAddress, EntityIdToEntityIdMap>>;
        using SliceInstanceToSliceInstanceMap = AZStd::unordered_map<SliceInstanceAddress, SliceInstanceAddress>;
        using EntityIdSet = AZStd::unordered_set<AZ::EntityId>;
        using SliceInstanceId = AZ::Uuid;

        /// @deprecated Use SliceReference.
        using PrefabReference = SliceReference;

        /// @deprecated Use SliceInstance.
        using PrefabInstance = SliceInstance;

        /// @deprecated Use SliceInstanceAddress.
        using PrefabInstanceAddress = SliceInstanceAddress;

        /// @deprecated Use SliceInstanceId.
        using PrefabInstanceId = SliceInstanceId;

        /**
         * For each entity, flags which may affect slice data inheritance.
         *
         * For example, if an entity has a component field flagged with ForceOverride,
         * that field never inherits the value from its the corresponding entity in the reference slice.
         *
         * Data flags affect how inheritance works, but the flags themselves are not inherited.
         * Data flags live at a particular level in an entity's slice ancestry.
         * For this reason, data flags are not stored within an entity or its components
         * (all of which are inherited by slice instances).
         *
         * See @ref DataPatch::Flags, @ref DataPatch::Flag, @ref DataPatch::FlagsMap for more.
         *
         * Care is taken to prune this datastructure so that no empty entries are stored.
         */
        class DataFlagsPerEntity
        {
        public:
            AZ_CLASS_ALLOCATOR(DataFlagsPerEntity, AZ::SystemAllocator);
            AZ_TYPE_INFO(DataFlagsPerEntity, "{57FE7B9E-B2AF-4F6F-9F8D-87F671E91C99}");
            static void Reflect(ReflectContext* context);

            using DataFlagsTransformFunction = AZStd::function<DataPatch::Flags(DataPatch::Flags)>;

            /// Function used to check whether a given entity ID is allowed.
            using IsValidEntityFunction = AZStd::function<bool(EntityId)>;

            DataFlagsPerEntity(const IsValidEntityFunction& isValidEntityFunction = nullptr);

            void SetIsValidEntityFunction(const IsValidEntityFunction& isValidEntityFunction);

            /// Replace all data flags.
            void CopyDataFlagsFrom(const DataFlagsPerEntity& rhs);
            void CopyDataFlagsFrom(DataFlagsPerEntity&& rhs);

            /// Add data flags.
            /// @from add data flags from this object
            /// @remapFromIdToId (optional) If provided, when importing data flags, remap which EntityId receives the flags.
            ///                  If an EntityId is not found in the map then its flags are not imported.
            ///                  If no map is provided then all data flags are imported and EntityIds are not remapped.
            /// @dataFlagsTransformFn (optional) If function is provided, then it will be applied to all imported flags.
            void ImportDataFlagsFrom(const DataFlagsPerEntity& from, const EntityIdToEntityIdMap* remapFromIdToId = nullptr, const DataFlagsTransformFunction& dataFlagsTransformFn = nullptr);

            /// Return all entities' data flags, using addresses that align with those in a data patch.
            DataPatch::FlagsMap GetDataFlagsForPatching(const EntityIdToEntityIdMap* remapFromIdToId = nullptr) const;

            /// Change all EntityIds.
            /// If an existing EntityId is not found in the map then its data is removed.
            void RemapEntityIds(const EntityIdToEntityIdMap& remapFromIdToId);

            /// Return all data flags for this entity.
            /// Addresses are relative the entity.
            const DataPatch::FlagsMap& GetEntityDataFlags(EntityId entityId) const;

            /// Set all data flags for this entity.
            /// Addresses should be relative to the entity.
            /// @return True if flags are set. False if IsValidEntity fails.
            bool SetEntityDataFlags(EntityId entityId, const DataPatch::FlagsMap& dataFlags);

            /// Set all data flags for this entity.
            /// Addresses should be relative to the entity.
            /// Should only be used during the Undo process
            /// Does not check IsValidEntity, as an entity is never valid during the undo process, only after
            /// @entityId the Id of the entity you wish to perform an undo action on
            /// @dataFlags the data flags you wish to map to the given entity Id
            void SetEntityDataFlagsForUndo(EntityId entityId, const DataPatch::FlagsMap& dataFlags);

            /// Clear all data flags for this entity.
            /// @return True if flags are cleared. False if IsValidEntity fails.
            bool ClearEntityDataFlags(EntityId entityId);

            /// Get the data flags set at a particular address within this entity.
            DataPatch::Flags GetEntityDataFlagsAtAddress(EntityId entityId, const DataPatch::AddressType& dataAddress) const;

            /// Set the data flags at a particular address within this entity.
            /// @return True if flags are set. False if IsValidEntity fails.
            bool SetEntityDataFlagsAtAddress(EntityId entityId, const DataPatch::AddressType& dataAddress, DataPatch::Flags flags);

            /// @return True if entityId passes the IsValidEntityFunction.
            /// Always returns true if no IsValidEntityFunction is set.
            bool IsValidEntity(EntityId entityId) const;

            /// Discard any entries for entities or addresses that no longer exist
            void Cleanup(const EntityList& validEntities);

        private:
            AZStd::unordered_map<EntityId, DataPatch::FlagsMap> m_entityToDataFlags;
            IsValidEntityFunction m_isValidEntityFunction;

            // Prevent copying whole class, since user probably doesn't want to change the IsValidEntityFunction.
            DataFlagsPerEntity(const DataFlagsPerEntity&) = delete;
            DataFlagsPerEntity& operator=(const DataFlagsPerEntity&) = delete;

        };
        using EntityDataFlagsMap = AZStd::unordered_map<EntityId, DataPatch::FlagsMap>;

        /**
        * Represents an ancestor of a live entity within a slice.
        * GetInstanceEntityAncestry can be used to retrieve all ancestral entities for a given entity,
        * or in other words, retrieve the corresponding entity at each level of the slice data hierarchy.
        */
        struct Ancestor
        {
            Ancestor()
                : m_entity(nullptr)
                , m_sliceAddress(nullptr, nullptr)
            {
            }
            Ancestor(AZ::Entity* entity, const SliceInstanceAddress& sliceAddress)
                : m_entity(entity)
                , m_sliceAddress(sliceAddress)
            {
            }
            AZ::Entity* m_entity;
            SliceInstanceAddress m_sliceAddress;
        };

        using EntityAncestorList = AZStd::vector<Ancestor>;

        SliceComponent();
        ~SliceComponent() override;

        /**
         * Container for instantiated entities (which are not serialized), but build from the source object and deltas \ref DataPatch.
         */
        struct InstantiatedContainer
        {
            AZ_CLASS_ALLOCATOR(InstantiatedContainer, SystemAllocator);
            AZ_TYPE_INFO(InstantiatedContainer, "{05038EF7-9EF7-40D8-A29B-503D85B85AF8}");

            InstantiatedContainer(bool deleteEntitiesOnDestruction = true);
            InstantiatedContainer(SliceComponent* sourceComponent, bool deleteEntitiesOnDestruction = true);
            ~InstantiatedContainer();

            void DeleteEntities();

            /**
            * Non-Destructively clears all data.
            * @note Only call this when using this type as a temporary object to store pointers to data owned elsewhere.
            */
            void ClearAndReleaseOwnership();

            EntityList m_entities;
            EntityList m_metadataEntities;
            bool m_deleteEntitiesOnDestruction;
        };

        /**
         * Stores information required to restore an entity back into an internal slice instance, which 
         * includes the address of the reference and owning instance at the time of capture.
         * EntityRestoreInfo must be retrieved via SliceReference::GetEntityRestoreInfo(). It can then be
         * provided to SliceComponent::RestoreEntity() to restore the entity, at which point the owning
         * reference and instance will be re-created if needed.
         */
        struct EntityRestoreInfo
        {
            AZ_TYPE_INFO(EntityRestoreInfo, "{AF2BE53F-C212-4BA4-880C-E1859BE75EA9}");

            EntityRestoreInfo()
            {}

            /**
             * @param asset The source slice asset of the slice instance containing the to-be-restored entity.
             * @param instanceId The Id of the slice instance that manages local data patches for its entities, including the entity in question.
             * @param ancestorId The Id of the entity defined in the source slice asset, that is the immediate ancestor of the to-be-restored entity. 
             * @param dataFlags A copy of the slice instance data flags before the entity is removed.
             */
            EntityRestoreInfo(const Data::Asset<SliceAsset>& asset, const SliceInstanceId& instanceId, const EntityId& ancestorId, const DataPatch::FlagsMap& dataFlags)
                : m_assetId(asset.GetId())
                , m_instanceId(instanceId)
                , m_ancestorId(ancestorId)
                , m_dataFlags(dataFlags)
            {}

            inline operator bool() const
            {
                return m_assetId.IsValid();
            }

            Data::AssetId       m_assetId;
            SliceInstanceId     m_instanceId;
            EntityId            m_ancestorId;
            DataPatch::FlagsMap m_dataFlags;
        };

        using EntityRestoreInfoList = AZStd::vector<AZStd::pair<AZ::EntityId, AZ::SliceComponent::EntityRestoreInfo>>;

        /**
         * Represents a slice instance in the current slice. For example if you refer to a base slice "lamppost"
         * you can have multiple instances of that slice (all of them with custom overrides) in the current slice.
         */
        class SliceInstance
        {
            friend class SliceComponent;
        public:

            AZ_TYPE_INFO(SliceInstance, "{E6F11FB3-E9BF-43BA-BD78-2A19F51D0ED3}");

            SliceInstance(const SliceInstanceId& id = SliceInstanceId::CreateRandom());
            SliceInstance(SliceInstance&& rhs);
            SliceInstance(const SliceInstance& rhs);
            ~SliceInstance();
            SliceInstance& operator=(const SliceInstance& rhs);

            /// Clears the instantiated container. Used when layers are borrowing and returning slice instances.
            void ClearInstantiated()
            {
                m_instantiated = nullptr;
            }

            /// Returns pointer to the instantiated entities. Null if the slice was not instantiated.
            const InstantiatedContainer* GetInstantiated() const
            {
                return m_instantiated;
            }

            /// Returns a reference to the data patch, it's always available even when the \ref DataPatch contain no data to apply (no deltas)
            const DataPatch& GetDataPatch() const
            {
                return m_dataPatch;
            }

            /// Returns a reference to the data flags per entity.
            const DataFlagsPerEntity& GetDataFlags() const
            {
                return m_dataFlags;
            }

            /// Returns a reference to the data flags per entity.
            DataFlagsPerEntity& GetDataFlags()
            {
                return m_dataFlags;
            }

            /// Returns a non-const mapping of the EntityIDs from the base entities, to the new EntityIDs of the entities we will instantiate with this instance.
            /// Marks the reverse lookup table dirty by clearing it
            /// @Note: This map may contain mappings to inactive entities.
            /// @Note: Editing this map will impact future instantiations of this instance. From what entities it instantiates from base to the ids of the entities it instantiates
            EntityIdToEntityIdMap& GetEntityIdMapForEdit()
            {
                m_entityIdToBaseCache.clear();
                return m_baseToNewEntityIdMap;
            }

            /// Returns a mapping of the EntityIDs from the base entities, to the new EntityIDs of the entities we will instantiate with this instance.
            /// @Note: This map may contain mappings to inactive entities.
            const EntityIdToEntityIdMap& GetEntityIdMap() const
            {
                return m_baseToNewEntityIdMap;
            }

            /// Returns the reverse of \ref GetEntityIdMap. The reverse table is built on demand.
            /// @Note: This map may contain mappings to inactive entities.
            const EntityIdToEntityIdMap& GetEntityIdToBaseMap() const
            {
                if (m_entityIdToBaseCache.empty())
                {
                    BuildReverseLookUp();
                }
                return m_entityIdToBaseCache;
            }

            /// Returns whether this entity exists in this instance.
            bool IsValidEntity(EntityId entityId) const;

            /// Returns the instance's unique Id.
            const SliceInstanceId& GetId() const
            {
                return m_instanceId;
            }

            // Instances are stored in a set - AZStd::hash<> requirement.
            operator size_t() const
            {
                return m_instanceId.GetHash();
            }

            // Returns the metadata entity for this instance.
            Entity* GetMetadataEntity() const;

        protected:

            void SetId(const SliceInstanceId& id)
            {
                m_instanceId = id;
            }

            /// Returns data flags whose addresses align with those in the data patch.
            DataPatch::FlagsMap GetDataFlagsForPatching() const;

            static DataFlagsPerEntity::IsValidEntityFunction GenerateValidEntityFunction(const SliceInstance*);

            // The lookup is built lazily when accessing the map, but constness is desirable
            // in the higher level APIs.
            void BuildReverseLookUp() const;
            InstantiatedContainer* m_instantiated; ///< Runtime only list of instantiated objects (serialization stores the delta \ref m_dataPatch only)
            mutable EntityIdToEntityIdMap m_entityIdToBaseCache; ///< reverse lookup to \ref m_baseToNewEntityIdMap, this is build on demand

            EntityIdToEntityIdMap m_baseToNewEntityIdMap; ///< Map of old entityId to new
            DataPatch m_dataPatch; ///< Stored data patch which will take us from the dependent slice to the instantiated entities. Addresses are relative to the InstantiatedContainer.
            DataFlagsPerEntity m_dataFlags; ///< For each entity, flags which may affect slice data inheritance. Addresses are relative to the entity, not the InstantiatedContainer.

            SliceInstanceId m_instanceId; ///< Unique Id of the instance.

            /// Pointer to the clone metadata entity associated with the asset used to instantiate this class.
            /// Data is owned the m_instantiated member of this class.
            Entity* m_metadataEntity;
        };

        /**
         * Reference to a dependent slice. Each dependent slice can have one or more
         * instances in the current slice.
         */
        class SliceReference
        {
            friend class SliceComponent;
        public:
            using SliceInstances = AZStd::unordered_set<SliceInstance>;

            /// @deprecated Use SliceInstances
            using PrefabInstances = SliceInstances;

            AZ_TYPE_INFO(SliceReference, "{F181B80D-44F0-4093-BB0D-C638A9A734BE}");

            SliceReference();

            /**
             * Create a new instance of the slice (with new IDs for every entity).
             * @param customMapper Used to generate runtime entity ids for the new instance. By default runtime ids are randomly generated.
             * @param sliceInstanceId The id assigned to the slice instance to be created. If no argument is passed in and random id will be generated as default.
             *    If the same sliceInstanceId is already registered to this reference a null SliceInstance is returned as error.
             * @return A pointer to the newly created slice instance. Returns nullptr on error.
             */
            SliceInstance* CreateInstance(const AZ::IdUtils::Remapper<AZ::EntityId>::IdMapper& customMapper = nullptr, 
                 SliceInstanceId sliceInstanceId = SliceInstanceId::CreateRandom());

            /**
             * Create a new instance of the slice out of a list of existing entities
             * @param entities A list of existing entities that will be moved into the new instance's InstantiatedContainer
             * @param assetToLiveIdMap A mapping between the asset EntityIDs of the slice asset and the "Live" entities passed in
             * @param sliceInstanceId The id assigned to the slice instance to be created. If no argument is passed in a random id will be generated as default.
             *    If the same sliceInstanceId is already registered to this reference a null SliceInstance is returned as error.
             * @return A pointer to the newly created slice instance. Returns nullptr on error or if the SliceComponent is not instantiated.
            */
            SliceInstance* CreateInstanceFromExistingEntities(AZStd::vector<AZ::Entity*>& entities,
                const EntityIdToEntityIdMap& assetToLiveIdMap,
                SliceInstanceId sliceInstanceId = SliceInstanceId::CreateRandom());

            /** 
             * Clones an existing slice instance
             * @param instance Source slice instance to be cloned
             * @param sourceToCloneEntityIdMap [out] The map between source entity ids and clone entity ids
             * @return A clone of \ref instance
             */
            SliceInstance* CloneInstance(SliceInstance* instance, EntityIdToEntityIdMap& sourceToCloneEntityIdMap);

            /**
             * Restores ownership of the passed in slice instance to this SliceReference, and clears the passed
             * in slice instance. This is used primarily by the layer system, to allow layers to briefly take
             * ownership of slice instances that are saved to those layers.
             * \param cachedInstance A valid, cached slice instance that used to be owned by this reference.
            */
            void RestoreAndClearCachedInstance(SliceInstance& cachedInstance);

            /// Locates an instance by Id.
            SliceInstance* FindInstance(const SliceInstanceId& instanceId);

            /** 
             * Remove a slice instance.
             * @param itr The iterator to the slice instance to remove.
             * @return The iterator following the last removed slice instance.
             */
            SliceReference::SliceInstances::iterator RemoveInstance(SliceInstances::iterator itr);

            /// Removes an instance of the slice.
            bool RemoveInstance(SliceInstance* instance);

            /// Remove an entity if belongs to some of the instances
            bool RemoveEntity(EntityId entityId, bool isDeleteEntity, SliceInstance* instance = nullptr);

            /// Retrieves the list of instances for this reference.
            const SliceInstances& GetInstances() const;

            /// Retrieves the list of instances for this reference.
            SliceInstances& GetInstances() { return m_instances; }

            /// Retrieves the underlying asset pointer.
            const Data::Asset<SliceAsset>& GetSliceAsset() const { return m_asset; }

            /// Retrieves the SliceComponent this reference belongs to.
            SliceComponent* GetSliceComponent() const { return m_component; }

            /// Checks if instances are instantiated
            bool IsInstantiated() const;

            /// Retrieves the specified entity's chain of ancestors and their associated assets along the slice data hierarchy.
            /// \param instanceEntityId - Must be Id of an entity within a live instance.
            /// \param ancestors - Output list of ancestors, up to maxLevels.
            /// \param maxLevels - Maximum cascade levels to explore.
            bool GetInstanceEntityAncestry(const AZ::EntityId& instanceEntityId, EntityAncestorList& ancestors, u32 maxLevels = 8) const;

            /// Compute data patch for all instances
            void ComputeDataPatch();

            /// Compute data patch for an individual instance
            void ComputeDataPatch(SliceInstance* instance);
        protected:

            /// Internal only function that computes the data patch for the given instance.
            /// This assumes that the instance has already been verified to be related to this slice.
            void ComputeDataPatchForInstanceKnownToReference(SliceInstance& instance, SerializeContext* serializeContext, InstantiatedContainer& sourceContainer);

            /// Creates a new Id'd instance slot internally, but does not instantiate it.
            SliceInstance* CreateEmptyInstance(const SliceInstanceId& instanceId = SliceInstanceId::CreateRandom());

            /// Helper that returns a valid empty instance if the slice reference is able to make instances at time of call
            /// Otherwise returns a null slice instance
            SliceInstance* PrepareCreateInstance(const SliceInstanceId& sliceInstanceId, bool allowUninstantiated);

            /// Helper that performs EntityID remaps, registers the instance in the SliceReference's EntityInfoMap and fixes up MetaDataEntities
            SliceInstance* FinalizeCreateInstance(SliceInstance& instance,
                void* remapContainer, const AZ::Uuid& classUuid,
                AZ::SerializeContext* serializeContext,
                const AZ::IdUtils::Remapper<AZ::EntityId>::IdMapper& customMapper = nullptr);

            /// Instantiate all instances (by default we just hold the deltas - data patch), the Slice component controls the instantiate state
            /// serializeContext and relativeToAbsoluteSlicePaths arguments are used for a specific case when asset processor is not available
            bool Instantiate(
                const AZ::ObjectStream::FilterDescriptor& filterDesc,
                AZ::SerializeContext* serializeContext = nullptr,
                AZStd::unordered_map<AZStd::string, AZStd::string>* relativeToAbsoluteSlicePaths = nullptr);

            void UnInstantiate();

            void InstantiateInstance(SliceInstance& instance, const AZ::ObjectStream::FilterDescriptor& filterDesc);

            void AddInstanceToEntityInfoMap(SliceInstance& instance);

            void RemoveInstanceFromEntityInfoMap(SliceInstance& instance);

            void FixUpMetadataEntityForSliceInstance(SliceInstance* sliceInstance);

            bool m_isInstantiated;

            SliceComponent* m_component = nullptr;

            SliceInstances m_instances; ///< Instances of the slice in our slice reference

            Data::Asset<SliceAsset> m_asset{ Data::AssetLoadBehavior::PreLoad }; ///< Asset reference to a dependent slice reference
        };

        using SliceAssetToSliceInstances = AZStd::unordered_map<Data::Asset<SliceAsset>, SliceReference::SliceInstances>;

        using SliceList = AZStd::list<SliceReference>; // we use list as we need stable iterators

        /**
         * Map from entity ID to entity and slice address (if it comes from a slice).
         */
        struct EntityInfo
        {
            EntityInfo()
                : m_entity(nullptr)
                , m_sliceAddress(nullptr, nullptr)
            {}

            EntityInfo(Entity* entity, const SliceInstanceAddress& sliceAddress)
                : m_entity(entity)
                , m_sliceAddress(sliceAddress)
            {}

            Entity* m_entity; ///< Pointer to the entity
            SliceInstanceAddress m_sliceAddress; ///< Address of the slice
        };
        using EntityInfoMap = AZStd::unordered_map<EntityId, EntityInfo>;

        SerializeContext* GetSerializeContext() const
        {
            return m_serializeContext;
        }

        void SetSerializeContext(SerializeContext* context)
        {
            m_serializeContext = context;
        }

        /// Connect to asset bus for dependencies.
        void ListenForAssetChanges();

        /// This will listen for child asset changes and instruct all other slices in the slice
        /// hierarchy to do the same.
        void ListenForDependentAssetChanges();

        /// Returns list of the entities that are "new" for the current slice (not based on an existing slice)
        const EntityList& GetNewEntities() const;

        /// Returns true for entities that are "new" entities in this slice (entities not based on another slice)
        bool IsNewEntity(EntityId entityId) const;

        /**
         * Returns all entities including the ones based on instances, you need to provide container as we don't
         * keep all entities in one list (we can change that if we need it easily).
         * If entities are not instantiated (the ones based on slices) \ref Instantiate will be called
         * \returns true if entities list has been populated (even if empty), and false if instantiation failed.
         */
        bool GetEntities(EntityList& entities);

        /**
         * Erases all entities passed in from this slice's m_entities list of loose, owned entities.
         * @param entities The list of entities to erase.
        */
        void EraseEntities(const EntityList& entities);

        /**
         * Adds the passed in entities to become owned and tracked by this slice. Used by the layer system
         * to return entities that were briefly borrowed from the root slice.
         * entities The list of entitites to add to this slice.
         */
        void AddEntities(const EntityList& entities);

        /**
         * Clears the m_entities list for this slice and replaces it with the passed in entity list.
         * Used by the layer system to return entities it borrowed. Used as a performance shortcut, the
         * layer system will have already retrieved a complete list of all entities managed by this slice.
         * @param entities The list of entities to replace m_entities with.
         */
        void ReplaceEntities(const EntityList& entities);

        /**
         * Adds the passed in slice instances to this slice. Used by the layer system on load, when a layer
         * is loaded, it may contain one or more slice instances. This allows layers to populate the root slice
         * with those instances.
         * @param sliceInstances instances that will be given to this slice. It will be empty after this operation completes.
         */
        void AddSliceInstances(SliceAssetToSliceInstancePtrs& sliceInstances, AZStd::unordered_set<const SliceInstance*>& instancesOut);
        
        /**
         * Adds the IDs of all non-metadata entities, including the ones based on instances, to the provided set.
        * @param entities An entity ID set to add the IDs to
        */
        bool GetEntityIds(EntityIdSet& entities);

        /**
         * Returns the count of all instantiated entities, including the ones based on instances.
         * If the slice has not been instantiated then 0 is returned.
         */
        size_t GetInstantiatedEntityCount() const;

        /**
        * Adds ID of every metadata entity that's part of this slice, including those based on instances, to the
        * given set.
        * @param entities An entity ID set to add the IDs to
        */
        bool GetMetadataEntityIds(EntityIdSet& entities);

        /// Returns list of all slices and their instantiated entities (for this slice component)
        const SliceList& GetSlices() const;

        /// Returns list of all slices and their instantiated entities (for this slice component)
        SliceList& GetSlices() { return m_slices; }

        /**
        * Returns the list of slice references associated with this slice that could not be loaded.
        * \returns The list of invalid slice references. This is empty if there are none.
        */
        const SliceList& GetInvalidSlices() const;

        SliceReference* GetSlice(const Data::Asset<SliceAsset>& sliceAsset);
        SliceReference* GetSlice(const Data::AssetId& sliceAssetId);

        /**
         * Adds a dependent slice, and instantiate the slices if needed.
         * @param sliceAsset slice asset.
         * @param customMapper optional entity runtime id mapper.
         * @param assetLoadFilter Optional asset load filter. Any filtered-out asset references that are not already memory-resident will not trigger loads.
         * @param sliceInstanceId The id assigned to the slice instance to be created. If no argument is passed in a random id will be generated as default.
         *    If the same sliceInstanceId is already registered to the underlying SlicReference an invalid SliceInstanceAddress is returned as error.
         * @returns A SliceInstanceAddress pointing to the added instance. An invalid SliceInstanceAddress if adding slice failed.
         */
        SliceInstanceAddress AddSlice(const Data::Asset<SliceAsset>& sliceAsset, const AZ::IdUtils::Remapper<AZ::EntityId>::IdMapper& customMapper = nullptr, 
            SliceInstanceId sliceInstanceId = SliceInstanceId::CreateRandom());

        /**
         * @param sliceAsset Asset of the slice being added
         * @param assetToLiveMap Mapping from the provided slice asset's EntityID's to existing EntityIDs.
         *    Existing EntityIDs should be owned by the SliceComponent making this call.
         * @param sliceInstanceId The id assigned to the slice instance to be created. If no argument is passed in a random id will be generated as default.
         *    If the same sliceInstanceId is already registered to an invalid SliceInstanceAddress is returned as error.
         * @returns A SliceInstanceAddress pointing to the added instance. A null SliceInstanceAddress if adding slice failed.
        */
        SliceInstanceAddress AddSliceUsingExistingEntities(const Data::Asset<SliceAsset>& sliceAsset, const AZ::SliceComponent::EntityIdToEntityIdMap& assetToLiveMap,
            SliceInstanceId sliceInstanceId = SliceInstanceId::CreateRandom());

        /// Adds a slice reference (moves it) along with its instance information.
        SliceReference* AddSlice(SliceReference& sliceReference);
        /// Adds a slice (moves it) from another already generated reference/instance pair.
        SliceInstanceAddress AddSliceInstance(SliceReference* sliceReference, SliceInstance* sliceInstance);

        /**
        * @param sourceSliceInstance The slice instance that contains the sub-slice instance
        * @param sourceSubsliceInstanceAncestry The ancestry in order from sourceSubslice to sourceSlice
        * @param sourceSubsliceInstanceAddress The address of the sub-slice instance to be cloned
        * @param subsliceToLiveMappingResult Stores the resulting mapping from the sub slice's base Entity Ids to the source slice's live Entity Ids
        * @param flipMapping subsliceToLiveMappingResult will be flipped to instead contain the mapping from source slice to sub slice. Defaults to false
        */
        static void GetMappingBetweenSubsliceAndSourceInstanceEntityIds(const SliceComponent::SliceInstance* sourceSliceInstance,
            const AZStd::vector<AZ::SliceComponent::SliceInstanceAddress>& sourceSubsliceInstanceAncestry,
            const AZ::SliceComponent::SliceInstanceAddress& sourceSubsliceInstanceAddress,
            AZ::SliceComponent::EntityIdToEntityIdMap& subsliceToLiveMappingResult,
            bool flipMapping = false);

        /**
         * Given a sub-slice instance, create a slice reference based on the sub-slice instance's SliceAsset (if the slice reference doesn't already exist), then clone the sub-slice instance and add the clone to the slice reference just created.
         * @param sourceSliceInstance The slice instance that contains the sub-slice instance.
         * @param sourceSubSliceInstanceAncestry The ancestry in order from sourceSubSlice to sourceSlice
         * @param sourceSubSliceInstanceAddress The address of the sub-slice instance to be cloned.
         * @param out_sourceToCloneEntityIdMap [Optional] If provided, the internal source to clone entity ID map will be returned 
         * @param preserveIds [Optional] If true will not generate new IDs for the clone and will direct entity ID maps from base ID to existing live IDs of the original
         * @return The address of the newly created clone-instance.
         */
        SliceInstanceAddress CloneAndAddSubSliceInstance(const SliceInstance* sourceSliceInstance, 
            const AZStd::vector<AZ::SliceComponent::SliceInstanceAddress>& sourceSubSliceInstanceAncestry,
            const AZ::SliceComponent::SliceInstanceAddress& sourceSubSliceInstanceAddress,
            AZ::SliceComponent::EntityIdToEntityIdMap* out_sourceToCloneEntityIdMap = nullptr, bool preserveIds = false);

        /// Remove an entire slice asset reference and all its instances.
        bool RemoveSlice(const Data::Asset<SliceAsset>& sliceAsset);
        bool RemoveSlice(const SliceReference* slice);

        /// Removes the slice instance, if this is last instance the SliceReference will be removed too.
        bool RemoveSliceInstance(SliceInstance* instance);
        bool RemoveSliceInstance(SliceInstanceAddress sliceAddress);

        /// Adds entity to the current slice and takes ownership over it (you should not manage/delete it)
        void AddEntity(Entity* entity);

        /**
        * Find an entity in the slice by entity Id.
        */
        AZ::Entity* FindEntity(EntityId entityId);

        /*
         * Removes and/or deletes an entity from the current slice by pointer. We delete by default it because we own the entities.
         * \param entity entity pointer to be removed, since this entity should already exists this function will NOT instantiate,
         * as this should have already be done to have a valid pointer.
         * \param isDeleteEntity true by default as we own all entities, pass false to just remove the entity and get ownership of it.
         * \param isRemoveEmptyInstance if this entity is the last in an instance, it removes the instance (default)
         * \returns true if entity was removed, otherwise false
         */
        bool RemoveEntity(Entity* entity, bool isDeleteEntity = true, bool isRemoveEmptyInstance = true);

        /// Same as \ref RemoveEntity but by using entityId
        bool RemoveEntity(EntityId entityId, bool isDeleteEntity = true, bool isRemoveEmptyInstance = true);

        /**
        * Removes and deletes a meta data entity from the current slice
        * @param metaDataEntityId EntityId of the to be removed meta data entity
        * @return Returns true if the operation succeeds. Otherwise returns false and logs an error.
        */
        bool RemoveMetaDataEntity(EntityId metaDataEntityId);

        /**
        * A performant way to remove every entity from a SliceComponent. Operates in the same way as though you were to loop
        * through every entity in a SliceComponent and call RemoveEntity(entityId) on them, but using this method will be
        * much faster and is highly recommended. 
        *
        * \param deleteEntities true by default as we own all entities, pass false to just remove the entity and gain ownership of it
        * \param removeEmptyInstances true by default. When set to true, instances will be removed when the last of their entities is removed
        */
        void RemoveAllEntities(bool deleteEntities = true, bool removeEmptyInstances = true); 

        /*
        * Removes an entity not associated with a slice instance.
        * Developed for the specific needs of the Undo system. For all general use, call RemoveEntity instead.
        */
        void RemoveLooseEntity(EntityId entityId);

        /**
         * Returns the slice information about an entity, if it belongs to the component and is in a slice.
         * \param entity pointer to instantiated entity
         * \returns pair of SliceReference and SliceInstance if the entity is in a "child" slice, otherwise null,null. This is true even if entity
         * belongs to the slice (but it's not in a "child" slice)
         */
        SliceInstanceAddress FindSlice(Entity* entity);
        SliceInstanceAddress FindSlice(EntityId entityId);
        
        /**
        * Flattens a slice reference directly into the slice component and then removes the reference, all dependencies inherited from the reference will become directly owned by the slice component
        * \param toFlatten The reference we are flattening into the component
        * \param toFlattenRoot The root entity used to determine common ancestry among the entities within the reference
        * \return false if flattening failed
        */
        bool FlattenSlice(SliceReference* toFlatten, const EntityId& toFlattenRoot);

        /**
         * Extracts data required to restore the specified entity.
         * \return false if the entity is not part of an internal instance, and doesn't require any special restore operations.
         */
        bool GetEntityRestoreInfo(const AZ::EntityId entityId, EntityRestoreInfo& info);

        /**
         * Adds an entity back to an existing instance.
         * This create a reference for the specified asset if it doesn't already exist.
         * If the reference exists, but the instance with the specified id does not, one will be created.
         * Ownership of the entity is transferred to the instance.
         * @param entity A pointer to the entity to be restored.
         * @param restoreInfo An object holding various information for restoring the entity. Please see \ref EntityRestoreInfo.
         * @param isEntityAdd specifies if the entity we're restoring is part of an entity add. Skips checks for if the entity is in the SliceComponent.
         *        As the entity will not be a part of the SliceComponent until it is restored. Defaults to false.
         * @return A pair of SliceReference and SliceInstance, or null if the operation failed.
         */
        SliceInstanceAddress RestoreEntity(AZ::Entity* entity, const EntityRestoreInfo& restoreInfo, bool isEntityAdd = false);

        /**
         * Sets the asset that owns this component, which allows us to listen for asset changes.
         */
        void SetMyAsset(SliceAsset* asset)
        {
            m_myAsset = asset;
        }

        /**
        * Gets the asset that owns this component.
        */
        const SliceAsset* GetMyAsset() const
        {
            return m_myAsset;
        }

        /**
         * Return all data flags set for this entity.
         * Addresses are relative the entity.
         */
        const DataPatch::FlagsMap& GetEntityDataFlags(EntityId entityId) const;

        /**
         * Set all data flags for this entity.
         * Addresses should be relative the entity.
         * @return True if flags are set. False if the entity does not exist in this slice.
         */
        bool SetEntityDataFlags(EntityId entityId, const DataPatch::FlagsMap& dataFlags);

        /**
         * Get effect of data flags at a particular address within this entity.
         * Note that the "effect" of data flags could be impacted by data flags set at
         * other addresses, and data flags set in the slice that this entity is based on.
         */
        DataPatch::Flags GetEffectOfEntityDataFlagsAtAddress(EntityId, const DataPatch::AddressType& dataAddress) const;

        /**
         * Get the data flags set at a particular address within this entity.
         */
        DataPatch::Flags GetEntityDataFlagsAtAddress(EntityId entityId, const DataPatch::AddressType& dataAddress) const;

        /**
         * Set the data flags at a particular address within this entity.
         * @return True if flags are set. False if the entity does not exist in this slice.
         */
        bool SetEntityDataFlagsAtAddress(EntityId entityId, const DataPatch::AddressType& dataAddress, DataPatch::Flags flags);

        /** 
        * Appends the metadata entities belonging only directly to each instance in the slice to the given list. Metadata entities belonging
        * to any instances within each instance are omitted.
        */
        bool GetInstanceMetadataEntities(EntityList& outMetadataEntities);

        /**
        * Appends all metadata entities belonging to instances owned by this slice, including those in nested instances to the end of the given list.
        */
        bool GetAllInstanceMetadataEntities(EntityList& outMetadataEntities);

        /**
        * Gets all metadata entities belonging to this slice. This includes instance metadata components and the slice's own metadata component.
        * Because the contents of the result could come from multiple objects, the calling function must provide its own container.
        * Returns true if the operation succeeded (Even if the resulting container is empty)
        */
        bool GetAllMetadataEntities(EntityList& outMetadataEntities);

        /**
        * Gets the metadata entity belonging to this slice
        */
        AZ::Entity* GetMetadataEntity();

        typedef AZStd::unordered_set<Data::AssetId> AssetIdSet;

        /**
         * Gathers referenced slice assets for this slice (slice assets this contains, slice assets this depends on)
         * \param idSet the container that contains the referenced assets after this function is called
         * \param recurse whether to recurse. true by default so you get ALL referenced assets, false will return only directly-referenced assets
         */
        void GetReferencedSliceAssets(AssetIdSet& idSet, bool recurse = true);

        /** 
         * Clones the slice, its references, and its instances. Entity Ids are not regenerated
         * during this process. This utility is currently used to clone slice asset data
         * for data-push without modifying the existing asset in-memory.
         * \param serializeContext SerializeContext to use for cloning
         * \param sourceToCloneSliceInstanceMap [out] (optional) The map between source SliceInstances and cloned SliceInstances.
         * \return A clone of this SliceComponent
         */
        SliceComponent* Clone(AZ::SerializeContext& serializeContext, SliceInstanceToSliceInstanceMap* sourceToCloneSliceInstanceMap = nullptr) const;

        /**
         * Set whether the slice can be instantiated despite missing dependent slices (allowed by default).
         */
        void AllowPartialInstantiation(bool allow)
        {
            m_allowPartialInstantiation = allow;
        }

        /**
         * Returns whether or not the slice allows partial instantiation.
         */
        bool IsAllowPartialInstantiation() const
        {
            return m_allowPartialInstantiation;
        }

        /**
         * Returns whether or not this is a dynamic slice to be exported for runtime instantiation.
         */
        bool IsDynamic() const
        {
            return m_isDynamic;
        }

        /**
         * Designates this slice as dynamic (can be instantiated at runtime).
         */
        void SetIsDynamic(bool isDynamic)
        {
            m_isDynamic = isDynamic;
        }

        enum class InstantiateResult
        {
            Success,
            MissingDependency,
            CyclicalDependency
        };

        /**
        * Instantiate entities for this slice, otherwise only the data are stored.
        * \param serializeContext and relativeToAbsoluteSlicePaths are used for a specific case when asset processor is not available
        */
        InstantiateResult Instantiate(
            AZ::SerializeContext* serializeContext = nullptr,
            AZStd::unordered_map<AZStd::string, AZStd::string>* relativeToAbsoluteSlicePaths = nullptr);
        bool IsInstantiated() const;
        /**
         * Generate new entity Ids and remap references
         * \param previousToNewIdMap Output map of previous entityIds to newly generated entityIds
         */
        void GenerateNewEntityIds(EntityIdToEntityIdMap* previousToNewIdMap = nullptr);

        /**
        * Newly created slices and legacy slices won't have required metadata components. This will check to see if
        * necessary components to the function of the metadata entities are present and
        * \param instance Source slice instance
        */
        void InitMetadata();

        /**
         * Removes the passed in instances from this slice, and caches them to restore later.
         * Used by the layer system so layers can briefly take ownership of these instances.
         * This allows the level slice to save with only slice instances and references
         * unique to the level, and allows layers to save slices instances and references to themselves.
         * instancesToRemove The collection of instances that should be removed from this slice.
         */
        void RemoveAndCacheInstances(const SliceReferenceToInstancePtrs& instancesToRemove);

        /**
         * Restores slice instances that were cached when RemoveAndCacheInstances was called.
         */
        void RestoreCachedInstances();

        /// Returns data flags for use when instantiating an instance of this slice.
        /// These data flags include those harvested from the entire slice ancestry.
        const DataFlagsPerEntity& GetDataFlagsForInstances() const;

    protected:

        //////////////////////////////////////////////////////////////////////////
        // Component base
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // Asset Bus
        void OnAssetReloaded(Data::Asset<Data::AssetData> asset) override;
        //////////////////////////////////////////////////////////////////////////

        /// \ref ComponentDescriptor::GetProvidedServices
        static void GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided);
        /// \ref ComponentDescriptor::GetDependentServices
        static void GetDependentServices(ComponentDescriptor::DependencyArrayType& dependent);
        /// \ref ComponentDescriptor::Reflect
        static void Reflect(ReflectContext* reflection);

        /// Prepare component entity for save, if this is a leaf entity, data patch will be build.
        void PrepareSave();

        /// Populate the entity info map. This will re-populate it even if already populated.
        void BuildEntityInfoMap();

        /// Repopulate the entity info map if it was already populated.
        void RebuildEntityInfoMapIfNecessary();

        /// Return the appropriate DataFlagsPerEntity collection containing this entity's data flags.
        DataFlagsPerEntity* GetCorrectBundleOfDataFlags(EntityId entityId);
        const DataFlagsPerEntity* GetCorrectBundleOfDataFlags(EntityId entityId) const;

        void BuildDataFlagsForInstances();

        /**
        * During instance instantiation, entities from root slices may be removed by data patches. We need to remove these
        * from the metadata associations in our newly cloned instance metadata entities.
        */
        void CleanMetadataAssociations();
        
        /// Returns the entity info map (and builds it if necessary).
        const EntityInfoMap& GetEntityInfoMap() const;

        /// Returns the reference associated with the specified asset Id. If none is present, one will be created.
        SliceReference* AddOrGetSliceReference(const Data::Asset<SliceAsset>& sliceAsset);

        /// Removes a slice reference (and all instances) by iterator.
        SliceComponent::SliceList::iterator RemoveSliceReference(SliceComponent::SliceList::iterator sliceReferenceIt);

        /// Utility function to apply a EntityIdToEntityIdMap to a EntityIdToEntityIdMap (the mapping will override values in the destination)
        static void ApplyEntityMapId(EntityIdToEntityIdMap& destination, const EntityIdToEntityIdMap& mapping);

        /// Utility function to add an assetId to the cycle checker vector
        void PushInstantiateCycle(const AZ::Data::AssetId& assetId);

        /// Utility function to check if the given assetId would cause an instantiation cycle, and if so
        /// output the chain of slices that causes the cycle.
        bool CheckContainsInstantiateCycle(const AZ::Data::AssetId& assetId);
        
        /// Utility function to pop an assetId to the cycle checker vector (also checks to make sure its at the tail of it and clears it)
        void PopInstantiateCycle(const AZ::Data::AssetId& assetId);

        SliceAsset* m_myAsset;   ///< Pointer to the asset we belong to, note this is just a reference stored by the handler, we don't need Asset<SliceAsset> as it's not a reference to another asset.

        SerializeContext* m_serializeContext;

        EntityInfoMap m_entityInfoMap; ///< A cached mapping built for quick lookups between an EntityId and its owning SliceInstance.

        EntityInfoMap m_metaDataEntityInfoMap; ///< A cached mapping built for quick lookups between a MetaDataEntityId and its owning SliceInstance

        EntityList m_entities;  ///< Entities that are new (not based on a slice).

        SliceList m_slices; ///< List of base slices and their instances in the world
        SliceAssetToSliceInstances m_cachedSliceInstances; ///< Slice instances saved in layers on the root slice are cached here during the serialization process.
        SliceList m_cachedSliceReferences; ///< Slice references saved in layers on the root slice are cached here during the serialization process.
        SliceList m_invalidSlices; ///< List of slice references that did not load correctly.

        AZ::Entity m_metadataEntity; ///< Entity for attaching slice metadata components

        DataFlagsPerEntity m_dataFlagsForNewEntities; ///< DataFlags for new entities (DataFlags for entities based on a slice are stored within the SliceInstance) 

        DataFlagsPerEntity m_cachedDataFlagsForInstances; ///< Cached DataFlags to be used when instantiating instances of this slice.
        bool m_hasGeneratedCachedDataFlags; ///< Whether the cached DataFlags have been generated yet.

        AZStd::atomic<bool> m_slicesAreInstantiated; ///< Instantiate state of the base slices (they should be instantiated or not)

        bool m_allowPartialInstantiation; ///< Instantiation is still allowed even if dependencies are missing.

        bool m_isDynamic;   ///< Dynamic slices are available for instantiation at runtime.

        AZ::Data::AssetFilterCB m_assetLoadFilterCB; ///< Asset load filter callback to apply for internal loads during data patching.
        AZ::u32 m_filterFlags; ///< Asset load filter flags to apply for internal loads during data patching

        AZStd::recursive_mutex m_instantiateMutex; ///< Used to prevent multiple threads from trying to instantiate the slices at once
        
        typedef AZStd::vector<AZ::Data::AssetId> AssetIdVector;

        // note that this vector provides a "global" view of instantiation occurring and is only accessed while m_instantiateMutex is locked.
        // in addition, it only temporarily contains data - during instantiation it is the stack of assetIds that is in the current branch
        // of instantiation, and when we finish instantiation and return from the recursive call it is emptied and the memory is freed, so
        // there should be no risk of leaking it or having it survive beyond the allocator's existence.
        static AssetIdVector m_instantiateCycleChecker; ///< Used to prevent cyclic dependencies
    };

    /// @deprecated Use SliceComponent.
    using PrefabComponent = SliceComponent;

    namespace EntityUtils
    {
        // This class provides unrestricted access to the Entity's ID.
        class EntityIdAccessor final : public AZ::Entity
        {
        public:
            AZ_CLASS_ALLOCATOR(EntityIdAccessor, SystemAllocator)
            void ForceSetId(AZ::EntityId id)
            {
                m_id = id;
            }
        };

        // Optimized specialization for InstantiatedContainers.
        template<>
        inline unsigned int ReplaceEntityIds(SliceComponent::InstantiatedContainer* container, const EntityIdMapper& mapper, SerializeContext* /*context*/)
        {
            unsigned int replaced = 0;

            if (container)
            {
                // We know where EntityIds are defined in an InstantiatedContainer,
                // we don't need to use SerializeContext to search for them.
                for (SliceComponent::EntityList* entityList : { &container->m_entities, &container->m_metadataEntities })
                {
                    for (Entity* entity : *entityList)
                    {
                        EntityId newId = mapper(entity->GetId(), true);

                        // Note: entity->SetId(newId) can't be used on entities that have been initialized.
                        // We get around the safety checks in Entity::SetId() using the little hack below.
                        // This is needed because slices momentarily remap their entities' IDs during serialization
                        // (but immediately put them back the way there were).
                        // The "cleaner" way to do this would be to make temporary clones of the entities
                        // and perform remapping on the clones, but that would be much less performant.
                        static_cast<EntityIdAccessor*>(entity)->ForceSetId(newId);

                        ++replaced;
                    }
                }
            }

            return replaced;
        }

        // Deserialize a slice without using asset processor and read root entity from it
        AZ::Entity* LoadRootEntityFromSlicePath(const char* filePath, SerializeContext* context);
    } // namespace EntityUtils

    namespace IdUtils
    {
        // Optimized specialization for InstantiatedContainers.
        template<>
        template<>
        inline unsigned int Remapper<EntityId>::ReplaceIdsAndIdRefs(SliceComponent::InstantiatedContainer* container, const Remapper<EntityId>::IdMapper& mapper, SerializeContext* context)
        {
            unsigned int replaced = EntityUtils::ReplaceEntityIds(container,
                [&mapper](const EntityId& originalId, bool isEntityId) { return mapper(originalId, isEntityId, &Entity::MakeId);  },
                context);

            replaced += RemapIds(container, mapper, context, false);

            return replaced;
        }
    } // namespace IdUtils

} // namespace AZ


namespace AZStd
{
    template<>
    struct hash<AZ::SliceComponent::SliceInstanceAddress>
    {
        using argument_type = AZ::SliceComponent::SliceInstanceAddress;
        using result_type = size_t;

        result_type operator() (const argument_type& slice) const
        {
            size_t h = 0;
            hash_combine(h, slice.GetReference());
            hash_combine(h, slice.GetInstance());
            return h;
        }
    };
}
