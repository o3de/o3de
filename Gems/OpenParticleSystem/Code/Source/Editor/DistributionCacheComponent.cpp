/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Editor/DistributionCacheComponent.h>

namespace OpenParticle
{
    DistributionCacheComponent::DistributionCacheComponent()
    {
    }

    DistributionCacheComponent::~DistributionCacheComponent()
    {
    }

    void DistributionCacheComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<DistributionCacheComponent, AZ::Component>()
                ->Version(1);
        }
    }

    void DistributionCacheComponent::Activate()
    {
        if (AZ::Interface<DistributionCacheInterface>::Get() == nullptr)
        {
            AZ::Interface<DistributionCacheInterface>::Register(this);
        }
    }

    void DistributionCacheComponent::Deactivate()
    {
        if (AZ::Interface<DistributionCacheInterface>::Get() != nullptr)
        {
            AZ::Interface<DistributionCacheInterface>::Unregister(this);
        }
        ClearDistribution();
    }

    AZStd::string DistributionCacheComponent::GetKey(const void* detailInfo,
        const AZStd::string& className, const AZStd::string& moduleName, const AZ::TypeId& paramId)
    {
        return AZStd::string::format("%llx_%s_%s_%zx", reinterpret_cast<AZ::u64>(detailInfo), className.data(), moduleName.data(), paramId.GetHash());
    }

    void DistributionCacheComponent::StashDistributionCurve(AZStd::string& key, CurveData& curve)
    {
        m_cacheCurve[key].emplace_back(curve);
    }

    bool DistributionCacheComponent::PopDistributionCurve(AZStd::string& key, AZStd::vector<CurveData>& dists)
    {
        if (m_cacheCurve.find(key) == m_cacheCurve.end())
        {
            return false;
        }
        dists = m_cacheCurve[key];
        m_cacheCurve.erase(key);
        return true;
    }

    void DistributionCacheComponent::StashDistributionRandom(AZStd::string& key, RandomData& random)
    {
        m_cacheRandom[key].emplace_back(random);
    }

    bool DistributionCacheComponent::PopDistributionRandom(AZStd::string& key, AZStd::vector<RandomData>& dists)
    {
        if (m_cacheRandom.find(key) == m_cacheRandom.end())
        {
            return false;
        }
        dists = m_cacheRandom[key];
        m_cacheRandom.erase(key);
        return true;
    }
    void DistributionCacheComponent::ClearDistribution()
    {
        m_cacheRandom.clear();
        m_cacheCurve.clear();
    }
} // namespace OpenParticle
