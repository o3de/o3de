/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/TypeInfo.h>
#include <Serializer/ParticleBase.h>

namespace OpenParticleSystemEditor
{
    class DistIndexUtil
    {
    public:
        static size_t GetDistIndex(void* address, const AZ::TypeId& id, const OpenParticle::DistributionType& distType, int index);
        static void SetDistIndex(void* address, const AZ::TypeId& id, const OpenParticle::DistributionType& distType, size_t value, int index);
        static void ClearDistIndex(void* address, const AZ::TypeId& id);
        static OpenParticle::DistributionType GetDistributionType(void* address, const AZ::TypeId& id);
        static void SetDistributionType(void* address, const AZ::TypeId& id, const OpenParticle::DistributionType& type);
        static bool IsUinform(void* address, const AZ::TypeId& id);
        static void SetUinform(void* address, const AZ::TypeId& id, bool isUniform);
    };
} // OpenParticleSystemEditor
