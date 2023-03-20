/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Material/MaterialPropertyId.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <Atom/RPI.Edit/Material/MaterialUtils.h>

namespace AZ
{
    namespace RPI
    {
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
                if (!MaterialUtils::IsValidName(token))
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
            if (MaterialUtils::CheckIsValidPropertyName(propertyName))
            {
                m_fullName = propertyName;
            }
        }

        MaterialPropertyId::MaterialPropertyId(AZStd::string_view groupName, AZStd::string_view propertyName)
        {
            if (MaterialUtils::CheckIsValidGroupName(groupName) && MaterialUtils::CheckIsValidPropertyName(propertyName))
            {
                m_fullName = AZStd::string::format("%.*s.%.*s", AZ_STRING_ARG(groupName), AZ_STRING_ARG(propertyName));
            }
        }
        
        MaterialPropertyId::MaterialPropertyId(const Name& groupName, const Name& propertyName)
            : MaterialPropertyId(groupName.GetStringView(), propertyName.GetStringView())
        {
        }
        
        MaterialPropertyId::MaterialPropertyId(const AZStd::span<AZStd::string> groupNames, AZStd::string_view propertyName)
        {
            for (const auto& name : groupNames)
            {
                if (!MaterialUtils::CheckIsValidGroupName(name))
                {
                    return;
                }
            }
            
            if (!MaterialUtils::CheckIsValidPropertyName(propertyName))
            {
                return;
            }

            if (groupNames.empty())
            {
                m_fullName = propertyName;
            }
            else
            {
                AZStd::string fullName;
                AzFramework::StringFunc::Join(fullName, groupNames.begin(), groupNames.end(), ".");
                fullName = AZStd::string::format("%s.%.*s", fullName.c_str(), AZ_STRING_ARG(propertyName));
                m_fullName = fullName;
            }
        }

        MaterialPropertyId::MaterialPropertyId(const AZStd::span<AZStd::string> names)
        {
            for (size_t i = 0; i < names.size(); ++i)
            {
                if (i == names.size() - 1)
                {
                    if (!MaterialUtils::CheckIsValidPropertyName(names[i]))
                    {
                        return;
                    }
                }
                else
                {
                    if (!MaterialUtils::CheckIsValidGroupName(names[i]))
                    {
                        return;
                    }
                }
            }

            AZStd::string fullName; // m_fullName is a Name, not a string, so we have to join into a local variable temporarily.
            AzFramework::StringFunc::Join(fullName, names.begin(), names.end(), ".");
            m_fullName = fullName;
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
