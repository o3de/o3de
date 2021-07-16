/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace EMotionFX
{
    class Parameter
    {
    public:
        AZ_RTTI(Parameter, "{4AF0BAFC-98F8-4EA3-8946-4AD87D7F2A6C}")
        AZ_CLASS_ALLOCATOR_DECL

        Parameter() = default;
        explicit Parameter(AZStd::string name, AZStd::string description = {})
            : m_name(AZStd::move(name))
            , m_description(AZStd::move(description))
        {
        }
        virtual ~Parameter() = default;

        static void Reflect(AZ::ReflectContext* context);

        virtual const char* GetTypeDisplayName() const = 0;
        
        const AZStd::string& GetName() const { return m_name; }
        void SetName(const AZStd::string& name) { m_name = name; }

        const AZStd::string& GetDescription() const { return m_description; }
        void SetDescription(const AZStd::string& description) { m_description = description; }

        static bool IsNameValid(const AZStd::string& name, AZStd::string* outInvalidCharacters);

    protected:
        AZStd::string       m_name;              /**< The name as it will appear in the interface. */
        AZStd::string       m_description;       /**< The description of the attribute. */

        static const char s_invalidCharacters[];
    };

    typedef AZStd::vector<Parameter*> ParameterVector;
}
