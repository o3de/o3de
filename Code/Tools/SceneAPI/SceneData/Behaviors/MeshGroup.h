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

namespace AZ
{
    namespace SceneAPI
    {
        namespace Behaviors
        {
            class MeshGroup 
                : public SceneCore::BehaviorComponent
                , public Events::ManifestMetaInfoBus::Handler
                , public Events::AssetImportRequestBus::Handler
            {
            public:
                AZ_COMPONENT(MeshGroup, "{52DD90C2-81F5-4763-AC64-6DB2294BE50A}", SceneCore::BehaviorComponent);

                ~MeshGroup() override = default;

                void Activate() override;
                void Deactivate() override;
                static void Reflect(ReflectContext* context);

                void GetCategoryAssignments(CategoryRegistrationList& categories, const Containers::Scene& scene) override;
                void InitializeObject(const Containers::Scene& scene, DataTypes::IManifestObject& target) override;
                Events::ProcessingResult UpdateManifest(Containers::Scene& scene, ManifestAction action,
                    RequestingApplication requester) override;
                void GetPolicyName(AZStd::string& result) const override
                {
                    result = "SceneAPI::MeshGroup";
                }

            private:
                Events::ProcessingResult BuildDefault(Containers::Scene& scene) const;
                Events::ProcessingResult UpdateMeshGroups(Containers::Scene& scene) const;

                bool SceneHasMeshGroup(const Containers::Scene& scene) const;

                static constexpr int s_meshGroupPreferredTabOrder{ 0 };
            };
        } // Behaviors
    } // SceneAPI
} // AZ
