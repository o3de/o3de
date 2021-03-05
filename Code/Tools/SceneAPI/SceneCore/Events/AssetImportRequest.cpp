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

#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneCore/Components/LoadingComponent.h>
#include <SceneAPI/SceneCore/Components/SceneSystemComponent.h>
#include <SceneAPI/SceneCore/Components/Utilities/EntityConstructor.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>
#include <SceneAPI/SceneCore/Events/SceneSerializationBus.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Events
        {
            //
            // Loading Result Combiner
            //

            LoadingResultCombiner::LoadingResultCombiner()
                : m_manifestResult(ProcessingResult::Ignored)
                , m_assetResult(ProcessingResult::Ignored)
            {
            }

            void LoadingResultCombiner::operator=(LoadingResult rhs)
            {
                switch (rhs)
                {
                case LoadingResult::Ignored:
                    return;
                case LoadingResult::AssetLoaded:
                    m_assetResult = m_assetResult != ProcessingResult::Failure ? ProcessingResult::Success : ProcessingResult::Failure;
                    return;
                case LoadingResult::ManifestLoaded:
                    m_manifestResult = m_manifestResult != ProcessingResult::Failure ? ProcessingResult::Success : ProcessingResult::Failure;
                    return;
                case LoadingResult::AssetFailure:
                    m_assetResult = ProcessingResult::Failure;
                    return;
                case LoadingResult::ManifestFailure:
                    m_manifestResult = ProcessingResult::Failure;
                    return;
                }
            }
            
            ProcessingResult LoadingResultCombiner::GetManifestResult() const
            {
                return m_manifestResult;
            }

            ProcessingResult LoadingResultCombiner::GetAssetResult() const
            {
                return m_assetResult;
            }

            //
            // Asset Importer Request
            //

            void AssetImportRequest::GetManifestExtension(AZStd::string& /*result*/)
            {
            }

            void AssetImportRequest::GetSupportedFileExtensions(AZStd::unordered_set<AZStd::string>& /*extensions*/)
            {
            }

            ProcessingResult AssetImportRequest::PrepareForAssetLoading(Containers::Scene& /*scene*/, RequestingApplication /*requester*/)
            {
                return ProcessingResult::Ignored;
            }

            LoadingResult AssetImportRequest::LoadAsset(Containers::Scene& /*scene*/, const AZStd::string& /*path*/, const Uuid& /*guid*/, 
                RequestingApplication /*requester*/)
            {
                return LoadingResult::Ignored;
            }

            void AssetImportRequest::FinalizeAssetLoading(Containers::Scene& /*scene*/, RequestingApplication /*requester*/)
            {
            }

            ProcessingResult AssetImportRequest::UpdateManifest(Containers::Scene& /*scene*/, ManifestAction /*action*/, RequestingApplication /*requester*/)
            {
                return ProcessingResult::Ignored;
            }

            void AssetImportRequest::AreCustomNormalsUsed(bool &value)
            {
                // Leave the SceneProcessingConfigSystemComponent do the job
                AZ_UNUSED(value);
            }

            AZStd::shared_ptr<Containers::Scene> AssetImportRequest::LoadSceneFromVerifiedPath(const AZStd::string& assetFilePath, const Uuid& sourceGuid,
                RequestingApplication requester)
            {
                AZStd::string sceneName;
                AzFramework::StringFunc::Path::GetFileName(assetFilePath.c_str(), sceneName);
                AZStd::shared_ptr<Containers::Scene> scene = AZStd::make_shared<Containers::Scene>(AZStd::move(sceneName));
                AZ_Assert(scene, "Unable to create new scene for asset importing.");

                // Unique pointer, will deactivate and clean up once going out of scope.
                SceneCore::EntityConstructor::EntityPointer loaders = 
                    SceneCore::EntityConstructor::BuildEntity("Scene Loading", SceneCore::LoadingComponent::TYPEINFO_Uuid());

                ProcessingResultCombiner areAllPrepared;
                AssetImportRequestBus::BroadcastResult(areAllPrepared, &AssetImportRequestBus::Events::PrepareForAssetLoading, *scene, requester);
                if (areAllPrepared.GetResult() == ProcessingResult::Failure)
                {
                    AZ_TracePrintf(Utilities::ErrorWindow, "Not all asset loaders could initialize.\n");
                    return nullptr;
                }

                LoadingResultCombiner filesLoaded;
                AssetImportRequestBus::BroadcastResult(filesLoaded, &AssetImportRequestBus::Events::LoadAsset, *scene, assetFilePath, sourceGuid, requester);
                AssetImportRequestBus::Broadcast(&AssetImportRequestBus::Events::FinalizeAssetLoading, *scene, requester);
                
                if (filesLoaded.GetAssetResult() != ProcessingResult::Success)
                {
                    AZ_TracePrintf(Utilities::ErrorWindow, "Failed to load requested scene file.\n");
                    return nullptr;
                }

                ManifestAction action = ManifestAction::Update;
                // If the result for manifest is ignored it means no manifest was found.
                if (filesLoaded.GetManifestResult() == ProcessingResult::Failure || filesLoaded.GetManifestResult() == ProcessingResult::Ignored)
                {
                    scene->GetManifest().Clear();
                    action = ManifestAction::ConstructDefault;
                }
                ProcessingResultCombiner manifestUpdate;
                AssetImportRequestBus::BroadcastResult(manifestUpdate, &AssetImportRequestBus::Events::UpdateManifest, *scene, action, requester);
                if (manifestUpdate.GetResult() == ProcessingResult::Failure)
                {
                    AZ_TracePrintf(Utilities::ErrorWindow, "Unable to %s manifest.\n", action == ManifestAction::ConstructDefault ? "create new" : "update");
                    return nullptr;
                }

                return scene;
            }

            bool AssetImportRequest::IsManifestExtension(const char* filePath)
            {
                AZStd::string manifestExtension;
                AssetImportRequestBus::Broadcast(&AssetImportRequestBus::Events::GetManifestExtension, manifestExtension);
                AZ_Assert(!manifestExtension.empty(), "Manifest extension was not declared.");
                return AzFramework::StringFunc::Path::IsExtension(filePath, manifestExtension.c_str());
            }

            bool AssetImportRequest::IsSceneFileExtension(const char* filePath)
            {
                AZStd::unordered_set<AZStd::string> extensions;
                AssetImportRequestBus::Broadcast(&AssetImportRequestBus::Events::GetSupportedFileExtensions, extensions);
                AZ_Assert(!extensions.empty(), "No extensions found for source files.");
                
                for (const AZStd::string& extension : extensions)
                {
                    if (AzFramework::StringFunc::Path::IsExtension(filePath, extension.c_str()))
                    {
                        return true;
                    }
                }

                return false;
            }
        } // namespace Events
    } // namespace SceneAPI
} // namespace AZ
