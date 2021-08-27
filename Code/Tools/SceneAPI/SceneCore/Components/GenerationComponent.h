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

namespace AZ::SceneAPI::SceneCore
{
    // Generation components are small logic units that exist only during scene generation. Each of
    //      these components take care of a piece of the generation process, allowing
    //      multiple components to do runtime creation of scene graph objects.
    //      Use the BindToCall from the CallProcessorBinder to be able to react to specific
    //      loading contexts/events.
    class SCENE_CORE_CLASS GenerationComponent
        : public AZ::Component
        , public Events::CallProcessorBinder
    {
    public:
        AZ_COMPONENT(GenerationComponent, "{3DBA42C1-894E-4437-B046-BC399E34366B}", Events::CallProcessorBinder);

        SCENE_CORE_API void Activate() override;
        SCENE_CORE_API void Deactivate() override;

        static void Reflect(ReflectContext* context);
    };
} // namespace AZ::SceneAPI::SceneCore
