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
#include <SceneAPI/SceneCore/Events/ManifestMetaInfoBus.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Behaviors
        {
            class SkeletonGroup 
                : public SceneCore::BehaviorComponent
                , public Events::ManifestMetaInfoBus::Handler
                , public Events::AssetImportRequestBus::Handler
            {
            public:
                AZ_COMPONENT(SkeletonGroup, "{9243A4BA-46BD-4961-950F-DEFAE9A919E5}", SceneCore::BehaviorComponent);

                ~SkeletonGroup() override = default;

                void Activate() override;
                void Deactivate() override;
                static void Reflect(ReflectContext* context);

                void GetCategoryAssignments(CategoryRegistrationList& categories, const Containers::Scene& scene) override;
                void InitializeObject(const Containers::Scene& scene, DataTypes::IManifestObject& target) override;
                Events::ProcessingResult UpdateManifest(Containers::Scene& scene, ManifestAction action,
                    RequestingApplication requester) override;

            private:
                Events::ProcessingResult BuildDefault(Containers::Scene& scene);
                Events::ProcessingResult UpdateSkeletonGroups(Containers::Scene& scene) const;

                bool SceneHasSkeletonGroup(const Containers::Scene& scene) const;
                
                static const int s_rigsPreferredTabOrder;
                bool m_isDefaultConstructing{ false };
            };
        } // namespace Behaviors
    } // namespace SceneAPI
} // namespace AZ
