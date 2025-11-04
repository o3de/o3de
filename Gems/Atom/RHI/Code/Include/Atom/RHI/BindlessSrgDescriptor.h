/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <Atom/RHI.Reflect/Limits.h>

namespace AZ::RHI
{
    //! Struct to help cache all indices related to bindless resource types as well as bindless srg binding slot
    struct BindlessSrgDescriptor
    {
        uint32_t m_roTextureIndex = AZ::RHI::InvalidIndex;
        uint32_t m_rwTextureIndex = AZ::RHI::InvalidIndex;
        uint32_t m_roTextureCubeIndex = AZ::RHI::InvalidIndex;
        uint32_t m_roBufferIndex = AZ::RHI::InvalidIndex;
        uint32_t m_rwBufferIndex = AZ::RHI::InvalidIndex;

        uint32_t m_bindlesSrgBindingSlot = AZ::RHI::InvalidIndex;
    };
}
