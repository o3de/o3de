/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Debug/Budget.h>
#include <AzCore/RTTI/RTTI.h>

#include <NvCloth/Types.h>

AZ_DECLARE_BUDGET(Cloth);

namespace NvCloth
{
    //! Interface that provides a set of functions to
    //! calculate tangent space information for cloth's particles.
    //!
    //! @note Use AZ::Interface<ITangentSpaceHelper>::Get() to call the interface, which
    //!       is available at both runtime and asset processing time (asset builders).
    class ITangentSpaceHelper
    {
    public:
        AZ_RTTI(ITangentSpaceHelper, "{1D96A3F2-7EC7-4DD0-A874-7C2ED4D6F360}");

        virtual ~ITangentSpaceHelper() = default;

        //! Calculates the normals of a simulation mesh.
        //!
        //! @param vertices List of particles, which are composed of positions and inverse masses.
        //! @param indices List of triangles' indices.
        //! @param outNormals The resulted list of normals.
        //! @return Whether it was able to calculate the normals or not.
        virtual bool CalculateNormals(
            const AZStd::vector<SimParticleFormat>& vertices,
            const AZStd::vector<SimIndexType>& indices,
            AZStd::vector<AZ::Vector3>& outNormals) = 0;

        //! Calculates the tangents and bitangents of a simulation mesh.
        //!
        //! @param vertices List of particles, which are composed of positions and inverse masses.
        //! @param indices List of triangles' indices.
        //! @param uvs List of UVs coordinates of the mesh.
        //! @param normals List of normals of the mesh.
        //! @param outTangents The resulted list of tangents.
        //! @param outBitangents The resulted list of bitangents.
        //! @return Whether it was able to calculate the tangents and bitangents or not.
        virtual bool CalculateTangentsAndBitagents(
            const AZStd::vector<SimParticleFormat>& vertices,
            const AZStd::vector<SimIndexType>& indices,
            const AZStd::vector<SimUVType>& uvs,
            const AZStd::vector<AZ::Vector3>& normals,
            AZStd::vector<AZ::Vector3>& outTangents,
            AZStd::vector<AZ::Vector3>& outBitangents) = 0;

        //! Calculates the tangents, bitangents and normals of a simulation mesh.
        //!
        //! @param vertices List of particles, which are composed of positions and inverse masses.
        //! @param indices List of triangles' indices.
        //! @param uvs List of UVs coordinates of the mesh.
        //! @param outTangents The resulted list of tangents.
        //! @param outBitangents The resulted list of bitangents.
        //! @param outNormals The resulted list of normals.
        //! @return Whether it was able to calculate the tangents, bitangents and normals or not.
        virtual bool CalculateTangentSpace(
            const AZStd::vector<SimParticleFormat>& vertices,
            const AZStd::vector<SimIndexType>& indices,
            const AZStd::vector<SimUVType>& uvs,
            AZStd::vector<AZ::Vector3>& outTangents,
            AZStd::vector<AZ::Vector3>& outBitangents,
            AZStd::vector<AZ::Vector3>& outNormals) = 0;
    };
} // namespace NvCloth
