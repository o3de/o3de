/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Component.h>
#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <GradientSignal/Ebuses/ImageGradientRequestBus.h>
#include <GradientSignal/ImageAsset.h>
#include <GradientSignal/Util.h>
#include <LmbrCentral/Dependency/DependencyMonitor.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace GradientSignal
{
    class ImageGradientConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(ImageGradientConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(ImageGradientConfig, "{1BDB5DA4-A4A8-452B-BE6D-6BD451D4E7CD}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);
        AZ::Data::Asset<ImageAsset> m_imageAsset = { AZ::Data::AssetLoadBehavior::QueueLoad };
        float m_tilingX = 1.0f;
        float m_tilingY = 1.0f;
    };

    static const AZ::Uuid ImageGradientComponentTypeId = "{4741F079-157F-457E-93E0-D6BA4EAF76FE}";

    /**
    * calculates a gradient value based on image data
    */
    class ImageGradientComponent
        : public AZ::Component
        , private AZ::Data::AssetBus::Handler
        , private GradientRequestBus::Handler
        , private ImageGradientRequestBus::Handler
    {
    public:
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(ImageGradientComponent, ImageGradientComponentTypeId);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        ImageGradientComponent(const ImageGradientConfig& configuration);
        ImageGradientComponent() = default;
        ~ImageGradientComponent() = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        //////////////////////////////////////////////////////////////////////////
        // GradientRequestBus
        float GetValue(const GradientSampleParams& sampleParams) const override;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Data::AssetBus::Handler
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetMoved(AZ::Data::Asset<AZ::Data::AssetData> asset, void* oldDataPointer) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

    protected:

        void SetupDependencies();

        //////////////////////////////////////////////////////////////////////////
        // ImageGradientRequestBus
        AZStd::string GetImageAssetPath() const override;
        void SetImageAssetPath(const AZStd::string& assetPath) override;

        float GetTilingX() const override;
        void SetTilingX(float tilingX) override;

        float GetTilingY() const override;
        void SetTilingY(float tilingY) override;

    private:
        ImageGradientConfig m_configuration;
        LmbrCentral::DependencyMonitor m_dependencyMonitor;
        mutable AZStd::recursive_mutex m_imageMutex;
    };
}
