/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/map.h>
#include <AzCore/std/string/string.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/NodeWindow/NamedPropertyStringValue.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/NodeWindow/SubMeshInfo.h>

namespace EMotionFX
{
    class Actor;
    class Mesh;
    class Node;
}

namespace EMStudio
{
    class MeshInfo final
    {
    public:
        AZ_RTTI(MeshInfo, "{19988140-5D60-4303-B294-D7C2B5C631FB}")
        AZ_CLASS_ALLOCATOR_DECL

        MeshInfo() {}
        MeshInfo(EMotionFX::Actor* actor, EMotionFX::Node* node, unsigned int lodLevel, EMotionFX::Mesh* mesh);
        ~MeshInfo() = default;

        static void Reflect(AZ::ReflectContext* context);

    private:
        unsigned int    m_lod;
        unsigned int    m_verticesCount;
        unsigned int    m_indicesCount;
        unsigned int    m_polygonsCount;
        bool            m_isTriangleMesh;
        bool            m_isQuadMesh;
        unsigned int    m_orgVerticesCount;
        float           m_vertexDupeRatio;
        AZStd::vector<uint32> m_verticesByInfluences;
        AZStd::vector<SubMeshInfo> m_submeshes;
        AZStd::vector<NamedPropertyStringValue> m_attributeLayers;
        AZStd::vector<NamedPropertyStringValue> m_sharedAttributeLayers;
    };
} // namespace EMStudio
