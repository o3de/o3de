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
        bool            m_isDeformable;
        unsigned int    m_verticesCount;
        unsigned int    m_indicesCount;
        unsigned int    m_polygonsCount;
        bool            m_isTriangleMesh;
        bool            m_isQuadMesh;
        unsigned int    m_orgVerticesCount;
        float           m_vertexDupeRatio;
        AZStd::vector<unsigned int> m_verticesByInfluences;
        AZStd::vector<SubMeshInfo> m_submeshes;
        AZStd::vector<NamedPropertyStringValue> m_attributeLayers;
        AZStd::vector<NamedPropertyStringValue> m_sharedAttributeLayers;
    };
} // namespace EMStudio
