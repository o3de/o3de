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

namespace SceneLoggingExample
{
    // The LoggingGroupBehavior shows how a behavior can be written that monitors
    // manifest activity and reacts to it in order to setup default values for 
    // manifest entries. It also demonstrates how to register new UI elements with 
    // the SceneAPI.
    class LoggingGroupBehavior
        : public AZ::SceneAPI::SceneCore::BehaviorComponent
        , public AZ::SceneAPI::Events::ManifestMetaInfoBus::Handler
        , public AZ::SceneAPI::Events::AssetImportRequestBus::Handler
    {
    public:
        AZ_COMPONENT(LoggingGroupBehavior, "{4DE18DD7-5C40-4A14-8CD7-67162171DCAA}", AZ::SceneAPI::SceneCore::BehaviorComponent);

        ~LoggingGroupBehavior() override = default;

        void Activate() override;
        void Deactivate() override;
        static void Reflect(AZ::ReflectContext* context);

        void GetCategoryAssignments(CategoryRegistrationList& categories, const AZ::SceneAPI::Containers::Scene& scene) override;
        AZ::SceneAPI::Events::ProcessingResult UpdateManifest(AZ::SceneAPI::Containers::Scene& scene, 
            ManifestAction action, RequestingApplication requester) override;
        void InitializeObject(const AZ::SceneAPI::Containers::Scene& scene, AZ::SceneAPI::DataTypes::IManifestObject& target) override;
        void GetPolicyName(AZStd::string& result) const override
        {
            result = "LoggingGroupBehavior";
        }

    private:
        static constexpr int s_loggingPreferredTabOrder{ 10 };
    };
} // namespace SceneLoggingExample
