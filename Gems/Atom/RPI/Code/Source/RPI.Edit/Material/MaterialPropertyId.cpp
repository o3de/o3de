/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Material/MaterialPropertyId.h>
#include <AzCore/std/string/regex.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace AZ
{
    namespace RPI
    {
        bool MaterialPropertyId::IsValidName(AZStd::string_view name)
        {
            // Checks for a c++ style identifier
            return AZStd::regex_match(name.begin(), name.end(), AZStd::regex("^[a-zA-Z_][a-zA-Z0-9_]*$"));
        }

        bool MaterialPropertyId::IsValidName(const AZ::Name& name)
        {
            return IsValidName(name.GetStringView());
        }

        bool MaterialPropertyId::IsValid() const
        {
            return !m_fullName.IsEmpty();
        }

        MaterialPropertyId MaterialPropertyId::Parse(AZStd::string_view fullPropertyId)
        {
            AZStd::vector<AZStd::string> tokens;
            AzFramework::StringFunc::Tokenize(fullPropertyId, tokens, '.', true, true);

            if (tokens.empty())
            {
                AZ_Error("MaterialPropertyId", false, "Property ID is empty.", fullPropertyId.data());
                return MaterialPropertyId{};
            }

            for (const auto& token : tokens)
            {
                if (!IsValidName(token))
                {
                    AZ_Error("MaterialPropertyId", false, "Property ID '%.*s' is not a valid identifier.", AZ_STRING_ARG(fullPropertyId));
                    return MaterialPropertyId{};
                }
            }

            MaterialPropertyId id;
            id.m_fullName = fullPropertyId;
            return id;
        }
        
        MaterialPropertyId::MaterialPropertyId(AZStd::string_view propertyName)
        {
            if (!IsValidName(propertyName))
            {
                AZ_Error("MaterialPropertyId", false, "Property name '%.*s' is not a valid identifier.", AZ_STRING_ARG(propertyName));
            }
            else
            {
                m_fullName = propertyName;
            }
        }

        MaterialPropertyId::MaterialPropertyId(AZStd::string_view groupName, AZStd::string_view propertyName)
        {
            if (!IsValidName(groupName))
            {
                AZ_Error("MaterialPropertyId", false, "Group name '%.*s' is not a valid identifier.", AZ_STRING_ARG(groupName));
            }
            else if (!IsValidName(propertyName))
            {
                AZ_Error("MaterialPropertyId", false, "Property name '%.*s' is not a valid identifier.", AZ_STRING_ARG(propertyName));
            }
            else
            {
                m_fullName = AZStd::string::format("%.*s.%.*s", AZ_STRING_ARG(groupName), AZ_STRING_ARG(propertyName));
            }
        }
        
        MaterialPropertyId::MaterialPropertyId(const Name& groupName, const Name& propertyName)
            : MaterialPropertyId(groupName.GetStringView(), propertyName.GetStringView())
        {
        }
        
        MaterialPropertyId::MaterialPropertyId(const AZStd::array_view<AZStd::string> names)
        {
            for (const auto& name : names)
            {
                if (!IsValidName(name))
                {
                    AZ_Error("MaterialPropertyId", false, "'%s' is not a valid identifier.", name.c_str());
                    return;
                }
            }

            AzFramework::StringFunc::Join(m_fullName, names.begin(), names.end(), ".");
        }

        MaterialPropertyId::operator const Name&() const
        {
            return m_fullName;
        }

        const char* MaterialPropertyId::GetCStr() const
        {
            return m_fullName.GetCStr();
        }
        
        AZStd::string_view MaterialPropertyId::GetStringView() const
        {
            return m_fullName.GetStringView();
        }

        Name::Hash MaterialPropertyId::GetHash() const
        {
            return m_fullName.GetHash();
        }

        bool MaterialPropertyId::operator==(const MaterialPropertyId& rhs) const
        {
            return m_fullName == rhs.m_fullName;
        }

        bool MaterialPropertyId::operator!=(const MaterialPropertyId& rhs) const
        {
            return m_fullName != rhs.m_fullName;
        }
    } // namespace RPI
} // namespace AZ
