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
#include <AzCore/Math/Color.h>

namespace EMotionFX
{
    class ColorParameter
        : public RangedValueParameter<AZ::Color, ColorParameter>
    {
        using BaseType = RangedValueParameter<AZ::Color, ColorParameter>;
    public:
        AZ_RTTI(ColorParameter, "{F6F59F14-0A81-4BA0-BEB5-E5DFEE6787A0}", BaseType);
        AZ_CLASS_ALLOCATOR_DECL

        ColorParameter()
            : BaseType(
                AZ::Color(1.0f, 0.0f, 0.0f, 1.0f),
                AZ::Color(0.0f, 0.0f, 0.0f, 1.0f),
                AZ::Color(1.0f, 1.0f, 1.0f, 1.0f)
            )
        {
        }

        static void Reflect(AZ::ReflectContext* context);

        const char* GetTypeDisplayName() const override;

        MCore::Attribute* ConstructDefaultValueAsAttribute() const override;
        uint32 GetType() const override;
        bool AssignDefaultValueToAttribute(MCore::Attribute* attribute) const override;

        bool SetDefaultValueFromAttribute(MCore::Attribute* attribute) override;
        bool SetMinValueFromAttribute(MCore::Attribute* attribute) override;
        bool SetMaxValueFromAttribute(MCore::Attribute* attribute) override;

        static AZ::Color GetUnboundedMinValue();
        static AZ::Color GetUnboundedMaxValue();
    };
}
