/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>

#include <AzFramework/Scene/SceneSystemInterface.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Reflect/System/RenderPipelineDescriptor.h>

namespace AZ::Render::Bootstrap
{
    class Request
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual AZ::RPI::ScenePtr GetOrCreateAtomSceneFromAzScene(AzFramework::Scene* scene) = 0;
        virtual bool EnsureDefaultRenderPipelineInstalledForScene(AZ::RPI::ScenePtr scene, AZ::RPI::ViewportContextPtr viewportContext) = 0;
        virtual void SwitchRenderPipeline(const AZ::RPI::RenderPipelineDescriptor& newRenderPipelineDesc, AZ::RPI::ViewportContextPtr viewportContext) = 0;
        virtual void SwitchAntiAliasing(const AZStd::string& newAntiAliasing, AZ::RPI::ViewportContextPtr viewportContext) = 0;
        virtual void SwitchMultiSample(const uint16_t newSampleCount, AZ::RPI::ViewportContextPtr viewportContext) = 0;
        virtual void RefreshWindowResolution() = 0;

    protected:
        ~Request() = default;
    };
    using RequestBus = AZ::EBus<Request>;
} // namespace AZ::Render::Bootstrap
