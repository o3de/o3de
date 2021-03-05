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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/Containers/Utilities/SceneGraphUtilities.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/ISceneNodeGroup.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMaterialData.h>
#include <SceneAPI/SceneData/Rules/MaterialRule.h>
#include <SceneAPI/SceneData/Behaviors/MaterialRuleBehavior.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneData
        {
            void MaterialRuleBehavior::Activate()
            {
                BusConnect();
            }

            void MaterialRuleBehavior::Deactivate()
            {
                BusDisconnect();
            }

            void MaterialRuleBehavior::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<MaterialRuleBehavior, BehaviorComponent>()->Version(1);
                }
            }

            void MaterialRuleBehavior::InitializeObject(const Containers::Scene& scene, DataTypes::IManifestObject& target)
            {
                if (target.RTTI_IsTypeOf(DataTypes::ISceneNodeGroup::TYPEINFO_Uuid()))
                {
                    DataTypes::ISceneNodeGroup* sceneNodeGroup = azrtti_cast<DataTypes::ISceneNodeGroup*>(&target);

                    // Note that other behaviors such as the physics can also add a material rule.
                    Containers::RuleContainer& rules = sceneNodeGroup->GetRuleContainer();
                    if (Utilities::DoesSceneGraphContainDataLike<DataTypes::IMaterialData>(scene, true) && !rules.ContainsRuleOfType<DataTypes::IMaterialRule>())
                    {
                        Events::ManifestMetaInfo::ModifiersList modifiers;
                        Events::ManifestMetaInfoBus::Broadcast(&Events::ManifestMetaInfo::GetAvailableModifiers, modifiers, scene, target);
                        if (AZStd::find(modifiers.begin(), modifiers.end(), azrtti_typeid<SceneData::MaterialRule>()) != modifiers.end())
                        {
                            rules.AddRule(AZStd::make_shared<SceneData::MaterialRule>());
                        }
                    }
                }
            }

        } // namespace SceneData
    } // namespace SceneAPI
} // namespace AZ
