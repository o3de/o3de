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

#include "RangedValueParameter.h"

namespace EMotionFX
{
    class IntParameter 
        : public RangedValueParameter<int, IntParameter>
    {
        using BaseType = RangedValueParameter<int, IntParameter>;
    public:
        AZ_RTTI(IntParameter , "{8F1C1579-E6B7-4CD0-8ABB-0250A131CF6C}", ValueParameter);
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
