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
