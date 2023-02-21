/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <SplashScreen/SplashScreenPass.h>

namespace AZ::Render
{
    AZ::RPI::Ptr<SplashScreenPass> SplashScreenPass::Create(const AZ::RPI::PassDescriptor& descriptor)
    {
        AZ::RPI::Ptr<SplashScreenPass> pass = aznew SplashScreenPass(descriptor);
        return pass;
    }

    SplashScreenPass::SplashScreenPass(const AZ::RPI::PassDescriptor& descriptor)
        : AZ::RPI::FullscreenTrianglePass(descriptor)
    {
        SetEnabled(false);
    }

    SplashScreenPass::~SplashScreenPass()
    {
        Clear();
    }

    void SplashScreenPass::SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph)
    {
        AZ::RPI::RenderPass::SetupFrameGraphDependencies(frameGraph);
    }

    void SplashScreenPass::InitializeInternal()
    {
        AZ::RPI::FullscreenTrianglePass::InitializeInternal();

        AZ::ApplicationTypeQuery appType;
        AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::QueryApplicationType, appType);
        if (appType.IsValid() && appType.IsEditor())
        {
            return;
        }

        auto settingsRegistry = AZ::SettingsRegistry::Get();
        static const AZStd::string setregPath = "/O3DE/Atom/Feature/SplashScreen";

        if (!(settingsRegistry && settingsRegistry->GetObject(&m_settings, azrtti_typeid(m_settings), setregPath.c_str())))
        {
            return;
        }

        m_splashScreenImage = AZ::RPI::LoadStreamingTexture(m_settings.m_imagePath);
        m_screenTime = m_settings.m_lastingTime;

        if (!m_splashScreenImage)
        {
            return;
        }

        m_splashScreenImageIndex.Reset();
        m_splashScreenImageIndex.Reset();

        AZ::TickBus::Handler::BusConnect();

        SetEnabled(true);
    }

    void SplashScreenPass::Clear()
    {
        m_splashScreenImage = nullptr;

        m_splashScreenImageIndex.Reset();
        m_splashScreenImageIndex.Reset();
    }

    void SplashScreenPass::FrameEndInternal()
    {
        FullscreenTrianglePass::FrameEndInternal();

        if (m_screenTime < 0.0f)
        {
            SetEnabled(false);
        }
    }

    void SplashScreenPass::CompileResources(const AZ::RHI::FrameGraphCompileContext& context)
    {
        m_shaderResourceGroup->SetImage(m_splashScreenImageIndex, m_splashScreenImage);
        m_shaderResourceGroup->SetConstant(m_splashScreenParamsIndex, m_splashScreenParams);

        AZ::RPI::FullscreenTrianglePass::CompileResources(context);
    }

    void SplashScreenPass::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        if (m_beginTimer)
        {
            m_screenTime -= deltaTime;
        }

        if (m_settings.m_fading)
        {
            m_splashScreenParams.m_fadingFactor = m_screenTime < 0.0f ? 0.0f : m_screenTime / 10.0f;
        }
        else
        {
            m_splashScreenParams.m_fadingFactor = 1.0f;
        }

        m_beginTimer = true;

        if (m_screenTime < 0.0f)
        {
            AZ::TickBus::Handler::BusDisconnect();
        }
    }
} // namespace AZ::Render
