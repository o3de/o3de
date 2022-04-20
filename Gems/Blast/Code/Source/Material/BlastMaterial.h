/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Material/BlastMaterialConfiguration.h>

#include <NvBlastExtDamageShaders.h>
#include <NvBlastExtStressSolver.h>

namespace Blast
{
    //! Runtime blast material, created from a MaterialConfiguration.
    class Material
    {
    public:
        Material(const MaterialConfiguration& configuration);

        //! Amount of damage destructible object with this material can withstand.
        float GetHealth() const;

        //! Amount by which magnitude of stress forces applied is divided before being subtracted from health.
        float GetForceDivider() const;

        //! Any amount lower than this threshold will not be applied. Only affects non-stress damage.
        float GetMinDamageThreshold() const;

        //! Any amount higher than this threshold will be capped by it. Only affects non-stress damage.
        float GetMaxDamageThreshold() const;

        //! Factor with which linear stress is applied to destructible objects. Linear stress includes
        //! direct application of BlastFamilyDamageRequests::StressDamage, collisions and gravity (only for static
        //! actors).
        float GetStressLinearFactor() const;

        //! Factor with which angular stress is applied to destructible objects. Angular stress is
        //! calculated based on angular velocity of an object (only non-static actors).
        float GetStressAngularFactor() const;

        //! Normalizes the non-stress damage based on the thresholds.
        float GetNormalizedDamage(float damage) const;

        //! Generates NvBlast stress solver settings from this material and provided iterationCount.
        Nv::Blast::ExtStressSolverSettings GetStressSolverSettings(uint32_t iterationCount) const;

        //! Returns underlying pointer of the native type.
        const void* GetNativePointer() const;

    private:
        float m_health;
        float m_stressLinearFactor;
        float m_stressAngularFactor;
        NvBlastExtMaterial m_blastMaterial;
    };
} // namespace Blast
