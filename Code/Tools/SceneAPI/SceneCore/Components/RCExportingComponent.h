/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
