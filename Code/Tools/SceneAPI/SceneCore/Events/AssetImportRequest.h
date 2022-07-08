/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/unordered_set.h>
#include <SceneAPI/SceneCore/SceneCoreConfiguration.h>
#include <SceneAPI/SceneCore/Events/ProcessingResult.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            class Scene;
        }
        namespace Events
        {
            enum class LoadingResult
            {
                Ignored,
                AssetLoaded,
                ManifestLoaded,
                AssetFailure,
                ManifestFailure
            };

            class SCENE_CORE_API LoadingResultCombiner
            {
            public:
                LoadingResultCombiner();
                void operator= (LoadingResult rhs);
                ProcessingResult GetManifestResult() const;
                ProcessingResult GetAssetResult() const;

            private:
                ProcessingResult m_manifestResult;
                ProcessingResult m_assetResult;
            };

            class SCENE_CORE_API AssetImportRequest
                : public AZ::EBusTraits
            {
            public:
                enum RequestingApplication
                {
                    Generic,
                    Editor,
                    AssetProcessor
                };
                enum ManifestAction
                {
                    Update,
                    ConstructDefault
                };

                static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
                using MutexType = AZStd::recursive_mutex;

                static AZ::Crc32 GetAssetImportRequestComponentTag()
                {
                    return AZ_CRC_CE("AssetImportRequest");
                }
                
                virtual ~AssetImportRequest() = 0;

                //! Fills the given list with all available file extensions, excluding the extension for the manifest.
                virtual void GetSupportedFileExtensions(AZStd::unordered_set<AZStd::string>& extensions);
                //! Gets the file extension for the manifest.
                virtual void GetManifestExtension(AZStd::string& result);
                //! Gets the file extension for the generated manifest.
                virtual void GetGeneratedManifestExtension(AZStd::string& result);

                //! Before asset loading starts this is called to allow for any required initialization.
                virtual ProcessingResult PrepareForAssetLoading(Containers::Scene& scene, RequestingApplication requester);
                //! Starts the loading of the asset at the given path in the given scene. Loading optimizations can be applied based on
                //! the calling application.
                virtual LoadingResult LoadAsset(Containers::Scene& scene, const AZStd::string& path, const Uuid& guid, RequestingApplication requester);
                //! FinalizeAssetLoading can be used to do any work to complete loading, such as complete asynchronous loading
                //! or adjust the loaded content in the the SceneGraph. While manifest changes can be done here as well, it's
                //! recommended to wait for the UpdateManifest call.
                virtual void FinalizeAssetLoading(Containers::Scene& scene, RequestingApplication requester);
                //! After all loading has completed, this call can be used to make adjustments to the manifest. Based on the given
                //! action this can mean constructing a new manifest or updating an existing manifest. This call is intended
                //! to deal with any default behavior of the manifest.
                virtual ProcessingResult UpdateManifest(Containers::Scene& scene, ManifestAction action,
                    RequestingApplication requester);

                // Get scene processing project setting: UseCustomNormal 
                virtual void AreCustomNormalsUsed(bool & value);

                /*!
                    Optional method for reporting source file dependencies that may exist in the scene manifest
                    Paths is a vector of JSON Path strings, relative to the IRule object
                    For example, the following path: /scriptFilename
                    Would match with this manifest:
                
                    {
                        "values": [
                            {
                                "$type": "Test",
                                "scriptFilename": "file.py"
                            }
                        ]
                    }
                 */
                virtual void GetManifestDependencyPaths(AZStd::vector<AZStd::string>& paths);

                //! Utility function to load an asset and manifest from file by using the EBus functions above.
                //! @param assetFilePath The absolute path to the source file (not the manifest).
                //! @param sourceGuid The guid assigned to the source file (not the manifest).
                //! @param requester The application making the request to load the file. This can be used to optimize the type and amount of data
                //! to load.
                //! @param loadingComponentUuid The UUID assigned to the loading component.
                static AZStd::shared_ptr<Containers::Scene> LoadSceneFromVerifiedPath(const AZStd::string& assetFilePath,
                    const Uuid& sourceGuid, RequestingApplication requester, const Uuid& loadingComponentUuid);

                //! Utility function to determine if a given file path points to a scene manifest file (.assetinfo).
                //! @param filePath A relative or absolute path to the file to check.
                static bool IsManifestExtension(const char* filePath);
                //! Utility function to determine if a given file path points to a scene file (for instance .fbx).
                //! @param filePath A relative or absolute path to the file to check.
                static bool IsSceneFileExtension(const char* filePath);
            };

            using AssetImportRequestBus = AZ::EBus<AssetImportRequest>;

            inline AssetImportRequest::~AssetImportRequest() = default;
        } // namespace Events
    } // namespace SceneAPI
} // namespace AZ
