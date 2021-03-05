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

#include "DefaultValueParameter.h"

namespace EMotionFX
{
    class StringParameter
        : public DefaultValueParameter<AZStd::string, StringParameter>
    {
        using BaseType = DefaultValueParameter<AZStd::string, StringParameter>;
    public:
        AZ_RTTI(StringParameter, "{2ADFD165-B5F9-4C6F-977C-2879610B2445}", ValueParameter);
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
