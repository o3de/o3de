/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <SceneAPI/SceneCore/Components/BehaviorComponent.h>
#include <SceneAPI/SceneCore/Events/ManifestMetaInfoBus.h>
#include <SceneAPI/SceneCore/Events/GraphMetaInfoBus.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Behaviors
        {
            class SkinGroup
                : public SceneCore::BehaviorComponent
                , public Events::ManifestMetaInfoBus::Handler
                , public Events::GraphMetaInfoBus::Handler
                , public Events::AssetImportRequestBus::Handler
            {
            public:
                AZ_COMPONENT(SkinGroup, "{348566F7-7113-4CEB-ADF8-C1CC686CD3BD}", SceneCore::BehaviorComponent);

                static Crc32 s_skinVirtualType;
                static const char* s_skinVirtualTypeName;

                ~SkinGroup() override = default;

                void Activate() override;
                void Deactivate() override;
                static void Reflect(ReflectContext* context);

                void GetCategoryAssignments(CategoryRegistrationList& categories, const Containers::Scene& scene) override;
                void InitializeObject(const Containers::Scene& scene, DataTypes::IManifestObject& target) override;
                Events::ProcessingResult UpdateManifest(Containers::Scene& scene, ManifestAction action,
                    RequestingApplication requester) override;

                void GetVirtualTypes(AZStd::set<Crc32>& types, const Containers::Scene& scene,
                    Containers::SceneGraph::NodeIndex node) override;
                void GetAllVirtualTypes(AZStd::set<Crc32>& types) override;
                void GetVirtualTypeName(AZStd::string& name, Crc32 type) override;

            private:
                Events::ProcessingResult BuildDefault(Containers::Scene& scene) const;
                Events::ProcessingResult UpdateGroups(Containers::Scene& scene) const;

                bool SceneHasSkinGroup(const Containers::Scene& scene) const;

                static const int s_rigsPreferredTabOrder;
            };
        } // namespace Behaviors
    } // namespace SceneAPI
} // namespace AZ
