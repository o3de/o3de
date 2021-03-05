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
    class BoolParameter 
        : public DefaultValueParameter<bool, BoolParameter>
    {
        using BaseType = DefaultValueParameter<bool, BoolParameter>;
    public:
        AZ_RTTI(BoolParameter, "{1057BEFA-09A8-4B13-93CD-614BACF18106}", ValueParameter);
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
