/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/base.h>

namespace AZ { class ReflectContext; }

namespace SceneColorView
{
    enum class CaptureResolution : AZ::u32
    {
        Full = 1,
        Half = 2,
        Quarter = 4
    };

    // Register beside the enum so O3DE can find these overloads through ADL.
    AZ_TYPE_INFO_SPECIALIZE(CaptureResolution, "{2151A17C-15E8-4F4A-8B89-B1F873BC9325}");

    struct Configuration
    {
        AZ_TYPE_INFO(Configuration, "{82D3B7CC-3B47-4B40-9B7B-D69EEA44CB85}");
        static void Reflect(AZ::ReflectContext* context);

        bool m_enabled = false;
        CaptureResolution m_resolutionDivisor = CaptureResolution::Half;
    };
}

