/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
