/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
            // Loading components are small logic units that exist only during loading. Each of
            //      these components take care of a small piece of the loading process, allowing
            //      multiple components to use the same sources to collect data.
            //      Use the BindToCall from the CallProcessorBinder to be able to react to specific
            //      loading contexts/events.
            class SCENE_CORE_CLASS LoadingComponent
                : public AZ::Component
                , public Events::CallProcessorBinder
            {
            public:
                AZ_COMPONENT(LoadingComponent, "{335A696D-38DA-4A4F-B3F3-DBAD1FE86888}", Events::CallProcessorBinder);

                LoadingComponent() = default;
                ~LoadingComponent() override = default;

                SCENE_CORE_API void Activate() override;
                SCENE_CORE_API void Deactivate() override;

                static void Reflect(ReflectContext* context);
            };
        } // namespace SceneCore
    } // namespace SceneAPI
} // namespace AZ
