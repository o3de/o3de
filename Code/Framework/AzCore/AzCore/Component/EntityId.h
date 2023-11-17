/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZCORE_ENTITY_ID_H
#define AZCORE_ENTITY_ID_H

#include <AzCore/base.h>
#include <AzCore/Memory/ChildAllocatorSchema.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/TypeInfoSimple.h>
#include <AzCore/std/string/string.h>

/** @file
  * Header file for the entity ID type. 
  * Entity IDs are used to uniquely identify entities.
  */

namespace AZ
{
    AZ_CHILD_ALLOCATOR_WITH_NAME(EntityAllocator, "EntityAllocator", "{C3FA54B6-DAFC-44A8-98C2-7EB0ACF92BE8}", SystemAllocator);

    /**
     * Entity ID type.
     * Entity IDs are used to uniquely identify entities. Each component that is 
     * attached to an entity is tagged with the entity's ID, and component buses 
     * are typically addressed by entity ID.
     */
    class EntityId
    {
        friend class JsonEntityIdSerializer;
        friend class Entity;

    public:
        AZ_CLASS_ALLOCATOR(EntityId, EntityAllocator);

        /**
         * Invalid entity ID with a machine ID of 0 and the maximum timestamp.
         */
        static constexpr u64 InvalidEntityId = 0x00000000FFFFFFFFull;
                 
        /**
         * Enables this class to be identified across modules and serialized into
         * different contexts.
         */
        AZ_TYPE_INFO(EntityId, "{6383F1D3-BB27-4E6B-A49A-6409B2059EAA}");

        /**
         * Creates an entity ID instance. 
         * If you do not provide a value for the entity ID, 
         * the entity ID is set to an invalid value.
         * @param id (Optional) An ID for the entity.
         */
        explicit AZ_FORCE_INLINE EntityId(u64 id = InvalidEntityId)
            : m_id(id)
        {}

        /**
         * Casts the entity ID to u64.
         * @return The entity ID.
         */
        AZ_FORCE_INLINE explicit operator u64() const
        {
            return m_id;
        }

        /**
         * Determines whether this entity ID is valid. 
         * An entity ID is invalid if you did not provide an argument 
         * to the entity ID constructor.
         * @return Returns true if the entity ID is valid. Otherwise, false.
         */
        AZ_FORCE_INLINE bool IsValid() const
        {
            return m_id != InvalidEntityId;
        }
        
        /**
         * Sets the entity ID to an invalid value.
         */
        AZ_FORCE_INLINE void SetInvalid()
        {
            m_id = InvalidEntityId;
        }

        /**
         * Returns the entity ID as a string.
         */
        AZStd::string ToString() const
        {
            return AZStd::string::format("[%llu]", m_id);
        }
        
        /**
         * Compares two entity IDs for equality.
         * @param rhs An entity ID whose value you want to compare to the
         * given entity ID.
         * @return True if the entity IDs are equal. Otherwise, false.
         */
        AZ_FORCE_INLINE bool operator==(const EntityId& rhs) const
        {
            return m_id == rhs.m_id;
        }
        /**
         * Compares two entity IDs.
         * @param rhs An entity ID whose value you want to compare to the 
         * given entity ID.
         * @return True if the entity IDs are different. Otherwise, false.
         */
        AZ_FORCE_INLINE bool operator!=(const EntityId& rhs) const
        {
            return m_id != rhs.m_id;
        }

        /**
         * Evaluates whether the entity ID is less than a given entity ID.
         * @param rhs An entity ID whose size you want to compare to the given 
         * entity ID.
         * @return True if the entity ID is less than the given entity ID.
         * Otherwise, false.
         */
        AZ_FORCE_INLINE bool operator<(const EntityId& rhs) const
        {
            return m_id < rhs.m_id;
        }

        /**
        * Evaluates whether the entity ID is greater than a given entity ID.
        * @param rhs An entity ID whose size you want to compare to the given
        * entity ID.
        * @return True if the entity ID is greater than the given entity ID.
        * Otherwise, false.
        */
        AZ_FORCE_INLINE bool operator>(const EntityId& rhs) const
        {
            return m_id > rhs.m_id;
        }

    protected:

        /**
         * Entity ID.
         */
        u64 m_id;
    };

    /// @cond EXCLUDE_DOCS
    static const EntityId SystemEntityId = EntityId(0);
    /// @endcond
  
}   // namespace AZ

namespace AZStd
{

    /**
     * Enables entity IDs to be keys in hashed data structures.
     */
    template<>
    struct hash<AZ::EntityId>
    {
        typedef AZ::EntityId    argument_type;
        typedef AZStd::size_t   result_type;
        AZ_FORCE_INLINE size_t operator()(const AZ::EntityId& id) const
        {
            AZStd::hash<AZ::u64> hasher;
            return hasher(static_cast<AZ::u64>(id));
        }

    };
}
#endif  // AZCORE_ENTITY_ID_H
#pragma once
