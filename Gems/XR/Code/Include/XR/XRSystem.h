/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <Atom/RHI/ValidationLayer.h>
#include <Atom/RHI/XRRenderingInterface.h>
#include <Atom/RPI.Public/Pass/PassSystemBus.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/XR/XRRenderingInterface.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <XR/XRDevice.h>
#include <XR/XRInstance.h>
#include <XR/XRSwapChain.h>

namespace XR
{
    //! This class is the window to everything XR related.
    //! It implements RPI::RenderingInterface and RHI::RenderingInterface but
    //! can be extended to implement other non rendering interfaces if needed.
    class System
        : public AZ::RPI::XRRenderingInterface
        , public AZ::RHI::XRRenderingInterface
        , public AZ::SystemTickBus::Handler
        , public AZStd::intrusive_base
        , public AZ::RPI::PassSystemTemplateNotificationsBus::MultiHandler
        , public AzFramework::AssetCatalogEventBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(System, AZ::SystemAllocator);
        AZ_RTTI(System, "{C3E0291D-FB30-4E27-AB0D-14606A8C3C1F}");

        System() = default;
        ~System() override = default;
        AZ_DISABLE_COPY_MOVE(System);

        struct Descriptor
        {
            AZ::RHI::ValidationMode m_validationMode = AZ::RHI::ValidationMode::Disabled;
        };

        //! Init the XRSystem.
        void Init(const Descriptor& descriptor);

        //! Destroy any relevant objects held by this .class
        void Shutdown();

        //! Handle XR events and actions
        void OnSystemTick() override;

        ///////////////////////////////////////////////////////////////////
        // AZ::RPI::XRRenderingInterface overrides
        AZ::RHI::ResultCode InitInstance() override;
        void AcquireSwapChainImage(AZ::u32 viewIndex) override;
        AZ::u32 GetNumViews() const override;
        AZ::RHI::ResultCode GetSwapChainImage(AZ::RHI::XRSwapChainDescriptor* swapchainDescriptor) const override;
        AZ::u32 GetSwapChainWidth(AZ::u32 viewIndex) const override;
        AZ::u32 GetSwapChainHeight(AZ::u32 viewIndex) const override;
        AZ::RHI::Format GetSwapChainFormat(AZ::u32 viewIndex) const override;
        AZ::RHI::ResultCode GetViewFov(AZ::u32 viewIndex, AZ::RPI::FovData& outFovData) const override;
        AZ::RHI::ResultCode GetViewPose(AZ::u32 viewIndex, AZ::RPI::PoseData& outPoseData) const override;
        AZ::RHI::ResultCode GetViewFrontPose(AZ::RPI::PoseData& outPoseData) const override;
        AZ::RHI::ResultCode GetViewLocalPose(AZ::RPI::PoseData& outPoseData) const override;
        AZ::RHI::ResultCode GetControllerStagePose(AZ::u32 handIndex, AZ::RPI::PoseData& outPoseData) const override;
        AZ::RHI::ResultCode GetControllerPose(AZ::u32 handIndex, AZ::RPI::PoseData& outPoseData) const override;
        AZ::RHI::ResultCode GetControllerTransform(const AZ::u32 handIndex, AZ::Transform& outTransform) const override;

        float GetControllerScale(AZ::u32 handIndex) const override;
        bool ShouldRender() const override;
        AZ::Matrix4x4 CreateStereoscopicProjection(float angleLeft, float angleRight,
                                                   float angleBottom, float angleTop,
                                                   float nearDist, float farDist, bool reverseDepth) override;
        AZ::RHI::XRRenderingInterface* GetRHIXRRenderingInterface() override;
        float GetXButtonState() const override;
        float GetYButtonState() const override;
        float GetAButtonState() const override;
        float GetBButtonState() const override;
        float GetXJoyStickState(AZ::u32 handIndex) const override;
        float GetYJoyStickState(AZ::u32 handIndex) const override;
        float GetSqueezeState(AZ::u32 handIndex) const override;
        float GetTriggerState(AZ::u32 handIndex) const override;
        ///////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////
        // AZ::RHI::XRRenderingInterface overrides
        AZ::u32 GetNumPhysicalDevices() const override;
        AZ::RHI::ResultCode GetXRPhysicalDevice(AZ::RHI::XRPhysicalDeviceDescriptor* physicalDeviceDescriptor, int32_t index) override;
        AZ::RHI::ResultCode CreateDevice(AZ::RHI::XRDeviceDescriptor* deviceDescriptor) override;
        AZ::RHI::ResultCode CreateSession(AZ::RHI::XRSessionDescriptor* sessionDescriptor) override;
        AZ::RHI::ResultCode CreateSwapChain() override;
        AZ::u32 GetCurrentImageIndex(AZ::u32 viewIndex) const override;
        void BeginFrame() override;
        void EndFrame() override;
        void PostFrame() override;
        bool IsDefaultRenderPipelineNeeded() const override;
        bool IsDefaultRenderPipelineEnabledOnHost() const override;
        AZ::RHI::ResultCode InitVariableRateShadingImageContent(AZ::RHI::Image* image, AZ::RHI::XRFoveatedLevel type) const override;
        ///////////////////////////////////////////////////////////////////

        // AzFramework::AssetCatalogEventBus::Handler overrides ...
        void OnCatalogLoaded(const char*) override;

        // PassSystemTemplateNotificationsBus::Handler overrides ....
        void OnAddingPassTemplate(const AZStd::shared_ptr<AZ::RPI::PassTemplate>& passTemplate) override;

    private:
        Instance* GetInstance();

        bool ConnectToPipelineTemplateListener(const char* pipelineAsset);

        Ptr<Instance> m_instance;
        Ptr<Session> m_session;
        Ptr<SwapChain> m_swapChain;
        Ptr<Device> m_device;
        AZ::RHI::ValidationMode m_validationMode = AZ::RHI::ValidationMode::Disabled;
        bool m_isInFrame = false;
        AZ::RHI::XRFoveatedLevel m_foveatedLevel = AZ::RHI::XRFoveatedLevel::None;
    };
} // namespace XR
