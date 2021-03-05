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
#include <AzCore/Math/Quaternion.h>

namespace EMotionFX
{
    class RotationParameter
        : public RangedValueParameter<AZ::Quaternion, RotationParameter>
    {
        using BaseType = RangedValueParameter<AZ::Quaternion, RotationParameter>;
    public:
        AZ_RTTI(RotationParameter, "{D84302D2-2977-43DD-B953-F038222E65BF}", ValueParameter);
        AZ_CLASS_ALLOCATOR_DECL

        RotationParameter()
            : BaseType(
                AZ::Quaternion::CreateIdentity(),
                AZ::Quaternion(-1000.0f, -1000.0f, -1000.0f, -1000.0f),
                AZ::Quaternion(1000.0f, 1000.0f, 1000.0f, 1000.0f)
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

        static AZ::Quaternion GetUnboundedMinValue();
        static AZ::Quaternion GetUnboundedMaxValue();
    };
}
