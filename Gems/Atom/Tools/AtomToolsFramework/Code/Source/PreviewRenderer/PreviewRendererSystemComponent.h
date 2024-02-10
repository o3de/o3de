/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/PreviewRenderer/PreviewRendererSystemRequestBus.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzFramework/Application/Application.h>
#include <PreviewRenderer/PreviewRenderer.h>

namespace AtomToolsFramework
{
    //! System component that manages a global PreviewRenderer.
    class PreviewRendererSystemComponent final
        : public AZ::Component
        , public AzFramework::ApplicationLifecycleEvents::Bus::Handler
        , public AZ::SystemTickBus::Handler
        , public PreviewRendererSystemRequestBus::Handler
    {
    public:
        AZ_COMPONENT(PreviewRendererSystemComponent, "{E9F79FD8-82F2-4C80-966D-95F28484F229}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        // AZ::Component interface overrides...
        void Init() override;
        void Activate() override;
        void Deactivate() override;

    private:
        // AzFramework::ApplicationLifecycleEvents overrides...
        void OnApplicationAboutToStop() override;

        // AZ::SystemTickBus::Handler overrides...
        void OnSystemTick() override;

        AZStd::unique_ptr<AtomToolsFramework::PreviewRenderer> m_previewRenderer;
    };
} // namespace AtomToolsFramework
