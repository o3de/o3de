/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <SceneAPI/SceneBuilder/ImportContextRegistryManager.h>
#include <SceneAPI/SceneCore/SceneCoreConfiguration.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneBuilder
        {
            // Scene system components are components that can be used to create system components
            //      in situations where the full initialization and/or construction of regular
            //      system components don't apply such as in the ResourceCompilerScene.
            class SceneBuilderSystemComponent
                : public AZ::Component
            {
            public:
                AZ_COMPONENT(SceneBuilderSystemComponent, "{9453ddf4-882c-4675-86eb-834f1d1dc5ef}");

                ~SceneBuilderSystemComponent() override = default;

                void Activate() override;
                void Deactivate() override;

                static void Reflect(ReflectContext* context);
            private:
                ImportContextRegistryManager m_sceneSystemRegistry;
            };
        } // namespace SceneCore
    } // namespace SceneAPI
} // namespace AZ
