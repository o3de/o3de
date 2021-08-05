/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "FloatParameter.h"

namespace EMotionFX
{
    class FloatSpinnerParameter 
        : public FloatParameter
    {
    public:
        AZ_RTTI(FloatSpinnerParameter, "{AD3D4357-F965-42E7-BAC8-7F4FF7F25FD0}", FloatParameter)
        AZ_CLASS_ALLOCATOR_DECL

        FloatSpinnerParameter()
            : FloatParameter()
        {}
        
        explicit FloatSpinnerParameter(AZStd::string name, AZStd::string description = {})
            : FloatParameter(AZStd::move(name), AZStd::move(description))
        {
        }

        static void Reflect(AZ::ReflectContext* context);
        
        const char* GetTypeDisplayName() const override;
    };
} 
