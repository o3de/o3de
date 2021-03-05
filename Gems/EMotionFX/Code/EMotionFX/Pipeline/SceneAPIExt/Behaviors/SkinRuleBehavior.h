#pragma once

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

#include <AzCore/Memory/Memory.h>
#include <SceneAPI/SceneCore/Events/ManifestMetaInfoBus.h>
#include <SceneAPI/SceneCore/Components/BehaviorComponent.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Behavior
        {
            class SkinRuleBehavior
                : public AZ::SceneAPI::SceneCore::BehaviorComponent
                , public AZ::SceneAPI::Events::ManifestMetaInfoBus::Handler
            {
            public:
                AZ_COMPONENT(SkinRuleBehavior, "{B212A863-32DD-4F92-948C-FC0ADAEEAB4A}", AZ::SceneAPI::SceneCore::BehaviorComponent);

                ~SkinRuleBehavior() override = default;

                // From BehaviorComponent
                void Activate() override;
                void Deactivate() override;
                static void Reflect(AZ::ReflectContext* context);

                void InitializeObject(const AZ::SceneAPI::Containers::Scene& scene, AZ::SceneAPI::DataTypes::IManifestObject& target) override;
            };
        } // Behavior
    } // Pipeline
} // EMotionFX