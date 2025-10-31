/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "RangedValueParameter.h"
#include <AzCore/Math/Color.h>

namespace EMotionFX
{
    class ColorParameter;
    AZ_TYPE_INFO_SPECIALIZE(ColorParameter, "{F6F59F14-0A81-4BA0-BEB5-E5DFEE6787A0}");

    class ColorParameter
        : public RangedValueParameter<AZ::Color, ColorParameter>
    {
        using BaseType = RangedValueParameter<AZ::Color, ColorParameter>;
    public:
        AZ_RTTI_NO_TYPE_INFO_DECL();
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
