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

#include "IntParameter.h"

namespace EMotionFX
{
    class IntSliderParameter 
        : public IntParameter
    {
    public:
        AZ_RTTI(IntSliderParameter, "{0AF8A855-06F1-4C3C-8860-88DF52095DD8}", IntParameter)
        AZ_CLASS_ALLOCATOR_DECL

        IntSliderParameter()
            : IntParameter()
        {}

        static void Reflect(AZ::ReflectContext* context);
        
        const char* GetTypeDisplayName() const override;
    };
} 
