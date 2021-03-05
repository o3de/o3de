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

#include <SceneAPI/SceneCore/Components/BehaviorComponent.h>
#include <AzCore/std/string/string.h>
#include <SceneAPI/SceneCore/Events/ManifestMetaInfoBus.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class IGroup;
        }
        namespace SceneData
        {
            class MaterialRuleBehavior
                : public SceneCore::BehaviorComponent
                , public Events::ManifestMetaInfoBus::Handler
            {
            public:
                AZ_COMPONENT(MaterialRuleBehavior, "{14FD7ECE-195D-46A7-85AB-135F77D757DC}", SceneCore::BehaviorComponent);

                ~MaterialRuleBehavior() override = default;

                void Activate() override;
                void Deactivate() override;
                static void Reflect(ReflectContext* context);

                void InitializeObject(const Containers::Scene& scene, DataTypes::IManifestObject& target) override;
            };
        } // namespace Behaviors
    } // namespace SceneAPI
} // namespace AZ
