/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
