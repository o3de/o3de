/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <Serializer/ParticleBase.h>

namespace OpenParticle
{
    class ParticleEditDataConfig
    {
    public:
        AZ_TYPE_INFO(ParticleEditDataConfig, "{0C07EB32-D257-DE0A-93E5-65EAC2987E90}");
        static void Reflect(AZ::ReflectContext* context);
    };
} // namespace OpenParticle
