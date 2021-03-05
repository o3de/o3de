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
            // Scene system components are components that can be used to create system components
            //      in situations where the full initialization and/or construction of regular
            //      system components don't apply such as in the Project Configurator's advanced
            //      settings and the ResourceCompilerScene.
            class SCENE_CORE_CLASS SceneSystemComponent
                : public AZ::Component
            {
            public:
                AZ_COMPONENT(SceneSystemComponent, "{480FE393-A6BE-4AB9-AF91-11468AAFDB36}");

                ~SceneSystemComponent() override = default;

                SCENE_CORE_API void Activate() override;
                SCENE_CORE_API void Deactivate() override;

                static void Reflect(ReflectContext* context);
            };
        } // namespace SceneCore
    } // namespace SceneAPI
} // namespace AZ