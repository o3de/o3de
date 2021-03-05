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

#include <AzCore/IO/FileIO.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IGroup.h>
#include <SceneAPI/SceneCore/Events/ManifestMetaInfoBus.h>
#include <SceneAPI/SceneCore/Events/SceneSerializationBus.h>
#include <SceneAPIExt/Rules/ExternalToolRule.h>
#include <MCore/Source/StringConversions.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Rule
        {
            template<class RuleClass, class ReflectableData>
            bool LoadFromGroup(const AZ::SceneAPI::DataTypes::IGroup& group, ReflectableData& outData)
            {
                AZStd::shared_ptr<RuleClass> rule = group.GetRuleContainerConst().FindFirstByType<RuleClass>();
                if (!rule)
                {
                    // TODO: Force to have some common clear method? Or make it a restriction that the object is empty when being passed?
                    //outMetaDataString.clear();
                    return false;
                }

                outData = rule->GetData();
                return true;
            }


            template<class RuleClass, class ReflectableData>
            void SaveToGroup(AZ::SceneAPI::Containers::Scene& scene, AZ::SceneAPI::DataTypes::IGroup& group, const ReflectableData& data)
            {
                namespace SceneEvents = AZ::SceneAPI::Events;
                AZ::SceneAPI::Containers::RuleContainer& rules = group.GetRuleContainer();

                // Update the data in case there is a rule of our type already.
                AZStd::shared_ptr<RuleClass> rule = rules.FindFirstByType<RuleClass>();
                if (rule)
                {
                    rule->SetData(data);
                    SceneEvents::ManifestMetaInfoBus::Broadcast(&SceneEvents::ManifestMetaInfoBus::Events::ObjectUpdated, scene, rule.get(), nullptr);
                }
                else
                {
                    // No rule exists yet, create one and add it.
                    rule = AZStd::make_shared<RuleClass>(data);
                    rules.AddRule(rule);
                    SceneEvents::ManifestMetaInfoBus::Broadcast(&SceneEvents::ManifestMetaInfoBus::Events::InitializeObject, scene, *rule);
                    SceneEvents::ManifestMetaInfoBus::Broadcast(&SceneEvents::ManifestMetaInfoBus::Events::ObjectUpdated, scene, &group, nullptr);
                }
            }


            template<class RuleClass, class ReflectableData>
            void RemoveRuleFromGroup(AZ::SceneAPI::Containers::Scene& scene, AZ::SceneAPI::DataTypes::IGroup& group)
            {
                namespace SceneEvents = AZ::SceneAPI::Events;
                AZ::SceneAPI::Containers::RuleContainer& rules = group.GetRuleContainer();

                AZStd::shared_ptr<RuleClass> rule = rules.FindFirstByType<RuleClass>();
                if (rule)
                {
                    rules.RemoveRule(rule);
                    SceneEvents::ManifestMetaInfoBus::Broadcast(&SceneEvents::ManifestMetaInfoBus::Events::ObjectUpdated, scene, &group, nullptr);
                }
            }
        } // Rule
    } // Pipeline
} // EMotionFX