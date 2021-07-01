/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>

#include <NvCloth/Types.h>

#include <System/NvTypes.h>

namespace NvCloth
{
    class Solver;
    class Fabric;
    class Cloth;

    //! This class knows how to construct Solver, Cloth and Fabric objects.
    //!
    //! All objects constructed by this factory will run on CPU.
    class Factory
    {
    public:
        AZ_RTTI(Factory, "{ABA9A937-2FE2-44A3-A143-E1594B479BE6}");

        virtual ~Factory() = default;

        virtual void Init();
        virtual void Destroy();

        AZStd::unique_ptr<Solver> CreateSolver(const AZStd::string& name);

        AZStd::unique_ptr<Fabric> CreateFabric(const FabricCookedData& fabricCookedData);

        AZStd::unique_ptr<Cloth> CreateCloth(
            const AZStd::vector<SimParticleFormat>& initialParticles,
            Fabric* fabric);

    protected:
        //! NvCloth factory object.
        NvFactoryUniquePtr m_nvFactory;
    };

} // namespace NvCloth
