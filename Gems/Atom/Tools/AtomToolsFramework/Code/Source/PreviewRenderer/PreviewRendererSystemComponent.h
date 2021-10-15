/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/PreviewRenderer/PreviewRendererSystemRequestBus.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Component.h>
#include <AzFramework/Application/Application.h>
#include <PreviewRenderer/PreviewRenderer.h>

namespace AtomToolsFramework
{
    //! System component that manages a global PreviewRenderer.
    class PreviewRendererSystemComponent final
        : public AZ::Component
        , public AzFramework::AssetCatalogEventBus::Handler
        , public AzFramework::ApplicationLifecycleEvents::Bus::Handler
        , public PreviewRendererSystemRequestBus::Handler
    {
    public:
        AZ_COMPONENT(PreviewRendererSystemComponent, "{E9F79FD8-82F2-4C80-966D-95F28484F229}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

    protected:
        // AZ::Component interface overrides...
        void Init() override;
        void Activate() override;
        void Deactivate() override;

    private:
        // AzFramework::AssetCatalogEventBus::Handler overrides ...
        void OnCatalogLoaded(const char* catalogFile) override;

        // AzFramework::ApplicationLifecycleEvents overrides...
        void OnApplicationAboutToStop() override;

        AZStd::unique_ptr<AtomToolsFramework::PreviewRenderer> m_previewRenderer;
    };
} // namespace AtomToolsFramework
