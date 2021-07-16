/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>
#include <SceneAPI/SceneCore/Components/BehaviorComponent.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>
#include <SceneAPI/SceneCore/Events/GraphMetaInfoBus.h>
#include <SceneAPI/SceneCore/Events/ManifestMetaInfoBus.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class IGroup;
            class ISceneNodeSelectionList;
        }
        namespace SceneData
        {
            class LodRule;

            class LodRuleBehavior
                : public SceneCore::BehaviorComponent
                , public Events::ManifestMetaInfoBus::Handler
                , public Events::AssetImportRequestBus::Handler
                , public Events::GraphMetaInfoBus::Handler
            {
            public:
                AZ_COMPONENT(LodRuleBehavior, "{D2E19864-9A4B-41FD-8ACC-DA6756728CB3}", SceneCore::BehaviorComponent);

                ~LodRuleBehavior() override = default;

                void Activate() override;
                void Deactivate() override;
                static void Reflect(ReflectContext* context);

                void InitializeObject(const Containers::Scene& scene, DataTypes::IManifestObject& target) override;
                Events::ProcessingResult UpdateManifest(Containers::Scene& scene, ManifestAction action,
                    RequestingApplication requester) override;

                void GetVirtualTypeName(AZStd::string& name, Crc32 type) override;
                void GetAllVirtualTypes(AZStd::set<Crc32>& types) override;

            private:
                size_t SelectLodMeshes(const Containers::Scene& scene, DataTypes::ISceneNodeSelectionList& selection, size_t lodLevel) const;
                void UpdateLodRules(Containers::Scene& scene) const;
            };
        } // namespace SceneData
    } // namespace SceneAPI
} // namespace AZ
