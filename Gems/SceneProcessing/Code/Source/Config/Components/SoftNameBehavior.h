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
#include <SceneAPI/SceneCore/Events/GraphMetaInfoBus.h>

namespace AZ
{
    namespace SceneProcessingConfig
    {
        class SoftNameBehavior
            : public SceneAPI::SceneCore::BehaviorComponent
            , protected SceneAPI::Events::GraphMetaInfoBus::Handler
        {
        public:
            AZ_COMPONENT(SoftNameBehavior, "{C2A9D207-485F-4752-B37B-388B0A52A956}", SceneAPI::SceneCore::BehaviorComponent);

            ~SoftNameBehavior() override = default;

            void Activate() override;
            void Deactivate() override;

            void GetVirtualTypes(AZStd::set<Crc32>& types, const SceneAPI::Containers::Scene& scene,
                SceneAPI::Containers::SceneGraph::NodeIndex node) override;
            void GetVirtualTypeName(AZStd::string& name, Crc32 type) override;
            void GetAllVirtualTypes(AZStd::set<Crc32>& types) override;

            static void Reflect(ReflectContext* context);
        };
    } // namespace SceneProcessingConfig
} // namespace AZ
