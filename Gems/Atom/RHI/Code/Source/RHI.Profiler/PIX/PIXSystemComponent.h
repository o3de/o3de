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

namespace AZ::RHI
{
    //! System component in charge of loading the PIX library or
    //! connecting to it if it's already loaded. If PIX is present and
    //! available, it registers to the GraphicsProfilerBus to provide GPU
    //! capture functionality using PIX.
    class PIXSystemComponent final
        : public AZ::Component
        , public GraphicsProfilerBus::Handler
    {
    public:
        AZ_COMPONENT(PIXSystemComponent, "{B9B07D8C-3854-4B04-9FEA-9547B4DE04B1}");
        static void Reflect(AZ::ReflectContext* context);

        PIXSystemComponent() = default;
        ~PIXSystemComponent() override = default;

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);

        // AZ::Component overrides ...
        void Activate() override;
        void Deactivate() override;

        // GraphicsProfilerBus overrides ...
        void StartCapture(const AzFramework::NativeWindowHandle window) override;
        bool EndCapture(const AzFramework::NativeWindowHandle window) override;
        void TriggerCapture() override;

    private:
        PIXSystemComponent(const PIXSystemComponent&) = delete;

        // Manage the loading/unloading of the PIX dynamic library.
        AZStd::unique_ptr<AZ::DynamicModuleHandle> m_dynamicModule;
    };
} // namespace AZ::RHI
