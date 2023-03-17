/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "DefaultValueParameter.h"

namespace EMotionFX
{
    class BoolParameter;
    AZ_TYPE_INFO_SPECIALIZE(BoolParameter, "{1057BEFA-09A8-4B13-93CD-614BACF18106}");

    class BoolParameter 
        : public DefaultValueParameter<bool, BoolParameter>
    {
        using BaseType = DefaultValueParameter<bool, BoolParameter>;
    public:
        AZ_RTTI_NO_TYPE_INFO_DECL()
        AZ_CLASS_ALLOCATOR_DECL

        BoolParameter()
            : BaseType(false)
        {
        }

        explicit BoolParameter(const AZStd::string& name, const AZStd::string& description = {})
            : BaseType(false, name, description)
        {
        }

        BoolParameter(bool defaultValue, const AZStd::string& name, const AZStd::string& description = {})
            : BaseType(defaultValue, name, description)
        {
        }

        const char* GetTypeDisplayName() const override;

        static void Reflect(AZ::ReflectContext* context);

        MCore::Attribute* ConstructDefaultValueAsAttribute() const override;
        uint32 GetType() const override;
        bool AssignDefaultValueToAttribute(MCore::Attribute* attribute) const override;

        bool SetDefaultValueFromAttribute(MCore::Attribute* attribute) override;
    };
} 
