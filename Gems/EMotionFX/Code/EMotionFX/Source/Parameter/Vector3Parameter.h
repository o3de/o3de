/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "RangedValueParameter.h"
#include <AzCore/Math/Vector3.h>

namespace EMotionFX
{
    class Vector3Parameter;
    AZ_TYPE_INFO_SPECIALIZE(Vector3Parameter, "{E647B621-27DA-454E-A14F-45C65E2C7874}");

    class Vector3Parameter
        : public RangedValueParameter<AZ::Vector3, Vector3Parameter>
    {
        using BaseType = RangedValueParameter<AZ::Vector3, Vector3Parameter>;
    public:
        AZ_RTTI_NO_TYPE_INFO_DECL()
        AZ_CLASS_ALLOCATOR_DECL

        Vector3Parameter()
            : BaseType(
                AZ::Vector3(0.0f, 0.0f, 0.0f),
                AZ::Vector3(-1000.0f, -1000.0f, -1000.0f),
                AZ::Vector3(1000.0f, 1000.0f, 1000.0f)
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

        static AZ::Vector3 GetUnboundedMinValue();
        static AZ::Vector3 GetUnboundedMaxValue();
    };
}
