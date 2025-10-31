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

namespace AZ::SceneAPI::Behaviors
{
    class ImportGroup 
        : public SceneCore::BehaviorComponent
        , public Events::ManifestMetaInfoBus::Handler
        , public Events::AssetImportRequestBus::Handler
    {
    public:
        AZ_COMPONENT(ImportGroup, "{209DF1FB-449F-403A-A468-32A775289AF8}", SceneCore::BehaviorComponent);

        ~ImportGroup() override = default;

        void Activate() override;
        void Deactivate() override;
        static void Reflect(ReflectContext* context);

        void GetCategoryAssignments(CategoryRegistrationList& categories, const Containers::Scene& scene) override;
        void InitializeObject(const Containers::Scene& scene, DataTypes::IManifestObject& target) override;
        Events::ProcessingResult UpdateManifest(Containers::Scene& scene, ManifestAction action,
            RequestingApplication requester) override;
        void GetPolicyName(AZStd::string& result) const override
        {
            result = "SceneAPI::ImportGroup";
        }

    private:
        static const int s_importGroupPreferredTabOrder;
    };
} // namespace AZ::SceneAPI::Behaviors
