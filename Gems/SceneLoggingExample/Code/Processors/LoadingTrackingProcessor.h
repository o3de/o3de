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

#include <SceneAPI/SceneCore/Components/LoadingComponent.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>

namespace SceneLoggingExample
{
    // The LoadingTrackingProcessor class demonstrates how to listen to EBus events that start  
    // and finalize the loading of scene files (such as .fbx files) and the manifest (.assetinfo  
    // file). It also shows the Call Processor events that can be fired during loading.
    class LoadingTrackingProcessor
        : public AZ::SceneAPI::SceneCore::LoadingComponent
        , public AZ::SceneAPI::Events::AssetImportRequestBus::Handler
    {
    public:
        AZ_COMPONENT(LoadingTrackingProcessor, "{E5E65E21-0BCD-4874-84B8-22E10CCAEE94}", AZ::SceneAPI::SceneCore::LoadingComponent);

        LoadingTrackingProcessor();
        ~LoadingTrackingProcessor() override = default;

        void Activate() override;
        void Deactivate() override;
        static void Reflect(AZ::ReflectContext* context);

        AZ::SceneAPI::Events::ProcessingResult PrepareForAssetLoading(AZ::SceneAPI::Containers::Scene& scene, 
            RequestingApplication requester) override;
        AZ::SceneAPI::Events::LoadingResult LoadAsset(AZ::SceneAPI::Containers::Scene& scene, 
            const AZStd::string& path, const AZ::Uuid& guid, RequestingApplication requester) override;
        void FinalizeAssetLoading(AZ::SceneAPI::Containers::Scene& scene, RequestingApplication requester);
        AZ::SceneAPI::Events::ProcessingResult UpdateManifest(AZ::SceneAPI::Containers::Scene& scene, ManifestAction action,
            RequestingApplication requester) override;

        uint8_t GetPriority() const override;

        AZ::SceneAPI::Events::ProcessingResult ContextCallback(AZ::SceneAPI::Events::ICallContext& context);
    };
} // namespace SceneLoggingExample