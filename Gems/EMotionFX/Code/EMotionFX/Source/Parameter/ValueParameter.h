/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "Parameter.h"
#include <MCore/Source/Config.h>

namespace MCore
{
    class Attribute;
}

namespace EMotionFX
{
    /**
     * ValueParameter inherits from Parameter and is the base type for all the parameters that contain a value (i.e. nOot groups)
     */
    class ValueParameter
        : public Parameter
    {
    public:
        
        AZ_RTTI(ValueParameter, "{46549C79-6B4C-4DDE-A5E3-E5FBEC455816}", Parameter)
        AZ_CLASS_ALLOCATOR_DECL

        ValueParameter() = default;
        explicit ValueParameter(
            AZStd::string name,
            AZStd::string description = {}
        )
            : Parameter(AZStd::move(name), AZStd::move(description))
        {
        }

        ~ValueParameter() override = default;

        static void Reflect(AZ::ReflectContext* context);

        // Methods required to support MCore::Attribute
        virtual MCore::Attribute* ConstructDefaultValueAsAttribute() const = 0;
        virtual uint32 GetType() const = 0;
        virtual bool AssignDefaultValueToAttribute(MCore::Attribute* attribute) const = 0;
        virtual bool SetDefaultValueFromAttribute(MCore::Attribute* attribute) = 0;
        virtual bool SetMinValueFromAttribute(MCore::Attribute* attribute) { AZ_UNUSED(attribute); return false; }
        virtual bool SetMaxValueFromAttribute(MCore::Attribute* attribute) { AZ_UNUSED(attribute); return false; }
    };

    typedef AZStd::vector<ValueParameter*> ValueParameterVector;
}
