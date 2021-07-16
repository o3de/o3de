#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/Memory.h>
#include <SceneAPI/SceneCore/Events/ManifestMetaInfoBus.h>
#include <SceneAPI/SceneCore/Components/BehaviorComponent.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneData
        {
            class SkinRuleBehavior
                : public SceneCore::BehaviorComponent
                , public Events::ManifestMetaInfoBus::Handler
            {
            public:
                AZ_COMPONENT(SkinRuleBehavior, "{B212A863-32DD-4F92-948C-FC0ADAEEAB4A}", SceneCore::BehaviorComponent);

                ~SkinRuleBehavior() override = default;

                // From BehaviorComponent
                void Activate() override;
                void Deactivate() override;
                static void Reflect(AZ::ReflectContext* context);

                void InitializeObject(const Containers::Scene& scene, DataTypes::IManifestObject& target) override;
            };
        }
    }
}
