/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Component/EntityId.h>

#include <NvCloth/Types.h>

namespace NvCloth
{
    //! Manages motion and separation constraints for cloth.
    class ClothConstraints
    {
    public:
        AZ_TYPE_INFO(ClothConstraints, "{EB14ED7C-37FD-4CA3-9137-EC6590712E50}");

        static AZStd::unique_ptr<ClothConstraints> Create(
            const AZStd::vector<float>& motionConstraintsData,
            const float motionConstraintsMaxDistance,
            const AZStd::vector<AZ::Vector2>& backstopData,
            const float backstopMaxRadius,
            const float backstopMaxBackOffset,
            const float backstopMaxFrontOffset,
            const AZStd::vector<SimParticleFormat>& simParticles,
            const AZStd::vector<SimIndexType>& simIndices,
            const AZStd::vector<int>& meshRemappedVertices);

        ClothConstraints() = default;

        void CalculateConstraints(
            const AZStd::vector<SimParticleFormat>& simParticles,
            const AZStd::vector<SimIndexType>& simIndices);

        const AZStd::vector<AZ::Vector4>& GetMotionConstraints() const;
        const AZStd::vector<AZ::Vector4>& GetSeparationConstraints() const;

        void SetMotionConstraintMaxDistance(float distance);
        void SetBackstopMaxRadius(float radius);
        void SetBackstopMaxOffsets(float backOffset, float frontOffset);

    private:
        void CalculateMotionConstraints();
        void CalculateSeparationConstraints();
        AZ::Vector3 CalculateBackstopSpherePosition(
            const AZ::Vector3& position,
            const AZ::Vector3& normal,
            float offset,
            float radius) const;

        // Simulation Particles
        AZStd::vector<SimParticleFormat> m_simParticles;

        // Motion constraints data
        AZStd::vector<float> m_motionConstraintsData;
        float m_motionConstraintsMaxDistance = 0.0f;

        // Backstop data
        AZStd::vector<AZ::Vector2> m_backstopData;
        float m_backstopMaxRadius = 0.0f;
        float m_backstopMaxBackOffset = 0.0f;
        float m_backstopMaxFrontOffset = 0.0f;
        AZStd::vector<AZ::Vector3> m_normals;

        // The current positions and radius of motion constraints.
        AZStd::vector<AZ::Vector4> m_motionConstraints;

        // The current positions and radius of separation constraints.
        AZStd::vector<AZ::Vector4> m_separationConstraints;
    };
} // namespace NvCloth
