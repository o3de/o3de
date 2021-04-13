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

#include <Atom/RPI.Public/ViewportContextManager.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/View.h>

namespace AZ
{
    namespace RPI
    {
        static constexpr const char* s_defaultViewportContextName = "Default ViewportContext";

        ViewportContextManager::ViewportContextManager()
        {
            AZ::Interface<ViewportContextRequestsInterface>::Register(this);
        }

        ViewportContextManager::~ViewportContextManager()
        {
            AZ::Interface<ViewportContextRequestsInterface>::Unregister(this);
        }

        void ViewportContextManager::Shutdown()
        {
            m_viewportContexts.clear();
            m_viewportViews.clear();
        }

        void ViewportContextManager::RegisterViewportContext(const Name& contextName, ViewportContextPtr viewportContext)
        {
            {
                AZStd::lock_guard lock(m_containerMutex);

                AZ_Assert(viewportContext != nullptr, "Attempted to register a null ViewportContext \"%s\"", contextName.GetCStr());
                if (!viewportContext)
                {
                    return;
                }

                // Create a context data entry, ensure there wasn't a still-registered existing one
                const auto viewportId = viewportContext->GetId();
                auto& viewportData = m_viewportContexts[viewportId];
                AZ_Assert(viewportData.context.expired(), "Attempted multiple registration for ViewportContext \"%s\" detected, please ensure you call IViewportContextManager::UnregisterViewportContext", contextName.GetCStr());
                if (!viewportData.context.expired())
                {
                    return;
                }
                viewportData.context = viewportContext;
                auto onSizeChanged = [contextName, viewportId](AzFramework::WindowSize size)
                {
                    ViewportContextNotificationBus::Event(contextName, &ViewportContextNotificationBus::Events::OnViewportSizeChanged, size);
                    ViewportContextIdNotificationBus::Event(viewportId, &ViewportContextIdNotificationBus::Events::OnViewportSizeChanged, size);
                };
                viewportContext->m_name = contextName;
                viewportData.sizeChangedHandler = ViewportContext::SizeChangedEvent::Handler(onSizeChanged);
                viewportContext->ConnectSizeChangedHandler(viewportData.sizeChangedHandler);
                ViewPtrStack& associatedViews = GetOrCreateViewStackForContext(contextName);
                viewportContext->SetDefaultView(associatedViews.back());
                onSizeChanged(viewportContext->GetViewportSize());
            }

            ViewportContextManagerNotificationsBus::Broadcast(&ViewportContextManagerNotifications::OnViewportContextAdded, viewportContext);
        }

        void ViewportContextManager::UnregisterViewportContext(AzFramework::ViewportId viewportId)
        {
            {
                AZStd::lock_guard lock(m_containerMutex);

                AZ_Assert(viewportId != AzFramework::InvalidViewportId, "Attempted to unregister an invalid viewport");

                auto viewportContextIt = m_viewportContexts.find(viewportId);
                if (viewportContextIt == m_viewportContexts.end())
                {
                    AZ_Assert(false, "Attempted to unregister a ViewportContext \"%i\" when none is registered", viewportId);
                    return;
                }
                m_viewportContexts.erase(viewportContextIt);
            }

            ViewportContextManagerNotificationsBus::Broadcast(&ViewportContextManagerNotifications::OnViewportContextRemoved, viewportId);
        }

        AzFramework::ViewportId ViewportContextManager::GetViewportIdFromName(const Name& contextName) const
        {
            AZStd::lock_guard lock(m_containerMutex);

            for (const auto& viewportData : m_viewportContexts)
            {
                auto viewportContextRef = viewportData.second.context;
                if (!viewportContextRef.expired() && viewportContextRef.lock()->GetName() == contextName)
                {
                    return viewportData.first;
                }
            }
            return AzFramework::InvalidViewportId;
        }

        ViewportContextPtr ViewportContextManager::CreateViewportContext(const Name& contextName, const CreationParameters& params)
        {
            AzFramework::ViewportId id = params.id;
            if (id != AzFramework::InvalidViewportId)
            {
                if (GetViewportContextById(id))
                {
                    AZ_Assert(false, "Attempted to register multiple ViewportContexts to ID %i", id);
                    return nullptr;
                }
            }
            else
            {
                // Find the first available ID
                do
                {
                    id = m_currentViewportId.fetch_add(1);
                } while (GetViewportContextById(id));
            }
            // Dynamically generate a context name if one isn't provided
            Name nameToUse = contextName;
            if (nameToUse.IsEmpty())
            {
                nameToUse = Name(AZStd::string::format("ViewportContext%i", id));
            }

            AZ_Assert(params.device, "Invalid device provided to CreateViewportContext");
            ViewportContextPtr viewportContext = AZStd::make_shared<ViewportContext>(
                this,
                id,
                nameToUse,
                *params.device,
                params.windowHandle,
                params.renderScene
            );
            viewportContext->GetWindowContext()->RegisterAssociatedViewportContext(viewportContext);
            RegisterViewportContext(nameToUse, viewportContext);
            return viewportContext;
        }

        ViewportContextPtr ViewportContextManager::GetViewportContextByName(const Name& contextName) const
        {
            return GetViewportContextById(GetViewportIdFromName(contextName));
        }

        ViewportContextPtr ViewportContextManager::GetViewportContextById(AzFramework::ViewportId id) const
        {
            AZStd::lock_guard lock(m_containerMutex);

            if (auto viewportIt = m_viewportContexts.find(id); viewportIt != m_viewportContexts.end())
            {
                return viewportIt->second.context.lock();
            }
            return {};
        }

        void ViewportContextManager::RenameViewportContext(ViewportContextPtr viewportContext, const Name& newContextName)
        {
            auto currentAssignedViewportContext = GetViewportContextByName(newContextName);
            if (currentAssignedViewportContext)
            {
                AZ_Assert(false, "Attempted to rename ViewportContext \"%s\" to \"%s\", but \"%s\" is already assigned to another ViewportContext", viewportContext->m_name.GetCStr(), newContextName.GetCStr(), newContextName.GetCStr());
                return;
            }
            GetOrCreateViewStackForContext(newContextName);
            viewportContext->m_name = newContextName;
            UpdateViewForContext(newContextName);
        }

        void ViewportContextManager::EnumerateViewportContexts(AZStd::function<void(ViewportContextPtr)> visitorFunction)
        {
            AZStd::lock_guard lock(m_containerMutex);

            for (const auto& viewportData : m_viewportContexts)
            {
                visitorFunction(viewportData.second.context.lock());
            }
        }

        AZ::Name ViewportContextManager::GetDefaultViewportContextName() const
        {
            return AZ::Name(s_defaultViewportContextName);
        }

        void ViewportContextManager::PushView(const Name& context, ViewPtr view)
        {
            {
                AZStd::lock_guard lock(m_containerMutex);

                AZ_Assert(view, "Attempted to push a null view to context \"%s\"", context.GetCStr());
                AZ_Assert((view->GetUsageFlags() & View::UsageFlags::UsageCamera) != 0, "Attempted to register a non-camera view to context \"%s\", ensure the view is flagged with UsageCamera", context.GetCStr());

                ViewPtrStack& associatedViews = GetOrCreateViewStackForContext(context);
                if (auto it = AZStd::find(associatedViews.begin(), associatedViews.end(), view); it != associatedViews.end())
                {
                    // Remove from its existing position, if any, before re-adding below
                    associatedViews.erase(it);
                }

                associatedViews.push_back(view);
            }

            UpdateViewForContext(context);
        }

        bool ViewportContextManager::PopView(const Name& context, ViewPtr view)
        {
            {
                AZStd::lock_guard lock(m_containerMutex);

                auto viewStackIt = m_viewportViews.find(context);
                if (viewStackIt == m_viewportViews.end())
                {
                    return false;
                }
                ViewPtrStack& associatedViews = viewStackIt->second;
                if (view == associatedViews[0])
                {
                    AZ_Assert(false, "Attempted to pop the root view for context \"%s\"", context.GetCStr());
                    return false;
                }
                auto viewIt = AZStd::find(associatedViews.begin(), associatedViews.end(), view);
                if (viewIt == associatedViews.end())
                {
                    return false;
                }
                associatedViews.erase(viewIt);
            }

            UpdateViewForContext(context);
            return true;
        }

        ViewPtr ViewportContextManager::GetCurrentView(const Name& context) const
        {
            AZStd::lock_guard lock(m_containerMutex);

            if (auto viewIt = m_viewportViews.find(context); viewIt != m_viewportViews.end())
            {
                return viewIt->second.back();
            }
            return {};
        }

        ViewportContextManager::ViewPtrStack& ViewportContextManager::GetOrCreateViewStackForContext(const Name& context)
        {
            // If a stack doesn't already exist, create one now to populate it
            ViewPtrStack& viewStack = m_viewportViews[context];
            if (viewStack.empty())
            {
                Name defaultViewName = Name(AZStd::string::format("%s (Root Camera)", context.GetCStr()));
                ViewPtr defaultView = View::CreateView(defaultViewName, View::UsageFlags::UsageCamera);
                viewStack.push_back(AZStd::move(defaultView));
            }
            return viewStack;
        }

        void ViewportContextManager::UpdateViewForContext(const Name& context)
        {
            auto currentView = GetCurrentView(context);

            for (const auto& viewportData : m_viewportContexts)
            {
                ViewportContextPtr viewportContext = viewportData.second.context.lock();
                if (viewportContext && viewportContext->GetName() == context)
                {
                    viewportContext->SetDefaultView(currentView);
                    ViewportContextIdNotificationBus::Event(
                        viewportContext->GetId(),
                        &ViewportContextIdNotificationBus::Events::OnViewportDefaultViewChanged,
                        currentView);
                    break;
                }
            }

            ViewportContextNotificationBus::Event(
                context,
                &ViewportContextIdNotificationBus::Events::OnViewportDefaultViewChanged,
                currentView);
        }
    } // namespace RPI
} // namespace AZ
