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
    const char* RootMotionExtractionRuleBehavior::s_defaultSampleJoint = "hip";

    void RootMotionExtractionRuleBehavior::Reflect(AZ::ReflectContext* context)
    {
        Rule::RootMotionExtractionRule::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
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
        auto nameStorage = scene.GetGraph().GetNameStorage();
        for (auto it = nameStorage.begin(); it != nameStorage.end(); ++it)
        {
            if (AzFramework::StringFunc::Find(it->GetName(), s_defaultSampleJoint) != AZStd::string::npos)
            {
                Rule::RootMotionExtractionData data = rootMotionExtractionRule->GetData();
                // Set the sample joint name to the 'path' which is unique.
                data.m_sampleJoint = it->GetPath();
                rootMotionExtractionRule->SetData(data);
                return;
            }
        }
    }
}
