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

        //! TickBus overrides...
        //! Update tick for animation in the splash screen pass.
        //! Due to deltaTime scalability, it is actually using an abosulte time stamp for delta time.
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        //! Scope producer functions...
        //! Set up srg indices for images and shader data.
        void CompileResources(const AZ::RHI::FrameGraphCompileContext& context) override;

    private:
        SplashScreenPass(const AZ::RPI::PassDescriptor& descriptor);
        void Clear();

        // Pass behavior overrides...
        void InitializeInternal() override;
        void FrameBeginInternal(FramePrepareParams params) override;
        void FrameEndInternal() override;

        //! Flag to begin timing.
        bool m_beginTimer = false;
        //! The time that the splash screen will last. Will be set from splash_screen.setreg.
        float m_durationSeconds = 10.0f;
        //! Time stamp in seconds, used to calculate unscaled delta time.
        float m_lastRealTimeStamp;

        // Data struct passed to the shader.
        struct SplashScreenParams
        {
            float m_fadingFactor;
        };

        //! Shader connections
        AZ::Data::Instance<AZ::RPI::StreamingImage> m_splashScreenImage;
        AZ::RHI::ShaderInputNameIndex m_splashScreenImageIndex = "m_splashScreenImage";

        SplashScreenParams m_splashScreenParams;
        AZ::RHI::ShaderInputNameIndex m_splashScreenParamsIndex = "m_splashScreenParams";

        //! Splash screen settings read from setreg
        SplashScreenSettings m_settings;
    };
}
