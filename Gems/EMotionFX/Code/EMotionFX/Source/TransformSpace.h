/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/RTTI/TypeInfo.h>


namespace AZ
{
    class ReflectContext;
}

namespace EMotionFX
{
    enum ETransformSpace : AZ::u8
    {
        TRANSFORM_SPACE_LOCAL = 0,
        TRANSFORM_SPACE_MODEL = 1,
        TRANSFORM_SPACE_WORLD = 2
    };

    class TransformSpace
    {
    public:
        static void Reflect(AZ::ReflectContext* context);
    };

}   // namespace EMotionFX

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(EMotionFX::ETransformSpace, "{25CD9BEE-690C-4696-874E-9188598F3FB7}");
} // namespace AZ

