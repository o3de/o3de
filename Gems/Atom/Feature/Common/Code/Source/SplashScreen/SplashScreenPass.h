/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>

#include <AzCore/Component/TickBus.h>
#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <Atom/RPI.Reflect/Pass/PassDescriptor.h>
#include <Atom/Feature/SplashScreen/SplashScreenSettings.h>

namespace AZ::Render
{
    class SplashScreenPass
        : public AZ::RPI::FullscreenTrianglePass
        , public AZ::TickBus::Handler
    {
        AZ_RPI_PASS(SplashScreenPass);

    public:
        AZ_RTTI(AZ::Render::SplashScreenPass, "{B12F4E30-94ED-4F69-A17D-85C65853ACD9}", AZ::RPI::FullscreenTrianglePass);
        AZ_CLASS_ALLOCATOR(AZ::Render::SplashScreenPass, AZ::SystemAllocator, 0);
        virtual ~SplashScreenPass();

        static AZ::RPI::Ptr<SplashScreenPass> Create(const AZ::RPI::PassDescriptor& descriptor);

        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        void SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph) override;
        void CompileResources(const AZ::RHI::FrameGraphCompileContext& context) override;

    private:
        SplashScreenPass(const AZ::RPI::PassDescriptor& descriptor);
        void Clear();

        void InitializeInternal() override;
        void FrameBeginInternal(FramePrepareParams params) override;
        void FrameEndInternal() override;

        bool m_beginTimer = false;
        float m_screenTime = 10.0f;
        float m_lastRealTimeStamp; // in seconds

        struct SplashScreenParams
        {
            float m_fadingFactor;
            uint32_t m_flags;
        };

        AZ::Data::Instance<AZ::RPI::StreamingImage> m_splashScreenImage;
        AZ::RHI::ShaderInputNameIndex m_splashScreenImageIndex = "m_splashScreenImage";

        SplashScreenParams m_splashScreenParams;
        AZ::RHI::ShaderInputNameIndex m_splashScreenParamsIndex = "m_splashScreenParams";

        SplashScreenSettings m_settings;
    };
}
