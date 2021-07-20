/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <GridMate/Serialize/Buffer.h>
#include <AzCore/Math/Uuid.h>

namespace GridMate
{
    /**
    * Specialized marshaler for AZ::Uuid
    */
    template<>
    class Marshaler<AZ::Uuid>
    {
    public:
        AZ_TYPE_INFO_LEGACY(Marshaler, "{1283D0CB-6B4B-4C69-AB4D-A8442AE14A64}", AZ::Uuid);

        typedef AZ::Uuid DataType;

        AZ_FORCE_INLINE void Marshal(WriteBuffer& wb, const DataType& value) const
        {
            wb.WriteRaw(value.begin(), value.end() - value.begin());
        }

        AZ_FORCE_INLINE void Unmarshal(DataType& value, ReadBuffer& rb) const
        {
            rb.ReadRaw(value.begin(), value.end() - value.begin());
        }
    };
}
