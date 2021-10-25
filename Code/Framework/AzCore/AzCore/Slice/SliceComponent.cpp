/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Slice/SliceMetadataInfoComponent.h>
#include <AzCore/Slice/SliceBus.h>

#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/smart_ptr/make_shared.h>
///////////////////////////////////////////////////////
// Temp while the asset system is done
#include <AzCore/Serialization/Utils.h>
///////////////////////////////////////////////////////

namespace AZ
{
    namespace Converters
    {
        // SliceReference Version 1 -> 2
        // SliceReference::m_instances converted from AZStd::list<SliceInstance> to AZStd::unordered_set<SliceInstance>.
        bool SliceReferenceVersionConverter(SerializeContext& context, SerializeContext::DataElementNode& classElement)
        {
            if (classElement.GetVersion() < 2)
            {
                const int instancesIndex = classElement.FindElement(AZ_CRC("Instances", 0x7a270069));
                if (instancesIndex > -1)
                {
                    auto& instancesElement = classElement.GetSubElement(instancesIndex);

                    // Extract sub-elements, which we can just re-add under the set.
                    AZStd::vector<SerializeContext::DataElementNode> subElements;
                    subElements.reserve(instancesElement.GetNumSubElements());
                    for (int i = 0, n = instancesElement.GetNumSubElements(); i < n; ++i)
                    {
                        subElements.push_back(instancesElement.GetSubElement(i));
                    }

                    // Convert to unordered_set.
                    using SetType = AZStd::unordered_set<SliceComponent::SliceInstance>;
                    if (instancesElement.Convert<SetType>(context))
                    {
                        for (const SerializeContext::DataElementNode& subElement : subElements)
                        {
                            instancesElement.AddElement(subElement);
                        }
                    }

                    return true;
                }

                return false;
            }

            return true;
        }

    } // namespace Converters

    // storage for the static member for cyclic instantiation checking.
    // note that this vector is only used during slice instantiation and is set to capacity 0 when it empties.
    SliceComponent::AssetIdVector SliceComponent::m_instantiateCycleChecker; // dependency checker.
    // this is cleared and set to 0 capacity after each use.

    //////////////////////////////////////////////////////////////////////////
    // SliceInstanceAddress
    //////////////////////////////////////////////////////////////////////////
    SliceComponent::SliceInstanceAddress::SliceInstanceAddress()
        : first(m_reference)
        , second(m_instance)
    {}

    SliceComponent::SliceInstanceAddress::SliceInstanceAddress(SliceReference* reference, SliceInstance* instance)
        : m_reference(reference)
        , m_instance(instance)
        , first(m_reference)
        , second(m_instance)
    {}

    SliceComponent::SliceInstanceAddress::SliceInstanceAddress(const SliceComponent::SliceInstanceAddress& RHS)
        : m_reference(RHS.m_reference)
        , m_instance(RHS.m_instance)
        , first(m_reference)
        , second(m_instance)
    {}

    SliceComponent::SliceInstanceAddress::SliceInstanceAddress(const SliceComponent::SliceInstanceAddress&& RHS)
        : m_reference(AZStd::move(RHS.m_reference))
        , m_instance(AZStd::move(RHS.m_instance))
        , first(m_reference)
        , second(m_instance)
    {}

    bool SliceComponent::SliceInstanceAddress::operator==(const SliceComponent::SliceInstanceAddress& rhs) const
    {
        return m_reference == rhs.m_reference && m_instance == rhs.m_instance;
    }

    bool SliceComponent::SliceInstanceAddress::operator!=(const SliceComponent::SliceInstanceAddress& rhs) const { return m_reference != rhs.m_reference || m_instance != rhs.m_instance; }

    SliceComponent::SliceInstanceAddress& SliceComponent::SliceInstanceAddress::operator=(const SliceComponent::SliceInstanceAddress& RHS)
    {
        m_reference = RHS.m_reference;
        m_instance = RHS.m_instance;
        first = m_reference;
        second = m_instance;
        return *this;
    }

    bool SliceComponent::SliceInstanceAddress::IsValid() const
    {
        return m_reference && m_instance;
    }

    SliceComponent::SliceInstanceAddress::operator bool() const
    {
        return IsValid();
    }

    const SliceComponent::SliceReference* SliceComponent::SliceInstanceAddress::GetReference() const
    {
        return m_reference;
    }

    SliceComponent::SliceReference* SliceComponent::SliceInstanceAddress::GetReference()
    {
        return m_reference;
    }

    void SliceComponent::SliceInstanceAddress::SetReference(SliceComponent::SliceReference* reference)
    {
        m_reference = reference;
    }

    const SliceComponent::SliceInstance* SliceComponent::SliceInstanceAddress::GetInstance() const
    {
        return m_instance;
    }

    SliceComponent::SliceInstance* SliceComponent::SliceInstanceAddress::GetInstance()
    {
        return m_instance;
    }

    void SliceComponent::SliceInstanceAddress::SetInstance(SliceComponent::SliceInstance* instance)
    {
        m_instance = instance;
    }

    //=========================================================================
    void SliceComponent::DataFlagsPerEntity::Reflect(ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<DataFlagsPerEntity>()
                ->Version(1)
                ->Field("EntityToDataFlags", &DataFlagsPerEntity::m_entityToDataFlags)
                ;
        }
    }

    //=========================================================================
    SliceComponent::DataFlagsPerEntity::DataFlagsPerEntity(const IsValidEntityFunction& isValidEntityFunction)
        : m_isValidEntityFunction(isValidEntityFunction)
    {
    }

    //=========================================================================
    void SliceComponent::DataFlagsPerEntity::SetIsValidEntityFunction(const IsValidEntityFunction& isValidEntityFunction)
    {
        m_isValidEntityFunction = isValidEntityFunction;
    }

    //=========================================================================
    void SliceComponent::DataFlagsPerEntity::CopyDataFlagsFrom(const DataFlagsPerEntity& rhs)
    {
        m_entityToDataFlags = rhs.m_entityToDataFlags;
    }

    //=========================================================================
    void SliceComponent::DataFlagsPerEntity::CopyDataFlagsFrom(DataFlagsPerEntity&& rhs)
    {
        m_entityToDataFlags = AZStd::move(rhs.m_entityToDataFlags);
    }

    //=========================================================================
    void SliceComponent::DataFlagsPerEntity::ImportDataFlagsFrom(
        const DataFlagsPerEntity& from,
        const EntityIdToEntityIdMap* remapFromIdToId/*=nullptr*/,
        const DataFlagsTransformFunction& dataFlagsTransformFn/*=nullptr*/)
    {
        AZ_PROFILE_FUNCTION(AzCore);

        for (const auto& entityIdFlagsMapPair : from.m_entityToDataFlags)
        {
            const EntityId& fromEntityId = entityIdFlagsMapPair.first;
            const DataPatch::FlagsMap& fromFlagsMap = entityIdFlagsMapPair.second;

            // remap EntityId if necessary
            EntityId toEntityId = fromEntityId;
            if (remapFromIdToId)
            {
                auto foundRemap = remapFromIdToId->find(fromEntityId);
                if (foundRemap == remapFromIdToId->end())
                {
                    // leave out entities that don't occur in the map
                    continue;
                }
                toEntityId = foundRemap->second;
            }

            // merge masked values with existing flags
            for (const auto& addressFlagPair : fromFlagsMap)
            {
                DataPatch::Flags fromFlags = addressFlagPair.second;

                // apply transform function if necessary
                if (dataFlagsTransformFn)
                {
                    fromFlags = dataFlagsTransformFn(fromFlags);
                }

                if (fromFlags != 0) // don't create empty entries
                {
                    m_entityToDataFlags[toEntityId][addressFlagPair.first] |= fromFlags;
                }
            }
        }
    }

    //=========================================================================
    DataPatch::FlagsMap SliceComponent::DataFlagsPerEntity::GetDataFlagsForPatching(const EntityIdToEntityIdMap* remapFromIdToId /*=nullptr*/) const
    {
        AZ_PROFILE_FUNCTION(AzCore);

        // Collect together data flags from all entities
        DataPatch::FlagsMap dataFlagsForAllEntities;

        for (auto& entityIdDataFlagsPair : m_entityToDataFlags)
        {
            const AZ::EntityId& fromEntityId = entityIdDataFlagsPair.first;
            const DataPatch::FlagsMap& entityDataFlags = entityIdDataFlagsPair.second;

            AZ::EntityId toEntityId = fromEntityId;
            if (remapFromIdToId)
            {
                auto foundRemap = remapFromIdToId->find(fromEntityId);
                if (foundRemap == remapFromIdToId->end())
                {
                    // leave out entities that don't occur in the map
                    continue;
                }
                toEntityId = foundRemap->second;
            }

            // Make the addressing relative to InstantiatedContainer (entityDataFlags are relative to the individual entity)
            DataPatch::AddressType addressPrefix;
            addressPrefix.push_back(AZ_CRC("Entities", 0x50ec64e5));
            addressPrefix.push_back(static_cast<u64>(toEntityId));

            for (auto& addressFlagsPair : entityDataFlags)
            {
                const DataPatch::AddressType& originalAddress = addressFlagsPair.first;

                DataPatch::AddressType prefixedAddress;
                prefixedAddress.reserve(addressPrefix.size() + originalAddress.size());
                prefixedAddress.insert(prefixedAddress.end(), addressPrefix.begin(), addressPrefix.end());
                prefixedAddress.insert(prefixedAddress.end(), originalAddress.begin(), originalAddress.end());

                dataFlagsForAllEntities.emplace(AZStd::move(prefixedAddress), addressFlagsPair.second);
            }
        }

        return dataFlagsForAllEntities;
    }

    //=========================================================================
    void SliceComponent::DataFlagsPerEntity::RemapEntityIds(const EntityIdToEntityIdMap& remapFromIdToId)
    {
        // move all data flags to this map, using remapped EntityIds
        AZStd::unordered_map<EntityId, DataPatch::FlagsMap> remappedEntityToDataFlags;

        for (auto& entityToDataFlagsPair : m_entityToDataFlags)
        {
            auto foundRemappedId = remapFromIdToId.find(entityToDataFlagsPair.first);
            if (foundRemappedId != remapFromIdToId.end())
            {
                remappedEntityToDataFlags[foundRemappedId->second] = AZStd::move(entityToDataFlagsPair.second);
            }
        }

        // replace current data with remapped data
        m_entityToDataFlags = AZStd::move(remappedEntityToDataFlags);
    }

    //=========================================================================
    const DataPatch::FlagsMap& SliceComponent::DataFlagsPerEntity::GetEntityDataFlags(EntityId entityId) const
    {
        auto foundDataFlags = m_entityToDataFlags.find(entityId);
        if (foundDataFlags != m_entityToDataFlags.end())
        {
            return foundDataFlags->second;
        }

        static const DataPatch::FlagsMap emptyFlagsMap;
        return emptyFlagsMap;
    }

    //=========================================================================
    bool SliceComponent::DataFlagsPerEntity::SetEntityDataFlags(EntityId entityId, const DataPatch::FlagsMap& dataFlags)
    {
        if (IsValidEntity(entityId))
        {
            if (!dataFlags.empty())
            {
                m_entityToDataFlags[entityId] = dataFlags;
            }
            else
            {
                // If entity has no data flags, erase the entity's map entry.
                m_entityToDataFlags.erase(entityId);
            }
            return true;
        }
        return false;
    }

    //=========================================================================
    void SliceComponent::DataFlagsPerEntity::SetEntityDataFlagsForUndo(EntityId entityId, const DataPatch::FlagsMap& dataFlags)
    {
        if (!dataFlags.empty())
        {
            m_entityToDataFlags[entityId] = dataFlags;
        }
        else
        {
            // If entity has no data flags, erase the entity's map entry.
            m_entityToDataFlags.erase(entityId);
        }
    }

    //=========================================================================
    bool SliceComponent::DataFlagsPerEntity::ClearEntityDataFlags(EntityId entityId)
    {
        if (IsValidEntity(entityId))
        {
            m_entityToDataFlags.erase(entityId);
            return true;
        }
        return false;
    }

    //=========================================================================
    DataPatch::Flags SliceComponent::DataFlagsPerEntity::GetEntityDataFlagsAtAddress(EntityId entityId, const DataPatch::AddressType& dataAddress) const
    {
        auto foundEntityDataFlags = m_entityToDataFlags.find(entityId);
        if (foundEntityDataFlags != m_entityToDataFlags.end())
        {
            const DataPatch::FlagsMap& entityDataFlags = foundEntityDataFlags->second;
            auto foundDataFlags = entityDataFlags.find(dataAddress);
            if (foundDataFlags != entityDataFlags.end())
            {
                return foundDataFlags->second;
            }
        }

        return 0;
    }

    //=========================================================================
    bool SliceComponent::DataFlagsPerEntity::SetEntityDataFlagsAtAddress(EntityId entityId, const DataPatch::AddressType& dataAddress, DataPatch::Flags flags)
    {
        if (IsValidEntity(entityId))
        {
            if (flags != 0)
            {
                m_entityToDataFlags[entityId][dataAddress] = flags;
            }
            else
            {
                // If clearing the flags, erase the data address's map entry.
                // If the entity no longer has any data flags, erase the entity's map entry.
                auto foundEntityDataFlags = m_entityToDataFlags.find(entityId);
                if (foundEntityDataFlags != m_entityToDataFlags.end())
                {
                    DataPatch::FlagsMap& entityDataFlags = foundEntityDataFlags->second;
                    entityDataFlags.erase(dataAddress);
                    if (entityDataFlags.empty())
                    {
                        m_entityToDataFlags.erase(foundEntityDataFlags);
                    }
                }
            }

            return true;
        }
        return false;
    }

    //=========================================================================
    bool SliceComponent::DataFlagsPerEntity::IsValidEntity(EntityId entityId) const
    {
        if (m_isValidEntityFunction)
        {
            if (m_isValidEntityFunction(entityId))
            {
                return true;
            }
            return false;
        }
        return true; // if no validity function is set, always return true
    }

    //=========================================================================
    void SliceComponent::DataFlagsPerEntity::Cleanup(const EntityList& validEntities)
    {
        AZ_PROFILE_FUNCTION(AzCore);

        EntityIdSet validEntityIds;
        for (const Entity* entity : validEntities)
        {
            validEntityIds.insert(entity->GetId());
        }

        for (auto entityDataFlagIterator = m_entityToDataFlags.begin(); entityDataFlagIterator != m_entityToDataFlags.end(); )
        {
            EntityId entityId = entityDataFlagIterator->first;
            if (validEntityIds.find(entityId) != validEntityIds.end())
            {
                // TODO LY-52686: Prune flags if their address doesn't line up with anything in this entity.

                ++entityDataFlagIterator;
            }
            else
            {
                entityDataFlagIterator = m_entityToDataFlags.erase(entityDataFlagIterator);
            }
        }
    }

    SliceComponent::InstantiatedContainer::InstantiatedContainer(bool deleteEntitiesOnDestruction)
        : m_deleteEntitiesOnDestruction(deleteEntitiesOnDestruction)
    {
    }

    SliceComponent::InstantiatedContainer::InstantiatedContainer(
        SliceComponent* sourceComponent,
        bool deleteEntitiesOnDestruction)
        : m_deleteEntitiesOnDestruction(deleteEntitiesOnDestruction)
    {
        if (!sourceComponent)
        {
            return;
        }
        sourceComponent->GetEntities(m_entities);
        sourceComponent->GetAllMetadataEntities(m_metadataEntities);
    }

    //=========================================================================
    // SliceComponent::InstantiatedContainer::~InstanceContainer
    //=========================================================================
    SliceComponent::InstantiatedContainer::~InstantiatedContainer()
    {
        if (m_deleteEntitiesOnDestruction)
        {
            DeleteEntities();
        }
    }

    //=========================================================================
    // SliceComponent::InstantiatedContainer::DeleteEntities
    //=========================================================================
    void SliceComponent::InstantiatedContainer::DeleteEntities()
    {
        for (Entity* entity : m_entities)
        {
            delete entity;
        }
        m_entities.clear();

        for (Entity* metaEntity : m_metadataEntities)
        {
            if (metaEntity->GetState() == AZ::Entity::State::Active)
            {
                SliceInstanceNotificationBus::Broadcast(&SliceInstanceNotificationBus::Events::OnMetadataEntityDestroyed, metaEntity->GetId());
            }

            delete metaEntity;
        }
        m_metadataEntities.clear();
    }

    //=========================================================================
    // SliceComponent::InstantiatedContainer::ClearAndReleaseOwnership
    //=========================================================================
    void SliceComponent::InstantiatedContainer::ClearAndReleaseOwnership()
    {
        m_entities.clear();
        m_metadataEntities.clear();
    }

    //=========================================================================
    // SliceComponent::SliceInstance::SliceInstance
    //=========================================================================
    SliceComponent::SliceInstance::SliceInstance(const SliceInstanceId& id)
        : m_instantiated(nullptr)
        , m_instanceId(id)
        , m_metadataEntity(nullptr)
    {
    }

    //=========================================================================
    // SliceComponent::SliceInstance::SliceInstance
    //=========================================================================
    SliceComponent::SliceInstance::SliceInstance(SliceInstance&& rhs)
    {
        m_instantiated = rhs.m_instantiated;
        rhs.m_instantiated = nullptr;
        m_baseToNewEntityIdMap = AZStd::move(rhs.m_baseToNewEntityIdMap);
        m_entityIdToBaseCache = AZStd::move(rhs.m_entityIdToBaseCache);
        m_dataPatch = AZStd::move(rhs.m_dataPatch);
        m_dataFlags.CopyDataFlagsFrom(AZStd::move(rhs.m_dataFlags));
        m_instanceId = rhs.m_instanceId;
        rhs.m_instanceId = SliceInstanceId::CreateNull();
        m_metadataEntity = AZStd::move(rhs.m_metadataEntity);
    }

    //=========================================================================
    // SliceComponent::SliceInstance::SliceInstance
    //=========================================================================
    SliceComponent::SliceInstance::SliceInstance(const SliceInstance& rhs)
    {
        operator=(rhs);
        m_instantiated = rhs.m_instantiated;
        m_baseToNewEntityIdMap = rhs.m_baseToNewEntityIdMap;
        m_entityIdToBaseCache = rhs.m_entityIdToBaseCache;
        m_dataPatch = rhs.m_dataPatch;
        m_dataFlags.CopyDataFlagsFrom(rhs.m_dataFlags);
        m_instanceId = rhs.m_instanceId;
        m_metadataEntity = rhs.m_metadataEntity;
    }

    //=========================================================================
    // SliceComponent::SliceInstance::operator=
    //=========================================================================
    SliceComponent::SliceInstance& SliceComponent::SliceInstance::operator=(const SliceInstance& rhs)
    {
        m_instantiated = rhs.m_instantiated;
        m_baseToNewEntityIdMap = rhs.m_baseToNewEntityIdMap;
        m_entityIdToBaseCache = rhs.m_entityIdToBaseCache;
        m_dataPatch = rhs.m_dataPatch;
        m_dataFlags.CopyDataFlagsFrom(rhs.m_dataFlags);
        m_instanceId = rhs.m_instanceId;
        m_metadataEntity = rhs.m_metadataEntity;
        return *this;
    }

    //=========================================================================
    // SliceComponent::SliceInstance::BuildReverseLookUp
    //=========================================================================
    void SliceComponent::SliceInstance::BuildReverseLookUp() const
    {
        m_entityIdToBaseCache.clear();
        for (const auto& entityIdPair : m_baseToNewEntityIdMap)
        {
            m_entityIdToBaseCache.insert(AZStd::make_pair(entityIdPair.second, entityIdPair.first));
        }
    }

    //=========================================================================
    // SliceComponent::SliceInstance::~SliceInstance
    //=========================================================================
    SliceComponent::SliceInstance::~SliceInstance()
    {
        delete m_instantiated;
    }

    //=========================================================================
    // SliceComponent::SliceInstance::GenerateValidEntityFunction
    //=========================================================================
    SliceComponent::DataFlagsPerEntity::IsValidEntityFunction SliceComponent::SliceInstance::GenerateValidEntityFunction(const SliceInstance* instance)
    {
        auto isValidEntityFunction = [instance](EntityId entityId)
        {
            const EntityIdToEntityIdMap& entityIdToBaseMap = instance->GetEntityIdToBaseMap();
            return entityIdToBaseMap.find(entityId) != entityIdToBaseMap.end();
        };
        return isValidEntityFunction;
    }

    //=========================================================================
    // SliceComponent::SliceInstance::GetDataFlagsForPatching
    //=========================================================================
    DataPatch::FlagsMap SliceComponent::SliceInstance::GetDataFlagsForPatching() const
    {
        // Collect all entities' data flags
        DataPatch::FlagsMap dataFlags;

        for (auto& baseIdInstanceIdPair : GetEntityIdMap())
        {
            // Make the addressing relative to InstantiatedContainer (m_dataFlags stores flags relative to the individual entity)
            DataPatch::AddressType addressPrefix;
            addressPrefix.push_back(AZ_CRC("Entities", 0x50ec64e5));
            addressPrefix.push_back(static_cast<u64>(baseIdInstanceIdPair.first));

            for (auto& addressDataFlagsPair : m_dataFlags.GetEntityDataFlags(baseIdInstanceIdPair.second))
            {
                const DataPatch::AddressType& originalAddress = addressDataFlagsPair.first;

                DataPatch::AddressType prefixedAddress;
                prefixedAddress.reserve(addressPrefix.size() + originalAddress.size());
                prefixedAddress.insert(prefixedAddress.end(), addressPrefix.begin(), addressPrefix.end());
                prefixedAddress.insert(prefixedAddress.end(), originalAddress.begin(), originalAddress.end());

                dataFlags.emplace(AZStd::move(prefixedAddress), addressDataFlagsPair.second);
            }
        }

        return dataFlags;
    }

    //=========================================================================
    // SliceComponent::SliceInstance::IsValidEntity
    //=========================================================================
    bool SliceComponent::SliceInstance::IsValidEntity(EntityId entityId) const
    {
        const EntityIdToEntityIdMap& entityIdToBaseMap = GetEntityIdToBaseMap();
        return entityIdToBaseMap.find(entityId) != entityIdToBaseMap.end();
    }

    //=========================================================================
    // Returns the metadata entity for this instance.
    //=========================================================================
    Entity* SliceComponent::SliceInstance::GetMetadataEntity() const
    {
        return m_metadataEntity;
    }

    //=========================================================================
    // SliceComponent::SliceReference::SliceReference
    //=========================================================================
    SliceComponent::SliceReference::SliceReference()
        : m_isInstantiated(false)
        , m_component(nullptr)
    {
    }
    
    //=========================================================================
    // SliceComponent::SliceReference::CreateEmptyInstance
    //=========================================================================
    SliceComponent::SliceInstance* SliceComponent::SliceReference::CreateEmptyInstance(const SliceInstanceId& instanceId)
    {
        auto emplaceResult = m_instances.emplace(instanceId);

        // An instance by this Id is already registered
        if (!emplaceResult.second)
        {
            AZ_Assert(false, "SliceComponent::SliceReference::CreateEmptyInstance: Attempted to CreateEmptyInstance using an already registered SliceInstanceId. Returning a null SliceInstance");
            return nullptr;
        }

        SliceInstance* instance = &(*emplaceResult.first);
        return instance;
    }

    //=========================================================================
    // SliceComponent::SliceReference::PrepareCreateInstance
    //=========================================================================
    SliceComponent::SliceInstance* SliceComponent::SliceReference::PrepareCreateInstance(const SliceInstanceId& sliceInstanceId, bool allowUninstantiated)
    {
        AZ_PROFILE_FUNCTION(AzCore);

        // create an empty instance (just copy of the existing data)
        SliceInstance* instance = CreateEmptyInstance(sliceInstanceId);

        // CreateEmptyInstance failed to generate a new instance.
        // Likely due to the supplied sliceInstanceId already being in use.
        // An assert was thrown by CreateEmptyInstance. No need to throw another.
        if (!instance)
        {
            return nullptr;
        }

        // We will not proceed if uninstantiated.
        // But if we allowUninstantiated then caller is opting to handle this case.
        // Return the empty instance.
        if (!m_isInstantiated && allowUninstantiated)
        {
            return instance;
        }
        else if (!m_isInstantiated)
        {
            AZ_Error("SliceComponent::SliceReference::PrepareCreateInstance",
                false,
                "Unable to create new slice instance due to caller of PrepareCreateInstance requiring owning slice reference to be instantiated");

            return nullptr;
        }

        if (!m_component)
        {
            AZ_Error("SliceComponent::SliceReference::PrepareCreateInstance",
                false,
                "SliceComponent is invalid. Unable to complete Slice Instance Creation");

            return nullptr;
        }

        // Ensure our asset is ready and we can access its component
        if (!m_asset.IsReady() || !m_asset.Get()->GetComponent())
        {
            AZ_Error("SliceComponent::SliceReference::PrepareCreateInstance",
                false,
                "If we are in an instantiated state, dependent asset should be ready. Unable to complete Slice Instance Creation");

            return nullptr;
        }

        return instance;
    }

    //=========================================================================
    // SliceComponent::SliceReference::FinalizeCreateInstance
    //=========================================================================
    SliceComponent::SliceInstance* SliceComponent::SliceReference::FinalizeCreateInstance(SliceInstance& instance,
        void* remapContainer,
        const AZ::Uuid& classUuid,
        AZ::SerializeContext* serializeContext,
        const AZ::IdUtils::Remapper<AZ::EntityId>::IdMapper& customMapper)
    {
        AZ_PROFILE_FUNCTION(AzCore);

        if (!remapContainer)
        {
            AZ_Error("SliceComponent::SliceReference::FinalizeCreateInstance",
                false,
                "Invalid Remap Container provided. Unable to proceed with finalizing the new Slice Instance");

            RemoveInstance(&instance);
            return nullptr;
        }

        if (!serializeContext)
        {
            AZ_Error("SliceComponent::SliceReference::FinalizeCreateInstance",
                false,
                "Invalid SerializeContext provided. Unable to proceed with finalizing the new Slice Instance");

            RemoveInstance(&instance);
            return nullptr;
        }

        if (!instance.m_instantiated || instance.m_instantiated->m_metadataEntities.empty())
        {
            AZ_Error("SliceComponent::SliceReference::FinalizeCreateInstance",
                false,
                "Metadata Entities must exist at slice instantiation time. Unable to proceed with finalizing the new Slice Instance");

            RemoveInstance(&instance);
            return nullptr;
        }

        AZ::IdUtils::Remapper<EntityId>::ReplaceIdsAndIdRefs(remapContainer, classUuid,
            [&](const EntityId& originalId, bool isEntityId, const AZStd::function<EntityId()>& idGenerator) -> EntityId
            {
                if (isEntityId) // replace EntityId
                {
                    EntityId newId = customMapper ? customMapper(originalId, isEntityId, idGenerator) : idGenerator();
                    auto insertIt = instance.m_baseToNewEntityIdMap.emplace(AZStd::make_pair(originalId, newId));
                    return insertIt.first->second;
                }
                else // replace EntityRef
                {
                    auto findIt = instance.m_baseToNewEntityIdMap.find(originalId);
                    if (findIt == instance.m_baseToNewEntityIdMap.end())
                    {
                        return originalId; // Referenced EntityId is not part of the slice, so keep the same id reference.
                    }
                    else
                    {
                        return findIt->second; // return the remapped id
                    }
                }
            }, serializeContext);

        if (!m_component->m_entityInfoMap.empty())
        {
            AddInstanceToEntityInfoMap(instance);
        }

        FixUpMetadataEntityForSliceInstance(&instance);

        return &instance;
    }

    //=========================================================================
    // SliceComponent::SliceReference::CreateInstance
    //=========================================================================
    SliceComponent::SliceInstance* SliceComponent::SliceReference::CreateInstance(const AZ::IdUtils::Remapper<AZ::EntityId>::IdMapper& customMapper, 
        SliceInstanceId sliceInstanceId)
    {
        AZ_PROFILE_FUNCTION(AzCore);

        // Validate that we are able to create an instance at this time
        // If we are instantiated then this includes verifying that we have a valid component and asset
        SliceInstance* instance = PrepareCreateInstance(sliceInstanceId, true);

        // If we are uninstantiated this empty instance is prepped to be instantiated later
        // If PrepareCreateInstance failed we want to return a nullptr.
        // In the latter case an error message has already been produced.
        if (!m_isInstantiated || !instance)
        {
            return instance;
        }

        SliceComponent* dependentSlice = m_asset.Get()->GetComponent();

        // Now we can build a temporary structure of all the entities we're going to clone
        InstantiatedContainer sourceObjects(false);
        dependentSlice->GetEntities(sourceObjects.m_entities);
        dependentSlice->GetAllMetadataEntities(sourceObjects.m_metadataEntities);

        instance->m_instantiated = dependentSlice->GetSerializeContext()->CloneObject(&sourceObjects);

        return FinalizeCreateInstance(*instance,
            instance->m_instantiated,
            azrtti_typeid(*instance->m_instantiated),
            dependentSlice->GetSerializeContext(),
            customMapper);
    }

    SliceComponent::SliceInstance* SliceComponent::SliceReference::CreateInstanceFromExistingEntities(AZStd::vector<AZ::Entity*>& entities,
        const EntityIdToEntityIdMap assetToLiveIdMap,
        SliceInstanceId sliceInstanceId)
    {
        AZ_PROFILE_FUNCTION(AzCore);

        // Validate that we are able to create an instance at this time
        // This includes verifying that we are instantiated, and have a valid component and asset
        // Instantiation is a requirement for creating an instance from existing entities
        SliceInstance* instance = PrepareCreateInstance(sliceInstanceId, false);

        // PrepareCreateInstance has failed
        // Return a null instance as error
        // PrepareCreateInstance has already logged an error message
        if (!instance)
        {
            return instance;
        }

        SliceComponent* dependentSlice = m_asset.Get()->GetComponent();

        // Acquire the asset's list of metadata entities
        EntityList assetMetaDataEntities;
        dependentSlice->GetAllMetadataEntities(assetMetaDataEntities);

        // Allocate the instance's Instantiated Container and clone the asset's metadata entities into it
        instance->m_instantiated = aznew InstantiatedContainer();
        m_component->GetSerializeContext()->CloneObjectInplace(instance->m_instantiated->m_metadataEntities, &assetMetaDataEntities);

        // Store the entities list and asset to live mapping into the instance
        instance->m_instantiated->m_entities = entities;
        instance->m_baseToNewEntityIdMap = assetToLiveIdMap;

        return FinalizeCreateInstance(*instance,
                &instance->m_instantiated->m_metadataEntities,
                azrtti_typeid(instance->m_instantiated->m_metadataEntities),
                dependentSlice->GetSerializeContext());
    }

    //=========================================================================
    // SliceComponent::SliceReference::CloneInstance
    //=========================================================================
    SliceComponent::SliceInstance* SliceComponent::SliceReference::CloneInstance(SliceComponent::SliceInstance* instance, 
                                                                                 SliceComponent::EntityIdToEntityIdMap& sourceToCloneEntityIdMap)
    {
        AZ_PROFILE_FUNCTION(AzCore);

        // check if source instance belongs to this slice reference
        auto findIt = AZStd::find_if(m_instances.begin(), m_instances.end(), [instance](const SliceInstance& element) -> bool { return &element == instance; });
        if (findIt == m_instances.end())
        {
            AZ_Error("Slice", false, "SliceInstance %p doesn't belong to this SliceReference %p!", instance, this);
            return nullptr;
        }

        SliceInstance* newInstance = CreateEmptyInstance();

        if (m_isInstantiated)
        {
            SerializeContext* serializeContext = m_asset.Get()->GetComponent()->GetSerializeContext();

            // clone the entities
            newInstance->m_instantiated = IdUtils::Remapper<EntityId>::CloneObjectAndGenerateNewIdsAndFixRefs(instance->m_instantiated, sourceToCloneEntityIdMap, serializeContext);

            const EntityIdToEntityIdMap& instanceToBaseSliceEntityIdMap = instance->GetEntityIdToBaseMap();
            for (AZStd::pair<AZ::EntityId, AZ::EntityId>& sourceIdToCloneId : sourceToCloneEntityIdMap)
            {
                EntityId sourceId = sourceIdToCloneId.first;
                EntityId cloneId = sourceIdToCloneId.second;

                auto instanceIdToBaseId = instanceToBaseSliceEntityIdMap.find(sourceId);
                if (instanceIdToBaseId == instanceToBaseSliceEntityIdMap.end())
                {
                    AZ_Assert(false, "An entity cloned (id: %s) couldn't be found in the source slice instance!", sourceId.ToString().c_str());
                }
                EntityId baseId = instanceIdToBaseId->second;

                newInstance->m_baseToNewEntityIdMap.insert(AZStd::make_pair(baseId, cloneId));
                newInstance->m_entityIdToBaseCache.insert(AZStd::make_pair(cloneId, baseId));

                newInstance->m_dataFlags.SetEntityDataFlags(cloneId, instance->m_dataFlags.GetEntityDataFlags(sourceId));
            }

            if (!m_component->m_entityInfoMap.empty())
            {
                AddInstanceToEntityInfoMap(*newInstance);
            }
        }
        else
        {
            // clone data patch
            AZ_Assert(false, "todo regenerate the entity map id and copy data flags");
            newInstance->m_dataPatch = instance->m_dataPatch;
        }

        return newInstance;
    }

    void SliceComponent::SliceReference::RestoreAndClearCachedInstance(SliceInstance& cachedInstance)
    {
        auto instance = m_instances.emplace(cachedInstance);
        cachedInstance.ClearInstantiated();
        AddInstanceToEntityInfoMap(*instance.first);
        instance.first->BuildReverseLookUp();
    }

    //=========================================================================
    // SliceComponent::SliceReference::FindInstance
    //=========================================================================
    SliceComponent::SliceInstance* SliceComponent::SliceReference::FindInstance(const SliceInstanceId& instanceId)
    {
        auto iter = m_instances.find_as(instanceId, AZStd::hash<SliceInstanceId>(), 
            [](const SliceInstanceId& id, const SliceInstance& instance)
            {
                return (id == instance.GetId());
            }
        );

        if (iter != m_instances.end())
        {
            return &(*iter);
        }

        return nullptr;
    }

    //=========================================================================
    // SliceComponent::SliceReference::RemoveInstance
    //=========================================================================
    SliceComponent::SliceReference::SliceInstances::iterator SliceComponent::SliceReference::RemoveInstance(SliceComponent::SliceReference::SliceInstances::iterator itr)
    {
        RemoveInstanceFromEntityInfoMap(*itr);
        return m_instances.erase(itr);
    }

    //=========================================================================
    // SliceComponent::SliceReference::RemoveInstance
    //=========================================================================
    bool SliceComponent::SliceReference::RemoveInstance(SliceComponent::SliceInstance* instance)
    {
        AZ_Assert(instance, "Invalid instance provided to SliceReference::RemoveInstance");
        auto iter = m_instances.find(*instance);
        if (iter != m_instances.end())
        {
            RemoveInstance(iter);
            return true;
        }

        return false;
    }

    //=========================================================================
    // SliceComponent::SliceReference::RemoveEntity
    //=========================================================================
    bool SliceComponent::SliceReference::RemoveEntity(EntityId entityId, bool isDeleteEntity, SliceInstance* instance)
    {
        if (!instance)
        {
            instance = m_component->FindSlice(entityId).GetInstance();

            if (!instance)
            {
                return false; // Can't find an instance that owns this entity
            }
        }

        auto entityIt = AZStd::find_if(instance->m_instantiated->m_entities.begin(), instance->m_instantiated->m_entities.end(), [entityId](Entity* entity) -> bool { return entity->GetId() == entityId; });
        if (entityIt != instance->m_instantiated->m_entities.end())
        {
            // RemoveEntity will be invoked again recursively if invoked with isDeleteEntity = true as the deleting the found entity
            // would cause the Entity destructor to send a ComponentApplicationRequests::RemoveEntity event causing the ComponentApplication
            // to send an ComponentApplicationEvents::OnEntityRemoved event that is handled by the EntityContext::OnEntityRemoved function which finally
            // calls this function again with isDeleteEntity = false. Therefore the EntityInfoMap and the container of Entities needs to have the 
            // current entity removed from it before deleting it
            AZStd::unique_ptr<AZ::Entity> entityToDelete(isDeleteEntity ? *entityIt : nullptr);
            instance->m_instantiated->m_entities.erase(entityIt);

            instance->m_dataFlags.ClearEntityDataFlags(entityId);

            instance->BuildReverseLookUp();
            auto entityToBaseIt = instance->m_entityIdToBaseCache.find(entityId);
            bool entityToBaseExists = entityToBaseIt != instance->m_entityIdToBaseCache.end();
            // Reverse lookup was built from base cache, if entity is not found in one that means it is also not in the other
            if (entityToBaseExists)
            {
                instance->m_baseToNewEntityIdMap.erase(entityToBaseIt->second);
                instance->m_entityIdToBaseCache.erase(entityToBaseIt);
            }
            return true;
        }

        return false;
    }

    //=========================================================================
    // SliceComponent::SliceReference::GetInstances
    //=========================================================================
    const SliceComponent::SliceReference::SliceInstances& SliceComponent::SliceReference::GetInstances() const
    {
        return m_instances;
    }

    //=========================================================================
    // SliceComponent::SliceReference::IsInstantiated
    //=========================================================================
    bool SliceComponent::SliceReference::IsInstantiated() const
    {
        return m_isInstantiated;
    }

    //=========================================================================
    // SliceComponent::SliceReference::Instantiate
    //=========================================================================
    bool SliceComponent::SliceReference::Instantiate(const AZ::ObjectStream::FilterDescriptor& filterDesc)
    {
        AZ_PROFILE_FUNCTION(AzCore);

        if (m_isInstantiated)
        {
            return true;
        }

        AZ::Data::AssetInfo assetInfo;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, m_asset.GetId());
        bool isAssetStillOnDisk = assetInfo.m_assetId.IsValid();

        bool isCachedAssetReady = m_asset.IsReady();

        if (!(isCachedAssetReady && isAssetStillOnDisk))
        {
            // If the asset has been queued for async loading but hasn't completed, we've reached the point where it is required
            // to be complete, so block until it finishes loading.
            if (m_asset.IsLoading())
            {
                m_asset.BlockUntilLoadComplete();
            }

            // If the asset still isn't ready, an unexpected error has occurred.
            if (!m_asset.IsReady())
            {
#if defined(AZ_ENABLE_TRACING)
                const Data::Asset<SliceAsset> owningAsset = m_component->m_myAsset ?
                    Data::Asset<SliceAsset>(Data::AssetManager::Instance().FindAsset(m_component->m_myAsset->GetId(), AZ::Data::AssetLoadBehavior::Default)) :
                    Data::Asset<SliceAsset>();
                AZ_Error("Slice", false,
                    "Instantiation of %d slice instance(s) of asset %s failed - asset not ready or not found during instantiation of owning slice %s!"
                    "Saving owning slice will lose data for these instances.",
                    m_instances.size(),
                    m_asset.ToString<AZStd::string>().c_str(),
                    m_component->m_myAsset ? owningAsset.ToString<AZStd::string>().c_str() : "[Could not find owning slice]");
#endif // AZ_ENABLE_TRACING
                return false;
            }
        }

        SliceComponent* dependentSlice = m_asset.Get()->GetComponent();
        InstantiateResult instantiationResult = dependentSlice->Instantiate();
        if (instantiationResult != InstantiateResult::Success)
        {
            #if defined(AZ_ENABLE_TRACING)
            Data::Asset<SliceAsset> owningAsset = m_component->m_myAsset ?
                Data::Asset<SliceAsset>(Data::AssetManager::Instance().FindAsset(m_component->m_myAsset->GetId(), AZ::Data::AssetLoadBehavior::Default)) :
                Data::Asset<SliceAsset>();
            AZ_Error("Slice", false, "Instantiation of %d slice instance(s) of asset %s failed - asset ready and available, but unable to instantiate "
                "for owning slice %s! Saving owning slice will lose data for these instances.",
                m_instances.size(),
                m_asset.ToString<AZStd::string>().c_str(),
                m_component->m_myAsset ? owningAsset.ToString<AZStd::string>().c_str() : "[Could not find owning slice]");
            #endif // AZ_ENABLE_TRACING
            // Cannot partially instantiate when a cyclical dependency exists.
            if (instantiationResult == InstantiateResult::CyclicalDependency)
            {
                return false;
            }
        }

        m_isInstantiated = true;

        for (SliceInstance& instance : m_instances)
        {
            InstantiateInstance(instance, filterDesc);
        }
        return true;
    }

    //=========================================================================
    // SliceComponent::SliceReference::UnInstantiate
    //=========================================================================
    void SliceComponent::SliceReference::UnInstantiate()
    {
        if (m_isInstantiated)
        {
            m_isInstantiated = false;

            for (SliceInstance& instance : m_instances)
            {
                delete instance.m_instantiated;
                instance.m_instantiated = nullptr;
            }
        }
    }

    //=========================================================================
    // SliceComponent::SliceReference::InstantiateInstance
    //=========================================================================
    void SliceComponent::SliceReference::InstantiateInstance(SliceInstance& instance, const AZ::ObjectStream::FilterDescriptor& filterDesc)
    {
        AZ_PROFILE_FUNCTION(AzCore);

        // Could have set this during SliceInstance() constructor, but we wait until instantiation since it involves allocation.
        instance.m_dataFlags.SetIsValidEntityFunction([&instance](EntityId entityId) { return instance.IsValidEntity(entityId); });

        InstantiatedContainer sourceObjects(false);
        SliceComponent* dependentSlice = m_asset.Get()->GetComponent();

        // Build a temporary collection of all the entities need to clone to create our instance
        const DataPatch& dataPatch = instance.m_dataPatch;
        EntityIdToEntityIdMap& entityIdMap = instance.m_baseToNewEntityIdMap;
        dependentSlice->GetEntities(sourceObjects.m_entities);

        dependentSlice->GetAllMetadataEntities(sourceObjects.m_metadataEntities);

        // We need to be able to quickly determine if EntityIDs belong to metadata entities
        EntityIdSet sourceMetadataEntityIds;
        dependentSlice->GetMetadataEntityIds(sourceMetadataEntityIds);

        // An empty map indicates its a fresh instance (i.e. has never be instantiated and then serialized).
        if (entityIdMap.empty())
        {
            AZ_PROFILE_SCOPE(AzCore, "SliceComponent::SliceReference::InstantiateInstance:FreshInstanceClone");

            // Generate new Ids and populate the map.
            AZ_Assert(!dataPatch.IsValid(), "Data patch is valid for slice instance, but entity Id map is not!");
            instance.m_instantiated = IdUtils::Remapper<EntityId>::CloneObjectAndGenerateNewIdsAndFixRefs(&sourceObjects, entityIdMap, dependentSlice->GetSerializeContext());
        }
        else
        {
            AZ_PROFILE_SCOPE(AzCore, "SliceComponent::SliceReference::InstantiateInstance:CloneAndApplyDataPatches");

            // Clone entities while applying any data patches.
            AZ_Assert(dataPatch.IsValid(), "Data patch is not valid for existing slice instance!");

            // Get data flags from source slice and slice instance
            DataPatch::FlagsMap sourceDataFlags = dependentSlice->GetDataFlagsForInstances().GetDataFlagsForPatching();
            DataPatch::FlagsMap targetDataFlags = instance.GetDataFlags().GetDataFlagsForPatching(&instance.GetEntityIdToBaseMap());

            instance.m_instantiated = dataPatch.Apply(&sourceObjects, dependentSlice->GetSerializeContext(), filterDesc, sourceDataFlags, targetDataFlags);

            if (!instance.m_instantiated)
            {
                AZ_Error("SliceComponent", false, "Failed to Apply override(s) to instance of slice: %s. Instantiation will complete without applying override(s)\n", m_asset.GetHint().c_str());
                instance.m_instantiated = dependentSlice->GetSerializeContext()->CloneObject<InstantiatedContainer>(&sourceObjects);
            }

            // Remap Ids & references.
            IdUtils::Remapper<EntityId>::ReplaceIdsAndIdRefs(instance.m_instantiated, [&entityIdMap](const EntityId& sourceId, bool isEntityId, const AZStd::function<EntityId()>& idGenerator) -> EntityId
            {
                auto findIt = entityIdMap.find(sourceId);
                if (findIt != entityIdMap.end() && findIt->second.IsValid())
                {
                    return findIt->second;
                }
                else
                {
                    if (isEntityId)
                    {
                        const EntityId id = idGenerator();
                        entityIdMap[sourceId] = id;
                        return id;
                    }

                    return sourceId;
                }
            }, dependentSlice->GetSerializeContext());

            // Prune any entities in from our map that are no longer present in the dependent slice.
            if (entityIdMap.size() != ( sourceObjects.m_entities.size() + sourceObjects.m_metadataEntities.size()))
            {
                const SliceComponent::EntityInfoMap& dependentInfoMap = dependentSlice->GetEntityInfoMap();
                for (auto entityMapIter = entityIdMap.begin(); entityMapIter != entityIdMap.end();)
                {
                    // Skip metadata entities
                    if (sourceMetadataEntityIds.find(entityMapIter->first) != sourceMetadataEntityIds.end())
                    {
                        ++entityMapIter;
                        continue;
                    }

                    auto findInDependentIt = dependentInfoMap.find(entityMapIter->first);
                    if (findInDependentIt == dependentInfoMap.end())
                    {
                        entityMapIter = entityIdMap.erase(entityMapIter);
                        continue;
                    }

                    ++entityMapIter;
                }
            }
        }

        // Find the instanced version of the source metadata entity from the asset associated with this reference
        // and store it in the instance for quick lookups later
        auto metadataIdMapIter = instance.m_baseToNewEntityIdMap.find(dependentSlice->GetMetadataEntity()->GetId());
        AZ_Assert(metadataIdMapIter != instance.m_baseToNewEntityIdMap.end(), "Dependent Metadata Entity ID was not remapped properly.");
        AZ::EntityId instancedMetadataEntityId = metadataIdMapIter->second;

        for (AZ::Entity* entity : instance.m_instantiated->m_metadataEntities)
        {
            if (instancedMetadataEntityId == entity->GetId())
            {
                instance.m_metadataEntity = entity;
                break;
            }
        }

        AZ_Assert(instance.m_metadataEntity, "Instance requires an attached metadata entity!");

        // Ensure reverse lookup is cleared (recomputed on access).
        instance.m_entityIdToBaseCache.clear();

        // Broadcast OnSliceEntitiesLoaded for freshly instantiated entities.
        if (!instance.m_instantiated->m_entities.empty())
        {
            AZ_PROFILE_SCOPE(AzCore, "SliceComponent::SliceReference::InstantiateInstance:OnSliceEntitiesLoaded");
            SliceAssetSerializationNotificationBus::Broadcast(&SliceAssetSerializationNotificationBus::Events::OnSliceEntitiesLoaded, instance.m_instantiated->m_entities);
        }
    }

    //=========================================================================
    // SliceComponent::SliceReference::AddInstanceToEntityInfoMap
    //=========================================================================
    void SliceComponent::SliceReference::AddInstanceToEntityInfoMap(SliceInstance& instance)
    {
        AZ_Assert(m_component, "You need to have a valid component set to update the global entityInfoMap!");
        if (instance.m_instantiated)
        {
            auto& entityInfoMap = m_component->m_entityInfoMap;
            auto& metaDataEntityInfoMap = m_component->m_metaDataEntityInfoMap;

            for (Entity* entity : instance.m_instantiated->m_entities)
            {
                entityInfoMap.emplace(entity->GetId(), EntityInfo(entity, SliceInstanceAddress(this, &instance)));
            }

            for (Entity* metaDataEntity : instance.m_instantiated->m_metadataEntities)
            {
                metaDataEntityInfoMap.emplace(metaDataEntity->GetId(), EntityInfo(metaDataEntity, SliceInstanceAddress(this, &instance)));
            }
        }
    }

    //=========================================================================
    // SliceComponent::SliceReference::RemoveInstanceFromEntityInfoMap
    //=========================================================================
    void SliceComponent::SliceReference::RemoveInstanceFromEntityInfoMap(SliceInstance& instance)
    {
        AZ_Assert(m_component, "You need to have a valid component set to update the global entityInfoMap!");

        // Nothing to clear
        if (!instance.m_instantiated)
        {
            return;
        }

        if (!m_component->m_entityInfoMap.empty())
        {
            for (Entity* entity : instance.m_instantiated->m_entities)
            {
                m_component->m_entityInfoMap.erase(entity->GetId());
            }
        }

        if (!m_component->m_metaDataEntityInfoMap.empty())
        {
            for (Entity* metaDataEntity : instance.m_instantiated->m_metadataEntities)
            {
                m_component->m_metaDataEntityInfoMap.erase(metaDataEntity->GetId());
            }
        }
    }

    //=========================================================================
    // SliceComponent::SliceReference::GetInstanceEntityAncestry
    //=========================================================================
    bool SliceComponent::SliceReference::GetInstanceEntityAncestry(const EntityId& instanceEntityId, EntityAncestorList& ancestors, u32 maxLevels) const
    {
        maxLevels = AZStd::GetMax(maxLevels, static_cast<u32>(1));

        // End recursion when we've reached the max level of ancestors requested
        if (ancestors.size() == maxLevels)
        {
            return true;
        }

        // Locate the instance containing the input entity, which should be a live instanced entity.
        for (const SliceInstance& instance : m_instances)
        {
            // Given the instance's entity Id, resolve the Id of the source entity in the asset.
            auto foundIt = instance.GetEntityIdToBaseMap().find(instanceEntityId);
            if (foundIt != instance.GetEntityIdToBaseMap().end())
            {
                const EntityId assetEntityId = foundIt->second;

                // Ancestor is assetEntityId in this instance's asset.
                const EntityInfoMap& assetEntityInfoMap = m_asset.Get()->GetComponent()->GetEntityInfoMap();
                auto entityInfoIt = assetEntityInfoMap.find(assetEntityId);
                if (entityInfoIt != assetEntityInfoMap.end())
                {
                    ancestors.emplace_back(entityInfoIt->second.m_entity, SliceInstanceAddress(const_cast<SliceReference*>(this), const_cast<SliceInstance*>(&instance)));
                    if (entityInfoIt->second.m_sliceAddress.GetReference())
                    {
                        return entityInfoIt->second.m_sliceAddress.GetReference()->GetInstanceEntityAncestry(assetEntityId, ancestors, maxLevels);
                    }
                }
                return true;
            }
        }
        return false;
    }

    //=========================================================================
    // SliceComponent::SliceReference::ComputeDataPatch
    //=========================================================================
    void SliceComponent::SliceReference::ComputeDataPatch()
    {
        AZ_PROFILE_FUNCTION(AzCore);

        // Get source entities from the base asset (instantiate if needed)
        InstantiatedContainer source(m_asset.Get()->GetComponent(), false);

        SerializeContext* serializeContext = m_asset.Get()->GetComponent()->GetSerializeContext();

        // Compute the delta/changes for each instance
        for (SliceInstance& instance : m_instances)
        {
            ComputeDataPatchForInstanceKnownToReference(instance, serializeContext, source);
        }
    }

    void SliceComponent::SliceReference::ComputeDataPatch(SliceInstance* instance)
    {
        if (instance == nullptr || m_instances.find(*instance) == m_instances.end())
        {
            AZ_Error("SliceComponent", false, "Cannot compute data patch for a slice instance that is not associated with this slice reference.");
            return;
        }
        InstantiatedContainer source(m_asset.Get()->GetComponent(), false);

        SerializeContext* serializeContext = m_asset.Get()->GetComponent()->GetSerializeContext();
        ComputeDataPatchForInstanceKnownToReference(*instance, serializeContext, source);
    }

    void SliceComponent::SliceReference::ComputeDataPatchForInstanceKnownToReference(SliceInstance& instance, SerializeContext* serializeContext, InstantiatedContainer& sourceContainer)
    {            
        // remap entity ids to the "original"
        const EntityIdToEntityIdMap& reverseLookUp = instance.GetEntityIdToBaseMap();
        IdUtils::Remapper<EntityId>::ReplaceIdsAndIdRefs(instance.m_instantiated, [&reverseLookUp](const EntityId& sourceId, bool /*isEntityId*/, const AZStd::function<EntityId()>& /*idGenerator*/) -> EntityId
        {
            auto findIt = reverseLookUp.find(sourceId);
            if (findIt != reverseLookUp.end())
            {
                return findIt->second;
            }
            else
            {
                return sourceId;
            }
        }, serializeContext);

        // Get data flags from source slice and slice instance
        DataPatch::FlagsMap sourceDataFlags = m_asset.Get()->GetComponent()->GetDataFlagsForInstances().GetDataFlagsForPatching();
        DataPatch::FlagsMap targetDataFlags = instance.GetDataFlags().GetDataFlagsForPatching(&instance.GetEntityIdToBaseMap());

        // compute the delta (what we changed from the base slice)
        instance.m_dataPatch.Create(&sourceContainer, instance.m_instantiated, sourceDataFlags, targetDataFlags, serializeContext);

        // remap entity ids back to the "instance onces"
        IdUtils::Remapper<EntityId>::ReplaceIdsAndIdRefs(instance.m_instantiated, [&instance](const EntityId& sourceId, bool /*isEntityId*/, const AZStd::function<EntityId()>& /*idGenerator*/) -> EntityId
        {
            auto findIt = instance.m_baseToNewEntityIdMap.find(sourceId);
            if (findIt != instance.m_baseToNewEntityIdMap.end())
            {
                return findIt->second;
            }
            else
            {
                return sourceId;
            }
        }, serializeContext);

        // clean up orphaned data flags (ex: for entities that no longer exist).
        instance.m_dataFlags.Cleanup(instance.m_instantiated->m_entities);
    }

    void SliceComponent::SliceReference::FixUpMetadataEntityForSliceInstance(SliceInstance* sliceInstance)
    {
        // Let the engine know about the newly created metadata entities.
        SliceComponent* assetSlice = m_asset.Get()->GetComponent();
        AZ::EntityId instancedMetadataEntityId = sliceInstance->m_baseToNewEntityIdMap.find(assetSlice->GetMetadataEntity()->GetId())->second;
        AZ_Assert(instancedMetadataEntityId.IsValid(), "Must have a valid entity ID.");
        for (AZ::Entity* instantiatedMetadataEntity : sliceInstance->m_instantiated->m_metadataEntities)
        {
            // While we're touching each one, find the instantiated entity associated with this instance and store it
            if (instancedMetadataEntityId == instantiatedMetadataEntity->GetId())
            {
                AZ_Assert(sliceInstance->m_metadataEntity == nullptr, "Multiple metadata entities associated with this instance found.");
                sliceInstance->m_metadataEntity = instantiatedMetadataEntity;
            }

            SliceInstanceNotificationBus::Broadcast(&SliceInstanceNotificationBus::Events::OnMetadataEntityCreated, SliceInstanceAddress(this, sliceInstance), *instantiatedMetadataEntity);
        }

        // Make sure we found the metadata entity associated with this instance
        AZ_Assert(sliceInstance->m_metadataEntity, "Instance requires an attached metadata entity!");
    }

    //=========================================================================
    // SliceComponent
    //=========================================================================
    SliceComponent::SliceComponent()
        : m_myAsset(nullptr)
        , m_serializeContext(nullptr)
        , m_hasGeneratedCachedDataFlags(false)
        , m_slicesAreInstantiated(false)
        , m_allowPartialInstantiation(true)
        , m_isDynamic(false)
        , m_filterFlags(0)
    {
    }

    //=========================================================================
    // ~SliceComponent
    //=========================================================================
    SliceComponent::~SliceComponent()
    {
        for (Entity* entity : m_entities)
        {
            delete entity;
        }

        if (m_metadataEntity.GetState() == AZ::Entity::State::Active)
        {
            SliceInstanceNotificationBus::Broadcast(&SliceInstanceNotificationBus::Events::OnMetadataEntityDestroyed, m_metadataEntity.GetId());
        }

        Data::AssetBus::MultiHandler::BusDisconnect();
    }

    //=========================================================================
    // SliceComponent::GetNewEntities
    //=========================================================================
    const SliceComponent::EntityList& SliceComponent::GetNewEntities() const
    {
        return m_entities;
    }

    //=========================================================================
    // SliceComponent::GetEntities
    //=========================================================================
    bool SliceComponent::GetEntities(EntityList& entities)
    {
        AZ_PROFILE_FUNCTION(AzCore);

        bool result = true;

        // make sure we have all entities instantiated
        if (Instantiate() != InstantiateResult::Success)
        {
            result = false;
        }

        // add all entities from base slices
        for (const SliceReference& slice : m_slices)
        {
            for (const SliceInstance& instance : slice.m_instances)
            {
                if (instance.m_instantiated)
                {
                    entities.insert(entities.end(), instance.m_instantiated->m_entities.begin(), instance.m_instantiated->m_entities.end());
                }
            }
        }

        // add new entities (belong to the current slice)
        entities.insert(entities.end(), m_entities.begin(), m_entities.end());

        return result;
    }

    //=========================================================================
    // SliceComponent::GetEntityIds
    //=========================================================================
    bool SliceComponent::GetEntityIds(EntityIdSet& entities)
    {
        AZ_PROFILE_FUNCTION(AzCore);

        bool result = true;

        // make sure we have all entities instantiated
        if (Instantiate() != InstantiateResult::Success)
        {
            result = false;
        }

        // add all entities from base slices
        for (const SliceReference& slice : m_slices)
        {
            for (const SliceInstance& instance : slice.m_instances)
            {
                if (instance.m_instantiated)
                {
                    for (const AZ::Entity* entity : instance.m_instantiated->m_entities)
                    {
                        entities.insert(entity->GetId());
                    }
                }
            }
        }

        // add new entities (belong to the current slice)
        for (const AZ::Entity* entity : m_entities)
        {
            entities.insert(entity->GetId());
        }

        return result;
    }

    //=========================================================================
    size_t SliceComponent::GetInstantiatedEntityCount() const
    {
        if (!m_slicesAreInstantiated)
        {
            return 0;
        }

        return const_cast<SliceComponent*>(this)->GetEntityInfoMap().size();
    }

    //=========================================================================
    // SliceComponent::GetMetadataEntityIds
    //=========================================================================
    bool SliceComponent::GetMetadataEntityIds(EntityIdSet& metadataEntities)
    {
        AZ_PROFILE_FUNCTION(AzCore);

        bool result = true;

        // make sure we have all entities instantiated
        if (Instantiate() != InstantiateResult::Success)
        {
            result = false;
        }

        // add all entities from base slices
        for (const SliceReference& slice : m_slices)
        {
            for (const SliceInstance& instance : slice.m_instances)
            {
                if (instance.m_instantiated)
                {
                    for (const AZ::Entity* entity : instance.m_instantiated->m_metadataEntities)
                    {
                        metadataEntities.insert(entity->GetId());
                    }
                }
            }
        }

        metadataEntities.insert(m_metadataEntity.GetId());

        return result;
    }

    //=========================================================================
    // SliceComponent::GetSlices
    //=========================================================================
    const SliceComponent::SliceList& SliceComponent::GetSlices() const
    {
        return m_slices;
    }

    const SliceComponent::SliceList& SliceComponent::GetInvalidSlices() const
    {
        return m_invalidSlices;
    }

    //=========================================================================
    // SliceComponent::GetSlice
    //=========================================================================
    SliceComponent::SliceReference* SliceComponent::GetSlice(const Data::Asset<SliceAsset>& sliceAsset)
    {
        return GetSlice(sliceAsset.GetId());
    }

    //=========================================================================
    // SliceComponent::GetSlice
    //=========================================================================
    SliceComponent::SliceReference* SliceComponent::GetSlice(const Data::AssetId& assetId)
    {
        for (SliceReference& slice : m_slices)
        {
            if (slice.m_asset.GetId() == assetId)
            {
                return &slice;
            }
        }

        return nullptr;
    }

    //=========================================================================
    // SliceComponent::Instantiate
    //=========================================================================
    SliceComponent::InstantiateResult SliceComponent::Instantiate()
    {
        AZ_PROFILE_FUNCTION(AzCore);
        AZStd::unique_lock<AZStd::recursive_mutex> lock(m_instantiateMutex);

        if (m_slicesAreInstantiated)
        {
            return InstantiateResult::Success;
        }

        // Could have set this during constructor, but we wait until Instantiate() since it involves allocation.
        m_dataFlagsForNewEntities.SetIsValidEntityFunction([this](EntityId entityId) { return IsNewEntity(entityId); });

        InstantiateResult result(InstantiateResult::Success);

        if (m_myAsset)
        {
            PushInstantiateCycle(m_myAsset->GetId());
        }
        
        for (SliceReference& slice : m_slices)
        {
            slice.m_component = this;
            AZ::Data::AssetId sliceAssetId = slice.m_asset.GetId();
            
            if (CheckContainsInstantiateCycle(sliceAssetId))
            {
                result = InstantiateResult::CyclicalDependency;
            }
            else
            {
                bool instantiateSuccess = slice.Instantiate(AZ::ObjectStream::FilterDescriptor(m_assetLoadFilterCB, m_filterFlags));
                if (instantiateSuccess)
                {
                    // Prune empty slice instances.
                    for (SliceReference::SliceInstances::iterator itr = slice.m_instances.begin(); itr != slice.m_instances.end();)
                    {
                        if (itr->m_instantiated->m_entities.empty())
                        {
                            itr = slice.RemoveInstance(itr);
                        }
                        else
                        {
                            ++itr;
                        }
                    }
                }
                else
                {
                    result = InstantiateResult::MissingDependency;
                }
            }
        }

        // Prune empty slice references.
        for (SliceList::iterator itr = m_slices.begin(); itr != m_slices.end();)
        {
            if (itr->GetInstances().empty())
            {
                itr = RemoveSliceReference(itr);
            }
            else
            {
                ++itr;
            }
        }

        if (m_myAsset)
        {
            PopInstantiateCycle(m_myAsset->GetId());
        }

        m_slicesAreInstantiated = result == InstantiateResult::Success;

        if (result != InstantiateResult::Success)
        {
            if (m_allowPartialInstantiation)
            {
                // Strip any references that failed to instantiate.
                for (auto iter = m_slices.begin(); iter != m_slices.end(); )
                {
                    if (!iter->IsInstantiated())
                    {
                        #if defined(AZ_ENABLE_TRACING)
                        Data::Asset<SliceAsset> thisAsset = GetMyAsset()
                            ? Data::Asset<SliceAsset>(Data::AssetManager::Instance().FindAsset(
                                  GetMyAsset()->GetId(), AZ::Data::AssetLoadBehavior::Default))
                            : Data::Asset<SliceAsset>();
                        AZ_Warning("Slice", false, "Removing %d instances of slice asset %s from parent asset %s due to failed instantiation. " 
                            "Saving parent asset will result in loss of slice data.", 
                            iter->GetInstances().size(),
                            iter->GetSliceAsset().ToString<AZStd::string>().c_str(),
                            thisAsset.ToString<AZStd::string>().c_str());
                        #endif // AZ_ENABLE_TRACING
                        Data::AssetBus::MultiHandler::BusDisconnect(iter->GetSliceAsset().GetId());
                        // Track missing dependencies. Cyclical dependencies should not be tracked, to make sure
                        // the asset isn't kept loaded due to a cyclical asset reference.
                        if (result == InstantiateResult::MissingDependency)
                        {
                            m_invalidSlices.push_back(*iter);
                        }
                        iter = m_slices.erase(iter);
                    }
                    else
                    {
                        ++iter;
                        m_slicesAreInstantiated = true; // At least one reference was instantiated.
                    }
                }
            }
            else
            {
                // Completely roll back instantiation.
                for (SliceReference& slice : m_slices)
                {
                    if (slice.IsInstantiated())
                    {
                        slice.UnInstantiate();
                    }
                }
            }
        }

        if (m_slicesAreInstantiated)
        {
            if (m_myAsset)
            {
                AZStd::string sliceAssetPath;
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(sliceAssetPath, &AZ::Data::AssetCatalogRequests::GetAssetPathById, m_myAsset->GetId());
                m_metadataEntity.SetName(sliceAssetPath);
            }
            else
            {
                m_metadataEntity.SetName("No Asset Association");
            }

            InitMetadata();
        }

        return result;
    }

    //=========================================================================
    // SliceComponent::IsInstantiated
    //=========================================================================
    bool SliceComponent::IsInstantiated() const
    {
        return m_slicesAreInstantiated;
    }

    //=========================================================================
    // SliceComponent::GenerateNewEntityIds
    //=========================================================================
    void SliceComponent::GenerateNewEntityIds(EntityIdToEntityIdMap* previousToNewIdMap)
    {
        InstantiatedContainer entityContainer(false);
        if (!GetEntities(entityContainer.m_entities))
        {
            return;
        }

        EntityIdToEntityIdMap entityIdMap;
        if (!previousToNewIdMap)
        {
            previousToNewIdMap = &entityIdMap;
        }
        AZ::EntityUtils::GenerateNewIdsAndFixRefs(&entityContainer, *previousToNewIdMap, m_serializeContext);

        m_dataFlagsForNewEntities.RemapEntityIds(*previousToNewIdMap);

        for (SliceReference& sliceReference : m_slices)
        {
            for (SliceInstance& instance : sliceReference.m_instances)
            {
                for (auto& entityIdPair : instance.m_baseToNewEntityIdMap)
                {
                    auto iter = previousToNewIdMap->find(entityIdPair.second);
                    if (iter != previousToNewIdMap->end())
                    {
                        entityIdPair.second = iter->second;
                    }
                }

                instance.m_entityIdToBaseCache.clear();
                instance.m_dataFlags.RemapEntityIds(*previousToNewIdMap);
            }
        }

        RebuildEntityInfoMapIfNecessary();
    }

    //=========================================================================
    // SliceComponent::AddSlice
    //=========================================================================
    SliceComponent::SliceInstanceAddress SliceComponent::AddSlice(const Data::Asset<SliceAsset>& sliceAsset, const AZ::IdUtils::Remapper<AZ::EntityId>::IdMapper& customMapper, 
        SliceInstanceId sliceInstanceId)
    {
        SliceReference* slice = AddOrGetSliceReference(sliceAsset);
        return SliceInstanceAddress(slice, slice->CreateInstance(customMapper, sliceInstanceId));
    }

    SliceComponent::SliceInstanceAddress SliceComponent::AddSliceUsingExistingEntities(const Data::Asset<SliceAsset>& sliceAsset, const AZ::SliceComponent::EntityIdToEntityIdMap& liveToAssetMap,
        SliceInstanceId sliceInstanceId)
    {
        AZ_PROFILE_FUNCTION(AzCore);

        if (!sliceAsset.Get()->GetComponent())
        {
            AZ_Error("SliceComponent::AddSliceUsingExistingEntities",
                false,
                "Unable to complete operation. Provided Slice Asset with Id: %s contains an invalid Slice Component. An empty SliceInstanceAddress will be returned.",
                sliceAsset.ToString<AZStd::string>().c_str());

            return SliceInstanceAddress();
        }

        if (liveToAssetMap.empty())
        {
            AZ_Warning("SliceComponent::AddSliceUsingExistingEntities",
                false,
                "Attempted to Add Slice with Asset Id: %s using existing entities. However provided entity Id map was empty. An empty SliceInstanceAddress will be returned.",
                sliceAsset.ToString<AZStd::string>().c_str());

            return SliceInstanceAddress();
        }

        SliceReference* slice = AddOrGetSliceReference(sliceAsset);

        AZStd::vector<AZ::Entity*> validEntities;
        EntityIdToEntityIdMap validAssetToLiveEntityMap;

        AZ::SliceComponent* assetSliceComponent = sliceAsset.Get()->GetComponent();

        // Ensure assetSliceComponent is instantiated before attempting to call FindEntity on it
        if (!assetSliceComponent->IsInstantiated())
        {
            assetSliceComponent->Instantiate();
        }

        for (const AZStd::pair<AZ::EntityId, AZ::EntityId>& liveToAssetPair : liveToAssetMap)
        {
            // See if we can find a matching asset entity in our asset slice
            AZ::Entity* assetEntity = assetSliceComponent->FindEntity(liveToAssetPair.second);

            if (!assetEntity)
            {
                continue;
            }

            // See if we can find a matching entity owned by "this" slice component
            AZ::Entity* liveEntity = FindEntity(liveToAssetPair.first);

            if (!liveEntity)
            {
                continue;
            }

            // Remove the entity from "this" slice component but do not delete it
            // Add it to our lists of valid entities
            RemoveEntity(liveEntity->GetId(), false);
            validEntities.emplace_back(liveEntity);
            validAssetToLiveEntityMap.emplace(liveToAssetPair.second, liveToAssetPair.first);
        }

        // No existing entities matched the mapping provided
        if (validEntities.empty())
        {
            AZ_Warning("SliceComponent::AddSliceUsingExistingEntities",
                       false,
                       "No existing entities owned by this SliceComponent matched the mapping provided. An empty SliceInstanceAddress will be returned.");

            return SliceInstanceAddress();
        }

        return SliceInstanceAddress(slice, slice->CreateInstanceFromExistingEntities(validEntities, validAssetToLiveEntityMap, sliceInstanceId));
    }

    //=========================================================================
    // SliceComponent::AddSliceInstance
    //=========================================================================
    SliceComponent::SliceReference* SliceComponent::AddSlice(SliceReference& sliceReference)
    {
        // Assert for input parameters
        // Assert that we don't already have a reference for this assetId
        AZ_Assert(!Data::AssetBus::MultiHandler::BusIsConnectedId(sliceReference.GetSliceAsset().GetId()), "We already have a slice reference to this asset");
        AZ_Assert(false == sliceReference.m_isInstantiated, "Slice reference is already instantiated.");

        Data::AssetBus::MultiHandler::BusConnect(sliceReference.GetSliceAsset().GetId());
        m_slices.emplace_back(AZStd::move(sliceReference));
        SliceReference* slice = &m_slices.back();
        slice->m_component = this;

        // check if we instantiated but the reference is not, instantiate
        // if the reference is and we are not, delete it
        if (m_slicesAreInstantiated)
        {
            slice->Instantiate(AZ::ObjectStream::FilterDescriptor(m_assetLoadFilterCB, m_filterFlags));
        }

        // Refresh entity map for newly-created instances.
        RebuildEntityInfoMapIfNecessary();

        return slice;
    }
    
    //=========================================================================
    // SliceComponent::GetEntityRestoreInfo
    //=========================================================================
    bool SliceComponent::GetEntityRestoreInfo(const EntityId entityId, EntityRestoreInfo& restoreInfo)
    {
        restoreInfo = EntityRestoreInfo();

        const EntityInfoMap& entityInfo = GetEntityInfoMap();
        auto entityInfoIter = entityInfo.find(entityId);
        if (entityInfoIter != entityInfo.end())
        {
            const SliceReference* reference = entityInfoIter->second.m_sliceAddress.GetReference();
            if (reference)
            {
                const SliceInstance* instance = entityInfoIter->second.m_sliceAddress.GetInstance();
                AZ_Assert(instance, "Entity %llu was found to belong to reference %s, but instance is invalid.", 
                          entityId, reference->GetSliceAsset().ToString<AZStd::string>().c_str());

                EntityAncestorList ancestors;
                reference->GetInstanceEntityAncestry(entityId, ancestors, 1);
                if (!ancestors.empty())
                {
                    restoreInfo = EntityRestoreInfo(reference->GetSliceAsset(), instance->GetId(), ancestors.front().m_entity->GetId(), instance->m_dataFlags.GetEntityDataFlags(entityId));
                    return true;
                }
                else
                {
                    AZ_Error("Slice", false, "Entity with id %llu was found, but has no valid ancestry.", entityId);
                }
            }
        }

        return false;
    }

    //=========================================================================
    // SliceComponent::RestoreEntity
    //=========================================================================
    SliceComponent::SliceInstanceAddress SliceComponent::RestoreEntity(Entity* entity, const EntityRestoreInfo& restoreInfo, bool isEntityAdd)
    {
        Data::Asset<SliceAsset> asset = Data::AssetManager::Instance().FindAsset(restoreInfo.m_assetId, AZ::Data::AssetLoadBehavior::Default);

        if (!asset.IsReady())
        {
            AZ_Error("Slice", false, "Slice asset %s is not ready. Caller needs to ensure the asset is loaded.", restoreInfo.m_assetId.ToString<AZStd::string>().c_str());
            return SliceInstanceAddress();
        }

        if (!IsInstantiated())
        {
            AZ_Error("Slice", false, "Cannot add entities to existing instances if the slice hasn't yet been instantiated.");
            return SliceInstanceAddress();
        }
        
        SliceComponent* sourceSlice = asset.GetAs<SliceAsset>()->GetComponent();
        sourceSlice->Instantiate();

        if (!isEntityAdd)
        {
            auto& sourceEntityMap = sourceSlice->GetEntityInfoMap();
            if (sourceEntityMap.find(restoreInfo.m_ancestorId) == sourceEntityMap.end())
            {
                AZ_Error("Slice", false, "Ancestor Id of %llu is invalid. It must match an entity in source asset %s.",
                    restoreInfo.m_ancestorId, asset.ToString<AZStd::string>().c_str());
                return SliceInstanceAddress();
            }
        }

        const SliceComponent::SliceInstanceAddress address = FindSlice(entity);
        if (address.GetReference())
        {
            return address;
        }

        SliceReference* reference = AddOrGetSliceReference(asset);
        SliceInstance* instance = reference->FindInstance(restoreInfo.m_instanceId);

        if (!instance)
        {
            // We're creating an instance just to hold the entity we're re-adding. We don't want to instantiate the underlying asset.
            instance = reference->CreateEmptyInstance(restoreInfo.m_instanceId);

            if (!instance)
            {
                AZ_Assert(false, "Restore info attempted to restore SliceInstanceId: %s which is already in use", restoreInfo.m_instanceId.ToString<AZStd::string>().c_str());
            }
            
            // Make sure the appropriate metadata entities are created with the instance
            InstantiatedContainer sourceObjects(false);
            sourceSlice->GetAllMetadataEntities(sourceObjects.m_metadataEntities);

            instance->m_instantiated = IdUtils::Remapper<EntityId>::CloneObjectAndGenerateNewIdsAndFixRefs(&sourceObjects, instance->m_baseToNewEntityIdMap, GetSerializeContext());

            // Find the instanced version of the source metadata entity from the asset associated with this reference
            // and store it in the instance for quick lookups later
            reference->FixUpMetadataEntityForSliceInstance(instance);
        }

        // Add the entity to the instance, and wipe the reverse lookup cache so it's updated on access.
        instance->m_instantiated->m_entities.push_back(entity);
        instance->m_baseToNewEntityIdMap[restoreInfo.m_ancestorId] = entity->GetId();
        instance->m_entityIdToBaseCache.clear();
        instance->m_dataFlags.SetEntityDataFlags(entity->GetId(), restoreInfo.m_dataFlags);

        RebuildEntityInfoMapIfNecessary();

        return SliceInstanceAddress(reference, instance);
    }

    //=========================================================================
    // SliceComponent::GetReferencedSliceAssets
    //=========================================================================
    void SliceComponent::GetReferencedSliceAssets(AssetIdSet& idSet, bool recurse)
    {
        for (auto& sliceReference : m_slices)
        {
            const Data::Asset<SliceAsset>& referencedSliceAsset = sliceReference.GetSliceAsset();
            const Data::AssetId referencedSliceAssetId = referencedSliceAsset.GetId();
            if (idSet.find(referencedSliceAssetId) == idSet.end())
            {
                idSet.insert(referencedSliceAssetId);
                if (recurse)
                {
                    referencedSliceAsset.Get()->GetComponent()->GetReferencedSliceAssets(idSet, recurse);
                }
            }
        }
    }

    //=========================================================================
    // SliceComponent::AddSliceInstance
    //=========================================================================
    SliceComponent::SliceInstanceAddress SliceComponent::AddSliceInstance(SliceReference* sliceReference, SliceInstance* sliceInstance)
    {
        if (sliceReference && sliceInstance)
        {
            // sanity check that instance belongs to slice reference
            auto findIt = AZStd::find_if(sliceReference->m_instances.begin(), sliceReference->m_instances.end(), [sliceInstance](const SliceInstance& element) -> bool { return &element == sliceInstance; });
            if (findIt == sliceReference->m_instances.end())
            {
                AZ_Error("Slice", false, "SliceInstance %p doesn't belong to SliceReference %p!", sliceInstance, sliceReference);
                return SliceInstanceAddress();
            }

            if (!m_slicesAreInstantiated && sliceReference->m_isInstantiated)
            {
                // if we are not instantiated, but the source sliceInstance is we need to instantiate
                // to capture any changes that might come with it.
                if (Instantiate() != InstantiateResult::Success)
                {
                    return SliceInstanceAddress();
                }
            }

            SliceReference* newReference = GetSlice(sliceReference->m_asset);
            if (!newReference)
            {
                Data::AssetBus::MultiHandler::BusConnect(sliceReference->m_asset.GetId());
                m_slices.push_back();
                newReference = &m_slices.back();
                newReference->m_component = this;
                newReference->m_asset = sliceReference->m_asset;
                newReference->m_isInstantiated = m_slicesAreInstantiated;
            }

            // Move the instance to the new reference and remove it from its old owner.
            const SliceInstanceId instanceId = sliceInstance->GetId();
            sliceReference->RemoveInstanceFromEntityInfoMap(*sliceInstance);
            SliceInstance& newInstance = *newReference->m_instances.emplace(AZStd::move(*sliceInstance)).first;
            // Set the id of old slice-instance back to what it was so that it can be removed, because SliceInstanceId is used as hash key.
            sliceInstance->SetId(instanceId);

            if (!m_entityInfoMap.empty())
            {
                newReference->AddInstanceToEntityInfoMap(newInstance);
            }

            sliceReference->RemoveInstance(sliceInstance);

            if (newReference->m_isInstantiated && !sliceReference->m_isInstantiated)
            {
                // the source instance is not instantiated, make sure we instantiate it.
                newReference->InstantiateInstance(newInstance, AZ::ObjectStream::FilterDescriptor(m_assetLoadFilterCB, m_filterFlags));
            }

            return SliceInstanceAddress(newReference, &newInstance);
        }
        return SliceInstanceAddress();
    }

    void SliceComponent::GetMappingBetweenSubsliceAndSourceInstanceEntityIds(const SliceComponent::SliceInstance* sourceSliceInstance,
        const AZStd::vector<AZ::SliceComponent::SliceInstanceAddress>& sourceSubsliceInstanceAncestry,
        const AZ::SliceComponent::SliceInstanceAddress& sourceSubsliceInstanceAddress,
        AZ::SliceComponent::EntityIdToEntityIdMap& subsliceToLiveMappingResult,
        bool flipMapping /*=false*/)
    {
        const SliceComponent::EntityIdToEntityIdMap& baseToLiveEntityIdMap = sourceSliceInstance->GetEntityIdMap();
        const SliceComponent::EntityIdToEntityIdMap& subsliceBaseToInstanceEntityIdMap = sourceSubsliceInstanceAddress.GetInstance()->GetEntityIdMap();

        for (auto& pair : subsliceBaseToInstanceEntityIdMap)
        {
            EntityId subsliceIntanceEntityId = pair.second;

            for (auto revIt = sourceSubsliceInstanceAncestry.rbegin(); revIt != sourceSubsliceInstanceAncestry.rend(); revIt++)
            {
                const SliceComponent::EntityIdToEntityIdMap& ancestorBaseToInstanceEntityIdMap = revIt->GetInstance()->GetEntityIdMap();
                auto foundItr = ancestorBaseToInstanceEntityIdMap.find(subsliceIntanceEntityId);
                if (foundItr != ancestorBaseToInstanceEntityIdMap.end())
                {
                    subsliceIntanceEntityId = foundItr->second;
                }
            }

            // If a live entity is deleted, it will also be deleted from baseToLiveEntityIdMap, so no need to check existence of entities here.
            auto foundItr = baseToLiveEntityIdMap.find(subsliceIntanceEntityId);
            if (foundItr == baseToLiveEntityIdMap.end())
            {
                continue;
            }

            if (!flipMapping)
            {
                subsliceToLiveMappingResult.emplace(pair.first, foundItr->second);
            }
            else
            {
                subsliceToLiveMappingResult.emplace(foundItr->second, pair.first);
            }
        }
    }

    SliceComponent::SliceInstanceAddress SliceComponent::CloneAndAddSubSliceInstance(
        const SliceComponent::SliceInstance* sourceSliceInstance,
        const AZStd::vector<AZ::SliceComponent::SliceInstanceAddress>& sourceSubSliceInstanceAncestry,
        const AZ::SliceComponent::SliceInstanceAddress& sourceSubSliceInstanceAddress,
        AZ::SliceComponent::EntityIdToEntityIdMap* out_sourceToCloneEntityIdMap /*= nullptr*/, bool preserveIds /*= false*/)
    {
        // Check if sourceSliceInstance belongs to this SliceComponent.
        if (sourceSliceInstance->m_instantiated->m_entities.empty() 
            || !FindEntity(sourceSliceInstance->m_instantiated->m_entities[0]->GetId()))
        {
            return SliceComponent::SliceInstanceAddress();
        }

        EntityIdToEntityIdMap subSliceBaseToLiveEntityIdMap;

        GetMappingBetweenSubsliceAndSourceInstanceEntityIds(sourceSliceInstance,
            sourceSubSliceInstanceAncestry,
            sourceSubSliceInstanceAddress,
            subSliceBaseToLiveEntityIdMap);

        if (subSliceBaseToLiveEntityIdMap.empty())
        {
            return SliceComponent::SliceInstanceAddress();
        }

        AZStd::unordered_map<AZ::EntityId, AZ::Entity*> liveMetadataEntityMap;
        for (AZ::Entity* metaEntity : sourceSliceInstance->GetInstantiated()->m_metadataEntities)
        {
            liveMetadataEntityMap.emplace(metaEntity->GetId(), metaEntity);
        }

        // Separate MetaData entities from normal ones.
        InstantiatedContainer sourceEntitiesContainer(false); // to be cloned
        for (auto& pair : subSliceBaseToLiveEntityIdMap)
        {
            Entity* entity = FindEntity(pair.second);
            if (entity)
            {
                sourceEntitiesContainer.m_entities.push_back(entity);
            }
            else
            {
                auto foundItr = liveMetadataEntityMap.find(pair.second);
                if (foundItr != liveMetadataEntityMap.end())
                {
                    sourceEntitiesContainer.m_metadataEntities.push_back(foundItr->second);
                }
            }
        }

        SliceReference* newSliceReference = AddOrGetSliceReference(sourceSubSliceInstanceAddress.GetReference()->GetSliceAsset());
        SliceInstance* newInstance = newSliceReference->CreateEmptyInstance();

        EntityIdToEntityIdMap sourceToCloneEntityIdMap;

        // If we are preserving entity ID's we want to clone directly and not generate new IDs
        if (preserveIds)
        {
            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            if (!serializeContext)
            {
                AZ_Error("Serialization", false, "No serialize context provided! Failed to get component application default serialize context! ComponentApp is not started or input serialize context should not be null!");
                newInstance->m_instantiated = nullptr;
            }
            else
            {
                newInstance->m_instantiated = serializeContext->CloneObject(&sourceEntitiesContainer);
            }
        }
        else
        {
            newInstance->m_instantiated = IdUtils::Remapper<AZ::EntityId>::CloneObjectAndGenerateNewIdsAndFixRefs(&sourceEntitiesContainer, sourceToCloneEntityIdMap);
        }

        // Generate EntityId map between entities in base sub-slice and the cloned entities, 
        // also generate DataFlags for cloned entities (in newInstance) based on its corresponding source entities.
        const DataFlagsPerEntity& liveInstanceDataFlags = sourceSliceInstance->GetDataFlags();
        DataFlagsPerEntity cloneInstanceDataFlags;
        EntityIdToEntityIdMap subsliceBaseToCloneEntityIdMap;
        for (auto& pair : subSliceBaseToLiveEntityIdMap)
        {
            if (preserveIds)
            {
                // If preserving ID's the map from subslice to live entity is valid over
                // The map of base to generated IDs
                subsliceBaseToCloneEntityIdMap.emplace(pair);
                newInstance->m_dataFlags.SetEntityDataFlags(pair.second, liveInstanceDataFlags.GetEntityDataFlags(pair.second));
            }
            else
            {
                AZ::EntityId liveEntityId = pair.second;
                auto foundItr = sourceToCloneEntityIdMap.find(liveEntityId);
                if (foundItr != sourceToCloneEntityIdMap.end())
                {
                    AZ::EntityId cloneId = foundItr->second;
                    subsliceBaseToCloneEntityIdMap.emplace(pair.first, cloneId);

                    newInstance->m_dataFlags.SetEntityDataFlags(cloneId, liveInstanceDataFlags.GetEntityDataFlags(liveEntityId));
                }
            }
        }
        newInstance->m_baseToNewEntityIdMap = AZStd::move(subsliceBaseToCloneEntityIdMap);

        newSliceReference->FixUpMetadataEntityForSliceInstance(newInstance);

        newSliceReference->AddInstanceToEntityInfoMap(*newInstance);

        if (out_sourceToCloneEntityIdMap)
        {
            *out_sourceToCloneEntityIdMap = AZStd::move(sourceToCloneEntityIdMap);
        }

        return SliceInstanceAddress(newSliceReference, newInstance);
    }

    //=========================================================================
    // SliceComponent::RemoveSlice
    //=========================================================================
    bool SliceComponent::RemoveSlice(const Data::Asset<SliceAsset>& sliceAsset)
    {
        for (auto sliceIt = m_slices.begin(); sliceIt != m_slices.end(); ++sliceIt)
        {
            if (sliceIt->m_asset == sliceAsset)
            {
                RemoveSliceReference(sliceIt);
                return true;
            }
        }
        return false;
    }

    //=========================================================================
    // SliceComponent::RemoveSlice
    //=========================================================================
    bool SliceComponent::RemoveSlice(const SliceReference* slice)
    {
        if (slice)
        {
            return RemoveSlice(slice->m_asset);
        }
        return false;
    }

    //=========================================================================
    // SliceComponent::RemoveSliceInstance
    //=========================================================================
    bool SliceComponent::RemoveSliceInstance(SliceComponent::SliceInstanceAddress sliceAddress)
    {
        AZ_PROFILE_FUNCTION(AzCore);
        if (!sliceAddress.IsValid())
        {
            AZ_Error("Slices", false, "Slice address is invalid.");
            return false;
        }

        if (sliceAddress.GetReference()->GetSliceComponent() != this)
        {
            AZ_Error("Slices", false, "SliceComponent does not own this slice");
            return false;
        }

        SliceReference* sliceReference = sliceAddress.GetReference();

        if (sliceReference->RemoveInstance(sliceAddress.GetInstance()))
        {
            if (sliceReference->m_instances.empty())
            {
                RemoveSlice(sliceReference);
            }

            return true;
        }

        return false;
    }

    //=========================================================================
    // SliceComponent::RemoveSliceInstance
    //=========================================================================
    bool SliceComponent::RemoveSliceInstance(SliceInstance* instance)
    {
        for (auto sliceReferenceIt = m_slices.begin(); sliceReferenceIt != m_slices.end(); ++sliceReferenceIt)
        {
            // note move this function to the slice reference for consistency
            if (sliceReferenceIt->RemoveInstance(instance))
            {
                if (sliceReferenceIt->m_instances.empty())
                {
                    RemoveSliceReference(sliceReferenceIt);
                }
                return true;
            }
        }
        return false;
    }

    //=========================================================================
    // SliceComponent::RemoveSliceReference
    //=========================================================================
    SliceComponent::SliceList::iterator SliceComponent::RemoveSliceReference(SliceComponent::SliceList::iterator sliceReferenceIt)
    {
        Data::AssetBus::MultiHandler::BusDisconnect(sliceReferenceIt->GetSliceAsset().GetId());
        return m_slices.erase(sliceReferenceIt);
    }

    //=========================================================================
    // SliceComponent::AddEntity
    //=========================================================================
    void SliceComponent::AddEntity(Entity* entity)
    {
        AZ_Assert(entity, "You passed an invalid entity!");
        m_entities.push_back(entity);

        if (!m_entityInfoMap.empty())
        {
            m_entityInfoMap.insert(AZStd::make_pair(entity->GetId(), EntityInfo(entity, SliceInstanceAddress())));
        }
    }

    //=========================================================================
    // SliceComponent::RemoveEntity
    //=========================================================================
    bool SliceComponent::RemoveEntity(Entity* entity, bool isDeleteEntity, bool isRemoveEmptyInstance)
    {
        if (entity)
        {
            return RemoveEntity(entity->GetId(), isDeleteEntity, isRemoveEmptyInstance);
        }

        return false;
    }

    //=========================================================================
    // SliceComponent::RemoveEntity
    //=========================================================================
    bool SliceComponent::RemoveEntity(EntityId entityId, bool isDeleteEntity, bool isRemoveEmptyInstance)
    {
        GetEntityInfoMap(); // ensure map is built
        auto entityInfoMapIt = m_entityInfoMap.find(entityId);
        if (entityInfoMapIt != m_entityInfoMap.end())
        {
            if (entityInfoMapIt->second.m_sliceAddress.GetInstance() == nullptr)
            {
                // should be in the entity lists
                auto entityIt = AZStd::find_if(m_entities.begin(), m_entities.end(), [entityId](Entity* entity) -> bool { return entity->GetId() == entityId; });
                if (entityIt != m_entities.end())
                {
                    // RemoveEntity will be invoked again recursively if invoked with isDeleteEntity = true as the deleting the found entity
                    // would cause the Entity destructor to send a ComponentApplicationRequests::RemoveEntity event causing the ComponentApplication
                    // to send an ComponentApplicationEvents::OnEntityRemoved event that is handled by the EntityContext::OnEntityRemoved function which finally
                    // calls this function again with isDeleteEntity = false. Therefore the EntityInfoMap and the container of Entities needs to have the
                    // current entity removed from it before deleting it
                    AZStd::unique_ptr<AZ::Entity> entityToDelete(isDeleteEntity ? *entityIt : nullptr);
                    m_dataFlagsForNewEntities.ClearEntityDataFlags(entityId);
                    m_entityInfoMap.erase(entityInfoMapIt);
                    m_entities.erase(entityIt);
                    return true;
                }
            }
            else
            {
                SliceReference* sliceReference = entityInfoMapIt->second.m_sliceAddress.GetReference();
                SliceInstance* sliceInstance = entityInfoMapIt->second.m_sliceAddress.GetInstance();

                if (sliceReference->RemoveEntity(entityId, isDeleteEntity, sliceInstance))
                {
                    if (isRemoveEmptyInstance)
                    {
                        if (sliceInstance->m_instantiated->m_entities.empty())
                        {
                            RemoveSliceInstance(sliceInstance);
                        }
                    }

                    m_entityInfoMap.erase(entityInfoMapIt);
                    return true;
                }
            }
        }

        return false;
    }

    bool SliceComponent::RemoveMetaDataEntity(EntityId metaDataEntityId)
    {
        AZ_PROFILE_FUNCTION(AzCore);

        GetEntityInfoMap(); // Ensure map is built

        // We can only delete meta data entities owned by this slice component
        auto metaDataEntityInfoMapIt = m_metaDataEntityInfoMap.find(metaDataEntityId);
        if (metaDataEntityInfoMapIt == m_metaDataEntityInfoMap.end())
        {
            AZ_Warning("SliceComponent::RemoveMetaDataEntity",
                false,
                "Attempted to remove a MetaData Entity with Id: %s but could not find it in the calling SliceComponent's Meta Data Entity Info Map",
                metaDataEntityId.ToString().c_str());

            return false;
        }

        AZ::SliceComponent::SliceInstanceAddress& owningSlice = metaDataEntityInfoMapIt->second.m_sliceAddress;
        AZ::SliceComponent::SliceInstance* owningSliceInstance = owningSlice.GetInstance();

        // Validate the info map's slice instance entry
        if (!owningSliceInstance)
        {
            AZ_Assert(false,
                "SliceComponent::RemoveMetaDataEntity: Attempted to remove a MetaData Entity with Id %s but no associated slice instance found in the calling SliceComponent's MetaData Entity Info Map",
                metaDataEntityId.ToString().c_str());

            return false;
        }

        AZ::SliceComponent::InstantiatedContainer* owningSliceInstantiatedContainer = owningSlice.GetInstance()->m_instantiated;

        // Validate slice instance has instantiated entities
        if (!owningSliceInstantiatedContainer)
        {
            AZ_Error("SliceComponent::RemoveMetaDataEntity",
                false,
                "Attempted to remove a MetaData Entity with Id: %s who's associated instance contains an invalid InstantiatedContainer. Unable to proceed in removing MetaData Entity from associated instance",
                metaDataEntityId.ToString().c_str());

            return false;
        }

        // Confirm that the slice instance's m_metadataEntities contains the meta data entity we wish to remove
        AZ::SliceComponent::EntityList& owningSliceMetaDataEntities = owningSliceInstantiatedContainer->m_metadataEntities;
        auto metaDataEntityIt = AZStd::find_if(owningSliceMetaDataEntities.begin(), owningSliceMetaDataEntities.end(),
            [metaDataEntityId](AZ::Entity* metaDataEntity) -> bool
            {
                AZ_Assert(metaDataEntity, "Attempted to find MetaData Entity with Id: %s in its owning slice's InstantiatedContainer and came across a nullptr MetaData Entity. The owning slice instance is in an invalid state",
                    metaDataEntityId.ToString().c_str());

                return metaDataEntity && (metaDataEntity->GetId() == metaDataEntityId);
            });

        if (metaDataEntityIt == owningSliceMetaDataEntities.end())
        {
            AZ_Error("SliceComponent::RemoveMetaDataEntity",
                false,
                "Attempted to remove a MetaData Entity with Id: %s but it could not be found in the associated slice instance in the calling SliceComponent's MetaData Entity Info Map",
                metaDataEntityId.ToString().c_str());

            return false;
        }

        AZ::Entity* metaDataEntity = *metaDataEntityIt;
        owningSliceMetaDataEntities.erase(metaDataEntityIt);

        delete metaDataEntity;

        // Signal the meta data entity context to remove this entity
        if (metaDataEntity->GetState() == AZ::Entity::State::Active)
        {
            SliceInstanceNotificationBus::Broadcast(&SliceInstanceNotificationBus::Events::OnMetadataEntityDestroyed, metaDataEntity->GetId());
        }

        // Rebake our lookup maps since we've removed an entry
        owningSliceInstance->BuildReverseLookUp();

        auto entityToBaseIt = owningSliceInstance->m_entityIdToBaseCache.find(metaDataEntityId);
        bool entityToBaseExists = entityToBaseIt != owningSliceInstance->m_entityIdToBaseCache.end();

        if (entityToBaseExists)
        {
            owningSliceInstance->m_baseToNewEntityIdMap.erase(entityToBaseIt->second);
            owningSliceInstance->m_entityIdToBaseCache.erase(entityToBaseIt);
        }

        m_metaDataEntityInfoMap.erase(metaDataEntityInfoMapIt);

        return true;
    }

    void SliceComponent::RemoveAllEntities(bool deleteEntities, bool removeEmptyInstances)
    {
        AZ_PROFILE_FUNCTION(AzCore);

        // If we are deleting the entities, we need to do that one by one
        if (deleteEntities)
        {
            for (auto entityIt = m_entities.rbegin(); entityIt != m_entities.rend(); ++entityIt)
            {
                delete *entityIt;
            }
        }

        m_entities.clear();

        // Need to use the m_entityInfoMap to find and handle all of the instances, then we can clear it
        for (auto entityInfo : m_entityInfoMap)
        {
            if (entityInfo.second.m_sliceAddress.GetInstance() == nullptr)
            {
                continue;
            }

            SliceReference* sliceReference = entityInfo.second.m_sliceAddress.GetReference();
            SliceInstance* sliceInstance = entityInfo.second.m_sliceAddress.GetInstance();

            if (sliceReference->RemoveEntity(entityInfo.first, deleteEntities, sliceInstance)
                && removeEmptyInstances
                && sliceInstance->m_instantiated->m_entities.empty())
            {
                RemoveSliceInstance(sliceInstance);
            }
        }

        m_entityInfoMap.clear();
    }

    //=========================================================================
    // SliceComponent::RemoveLooseEntity
    //=========================================================================
    void SliceComponent::RemoveLooseEntity(EntityId entityId)
    {
        auto entityIt = AZStd::find_if(
            m_entities.begin(), 
            m_entities.end(), 
            [entityId](Entity* entity) -> bool { return entity->GetId() == entityId; });

        if (entityIt != m_entities.end())
        {
            m_entities.erase(entityIt);
            // Clearing EntityInfoMap will force it to be regenerated
            m_entityInfoMap.clear();
        }
    }

    //=========================================================================
    // SliceComponent::FindEntity
    //=========================================================================
    AZ::Entity* SliceComponent::FindEntity(EntityId entityId)
    {
        const EntityInfoMap& entityInfoMap = GetEntityInfoMap();
        auto entityInfoMapIt = entityInfoMap.find(entityId);
        if (entityInfoMapIt != entityInfoMap.end())
        {
            return entityInfoMapIt->second.m_entity;
        }

        return nullptr;
    }

    //=========================================================================
    // SliceComponent::FindSlice
    //=========================================================================
    SliceComponent::SliceInstanceAddress SliceComponent::FindSlice(Entity* entity)
    {
        // if we have valid entity pointer and we are instantiated (if we have not instantiated the entities, there is no way
        // to have a pointer to it... that pointer belongs somewhere else).
        if (entity && m_slicesAreInstantiated)
        {
            return FindSlice(entity->GetId());
        }

        return SliceInstanceAddress();
    }

    //=========================================================================
    // SliceComponent::FindSlice
    //=========================================================================
    SliceComponent::SliceInstanceAddress SliceComponent::FindSlice(EntityId entityId)
    {
        if (entityId.IsValid())
        {
            const EntityInfoMap& entityInfo = GetEntityInfoMap();
            auto entityInfoIter = entityInfo.find(entityId);
            if (entityInfoIter != entityInfo.end())
            {
                return entityInfoIter->second.m_sliceAddress;
            }
        }

        return SliceInstanceAddress();
    }

    bool SliceComponent::FlattenSlice(SliceComponent::SliceReference* toFlatten, const EntityId& toFlattenRoot)
    {
        // Sanity check that we're operating on a reference we own
        if (!toFlatten || toFlatten->GetSliceComponent() != this)
        {
            AZ_Warning("Slice", false, "Slice Component attempted to flatten a Slice Reference it doesn't own");
            return false;
        }

        // Sanity check that the root entity is owned by the reference
        SliceInstanceAddress rootSlice = FindSlice(toFlattenRoot);
        if (!rootSlice.IsValid() || rootSlice.GetReference() != toFlatten)
        {
            AZ_Warning("Slice", false, "Attempted to flatten Slice Reference using a root entity it doesn't own");
            return false;
        }

        // Get slice path for error printouts
        AZStd::string slicePath;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(
            slicePath,
            &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById,
            toFlatten->GetSliceAsset().GetId());

        // Our shared ancestry is the ancestry of the root entity in the slice we are flattening
        EntityAncestorList sharedAncestry;
        toFlatten->GetInstanceEntityAncestry(toFlattenRoot, sharedAncestry);

        // Walk through each instance's instantiated entities
        // Since they are instantiated they will already be data patched
        // Each entity will fall under 4 catagories:
        // 1. The root entity
        // 2. An entity owned directly by the slice instance
        // 3. A subslice instance root entity
        // 4. An entity owned by a subslice instance root entity
        for (SliceInstance& instance : toFlatten->m_instances)
        {
            // Make a copy of our instantiated entities so we can safely remove them
            // from the source container while iterating
            EntityList instantiatedEntityList = instance.m_instantiated->m_entities;
            for (AZ::Entity* entity : instantiatedEntityList)
            {
                if (!entity)
                {
                    AZ_Error("Slice", false , "Null entity found in a slice instance of %s in FlattenSlice", slicePath.c_str());
                    return false;
                }
                // Gather the ancestry of each entity to compare against the shared ancestry
                EntityAncestorList entityAncestors;
                toFlatten->GetInstanceEntityAncestry(entity->GetId(), entityAncestors);

                // Get rid of the first element as all entities are guaranteed to share it
                AZStd::vector<SliceInstanceAddress> ancestorList;
                entityAncestors.erase(entityAncestors.begin());

                // Walk the entities ancestry to confirm which of the 4 cases the entity is
                bool addAsEntity = true;
                for (const Ancestor& ancestor : entityAncestors)
                {
                    // Keep track of each ancestor in preperation for cloning a subslice
                    ancestorList.push_back(ancestor.m_sliceAddress);

                    // Check if the entity's ancestor is shared with the root
                    AZ::Data::Asset<SliceAsset> ancestorAsset = ancestor.m_sliceAddress.GetReference()->GetSliceAsset();
                    auto isShared = AZStd::find_if(sharedAncestry.begin(), sharedAncestry.end(),
                        [ancestorAsset](const Ancestor& sharedAncestor) 
                        {
                            return sharedAncestor.m_sliceAddress.GetReference()->GetSliceAsset() == ancestorAsset;
                        });

                    // If it is keep checking
                    if (isShared != sharedAncestry.end())
                    {
                        continue;
                    }

                    // If it isn't we've found a subslice entity
                    if (ancestor.m_entity)
                    {
                        AZ::Entity::State originalState = ancestor.m_entity->GetState();

                        // Quickly activate the ancestral entity so we can get its transform data
                        if (originalState == AZ::Entity::State::Constructed)
                        {
                            ancestor.m_entity->Init();
                        }

                        if (ancestor.m_entity->GetState() == AZ::Entity::State::Init)
                        {
                            ancestor.m_entity->Activate();
                        }

                        if (ancestor.m_entity->GetState() != AZ::Entity::State::Active)
                        {
                            AZ_Error("Slice", false, "Could not activate entity with id %s during FlattenSlice", ancestor.m_entity->GetId().ToString().c_str());
                            return false;
                        }

                        // If the ancestor's entity doesn't have a valid parent we've found
                        // Case 3 a subslice root entity
                        AZ::EntityId parentId;
                        AZ::TransformBus::EventResult(parentId, ancestor.m_entity->GetId(), &AZ::TransformBus::Events::GetParentId);
                        if (!parentId.IsValid())
                        {
                            // Acquire the subslice instance and promote it from an instance in our toFlatten reference
                            // into a reference owned directly by the Slice component
                            SliceInstanceAddress owningSlice = FindSlice(entity);
                            CloneAndAddSubSliceInstance(owningSlice.GetInstance(), ancestorList, ancestor.m_sliceAddress, nullptr, true);
                        }

                        // else if we have a valid parent we are case 4 an entity owned by a subslice root
                        // We will be included in the subslice root's clone and can be skipped

                        // Deactivate the entity if we started that way
                        if (originalState != AZ::Entity::State::Active && originalState != AZ::Entity::State::Activating)
                        {
                            ancestor.m_entity->Deactivate();
                        }
                    }

                    // We've either added a subslice root or skipped an entity owned by one
                    // We should not add it as a plain entity
                    addAsEntity = false;
                    break;
                }

                // If all our ancestry is shared with the root then we're directly owned by it
                // We can add it directly to the slice component
                if (addAsEntity)
                {
                    // Remove the entity non-destructively from our reference
                    // and add it directly to the slice component
                    toFlatten->RemoveEntity(entity->GetId(), false, &instance);
                    AddEntity(entity);

                    // Register entity with this components metadata
                    SliceMetadataInfoComponent* metadataComponent = m_metadataEntity.FindComponent<SliceMetadataInfoComponent>();
                    if (!metadataComponent)
                    {
                        AZ_Error("Slice", false, "Attempted to add entity to Slice Component with no Metadata component");
                        return false;
                    }
                    metadataComponent->AddAssociatedEntity(entity->GetId());
                }
            }
        }

        // All entities have been promoted directly into the Slice component
        // Removing it will remove our dependency on it and its ancestors
        if (!RemoveSlice(toFlatten))
        {
            AZ_Error("Slice", false, "Failed to remove flattened reference of slice %s from Slice Component", slicePath.c_str());
        }

        CleanMetadataAssociations();
        return true;
    }
    //=========================================================================
    // SliceComponent::GetEntityInfoMap
    //=========================================================================
    const SliceComponent::EntityInfoMap& SliceComponent::GetEntityInfoMap() const
    {
        AZ_Assert(IsInstantiated() || IsAllowPartialInstantiation(), "GetEntityInfoMap() should only be called on an instantiated or at least a partially initialized slice.");

        if (m_entityInfoMap.empty())
        {
            // Don't build the entity info map until it's queried.
            const_cast<SliceComponent*>(this)->BuildEntityInfoMap();
        }

        return m_entityInfoMap;
    }

    //=========================================================================
    // SliceComponent::ListenForAssetChanges
    //=========================================================================
    void SliceComponent::ListenForAssetChanges()
    {
        if (!m_serializeContext)
        {
            // use the default app serialize context
            EBUS_EVENT_RESULT(m_serializeContext, ComponentApplicationBus, GetSerializeContext);
            if (!m_serializeContext)
            {
                AZ_Error("Slices", false, "SliceComponent: No serialize context provided! Failed to get component application default serialize context! ComponentApp is not started or SliceComponent serialize context should not be null!");
            }
        }

        // Listen for asset events and set reference to myself
        for (SliceReference& slice : m_slices)
        {
            slice.m_component = this;
            Data::AssetBus::MultiHandler::BusConnect(slice.m_asset.GetId());
        }
    }

    //=========================================================================
    // SliceComponent::ListenForDependentAssetChanges
    //=========================================================================
    void SliceComponent::ListenForDependentAssetChanges()
    {
        if (!m_serializeContext)
        {
            // use the default app serialize context
            ComponentApplicationBus::BroadcastResult(m_serializeContext, &ComponentApplicationBus::Events::GetSerializeContext);
            if (!m_serializeContext)
            {
                AZ_Error("Slices", false, "SliceComponent: No serialize context provided! Failed to get component application default serialize context! ComponentApp is not started or SliceComponent serialize context should not be null!");
            }
        }

        ListenForAssetChanges();

        // Listen for asset events and set reference to myself
        for (SliceReference& slice : m_slices)
        {
            SliceAsset* referencedSliceAsset = slice.GetSliceAsset().Get();
            SliceComponent* referencedSliceComponent = referencedSliceAsset ? referencedSliceAsset->GetComponent() : nullptr;
            // recursively listen down the slice tree.
            if (!referencedSliceAsset || !referencedSliceComponent)
            {
                AZStd::string mySliceAssetPath;
                if (m_myAsset)
                {
                    AZ::Data::AssetCatalogRequestBus::BroadcastResult(mySliceAssetPath, &AZ::Data::AssetCatalogRequests::GetAssetPathById, m_myAsset->GetId());
                }
                AZStd::string referencedSliceAssetPath;
                if (referencedSliceAsset && slice.GetSliceAsset())
                {
                    AZ::Data::AssetCatalogRequestBus::BroadcastResult(referencedSliceAssetPath, &AZ::Data::AssetCatalogRequests::GetAssetPathById, slice.GetSliceAsset()->GetId());
                }
                AZ_Error("Slices", false, "Slice with asset ID %s and path %s has an invalid slice reference to slice with path %s. The Open 3D Engine editor may be unstable, it is recommended you re-launch the editor.",
                    !m_myAsset ? "invalid asset" : m_myAsset->GetId().ToString<AZStd::string>().c_str(),
                    mySliceAssetPath.empty() ? "invalid path" : mySliceAssetPath.c_str(),
                    referencedSliceAssetPath.empty() ? "invalid path" : referencedSliceAssetPath.c_str());
                continue;
            }
            referencedSliceComponent->ListenForDependentAssetChanges();
        }
    }

    //=========================================================================
    // SliceComponent::Activate
    //=========================================================================
    void SliceComponent::Activate()
    {
        ListenForAssetChanges();
    }

    //=========================================================================
    // SliceComponent::Deactivate
    //=========================================================================
    void SliceComponent::Deactivate()
    {
        Data::AssetBus::MultiHandler::BusDisconnect();
    }

    //=========================================================================
    // SliceComponent::Deactivate
    //=========================================================================
    void SliceComponent::OnAssetReloaded(Data::Asset<Data::AssetData> /*asset*/)
    {
        AZ_PROFILE_FUNCTION(AzCore);

        if (!m_myAsset)
        {
            AZ_Assert(false, "Cannot reload a slice component that is not owned by an asset.");
            return;
        }

        AZ::Data::AssetId myAssetId = m_myAsset->GetId();

        AZStd::string sliceAssetPath;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(sliceAssetPath, &AZ::Data::AssetCatalogRequests::GetAssetPathById, myAssetId);

        if (!sliceAssetPath.empty())
        {
            // reload the slice asset from scratch to avoid the inconsistency between the data in memory and that on disk
            Data::AssetManager::Instance().ReloadAsset(m_myAsset->GetId(), AZ::Data::AssetLoadBehavior::Default);
            return;
        }

        // One of our dependent assets has changed.
        // We need to identify any dependent assets that have changed, since there may be multiple
        // due to the nature of cascaded slice reloading.
        // Because we're about to re-construct our own asset and dispatch the change notification, 
        // we need to handle all pending dependent changes.
        bool dependencyHasChanged = false;
        for (SliceReference& slice : m_slices)
        {
            Data::Asset<SliceAsset> dependentAsset = Data::AssetManager::Instance().FindAsset(slice.m_asset.GetId(), AZ::Data::AssetLoadBehavior::Default);

            if (slice.m_asset.Get() != dependentAsset.Get())
            {
                dependencyHasChanged = true;
                break;
            }
        }

        if (!dependencyHasChanged)
        {
            return;
        }

        // Create new SliceAsset data based on our data. We don't want to update our own
        // data in place, but instead propagate through asset reloading. Otherwise data
        // patches are not maintained properly up the slice dependency chain.
        SliceComponent* updatedAssetComponent = Clone(*m_serializeContext);
        Entity* updatedAssetEntity = aznew Entity();
        updatedAssetEntity->AddComponent(updatedAssetComponent);

        Data::Asset<SliceAsset> updatedAsset(m_myAsset->Clone(), AZ::Data::AssetLoadBehavior::Default);
        updatedAsset.Get()->SetData(updatedAssetEntity, updatedAssetComponent);
        updatedAssetComponent->SetMyAsset(updatedAsset.Get());

        // Update data patches against the old version of the asset.
        updatedAssetComponent->PrepareSave();

        PushInstantiateCycle(myAssetId);

        // Update asset reference for any modified dependencies, and re-instantiate nested instances.
        for (auto listIterator = updatedAssetComponent->m_slices.begin(); listIterator != updatedAssetComponent->m_slices.end();)
        {
            SliceReference& slice = *listIterator;
            Data::Asset<SliceAsset> dependentAsset = Data::AssetManager::Instance().FindAsset(slice.m_asset.GetId(), slice.m_asset.GetAutoLoadBehavior());

            bool recursionDetected = CheckContainsInstantiateCycle(dependentAsset.GetId());
            bool wasModified = (dependentAsset.Get() != slice.m_asset.Get());

            if (wasModified || recursionDetected)
            {
                // uninstantiate if its been instantiated:
                if (slice.m_isInstantiated && !slice.m_instances.empty())
                {
                    for (SliceInstance& instance : slice.m_instances)
                    {
                        delete instance.m_instantiated;
                        instance.m_instantiated = nullptr;
                    }
                    slice.m_isInstantiated = false;
                }
            }

            if (!recursionDetected)
            {
                if (wasModified) // no need to do anything here if it wasn't actually changed.
                {
                    // Asset data differs. Acquire new version and re-instantiate.
                    slice.m_asset = dependentAsset;
                    slice.Instantiate(AZ::ObjectStream::FilterDescriptor(m_assetLoadFilterCB, m_filterFlags));
                }
                ++listIterator;
            }
            else
            {
                // whenever recursion was detected we remove this element.
                listIterator = updatedAssetComponent->m_slices.erase(listIterator);
            }
        }

        PopInstantiateCycle(myAssetId);

        // Rebuild entity info map based on new instantiations.
        RebuildEntityInfoMapIfNecessary();

        // note that this call will destroy the "this" pointer, because the main editor context will respond to this by deleting and recreating all slices
        // (this call will cascade up the tree of inheritence all the way to the root.)
        // for this reason, we disconnect from the busses to prevent recursion.
        Data::AssetBus::MultiHandler::BusDisconnect();
        Data::AssetManager::Instance().ReloadAssetFromData(updatedAsset);
    }

    //=========================================================================
    // SliceComponent::GetProvidedServices
    //=========================================================================
    void SliceComponent::GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("Prefab", 0xa60af5fc));
    }

    //=========================================================================
    // SliceComponent::GetDependentServices
    //=========================================================================
    void SliceComponent::GetDependentServices(ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC("AssetDatabaseService", 0x3abf5601));
    }

    /**
    * \note when we reflecting we can check if the class is inheriting from IEventHandler
    * and use the this->events.
    */
    class SliceComponentSerializationEvents
        : public SerializeContext::IEventHandler
    {
        /// Called right before we start reading from the instance pointed by classPtr.
        void OnReadBegin(void* classPtr) override
        {
            SliceComponent* component = reinterpret_cast<SliceComponent*>(classPtr);
            component->PrepareSave();
        }

        /// Called right after we finish writing data to the instance pointed at by classPtr.
        void OnWriteEnd(void* classPtr) override
        {
            AZ_PROFILE_FUNCTION(AzCore);

            SliceComponent* sliceComponent = reinterpret_cast<SliceComponent*>(classPtr);
            EBUS_EVENT(SliceAssetSerializationNotificationBus, OnWriteDataToSliceAssetEnd, *sliceComponent);

            // Broadcast OnSliceEntitiesLoaded for entities that are "new" to this slice.
            // We can't broadcast this event for instanced entities yet, since they don't exist until instantiation.
            if (!sliceComponent->GetNewEntities().empty())
            {
                AZ_PROFILE_SCOPE(AzCore, "SliceComponentSerializationEvents::OnWriteEnd:OnSliceEntitiesLoaded");
                EBUS_EVENT(SliceAssetSerializationNotificationBus, OnSliceEntitiesLoaded, sliceComponent->GetNewEntities());
            }
        }
    };

    //=========================================================================
    // Reflect
    //=========================================================================
    void SliceComponent::PrepareSave()
    {
        AZ_PROFILE_FUNCTION(AzCore);

        if (m_slicesAreInstantiated)
        {
            // if we had the entities instantiated, recompute the deltas.
            for (SliceReference& slice : m_slices)
            {
                slice.ComputeDataPatch();
            }
        }

        // clean up orphaned data flags (ex: for entities that no longer exist).
        m_dataFlagsForNewEntities.Cleanup(m_entities);
    }

    //=========================================================================
    // InitMetadata
    //=========================================================================
    void SliceComponent::InitMetadata()
    {
        // If we have a new metadata entity, we need to add a SliceMetadataInfoComponent to it and 
        // fill it in with general metadata hierarchy and association data as well as assign a
        // useful name to the entity.
        auto* metadataInfoComponent = m_metadataEntity.FindComponent<SliceMetadataInfoComponent>();
        if (!metadataInfoComponent)
        {
            // If this component is missing, we can assume this is a brand new empty metadata entity that has
            // either come from a newly created slice or loaded from a legacy slice without metadata. We can use
            // this opportunity to give it an appropriate name and a required component. We're adding the component
            // here during the various steps of slice instantiation because it becomes costly and difficult to do
            // so later.
            metadataInfoComponent = m_metadataEntity.CreateComponent<SliceMetadataInfoComponent>();

            // Every instance of another slice referenced by this component has set of metadata entities.
            // Exactly one of those metadata entities in each of those sets is instanced from the slice asset
            // that was used to create the instance. We want only those metadata entities.
            EntityList instanceEntities;
            GetInstanceMetadataEntities(instanceEntities);

            for (auto& instanceMetadataEntity : instanceEntities)
            {
                // Add hierarchy information to our new info component
                metadataInfoComponent->AddChildMetadataEntity(instanceMetadataEntity->GetId());

                auto* instanceInfoComponent = instanceMetadataEntity->FindComponent<SliceMetadataInfoComponent>();
                AZ_Assert(instanceInfoComponent, "Instanced entity must have a metadata component.");
                if (instanceInfoComponent)
                {
                    metadataInfoComponent->SetParentMetadataEntity(m_metadataEntity.GetId());
                }
            }

            // All new entities in the current slice need to be associated with the slice's metadata entity
            for (auto& entity : m_entities)
            {
                metadataInfoComponent->AddAssociatedEntity(entity->GetId());
            }
        }

        // Finally, clean up all the metadata associations belonging to our instances
        // We need to do this every time we initialize a slice because we can't know if one of our referenced slices has changed
        // since our last initialization.
        CleanMetadataAssociations();
    }

    void SliceComponent::RemoveAndCacheInstances(const SliceReferenceToInstancePtrs& instancesToRemove)
    {
        AZ_Assert(m_cachedSliceInstances.size() == 0, "Stale layer instances are cached. Your scene is in an invalid state.");
        m_cachedSliceInstances.clear();
        for (const auto& refToInstanceSet : instancesToRemove)
        {
            for (SliceInstance* instancePtr : refToInstanceSet.second)
            {
                refToInstanceSet.first->RemoveInstanceFromEntityInfoMap(*instancePtr);
                m_cachedSliceInstances[refToInstanceSet.first->GetSliceAsset()].insert(*instancePtr);
                // The instantiated list is now tracked by the cached data, make sure it's null so garbage data isn't deleted in the erase
                // from the m_instances list.
                instancePtr->m_instantiated = nullptr;
                refToInstanceSet.first->m_instances.erase(*instancePtr);
                // If the last instance was removed, then don't save the slice reference in the level.
                if (refToInstanceSet.first->GetInstances().empty())
                {
                    for (SliceList::iterator sliceIter = m_slices.begin(); sliceIter != m_slices.end(); ++sliceIter)
                    {
                        if (&(*sliceIter) == refToInstanceSet.first)
                        {
                            m_cachedSliceReferences.emplace_back(AZStd::move(*sliceIter));
                            m_slices.erase(sliceIter);
                            break;
                        }
                    }
                }
            }
        }
        
    }

    void SliceComponent::RestoreCachedInstances()
    {
        if (!m_cachedSliceReferences.empty())
        {
            for (SliceList::iterator sliceIter = m_cachedSliceReferences.begin(); sliceIter != m_cachedSliceReferences.end(); ++sliceIter)
            {
                m_slices.emplace_back(AZStd::move(*sliceIter));
            }
            m_cachedSliceReferences.clear();
        }

        for (auto& refToInstances : m_cachedSliceInstances)
        {
            for (SliceReference& slice : m_slices)
            {
                if (slice.GetSliceAsset() != refToInstances.first)
                {
                    continue;
                }
                for (SliceInstance& instance : refToInstances.second)
                {
                    slice.RestoreAndClearCachedInstance(instance);
                }
                break;
            }
            refToInstances.second.clear();
        }
        m_cachedSliceInstances.clear();
    }

    void SliceComponent::CleanMetadataAssociations()
    {
        // Now we need to get all of the metadata entities that belong to all of our slice instances.
        EntityList instancedMetadataEntities;
        GetAllInstanceMetadataEntities(instancedMetadataEntities);

        // Easy out - No instances means no associations to fix
        if (!instancedMetadataEntities.empty())
        {
            // We need to check every associated entity in every instance to see if it wasn't removed by our data patch.
            // First, Turn the list of all entities belonging to this component into a set of IDs to reduce find times.
            EntityIdSet entityIds;
            GetEntityIds(entityIds);

            // Now we need to get the metadata info component for each of these entities and get the list of their associated entities.
            for (auto& instancedMetadataEntity : instancedMetadataEntities)
            {
                auto* instancedInfoComponent = instancedMetadataEntity->FindComponent<SliceMetadataInfoComponent>();
                AZ_Assert(instancedInfoComponent, "Instanced metadata entities MUST have metadata info components.");
                if (instancedInfoComponent)
                {
                    // Copy the set of associated IDs to iterate through to avoid invalidating our iterator as we check each and
                    // remove it from the info component if it wasn't cloned into this slice.
                    AZStd::set<AZ::EntityId> associatedEntityIds;
                    instancedInfoComponent->GetAssociatedEntities(associatedEntityIds);
                    for (const auto& entityId : associatedEntityIds)
                    {
                        if (entityIds.find(entityId) == entityIds.end())
                        {
                            instancedInfoComponent->RemoveAssociatedEntity(entityId);
                        }
                    }
                }
            }
        }
    }

    //=========================================================================
    // SliceComponent::BuildEntityInfoMap
    //=========================================================================
    void SliceComponent::BuildEntityInfoMap()
    {
        AZ_PROFILE_FUNCTION(AzCore);

        m_entityInfoMap.clear();
        m_metaDataEntityInfoMap.clear();

        for (Entity* entity : m_entities)
        {
            m_entityInfoMap.insert(AZStd::make_pair(entity->GetId(), EntityInfo(entity, SliceInstanceAddress())));
        }

        for (SliceReference& slice : m_slices)
        {
            for (SliceInstance& instance : slice.m_instances)
            {
                slice.AddInstanceToEntityInfoMap(instance);
            }
        }
    }

    //=========================================================================
    // SliceComponent::RebuildEntityInfoMapIfNecessary
    //=========================================================================
    void SliceComponent::RebuildEntityInfoMapIfNecessary()
    {
        if (!m_entityInfoMap.empty())
        {
            BuildEntityInfoMap();
        }
    }

    //=========================================================================
    // ApplyEntityMapId
    //=========================================================================
    void SliceComponent::ApplyEntityMapId(EntityIdToEntityIdMap& destination, const EntityIdToEntityIdMap& remap)
    {
        // apply the instance entityIdMap on the top of the base
        for (const auto& entityIdPair : remap)
        {
            auto iteratorBoolPair = destination.insert(entityIdPair);
            if (!iteratorBoolPair.second)
            {
                // if not inserted just overwrite the value
                iteratorBoolPair.first->second = entityIdPair.second;
            }
        }
    }

    //=========================================================================
    // PushInstantiateCycle
    //=========================================================================
    void SliceComponent::PushInstantiateCycle(const AZ::Data::AssetId& assetId)
    {
        AZStd::unique_lock<AZStd::recursive_mutex> lock(m_instantiateMutex);
        m_instantiateCycleChecker.push_back(assetId);
    }

    //=========================================================================
    // ContainsInstantiateCycle
    //=========================================================================
    bool SliceComponent::CheckContainsInstantiateCycle(const AZ::Data::AssetId& assetId)
    {
        /* this function is called during recursive slice instantiation to check for cycles.
        /  It is assumed that before recursing, PushInstantiateCycle is called, 
        /  and when recursion returns, PopInstantiateCycle is called.
        /  we want to make sure we are never in the following situation:
        /     SliceA
        /        + SliceB
        /           + SliceA (again - cyclic!)
        /  note that its safe for the same asset to pop up a few times in different branches of the instantiation
        /  hierarchy, just not within the same direct line as its parents
        */

        AZStd::unique_lock<AZStd::recursive_mutex> lock(m_instantiateMutex);
        if (AZStd::find(m_instantiateCycleChecker.begin(), m_instantiateCycleChecker.end(), assetId) != m_instantiateCycleChecker.end())
        {
   
#if defined(AZ_ENABLE_TRACING)
            AZ::Data::Asset<SliceAsset> fullAsset = AZ::Data::AssetManager::Instance().FindAsset(assetId, AZ::Data::AssetLoadBehavior::Default);
            AZStd::string messageString = "Cyclic Slice references detected!  Please see the hierarchy below:";
            for (const AZ::Data::AssetId& assetListElement : m_instantiateCycleChecker)
            {
                Data::Asset<SliceAsset> callerAsset = Data::AssetManager::Instance().FindAsset(assetListElement, AZ::Data::AssetLoadBehavior::Default);
                if (callerAsset.GetId() == assetId)
                {
                    messageString.append("\n --> "); // highlight the ones that are the problem!
                }
                else
                {
                    messageString.append("\n     ");
                }
                messageString.append(callerAsset.ToString<AZStd::string>().c_str());
            }
            // put the one that we are currently checking for at the end of the list for easy readability:
            messageString.append("\n --> ");
            messageString.append(fullAsset.ToString<AZStd::string>().c_str());
            AZ_Error("Slice", false, "%s", messageString.c_str());
#endif // AZ_ENABLE_TRACING

            return true;
        }

        return false;
    }

    //=========================================================================
    // PopInstantiateCycle
    //=========================================================================
    void SliceComponent::PopInstantiateCycle(const AZ::Data::AssetId& assetId)
    {
#if defined(AZ_ENABLE_TRACING)
        AZ_Assert(m_instantiateCycleChecker.back() == assetId, "Programmer error - Cycle-Checking vector should be symmetrically pushing and popping elements.");
#else
        AZ_UNUSED(assetId);
#endif
        m_instantiateCycleChecker.pop_back();
        if (m_instantiateCycleChecker.empty())
        {
            // reclaim memory so that there is no leak of this static object.
            m_instantiateCycleChecker.set_capacity(0);
        }
    }

    //=========================================================================
    // SliceComponent::AddOrGetSliceReference
    //=========================================================================
    SliceComponent::SliceReference* SliceComponent::AddOrGetSliceReference(const Data::Asset<SliceAsset>& sliceAsset)
    {
        SliceReference* reference = GetSlice(sliceAsset.GetId());
        if (!reference)
        {
            Data::AssetBus::MultiHandler::BusConnect(sliceAsset.GetId());
            m_slices.push_back();
            reference = &m_slices.back();

            auto loadBehavior = reference->m_asset.GetAutoLoadBehavior();

            reference->m_component = this;
            reference->m_asset = sliceAsset;
            reference->m_isInstantiated = m_slicesAreInstantiated;

            reference->m_asset.SetAutoLoadBehavior(loadBehavior);
        }

        return reference;
    }

    //=========================================================================
    const SliceComponent::DataFlagsPerEntity& SliceComponent::GetDataFlagsForInstances() const
    {
        // DataFlags can affect how DataPaches are applied when a slice is instantiated.
        // We need to collect the DataFlags of each entity's entire ancestry.
        // We cache these flags in the slice to avoid recursing the ancestry each time that slice is instanced.
        // The cache is generated the first time it's requested.
        if (!m_hasGeneratedCachedDataFlags)
        {
            const_cast<SliceComponent*>(this)->BuildDataFlagsForInstances();
        }
        return m_cachedDataFlagsForInstances;
    }

    //=========================================================================
    void SliceComponent::BuildDataFlagsForInstances()
    {
        AZ_PROFILE_FUNCTION(AzCore);
        AZ_Assert(IsInstantiated(), "Slice must be instantiated before the ancestry of its data flags can be calculated.");

        // Use lock since slice instantiation can occur from multiple threads
        AZStd::unique_lock<AZStd::recursive_mutex> lock(m_instantiateMutex);
        if (m_hasGeneratedCachedDataFlags)
        {
            return;
        }

        // Collect data flags from entities originating in this slice.
        m_cachedDataFlagsForInstances.CopyDataFlagsFrom(m_dataFlagsForNewEntities);

        // Collect data flags from each slice that's instantiated within this slice...
        for (const SliceReference& sliceReference : m_slices)
        {
            for (const SliceInstance& sliceInstance : sliceReference.m_instances)
            {
                // Collect data flags from the entire ancestry by incorporating the cached data flags
                // from the SliceComponent that this instance was instantiated from.
                m_cachedDataFlagsForInstances.ImportDataFlagsFrom(
                    sliceReference.GetSliceAsset().Get()->GetComponent()->GetDataFlagsForInstances(),
                    &sliceInstance.GetEntityIdMap(),
                    DataPatch::GetEffectOfSourceFlagsOnThisAddress);

                // Collect data flags unique to this instance
                m_cachedDataFlagsForInstances.ImportDataFlagsFrom(
                    sliceInstance.GetDataFlags(),
                    &sliceInstance.GetEntityIdMap());
            }
        }

        m_hasGeneratedCachedDataFlags = true;
    }

    //=========================================================================
    const SliceComponent::DataFlagsPerEntity* SliceComponent::GetCorrectBundleOfDataFlags(EntityId entityId) const
    {
        // It would be possible to search non-instantiated slices by crawling over lists, but we haven't needed the capability yet.
        AZ_Assert(IsInstantiated(), "Data flag access is only permitted after slice is instantiated.");

        if (IsInstantiated())
        {
            // Slice is instantiated, we can use EntityInfoMap
            const EntityInfoMap& entityInfoMap = GetEntityInfoMap();
            auto foundEntityInfo = entityInfoMap.find(entityId);
            if (foundEntityInfo != entityInfoMap.end())
            {
                const EntityInfo& entityInfo = foundEntityInfo->second;
                if (const SliceInstance* entitySliceInstance = entityInfo.m_sliceAddress.GetInstance())
                {
                    return &entitySliceInstance->GetDataFlags();
                }
                else
                {
                    return &m_dataFlagsForNewEntities;
                }
            }
        }

        return nullptr;
    }

    //=========================================================================
    SliceComponent::DataFlagsPerEntity* SliceComponent::GetCorrectBundleOfDataFlags(EntityId entityId)
    {
        // call const version of same function
        return const_cast<DataFlagsPerEntity*>(const_cast<const SliceComponent*>(this)->GetCorrectBundleOfDataFlags(entityId));
    }

    //=========================================================================
    bool SliceComponent::IsNewEntity(EntityId entityId) const
    {
        // return true if this entity is not based on an entity from another slice
        if (IsInstantiated())
        {
            // Slice is instantiated, use the entity info map
            const EntityInfoMap& entityInfoMap = GetEntityInfoMap();
            auto foundEntityInfo = entityInfoMap.find(entityId);
            if (foundEntityInfo != entityInfoMap.end())
            {
                return !foundEntityInfo->second.m_sliceAddress.IsValid();
            }
        }
        else
        {
            // Slice is not instantiated, crawl over list of new entities
            for (Entity* newEntity : GetNewEntities())
            {
                if (newEntity->GetId() == entityId)
                {
                    return true;
                }
            }
        }

        return false;
    }

    //=========================================================================
    const DataPatch::FlagsMap& SliceComponent::GetEntityDataFlags(EntityId entityId) const
    {
        if (const DataFlagsPerEntity* dataFlagsPerEntity = GetCorrectBundleOfDataFlags(entityId))
        {
            return dataFlagsPerEntity->GetEntityDataFlags(entityId);
        }

        static const DataPatch::FlagsMap emptyFlagsMap;
        return emptyFlagsMap;
    }

    //=========================================================================
    bool SliceComponent::SetEntityDataFlags(EntityId entityId, const DataPatch::FlagsMap& dataFlags)
    {
        if (DataFlagsPerEntity* dataFlagsPerEntity = GetCorrectBundleOfDataFlags(entityId))
        {
            return dataFlagsPerEntity->SetEntityDataFlags(entityId, dataFlags);
        }
        return false;
    }

    //=========================================================================
    DataPatch::Flags SliceComponent::GetEffectOfEntityDataFlagsAtAddress(EntityId entityId, const DataPatch::AddressType& dataAddress) const
    {
        // if this function is a performance bottleneck, it could be optimized with caching
        // be wary not to create the cache in-game if the information is only needed by tools
        AZ_PROFILE_FUNCTION(AzCore);

        if (!IsInstantiated())
        {
            AZ_Assert(false, "Data flag access is only permitted after slice is instantiated.");
            return 0;
        }

        // look up entity
        const EntityInfoMap& entityInfoMap = GetEntityInfoMap();
        auto foundEntityInfo = entityInfoMap.find(entityId);
        if (foundEntityInfo == entityInfoMap.end())
        {
            return 0;
        }

        const EntityInfo& entityInfo = foundEntityInfo->second;

        // Get data flags for this instance of the entity.
        // If this entity is based on another slice, get the data flags from that slice as well.
        const DataFlagsPerEntity* dataFlagsForEntity = nullptr;
        const DataFlagsPerEntity* dataFlagsForBaseEntity = nullptr;
        EntityId baseEntityId;

        if (const SliceInstance* entitySliceInstance = entityInfo.m_sliceAddress.GetInstance())
        {
            dataFlagsForEntity = &entitySliceInstance->GetDataFlags();

            const EntityIdToEntityIdMap& entityIdToBaseMap = entitySliceInstance->GetEntityIdToBaseMap();
            auto foundBaseEntity = entityIdToBaseMap.find(entityId);
            if (foundBaseEntity != entityIdToBaseMap.end())
            {
                baseEntityId = foundBaseEntity->second;
                dataFlagsForBaseEntity = &entityInfo.m_sliceAddress.GetReference()->GetSliceAsset().Get()->GetComponent()->GetDataFlagsForInstances();
            }
        }
        else
        {
            dataFlagsForEntity = &m_dataFlagsForNewEntities;
        }

        // To calculate the full effect of data flags on an address,
        // we need to compile flags from all parent addresses, and all ancestor addresses.
        DataPatch::Flags flags = 0;

        // iterate through address, beginning with the "blank" initial address
        DataPatch::AddressType address;
        address.reserve(dataAddress.size());
        DataPatch::AddressType::const_iterator addressIterator = dataAddress.begin();
        while(true)
        {
            // functionality is identical to DataPatch::CalculateDataFlagAtThisAddress(...)
            // but this gets data from DataFlagsPerEntity instead of DataPatch::FlagsMap
            if (flags)
            {
                flags |= DataPatch::GetEffectOfParentFlagsOnThisAddress(flags);
            }

            if (dataFlagsForBaseEntity)
            {
                if (DataPatch::Flags baseEntityFlags = dataFlagsForBaseEntity->GetEntityDataFlagsAtAddress(baseEntityId, address))
                {
                    flags |= DataPatch::GetEffectOfSourceFlagsOnThisAddress(baseEntityFlags);
                }
            }

            if (DataPatch::Flags entityFlags = dataFlagsForEntity->GetEntityDataFlagsAtAddress(entityId, address))
            {
                flags |= DataPatch::GetEffectOfTargetFlagsOnThisAddress(entityFlags);
            }

            if (addressIterator != dataAddress.end())
            {
                address.push_back(*addressIterator++);
            }
            else
            {
                break;
            }
        }

        return flags;
    }

    //=========================================================================
    DataPatch::Flags SliceComponent::GetEntityDataFlagsAtAddress(EntityId entityId, const DataPatch::AddressType& dataAddress) const
    {
        if (const DataFlagsPerEntity* dataFlagsPerEntity = GetCorrectBundleOfDataFlags(entityId))
        {
            return dataFlagsPerEntity->GetEntityDataFlagsAtAddress(entityId, dataAddress);
        }
        return 0;
    }

    //=========================================================================
    bool SliceComponent::SetEntityDataFlagsAtAddress(EntityId entityId, const DataPatch::AddressType& dataAddress, DataPatch::Flags flags)
    {
        if (DataFlagsPerEntity* dataFlagsPerEntity = GetCorrectBundleOfDataFlags(entityId))
        {
            return dataFlagsPerEntity->SetEntityDataFlagsAtAddress(entityId, dataAddress, flags);
        }
        return false;
    }

    //=========================================================================
    // SliceComponent::GetInstanceMetadataEntities
    //=========================================================================
    bool SliceComponent::GetInstanceMetadataEntities(EntityList& outMetadataEntities)
    {
        bool result = true;

        // make sure we have all entities instantiated
        if (Instantiate() != InstantiateResult::Success)
        {
            result = false;
        }

        // add all entities from base slices
        for (const SliceReference& slice : m_slices)
        {
            for (const SliceInstance& instance : slice.m_instances)
            {
                outMetadataEntities.push_back(instance.GetMetadataEntity());
            }
        }
        return result;
    }

    //=========================================================================
    // SliceComponent::GetAllInstanceMetadataEntities
    //=========================================================================
    bool SliceComponent::GetAllInstanceMetadataEntities(EntityList& outMetadataEntities)
    {
        bool result = true;

        // make sure we have all entities instantiated
        if (Instantiate() != InstantiateResult::Success)
        {
            result = false;
        }

        // add all entities from base slices
        for (const SliceReference& slice : m_slices)
        {
            for (const SliceInstance& instance : slice.m_instances)
            {
                if (instance.m_instantiated)
                {
                    outMetadataEntities.insert(outMetadataEntities.end(), instance.m_instantiated->m_metadataEntities.begin(), instance.m_instantiated->m_metadataEntities.end());
                }
            }
        }

        return result;
    }

    //=========================================================================
    // SliceComponent::GetAllMetadataEntities
    //=========================================================================
    bool SliceComponent::GetAllMetadataEntities(EntityList& outMetadataEntities)
    {
        bool result = GetAllInstanceMetadataEntities(outMetadataEntities);

        // add the entity from this component
        outMetadataEntities.push_back(GetMetadataEntity());

        return result;
    }

    AZ::Entity* SliceComponent::GetMetadataEntity()
    {
        return &m_metadataEntity;
    }

    //=========================================================================
    // Clone
    //=========================================================================
    SliceComponent* SliceComponent::Clone(AZ::SerializeContext& serializeContext, SliceInstanceToSliceInstanceMap* sourceToCloneSliceInstanceMap) const
    {
        AZ_PROFILE_FUNCTION(AzCore);

        SliceComponent* clonedComponent = serializeContext.CloneObject(this);

        if (!clonedComponent)
        {
            AZ_Error("SliceAsset", false, "Failed to clone asset.");
            return nullptr;
        }

        clonedComponent->SetSerializeContext(&serializeContext);

        AZ_Assert(clonedComponent, "Cloned asset doesn't contain a slice component.");
        AZ_Assert(clonedComponent->GetSlices().size() == GetSlices().size(),
            "Cloned asset does not match source asset.");

        const SliceComponent::SliceList& myReferences = m_slices;
        SliceComponent::SliceList& clonedReferences = clonedComponent->m_slices;

        auto myReferencesIt = myReferences.begin();
        auto clonedReferencesIt = clonedReferences.begin();

        // For all slice references, clone instantiated instances.
        for (; myReferencesIt != myReferences.end(); ++myReferencesIt, ++clonedReferencesIt)
        {
            const SliceComponent::SliceReference::SliceInstances& myRefInstances = myReferencesIt->m_instances;
            SliceComponent::SliceReference::SliceInstances& clonedRefInstances = clonedReferencesIt->m_instances;

            AZ_Assert(myRefInstances.size() == clonedRefInstances.size(),
                "Cloned asset reference does not contain the same number of instances as the source asset reference.");

            auto myRefInstancesIt = myRefInstances.begin();
            auto clonedRefInstancesIt = clonedRefInstances.begin();

            for (; myRefInstancesIt != myRefInstances.end(); ++myRefInstancesIt, ++clonedRefInstancesIt)
            {
                const SliceComponent::SliceInstance& myRefInstance = (*myRefInstancesIt);
                SliceComponent::SliceInstance& clonedRefInstance = (*clonedRefInstancesIt);

                if (sourceToCloneSliceInstanceMap)
                {
                    SliceComponent::SliceInstanceAddress sourceAddress(const_cast<SliceComponent::SliceReference*>(&(*myReferencesIt)), const_cast<SliceComponent::SliceInstance*>(&myRefInstance));
                    SliceComponent::SliceInstanceAddress clonedAddress(&(*clonedReferencesIt), &clonedRefInstance);
                    (*sourceToCloneSliceInstanceMap)[sourceAddress] = clonedAddress;
                }

                // Slice instances should not support copying natively, but to clone we copy member-wise
                // and clone instantiated entities.
                clonedRefInstance.m_baseToNewEntityIdMap = myRefInstance.m_baseToNewEntityIdMap;
                clonedRefInstance.m_entityIdToBaseCache = myRefInstance.m_entityIdToBaseCache;
                clonedRefInstance.m_dataPatch = myRefInstance.m_dataPatch;
                clonedRefInstance.m_dataFlags.CopyDataFlagsFrom(myRefInstance.m_dataFlags);
                clonedRefInstance.m_instantiated = myRefInstance.m_instantiated ? serializeContext.CloneObject(myRefInstance.m_instantiated) : nullptr;
            }

            clonedReferencesIt->m_isInstantiated = myReferencesIt->m_isInstantiated;
            clonedReferencesIt->m_asset = myReferencesIt->m_asset;
            clonedReferencesIt->m_component = clonedComponent;
        }

        // Finally, mark the cloned asset as instantiated.
        clonedComponent->m_slicesAreInstantiated = IsInstantiated();

        return clonedComponent;
    }

    void SliceComponent::EraseEntities(const EntityList& entities)
    {
        m_entities.erase(
            AZStd::remove_if(
                m_entities.begin(), m_entities.end(),
                [&entities](const AZ::Entity* entity)
        {
            return AZStd::find(entities.begin(), entities.end(), entity) != entities.end();
        }),
        m_entities.end());
    }

    void SliceComponent::AddEntities(const EntityList& entities)
    {
        m_entities.insert(m_entities.end(), entities.begin(), entities.end());
    }

    void SliceComponent::ReplaceEntities(const EntityList& entities)
    {
        m_entities.clear();
        AddEntities(entities);
    }

    void SliceComponent::AddSliceInstances(SliceAssetToSliceInstancePtrs& sliceInstances, AZStd::unordered_set<const SliceInstance*>& instancesOut)
    {
        // sliceInstances is in a map and can be searched, so iterate through m_slices first.
        for (SliceReference& sliceReference : m_slices)
        {
            SliceAssetToSliceInstancePtrs::iterator instancesForReference = sliceInstances.find(sliceReference.GetSliceAsset());
            if (instancesForReference == sliceInstances.end())
            {
                continue;
            }

            for (SliceInstance* instance : instancesForReference->second)
            {
                // Check if an instance already exists with this ID, if so replace it.
                // We want to minimize lost work in the case of a conflict, and the newer
                // instance is likely to have the latest work.
                auto toErase = AZStd::remove_if(sliceReference.GetInstances().begin(), sliceReference.GetInstances().end(), [&instance, &sliceReference](const SliceInstance& existingInstance)
                {
                    if (instance->GetId() == existingInstance.GetId())
                    {
                        AZ_UNUSED(sliceReference); // Prevent unused warning in release builds
                        AZ_Warning("Slice", false, "Multiple slice instances with the same ID from slice %s were found. The last instance found has been loaded.",
                            sliceReference.GetSliceAsset().GetHint().c_str());
                        return true;
                    }
                    return false;
                });
                sliceReference.GetInstances().erase(toErase, sliceReference.GetInstances().end());
                const auto& emplaceResult = sliceReference.GetInstances().emplace(AZStd::move(*instance));
                // emplace returns a pair, with the first element being an iterator.
                // Convert that iterator to a pointer, so the moved instances can be returned via the output parameter.
                instancesOut.insert(&(*emplaceResult.first));

                if (sliceReference.m_isInstantiated)
                {
                    // If this reference was already instantiated, instantiate the loaded instance.
                    // This may occur when recovering a layer.
                    sliceReference.InstantiateInstance(*emplaceResult.first, AZ::ObjectStream::FilterDescriptor(m_assetLoadFilterCB, m_filterFlags));
                    // If the entity info map has already been populated, then add this new instance to it.
                    // Otherwise, it will be added when the info map is populated later.
                    if (!m_entityInfoMap.empty())
                    {
                        sliceReference.AddInstanceToEntityInfoMap(*emplaceResult.first);
                    }
                }
            }
            instancesForReference->second.clear();
            sliceInstances.erase(instancesForReference);
        }

        // Anything left in the instance list are references that haven't been loaded yet, so load them.
        for (AZStd::pair<Data::Asset<SliceAsset>, SliceInstancePtrSet> refToAdd : sliceInstances)
        {
            m_slices.push_back();
            SliceReference *newReference = &m_slices.back();
            newReference->m_component = this;
            newReference->m_asset = refToAdd.first;

            for (SliceInstance* instance : refToAdd.second)
            {
                // Don't need to force an instance instantiation because a new reference can't already be instantiated.
                const auto& emplaceResult = newReference->GetInstances().emplace(AZStd::move(*instance));
                // emplace returns a pair, with the first element being an iterator.
                // Convert that iterator to a pointer, so the moved instances can be returned via the output parameter.
                instancesOut.insert(&(*emplaceResult.first));
            }
            refToAdd.second.clear();

            // Make sure the reference, and any instances of that reference, are initialized and setup.
            newReference->Instantiate(AZ::ObjectStream::FilterDescriptor(m_assetLoadFilterCB, m_filterFlags));
            // If the entity info map has already been populated, then add this new instance to it.
            // Otherwise, it will be added when the info map is populated later.
            if (!m_entityInfoMap.empty())
            {
                for (auto& instance : newReference->GetInstances())
                {
                    newReference->AddInstanceToEntityInfoMap(instance);
                }
            }
        }
        // Clear out the passed in instance map, it has invalid data now after the move calls.
        sliceInstances.clear();
    }

    //=========================================================================
    // Reflect
    //=========================================================================
    void SliceComponent::Reflect(ReflectContext* reflection)
    {
        DataFlagsPerEntity::Reflect(reflection);

        SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<SliceComponent, Component>()->
                Version(3)->
                EventHandler<SliceComponentSerializationEvents>()->
                Field("Entities", &SliceComponent::m_entities)->
                Field("Prefabs", &SliceComponent::m_slices)->
                Field("IsDynamic", &SliceComponent::m_isDynamic)->
                Field("MetadataEntity", &SliceComponent::m_metadataEntity)->
                Field("DataFlagsForNewEntities", &SliceComponent::m_dataFlagsForNewEntities); // added at v3

            serializeContext->Class<InstantiatedContainer>()->
                Version(2)->
                Field("Entities", &InstantiatedContainer::m_entities)->
                Field("MetadataEntities", &InstantiatedContainer::m_metadataEntities);

            serializeContext->Class<SliceInstance>()->
                Version(3)->
                Field("Id", &SliceInstance::m_instanceId)->
                Field("EntityIdMap", &SliceInstance::m_baseToNewEntityIdMap)->
                Field("DataPatch", &SliceInstance::m_dataPatch)->
                Field("DataFlags", &SliceInstance::m_dataFlags); // added at v3

            serializeContext->Class<SliceReference>()->
                Version(2, &Converters::SliceReferenceVersionConverter)->
                Field("Instances", &SliceReference::m_instances)->
                Field("Asset", &SliceReference::m_asset);

            serializeContext->Class<EntityRestoreInfo>()->
                Version(1)->
                Field("AssetId", &EntityRestoreInfo::m_assetId)->
                Field("InstanceId", &EntityRestoreInfo::m_instanceId)->
                Field("AncestorId", &EntityRestoreInfo::m_ancestorId)->
                Field("DataFlags", &EntityRestoreInfo::m_dataFlags); // added at v1
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection))
        {
            behaviorContext->Class<SliceComponent::SliceInstanceAddress>()
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "slice")
                ->Method("IsValid", &SliceComponent::SliceInstanceAddress::operator bool)
                ;
        }
    }
} // namespace AZ
