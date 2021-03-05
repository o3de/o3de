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
#include <AzCore/Math/Vector4.h>

namespace EMotionFX
{
    class Vector4Parameter
        : public RangedValueParameter<AZ::Vector4, Vector4Parameter>
    {
        using BaseType = RangedValueParameter<AZ::Vector4, Vector4Parameter>;
    public:
        AZ_RTTI(Vector4Parameter, "{63D0D19F-DC97-4F56-9FE8-A5A4225E0850}", BaseType);
        AZ_CLASS_ALLOCATOR_DECL

        Vector4Parameter()
            : BaseType(
                AZ::Vector4(0.0f, 0.0f, 0.0f, 0.0f),
                AZ::Vector4(-1000.0f, -1000.0f, -1000.0f, -1000.0f),
                AZ::Vector4(1000.0f, 1000.0f, 1000.0f, 1000.0f)
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

        static AZ::Vector4 GetUnboundedMinValue();
        static AZ::Vector4 GetUnboundedMaxValue();
    };
}
