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
            class IGroup;
            class ISceneNodeSelectionList;
        }
        namespace SceneData
        {
            class BlendShapeRuleBehavior 
                : public SceneCore::BehaviorComponent
                , public Events::ManifestMetaInfoBus::Handler
                , public Events::AssetImportRequestBus::Handler
            {
            public:
                AZ_COMPONENT(BlendShapeRuleBehavior, "{D07DABE6-D731-4F4F-B55E-019EDE5B435E}", SceneCore::BehaviorComponent);

                ~BlendShapeRuleBehavior() override = default;

                void Activate() override;
                void Deactivate() override;
                static void Reflect(ReflectContext* context);

                void InitializeObject(const Containers::Scene& scene, DataTypes::IManifestObject& target) override;
                Events::ProcessingResult UpdateManifest(Containers::Scene& scene, ManifestAction action,
                    RequestingApplication requester) override;
                void GetPolicyName(AZStd::string& result) const override
                {
                    result = "BlendShapeRuleBehavior";
                }
            private:
                size_t SelectBlendShapes(const Containers::Scene& scene, DataTypes::ISceneNodeSelectionList& selection) const;
                void UpdateBlendShapeRules(Containers::Scene& scene) const;
            };
        } // namespace SceneData
    } // namespace SceneAPI
} // namespace AZ
