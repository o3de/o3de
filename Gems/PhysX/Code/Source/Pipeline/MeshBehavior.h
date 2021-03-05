#pragma once

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

#include <SceneAPI/SceneCore/Components/BehaviorComponent.h>
#include <SceneAPI/SceneCore/Events/ManifestMetaInfoBus.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>

namespace PhysX
{
    namespace Pipeline
    {
        class MeshBehavior
            : public AZ::SceneAPI::SceneCore::BehaviorComponent
            , public AZ::SceneAPI::Events::ManifestMetaInfoBus::Handler
            , public AZ::SceneAPI::Events::AssetImportRequestBus::Handler
        {
        public:
            AZ_COMPONENT(MeshBehavior, "{B6AFB216-2A49-402F-A2B1-C3A17812D53F}", AZ::SceneAPI::SceneCore::BehaviorComponent);

            ~MeshBehavior() override = default;

            void Activate() override;
            void Deactivate() override;
            static void Reflect(AZ::ReflectContext* context);

            void GetCategoryAssignments(CategoryRegistrationList& categories, const AZ::SceneAPI::Containers::Scene& scene) override;
            void InitializeObject(const AZ::SceneAPI::Containers::Scene& scene, AZ::SceneAPI::DataTypes::IManifestObject& target) override;
            AZ::SceneAPI::Events::ProcessingResult UpdateManifest(AZ::SceneAPI::Containers::Scene& scene, ManifestAction action,
                RequestingApplication requester) override;

        private:
            AZ::SceneAPI::Events::ProcessingResult BuildDefault(AZ::SceneAPI::Containers::Scene& scene) const;
            AZ::SceneAPI::Events::ProcessingResult UpdatePhysXMeshGroups(AZ::SceneAPI::Containers::Scene& scene) const;
        };
    } // Pipeline
} // PhysX
