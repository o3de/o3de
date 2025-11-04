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
    class StringParameter;
    AZ_TYPE_INFO_SPECIALIZE(StringParameter, "{2ADFD165-B5F9-4C6F-977C-2879610B2445}");

    class StringParameter
        : public DefaultValueParameter<AZStd::string, StringParameter>
    {
        using BaseType = DefaultValueParameter<AZStd::string, StringParameter>;
    public:
        AZ_RTTI_NO_TYPE_INFO_DECL();
        AZ_CLASS_ALLOCATOR_DECL
        StringParameter()
            : BaseType("")
        {
        }

        explicit StringParameter(const AZStd::string& name, const AZStd::string& description = {})
            : BaseType("", name, description)
        {
        }

        StringParameter(const AZStd::string& defaultValue, const AZStd::string& name, const AZStd::string& description = {})
            : BaseType(defaultValue, name, description)
        {
        }

        static void Reflect(AZ::ReflectContext* context);

        const char* GetTypeDisplayName() const override;

        MCore::Attribute* ConstructDefaultValueAsAttribute() const override;
        uint32 GetType() const override;
        bool AssignDefaultValueToAttribute(MCore::Attribute* attribute) const override;

        bool SetDefaultValueFromAttribute(MCore::Attribute* attribute) override;
    };
} // namespace EMotionFX
