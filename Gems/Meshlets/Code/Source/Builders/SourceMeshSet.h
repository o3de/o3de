/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Asset/AssetCommon.h>

namespace AZ::Meshlets::Builders
{
    //! Per-mesh source data the builder consumes. Both ingestion paths normalize
    //! to this shape:
    //!   - SceneAPI export module: walks IMeshNodes in the FBX scene graph.
    //!   - JSON sidecar builder: loads the referenced .azmodel and pulls LOD 0
    //!     meshes from RPI::ModelAsset.
    struct SourceMesh
    {
        AZStd::string m_name;

        // Vertex streams -- interleaved per-attribute, total count = vertex count.
        // All five must be populated; an absent UV/Tangent/Bitangent in the
        // source must be synthesized (zero-fill or reconstructed) before this
        // struct is built.
        AZStd::vector<float> m_positions;   //!< 3 floats per vertex
        AZStd::vector<float> m_normals;     //!< 3 floats per vertex
        AZStd::vector<float> m_tangents;    //!< 4 floats per vertex (xyz + handedness)
        AZStd::vector<float> m_bitangents;  //!< 3 floats per vertex
        AZStd::vector<float> m_uv0;         //!< 2 floats per vertex

        // Triangle index buffer, 3 indices per triangle, indices into the
        // streams above. Must be u32 (16-bit sources are widened by the
        // caller).
        AZStd::vector<AZ::u32> m_indices;
    };

    //! One logical mesh together with its source LOD chain. m_lods[0] is the
    //! finest level (LOD0); higher indices are progressively coarser. The
    //! ingestion path fills as many levels as the source model ships; the
    //! builder (MeshletPackBuilderCore) generates the remaining coarser LODs
    //! from LOD0 via meshopt_simplify when fewer than the target K are present.
    struct SourceMeshLods
    {
        AZStd::string m_name;
        AZStd::vector<SourceMesh> m_lods;
    };

    //! Top-level builder input.
    struct SourceMeshSet
    {
        //! Source AssetId of the .azmodel that this pack will be associated
        //! with. Stored verbatim in the pack's PackHeader.
        AZ::Data::AssetId m_sourceModelAssetId;

        //! One entry per logical mesh; each carries its LOD chain (m_lods[0] = LOD0).
        AZStd::vector<SourceMeshLods> m_meshes;

        // Builder-config copied from MeshletPackRule / JSON sidecar.
        AZ::u16 m_maxVerticesPerCluster  = 64;
        AZ::u16 m_maxTrianglesPerCluster = 64;
        float   m_coneWeight             = 0.5f;
        //! Phase 6: build a cluster-simplification DAG (v3 pack with DagNodes)
        //! instead of the discrete LOD chain. See JsonSidecarDescriptor.
        bool    m_generateClusterDag     = false;
        //! Phase 7: emit leaf streaming pages (v4). Implies m_generateClusterDag.
        bool    m_generatePages          = false;
    };

} // namespace AZ::Meshlets::Builders
