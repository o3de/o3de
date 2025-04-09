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
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class ISceneNodeGroup;
        }

        namespace Containers
        {
            class Scene;
            class SceneGraph;
        }
    }
}

namespace NvCloth
{
    namespace Pipeline
    {
        class ClothRule;

        //! This class defines the behavior of how to treat the cloth rule data
        //! through the SceneAPI.
        //! It specifies the valid Scene Groups that are allowed to have
        //! cloth rules (aka cloth modifiers), these are Mesh and Actor groups.
        //! It also validates the cloth rules data for the manifest (asset containing
        //! all the Scene information from the Scene Settings).
        class ClothRuleBehavior
            : public AZ::SceneAPI::SceneCore::BehaviorComponent
            , public AZ::SceneAPI::Events::ManifestMetaInfoBus::Handler
            , public AZ::SceneAPI::Events::AssetImportRequestBus::Handler
        {
        public:
            AZ_COMPONENT(ClothRuleBehavior, "{00FA6C8A-27D2-4C0E-B601-6917950432E5}", AZ::SceneAPI::SceneCore::BehaviorComponent);

            static void Reflect(AZ::ReflectContext* context);

            // BehaviorComponent overrides ...
            void Activate() override;
            void Deactivate() override;

            // ManifestMetaInfoBus::Handler overrides ...
            void GetAvailableModifiers(
                AZ::SceneAPI::Events::ManifestMetaInfo::ModifiersList& modifiers,
                const AZ::SceneAPI::Containers::Scene& scene,
                const AZ::SceneAPI::DataTypes::IManifestObject& target) override;
            void InitializeObject(const AZ::SceneAPI::Containers::Scene& scene, AZ::SceneAPI::DataTypes::IManifestObject& target) override;
            void GetPolicyName(AZStd::string& result) const override
            {
                result = "ClothRuleBehavior";
            }

            // AssetImportRequestBus::Handler overrides ....
            AZ::SceneAPI::Events::ProcessingResult UpdateManifest(AZ::SceneAPI::Containers::Scene& scene, ManifestAction action, RequestingApplication requester) override;

        protected:
            bool IsValidGroupType(const AZ::SceneAPI::DataTypes::ISceneNodeGroup& group) const;

            bool UpdateClothRules(AZ::SceneAPI::Containers::Scene& scene);
            bool UpdateClothRule(const AZ::SceneAPI::Containers::SceneGraph& graph, const AZ::SceneAPI::DataTypes::ISceneNodeGroup& group, ClothRule& clothRule);

            bool ContainsVertexColorStream(const AZ::SceneAPI::Containers::SceneGraph& graph, const AZStd::string& streamName) const;
        };
    } // namespace Pipeline
} // namespace NvCloth
