/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_ANIMATION_SKELETONHIERARCHY_H
#define CRYINCLUDE_EDITOR_ANIMATION_SKELETONHIERARCHY_H
#pragma once

namespace Skeleton {
    class CHierarchy
        : public _reference_target_t
    {
    public:
        struct SNode
        {
            string name;
            QuatT pose;

            int32 parent;

            /* TODO: Implement
                    uint32 childrenIndex;
                    uint32 childrenCount;
            */
        };

    public:
        CHierarchy();
        ~CHierarchy();

    public:
        uint32 AddNode(const char* name, const QuatT& pose, int32 parent = -1);
        uint32 GetNodeCount() const { return uint32(m_nodes.size()); }
        SNode* GetNode(uint32 index) { return &m_nodes[index]; }
        const SNode* GetNode(uint32 index) const { return &m_nodes[index]; }
        int32 FindNodeIndexByName(const char* name) const;
        const SNode* FindNode(const char* name) const;
        void ClearNodes() { m_nodes.clear(); }

        void CreateFrom(IDefaultSkeleton* rIDefaultSkeleton);
        void ValidateReferences();

        void AbsoluteToRelative(const QuatT* pSource, QuatT* pDestination);

        bool SerializeTo(XmlNodeRef& node);

    private:
        std::vector<SNode> m_nodes;
    };
} // namespace Skeleton

#endif // CRYINCLUDE_EDITOR_ANIMATION_SKELETONHIERARCHY_H
