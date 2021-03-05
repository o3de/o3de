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

#pragma once

#include <AzCore/Component/Component.h>
#include <SceneAPI/SceneCore/SceneCoreConfiguration.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneCore
        {
            // Behavior components are small logic units that exist as long as the SceneAPI is
            //      initialized and active. These components can react to various events that
            //      happen to a scene and make appropriate changes, additions or removals. These
            //      components are also responsible to register their associated data with the
            //      reflect context.
            class SCENE_CORE_CLASS BehaviorComponent
                : public AZ::Component
            {
            public:
                AZ_COMPONENT(BehaviorComponent, "{DA66AE07-9ECF-4108-9CCC-9BFF618DD4AD}");

                ~BehaviorComponent() override = default;

                SCENE_CORE_API void Activate() override;
                SCENE_CORE_API void Deactivate() override;

                static void Reflect(ReflectContext* context);
            };
        } // namespace SceneCore
    } // namespace SceneAPI
} // namespace AZ