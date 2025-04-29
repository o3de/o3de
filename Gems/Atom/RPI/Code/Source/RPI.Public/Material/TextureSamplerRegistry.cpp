/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AzCore/std/smart_ptr/make_shared.h"
#include <Atom/RPI.Public/Material/TextureSamplerRegistry.h>

namespace AZ::RPI
{
    void TextureSamplerRegistry::Init(const uint32_t maxSamplerStates, RHI::SamplerState defaultSamplerState)
    {
        // we might be re-using this registry, so reset everything
        *this = {};
        m_maxSamplerStates = maxSamplerStates;
        if (m_maxSamplerStates > 0)
        {
            // create a shared_ptr to the default sampler, so it never expires
            m_defaultSamplerState = RegisterTextureSampler(defaultSamplerState);
        }
    }

    void TextureSamplerRegistry::CleanupSamplerLookup(const uint32_t indexToRemove)
    {
        for (auto& [samplerState, index] : m_samplerLookup)
        {
            if (index == indexToRemove)
            {
                m_samplerLookup.erase(samplerState);
                break;
            }
        }
    }

    AZStd::shared_ptr<SharedSamplerState> TextureSamplerRegistry::GetSharedSamplerState(const uint32_t index)
    {
        if (m_samplerStates.size() <= index || m_samplerStates[index].expired())
        {
            return m_defaultSamplerState;
        }
        return m_samplerStates[index].lock();
    }

    AZStd::vector<RHI::SamplerState> TextureSamplerRegistry::CollectSamplerStates()
    {
        AZStd::vector<RHI::SamplerState> result;
        result.reserve(m_samplerStates.size());
        for (int index = 0; index < m_samplerStates.size(); ++index)
        {
            if (m_samplerStates[index].expired())
            {
                if (m_defaultSamplerState)
                {
                    result.push_back(m_defaultSamplerState->m_samplerState);
                }
                else
                {
                    result.push_back({});
                }
            }
            else
            {
                auto samplerState = m_samplerStates[index].lock();
                result.push_back(samplerState->m_samplerState);
            }
        }
        return result;
    }

    AZStd::shared_ptr<SharedSamplerState> TextureSamplerRegistry::MakeSharedSamplerState(const RHI::SamplerState& samplerState)
    {
        if (m_maxSamplerStates == 0) // shared sampler states are disabled
        {
            return nullptr;
        }
        // find an expired sampler state and replace it
        for (int index = 0; index < m_samplerStates.size(); ++index)
        {
            if (m_samplerStates[index].expired())
            {
                // we are replacing a sampler, so remove the lookup entry pointing to this index
                CleanupSamplerLookup(index);
                auto sharedSampler =
                    AZStd::make_shared<SharedSamplerState>(SharedSamplerState{ static_cast<uint32_t>(index), samplerState });
                m_samplerStates[index] = sharedSampler;
                m_samplerLookup[samplerState] = index;
                return sharedSampler;
            }
        }
        // No expired sampler state found: append a new sampler state
        if (m_samplerStates.size() < m_maxSamplerStates)
        {
            uint32_t index = static_cast<uint32_t>(m_samplerStates.size());
            auto sharedSampler = AZStd::make_shared<SharedSamplerState>(SharedSamplerState{ index, samplerState });
            m_samplerStates.emplace_back(sharedSampler);
            m_samplerLookup[samplerState] = index;
            return sharedSampler;
        }
        else
        {
            AZ_Assert(false, "Max number of unique texture samplers (%d) exceeded", m_maxSamplerStates);
        }
        return nullptr;
    }

    AZStd::shared_ptr<SharedSamplerState> TextureSamplerRegistry::RegisterTextureSampler(const RHI::SamplerState& samplerState)
    {
        auto it = m_samplerLookup.find(samplerState);
        if (it == m_samplerLookup.end())
        {
            auto result = MakeSharedSamplerState(samplerState);
            if (result)
            {
                m_samplerLookup[result->m_samplerState] = result->m_samplerIndex;
            }
            return result;
        }
        else
        {
            // the sampler state expired but wasn't replaced yet, so un-expire it
            if (m_samplerStates[it->second].expired())
            {
                auto sharedSampler = AZStd::make_shared<SharedSamplerState>(SharedSamplerState{ it->second, samplerState });
                m_samplerStates[it->second] = sharedSampler;
                return sharedSampler;
            }
            else
            {
                return m_samplerStates[it->second].lock();
            }
        }
    }
} // namespace AZ::RPI
