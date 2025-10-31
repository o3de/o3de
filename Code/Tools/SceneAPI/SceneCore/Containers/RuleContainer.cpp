/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneCore/Containers/RuleContainer.h>


namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            size_t RuleContainer::GetRuleCount() const
            {
                return m_rules.size();
            }


            AZStd::shared_ptr<DataTypes::IRule> RuleContainer::GetRule(size_t index) const
            {
                AZ_Assert(index < m_rules.size(), "Cannot get rule. Index %i is out of range.", index);

                if (index >= m_rules.size())
                {
                    return nullptr;
                }

                return m_rules[index];
            }


            void RuleContainer::AddRule(const AZStd::shared_ptr<DataTypes::IRule>& rule)
            {
                AZ_Assert(AZStd::find(m_rules.begin(), m_rules.end(), rule) == m_rules.end(), "Unable to add rule as it's already been added.");
                m_rules.push_back(rule);
            }


            void RuleContainer::AddRule(AZStd::shared_ptr<DataTypes::IRule>&& rule)
            {
                AZ_Assert(AZStd::find(m_rules.begin(), m_rules.end(), rule) == m_rules.end(), "Unable to add rule as it's already been added.");
                m_rules.push_back(rule);
            }


            void RuleContainer::InsertRule(const AZStd::shared_ptr<DataTypes::IRule>& rule, size_t position)
            {
                AZ_Assert(AZStd::find(m_rules.begin(), m_rules.end(), rule) == m_rules.end(), "Unable to insert rule as it has already been added.");
                m_rules.insert(m_rules.begin() + position, rule);
            }


            void RuleContainer::RemoveRule(size_t index)
            {
                if (index < m_rules.size())
                {
                    m_rules.erase(m_rules.begin() + index);
                }
            }


            void RuleContainer::RemoveRule(const AZStd::shared_ptr<DataTypes::IRule>& rule)
            {
                auto it = AZStd::find(m_rules.begin(), m_rules.end(), rule);

                if (it != m_rules.end())
                {
                    m_rules.erase(it);
                }
            }

            void RuleContainer::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (!serializeContext)
                {
                    return;
                }

                serializeContext->Class<RuleContainer>()
                    ->Version(1)
                    ->Field("rules", &RuleContainer::m_rules);

                EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<RuleContainer>("Rule Container", "Description.")
                        ->DataElement(AZ_CRC_CE("ManifestVector"), &RuleContainer::m_rules, "", "Add or remove entries to fine-tune source file processing.")
                            ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                            ->Attribute(AZ_CRC_CE("CollectionName"), "Modifiers")
                            ->Attribute(AZ_CRC_CE("ObjectTypeName"), "Modifier")
                            ->ElementAttribute(AZ::Edit::Attributes::Visibility, AZ_CRC_CE("PropertyVisibility_Hide"));
                }
            }

            
            // Previously, groups stored the vector of shared pointers of rules. We moved the vector of shared pointers of rules to the RuleContainer and
            // groups now have a RuleContainer as a member. This version converter converts from groups holding the vector
            bool RuleContainer::VectorToRuleContainerConverter(SerializeContext& context, SerializeContext::DataElementNode& classElement)
            {
                int elementIndex = classElement.FindElement(AZ_CRC_CE("rules"));
                if (elementIndex >= 0)
                {
                    AZ::SerializeContext::DataElementNode& rulesElement = classElement.GetSubElement(elementIndex);

                    // Clone the rule elements.
                    AZStd::vector<AZ::SerializeContext::DataElementNode> rules;
                    const int numSubElements = rulesElement.GetNumSubElements();
                    rules.reserve(numSubElements);

                    for (int i = 0; i < numSubElements; i++)
                    {
                        AZ::SerializeContext::DataElementNode& sharedPtrElement = rulesElement.GetSubElement(i);

                        if (sharedPtrElement.GetNumSubElements() > 0)
                        {
                            AZ::SerializeContext::DataElementNode& ruleElement = sharedPtrElement.GetSubElement(0);
                            rules.push_back(ruleElement);
                        }
                    }

                    // Remove the original rule vector element.
                    classElement.RemoveElement(elementIndex);

                    // Add a new rule container element.
                    const int ruleContainerIndex = classElement.AddElement<RuleContainer>(context, "rules");
                    if (ruleContainerIndex >= 0)
                    {
                        AZ::SerializeContext::DataElementNode& ruleContainerElement = classElement.GetSubElement(ruleContainerIndex);

                        // Create a rule vector element.
                        const int rulesVectorIndex = ruleContainerElement.AddElement<AZStd::vector<AZStd::shared_ptr<DataTypes::IRule>>>(context, "rules");
                        AZ::SerializeContext::DataElementNode& ruleVectorElement = ruleContainerElement.GetSubElement(rulesVectorIndex);
               
                        // Add the copied rules to the rule vector element.
                        for (SerializeContext::DataElementNode& rule : rules)
                        {
                            int valueIndex = ruleVectorElement.AddElement<AZStd::shared_ptr<DataTypes::IRule>>(context, "element");
                            SerializeContext::DataElementNode& pointerNode = ruleVectorElement.GetSubElement(valueIndex);

                            pointerNode.AddElement(rule);
                        }
                    }
                }

                return true;
            }

        } // Containers
    } // SceneAPI
} // AZ
