/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

namespace AZ::Render
{
    //! Inputs deciding whether a submesh instance is rendered by the GPU-driven shaded path (Phase 1).
    //! Phase 1 only claims opaque, static, StandardPBR submeshes that contribute to the forward draw list;
    //! everything else stays on the CPU MeshDrawPacket path.
    struct GpuDrivenEligibilityInputs
    {
        bool m_cvarEnabled = false;          //!< r_gpuDrivenRendering AND the scene has a GpuDrivenForwardPass
                                             //!< (see MeshFeatureProcessor::CheckForInstancingCVarChange)
        bool m_isOpaque = false;             //!< OpacityMode == Opaque
        bool m_isStatic = false;             //!< not dynamic / not skinned
        bool m_isStandardPbr = false;        //!< material type is StandardPBR (the only wired type this phase)
        bool m_contributesToForward = false; //!< drawListMask includes the "forward" tag
        //! r_meshInstancingEnabled is OFF. Under mesh instancing a single MeshDrawPacket is shared
        //! by every member of an instance group, so the CPU-side forward suppression below cannot be
        //! applied per-instance -- disabling that packet would hide the group's ineligible members
        //! (e.g. a dynamic instance sharing a mesh with a static one) too. Rather than silently
        //! double-draw in that configuration, Phase 1 simply does not claim instanced meshes.
        bool m_meshInstancingOff = false;
    };

    //! True iff the instance must be drawn by the GPU-driven path (and therefore skipped on the CPU path).
    //! This is the single source of truth for CPU/GPU coexistence: an eligible instance is drawn once by
    //! the GPU path and never by the CPU path (no double-draw); an ineligible instance is never drawn by GPU.
    inline bool IsGpuDrivenEligible(const GpuDrivenEligibilityInputs& in)
    {
        return in.m_cvarEnabled
            && in.m_isOpaque
            && in.m_isStatic
            && in.m_isStandardPbr
            && in.m_contributesToForward
            && in.m_meshInstancingOff;
    }
}
