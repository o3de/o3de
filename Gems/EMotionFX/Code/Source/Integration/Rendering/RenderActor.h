/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>


namespace EMotionFX
{
    namespace Integration
    {
        class RenderActor
        {
        public:
            AZ_RTTI(EMotionFX::Integration::RenderActor, "{827A2CD5-C5FC-4D14-984D-B44A52EA92CC}")
            AZ_CLASS_ALLOCATOR_DECL

            RenderActor() = default;
            virtual ~RenderActor() = default;
        };
    } // namespace Integration
} // namespace EMotionFX
