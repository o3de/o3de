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

#include <IConvertor.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/string.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>

namespace AssetBuilderSDK
{
    struct ProcessJobRequest;
    struct ProcessJobResponse;
}
namespace AZ
{
    class Entity;

    namespace SceneAPI
    {
        namespace Containers
        {
            class Scene;
            class SceneManifest;
        }
        namespace Events
        {
            struct ExportProduct;
        }
        namespace Import
        {
            class IImportersList;
        }
    }
    namespace RC
    {
        // Used, among other things, to make sure a valid SerializeContext is available.
        class RCToolApplication : public AzToolsFramework::ToolsApplication
        {
        public:
            RCToolApplication();
            void RegisterDescriptors();

            AZ::ComponentTypeList GetRequiredSystemComponents() const override;

            void SetSettingsRegistrySpecializations(AZ::SettingsRegistryInterface::Specializations& specializations) override;
        };

        class ISceneConfig;
        class SceneCompiler
            : public ICompiler
        {
        public:
            SceneCompiler(const AZStd::shared_ptr<ISceneConfig>& config, const char* appRoot);

            void Release() override;

            void BeginProcessing(const IConfig* config) override;
            bool Process() override;
            void EndProcessing() override;

            IConvertContext* GetConvertContext() override;

        protected:
            bool PrepareForExporting(RCToolApplication& application, const AZStd::string& appRoot, bool& connectedToAssetProcessor);
            bool LoadAndExportScene(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response);

            // @brief Execute runtime modifications to the Scene graph
            //
            // This step is run after the scene is loaded, but before the scene
            // is exported. It emits events with the GenerateEventContext.
            // Event handlers bound to that event can apply arbitrary
            // transformations to the Scene, adding new nodes, replacing nodes,
            // or removing nodes.
            bool GenerateScene(AZ::SceneAPI::Containers::Scene& scene, const char* platformIdentifier);
            bool ExportScene(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response, const AZ::SceneAPI::Containers::Scene& scene, const char* platformIdentifier);
            
            // Several file produced by this compiler used to have their sub id automatically assigned by the AP. This was causing problems with keeping
            //      the sub id stable and the sub id was changed to be provided by this compiler. However these new sub ids differ from the original sub id
            //      so to be compatible with legacy sub ids, the previously automatically created sub id is calculated for all files that used to 
            //      have them. This has to be limited to only the products that would have previously had an automated sub id assigned as some of
            //      the automatically generated sub ids were file order depended.
            virtual bool IsPreSubIdFile(const AZStd::string& file) const;
            virtual void ResolvePreSubIds(AssetBuilderSDK::ProcessJobResponse& response, const AZStd::map<AZStd::string, size_t>& preSubIdFiles) const;
            virtual u32 BuildSubId(const SceneAPI::Events::ExportProduct& product) const;
            
            virtual AZStd::unique_ptr<AssetBuilderSDK::ProcessJobRequest> ReadJobRequest(const char* cacheFolder) const;
            virtual bool WriteResponse(const char* cacheFolder, AssetBuilderSDK::ProcessJobResponse& response, AssetBuilderSDK::ProcessJobResultCode jobResult) const;

            ConvertContext m_context;
            AZStd::shared_ptr<ISceneConfig> m_config;
            AZStd::string m_appRoot;

        private:
        };
    } // namespace RC
} // namespace AZ
