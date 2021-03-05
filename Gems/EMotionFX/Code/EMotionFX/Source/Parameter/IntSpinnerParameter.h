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
    class IntSpinnerParameter 
        : public IntParameter
    {
    public:
        AZ_RTTI(IntSpinnerParameter, "{3FFC78F6-B2CC-47D0-B453-4EFD358B1020}", IntParameter)
        AZ_CLASS_ALLOCATOR_DECL

        IntSpinnerParameter()
            : IntParameter()
        {}

        static void Reflect(AZ::ReflectContext* context);
        
        const char* GetTypeDisplayName() const override;
    };
} 
