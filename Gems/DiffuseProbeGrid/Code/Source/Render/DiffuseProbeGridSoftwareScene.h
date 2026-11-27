/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Render/DiffuseProbeGridSoftwareBvh.h>
#include <AzCore/Math/Matrix3x4.h>
#include <Atom/Feature/Mesh/MeshFeatureProcessorInterface.h>
#include <Atom/RPI.Reflect/Buffer/BufferAsset.h>

namespace AZ::Render
{
    //! Gem-private scene snapshot. MeshInfo supplies current geometry, materials and transforms on both tracing backends.
    //! This owns no mesh handles and never changes model assets or their CPU buffer retention policy.
    class DiffuseProbeGridSoftwareScene
    {
    public:
        struct Geometry
        {
            Data::Asset<RPI::ModelAsset> m_modelAsset;
            uint32_t m_lod = 0;
            uint32_t m_mesh = 0;
            DiffuseProbeGridSoftwareBvh m_bvh;
            Data::Asset<RPI::BufferAsset> m_positions;
            Data::Asset<RPI::BufferAsset> m_indices;
            bool m_ready = false;
            bool m_used = false;
        };

        struct Instance
        {
            AZStd::shared_ptr<Geometry> m_geometry;
            RHI::Ptr<MeshInfoEntry> m_meshInfo;
            MeshInfoHandle m_meshInfoHandle;
            Aabb m_worldBounds = Aabb::CreateNull();
            bool m_skinned = false;
            Matrix3x4 m_transform = Matrix3x4::CreateIdentity();
        };

        struct GpuInstance
        {
            uint32_t m_topologyBase;
            uint32_t m_boundsBase;
            uint32_t m_primitiveBase;
            uint32_t m_meshInfoIndex;
        };

        struct RefitWork
        {
            uint32_t m_node;
            uint32_t m_topologyBase;
            uint32_t m_boundsBase;
            uint32_t m_primitiveBase;
            uint32_t m_meshInfoIndex;
            uint32_t m_kind; //!< 0: triangle leaf, 1: internal node, 2: instance leaf.
            uint32_t m_padding[2] = {};
        };

        struct GpuData
        {
            AZStd::vector<DiffuseProbeGridSoftwareBvh::Node> m_nodes;
            AZStd::vector<DiffuseProbeGridSoftwareBvh::Bounds> m_bounds;
            AZStd::vector<uint32_t> m_primitiveIndices;
            AZStd::vector<GpuInstance> m_instances;
            AZStd::vector<AZStd::vector<RefitWork>> m_initialRefit;
            AZStd::vector<AZStd::vector<RefitWork>> m_dynamicRefit;
            uint32_t m_tlasTopologyBase = 0;
            uint32_t m_tlasBoundsBase = 0;
            uint32_t m_tlasPrimitiveBase = 0;
        };

        //! Pack shared topology and static bounds once; animated instances receive private bounds. The caller uploads
        //! the result only when instance membership/topology changes. Transforms and materials remain in SceneSrg.
        GpuData BuildGpuData() const;

        //! Called after simulation. Includes off-screen geometry; hidden and ray-tracing-disabled meshes are omitted.
        void Update(MeshFeatureProcessorInterface& meshes, TransformServiceFeatureProcessorInterface& transforms);
        void Clear();

        const AZStd::vector<Instance>& GetInstances() const { return m_instances; }

    private:
        static bool BuildGeometry(Geometry& geometry);
        AZStd::vector<AZStd::shared_ptr<Geometry>> m_geometries;
        AZStd::vector<Instance> m_instances;
    };
}

static_assert(sizeof(AZ::Render::DiffuseProbeGridSoftwareScene::GpuInstance) == 16);
static_assert(sizeof(AZ::Render::DiffuseProbeGridSoftwareScene::RefitWork) == 32);
