/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Name/Name.h>
#include <AzCore/std/containers/vector.h>

namespace AZ
{
    class ReflectContext;
    

    namespace RHI
    {
        class PlatformLimits;
        struct RHISystemDescriptor final
        {
            AZ_TYPE_INFO(RHISystemDescriptor, "{A506DA28-856C-483A-938D-73471D2C5A5B}");
            static void Reflect(AZ::ReflectContext* context);

            //! The set of globally declared draw list tags, which will be registered with the registry at startup.
            AZStd::vector<AZ::Name> m_drawListTags;

            const RHI::PlatformLimits* m_platformLimits = nullptr;
        };
    } // namespace RHI
} // namespace AZ
