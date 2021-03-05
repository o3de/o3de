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
#include <AzCore/Math/Vector2.h>

namespace EMotionFX
{
    class Vector2Parameter
        : public RangedValueParameter<AZ::Vector2, Vector2Parameter>
    {
        using BaseType = RangedValueParameter<AZ::Vector2, Vector2Parameter>;
    public:
        AZ_RTTI(Vector2Parameter, "{4133BFD5-81F3-4CBC-AA9B-28CF0C1439E0}", ValueParameter);
        AZ_CLASS_ALLOCATOR_DECL

        Vector2Parameter()
            : BaseType(
                AZ::Vector2(0.0f, 0.0f),
                AZ::Vector2(-1000.0f, -1000.0f),
                AZ::Vector2(1000.0f, 1000.0f),
                false,
                false
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
        
        static AZ::Vector2 GetUnboundedMinValue();
        static AZ::Vector2 GetUnboundedMaxValue();
    };
}
