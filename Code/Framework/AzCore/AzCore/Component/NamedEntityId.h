/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    class ReflectContext;

    class NamedEntityId
        : public EntityId
    {
    public: 
        AZ_CLASS_ALLOCATOR(NamedEntityId, EntityAllocator);
        AZ_RTTI(NamedEntityId, "{27F37921-4B40-4BE6-B47B-7D3AB8682D58}", EntityId);

        static void Reflect(AZ::ReflectContext* context);
        
        NamedEntityId();
        NamedEntityId(const AZ::EntityId& entityId, AZStd::string_view entityName = "");
        virtual ~NamedEntityId() = default;

        AZStd::string ToString() const;

        AZStd::string_view GetName() const;

        bool operator==(const NamedEntityId& rhs) const;
        bool operator==(const EntityId& rhs) const;

        bool operator!=(const NamedEntityId& rhs) const;
        bool operator!=(const EntityId& rhs) const;

        bool operator<(const NamedEntityId& rhs) const;
        bool operator<(const EntityId& rhs) const;

        bool operator>(const NamedEntityId& rhs) const;
        bool operator>(const EntityId& rhs) const;

    private:
        AZStd::string m_entityName;
    };
}   // namespace AZ

namespace AZStd
{

    /**
    * Enables entity IDs to be keys in hashed data structures.
    */
    template<>
    struct hash<AZ::NamedEntityId>
    {
        typedef AZ::NamedEntityId    argument_type;
        typedef AZStd::size_t        result_type;

        AZ_FORCE_INLINE size_t operator()(const AZ::NamedEntityId& namedId) const
        {
            return AZStd::hash<AZ::EntityId>()(namedId);
        }
    };
} // namespace AZStd
