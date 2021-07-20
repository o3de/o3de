/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Interface/Interface.h>

#include <Components/ClothComponentMesh/ClothConstraints.h>

#include <NvCloth/ITangentSpaceHelper.h>

namespace NvCloth
{
    AZStd::unique_ptr<ClothConstraints> ClothConstraints::Create(
        const AZStd::vector<float>& motionConstraintsData,
        const float motionConstraintsMaxDistance,
        const AZStd::vector<AZ::Vector2>& backstopData,
        const float backstopMaxRadius,
        const float backstopMaxBackOffset,
        const float backstopMaxFrontOffset,
        const AZStd::vector<SimParticleFormat>& simParticles,
        const AZStd::vector<SimIndexType>& simIndices,
        const AZStd::vector<int>& meshRemappedVertices)
    {
        AZStd::unique_ptr<ClothConstraints> clothConstraints = AZStd::make_unique<ClothConstraints>();

        clothConstraints->m_motionConstraintsData.resize(simParticles.size(), AZ::Constants::FloatMax);
        clothConstraints->m_motionConstraintsMaxDistance = motionConstraintsMaxDistance;
        clothConstraints->m_motionConstraints.resize(simParticles.size());

        for (size_t i = 0; i < motionConstraintsData.size(); ++i)
        {
            const int remappedIndex = meshRemappedVertices[i];
            if (remappedIndex < 0)
            {
                // Removed particle
                continue;
            }

            // Keep the minimum distance for remapped duplicated vertices
            if (motionConstraintsData[i] < clothConstraints->m_motionConstraintsData[remappedIndex])
            {
                clothConstraints->m_motionConstraintsData[remappedIndex] = motionConstraintsData[i];
            }
        }

        const bool hasBackstopData = AZStd::any_of(
            backstopData.cbegin(),
            backstopData.cend(),
            [](const AZ::Vector2& backstop)
            {
                const float radius = backstop.GetY();
                return radius > 0.0f;
            });
        if (hasBackstopData)
        {
            clothConstraints->m_backstopData.resize(simParticles.size(), AZ::Vector2(0.0f, AZ::Constants::FloatMax));
            clothConstraints->m_backstopMaxRadius = backstopMaxRadius;
            clothConstraints->m_backstopMaxBackOffset = backstopMaxBackOffset;
            clothConstraints->m_backstopMaxFrontOffset = backstopMaxFrontOffset;
            clothConstraints->m_separationConstraints.resize(simParticles.size());

            for (size_t i = 0; i < backstopData.size(); ++i)
            {
                const int remappedIndex = meshRemappedVertices[i];
                if (remappedIndex < 0)
                {
                    // Removed particle
                    continue;
                }

                // Keep the minimum radius for remapped duplicated vertices
                if (backstopData[i].GetY() < clothConstraints->m_backstopData[remappedIndex].GetY())
                {
                    clothConstraints->m_backstopData[remappedIndex] = backstopData[i];
                }
            }
        }

        // Calculates the current constraints and fills the data as nvcloth needs them,
        // ready to be queried by the cloth component.
        clothConstraints->CalculateConstraints(simParticles, simIndices);

        return clothConstraints;
    }

    void ClothConstraints::CalculateConstraints(
        const AZStd::vector<SimParticleFormat>& simParticles,
        const AZStd::vector<SimIndexType>& simIndices)
    {
        if (simParticles.size() != m_motionConstraints.size())
        {
            return;
        }

        m_simParticles = simParticles;

        CalculateMotionConstraints();

        if (!m_separationConstraints.empty())
        {
            [[maybe_unused]] bool normalsCalculated = AZ::Interface<ITangentSpaceHelper>::Get()->CalculateNormals(simParticles, simIndices, m_normals);
            AZ_Assert(normalsCalculated, "Cloth constraints failed to calculate normals.");

            CalculateSeparationConstraints();
        }
    }

    const AZStd::vector<AZ::Vector4>& ClothConstraints::GetMotionConstraints() const
    {
        return m_motionConstraints;
    }

    const AZStd::vector<AZ::Vector4>& ClothConstraints::GetSeparationConstraints() const
    {
        return m_separationConstraints;
    }

    void ClothConstraints::SetMotionConstraintMaxDistance(float distance)
    {
        m_motionConstraintsMaxDistance = distance;

        CalculateMotionConstraints();
    }

    void ClothConstraints::SetBackstopMaxRadius(float radius)
    {
        m_backstopMaxRadius = radius;

        CalculateSeparationConstraints();
    }

    void ClothConstraints::SetBackstopMaxOffsets(float backOffset, float frontOffset)
    {
        m_backstopMaxBackOffset = backOffset;
        m_backstopMaxFrontOffset = frontOffset;

        CalculateSeparationConstraints();
    }

    void ClothConstraints::CalculateMotionConstraints()
    {
        for (size_t i = 0; i < m_motionConstraints.size(); ++i)
        {
            const float maxDistance = (m_simParticles[i].GetW() > 0.0f)
                ? m_motionConstraintsData[i] * m_motionConstraintsMaxDistance
                : 0.0f;

            m_motionConstraints[i].Set(m_simParticles[i].GetAsVector3(), maxDistance);
        }
    }

    void ClothConstraints::CalculateSeparationConstraints()
    {
        for (size_t i = 0; i < m_separationConstraints.size(); ++i)
        {
            const float offsetScale = m_backstopData[i].GetX();
            const float offset = offsetScale * ((offsetScale >= 0.0f) ? m_backstopMaxBackOffset : m_backstopMaxFrontOffset);

            const float radiusScale = m_backstopData[i].GetY();
            const float radius = radiusScale * m_backstopMaxRadius;

            const AZ::Vector3 position = CalculateBackstopSpherePosition(
                m_simParticles[i].GetAsVector3(), m_normals[i], offset, radius);

            m_separationConstraints[i].Set(position, radius);
        }
    }

    AZ::Vector3 ClothConstraints::CalculateBackstopSpherePosition(
        const AZ::Vector3& position,
        const AZ::Vector3& normal,
        float offset,
        float radius) const
    {
        AZ::Vector3 spherePosition = position;
        if (offset >= 0.0f)
        {
            spherePosition -= normal * (radius + offset); // Place sphere behind the particle
        }
        else
        {
            spherePosition += normal * (radius - offset); // Place sphere in front of the particle
        }
        return spherePosition;
    }
} // namespace NvCloth
