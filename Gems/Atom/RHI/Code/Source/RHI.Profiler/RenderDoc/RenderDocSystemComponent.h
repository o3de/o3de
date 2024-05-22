/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <Atom/RHI.Profiler/GraphicsProfilerBus.h>

#include <renderdoc_app.h>

namespace AZ::RHI
{
    //! System component in charge of loading the RenderDoc library or
    //! connecting to it if it's already loaded. If RenderDoc is present and
    //! available, it registers to the GraphicsProfilerBus to provide GPU
    //! capture functionality using RenderDoc.
    class RenderDocSystemComponent final
        : public AZ::Component
        , public GraphicsProfilerBus::Handler
    {
    public:
        AZ_COMPONENT(RenderDocSystemComponent, "{32718E24-774B-49AD-BD1F-2079D257F3C4}");
        static void Reflect(AZ::ReflectContext* context);

        RenderDocSystemComponent() = default;
        ~RenderDocSystemComponent() override = default;

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);

        // AZ::Component overrides ...
        void Activate() override;
        void Deactivate() override;

        // GraphicsProfilerBus overrides ...
        void StartCapture(const AzFramework::NativeWindowHandle window) override;
        bool EndCapture(const AzFramework::NativeWindowHandle window) override;
        void TriggerCapture() override;

    private:
        RenderDocSystemComponent(const RenderDocSystemComponent&) = delete;

        // Function pointer for accessing the renderdoc functionality.
        RENDERDOC_API_1_1_2* m_renderDocApi = nullptr;
        AZStd::unique_ptr<AZ::DynamicModuleHandle> m_dynamicModule;
    };
} // namespace AZ::RHI
