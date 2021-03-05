/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/std/containers/deque.h>
#include <AzCore/std/parallel/atomic.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <Atom/RPI.Public/ViewportContext.h>

namespace AZ
{
    namespace RPI
    {
        //! Exposes the AZ::Interface for ViewportContextRequestsInterface
        class ViewportContextManager final
            : public ViewportContextRequestsInterface
        {
        public:
            AZ_RTTI(ViewportContextManager, "{1198B0B3-109A-4D34-B8EC-2E727ACD9740}", ViewportContextRequestsInterface);

            static constexpr AzFramework::ViewportId StartingViewportId = 1000;

            ~ViewportContextManager();
            void Shutdown();

            // IViewportContextManagerRequests interface
            ViewportContextPtr CreateViewportContext(const Name& contextName, const CreationParameters& params) override;
            ViewportContextPtr GetViewportContextByName(const Name& contextName) const override;
            ViewportContextPtr GetViewportContextById(AzFramework::ViewportId id) const override;
            void RenameViewportContext(ViewportContextPtr viewportContext, const Name& newContextName) override;
            void EnumerateViewportContexts(AZStd::function<void(ViewportContextPtr)> visitorFunction) override;

            AZ::Name GetDefaultViewportContextName() const override;
            void PushView(const Name& contextName, ViewPtr view) override;
            void PopView(const Name& contextName, ViewPtr view) override;
            ViewPtr GetCurrentView(const Name& contextName) const override;

        private:
            void RegisterViewportContext(const Name& contextName, ViewportContextPtr viewportContext);
            void UnregisterViewportContext(AzFramework::ViewportId id);
            AzFramework::ViewportId GetViewportIdFromName(const Name& contextName) const;

            using ViewPtrStack = AZStd::deque<ViewPtr>;
            struct ViewportContextData
            {
                AZStd::weak_ptr<ViewportContext> context;
                ViewportContext::SizeChangedEvent::Handler sizeChangedHandler;
            };

            // ViewportContextManager is a singleton owned solely by RPISystem, which is tagged as a friend
            ViewportContextManager();

            ViewPtrStack& GetOrCreateViewStackForContext(const Name& context);
            void UpdateViewForContext(const Name& context);

            AZStd::unordered_map<AzFramework::ViewportId, ViewportContextData> m_viewportContexts;
            AZStd::unordered_map<AZ::Name, ViewPtrStack> m_viewportViews;
            mutable AZStd::recursive_mutex m_containerMutex;
            AZStd::atomic<AzFramework::ViewportId> m_currentViewportId = StartingViewportId;

            friend class RPISystem;
            friend class ViewportContext;
        };
    } // namespace RPI
} // namespace AZ
