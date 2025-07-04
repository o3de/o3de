/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <Atom/RHI.Reflect/SamplerState.h>
#include <Atom/RPI.Public/Material/MaterialInstanceHandler.h>
#include <Atom/RPI.Public/Material/PersistentIndexAllocator.h>

namespace AZ::RPI
{
    // Manages an array with unique RHI::SamplerState entries, which are stored in the 'm_samplers[]' array in the Material-SRGs.
    // This is used to support user-defined Sampler-States in Materials.
    class TextureSamplerRegistry
    {
    public:
        // The first sampler will be the default sampler state, and will not be released
        void Init(const uint32_t maxSamplerStates, RHI::SamplerState defaultSamplerState);
        AZStd::pair<AZStd::shared_ptr<SharedSamplerState>, bool> RegisterTextureSampler(const RHI::SamplerState& samplerState);
        AZStd::shared_ptr<SharedSamplerState> GetSharedSamplerState(const uint32_t index);
        AZStd::vector<RHI::SamplerState> CollectSamplerStates();
        const uint32_t GetMaxNumSamplerStates() const
        {
            return m_maxSamplerStates;
        }

    private:
        AZStd::shared_ptr<SharedSamplerState> MakeSharedSamplerState(const RHI::SamplerState& samplerState);
        void CleanupSamplerLookup(const uint32_t index);

        AZStd::shared_ptr<SharedSamplerState> m_defaultSamplerState = {};
        uint32_t m_maxSamplerStates{ 0 };
        AZStd::vector<AZStd::weak_ptr<SharedSamplerState>> m_samplerStates;
        AZStd::unordered_map<RHI::SamplerState, uint32_t> m_samplerLookup;
        };

} // namespace AZ::RPI
