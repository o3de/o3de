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
#include <AzCore/Time/TimeSystem.h>
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
    }

    SplashScreenPass::~SplashScreenPass()
    {
        Clear();
    }

    void SplashScreenPass::InitializeInternal()
    {
        AZ::RPI::FullscreenTrianglePass::InitializeInternal();

        auto settingsRegistry = AZ::SettingsRegistry::Get();
        static const AZStd::string setregPath = "/O3DE/Atom/Feature/SplashScreen";

        if (!(settingsRegistry && settingsRegistry->GetObject(&m_settings, azrtti_typeid(m_settings), setregPath.c_str())))
        {
            return;
        }

        m_splashScreenImage = AZ::RPI::LoadStreamingTexture(m_settings.m_imagePath);
        m_durationSeconds = m_settings.m_durationSeconds;

        if (!m_splashScreenImage)
        {
            // Could not find an image based on the setreg path provided
            AZ_Error("SplashScreen", false, "Image path '%s' not found. Please update the /O3DE/Atom/Feature/SplashScreen/ImagePath setreg to a valid asset cache image file.", m_settings.m_imagePath.c_str());
            SetEnabled(false);
            return;
        }

        m_splashScreenImageIndex.Reset();
        m_splashScreenParamsIndex.Reset();

        const TimeUs currentTimeUs = static_cast<TimeUs>(AZStd::GetTimeNowMicroSecond());
        m_lastRealTimeStamp = aznumeric_cast<float>(currentTimeUs) / 1000000.0f;

        AZ::TickBus::Handler::BusConnect();
    }

    void SplashScreenPass::Clear()
    {
        m_splashScreenImage = nullptr;

        m_splashScreenImageIndex.Reset();
        m_splashScreenParamsIndex.Reset();
    }

    void SplashScreenPass::FrameBeginInternal([[maybe_unused]] FramePrepareParams params)
    {
        FullscreenTrianglePass::FrameBeginInternal(params);
    }

    void SplashScreenPass::FrameEndInternal()
    {
        FullscreenTrianglePass::FrameEndInternal();

        if (m_durationSeconds < 0.0f)
        {
            m_beginTimer = false;

            // Disable the pass after life time passes.
            SetEnabled(false);
        }
    }

    void SplashScreenPass::CompileResources(const AZ::RHI::FrameGraphCompileContext& context)
    {
        m_shaderResourceGroup->SetImage(m_splashScreenImageIndex, m_splashScreenImage);
        m_shaderResourceGroup->SetConstant(m_splashScreenParamsIndex, m_splashScreenParams);

        AZ::RPI::FullscreenTrianglePass::CompileResources(context);
    }

    void SplashScreenPass::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        // Not using the delta time from Tick bus because it could be scaled.
        const TimeUs currentTimeUs = static_cast<TimeUs>(AZStd::GetTimeNowMicroSecond());
        float currentRealTimeStamp = aznumeric_cast<float>(currentTimeUs) / 1000000.0f;
        float realDeltaTime = currentRealTimeStamp - m_lastRealTimeStamp;
        m_lastRealTimeStamp = currentRealTimeStamp;

        if (m_beginTimer)
        {
            m_durationSeconds -= realDeltaTime;
        }

        if (m_settings.m_fading)
        {
            float leftTimeRatio = m_durationSeconds / m_settings.m_durationSeconds;

            m_splashScreenParams.m_fadingFactor = m_durationSeconds < 0.0f ? 0.0f : leftTimeRatio * leftTimeRatio * leftTimeRatio;
        }
        else
        {
            m_splashScreenParams.m_fadingFactor = 1.0f;
        }

        // Skipping the first frame for engine initialization time.
        m_beginTimer = true;

        if (m_durationSeconds < 0.0f)
        {
            AZ::TickBus::Handler::BusDisconnect();
        }
    }
} // namespace AZ::Render
