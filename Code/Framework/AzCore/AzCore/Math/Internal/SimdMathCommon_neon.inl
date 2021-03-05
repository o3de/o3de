/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

namespace AZ
{
    namespace Simd
    {
        namespace Neon
        {
            AZ_MATH_INLINE bool AreAllLanesTrue(uint32x2_t value)
            {
                return vminv_u32(value) != 0;
            }

            AZ_MATH_INLINE bool AreAllLanesTrue(uint32x4_t value)
            {
                return vminvq_u32(value) != 0;
            }
        }
    }
}
