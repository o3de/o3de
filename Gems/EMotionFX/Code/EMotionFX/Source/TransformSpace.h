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

