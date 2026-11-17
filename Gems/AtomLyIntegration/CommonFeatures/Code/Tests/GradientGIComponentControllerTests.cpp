/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/Asset/AssetManager.h>
#include <Atom/RHI.Reflect/ImageSubresource.h>
#include <Atom/RPI.Reflect/Asset/AssetHandler.h>
#include <Atom/RPI.Reflect/Image/ImageMipChainAssetCreator.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAssetCreator.h>
#include <Atom/Feature/GradientGI/GradientGILogic.h>
#include <GradientGI/GradientGIComponentController.h>

namespace UnitTest
{
    using namespace AZ;
    using namespace AZ::Render;

    class GradientGIFeatureProcessorStub final
        : public GradientGIFeatureProcessorInterface
    {
    public:
        void SetGradientColors(const Color& low, const Color&, const Color&) override
        {
            m_lowColor = low;
        }

        void SetExposure(float exposure) override
        {
            m_exposure = exposure;
        }

        void SetFaceResolution(uint32_t resolution) override
        {
            m_resolution = resolution;
        }

        void SetUpdateMode(UpdateMode mode) override
        {
            m_mode = mode;
        }

        UpdateMode GetUpdateMode() const override
        {
            return m_mode;
        }

        void SetDetailTexture(const Data::Asset<RPI::StreamingImageAsset>& asset) override
        {
            m_detail = asset;
            ++m_detailWrites;
        }

        void SetSpecularTexture(const Data::Asset<RPI::StreamingImageAsset>& asset) override
        {
            m_specular = asset;
            ++m_specularWrites;
        }

        void SetDetailParams(uint8_t, uint8_t, float) override
        {
        }

        void SetSpecularParams(uint8_t, uint8_t, float) override
        {
        }

        void Enable(EntityId owner) override
        {
            m_owners.Add(owner);
            m_ownerChanged.Signal(owner);
        }

        void Disable(EntityId owner) override
        {
            const EntityId previous = m_owners.Current();
            m_owners.Remove(owner);

            if (m_owners.Current() != previous && !m_owners.Empty())
            {
                m_ownerChanged.Signal(m_owners.Current());
            }
        }

        void ConnectToOwnerChangedEvent(OwnerChangedEvent::Handler& handler) override
        {
            handler.Connect(m_ownerChanged);
        }

        EntityId GetCurrentOwner() const override
        {
            return m_owners.Current();
        }

        GradientGI::OwnerRegistry m_owners;
        OwnerChangedEvent m_ownerChanged;
        UpdateMode m_mode = UpdateMode::Static;
        Color m_lowColor = Color::CreateZero();
        float m_exposure = 0.0f;
        uint32_t m_resolution = 0;
        Data::Asset<RPI::StreamingImageAsset> m_detail;
        Data::Asset<RPI::StreamingImageAsset> m_specular;
        uint32_t m_detailWrites = 0;
        uint32_t m_specularWrites = 0;
    };

    class GradientGIComponentControllerFixture
        : public ::testing::Test
    {
    protected:
        void Attach(GradientGIComponentController& controller, EntityId id)
        {
            controller.m_entityId = id;
            controller.m_featureProcessor = &m_processor;

            m_processor.ConnectToOwnerChangedEvent(controller.m_ownerChangedHandler);
            m_processor.Enable(id);
        }

        void Tick()
        {
            TickBus::Broadcast(&TickBus::Events::OnTick, 0.0f, ScriptTimePoint{});
        }

        void SetUp() override
        {
            Data::AssetManager::Instance().RegisterHandler(&m_imageHandler, azrtti_typeid<RPI::StreamingImageAsset>());
            Data::AssetManager::Instance().RegisterHandler(&m_mipHandler, azrtti_typeid<RPI::ImageMipChainAsset>());

            Attach(m_controller, EntityId(1));
            Tick();
        }

        void TearDown() override
        {
            m_controller.Deactivate();
        }

        Data::Asset<RPI::StreamingImageAsset> CreateReadyImage()
        {
            const uint32_t pixel = 0xFFFFFFFFu;
            const auto layout = RHI::GetImageSubresourceLayout(RHI::Size(1, 1, 1), RHI::Format::R8G8B8A8_UNORM);

            RPI::ImageMipChainAssetCreator mipCreator;
            mipCreator.Begin(Data::AssetId(Uuid::CreateRandom()), 1, 1);
            mipCreator.BeginMip(layout);
            mipCreator.AddSubImage(&pixel, sizeof(pixel));
            mipCreator.EndMip();

            Data::Asset<RPI::ImageMipChainAsset> mip;
            EXPECT_TRUE(mipCreator.End(mip));

            RPI::StreamingImageAssetCreator creator;
            creator.Begin(Data::AssetId(Uuid::CreateRandom()));
            creator.SetImageDescriptor(RHI::ImageDescriptor::Create2D(
                RHI::ImageBindFlags::ShaderRead,
                1,
                1,
                RHI::Format::R8G8B8A8_UNORM));
            creator.AddMipChainAsset(*mip);

            Data::Asset<RPI::StreamingImageAsset> image;
            EXPECT_TRUE(creator.End(image));

            return image;
        }

        void CheckSharedSubscription(bool replaceSpecular)
        {
            const auto shared = CreateReadyImage();
            m_controller.m_configuration.m_detailTexture = shared;
            m_controller.m_configuration.m_specularTexture = shared;
            m_controller.Data::AssetBus::MultiHandler::BusConnect(shared.GetId());

            if (replaceSpecular)
            {
                m_controller.SetSpecularTexture({});
            }
            else
            {
                m_controller.SetDetailTexture({});
            }

            EXPECT_TRUE(m_controller.Data::AssetBus::MultiHandler::BusIsConnectedId(shared.GetId()));

            Data::AssetBus::Event(shared.GetId(), &Data::AssetBus::Events::OnAssetReloaded, shared);
            Tick();

            if (replaceSpecular)
            {
                EXPECT_EQ(m_processor.m_detail.GetId(), shared.GetId());
                EXPECT_FALSE(m_processor.m_specular.GetId().IsValid());
            }
            else
            {
                EXPECT_EQ(m_processor.m_specular.GetId(), shared.GetId());
                EXPECT_FALSE(m_processor.m_detail.GetId().IsValid());
            }
        }

        void ReadyThenReplaceBeforeTick(bool specular)
        {
            // Bypass setter cancellation to exercise the readiness guard without disk I/O.
            const auto first = CreateReadyImage();
            ASSERT_TRUE(first.IsReady());

            Data::Asset<RPI::StreamingImageAsset> pending(
                Data::AssetId(Uuid::CreateRandom()),
                aznew RPI::StreamingImageAsset,
                Data::AssetLoadBehavior::NoLoad);

            if (specular)
            {
                m_controller.m_configuration.m_specularTexture = first;
                m_controller.OnAssetReady(first);
                m_controller.m_configuration.m_specularTexture = pending;
            }
            else
            {
                m_controller.m_configuration.m_detailTexture = first;
                m_controller.OnAssetReady(first);
                m_controller.m_configuration.m_detailTexture = pending;
            }

            Tick();
            EXPECT_FALSE(m_processor.m_detail.GetId().IsValid());
            EXPECT_FALSE(m_processor.m_specular.GetId().IsValid());
        }

        void SetMode(GradientGIUpdateMode mode)
        {
            m_controller.SetUpdateMode(mode);
        }

        void SetResolution(float resolution)
        {
            m_controller.SetFaceResolution(resolution);
        }

        void SetExposure(float exposure)
        {
            m_controller.SetExposure(exposure);
        }

        void ArmPendingUpdates()
        {
            m_controller.m_detailTextureNeedsUpdate = true;
            m_controller.m_specularTextureNeedsUpdate = true;
            m_controller.TickBus::Handler::BusConnect();
        }

        RPI::AssetHandler<RPI::StreamingImageAsset> m_imageHandler;
        RPI::AssetHandler<RPI::ImageMipChainAsset> m_mipHandler;
        GradientGIFeatureProcessorStub m_processor;
        GradientGIComponentController m_controller;
    };

    TEST_F(GradientGIComponentControllerFixture, DetailPendingReplacementIsNeverForwardedToGpu)
    {
        ReadyThenReplaceBeforeTick(false);
    }

    TEST_F(GradientGIComponentControllerFixture, SpecularPendingReplacementIsNeverForwardedToGpu)
    {
        ReadyThenReplaceBeforeTick(true);
    }

    TEST_F(GradientGIComponentControllerFixture, ChangingDetailPreservesSharedSpecularSubscription)
    {
        CheckSharedSubscription(false);
    }

    TEST_F(GradientGIComponentControllerFixture, ChangingSpecularPreservesSharedDetailSubscription)
    {
        CheckSharedSubscription(true);
    }

    TEST_F(GradientGIComponentControllerFixture, DeactivateCancelsPendingTextureWrites)
    {
        ArmPendingUpdates();
        const uint32_t detailWrites = m_processor.m_detailWrites;
        const uint32_t specularWrites = m_processor.m_specularWrites;

        m_controller.Deactivate();
        Tick();

        EXPECT_EQ(m_processor.m_detailWrites, detailWrites);
        EXPECT_EQ(m_processor.m_specularWrites, specularWrites);
    }

    TEST_F(GradientGIComponentControllerFixture, ModeRoundTripForwardsSnappedResolution)
    {
        SetMode(GradientGIUpdateMode::Dynamic);
        SetResolution(91.0f);

        EXPECT_EQ(m_processor.m_resolution, 91u);

        SetMode(GradientGIUpdateMode::Static);

        EXPECT_EQ(m_controller.GetConfiguration().m_faceResolution, 64u);
        EXPECT_EQ(m_processor.m_resolution, 64u);

        SetMode(GradientGIUpdateMode::Dynamic);

        EXPECT_EQ(m_processor.m_resolution, 64u);
    }

    TEST_F(GradientGIComponentControllerFixture, RemovingNewestOwnerRestoresSurvivingConfiguration)
    {
        SetExposure(2.0f);

        GradientGIComponentConfig otherConfig;
        otherConfig.m_exposure = -3.0f;

        GradientGIComponentController other(otherConfig);
        Attach(other, EntityId(2));

        EXPECT_EQ(m_processor.m_exposure, -3.0f);

        SetExposure(4.0f);

        EXPECT_EQ(m_processor.m_exposure, -3.0f);

        other.Deactivate();

        EXPECT_EQ(m_processor.m_exposure, 4.0f);
        EXPECT_EQ(m_processor.GetCurrentOwner(), EntityId(1));
    }

    TEST_F(GradientGIComponentControllerFixture, ConfigurationReplacementAppliesOnlyForCurrentOwner)
    {
        GradientGIComponentController other;
        Attach(other, EntityId(2));

        GradientGIComponentConfig config;
        config.m_exposure = 5.0f;
        m_controller.SetConfiguration(config);

        EXPECT_NE(m_processor.m_exposure, 5.0f);

        other.Deactivate();

        EXPECT_EQ(m_processor.m_exposure, 5.0f);

        config.m_exposure = 7.0f;
        m_controller.SetConfiguration(config);

        EXPECT_EQ(m_processor.m_exposure, 7.0f);
    }
} // namespace UnitTest
