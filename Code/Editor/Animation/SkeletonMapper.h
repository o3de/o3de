/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_ANIMATION_SKELETONMAPPER_H
#define CRYINCLUDE_EDITOR_ANIMATION_SKELETONMAPPER_H
#pragma once


#include "SkeletonHierarchy.h"
#include "SkeletonMapperOperator.h"

namespace Skeleton {
    class CMapper
    {
    public:
        struct SNode
        {
            _smart_ptr<CMapperOperator> position;
            _smart_ptr<CMapperOperator> orientation;
        };

    public:
        CMapper();
        ~CMapper();

    public:
        CHierarchy& GetHierarchy() { return m_hierarchy; }
        void CreateFromHierarchy();

        uint32 GetNodeCount() const { return uint32(m_nodes.size()); }
        SNode* GetNode(uint32 index) { return &m_nodes[index]; }
        const SNode* GetNode(uint32 index) const { return &m_nodes[index]; }

        uint32 CreateLocation(const char* name);
        void ClearLocations();
        int32 FindLocation(const char* name) const;

        uint32 GetLocationCount() const { return uint32(m_locations.size()); }
        void SetLocation(CMapperLocation& location);
        CMapperLocation* GetLocation(uint32 index) { return m_locations[index]; }
        const CMapperLocation* GetLocation(uint32 index)  const { return m_locations[index]; }

        bool CreateLocationsHierarchy(CHierarchy& hierarchy);

        void Map(QuatT* pResult);

        bool SerializeTo(XmlNodeRef& node);
        bool SerializeFrom(XmlNodeRef& node);

    private:
        bool NodeHasLocation(uint32 index);
        bool ChildrenHaveLocation(uint32 index);
        bool NodeOrChildrenHaveLocation(uint32 index);

        bool SerializeFrom(XmlNodeRef& node, int32 parent);

        bool CreateLocationsHierarchy(uint32 index, CHierarchy& hierarchy, int32 hierarchyParent = -1);

        // TEMP
        void GetChildrenIndices(uint32 parent, std::vector<uint32>& children);

    private:
        CHierarchy m_hierarchy;
        std::vector<_smart_ptr<CMapperLocation> > m_locations;

        std::vector<SNode> m_nodes;
    };
} // namespace Skeleton

#endif // CRYINCLUDE_EDITOR_ANIMATION_SKELETONMAPPER_H
