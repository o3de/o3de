/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/HphaSchema.h>
#include <AzCore/Memory/SimpleSchemaAllocator.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    namespace Debug
    {
        struct AssetTrackingId;
    }
}

namespace AZStd
{
    // Declare hash specializations for types that need them; implementations will have to come after the classes are fully defined
    template<>
    struct hash<AZ::Debug::AssetTrackingId>
    {
        size_t operator()(const AZ::Debug::AssetTrackingId& id) const;
    };
}

namespace AZ
{
    namespace Debug
    {
        class AssetTrackingImpl;

        // Custom allocator for the Analyzer that doesn't go through profiling tools and cannot be overridden
        class AssetTrackingAllocator : public AZ::SimpleSchemaAllocator<AZ::HphaSchema, AZ::HphaSchema::Descriptor, false>
        {
        public:
            AZ_TYPE_INFO(AssetTrackingAllocator, "{F6C08E92-559C-4153-9620-6A8491F78F10}");

            using Base = AZ::SimpleSchemaAllocator<AZ::HphaSchema, AZ::HphaSchema::Descriptor, false>;
            using Descriptor = Base::Descriptor;

            AssetTrackingAllocator()
                : Base("AssetTrackingAllocator", "Allocator for the AssetTracking")
            {
                DisableOverriding();
            }
        };

        using AZStdAssetTrackingAllocator = AZ::AZStdAlloc<AssetTrackingAllocator>;
        using AssetTrackingString = AZStd::basic_string<char, AZStd::char_traits<char>, AZStdAssetTrackingAllocator>;

        template<typename Key, typename MappedType>
        using AssetTrackingMap = AZStd::unordered_map<Key, MappedType, AZStd::hash<Key>, AZStd::equal_to<Key>, AZStdAssetTrackingAllocator>;


        // ID for an asset that is hashable.
        // Currently only contains one string identifier, but we may want to store a more sophisticated ID in the future.
        struct AssetTrackingId
        {
            AssetTrackingId(const char* id) : m_id(id)
            {
            }

            bool operator==(const AssetTrackingId& other) const
            {
                return m_id == other.m_id;
            }

            AssetTrackingString m_id;
        };

        // Primary information about an asset.
        // Currently just contains the ID of the asset, but in the future may carry additional information about that asset (such as where in code it was initialized).
        struct AssetPrimaryInfo
        {
            const AssetTrackingId* m_id;
        };

        // Base class for a node in the asset tree. Implemented by the template AssetTreeNode<>.
        class AssetTreeNodeBase
        {
        public:
            virtual ~AssetTreeNodeBase() = default;
            virtual const AssetPrimaryInfo* GetAssetPrimaryInfo() const = 0;
            virtual AssetTreeNodeBase* FindOrAddChild(const AssetTrackingId& id, const AssetPrimaryInfo* info) = 0;
        };

        // Base class for an asset tree. Implemented by the template AssetTree<>.
        class AssetTreeBase
        {
        public:
            virtual ~AssetTreeBase() = default;
            virtual AssetTreeNodeBase& GetRoot() = 0;
        };

        // Base class for an asset allocation table. Implemented by the template AssetAllocationTable<>.
        class AssetAllocationTableBase
        {
        public:
            virtual ~AssetAllocationTableBase() = default;
            virtual AssetTreeNodeBase* FindAllocation(void* ptr) const = 0;
        };
    }
}

///////////////////////////////////////////////////////////////////////////////
// Hash functions for map support
///////////////////////////////////////////////////////////////////////////////

inline size_t AZStd::hash<AZ::Debug::AssetTrackingId>::operator()(const AZ::Debug::AssetTrackingId& info) const
{
    return AZStd::hash<AZ::Debug::AssetTrackingString>()(info.m_id);
}

