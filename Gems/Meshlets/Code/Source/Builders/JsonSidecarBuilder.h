/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <AzCore/std/string/string.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>

namespace AZ::Meshlets::Builders
{
    //! Watches *.meshletpack source JSON files. For each, loads the referenced
    //! .azmodel, walks its LOD-0 meshes, calls BuildPackBytes, writes the
    //! product alongside the source.
    class JsonSidecarBuilder
    {
    public:
        static constexpr const char* JobKey      = "Meshlet Pack JSON Builder";
        static constexpr const char* SourceExt   = "*.meshletpack";
        static constexpr const char* ProductExt  = ".azmeshletpack";
        // v4: JobProduct now records the correct MeshletPackAsset type id
        // (previously used CreateRandom, which left every .azmeshletpack entry
        // in the catalog with a random type -- PackResolver filters by type to
        // find packs, so the editor's runtime warning "No .azmeshletpack
        // product registered" fired even when the product existed). Bumping
        // forces AP to re-emit all products with the correct type so the
        // catalog's m_assetType matches azrtti_typeid<MeshletPackAsset>().
        // v5: pack format v2 -- adds SectionKind::ExpandedIndices so the
        //     runtime can point an SRV at pre-baked flat triangle indices
        //     (no runtime CPU expansion, no compute-pass cross-pass barrier).
        // v6: Phase 6 -- adds SectionKind::ConeBounds (per-cluster bounding
        //     sphere + normal cone) for GPU frustum + backface cluster culling.
        // v7: cluster-budget defaults raised 64/64 -> 128/256 and the import-rule
        //     range widened to the meshopt caps (verts<=255, tris<=512 multiple-of-4),
        //     with build-time clamping. Bump forces a re-bake so existing models pick
        //     up the larger clusters (fewer per-cluster draws / better cache).
        // v8: LOD system -- packs now carry K LODs per mesh (MeshDescriptorPrefix
        //     .m_lodCount > 1 with K MeshDescriptorLodEntry records, each owning its
        //     own cluster/vertex/triangle slice). LODs are baked from the source
        //     model's own LODs and, where absent, generated from LOD0 via
        //     meshopt_simplify. Bump forces a re-bake so existing 1-LOD packs are
        //     replaced with multi-LOD packs (the runtime selects LOD per-instance by
        //     screen coverage). The on-disk format is unchanged (m_lodCount was
        //     always present); a stale 1-LOD pack still loads via the m_lodCount=1
        //     path, so no PackVersion bump is required.
        // v9: adds SectionKind::LodError (meshopt_simplify's resultError per LOD,
        //     used as-is -- meshoptimizer already normalizes it relative to mesh
        //     extents, so no extra AABB-diagonal division is applied on top).
        //     Bump forces a re-bake so existing packs pick up per-LOD geometric
        //     error. The on-disk format is
        //     purely additive (a new optional section, like ConeBounds); a pack
        //     without it simply falls back to screen-coverage LOD selection, so no
        //     PackVersion bump is required and no manual rebuild is needed for
        //     correctness -- only to opt into the better metric.
        static constexpr AZ::u32     BuilderVersion = 9;

        void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request,
                        AssetBuilderSDK::CreateJobsResponse& response);
        void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request,
                        AssetBuilderSDK::ProcessJobResponse& response);

        //! Cancel hook. AssetBuilder may invoke this from another thread.
        void OnCancel();

    private:
        AZStd::atomic_bool m_cancelled{ false };
    };

} // namespace AZ::Meshlets::Builders
