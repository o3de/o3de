/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Material/BlastMaterial.h>

namespace Blast
{
    Material::Material(const MaterialConfiguration& configuration)
    {
        m_health = configuration.m_health;
        m_stressLinearFactor = configuration.m_stressLinearFactor;
        m_stressAngularFactor = configuration.m_stressAngularFactor;

        m_blastMaterial.health = configuration.m_forceDivider; // This is not an error, health in ExtPxMaterial is actually damage divider and not health.
        m_blastMaterial.minDamageThreshold = configuration.m_minDamageThreshold;
        m_blastMaterial.maxDamageThreshold = configuration.m_maxDamageThreshold;
    }

    float Material::GetHealth() const
    {
        return m_health;
    }

    float Material::GetForceDivider() const
    {
        return m_blastMaterial.health;
    }

    float Material::GetMinDamageThreshold() const
    {
        return m_blastMaterial.minDamageThreshold;
    }

    float Material::GetMaxDamageThreshold() const
    {
        return m_blastMaterial.maxDamageThreshold;
    }

    float Material::GetStressLinearFactor() const
    {
        return m_stressLinearFactor;
    }

    float Material::GetStressAngularFactor() const
    {
        return m_stressAngularFactor;
    }

    float Material::GetNormalizedDamage(float damage) const
    {
        // Same normalization behavior as NvBlastExtMaterial::getNormalizedDamage function.
        // Since the damage input parameter is expected in the right range already, it's better
        // to do the operation directly rather than passing a scaled value of damage expecting
        // getNormalizedDamage to invert that scale to get the original value, which due
        // to floating precision issues it results in an slightly different number, causing
        // the normalization to return unexpected results in the limits.
        damage = (m_blastMaterial.health > 0.0f) ? damage : 1.0f;
        if (damage > m_blastMaterial.minDamageThreshold)
        {
            return AZ::GetMin(damage, m_blastMaterial.maxDamageThreshold);
        }
        else
        {
            return 0.0f;
        }
    }

    Nv::Blast::ExtStressSolverSettings Material::GetStressSolverSettings(uint32_t iterationCount) const
    {
        Nv::Blast::ExtStressSolverSettings settings;
        settings.hardness = m_blastMaterial.health;
        settings.stressLinearFactor = m_stressLinearFactor;
        settings.stressAngularFactor = m_stressAngularFactor;
        settings.graphReductionLevel = 0;
        settings.bondIterationsPerFrame = iterationCount;
        return settings;
    }

    const void* Material::GetNativePointer() const
    {
        return &m_blastMaterial;
    }
} // namespace Blast
