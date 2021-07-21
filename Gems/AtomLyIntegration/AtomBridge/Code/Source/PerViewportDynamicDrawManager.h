/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomBridge/PerViewportDynamicDrawInterface.h>

namespace AZ::AtomBridge
{
    class PerViewportDynamicDrawManager final : public PerViewportDynamicDrawInterface
    {
    public:
        AZ_TYPE_INFO(PerViewportDynamicDrawManager, "{BED66185-00A7-43F7-BD28-C56BC8E4C535}");

        PerViewportDynamicDrawManager();
        ~PerViewportDynamicDrawManager();

        // PerViewportDynamicDrawInterface overrides...
        void RegisterDynamicDrawContext(AZ::Name name, DrawContextFactory contextInitializer) override;
        void UnregisterDynamicDrawContext(AZ::Name name) override;
        RHI::Ptr<RPI::DynamicDrawContext> GetDynamicDrawContextForViewport(AZ::Name name, AzFramework::ViewportId viewportId) override;

    private:
        struct ViewportData
        {
            AZStd::unordered_map<AZ::Name, RHI::Ptr<RPI::DynamicDrawContext>> m_dynamicDrawContexts;

            // Event handlers
            AZ::Event<RPI::RenderPipelinePtr>::Handler m_pipelineChangedHandler;
            AZ::Event<AzFramework::ViewportId>::Handler m_viewportDestroyedHandler;

            // Cached state
            bool m_initialized = false;
        };
        AZStd::map<AzFramework::ViewportId, ViewportData> m_viewportData;
        AZStd::unordered_map<AZ::Name, DrawContextFactory> m_registeredDrawContexts;
        AZStd::mutex m_mutexDrawContexts;
    };
} //namespace AZ::AtomBridge
