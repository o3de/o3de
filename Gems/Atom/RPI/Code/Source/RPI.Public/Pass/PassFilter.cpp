/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>

namespace AZ
{
    namespace RPI
    {
        PassHierarchyFilter::PassHierarchyFilter(const Name& passName)
        {
            m_passName = passName;
        }

        PassHierarchyFilter::PassHierarchyFilter(const AZStd::vector<AZStd::string>& passHierarchy)
        {
            if (passHierarchy.size() == 0)
            {
                AZ_Assert(false, "passHierarchy should have at least one element");
                return;
            }

            m_passName = Name(passHierarchy.back());

            m_parentNames.resize(passHierarchy.size() - 1);
            for (uint32_t index = 0; index < m_parentNames.size(); index++)
            {
                m_parentNames[index] = Name(passHierarchy[index]);
            }
        }

        PassHierarchyFilter::PassHierarchyFilter(const AZStd::vector<Name>& passHierarchy)
        {
            if (passHierarchy.size() == 0)
            {
                AZ_Assert(false, "passHierarchy should have at least one element");
                return;
            }

            m_passName = passHierarchy.back();

            m_parentNames.resize(passHierarchy.size() - 1);
            for (uint32_t index = 0; index < m_parentNames.size(); index++)
            {
                m_parentNames[index] = passHierarchy[index];
            }
        }

        bool PassHierarchyFilter::Matches(const Pass* pass) const
        {
            if (pass->GetName() != m_passName)
            {
                return false;
            }

            ParentPass* parent = pass->GetParent();

            // search from the back of the array with the most close parent 
            for (int32_t index = static_cast<int32_t>(m_parentNames.size() - 1); index >= 0; index--)
            {
                const Name& parentName = m_parentNames[index];
                while (parent)
                {
                    if (parent->GetName() == parentName)
                    {
                        break;
                    }
                    parent = parent->GetParent();
                }

                // if parent is nullptr the it didn't find a parent has matching current parentName
                if (!parent)
                {
                    return false;
                }

                // move to next parent 
                parent = parent->GetParent();
            }

            return true;
        }

        const Name* PassHierarchyFilter::GetPassName() const
        {
            return &m_passName;
        }

        AZStd::string PassHierarchyFilter::ToString() const
        {
            AZStd::string result = "PassHierarchyFilter";
            for (uint32_t index = 0; index < m_parentNames.size(); index++)
            {
                result += AZStd::string::format(" [%s]", m_parentNames[index].GetCStr());
            }

            result += AZStd::string::format(" [%s]", m_passName.GetCStr());
            return result;
        }

    }   // namespace RPI
}   // namespace AZ
