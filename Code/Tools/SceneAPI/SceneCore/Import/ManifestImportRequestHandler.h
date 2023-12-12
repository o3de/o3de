/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <SceneAPI/SceneCore/Components/BehaviorComponent.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Import
        {
            class SCENE_CORE_CLASS ManifestImportRequestHandler
                : public SceneCore::BehaviorComponent
                , public Events::AssetImportRequestBus::Handler
            {
            public:
                AZ_COMPONENT(ManifestImportRequestHandler, "{6CF0520E-D5A9-4003-81A5-F20D62010E6F}", SceneCore::BehaviorComponent);

                SCENE_CORE_API ManifestImportRequestHandler() = default;
                SCENE_CORE_API ~ManifestImportRequestHandler() override = default;

                void Activate() override;
                void Deactivate() override;
                static void Reflect(ReflectContext* context);

                void GetManifestExtension(AZStd::string& result) override;
                void GetGeneratedManifestExtension(AZStd::string& result) override;
                SCENE_CORE_API Events::LoadingResult LoadAsset(
                    Containers::Scene& scene, const AZStd::string& path, const Uuid& guid,
                    RequestingApplication requester) override;
                void GetPolicyName(AZStd::string& result) const override
                {
                    result = "ManifestImportRequestHandler";
                }
                
            private:
                static const char* s_extension;
                static const char* s_generated;
            };
        } // namespace Import
    } // namespace SceneAPI
} // namespace AZ
