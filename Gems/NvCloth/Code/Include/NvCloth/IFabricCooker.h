/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/optional.h>

#include <NvCloth/Types.h>

namespace NvCloth
{
    //! Interface to cook particles into fabric.
    //!
    //! @note Use AZ::Interface<IFabricCooker>::Get() to call the interface, which
    //!       is available at both runtime and asset processing time (asset builders).
    class IFabricCooker
    {
    public:
        AZ_RTTI(IFabricCooker, "{32E97A6F-A32C-42D2-8BA9-83896E57FA72}");

        virtual ~IFabricCooker() = default;

        //! Generates fabric cooked data from particles information, this data will be used to create cloths instances.
        //! Cooking a fabric can be computationally expensive when using many particles and indices. When possible,
        //! cook at asset processing time (asset builders) to have it ready at runtime.
        //!
        //! @param particles List of particles, which are composed of positions and inverse masses.
        //! @param indices List of triangles' indices.
        //! @param fabricGravity Gravity value to use to cook the fabric.
        //! @param useGeodesicTether Whether use geodesic distance  (using triangle adjacencies) or vertex distance when calculating tether constraints.
        //!                          Using geodesic distance is more expensive during the cooking process, but it results in a more realistic cloth
        //!                          behavior when applying tether constraints.
        //! @return The fabric cooked data or nullopt if cooking process failed.
        virtual AZStd::optional<FabricCookedData> CookFabric(
            const AZStd::vector<SimParticleFormat>& particles,
            const AZStd::vector<SimIndexType>& indices,
            const AZ::Vector3& fabricGravity = AZ::Vector3(0.0f, 0.0f, -9.81f),
            bool useGeodesicTether = true) = 0;

        //! Simplifies a list of particles by welding vertices that are in the same location.
        //! Graphical meshes often have duplicated vertices for texture rendering, this is not
        //! suitable for a physically simulated mesh where the mesh topology is essential.
        //!
        //! @param particles List of particles, which are composed of positions and inverse masses.
        //! @param indices List of triangles' indices.
        //! @param simplifiedParticles New simplified list of particles.
        //! @param simplifiedIndices New simplified list of triangles' indices.
        //! @param remappedVertices Mapping of vertices between the mesh and the simplified mesh.
        //!                         A negative element means the vertex has been removed.
        //! @param removeStaticTriangles When true it removes triangles whose particles are all static.
        virtual void SimplifyMesh(
            const AZStd::vector<SimParticleFormat>& particles,
            const AZStd::vector<SimIndexType>& indices,
            AZStd::vector<SimParticleFormat>& simplifiedParticles,
            AZStd::vector<SimIndexType>& simplifiedIndices,
            AZStd::vector<int>& remappedVertices,
            bool removeStaticTriangles = true) = 0;
    };
} // namespace NvCloth
