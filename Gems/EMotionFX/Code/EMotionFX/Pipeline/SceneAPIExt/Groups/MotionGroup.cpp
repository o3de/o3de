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

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IBoneData.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>

#include <SceneAPIExt/Rules/MetaDataRule.h>
#include <SceneAPIExt/Rules/MotionRangeRule.h>
#include <SceneAPIExt/Rules/MotionCompressionSettingsRule.h>
#include <SceneAPIExt/Rules/MotionSamplingRule.h>
#include <SceneAPIExt/Groups/MotionGroup.h>

#include <EMotionFX/Source/MotionData/NonUniformMotionData.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Group
        {
            AZ_CLASS_ALLOCATOR_IMPL(MotionGroup, AZ::SystemAllocator, 0)

            MotionGroup::MotionGroup()
                : m_id(AZ::Uuid::CreateRandom())
            {
            }

            void MotionGroup::SetName(const AZStd::string& name)
            {
                m_name = name;
            }

            void MotionGroup::SetName(AZStd::string&& name)
            {
                m_name = AZStd::move(name);
            }

            const AZ::Uuid& MotionGroup::GetId() const
            {
                return m_id;
            }

            void MotionGroup::OverrideId(const AZ::Uuid& id)
            {
                m_id = id;
            }

            AZ::SceneAPI::Containers::RuleContainer& MotionGroup::GetRuleContainer()
            {
                return m_rules;
            }

            const AZ::SceneAPI::Containers::RuleContainer& MotionGroup::GetRuleContainerConst() const
            {
                return m_rules;
            }

            const AZStd::string& MotionGroup::GetSelectedRootBone() const
            {
                return m_selectedRootBone;
            }

            void MotionGroup::SetSelectedRootBone(const AZStd::string& selectedRootBone)
            {
                m_selectedRootBone = selectedRootBone;
            }

            const AZStd::string& MotionGroup::GetName() const
            {
                return m_name;
            }
            
            void MotionGroup::Reflect(AZ::ReflectContext* context)
            {
                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                if (!serializeContext)
                {
                    return;
                }

                serializeContext->Class<IMotionGroup, AZ::SceneAPI::DataTypes::IGroup>()->Version(1);

                serializeContext->Class<MotionGroup, IMotionGroup>()->Version(4, VersionConverter)
                    ->Field("name", &MotionGroup::m_name)
                    ->Field("selectedRootBone", &MotionGroup::m_selectedRootBone)
                    ->Field("id", &MotionGroup::m_id)
                    ->Field("rules", &MotionGroup::m_rules);

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<MotionGroup>("Motion", "Configure animation data for exporting.")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute("AutoExpand", true)
                            ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                        ->DataElement(AZ_CRC("ManifestName", 0x5215b349), &MotionGroup::m_name, "Name motion",
                            "Name for the group. This name will also be used as the name for the generated file.")
                            ->Attribute("FilterType", IMotionGroup::TYPEINFO_Uuid())
                        ->DataElement("NodeListSelection", &MotionGroup::m_selectedRootBone, "Select root bone", "The root bone of the animation that will be exported.")
                            ->Attribute("ClassTypeIdFilter", AZ::SceneAPI::DataTypes::IBoneData::TYPEINFO_Uuid())
                        ->DataElement(AZ::Edit::UIHandlers::Default, &MotionGroup::m_rules, "", "Add or remove rules to fine-tune the export process.")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly", 0xef428f20));
                }
            }

            bool MotionGroup::VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
            {
                const unsigned int version = classElement.GetVersion();

                bool result = true;
                // Added a uuid "id" as the unique identifier to replace the file name.
                // Setting it to null by default and expecting a behavior to patch this when additional information is available.
                if (version < 2)
                {
                    result = result && classElement.AddElementWithData<AZ::Uuid>(context, "id", AZ::Uuid::CreateNull()) != -1;
                }

                // Start frame and end frame is moved to motion range rule.
                if (version < 3)
                {
                    AZ::u32 startFrame, endFrame;
                    AZ::SerializeContext::DataElementNode* startFrameNode = classElement.FindSubElement(AZ_CRC("startFrame", 0xd7642114));
                    AZ::SerializeContext::DataElementNode* endFrameNode = classElement.FindSubElement(AZ_CRC("endFrame", 0xc61fc092));
                    if (startFrameNode && endFrameNode)
                    {
                        startFrameNode->GetData<AZ::u32>(startFrame);
                        endFrameNode->GetData<AZ::u32>(endFrame);

                        AZ::SceneAPI::Containers::RuleContainer ruleContainer;
                        AZ::SerializeContext::DataElementNode* ruleContainerNode = classElement.FindSubElement(AZ_CRC("rules", 0x899a993c));
                        if (ruleContainerNode)
                        {
                            ruleContainerNode->GetDataHierarchy<AZ::SceneAPI::Containers::RuleContainer>(context, ruleContainer);
                            ruleContainerNode->RemoveElementByName(AZ_CRC("rules", 0x899a993c));
                        }
                        else
                        {
                            AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "Can't find rule container.\n");
                            return false;
                        }

                        AZStd::shared_ptr<Rule::MotionRangeRule> rule = AZStd::make_shared<Rule::MotionRangeRule>();
                        rule->SetStartFrame(startFrame);
                        rule->SetEndFrame(endFrame);
                        rule->SetProcessRangeRuleConversion(true);
                        ruleContainer.AddRule(rule);
                        ruleContainerNode->SetData(context, ruleContainer);

                        classElement.RemoveElementByName(AZ_CRC("startFrame", 0xd7642114));
                        classElement.RemoveElementByName(AZ_CRC("endFrame", 0xc61fc092));
                    }
                }

                // Motion compression settings rule is converted to motion sampling rule under 'Non-uniform sampling'.
                if (version < 4)
                {
                    AZ::SerializeContext::DataElementNode* ruleContainerNode = classElement.FindSubElement(AZ_CRC("rules", 0x899a993c));
                    if (!ruleContainerNode)
                    {
                        AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "Can't find rule container.\n");
                        return false;
                    }

                    AZ::SerializeContext::DataElementNode* rulesNode = ruleContainerNode->FindSubElement(AZ_CRC("rules", 0x899a993c));
                    if (!rulesNode)
                    {
                        AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "Can't find rules within rule container.\n");
                        return false;
                    }

                    const int numRules = rulesNode->GetNumSubElements();
                    float translationError = 0.0f;
                    float rotationError = 0.0f;
                    float scaleError= 0.0f;
                    for (int i = 0; i < numRules; ++i)
                    {
                        AZ::SerializeContext::DataElementNode& sharedPointerNode = rulesNode->GetSubElement(i);
                        if (sharedPointerNode.GetNumSubElements() == 1)
                        {
                            AZ::SerializeContext::DataElementNode& currentRuleNode = sharedPointerNode.GetSubElement(0);
                            if (currentRuleNode.GetId() == azrtti_typeid<Rule::MotionCompressionSettingsRule>())
                            {
                                currentRuleNode.FindSubElementAndGetData(AZ_CRC("maxTranslationError"), translationError);
                                currentRuleNode.FindSubElementAndGetData(AZ_CRC("maxRotationError"), rotationError);
                                currentRuleNode.FindSubElementAndGetData(AZ_CRC("maxScaleError"), scaleError);

                                // Create the motion sampling rule.
                                AZStd::shared_ptr<Rule::MotionSamplingRule> motionSamplingRule = AZStd::make_shared<Rule::MotionSamplingRule>();
                                motionSamplingRule->SetMotionDataTypeId(AZ::TypeId::CreateNull());  // Set to automatic mode.

                                // Convert the old compression data to new percentage data.
                                motionSamplingRule->SetTranslationQualityByTranslationError(translationError);
                                motionSamplingRule->SetScaleQualityByScaleError(scaleError);
                                // NOTE: Rotation error was calculated using vector differences in old system, but in the new algorithm is using quaternion comparison. 
                                motionSamplingRule->SetRotationQualityByRotationError(rotationError);

                                // Overwrite the rules.
                                AZ::SceneAPI::Containers::RuleContainer ruleContainer;
                                ruleContainerNode->GetDataHierarchy<AZ::SceneAPI::Containers::RuleContainer>(context, ruleContainer);
                                ruleContainer.RemoveRule(i);
                                ruleContainer.AddRule(motionSamplingRule);
                                ruleContainerNode->SetData(context, ruleContainer);
                                break;
                            }
                        }
                    }
                }

                return result;
            }
        }
    }
}
