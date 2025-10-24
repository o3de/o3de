/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Editor/DistributionCacheInterface.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/utils.h>
#include <AzCore/Component/Component.h>
#include <Serializer/ParticleBase.h>

namespace OpenParticle
{
    class DistributionCacheComponent
        : public AZ::Component
        , public DistributionCacheInterface
    {
    public:
        AZ_COMPONENT(DistributionCacheComponent, "{A15FA27F-1213-4046-AE08-7958A7918CFA}");

        DistributionCacheComponent();
        virtual ~DistributionCacheComponent();

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("DistributionCacheService"));
        }
        static void GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required) {}
        static void GetIncompatibleServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& incompatible) {}

        void Init() override {};
        void Activate() override;
        void Deactivate() override;

        AZStd::string GetKey(const void* detailInfo, const AZStd::string& className,
            const AZStd::string& moduleName, const AZ::Uuid& paramId) override;
        void StashDistributionRandom(AZStd::string& key, RandomData& dist) override;
        bool PopDistributionRandom(AZStd::string& key, AZStd::vector<RandomData>& dists) override;
        void StashDistributionCurve(AZStd::string& key, CurveData& curve) override;
        bool PopDistributionCurve(AZStd::string& key, AZStd::vector<CurveData>& dists) override;
        void ClearDistribution() override;

    protected:
        AZStd::unordered_map<AZStd::string, AZStd::vector<RandomData>> m_cacheRandom;
        AZStd::unordered_map<AZStd::string, AZStd::vector<CurveData>> m_cacheCurve;
    };
} // namespace OpenParticle
