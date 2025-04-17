/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/FileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/ToolsComponents/ToolsAssetCatalogComponent.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/Asset/AssetProcessorMessages.h>
#include <AzFramework/Network/AssetProcessorConnection.h>

namespace AssetProcessor
{
    void ToolsAssetCatalogComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ToolsAssetCatalogComponent, AZ::Component>()
                ->Version(1);
        }
    }

    void ToolsAssetCatalogComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("AssetCatalogService"));
    }

    void ToolsAssetCatalogComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("AssetDatabaseService"));
    }

    void ToolsAssetCatalogComponent::Activate()
    {
        AZ::Data::AssetCatalogRequestBus::Handler::BusConnect();
    }

    void ToolsAssetCatalogComponent::Deactivate()
    {
        AZ::Data::AssetCatalogRequestBus::Handler::BusDisconnect();

        DisableCatalog();
    }

    AZ::Data::AssetStreamInfo ToolsAssetCatalogComponent::GetStreamInfoForLoad(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType& assetType)
    {
        AzFramework::SocketConnection* engineConnection = AzFramework::SocketConnection::GetInstance();
        if (!engineConnection || !engineConnection->IsConnected())
        {
            return {};
        }

        AzFramework::AssetSystem::AssetInfoRequest request(assetId);
        AzFramework::AssetSystem::AssetInfoResponse response;

        request.m_assetType = assetType;
        request.m_platformName = m_currentPlatform;

        if (!SendRequest(request, response))
        {
            AZ_Error("ToolsAssetCatalogComponent", false, "Failed to send GetAssetInfoById request for %s", assetId.ToString<AZStd::string>().c_str());
            return {};
        }

        if (response.m_found)
        {
            AZStd::string fullPath;

            if(response.m_rootFolder.empty())
            {
                fullPath = response.m_assetInfo.m_relativePath;
            }
            else
            {
                AzFramework::StringFunc::Path::Join(response.m_rootFolder.c_str(), response.m_assetInfo.m_relativePath.c_str(), fullPath);
            }

            AZ::Data::AssetStreamInfo streamInfo;
            streamInfo.m_dataLen = response.m_assetInfo.m_sizeBytes;
            streamInfo.m_streamName = fullPath;
            streamInfo.m_streamFlags = AZ::IO::OpenMode::ModeRead;

            return streamInfo;
        }

        return {};
    }

    AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> ToolsAssetCatalogComponent::GetProductDependencies(
        const AZ::Data::AssetId& id,
        AzFramework::AssetSystem::AssetDependencyInfoRequest::DependencyType dependencyType,
        AzFramework::AssetSystem::AssetDependencyInfoResponse& response)
    {
        response.m_found = false;

        AzFramework::SocketConnection* engineConnection = AzFramework::SocketConnection::GetInstance();
        if (!engineConnection || !engineConnection->IsConnected())
        {
            return AZ::Failure(AZStd::string("No tool connection present."));
        }

        AzFramework::AssetSystem::AssetDependencyInfoRequest request(id, dependencyType);

        if (!SendRequest(request, response))
        {
            AZ_Error("ToolsAssetCatalogComponent", false, "Failed to send GetProductDependencies request for %s",
                id.ToString<AZStd::string>().c_str());
        }

        if (response.m_found)
        {
            return AZ::Success(AZStd::move(response.m_dependencies));
        }

        return AZ::Failure(response.m_errorString);
    }


    AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> ToolsAssetCatalogComponent::GetDirectProductDependencies(
        const AZ::Data::AssetId& id)
    {
        AzFramework::AssetSystem::AssetDependencyInfoResponse response;
        return GetProductDependencies(
            id,
            AzFramework::AssetSystem::AssetDependencyInfoRequest::DependencyType::DirectDependencies,
            response);
    }

    AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> ToolsAssetCatalogComponent::GetAllProductDependencies(
        const AZ::Data::AssetId& id)
    {
        AzFramework::AssetSystem::AssetDependencyInfoResponse response;
        return GetProductDependencies(
            id,
            AzFramework::AssetSystem::AssetDependencyInfoRequest::DependencyType::AllDependencies,
            response);
    }

    AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> ToolsAssetCatalogComponent::GetLoadBehaviorProductDependencies(
        const AZ::Data::AssetId& id, AZStd::unordered_set<AZ::Data::AssetId>& noloadSet,
        AZ::Data::PreloadAssetListType& preloadAssetList)
    {
        AzFramework::AssetSystem::AssetDependencyInfoResponse response;
        auto result = GetProductDependencies(
            id,
            AzFramework::AssetSystem::AssetDependencyInfoRequest::DependencyType::LoadBehaviorDependencies,
            response);

        // Copy the NoLoad and PreLoad dependency lists out of the response before returning.
        noloadSet = response.m_noloadSet;
        preloadAssetList = response.m_preloadAssetList;

        return result;
    }


    void ToolsAssetCatalogComponent::EnableCatalogForAsset(const AZ::Data::AssetType& assetType)
    {
        AZ_Assert(AZ::Data::AssetManager::IsReady(), "Asset manager is not ready.");
        AZ::Data::AssetManager::Instance().RegisterCatalog(this, assetType);
    }

    void ToolsAssetCatalogComponent::DisableCatalog()
    {
        if (AZ::Data::AssetManager::IsReady())
        {
            AZ::Data::AssetManager::Instance().UnregisterCatalog(this);
        }
    }

    void ToolsAssetCatalogComponent::SetActivePlatform(const AZStd::string& platform)
    {
        m_currentPlatform = platform;
    }

    AZStd::string ToolsAssetCatalogComponent::GetAssetPathById(const AZ::Data::AssetId& id)
    {
        return GetAssetInfoById(id).m_relativePath;
    }

    AZ::Data::AssetId ToolsAssetCatalogComponent::GetAssetIdByPath(
        const char* path,
        const AZ::Data::AssetType& typeToRegister,
        bool autoRegisterIfNotFound)
    {
        AZ_UNUSED(autoRegisterIfNotFound);
        AZ_Assert(autoRegisterIfNotFound == false, "Auto registration is invalid during asset processing.");
        AZ_UNUSED(typeToRegister);

        AzFramework::SocketConnection* engineConnection = AzFramework::SocketConnection::GetInstance();
        if (!engineConnection || !engineConnection->IsConnected())
        {
            return {};
        }

        AzFramework::AssetSystem::AssetInfoRequest request(path);
        AzFramework::AssetSystem::AssetInfoResponse response;

        request.m_platformName = m_currentPlatform;

        if (!SendRequest(request, response))
        {
            AZ_Error("ToolsAssetCatalogComponent", false, "Failed to send GetAssetIdByPath request for %s", path);
            return {};
        }

        return response.m_assetInfo.m_assetId;
    }

    AZ::Data::AssetInfo ToolsAssetCatalogComponent::GetAssetInfoById(const AZ::Data::AssetId& id)
    {
        AzFramework::SocketConnection* engineConnection = AzFramework::SocketConnection::GetInstance();
        if (!engineConnection || !engineConnection->IsConnected())
        {
            return {};
        }

        AzFramework::AssetSystem::AssetInfoRequest request(id);
        AzFramework::AssetSystem::AssetInfoResponse response;

        request.m_platformName = m_currentPlatform;

        if (!SendRequest(request, response))
        {
            AZ_Error("ToolsAssetCatalogComponent", false, "Failed to send GetAssetInfoById request for %s", id.ToString<AZStd::string>().c_str());
            return {};
        }

        return response.m_assetInfo;

    }
}
