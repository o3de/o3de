/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SceneAPIExt/Rules/RootMotionExtractionRule.h>
#include <SceneAPIExt/Behaviors/RootMotionExtractionRuleBehavior.h>

namespace EMotionFX::Pipeline::Behavior
{
    // List of default names for the sample joints. The first joint name matches any of those names will be default selected.
    static constexpr const AZStd::array s_defaultSampleJoints { "Hip", "Pelvis" };

    void RootMotionExtractionRuleBehavior::Reflect(AZ::ReflectContext* context)
    {
        Rule::RootMotionExtractionRule::Reflect(context);

        auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<RootMotionExtractionRuleBehavior, AZ::SceneAPI::SceneCore::BehaviorComponent>()->Version(1);
        }
    }

    void RootMotionExtractionRuleBehavior::Activate()
    {
        AZ::SceneAPI::Events::ManifestMetaInfoBus::Handler::BusConnect();
    }

    void RootMotionExtractionRuleBehavior::Deactivate()
    {
        AZ::SceneAPI::Events::ManifestMetaInfoBus::Handler::BusDisconnect();
    }

    void RootMotionExtractionRuleBehavior::InitializeObject(const AZ::SceneAPI::Containers::Scene& scene, AZ::SceneAPI::DataTypes::IManifestObject& target)
    {
        if (!target.RTTI_IsTypeOf(Rule::RootMotionExtractionRule::TYPEINFO_Uuid()))
        {
            return;
        }

        Rule::RootMotionExtractionRule* rootMotionExtractionRule = azrtti_cast<Rule::RootMotionExtractionRule*>(&target);
        auto data = rootMotionExtractionRule->GetData();
        if (!data)
        {
            return;
        }

        auto nameStorage = scene.GetGraph().GetNameStorage();
        for (auto it = nameStorage.begin(); it != nameStorage.end(); ++it)
        {
            for (const char* defaultJoint : s_defaultSampleJoints)
            {
                // Select the first matching joint in the default name list.
                if (AzFramework::StringFunc::Find(it->GetName(), defaultJoint) != AZStd::string::npos)
                {
                    // Set the sample joint name to the 'path' which is unique.
                    data->m_sampleJoint = it->GetPath();
                    rootMotionExtractionRule->SetData(data);
                    return;
                }
            }
        }
    }
} // namespace EMotionFX::Pipeline::Behavior
