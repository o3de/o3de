/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI.Reflect/Handle.h>
#include <AzCore/Name/Name.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/sort.h>

namespace AZ::RHI
{
    template <typename IndexType>
    struct ReflectionNamePair
    {
        ReflectionNamePair() = default;
        ReflectionNamePair(AZ::Name name, IndexType index)
            : m_name{ AZStd::move(name) }
            , m_index{ index }
        {}

        bool operator < (AZ::Name::Hash otherHash) const
        {
            return m_name.GetHash() < otherHash;
        }

        bool operator < (const ReflectionNamePair<IndexType>& otherPair) const
        {
            return m_name.GetHash() < otherPair.m_name.GetHash();
        }

        AZ::Name m_name;
        IndexType m_index;
    };

    //! This class is a simple utility for mapping an AZ::Name to an RHI::Handle<> instance. It
    //! is useful for implementing name-to-index reflection. It uses a sorted vector with binary
    //! search to be cache friendly and use a single allocation (when serialized).
    template <typename IndexType = Handle<>>
    class NameIdReflectionMap
    {
    public:
        static void Reflect(ReflectContext* context);

        /// Clears the container back to empty.
        void Clear();

        /// Reserves sufficient memory for capacity elements.
        void Reserve(size_t capacity);

        /// Inserts a new id -> index mapping. Emits an error and returns false if the same entry is added twice.
        bool Insert(const Name& id, IndexType index);

        /// Finds and returns the index associated with the requested id. If no matching id exists, a null index is returned.
        IndexType Find(const Name& id) const;

        /// Finds and returns the name associated with the index mapping. If no matching index mapping exists, an empty name is returned.
        Name Find(IndexType index) const;

        /// Return the number of entries
        size_t Size() const;

        // Returns true if size is zero
        bool IsEmpty() const;

        class NameIdReflectionMapSerializationEvents
            : public SerializeContext::IEventHandler
        {
            void OnLoadedFromObjectStream(void* classPtr) override
            {
                // Serialization doesn't save in a hash sorted way, so we need to re-sort upon de-serialization
                NameIdReflectionMap<IndexType>* mapReference = reinterpret_cast<NameIdReflectionMap<IndexType>*>(classPtr);
                mapReference->Sort();
            }
        };

    private:
        /// Sort by the hash value of names
        void Sort();

        using ReflectionPair = AZ::RHI::ReflectionNamePair<IndexType>;

        AZStd::vector<ReflectionPair> m_reflectionMap;
    };

    template <typename IndexType>
    void NameIdReflectionMap<IndexType>::Reflect(AZ::ReflectContext* context)
    {
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<typename NameIdReflectionMap<IndexType>::ReflectionPair>()
                ->Version(2)
                ->Field("Name", &ReflectionPair::m_name)
                ->Field("Index", &ReflectionPair::m_index);

            serializeContext->Class<NameIdReflectionMap<IndexType>>()
                ->Version(0)
                ->template EventHandler<typename NameIdReflectionMap<IndexType>::NameIdReflectionMapSerializationEvents>()
                ->Field("ReflectionMap", &NameIdReflectionMap<IndexType>::m_reflectionMap);
        }
    }

    template <typename IndexType>
    void NameIdReflectionMap<IndexType>::Clear()
    {
        m_reflectionMap.clear();
    }

    template <typename IndexType>
    void NameIdReflectionMap<IndexType>::Reserve(size_t capacity)
    {
        m_reflectionMap.reserve(capacity);
    }

    template <typename IndexType>
    bool NameIdReflectionMap<IndexType>::Insert(const Name& id, IndexType index)
    {
        // NOTE: The NameDictionary already validates against hash collisions.
        const AZ::Name::Hash nameHash = id.GetHash();
            
        auto it = AZStd::lower_bound(m_reflectionMap.begin(), m_reflectionMap.end(), nameHash);
        if (it == m_reflectionMap.end() || it->m_name.GetHash() != id.GetHash())
        {
            m_reflectionMap.emplace(it, id, index);
            return true;
        }

        AZ_Error("NameIdReflectionMap", false, "ID already exists in the map. It is not permitted to insert the same ID multiple times.");
        return false;
    }

    template <typename IndexType>
    IndexType NameIdReflectionMap<IndexType>::Find(const Name& id) const
    {
        const AZ::Name::Hash nameHash = id.GetHash();

        const auto it = AZStd::lower_bound(m_reflectionMap.begin(), m_reflectionMap.end(), nameHash);

        if (it != m_reflectionMap.end() && it->m_name.GetHash() == nameHash)
        {
            return it->m_index;
        }
        return IndexType::Null;
    }

    template <typename IndexType>
    Name NameIdReflectionMap<IndexType>::Find(IndexType index) const
    {
        for (const auto& it : m_reflectionMap)
        {
            if (it.m_index == index)
            {
                return it.m_name;
            }
        }

        return Name();
    }

    template <typename IndexType>
    size_t NameIdReflectionMap<IndexType>::Size() const
    {
        return m_reflectionMap.size();
    }

    template <typename IndexType>
    bool NameIdReflectionMap<IndexType>::IsEmpty() const
    {
        return Size() == 0;
    }

    template <typename IndexType>
    void NameIdReflectionMap<IndexType>::Sort()
    {
        AZStd::sort(m_reflectionMap.begin(), m_reflectionMap.end());
    }
}

namespace AZ
{
    AZ_TYPE_INFO_TEMPLATE(AZ::RHI::NameIdReflectionMap, "{153CEFAB-7781-4307-AC0E-41DEA51FADFC}", AZ_TYPE_INFO_TYPENAME);
    AZ_TYPE_INFO_TEMPLATE(AZ::RHI::ReflectionNamePair, "{2E2722BE-9BE7-4D5C-8173-411AC20F20B8}", AZ_TYPE_INFO_TYPENAME);
}
