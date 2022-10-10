/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <SceneAPI/SceneCore/Components/BehaviorComponent.h>
#include <AzCore/std/string/string.h>
#include <SceneAPI/SceneCore/Events/ManifestMetaInfoBus.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {

            class IMeshAdvancedRule;
        }

        namespace Behaviors
        {
            class MeshAdvancedRule 
                : public SceneCore::BehaviorComponent
                , public Events::ManifestMetaInfoBus::Handler
                , public Events::AssetImportRequestBus::Handler
            {
            public:
                AZ_COMPONENT(MeshAdvancedRule, "{4217B46E-87A6-438E-8ACE-0397828AE889}", SceneCore::BehaviorComponent);

                ~MeshAdvancedRule() override = default;

                void Activate() override;
                void Deactivate() override;
                static void Reflect(ReflectContext* context);

                void InitializeObject(const Containers::Scene& scene, DataTypes::IManifestObject& target) override;
                Events::ProcessingResult UpdateManifest(Containers::Scene& scene, ManifestAction action,
                    RequestingApplication requester) override;
                void GetPolicyName(AZStd::string& result) const override
                {
                    result = "MeshAdvancedRule";
                }

            private:
                void UpdateMeshAdvancedRules(Containers::Scene& scene) const;

                void UpdateMeshAdvancedRule(Containers::Scene& scene, DataTypes::IMeshAdvancedRule* rule) const;

                AZStd::string GetFirstVertexColorStream(const Containers::Scene& scene) const;
            };
        } // namespace Behaviors
    } // namespace SceneAPI
} // namespace AZ
