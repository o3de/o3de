/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Entity.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/functional.h>
#include <AzCore/Serialization/IdUtils.h>

namespace AZ
{
    namespace EntityUtils
    {
        /// Return default application serialization context.
        SerializeContext* GetApplicationSerializeContext();

        /**
         * Serializable container for entities, useful for data patching and serializer enumeration.
         * Does not assume ownership of stored entities.
         */
        struct SerializableEntityContainer
        {
            AZ_CLASS_ALLOCATOR(SerializableEntityContainer, SystemAllocator);
            AZ_TYPE_INFO(SerializableEntityContainer, "{E98CF1B5-6B72-46C5-AB87-3DB85FD1B48D}");

            AZStd::vector<AZ::Entity*> m_entities;
        };

        /**
         * Reflect entity utils data types.
         */
        void Reflect(ReflectContext* context);

        /**
        * Given key, return the EntityId to map to
        */
        typedef AZStd::function< EntityId(const EntityId& /*originalId*/, bool /*isEntityId*/) > EntityIdMapper;
        typedef AZStd::function< void(const AZ::EntityId& /*id*/, bool /*isEntityId*/, const AZ::SerializeContext::ClassElement* /*elementData*/) > EntityIdVisitor;

        /**
        * Enumerates all entity references in the object's hierarchy and remaps them with the result returned by mapper.
        * "entity reference" is considered any EntityId type variable, except Entity::m_id. Entity class had only one EntityId member (m_id)
        * which represents an actual entity id, every other stored EntityId is considered a reference and will be remapped if when needed.
        * What if I want to store and EntityId that should never be remapped. You should NOT do that, as we clone entities a lot and remap all
        * references to maintain the same behavior. If you really HAVE TO store entity id which is not no be remapped, just use "u64" type variable.
        */
        template<class T>
        unsigned int ReplaceEntityRefs(T* classPtr, const EntityIdMapper& mapper, SerializeContext* context = nullptr)
        {
            AZ_PROFILE_FUNCTION(AzCore);
            auto idMapper = [&mapper](const EntityId& originalId, bool isEntityId, const IdUtils::Remapper<EntityId>::IdGenerator&) -> EntityId
            {
                return mapper(originalId, isEntityId);
            };
            return IdUtils::Remapper<EntityId>::RemapIds(classPtr, SerializeTypeInfo<T>::GetUuid(classPtr), idMapper, context, false);
        }

        /**
        * Enumerates all entity references in the object's hierarchy and invokes the specified visitor.
        * \param classPtr - the object instance to enumerate.
        * \param classUuid - the object instance's type Id.
        * \param visitor - the visitor callback to be invoked for Entity::m_id (isEntityId==true) or reflected EntityId field, aka reference (isEntityId==false).
        */
        void EnumerateEntityIds(const void* classPtr, const Uuid& classUuid, const EntityIdVisitor& visitor, SerializeContext* context = nullptr);

        template<class T>
        void EnumerateEntityIds(const T* classPtr, const EntityIdVisitor& visitor, SerializeContext* context = nullptr)
        {
            EnumerateEntityIds(classPtr, SerializeTypeInfo<T>::GetUuid(classPtr), visitor, context);
        }

        /**
         * Replaces all entity ids in the object's hierarchy and remaps them with the result returned by the mapper.
         * "entity id" is only Entity::m_id every other EntityId variable is considered a reference \ref ReplaceEntityRefs
         */
        template<class T>
        unsigned int ReplaceEntityIds(T* classPtr, const EntityIdMapper& mapper, SerializeContext* context = nullptr)
        {
            AZ_PROFILE_FUNCTION(AzCore);
            auto idMapper = [&mapper](const EntityId& originalId, bool isEntityId, const IdUtils::Remapper<EntityId>::IdGenerator&) -> EntityId
            {
                return mapper(originalId, isEntityId);
            };
            return IdUtils::Remapper<EntityId>::RemapIds(classPtr, SerializeTypeInfo<T>::GetUuid(classPtr), idMapper, context, true);
        }

        /**
         * Replaces all EntityId objects (entity ids and entity refs) in the object's hierarchy.
         */
        template<class T>
        unsigned int ReplaceEntityIdsAndEntityRefs(T* classPtr, const EntityIdMapper& mapper, SerializeContext* context = nullptr)
        {
            AZ_PROFILE_FUNCTION(AzCore);
            auto idMapper = [&mapper](const EntityId& originalId, bool isEntityId, const IdUtils::Remapper<EntityId>::IdGenerator&) -> EntityId
            {
                return mapper(originalId, isEntityId);
            };
            return IdUtils::Remapper<EntityId>::ReplaceIdsAndIdRefs(classPtr, idMapper, context);
        }

        /**
        * Generate new entity ids, and remap all reference
        */
        template<class T, class MapType>
        void GenerateNewIdsAndFixRefs(T* object, MapType& newIdMap, SerializeContext* context = nullptr)
        {
            IdUtils::Remapper<EntityId>::GenerateNewIdsAndFixRefs(object, newIdMap, context);
        }

        /**
         * Clone the object T, generate new ids for all entities in the hierarchy, and fix all entityId.
         */
        template<class T, class Map>
        T* CloneObjectAndFixEntities(const T* object, Map& newIdMap, SerializeContext* context = nullptr)
        {
            return IdUtils::Remapper<EntityId>::CloneObjectAndGenerateNewIdsAndFixRefs(object, newIdMap, context);
        }

        /**
         * Clones entities, generates new entityIds \ref ReplaceEntityIds and fixes all entity references \ref ReplaceEntityRefs
         */
        template<class InputIterator, class OutputIterator, class TempAllocatorType >
        void CloneAndFixEntities(InputIterator first, InputIterator last, OutputIterator result, const TempAllocatorType& allocator, SerializeContext* context = nullptr)
        {
            for (; first != last; ++first, ++result)
            {
                *result = CloneObjectAndFixEntities(*first, allocator, context);
            }
        }

        template<class InputIterator, class OutputIterator>
        void CloneAndFixEntities(InputIterator first, InputIterator last, OutputIterator result, SerializeContext* context = nullptr)
        {
            for (; first != last; ++first, ++result)
            {
                *result = CloneObjectAndFixEntities(*first, context);
            }
        }


        /// Return the first component that is either of the specified type or derive from the specified type
        Component* FindFirstDerivedComponent(const Entity* entity, const Uuid& typeId);
        Component* FindFirstDerivedComponent(EntityId entityId, const Uuid& typeId);

        /// Return the first component that is either of the specified type or derive from the specified type
        template<class ComponentType>
        inline ComponentType* FindFirstDerivedComponent(const Entity* entity)
        {
            return azrtti_cast<ComponentType*>(FindFirstDerivedComponent(entity, AzTypeInfo<ComponentType>::Uuid()));
        }

        template<class ComponentType>
        inline ComponentType* FindFirstDerivedComponent(EntityId entityId)
        {
            Entity* entity{};
            ComponentApplicationBus::BroadcastResult(entity, &ComponentApplicationRequests::FindEntity, entityId);
            return entity ? FindFirstDerivedComponent<ComponentType>(entity): nullptr;
        }

        /// Return a vector of all components that are either of the specified type or derive from the specified type
        Entity::ComponentArrayType FindDerivedComponents(const Entity* entity, const Uuid& typeId);
        Entity::ComponentArrayType FindDerivedComponents(EntityId entityId, const Uuid& typeId);

        /// Return a vector of all components that are either of the specified type or derive from the specified type
        template<class ComponentType>
        inline AZStd::vector<ComponentType*> FindDerivedComponents(Entity* entity)
        {
            AZStd::vector<ComponentType*> result;
            for (AZ::Component* component : entity->GetComponents())
            {
                auto derivedComponent = azrtti_cast<ComponentType*>(component);
                if (derivedComponent)
                {
                    result.push_back(derivedComponent);
                }
            }
            return result;
        }

        template<class ComponentType>
        inline AZStd::vector<ComponentType*> FindDerivedComponents(EntityId entityId)
        {
            Entity* entity{};
            ComponentApplicationBus::BroadcastResult(entity, &ComponentApplicationRequests::FindEntity, entityId);
            return entity ? FindDerivedComponents<ComponentType>(entity) : AZStd::vector<ComponentType*>();
        }

        using EnumerateBaseRecursiveVisitor = AZStd::function< bool(const SerializeContext::ClassData*, const Uuid&)>;
        bool EnumerateBaseRecursive(SerializeContext* context, const EnumerateBaseRecursiveVisitor& baseClassVisitor, const TypeId& typeToExamine);

        //! Performs a recursive search of all classes declared in the serialize hierarchy of typeToExamine
        //! and returns true it has been marked as deprecated, false otherwise.
        bool CheckIfClassIsDeprecated(SerializeContext* context, const TypeId& typeToExamine);

        //! performs a recursive search of all classes declared in the serialize hierarchy of typeToExamine
        //! and returns true if it finds typeToFind, false otherwise.
        bool CheckDeclaresSerializeBaseClass(SerializeContext* context, const TypeId& typeToFind, const TypeId& typeToExamine);

        //! Checks if the provided service array has any duplicates of the iterator, after that iterator.
        //! If a duplicate is found, a warning is given and the duplicate is removed from the providedServiceArray.
        //! The caller is responsible for verifying the iterator is for the array provided.
        //! \param iterator The iterator to start from for checking for duplicates.
        //! \param providedServiceArray The container of services to scan for duplicates.
        //! \param entity An optional associated entity, used in error reporting.
        //! \return True if a duplicate service was found, false if not.
        bool RemoveDuplicateServicesOfAndAfterIterator(
            const ComponentDescriptor::DependencyArrayType::iterator& iterator,
            ComponentDescriptor::DependencyArrayType& providedServiceArray,
            const Entity* entity);

        //! Converts a vector of components to a map of pairs of component alias and component.
        //! \param components Component vector to be converted.
        //! \param[out] componentMapOut Component map that stores a component alias as key and a component as value.
        void ConvertComponentVectorToMap(
            AZStd::span<AZ::Component* const> components, AZStd::unordered_map<AZStd::string, AZ::Component*>& componentMapOut);

    } // namespace EntityUtils
}   // namespace AZ
