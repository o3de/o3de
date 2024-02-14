#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            static void Reflect(AZ::ReflectContext* context);

            ~MeshBehavior() override = default;

            // BehaviorComponent overrides ...
            void Activate() override;
            void Deactivate() override;

            // ManifestMetaInfoBus overrides ...
            void GetCategoryAssignments(CategoryRegistrationList& categories, const AZ::SceneAPI::Containers::Scene& scene) override;
            void GetAvailableModifiers(
                AZ::SceneAPI::Events::ManifestMetaInfo::ModifiersList& modifiers,
                const AZ::SceneAPI::Containers::Scene& scene,
                const AZ::SceneAPI::DataTypes::IManifestObject& target) override;
            void InitializeObject(const AZ::SceneAPI::Containers::Scene& scene, AZ::SceneAPI::DataTypes::IManifestObject& target) override;

            // AssetImportRequestBus overrides ...
            AZ::SceneAPI::Events::ProcessingResult UpdateManifest(AZ::SceneAPI::Containers::Scene& scene, ManifestAction action,
                RequestingApplication requester) override;
            void GetPolicyName(AZStd::string& result) const override
            {
                result = "PhysX::Pipeline::MeshBehavior";
            }

        private:
            AZ::SceneAPI::Events::ProcessingResult BuildDefault(AZ::SceneAPI::Containers::Scene& scene) const;
            AZ::SceneAPI::Events::ProcessingResult UpdatePhysXMeshGroups(AZ::SceneAPI::Containers::Scene& scene) const;

            static constexpr int s_meshBehaviorPreferredTabOrder{ 5 };
        };
    } // Pipeline
} // PhysX
