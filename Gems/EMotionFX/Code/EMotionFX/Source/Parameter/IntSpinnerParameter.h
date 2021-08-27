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
