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

#include <SceneAPI/SceneCore/Containers/RuleContainer.h>


namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {

            template<typename T>
            AZStd::shared_ptr<T> RuleContainer::FindFirstByType() const
            {
                static_assert(AZStd::is_base_of<DataTypes::IRule, T>::value, "Specified type T is not derived from IRule.");

                for (const AZStd::shared_ptr<DataTypes::IRule>& rule : m_rules)
                {
                    if (rule && rule->RTTI_IsTypeOf(T::TYPEINFO_Uuid()))
                    {
                        return AZStd::static_pointer_cast<T>(rule);
                    }
                }

                return AZStd::shared_ptr<T>();
            }


            template<typename T>
            bool RuleContainer::ContainsRuleOfType() const
            {
                static_assert(AZStd::is_base_of<DataTypes::IRule, T>::value, "Specified type T is not derived from IRule.");

                for (const AZStd::shared_ptr<DataTypes::IRule>& rule : m_rules)
                {
                    if (rule && rule->RTTI_IsTypeOf(T::TYPEINFO_Uuid()))
                    {
                        return true;
                    }
                }

                return false;
            }

        } // Containers
    } // SceneAPI
} // AZ