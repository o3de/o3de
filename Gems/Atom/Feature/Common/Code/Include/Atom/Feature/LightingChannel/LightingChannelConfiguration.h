/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/Base.h>
#include <AzCore/base.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/RTTI/ReflectContext.h>

namespace AZ
{
    namespace Render
    {
        const uint32_t LightingChannelsCount = 5;

        struct ATOM_FEATURE_COMMON_API LightingChannelConfiguration
        {
            AZ_TYPE_INFO(LightingChannelConfiguration, "{7FFD6D01-BABE-FE35-612F-63A30925E5F7}");

            static void Reflect(AZ::ReflectContext* context);

            LightingChannelConfiguration() = default;

            void SetLightingChannelMask(const uint32_t mask);

            uint32_t GetLightingChannelMask() const;

            AZStd::array<bool, LightingChannelsCount> m_lightingChannelFlags{ true, true, true, true, true };

        private:
            AZStd::string GetLabelForIndex(int index) const;
        };
    }
}
