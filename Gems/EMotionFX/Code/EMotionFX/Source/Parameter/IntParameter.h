/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "RangedValueParameter.h"

namespace EMotionFX
{
    class IntParameter;
    AZ_TYPE_INFO_SPECIALIZE(IntParameter , "{8F1C1579-E6B7-4CD0-8ABB-0250A131CF6C}");

    class IntParameter 
        : public RangedValueParameter<int, IntParameter>
    {
        using BaseType = RangedValueParameter<int, IntParameter>;
    public:
        AZ_RTTI_NO_TYPE_INFO_DECL();
        AZ_CLASS_ALLOCATOR_DECL

        IntParameter()
            : BaseType(0, -1000, 1000)
        {
        }

        static void Reflect(AZ::ReflectContext* context);
        
        MCore::Attribute* ConstructDefaultValueAsAttribute() const override;
        uint32 GetType() const override;
        bool AssignDefaultValueToAttribute(MCore::Attribute* attribute) const override;

        bool SetDefaultValueFromAttribute(MCore::Attribute* attribute) override;
        bool SetMinValueFromAttribute(MCore::Attribute* attribute) override;
        bool SetMaxValueFromAttribute(MCore::Attribute* attribute) override;

        static int GetUnboundedMinValue();
        static int GetUnboundedMaxValue();
    };
} 
