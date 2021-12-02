/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Asset/FileTagAsset.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include "FileTagComponent.h"


#include <AzCore/std/string/wildcard.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/XML/rapidxml.h>

namespace AzFramework
{
    namespace FileTag
    {
        void FileTagComponent::Activate()
        {
            m_fileTagManager.reset(aznew FileTagManager());
        }
        void FileTagComponent::Deactivate()
        {
            m_fileTagManager.reset();
        }
        void FileTagComponent::Reflect(AZ::ReflectContext* context)
        {
            // CustomAssetTypeComponent is responsible for reflecting the FileTagAsset class
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<FileTagComponent>()
                    ->Version(1);
            }
        }

        void FileTagQueryComponent::Activate()
        {
            m_excludeFileQueryManager.reset(aznew FileTagQueryManager(FileTagType::Exclude));
            m_includeFileQueryManager.reset(aznew FileTagQueryManager(FileTagType::Include));
        }
        void FileTagQueryComponent::Deactivate()
        {
            m_excludeFileQueryManager.reset();
            m_includeFileQueryManager.reset();
        }

        void FileTagQueryComponent::Reflect(AZ::ReflectContext* context)
        {
            // CustomAssetTypeComponent is responsible for reflecting the FileTagAsset class
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<FileTagQueryComponent>()
                    ->Version(1);
            }
        }

        void ExcludeFileComponent::Activate()
        {
            m_excludeFileQueryManager.reset(aznew FileTagQueryManager(FileTagType::Exclude));
            if (!m_excludeFileQueryManager.get()->Load())
            {
                AZ_Error("FileTagQueryComponent", false, "Not able to load default exclude file (%s). Please make sure that it exists on disk.\n",
                    FileTagQueryManager::GetDefaultFileTagFilePath(FileTagType::Exclude).c_str());
            }

            AzFramework::AssetCatalogEventBus::Handler::BusConnect();
        }

        void ExcludeFileComponent::Deactivate()
        {
            AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
            m_excludeFileQueryManager.reset();
        }

        void ExcludeFileComponent::Reflect(AZ::ReflectContext* context)
        {
            // CustomAssetTypeComponent is responsible for reflecting the FileTagAsset class
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<ExcludeFileComponent, AZ::Component>()
                    ->Version(1);
            }
        }

        void ExcludeFileComponent::OnCatalogLoaded(const char* catalogFile)
        {
            AZ_UNUSED(catalogFile);

            AZStd::vector<AZStd::string> registeredAssetPaths;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(registeredAssetPaths, &AZ::Data::AssetCatalogRequests::GetRegisteredAssetPaths);

            const char* dependencyXmlPattern = "*_dependencies.xml";
            for (const AZStd::string& assetPath : registeredAssetPaths)
            {
                if (!AZStd::wildcard_match(dependencyXmlPattern, assetPath.c_str()))
                {
                    continue;
                }

                if (!m_excludeFileQueryManager.get()->LoadEngineDependencies(assetPath))
                {
                    AZ_Error("ExcludeFileComponent", false, "Failed to add assets referenced from %s to the blocked list", assetPath.c_str());
                }
            }
        }
    }
}
