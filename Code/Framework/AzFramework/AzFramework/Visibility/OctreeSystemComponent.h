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

#pragma once

#include <AzFramework/Visibility/IVisibilitySystem.h>
#include <AzCore/Math/Plane.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/std/containers/stack.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/fixed_vector.h>

namespace AzFramework
{
    class OctreeSystemComponent;

    //! An internal node within the tree.
    //! It contains all objects that are *fully contained* by the node, if an object spans multiple child nodes that object will be stored in the parent.
    class OctreeNode
        : public VisibilityNode
    {
    public:

        OctreeNode() = default;
        explicit OctreeNode(const AZ::Aabb& bounds);
        OctreeNode(OctreeNode&& rhs);

        virtual ~OctreeNode() = default;

        OctreeNode& operator=(OctreeNode&& rhs);

        //! Inserts a VisibilityEntry into this OctreeNode, potentially triggering a split.
        void Insert(OctreeSystemComponent& octreeSystemComponent, VisibilityEntry* entry);

        //! Updates a VisibilityEntry that is currently bound to this OctreeNode.
        //! The provided entry must be bound to this node, but may no longer be bound to this node upon function exit.
        void Update(OctreeSystemComponent& octreeSystemComponent, VisibilityEntry* entry);

        //! Removes a VisibilityEntry from this OctreeNode.
        //! The provided entry must be bound to this node.
        void Remove(OctreeSystemComponent& octreeSystemComponent, VisibilityEntry* entry);

        //! Recursively enumerates any OctreeNodes and their children that intersect the provided bounding volume.
        //! @{
        void Enumerate(const AZ::Aabb& aabb, const IVisibilitySystem::EnumerateCallback& callback) const;
        void Enumerate(const AZ::Sphere& sphere, const IVisibilitySystem::EnumerateCallback& callback) const;
        void Enumerate(const AZ::Frustum& frustum, const IVisibilitySystem::EnumerateCallback& callback) const;
        //! @}

        //! Recursively enumerate *all* OctreeNodes that have any entries in them (without any culling).
        void EnumerateNoCull(const IVisibilitySystem::EnumerateCallback& callback) const;

        //! Returns the set of entries bound to this node.
        const AZStd::vector<VisibilityEntry*>& GetEntries() const;

        //! Returns the array of child nodes for this OctreeNode, may be nullptr if this OctreeNode is a leaf node.
        OctreeNode* GetChildren() const;

        //! Returns true if this is a leaf node.
        bool IsLeaf() const;

    private:

        void TryMerge(OctreeSystemComponent& octreeSystemComponent);

        template <typename T>
        void EnumerateHelper(const T& boundingVolume, const IVisibilitySystem::EnumerateCallback& callback) const;

        void Split(OctreeSystemComponent& octreeSystemComponent);
        void Merge(OctreeSystemComponent& octreeSystemComponent);

        // The page is stored in the upper 16-bits of the child node index, the offset into the page is the lower 16-bits
        // This gives us a maximum of 65,536 pages and 65,536 nodes per page, for a total of 2^32 - 1 total pages (-1 reserved for the invalid index)
        static constexpr uint32_t InvalidChildNodeIndex = 0xFFFFFFFF;
        uint32_t m_childNodeIndex = InvalidChildNodeIndex;
        AZ::Aabb m_bounds;
        OctreeNode* m_parent = nullptr; //< This is a pointer to an array of GetChildNodeCount() nodes, or nullptr if this is a leaf node
        OctreeNode* m_children = nullptr;
        AZStd::vector<VisibilityEntry*> m_entries;
    };

    //! Implementation of the visibility system interface.
    //! This uses a simple adaptive octreeSystemComponent to support partitioning an object set and efficiently running gathers and visibility queries.
    class OctreeSystemComponent
        : public AZ::Component
        , public IVisibilitySystemRequestBus::Handler
    {
    public:

        AZ_COMPONENT(OctreeSystemComponent, "{CD4FF1C5-BAF4-421D-951B-1E05DAEEF67B}");

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        OctreeSystemComponent();
        virtual ~OctreeSystemComponent();

        //! AZ::Component overrides.
        //! @{
        void Activate() override;
        void Deactivate() override;
        //! @}

        //! IVisibilitySystem overrides.
        //! @{
        void InsertOrUpdateEntry(VisibilityEntry& entry) override;
        void RemoveEntry(VisibilityEntry& entry) override;
        void Enumerate(const AZ::Aabb& aabb, const IVisibilitySystem::EnumerateCallback& callback) const override;
        void Enumerate(const AZ::Sphere& sphere, const IVisibilitySystem::EnumerateCallback& callback) const override;
        void Enumerate(const AZ::Frustum& frustum, const IVisibilitySystem::EnumerateCallback& callback) const override;
        void EnumerateNoCull(const IVisibilitySystem::EnumerateCallback& callback) const override;
        uint32_t GetEntryCount() const override;
        //! @}

        //! Returns the OctreeSystemComponent's root node.
        OctreeNode& GetRoot();

        //! OctreeSystemComponent stats
        //! @{
        uint32_t GetNodeCount() const;
        uint32_t GetFreeNodeCount() const;
        uint32_t GetPageCount() const;
        uint32_t GetChildNodeCount() const;
        void DumpStats(const AZ::ConsoleCommandContainer& arguments);
        //! @}

    private:

        uint32_t AllocateChildNodes();
        void ReleaseChildNodes(uint32_t nodeIndex);
        OctreeNode* GetChildNodesAtIndex(uint32_t nodeIndex) const;

        // Bind the DumpStats member function to the console as 'OctreeSystemComponent.DumpStats'
        AZ_CONSOLEFUNC(OctreeSystemComponent, DumpStats, AZ::ConsoleFunctorFlags::Null, "Dump octreeSystemComponent stats to the console window");

        OctreeNode m_root; //< The root node for the octreeSystemComponent.

        uint32_t m_entryCount = 0; //< Metric tracking the number of entries inserted into the octreeSystemComponent.
        uint32_t m_nodeCount = 1; //< Metric tracking the number of nodes allocated by the octreeSystemComponent, at least one for the root node.

        static constexpr uint32_t BlockSize = 8192; //< This represents the number of nodes that can be stored in each page
        static_assert(BlockSize < 0xFFFF, "BlockSize must be less than 2^16");

        using OctreeNodePage = AZStd::fixed_vector<OctreeNode, BlockSize>;
        AZStd::vector<OctreeNodePage*> m_nodeCache; //< Array of contiguous memory blocks for all allocated nodes within the tree.
        AZStd::stack<uint32_t> m_freeOctreeNodes; //< Indices of free nodes, each entry represents a contiguous block of free OctreeNodeChildCount nodes.

        friend class OctreeNode; // For access to the node allocator methods
    };
}
