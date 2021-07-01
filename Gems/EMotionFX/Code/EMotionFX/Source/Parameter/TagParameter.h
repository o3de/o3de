/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "BoolParameter.h"

namespace EMotionFX
{
    class TagParameter 
        : public BoolParameter
    {
    public:
        AZ_RTTI(TagParameter, "{E952924C-8C3D-452E-9E5F-45776BB83F33}", BoolParameter)
        AZ_CLASS_ALLOCATOR_DECL

        TagParameter()
            : BoolParameter()
        {}
        
        const char* GetTypeDisplayName() const override;

        static void Reflect(AZ::ReflectContext* context);
    };
} 
