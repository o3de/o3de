/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <SceneAPI/SceneCore/Import/ManifestImportRequestHandler.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneManifest.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>

#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzCore/Utils/Utils.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Import
        {
            const char* ManifestImportRequestHandler::s_extension = ".assetinfo";
            const char* ManifestImportRequestHandler::s_generated = ".generated"; // support for foo.fbx.assetinfo.generated

            void ManifestImportRequestHandler::Activate()
            {
                BusConnect();
            }

            void ManifestImportRequestHandler::Deactivate()
            {
                BusDisconnect();
            }

            void ManifestImportRequestHandler::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<ManifestImportRequestHandler, BehaviorComponent>()->Version(1);
                }
            }

            void ManifestImportRequestHandler::GetManifestExtension(AZStd::string& result)
            {
                result = s_extension;
            }

            void ManifestImportRequestHandler::GetGeneratedManifestExtension(AZStd::string& result)
            {
                result = s_extension;
                result.append(s_generated);
            }

            Events::LoadingResult ManifestImportRequestHandler::LoadAsset(Containers::Scene& scene, const AZStd::string& path, 
                                                                          const Uuid& /*guid*/, RequestingApplication /*requester*/)
            {
                AZStd::string manifestPath = path + s_extension;

                scene.SetManifestFilename(manifestPath);
                if (!AZ::IO::FileIOBase::GetInstance()->Exists(manifestPath.c_str()))
                {
                    AZ::SettingsRegistryInterface::FixedValueString assetCacheRoot;
                    AZ::SettingsRegistry::Get()->Get(assetCacheRoot, AZ::SettingsRegistryMergeUtils::FilePathKey_CacheRootFolder);
                    if (assetCacheRoot.empty())
                    {
                        // If there's no cache root folder, the default settings will do.
                        return Events::LoadingResult::Ignored;
                    }

                    // looking for filename in the pattern of sourceFileName.extension.assetinfo.generated in the cache
                    AZStd::string filename = path;
                    AZ::StringFunc::Path::GetFullFileName(filename.c_str(), filename);
                    filename += s_extension;
                    filename += s_generated;

                    AZStd::string altManifestFolder = path;
                    AzFramework::ApplicationRequests::Bus::Broadcast(
                        &AzFramework::ApplicationRequests::Bus::Events::MakePathRelative,
                        altManifestFolder,
                        AZ::Utils::GetProjectPath().c_str());

                    AZ::StringFunc::Path::GetFolderPath(altManifestFolder.c_str(), altManifestFolder);

                    AZStd::string generatedAssetInfoPath;
                    AZ::StringFunc::Path::Join(assetCacheRoot.c_str(), altManifestFolder.c_str(), generatedAssetInfoPath);
                    AZ::StringFunc::Path::ConstructFull(generatedAssetInfoPath.c_str(), filename.c_str(), generatedAssetInfoPath);

                    if (!AZ::IO::FileIOBase::GetInstance()->Exists(generatedAssetInfoPath.c_str()))
                    {
                        // If there's no manifest file, the default settings will do.
                        return Events::LoadingResult::Ignored;
                    }
                    manifestPath = generatedAssetInfoPath;
                    scene.SetManifestFilename(AZStd::move(generatedAssetInfoPath));
                }

                return scene.GetManifest().LoadFromFile(manifestPath) ? Events::LoadingResult::ManifestLoaded : Events::LoadingResult::ManifestFailure;
            }
        } // namespace Import
    } // namespace SceneAPI
} // namespace AZ

