/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Math/Crc.h>

namespace WhiteBox
{
    //! The render model's material slot for faces of one material and paint colour; a Material component saves its overrides
    //! by this id, so it is hashed (deterministic across runs) rather than numbered. Invalid material is the entity default.
    inline AZ::u32 MaterialSlotStableId(const AZ::Data::AssetId& material, const AZ::u32 paintColor)
    {
        AZ::Crc32 crc;
        if (material.IsValid())
        {
            crc.Add(&material.m_guid, sizeof(material.m_guid));
            crc.Add(&material.m_subId, sizeof(material.m_subId));
        }
        crc.Add(&paintColor, sizeof(paintColor));
        return static_cast<AZ::u32>(crc);
    }
} // namespace WhiteBox
