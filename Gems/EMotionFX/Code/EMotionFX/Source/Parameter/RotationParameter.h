/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "RangedValueParameter.h"
#include <AzCore/Math/Quaternion.h>

namespace EMotionFX
{
    class RotationParameter;
    AZ_TYPE_INFO_SPECIALIZE(RotationParameter, "{D84302D2-2977-43DD-B953-F038222E65BF}");

    class RotationParameter
        : public RangedValueParameter<AZ::Quaternion, RotationParameter>
    {
        using BaseType = RangedValueParameter<AZ::Quaternion, RotationParameter>;
    public:
        AZ_RTTI_NO_TYPE_INFO_DECL();
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
