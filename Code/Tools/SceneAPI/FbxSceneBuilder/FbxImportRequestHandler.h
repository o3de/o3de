/*
 * Copyright (c) Contributors to the Open 3D Engine Project
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
        namespace FbxSceneImporter
        {
            struct SceneImporterSettings
            {
                AZ_TYPE_INFO(SceneImporterSettings, "{8BB6C7AD-BF99-44DC-9DA1-E7AD3F03DC10}");
                
                static void Reflect(AZ::ReflectContext* context);

                AZStd::unordered_set<AZStd::string> m_supportedFileTypeExtensions;
            };

            class FbxImportRequestHandler
                : public AZ::Component
                , public Events::AssetImportRequestBus::Handler
            {
            public:
                AZ_COMPONENT(FbxImportRequestHandler, "{9F4B189C-0A96-4F44-A5F0-E087FF1561F8}");

                ~FbxImportRequestHandler() override = default;

                void Activate() override;
                void Deactivate() override;
                static void Reflect(ReflectContext* context);

                void GetSupportedFileExtensions(AZStd::unordered_set<AZStd::string>& extensions) override;
                Events::LoadingResult LoadAsset(Containers::Scene& scene, const AZStd::string& path, const Uuid& guid,
                    RequestingApplication requester) override;

                static void GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided);

            private:

                SceneImporterSettings m_settings;

                static constexpr const char* SettingsFilename = "AssetImporterSettings.json";
            };
        } // namespace FbxSceneImporter
    } // namespace SceneAPI
} // namespace AZ
