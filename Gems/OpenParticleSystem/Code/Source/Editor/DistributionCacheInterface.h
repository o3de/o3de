/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/any.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/utils.h>
#include <Serializer/ParticleSourceData.h>

namespace OpenParticle
{
    using RandomData = AZStd::pair<ParamDistInfo, Random*>;
    using CurveData = AZStd::pair<ParamDistInfo, Curve*>;
    class DistributionCacheInterface
    {
    public:
        AZ_RTTI(DistributionCacheInterface, "{7BA4AB4D-C284-4F3A-AC07-1DFAED6139EE}");

        virtual AZStd::string GetKey(const void* detailInfo, const AZStd::string& className, const AZStd::string& moduleName, const AZ::Uuid& paramId) = 0;
        virtual void StashDistributionRandom(AZStd::string& key, RandomData& dist) = 0;
        virtual bool PopDistributionRandom(AZStd::string& key, AZStd::vector<RandomData>& dists) = 0;

        virtual void StashDistributionCurve(AZStd::string& key, CurveData& curve) = 0;
        virtual bool PopDistributionCurve(AZStd::string& key, AZStd::vector<CurveData>& dists) = 0;

        virtual void ClearDistribution() = 0;
    };
} // namespace OpenParticle
