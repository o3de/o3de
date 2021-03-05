/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Interface/Interface.h>

#include <NvCloth/IFabricCooker.h>

namespace NvCloth
{
    //! Implementation of the IFabricCooker interface.
    class FabricCooker
        : public AZ::Interface<IFabricCooker>::Registrar
    {
    public:
        AZ_RTTI(FabricCooker, "{14EC2D3E-A36C-466E-BBDB-462A9194586E}", IFabricCooker);

    protected:
        // IFabricCooker overrides ...
        AZStd::optional<FabricCookedData> CookFabric(
            const AZStd::vector<SimParticleFormat>& particles,
            const AZStd::vector<SimIndexType>& indices,
            const AZ::Vector3& fabricGravity = AZ::Vector3(0.0f, 0.0f, -9.81f),
            bool useGeodesicTether = true) override;
        void SimplifyMesh(
            const AZStd::vector<SimParticleFormat>& particles,
            const AZStd::vector<SimIndexType>& indices,
            AZStd::vector<SimParticleFormat>& simplifiedParticles,
            AZStd::vector<SimIndexType>& simplifiedIndices,
            AZStd::vector<int>& remappedVertices,
            bool removeStaticTriangles = true) override;
    };
} // namespace NvCloth
