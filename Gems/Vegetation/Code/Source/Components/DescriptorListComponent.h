/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Component.h>
#include <LmbrCentral/Dependency/DependencyMonitor.h>
#include <SurfaceData/SurfaceDataTagEnumeratorRequestBus.h>
#include <Vegetation/DescriptorListAsset.h>
#include <Vegetation/Ebuses/DescriptorListRequestBus.h>
#include <Vegetation/Ebuses/DescriptorProviderRequestBus.h>
#include <Vegetation/Ebuses/DescriptorNotificationBus.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace Vegetation
{
    class DescriptorListConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(DescriptorListConfig, AZ::SystemAllocator);
        AZ_RTTI(DescriptorListConfig, "{902F6253-A8FA-4350-B9F1-C176F3E2D305}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);

        DescriptorListSourceType m_sourceType = DescriptorListSourceType::EMBEDDED;
        DescriptorListSourceType GetDescriptorListSourceType() const;
        void SetDescriptorListSourceType(DescriptorListSourceType sourceType);
        bool IsExternalSource() const { return m_sourceType == DescriptorListSourceType::EXTERNAL; }
        bool IsEmbeddedSource() const { return m_sourceType == DescriptorListSourceType::EMBEDDED; }

        AZ::Data::Asset<DescriptorListAsset> m_descriptorListAsset;
        AZStd::string GetDescriptorAssetPath() const;
        void SetDescriptorAssetPath(const AZStd::string& assetPath);
        AZStd::vector<Descriptor> m_descriptors;
        size_t GetNumDescriptors() const;
        Descriptor* GetDescriptor(int index);
        void RemoveDescriptor(int index);
        void SetDescriptor(int index, Descriptor* descriptor);
        void AddDescriptor(Descriptor* descriptor);
    };

    inline constexpr AZ::TypeId  DescriptorListComponentTypeId{ "{8427ED43-9B1F-497F-A356-0FD9AADD2FDB}" };

    /**
    * Default placement logic for vegetation in an area
    */
    class DescriptorListComponent
        : public AZ::Component
        , private AZ::Data::AssetBus::MultiHandler
        , private DescriptorListRequestBus::Handler
        , private DescriptorProviderRequestBus::Handler
        , private SurfaceData::SurfaceDataTagEnumeratorRequestBus::Handler
        , private DescriptorNotificationBus::MultiHandler
    {
    public:
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(DescriptorListComponent, DescriptorListComponentTypeId);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        DescriptorListComponent(const DescriptorListConfig& configuration);
        DescriptorListComponent() = default;
        ~DescriptorListComponent() = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        //////////////////////////////////////////////////////////////////////////
        // DescriptorProviderRequestBus
        void GetDescriptors(DescriptorPtrVec& descriptors) const override;

        //////////////////////////////////////////////////////////////////////////
        // SurfaceData::SurfaceDataTagEnumeratorRequestBus
        void GetInclusionSurfaceTags(SurfaceData::SurfaceTagVector& tags, bool& includeAll) const override;
        void GetExclusionSurfaceTags(SurfaceData::SurfaceTagVector& tags) const override;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Data::AssetBus::Handler
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

    private:
        //////////////////////////////////////////////////////////////////////////
        // DescriptorListRequestBus
        DescriptorListSourceType GetDescriptorListSourceType() const override;
        void SetDescriptorListSourceType(DescriptorListSourceType sourceType) override;

        //////////////////////////////////////////////////////////////////////////
        // DescriptorNotificationBus
        void OnDescriptorAssetsLoaded() override;
        void OnDescriptorAssetsUnloaded() override;


        AZStd::string GetDescriptorAssetPath() const override;
        void SetDescriptorAssetPath(const AZStd::string& assetPath) override;

        size_t GetNumDescriptors() const override;
        Descriptor* GetDescriptor(int index) override;
        void RemoveDescriptor(int index) override;
        void SetDescriptor(int index, Descriptor* descriptor) override;
        void AddDescriptor(Descriptor* descriptor) override;

        void LoadAssets(AZStd::vector<Descriptor>& descriptors);
        void LoadAssets();
        void LoadAssetsFromDescriptorList();

        void RegisterUniqueDescriptors(AZStd::vector<Descriptor>& descriptors);
        void ProcessDescriptorLoadingStatus();

        void ReleaseUniqueDescriptors();

        bool IsFullyLoaded(const AZStd::vector<Descriptor>& descriptors) const;
        bool IsFullyLoaded() const;

        DescriptorListConfig m_configuration;
        DescriptorPtrVec m_uniqueDescriptors;
        mutable AZStd::recursive_mutex uniqueDescriptorsMutex;
    };
}
