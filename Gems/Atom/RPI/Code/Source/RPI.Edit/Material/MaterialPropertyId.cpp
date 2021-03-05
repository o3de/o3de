/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
            const bool groupNameIsValid = m_groupName.IsEmpty() || IsValidName(m_groupName);
            const bool propertyNameIsValid = IsValidName(m_propertyName);
            return groupNameIsValid && propertyNameIsValid;
        }

        MaterialPropertyId MaterialPropertyId::Parse(AZStd::string_view fullPropertyId)
        {
            AZStd::vector<AZStd::string> tokens;
            AzFramework::StringFunc::Tokenize(fullPropertyId.data(), tokens, '.', true, true);

            if (tokens.size() == 1)
            {
                return MaterialPropertyId{"", tokens[0]};
            }
            else if (tokens.size() == 2)
            {
                return MaterialPropertyId{tokens[0], tokens[1]};
            }
            else
            {
                AZ_Error("MaterialPropertyId", false, "Property ID '%s' is not a valid identifier.", fullPropertyId.data());
                return MaterialPropertyId{};
            }
        }

        MaterialPropertyId::MaterialPropertyId(AZStd::string_view groupName, AZStd::string_view propertyName)
            : MaterialPropertyId(Name{groupName}, Name{propertyName})
        {
        }

        MaterialPropertyId::MaterialPropertyId(const Name& groupName, const Name& propertyName)
        {
            AZ_Error("MaterialPropertyId", groupName.IsEmpty() || IsValidName(groupName), "Group name '%s' is not a valid identifier.", groupName.GetCStr());
            AZ_Error("MaterialPropertyId", IsValidName(propertyName), "Property name '%s' is not a valid identifier.", propertyName.GetCStr());
            m_groupName = groupName;
            m_propertyName = propertyName;
            if (groupName.IsEmpty())
            {
                m_fullName = m_propertyName.GetStringView();
            }
            else
            {
                m_fullName = AZStd::string::format("%s.%s", m_groupName.GetCStr(), m_propertyName.GetCStr());
            }
        }

        const Name& MaterialPropertyId::GetGroupName() const
        {
            return m_groupName;
        }

        const Name& MaterialPropertyId::GetPropertyName() const
        {
            return m_propertyName;
        }

        const Name& MaterialPropertyId::GetFullName() const
        {
            return m_fullName;
        }

        const char* MaterialPropertyId::GetCStr() const
        {
            return m_fullName.GetCStr();
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
