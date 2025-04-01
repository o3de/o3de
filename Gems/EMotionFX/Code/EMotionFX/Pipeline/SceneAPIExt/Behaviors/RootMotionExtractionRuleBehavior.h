/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory.h>
#include <SceneAPI/SceneCore/Events/ManifestMetaInfoBus.h>
#include <SceneAPI/SceneCore/Components/BehaviorComponent.h>


namespace EMotionFX::Pipeline::Behavior
{
    class RootMotionExtractionRuleBehavior
        : public AZ::SceneAPI::SceneCore::BehaviorComponent
        , public AZ::SceneAPI::Events::ManifestMetaInfoBus::Handler
    {
    public:
        AZ_COMPONENT(RootMotionExtractionRuleBehavior, "{31427D74-8CC7-46F3-B419-9CADDDD468DD}", AZ::SceneAPI::SceneCore::BehaviorComponent);

        ~RootMotionExtractionRuleBehavior() override = default;

        // BehaviorComponent override...
        void Activate() override;
        void Deactivate() override;
        static void Reflect(AZ::ReflectContext* context);

        // ManifestMetaInfo override...
        void InitializeObject(const AZ::SceneAPI::Containers::Scene& scene, AZ::SceneAPI::DataTypes::IManifestObject& target) override;
    };
} // namespace EMotionFX::Pipeline::Behavior
