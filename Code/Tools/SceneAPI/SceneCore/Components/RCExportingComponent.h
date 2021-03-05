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
#include <SceneAPI/SceneCore/Events/CallProcessorBinder.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneCore
        {
            // Component used to support legacy systems. Use ExportingComponent for any new
            //      development.
            class SCENE_CORE_CLASS RCExportingComponent
                : public AZ::Component
                , public Events::CallProcessorBinder
            {
            public:
                AZ_COMPONENT(RCExportingComponent, "{128286A3-41EF-4910-8C62-E9EECA43C4EF}", Events::CallProcessorBinder);

                RCExportingComponent() = default;

                ~RCExportingComponent() override = default;

                SCENE_CORE_API void Activate() override;
                SCENE_CORE_API void Deactivate() override;

                static void Reflect(ReflectContext* context);
            };
        } // namespace SceneCore
    } // namespace SceneAPI
} // namespace AZ