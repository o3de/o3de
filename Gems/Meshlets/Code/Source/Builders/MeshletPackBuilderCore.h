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
#include <Builders/SourceMeshSet.h>

namespace AZ::Meshlets::Builders
{
    struct BuildResult
    {
        bool m_success = false;
        AZStd::string m_errorMessage;
        AZStd::vector<AZ::u8> m_packBytes;  //!< Empty on failure.
    };

    //! Offline meshletizer. Lifts what `MeshletsRenderObject::CreateMeshlets*`
    //! does at runtime today into a builder-side function:
    //!   1. meshopt_optimizeVertexFetch (per-mesh, reorders for cluster locality)
    //!   2. meshopt_buildMeshlets (max_vertices/max_triangles/cone_weight)
    //!   3. encode triangle indices as 3x8-bit-in-u32 (top byte zero)
    //!   4. emit .azmeshletpack v1 with kinds 0-5 populated
    //!
    //! Deterministic: same SourceMeshSet -> byte-identical pack output.
    BuildResult BuildPackBytes(const SourceMeshSet& source);

} // namespace AZ::Meshlets::Builders
