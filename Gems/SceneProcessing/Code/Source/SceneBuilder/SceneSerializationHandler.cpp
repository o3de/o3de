/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SceneBuilder/SceneSerializationHandler.h>

#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/string/conversions.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneCore/Components/LoadingComponent.h>

namespace SceneBuilder
{
    void SceneSerializationHandler::Activate()
    {
        BusConnect();
    }

    void SceneSerializationHandler::Deactivate()
    {
        BusDisconnect();
    }

    void SceneSerializationHandler::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<SceneSerializationHandler, AZ::Component>()->Version(1)
                ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AssetBuilderSDK::ComponentTags::AssetBuilder }))
            ;
        }
    }

    AZStd::shared_ptr<AZ::SceneAPI::Containers::Scene> SceneSerializationHandler::LoadScene(
        const AZStd::string& filePath,
        AZ::Uuid sceneSourceGuid,
        const AZStd::string& watchFolder
    )
    {
        namespace Utilities = AZ::SceneAPI::Utilities;
        using AZ::SceneAPI::Events::AssetImportRequest;

        AZ_TraceContext("File", filePath);

        if (sceneSourceGuid.IsNull())
        {
            AZ_TracePrintf(Utilities::ErrorWindow, "Invalid source guid for the scene file.");
            return nullptr;
        }

        if (AZ::SceneAPI::Events::AssetImportRequest::IsManifestExtension(filePath.c_str()))
        {
            AZ_TracePrintf(Utilities::ErrorWindow, "Provided path contains the manifest path, not the path to the source file.");
            return nullptr;
        }
        if (!AZ::SceneAPI::Events::AssetImportRequest::IsSceneFileExtension(filePath.c_str()))
        {
            AZ_TracePrintf(Utilities::ErrorWindow, "Provided path doesn't contain an extension supported by the SceneAPI.");
            return nullptr;
        }
        if (AzFramework::StringFunc::Path::IsRelative(filePath.c_str()))
        {
            AZ_TracePrintf(Utilities::ErrorWindow, "Given file path is relative where an absolute path was expected.");
            return nullptr;
        }

        if (!AZ::IO::SystemFile::Exists(filePath.c_str()))
        {
            AZ_TracePrintf(Utilities::ErrorWindow, "No file exists at given source path.");
            return nullptr;
        }

        AZStd::shared_ptr<AZ::SceneAPI::Containers::Scene> scene = AssetImportRequest::LoadSceneFromVerifiedPath(
            filePath, sceneSourceGuid, AssetImportRequest::RequestingApplication::AssetProcessor,
            AZ::SceneAPI::SceneCore::LoadingComponent::TYPEINFO_Uuid(), watchFolder);

        if (!scene)
        {
            AZ_TracePrintf(Utilities::ErrorWindow, "Failed to load the requested scene.");
            return nullptr;
        }

        return scene;
    }
} // namespace SceneBuilder
