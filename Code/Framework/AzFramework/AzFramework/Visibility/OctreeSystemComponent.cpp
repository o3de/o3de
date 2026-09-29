/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Visibility/OctreeSystemComponent.h>

#include <AzCore/Debug/Profiler.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/ShapeIntersection.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AzFramework
{
    constexpr float DefaultOctreeRootHalfExtent = 16384.0f;

    AZ_CVAR(bool,     bg_octreeUseQuadtree,        false, nullptr, AZ::ConsoleFunctorFlags::ReadOnly, "If set to true, the visibility octrees will degenerate to a quadtree split along the X/Y plane");
    AZ_CVAR(float, bg_octreeMaxWorldExtents, DefaultOctreeRootHalfExtent, nullptr, AZ::ConsoleFunctorFlags::Null, "Initial root half extent, captured per scene. The root grows automatically to contain supported finite bounds");
    AZ_CVAR(uint32_t, bg_octreeNodeMaxEntries,        64, nullptr, AZ::ConsoleFunctorFlags::Null, "Maximum number of entries to allow in any node before forcing a split");
    AZ_CVAR(uint32_t, bg_octreeNodeMinEntries,        32, nullptr, AZ::ConsoleFunctorFlags::Null, "Minimum number of entries to allow in a node resulting from a merge operation");

    static uint32_t GetChildNodeCount()
    {
        constexpr uint32_t QuadtreeNodeChildCount = 4;
        constexpr uint32_t OctreeNodeChildCount   = 8;
        return (bg_octreeUseQuadtree) ? QuadtreeNodeChildCount : OctreeNodeChildCount;
    }

    static AZ::Aabb GetChildBounds(
        const AZ::Aabb& parentBounds,
        uint32_t child)
    {
        const AZ::Vector3 midpoint = parentBounds.GetMin() + parentBounds.GetExtents() * 0.5f;
        AZ::Vector3 childMin = parentBounds.GetMin();
        AZ::Vector3 childMax = parentBounds.GetMax();

        if (child & 0x01)
        {
            childMin.SetX(midpoint.GetX());
        }
        else
        {
            childMax.SetX(midpoint.GetX());
        }
        if (child & 0x02)
        {
            childMin.SetY(midpoint.GetY());
        }
        else
        {
            childMax.SetY(midpoint.GetY());
        }
        if (!bg_octreeUseQuadtree)
        {
            if (child & 0x04)
            {
                childMin.SetZ(midpoint.GetZ());
            }
            else
            {
                childMax.SetZ(midpoint.GetZ());
            }
        }

        return AZ::Aabb::CreateFromMinMax(childMin, childMax);
    }

    static bool CalculateNextRootBounds(
        const AZ::Aabb& currentBounds,
        const AZ::Aabb& targetBounds,
        AZ::Aabb& nextBounds,
        uint32_t& oldRootChild)
    {
        const bool useQuadtree = bg_octreeUseQuadtree;
        const AZ::Vector3 currentExtents = currentBounds.GetExtents();
        AZ::Vector3 newExtents = currentExtents * 2.0f;
        if (useQuadtree)
        {
            newExtents.SetZ(currentExtents.GetZ());
        }

        if (!currentExtents.IsFinite()
            || !newExtents.IsFinite()
            || currentExtents.GetMinElement() <= 0.0f)
        {
            return false;
        }

        const AZ::Vector3 negativeOverflow = currentBounds.GetMin() - targetBounds.GetMin();
        const AZ::Vector3 positiveOverflow = targetBounds.GetMax() - currentBounds.GetMax();
        AZ::Vector3 growthDirection(-1.0f);
        if (positiveOverflow.GetX() >= negativeOverflow.GetX())
        {
            growthDirection.SetX(1.0f);
        }
        if (positiveOverflow.GetY() >= negativeOverflow.GetY())
        {
            growthDirection.SetY(1.0f);
        }
        if (useQuadtree)
        {
            growthDirection.SetZ(0.0f);
        }
        else if (positiveOverflow.GetZ() >= negativeOverflow.GetZ())
        {
            growthDirection.SetZ(1.0f);
        }

        AZ::Vector3 newMin = currentBounds.GetMin();
        AZ::Vector3 newMax = currentBounds.GetMax();
        if (growthDirection.GetX() < 0.0f)
        {
            newMin.SetX(newMax.GetX() - newExtents.GetX());
        }
        else
        {
            newMax.SetX(newMin.GetX() + newExtents.GetX());
        }
        if (growthDirection.GetY() < 0.0f)
        {
            newMin.SetY(newMax.GetY() - newExtents.GetY());
        }
        else
        {
            newMax.SetY(newMin.GetY() + newExtents.GetY());
        }
        if (!useQuadtree)
        {
            if (growthDirection.GetZ() < 0.0f)
            {
                newMin.SetZ(newMax.GetZ() - newExtents.GetZ());
            }
            else
            {
                newMax.SetZ(newMin.GetZ() + newExtents.GetZ());
            }
        }

        nextBounds = AZ::Aabb::CreateFromMinMax(newMin, newMax);
        // Keep centers finite for every descendant AABB, including those near either outer edge.
        if (!nextBounds.IsValid()
            || !nextBounds.IsFinite()
            || !nextBounds.GetExtents().IsFinite()
            || !(newMin * 2.0f).IsFinite()
            || !(newMax * 2.0f).IsFinite()
            || !AZ::ShapeIntersection::Contains(nextBounds, currentBounds))
        {
            return false;
        }

        oldRootChild = 0;
        if (growthDirection.GetX() < 0.0f)
        {
            oldRootChild |= 0x01;
        }
        if (growthDirection.GetY() < 0.0f)
        {
            oldRootChild |= 0x02;
        }
        if (!useQuadtree && growthDirection.GetZ() < 0.0f)
        {
            oldRootChild |= 0x04;
        }

        return true;
    }

    OctreeNode::OctreeNode(const AZ::Aabb& bounds)
        : m_bounds(bounds)
    {
        ;
    }

    OctreeNode::OctreeNode(OctreeNode&& rhs)
    {
        // Custom assignment repairs pointers that a default move would leave referring to rhs.
        *this = AZStd::move(rhs);
    }

    OctreeNode& OctreeNode::operator=(OctreeNode&& rhs)
    {
        AZ_Assert(!m_children && m_entries.empty(), "Move-assignment requires an empty destination OctreeNode");

        m_childNodeIndex = rhs.m_childNodeIndex;
        m_bounds = rhs.m_bounds;
        m_parent = rhs.m_parent;
        m_children = rhs.m_children;
        m_entries = AZStd::move(rhs.m_entries);

        // Correct internal node pointers
        for (VisibilityEntry* entry : m_entries)
        {
            entry->m_internalNode = this;
        }

        if (m_children)
        {
            const uint32_t childCount = GetChildNodeCount();
            for (uint32_t child = 0; child < childCount; ++child)
            {
                m_children[child].m_parent = this;
            }
        }

        rhs.m_childNodeIndex = InvalidChildNodeIndex;
        rhs.m_parent = nullptr;
        rhs.m_children = nullptr;
        rhs.m_entries.clear();

        return *this;
    }

    void OctreeNode::Insert(OctreeScene& octreeScene, VisibilityEntry* entry)
    {
        AZ_Assert(entry->m_internalNode == nullptr, "Double-insertion: Insert invoked for an entry already bound to the OctreeScene");

        // If this is not a leaf node, try to insert into the child nodes
        if (m_children != nullptr)
        {
            const AZ::Aabb boundingVolume = entry->m_boundingVolume;
            const uint32_t childCount = GetChildNodeCount();
            const bool useQuadtree = bg_octreeUseQuadtree;
            for (uint32_t child = 0; child < childCount; ++child)
            {
                AZ::Aabb& childBounds = m_children[child].m_bounds;
                bool contains = AZ::ShapeIntersection::Contains(childBounds, boundingVolume);
                if (useQuadtree)
                {
                    contains = childBounds.GetMin().GetX() <= boundingVolume.GetMin().GetX()
                        && childBounds.GetMax().GetX() >= boundingVolume.GetMax().GetX()
                        && childBounds.GetMin().GetY() <= boundingVolume.GetMin().GetY()
                        && childBounds.GetMax().GetY() >= boundingVolume.GetMax().GetY();
                }
                if (contains)
                {
                    if (useQuadtree && (boundingVolume.GetMin().GetZ() < childBounds.GetMin().GetZ()
                        || boundingVolume.GetMax().GetZ() > childBounds.GetMax().GetZ()))
                    {
                        // Widen only the insertion path.
                        // Its parent already contains the full entry.
                        AZ::Vector3 childMin = childBounds.GetMin();
                        AZ::Vector3 childMax = childBounds.GetMax();
                        childMin.SetZ(AZStd::min(childMin.GetZ(), boundingVolume.GetMin().GetZ()));
                        childMax.SetZ(AZStd::max(childMax.GetZ(), boundingVolume.GetMax().GetZ()));
                        childBounds = AZ::Aabb::CreateFromMinMax(childMin, childMax);
                    }
                    return m_children[child].Insert(octreeScene, entry);
                }
            }
        }

        // If we reach here, either we don't have children or the entry overlaps multiple child nodes
        // Attempt to add the entry to the current nodes entry set, forcing a split if necessary
        if ((m_children == nullptr) && (m_entries.size() >= bg_octreeNodeMaxEntries) && Split(octreeScene))
        {
            Insert(octreeScene, entry);
        }
        else
        {
            m_entries.push_back(entry);
            entry->m_internalNode = this;
            entry->m_internalNodeIndex = aznumeric_cast<uint32_t>(m_entries.size() - 1);
        }
    }

    void OctreeNode::Update(OctreeScene& octreeScene, VisibilityEntry* entry)
    {
        AZ_Assert(entry->m_internalNode == this, "Update invoked for an entry bound to a different OctreeNode");

        const AZ::Aabb boundingVolume = entry->m_boundingVolume;
        if (IsLeaf() && AZ::ShapeIntersection::Contains(m_bounds, boundingVolume))
        {
            // Entry moved, but is still fully contained within the current node
            // We can only do this for leaf nodes, otherwise entries can get 'stuck' in non-leaf nodes
            // even when one of the child nodes would be an adequate fit, due to this early out check
            return;
        }

        // Removing the entry can merge and release this node, but its parent survives that merge.
        OctreeNode* insertCheck = this;
        if (m_parent)
        {
            insertCheck = m_parent;
        }
        Remove(octreeScene, entry);

        // Traverse up our ancestor nodes to find the first node that fully contains the entry
        // This strategy assumes an entry will typically move a small distance relative to the total world
        while (insertCheck)
        {
            if (AZ::ShapeIntersection::Contains(insertCheck->m_bounds, boundingVolume) || !insertCheck->m_parent)
            {
                // Insert here if the entry is fully contained or if we've reached the root node
                return insertCheck->Insert(octreeScene, entry);
            }
            insertCheck = insertCheck->m_parent;
        }
    }

    void OctreeNode::Remove(OctreeScene& octreeScene, VisibilityEntry* entry)
    {
        AZ_Assert(entry->m_internalNode == this, "Remove invoked for an entry bound to a different OctreeNode");
        AZ_Assert(m_entries[entry->m_internalNodeIndex] == entry, "Visibility entry data is corrupt");

        // Swap and pop the removed entry
        const uint32_t removeIndex = entry->m_internalNodeIndex;
        m_entries[removeIndex]->m_internalNode = nullptr;
        m_entries[removeIndex]->m_internalNodeIndex = 0;
        if (removeIndex < (m_entries.size() - 1))
        {
            AZStd::swap(m_entries[removeIndex], m_entries.back());
            m_entries[removeIndex]->m_internalNodeIndex = removeIndex;
        }
        m_entries.pop_back();

        if (m_parent != nullptr)
        {
            m_parent->TryMerge(octreeScene);
        }
    }

    void OctreeNode::Enumerate(const AZ::Aabb& aabb, const IVisibilityScene::EnumerateCallback& callback) const
    {
        if (AZ::ShapeIntersection::Overlaps(aabb, m_bounds))
        {
            EnumerateHelper(aabb, callback);
        }
    }

    void OctreeNode::Enumerate(const AZ::Sphere& sphere, const IVisibilityScene::EnumerateCallback& callback) const
    {
        if (AZ::ShapeIntersection::Overlaps(sphere, m_bounds))
        {
            EnumerateHelper(sphere, callback);
        }
    }

    void OctreeNode::Enumerate(const AZ::Hemisphere& hemisphere, const IVisibilityScene::EnumerateCallback& callback) const
    {
        if (AZ::ShapeIntersection::Overlaps(hemisphere, m_bounds))
        {
            EnumerateHelper(hemisphere, callback);
        }
    }

    void OctreeNode::Enumerate(const AZ::Capsule& capsule, const IVisibilityScene::EnumerateCallback& callback) const
    {
        if (AZ::ShapeIntersection::Overlaps(capsule, m_bounds))
        {
            EnumerateHelper(capsule, callback);
        }
    }

    void OctreeNode::Enumerate(const AZ::Frustum& frustum, const IVisibilityScene::EnumerateCallback& callback) const
    {
        if (AZ::ShapeIntersection::Overlaps(frustum, m_bounds))
        {
            EnumerateHelper(frustum, callback);
        }
    }

    void OctreeNode::Enumerate(const AZ::Frustum& includeFrustum, const AZ::Frustum& excludeFrustum, const IVisibilityScene::EnumerateCallback& callback) const
    {
        if (AZ::ShapeIntersection::Overlaps(includeFrustum, m_bounds) && !AZ::ShapeIntersection::Contains(excludeFrustum, m_bounds))
        {
            // Invoke the callback for the current node
            if (!m_entries.empty())
            {
                callback({ m_bounds, m_entries });
            }

            if (m_children != nullptr)
            {
                // If this is not a leaf node, recurse into the children
                const uint32_t childCount = GetChildNodeCount();
                for (uint32_t child = 0; child < childCount; ++child)
                {
                    m_children[child].Enumerate(includeFrustum, excludeFrustum, callback);
                }
            }
        }
    }

    void OctreeNode::EnumerateNoCull(const IVisibilityScene::EnumerateCallback& callback) const
    {
        // Invoke the callback for the current node
        if (!m_entries.empty())
        {
            callback({m_bounds, m_entries});
        }

        if (m_children != nullptr)
        {
            // If this is not a leaf node, recurse into the children
            const uint32_t childCount = GetChildNodeCount();
            for (uint32_t child = 0; child < childCount; ++child)
            {
                m_children[child].EnumerateNoCull(callback);
            }
        }
    }

    const AZStd::vector<VisibilityEntry*>& OctreeNode::GetEntries() const
    {
        return m_entries;
    }

    OctreeNode* OctreeNode::GetChildren() const
    {
        return m_children;
    }

    bool OctreeNode::IsLeaf() const
    {
        return m_children == nullptr;
    }

    void OctreeNode::TryMerge(OctreeScene& octreeScene)
    {
        if (IsLeaf())
        {
            return;
        }

        uint32_t potentialNodeCount = aznumeric_cast<uint32_t>(m_entries.size());

        // Check ourselves and all our siblings for mergeability
        const uint32_t childCount = GetChildNodeCount();
        for (uint32_t child = 0; child < childCount; ++child)
        {
            m_children[child].TryMerge(octreeScene);
            if (!m_children[child].IsLeaf())
            {
                return;
            }
            potentialNodeCount += aznumeric_cast<uint32_t>(m_children[child].m_entries.size());
        }

        if (potentialNodeCount <= bg_octreeNodeMinEntries)
        {
            Merge(octreeScene);
        }
    }

    template <typename T>
    void OctreeNode::EnumerateHelper(const T& boundingVolume, const IVisibilityScene::EnumerateCallback& callback) const
    {
        AZ_Assert(AZ::ShapeIntersection::Overlaps(boundingVolume, m_bounds), "EnumerateHelper invoked on an octreeSystemComponent node that is not within the bounding volume");

        // Invoke the callback for the current node
        if (!m_entries.empty())
        {
            callback({m_bounds, m_entries});
        }

        if (m_children != nullptr)
        {
            // If this is not a leaf node, recurse into the children
            const uint32_t childCount = GetChildNodeCount();
            for (uint32_t child = 0; child < childCount; ++child)
            {
                if (AZ::ShapeIntersection::Overlaps(boundingVolume, m_children[child].m_bounds))
                {
                    m_children[child].EnumerateHelper(boundingVolume, callback);
                }
            }
        }
    }

    bool OctreeNode::Split(OctreeScene& octreeScene)
    {
        AZ_Assert(m_children == nullptr, "Split invoked on an octreeScene node that has already been split");
        const AZ::Vector3 midpoint = m_bounds.GetMin() + m_bounds.GetExtents() * 0.5f;
        if (midpoint.GetX() <= m_bounds.GetMin().GetX()
            || midpoint.GetX() >= m_bounds.GetMax().GetX()
            || midpoint.GetY() <= m_bounds.GetMin().GetY()
            || midpoint.GetY() >= m_bounds.GetMax().GetY()
            || (!bg_octreeUseQuadtree && (midpoint.GetZ() <= m_bounds.GetMin().GetZ() || midpoint.GetZ() >= m_bounds.GetMax().GetZ())))
        {
            // Retain an overfull leaf when floating-point subdivision cannot make progress.
            return false;
        }
        m_childNodeIndex = octreeScene.AllocateChildNodes();
        m_children = octreeScene.GetChildNodesAtIndex(m_childNodeIndex);

        // Set child split planes and bounding volumes.
        // The bit ordering splits X/Y in quadtree mode and X/Y/Z otherwise.
        const uint32_t childCount = GetChildNodeCount();
        for (uint32_t child = 0; child < childCount; ++child)
        {
            m_children[child].m_bounds = GetChildBounds(m_bounds, child);
            m_children[child].m_parent = this;
        }

        // Re-partition our entry set across ourself and our child nodes
        AZStd::vector<VisibilityEntry*> entrySet(AZStd::move(m_entries));
        for (VisibilityEntry* entry : entrySet)
        {
            entry->m_internalNode = nullptr;
            entry->m_internalNodeIndex = 0;
            Insert(octreeScene, entry);
        }

        return true;
    }

    void OctreeNode::Merge(OctreeScene& octreeScene)
    {
        AZ_Assert(m_children != nullptr, "Merge invoked on an octreeScene node that does not have children");

        // Move all child entries to our own entry set
        const uint32_t childCount = GetChildNodeCount();
        for (uint32_t child = 0; child < childCount; ++child)
        {
            for (VisibilityEntry* childEntry : m_children[child].m_entries)
            {
                childEntry->m_internalNode = this;
                childEntry->m_internalNodeIndex = aznumeric_cast<uint32_t>(m_entries.size());
                m_entries.push_back(childEntry);
            }
            m_children[child].m_entries.clear();
        }

        octreeScene.ReleaseChildNodes(m_childNodeIndex);
        m_childNodeIndex = InvalidChildNodeIndex;
        m_children = nullptr;
    }

    OctreeScene::OctreeScene(const AZ::Name& sceneName)
        : m_sceneName(sceneName)
    {
        AZ_Assert(!sceneName.IsEmpty(), "sceneName must be a valid string");

        float initialRootHalfExtent = bg_octreeMaxWorldExtents;
        if (!AZ::IsFiniteFloat(initialRootHalfExtent)
            || initialRootHalfExtent <= 0.0f
            || !AZ::IsFiniteFloat(initialRootHalfExtent * 2.0f))
        {
            AZ_Warning("OctreeScene", false, "Invalid bg_octreeMaxWorldExtents value %f. Using the default value %f", initialRootHalfExtent, DefaultOctreeRootHalfExtent);
            initialRootHalfExtent = DefaultOctreeRootHalfExtent;
        }
        m_root.m_bounds = AZ::Aabb::CreateCenterHalfExtents(AZ::Vector3::CreateZero(), AZ::Vector3(initialRootHalfExtent));
    }

    OctreeScene::~OctreeScene()
    {
        for (auto page : m_nodeCache)
        {
            delete page;
        }
        m_nodeCache.reserve(0);
        m_nodeCache.shrink_to_fit();
    }

    const AZ::Name& OctreeScene::GetName() const
    {
        return m_sceneName;
    }

    bool OctreeScene::GrowToContain(const AZ::Aabb& bounds)
    {
        if (!bounds.IsValid() || !bounds.IsFinite())
        {
            return false;
        }

        if (AZ::ShapeIntersection::Contains(m_root.m_bounds, bounds))
        {
            return true;
        }

        AZ_PROFILE_SCOPE(AzFramework, "OctreeScene::GrowToContain");

        AZ::Aabb startingBounds = m_root.m_bounds;
        if (bg_octreeUseQuadtree)
        {
            AZ::Vector3 rootMin = startingBounds.GetMin();
            AZ::Vector3 rootMax = startingBounds.GetMax();
            rootMin.SetZ(AZStd::min(rootMin.GetZ(), bounds.GetMin().GetZ()));
            rootMax.SetZ(AZStd::max(rootMax.GetZ(), bounds.GetMax().GetZ()));
            startingBounds = AZ::Aabb::CreateFromMinMax(rootMin, rootMax);
            if (!startingBounds.GetExtents().IsFinite()
                || !(rootMin * 2.0f).IsFinite()
                || !(rootMax * 2.0f).IsFinite())
            {
                return false;
            }
        }

        // Preflight all growth so numerical failure leaves the existing tree unchanged.
        AZ::Aabb candidateBounds = startingBounds;
        while (!AZ::ShapeIntersection::Contains(candidateBounds, bounds))
        {
            AZ::Aabb nextBounds;
            uint32_t oldRootChild = 0;
            if (!CalculateNextRootBounds(candidateBounds, bounds, nextBounds, oldRootChild))
            {
                return false;
            }
            candidateBounds = nextBounds;
        }

        if (m_root.IsLeaf())
        {
            m_root.m_bounds = candidateBounds;
            ++m_growthCount;
            return true;
        }

        if (startingBounds != m_root.m_bounds)
        {
            m_root.m_bounds = startingBounds;
            ++m_growthCount;
        }

        while (!AZ::ShapeIntersection::Contains(m_root.m_bounds, bounds))
        {
            AZ::Aabb newBounds;
            uint32_t oldRootChild = 0;
            if (!CalculateNextRootBounds(m_root.m_bounds, bounds, newBounds, oldRootChild))
            {
                return false;
            }

            const uint32_t newChildNodeIndex = AllocateChildNodes();
            OctreeNode* newChildren = GetChildNodesAtIndex(newChildNodeIndex);
            // Moving the old root repairs pointers for entries stored directly in it.
            // Descendants stay in place, so each wrap is O(root-local entries + child count).
            newChildren[oldRootChild] = AZStd::move(m_root);
            m_root.m_bounds = newBounds;
            m_root.m_childNodeIndex = newChildNodeIndex;
            m_root.m_children = newChildren;
            const uint32_t childCount = GetChildNodeCount();
            for (uint32_t child = 0; child < childCount; ++child)
            {
                if (child != oldRootChild)
                {
                    newChildren[child].m_bounds = GetChildBounds(newBounds, child);
                }
                newChildren[child].m_parent = &m_root;
            }
            ++m_growthCount;
        }

        return true;
    }

    void OctreeScene::InsertOrUpdateEntry(VisibilityEntry& entry)
    {
        AZStd::lock_guard<AZStd::shared_mutex> lock(m_sharedMutex);

        if (!GrowToContain(entry.m_boundingVolume))
        {
            if (entry.m_internalNode)
            {
                static_cast<OctreeNode*>(entry.m_internalNode)->Remove(*this, &entry);
                --m_entryCount;
            }
            if (!m_growthWarningIssued)
            {
                AZ_Warning("OctreeScene", false, "Scene %s rejected visibility bounds that cannot be represented safely", m_sceneName.GetCStr());
                m_growthWarningIssued = true;
            }
            return;
        }

        if (entry.m_internalNode != nullptr)
        {
            static_cast<OctreeNode*>(entry.m_internalNode)->Update(*this, &entry);
        }
        else
        {
            m_root.Insert(*this, &entry);
            ++m_entryCount;
        }
    }

    void OctreeScene::RemoveEntry(VisibilityEntry& entry)
    {
        AZStd::lock_guard<AZStd::shared_mutex> lock(m_sharedMutex);
        if (entry.m_internalNode)
        {
            static_cast<OctreeNode*>(entry.m_internalNode)->Remove(*this, &entry);
            --m_entryCount;
        }
    }

    void OctreeScene::Enumerate(const AZ::Aabb& aabb, const IVisibilityScene::EnumerateCallback& callback) const
    {
        AZStd::shared_lock<AZStd::shared_mutex> lock(m_sharedMutex);
        m_root.Enumerate(aabb, callback);
    }

    void OctreeScene::Enumerate(const AZ::Sphere& sphere, const IVisibilityScene::EnumerateCallback& callback) const
    {
        AZStd::shared_lock<AZStd::shared_mutex> lock(m_sharedMutex);
        m_root.Enumerate(sphere, callback);
    }

    void OctreeScene::Enumerate(const AZ::Hemisphere& hemisphere, const IVisibilityScene::EnumerateCallback& callback) const
    {
        AZStd::shared_lock<AZStd::shared_mutex> lock(m_sharedMutex);
        m_root.Enumerate(hemisphere, callback);
    }

    void OctreeScene::Enumerate(const AZ::Capsule & capsule, const IVisibilityScene::EnumerateCallback& callback) const
    {
        AZStd::shared_lock<AZStd::shared_mutex> lock(m_sharedMutex);
        m_root.Enumerate(capsule, callback);
    }

    void OctreeScene::Enumerate(const AZ::Frustum& frustum, const IVisibilityScene::EnumerateCallback& callback) const
    {
        AZStd::shared_lock<AZStd::shared_mutex> lock(m_sharedMutex);
        m_root.Enumerate(frustum, callback);
    }

    void OctreeScene::Enumerate(const AZ::Frustum& includeFrustum, const AZ::Frustum& excludeFrustum, const EnumerateCallback& callback) const
    {
        AZStd::shared_lock<AZStd::shared_mutex> lock(m_sharedMutex);
        m_root.Enumerate(includeFrustum, excludeFrustum, callback);
    }

    void OctreeScene::EnumerateNoCull(const IVisibilityScene::EnumerateCallback& callback) const
    {
        AZStd::shared_lock<AZStd::shared_mutex> lock(m_sharedMutex);
        m_root.EnumerateNoCull(callback);
    }

    uint32_t OctreeScene::GetEntryCount() const
    {
        return m_entryCount;
    }

    uint32_t OctreeScene::GetNodeCount() const
    {
        return m_nodeCount;
    }

    uint32_t OctreeScene::GetFreeNodeCount() const
    {
        // Each entry represents GetChildNodeCount() nodes
        return aznumeric_cast<uint32_t>(m_freeOctreeNodes.size() * GetChildNodeCount());
    }

    uint32_t OctreeScene::GetPageCount() const
    {
        return aznumeric_cast<uint32_t>(m_nodeCache.size());
    }

    uint32_t OctreeScene::GetChildNodeCount() const
    {
        return AzFramework::GetChildNodeCount();
    }

    void OctreeScene::DumpStats()
    {
        AZStd::shared_lock<AZStd::shared_mutex> lock(m_sharedMutex);
        AZ_TracePrintf("Console", "OctreeScene[\"%s\"]::EntryCount = %u", GetName().GetCStr(), GetEntryCount());
        AZ_TracePrintf("Console", "OctreeScene[\"%s\"]::NodeCount = %u", GetName().GetCStr(), GetNodeCount());
        AZ_TracePrintf("Console", "OctreeScene[\"%s\"]::FreeNodeCount = %u", GetName().GetCStr(), GetFreeNodeCount());
        AZ_TracePrintf("Console", "OctreeScene[\"%s\"]::PageCount = %u", GetName().GetCStr(), GetPageCount());
        AZ_TracePrintf("Console", "OctreeScene[\"%s\"]::ChildNodeCount = %u", GetName().GetCStr(), GetChildNodeCount());
        AZ_TracePrintf("Console", "OctreeScene[\"%s\"]::GrowthCount = %u", GetName().GetCStr(), m_growthCount);
        AZ_TracePrintf("Console", "OctreeScene[\"%s\"]::RootBounds = (%f, %f, %f) - (%f, %f, %f)", GetName().GetCStr(), m_root.m_bounds.GetMin().GetX(), m_root.m_bounds.GetMin().GetY(), m_root.m_bounds.GetMin().GetZ(), m_root.m_bounds.GetMax().GetX(), m_root.m_bounds.GetMax().GetY(), m_root.m_bounds.GetMax().GetZ());
    }

    static inline uint32_t CreateNodeIndex(uint32_t page, uint32_t offset)
    {
        AZ_Assert(page <= 0xFFFF && offset <= 0xFFFF, "Out of range values passed to CreateNodeIndex");
        return (page << 16) | offset;
    }

    static inline void ExtractPageAndOffsetFromIndex(uint32_t index, uint32_t& page, uint32_t& offset)
    {
        offset = index & 0x0000FFFF;
        page = index >> 16;
    }

    uint32_t OctreeScene::AllocateChildNodes()
    {
        const uint32_t childCount = GetChildNodeCount();
        m_nodeCount += childCount;

        if (m_nodeCache.empty())
        {
            m_nodeCache.push_back(new OctreeNodePage);
        }

        uint32_t nextChildPage = aznumeric_cast<uint32_t>(m_nodeCache.size() - 1);
        uint32_t nextChildOffset = aznumeric_cast<uint32_t>(m_nodeCache[nextChildPage]->size());

        if (!m_freeOctreeNodes.empty())
        {
            // Take a free block of child nodes from our free list
            ExtractPageAndOffsetFromIndex(m_freeOctreeNodes.top(), nextChildPage, nextChildOffset);
            m_freeOctreeNodes.pop();
        }
        else
        {
            if (nextChildOffset >= BlockSize)
            {
                // Our last page is already full, so we need to allocate a new page
                m_nodeCache.push_back(new OctreeNodePage);
                ++nextChildPage;
                nextChildOffset = 0;
            }

            // Our (potentially new) last page has unused capacity
            m_nodeCache[nextChildPage]->resize_no_construct(nextChildOffset + GetChildNodeCount());

            // We resize_no_construct to prevent fixed_vector from using copy or assignment operators, but this means we have to explicitly construct nodes ourselves
            OctreeNode* childNodes = &(*m_nodeCache[nextChildPage])[nextChildOffset];
            for (uint32_t child = 0; child < childCount; ++child)
            {
                new (&childNodes[child]) OctreeNode;
            }
        }

        return CreateNodeIndex(nextChildPage, nextChildOffset);
    }

    void OctreeScene::ReleaseChildNodes(uint32_t nodeIndex)
    {
        m_nodeCount -= GetChildNodeCount();
        m_freeOctreeNodes.push(nodeIndex);
    }

    OctreeNode* OctreeScene::GetChildNodesAtIndex(uint32_t nodeIndex) const
    {
        uint32_t childPage;
        uint32_t childOffset;
        ExtractPageAndOffsetFromIndex(nodeIndex, childPage, childOffset);
        return &(*m_nodeCache[childPage])[childOffset];
    }

    void OctreeSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<OctreeSystemComponent, AZ::Component>()
                ->Version(1);
        }
    }

    void OctreeSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("OctreeService"));
    }

    void OctreeSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("OctreeService"));
    }

    OctreeSystemComponent::OctreeSystemComponent()        
    {
        AZ::Interface<IVisibilitySystem>::Register(this);
        IVisibilitySystemRequestBus::Handler::BusConnect();

        m_defaultScene = aznew OctreeScene(AZ::Name("DefaultVisibilityScene"));
    }

    OctreeSystemComponent::~OctreeSystemComponent()
    {
        AZ_Assert(m_scenes.empty(), "All IVisibilityScenes must be destroyed before shutdown");

        delete m_defaultScene;

        IVisibilitySystemRequestBus::Handler::BusDisconnect();
        AZ::Interface<IVisibilitySystem>::Unregister(this);        
    }

    void OctreeSystemComponent::Activate()
    {
        ;
    }

    void OctreeSystemComponent::Deactivate()
    {
        ;
    }

    IVisibilityScene* OctreeSystemComponent::GetDefaultVisibilityScene()
    {
        return m_defaultScene;
    }

    IVisibilityScene* OctreeSystemComponent::CreateVisibilityScene(const AZ::Name& sceneName)
    {
        AZ_Assert(FindVisibilityScene(sceneName) == nullptr, "Scene with same name already created!");
        OctreeScene* newScene = aznew OctreeScene(sceneName);
        m_scenes.push_back(newScene);
        return newScene;
    }

    void OctreeSystemComponent::DestroyVisibilityScene(IVisibilityScene* visScene)
    {
        for (auto iter = m_scenes.begin(); iter != m_scenes.end(); ++iter)
        {
            if (*iter == visScene)
            {
                delete visScene;
                m_scenes.erase(iter);
                return;
            }
        }
        AZ_Assert(false, "visScene[\"%s\"] not found in the OctreeSystemComponent", visScene->GetName().GetCStr());
    }

    IVisibilityScene* OctreeSystemComponent::FindVisibilityScene(const AZ::Name& sceneName)
    {
        for (OctreeScene* scene : m_scenes)
        {
            if(scene->GetName() == sceneName)
            {
                return scene;
            }
        }
        return nullptr;
    }

    void OctreeSystemComponent::DumpStats([[maybe_unused]] const AZ::ConsoleCommandContainer& arguments)
    {
        for (OctreeScene* scene : m_scenes)
        {
            AZ_TracePrintf("Console", "============================================");
            scene->DumpStats();
        }
        AZ_TracePrintf("Console", "============================================");
    }
}
