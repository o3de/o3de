/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/Profiler.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>
#include <SceneAPI/SceneCore/Components/LoadingComponent.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneSerializationHandler.h>

namespace AZ
{
    SceneSerializationHandler::~SceneSerializationHandler()
    {
        Deactivate();
    }

    void SceneSerializationHandler::Activate()
    {
        BusConnect();
    }

    void SceneSerializationHandler::Deactivate()
    {
        BusDisconnect();
    }

    AZStd::shared_ptr<SceneAPI::Containers::Scene> SceneSerializationHandler::LoadScene(
        const AZStd::string& filePath, Uuid sceneSourceGuid)
    {
        AZ_PROFILE_FUNCTION(Editor);
        namespace Utilities = AZ::SceneAPI::Utilities;
        using AZ::SceneAPI::Events::AssetImportRequest;

        CleanSceneMap();

        AZ_TraceContext("File", filePath);
        if (!IsValidExtension(filePath))
        {
            return nullptr;
        }

        AZ::IO::Path enginePath;
        if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
        {
            settingsRegistry->Get(enginePath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
        }

        AZ::IO::Path cleanPath = (enginePath / filePath).LexicallyNormal();

        auto sceneIt = m_scenes.find(cleanPath.Native());
        if (sceneIt != m_scenes.end())
        {
            AZStd::shared_ptr<SceneAPI::Containers::Scene> scene = sceneIt->second.lock();
            if (scene)
            {
                return scene;
            }
            // There's a small window in between which the scene was closed after searching for
            //      it in the scene map. In this case continue and simply reload the scene.
        }

        if (!AZ::IO::SystemFile::Exists(cleanPath.c_str()))
        {
            AZ_TracePrintf(Utilities::ErrorWindow, "No file exists at given source path.");
            return nullptr;
        }

        if (sceneSourceGuid.IsNull())
        {
            bool result = false;
            AZ::Data::AssetInfo info;
            AZStd::string watchFolder;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(result, &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath, cleanPath.c_str(), info, watchFolder);
            if (!result)
            {
                AZ_TracePrintf(Utilities::ErrorWindow, "Failed to retrieve file info needed to determine the uuid of the source file.");
                return nullptr;
            }
            sceneSourceGuid = info.m_assetId.m_guid;
        }

        AZStd::shared_ptr<SceneAPI::Containers::Scene> scene = 
            AssetImportRequest::LoadSceneFromVerifiedPath(cleanPath.Native(), sceneSourceGuid, AssetImportRequest::RequestingApplication::Editor, SceneAPI::SceneCore::LoadingComponent::TYPEINFO_Uuid());
        if (!scene)
        {
            AZ_TracePrintf(Utilities::ErrorWindow, "Failed to load the requested scene.");
            return nullptr;
        }

        m_scenes.emplace(AZStd::move(cleanPath.Native()), scene);
        
        return scene;
    }

    bool SceneSerializationHandler::IsValidExtension(const AZStd::string& filePath) const
    {
        namespace Utilities = AZ::SceneAPI::Utilities;

        if (AZ::SceneAPI::Events::AssetImportRequest::IsManifestExtension(filePath.c_str()))
        {
            AZ_TracePrintf(Utilities::ErrorWindow, "Provided path contains the manifest path, not the path to the source file.");
            return false;
        }

        if (!AZ::SceneAPI::Events::AssetImportRequest::IsSceneFileExtension(filePath.c_str()))
        {
            AZ_TracePrintf(Utilities::ErrorWindow, "Provided path doesn't contain an extension supported by the SceneAPI.");
            return false;
        }

        return true;
    }

    void SceneSerializationHandler::CleanSceneMap()
    {
        for (auto it = m_scenes.begin(); it != m_scenes.end(); )
        {
            if (it->second.expired())
            {
                it = m_scenes.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
} // namespace AZ
