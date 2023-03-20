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
    class FloatParameter;
    AZ_TYPE_INFO_SPECIALIZE(FloatParameter, "{0F0B8531-0B07-4D9B-A8AC-3A32D15E8762}");

    class FloatParameter 
        : public RangedValueParameter<float, FloatParameter>
    {
        using BaseType = RangedValueParameter<float, FloatParameter>;
    public:
        AZ_RTTI_NO_TYPE_INFO_DECL()
        AZ_CLASS_ALLOCATOR_DECL

        FloatParameter()
            : BaseType(0.0f, 0.0f, 1.0f)
        {
        }

        explicit FloatParameter(AZStd::string name, AZStd::string description = {})
            : BaseType(0.0f, 0.0f, 1.0f, true, true, AZStd::move(name), AZStd::move(description))
        {
        }

        static void Reflect(AZ::ReflectContext* context);

        MCore::Attribute* ConstructDefaultValueAsAttribute() const override;
        uint32 GetType() const override;
        bool AssignDefaultValueToAttribute(MCore::Attribute* attribute) const override;

        bool SetDefaultValueFromAttribute(MCore::Attribute* attribute) override;
        bool SetMinValueFromAttribute(MCore::Attribute* attribute) override;
        bool SetMaxValueFromAttribute(MCore::Attribute* attribute) override;

        static float GetUnboundedMinValue();
        static float GetUnboundedMaxValue();
    };
} 
