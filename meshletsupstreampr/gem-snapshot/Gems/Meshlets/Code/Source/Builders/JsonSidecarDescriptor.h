/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    class ReflectContext;
}

namespace AZ::Meshlets::Builders
{
    //! Reflected shape of a *.meshletpack source JSON file.
    //!
    //! The source AssetId is stored on disk as a string in the standard
    //! "{uuid}:subid" textual form. AZ::Data::AssetId itself isn't a
    //! supported JSON deserialization target (the JSON serializer emits
    //! "Reading into targets of type 'AssetId' is not supported"), so we
    //! load it as a string here and convert via AssetId::CreateString in
    //! JsonSidecarBuilder::ProcessJob.
    struct JsonSidecarDescriptor
    {
        AZ_TYPE_INFO(JsonSidecarDescriptor, "{B71E4D2A-3F8C-4E5A-9B6D-1C3E7F4A2D81}");

        AZStd::string m_sourceModelAssetIdStr;
        //! Cluster budgets (defaults raised from 64/64). Larger clusters => fewer
        //! per-cluster draw commands + better vertex-cache reuse. The builder clamps
        //! to meshopt limits (verts<=255, tris<=512 & multiple-of-4), so a sidecar may
        //! request any value safely. Omitting a field in the .meshlet JSON uses these.
        AZ::u16  m_maxVerticesPerCluster  = 128;
        AZ::u16  m_maxTrianglesPerCluster = 256;
        float    m_coneWeight             = 0.5f;
        //! Phase 6: build a cluster-simplification DAG (continuous per-cluster LOD for
        //! the AS mesh-shader path) instead of the discrete per-instance LOD chain.
        //! Produces a v3 pack with a DagNodes section; leaves stay contiguous-first so
        //! non-DAG-aware paths draw exactly the LOD0 cluster set.
        bool     m_generateClusterDag     = false;
        //! Phase 7: additionally emit self-contained LEAF streaming pages (PageTable/
        //! PageData/ParentIndex sections, pack v4). Implies m_generateClusterDag.
        bool     m_generatePages          = false;
        AZStd::vector<AZStd::string> m_meshFilter;

        static void Reflect(ReflectContext* context);
    };

} // namespace AZ::Meshlets::Builders
