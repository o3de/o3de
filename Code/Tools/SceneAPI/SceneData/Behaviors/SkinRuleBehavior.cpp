/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/Utilities/SceneGraphUtilities.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IMeshGroup.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/ISkinWeightData.h>
#include <SceneAPI/SceneData/Rules/SkinRule.h>
#include <SceneAPI/SceneData/Behaviors/SkinRuleBehavior.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneData
        {
            void SkinRuleBehavior::Reflect(AZ::ReflectContext* context)
            {
                SkinRule::Reflect(context);

                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<SkinRuleBehavior, SceneCore::BehaviorComponent>()->Version(1);
                }
            }

            void SkinRuleBehavior::Activate()
            {
                AZ::SceneAPI::Events::ManifestMetaInfoBus::Handler::BusConnect();
            }

            void SkinRuleBehavior::Deactivate()
            {
                AZ::SceneAPI::Events::ManifestMetaInfoBus::Handler::BusDisconnect();
            }

            void SkinRuleBehavior::InitializeObject([[maybe_unused]] const Containers::Scene& scene, DataTypes::IManifestObject& target)
            {
                if (azrtti_istypeof<DataTypes::IMeshGroup>(target))
                {
                    DataTypes::IMeshGroup* meshGroup = azrtti_cast<DataTypes::IMeshGroup*>(&target);

                    AZ::SceneAPI::Containers::RuleContainer& rules = meshGroup->GetRuleContainer();
                    // Only add the skin rule if the scene contain any skinning data.
                    if (!rules.ContainsRuleOfType<SkinRule>() && Utilities::DoesSceneGraphContainDataLike<DataTypes::ISkinWeightData>(scene, true))
                    {
                        rules.AddRule(AZStd::make_shared<SkinRule>());
                    }
                }
            }
        }
    }
}
