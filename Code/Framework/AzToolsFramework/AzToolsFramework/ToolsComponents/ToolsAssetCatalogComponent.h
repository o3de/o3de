/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <AzToolsFramework/Asset/AssetProcessorMessages.h>

namespace AssetProcessor
{
    class IToolsAssetCatalog
    {
    public:
        AZ_TYPE_INFO(IToolsAssetCatalog, "{F8BF6237-5BD5-46C9-9589-EA041BA4534C}");

        IToolsAssetCatalog() = default;

        virtual void SetActivePlatform(const AZStd::string& platform) = 0;

        AZ_DISABLE_COPY_MOVE(IToolsAssetCatalog);
    };

    //! Tools replacement for the AssetCatalogComponent
    //! Services the AssetCatalogRequestBus by interfacing with the AssetProcessor over a network connection
    class ToolsAssetCatalogComponent :
        public AZ::Component,
        public AZ::Interface<IToolsAssetCatalog>::Registrar,
        public AZ::Data::AssetCatalogRequestBus::Handler,
        public AZ::Data::AssetCatalog
    {
    public:
        
        AZ_COMPONENT(ToolsAssetCatalogComponent, "{AE68E46B-0E21-499A-8309-41408BCBE4BF}");

        ToolsAssetCatalogComponent() = default;
        ~ToolsAssetCatalogComponent() override = default;

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& services);

        void Activate() override;
        void Deactivate() override;

        // AssetCatalog overrides
        AZ::Data::AssetStreamInfo GetStreamInfoForLoad(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType& assetType) override;
        AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> GetDirectProductDependencies(
            const AZ::Data::AssetId& id) override;
        AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> GetAllProductDependencies(
            const AZ::Data::AssetId& id) override;
        AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> GetLoadBehaviorProductDependencies(
            const AZ::Data::AssetId& id, AZStd::unordered_set<AZ::Data::AssetId>& noloadSet,
            AZ::Data::PreloadAssetListType& preloadAssetList) override;
        ////////////////////////////////////////////////////////////////////////////////

        // AssetCatalogRequestBus overrides
        AZStd::string GetAssetPathById(const AZ::Data::AssetId& id) override;
        AZ::Data::AssetId GetAssetIdByPath(const char* path, const AZ::Data::AssetType& typeToRegister, bool autoRegisterIfNotFound) override;
        AZ::Data::AssetInfo GetAssetInfoById(const AZ::Data::AssetId& id) override;
        void EnableCatalogForAsset(const AZ::Data::AssetType& assetType) override;
        void DisableCatalog() override;
        ////////////////////////////////////////////////////////////////////////////////

        // IToolsAssetCatalog overrides
        void SetActivePlatform(const AZStd::string& platform) override;
        ////////////////////////////////////////////////////////////////////////////////

    protected:
        AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string>  GetProductDependencies(
            const AZ::Data::AssetId& id,
            AzFramework::AssetSystem::AssetDependencyInfoRequest::DependencyType dependencyType,
            AzFramework::AssetSystem::AssetDependencyInfoResponse& response);

        // Keeps track of the currently active platform - the platform for the currently processing ProcessJob request
        AZStd::string m_currentPlatform;
    };
}
