/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            m_defaultViewportContextName = AZ::Name(s_defaultViewportContextName);
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
                auto onSizeChanged = [this, viewportId](AzFramework::WindowSize size)
                {
                    // Ensure we emit OnViewportSizeChanged with the correct name.
                    auto viewportContext = GetViewportContextById(viewportId);
                    if (viewportContext)
                    {
                        ViewportContextNotificationBus::Event(viewportContext->GetName(), &ViewportContextNotificationBus::Events::OnViewportSizeChanged, size);
                    }
                    ViewportContextIdNotificationBus::Event(viewportId, &ViewportContextIdNotificationBus::Events::OnViewportSizeChanged, size);
                };
                auto onDpiScalingChanged = [this, viewportId](float dpiScalingFactor)
                {
                    // Ensure we emit OnViewportDpiScalingChanged with the correct name.
                    auto viewportContext = GetViewportContextById(viewportId);
                    if (viewportContext)
                    {
                        ViewportContextNotificationBus::Event(viewportContext->GetName(), &ViewportContextNotificationBus::Events::OnViewportDpiScalingChanged, dpiScalingFactor);
                    }
                    ViewportContextIdNotificationBus::Event(viewportId, &ViewportContextIdNotificationBus::Events::OnViewportDpiScalingChanged, dpiScalingFactor);
                };
                viewportContext->m_name = contextName;
                viewportData.sizeChangedHandler = ViewportContext::SizeChangedEvent::Handler(onSizeChanged);
                viewportData.dpiScalingChangedHandler = ViewportContext::ScalarChangedEvent::Handler(onDpiScalingChanged);
                viewportContext->ConnectSizeChangedHandler(viewportData.sizeChangedHandler);
                viewportContext->ConnectDpiScalingFactorChangedHandler(viewportData.dpiScalingChangedHandler);


                ViewPtrStack& associatedViews = GetOrCreateViewStackForContext(contextName);
                viewportContext->SetDefaultViewGroup(associatedViews.back());
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

        ViewportContextPtr ViewportContextManager::GetViewportContextByScene(const Scene* scene) const
        {
            AZStd::lock_guard lock(m_containerMutex);
            for (const auto& viewportData : m_viewportContexts)
            {
                ViewportContextPtr viewportContext = viewportData.second.context.lock();
                if (viewportContext && viewportContext->GetRenderScene().get() == scene)
                {
                    return viewportContext;
                }
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
            // Ensure anyone listening on per-name viewport size updates gets notified.
            ViewportContextNotificationBus::Event(newContextName, &ViewportContextNotificationBus::Events::OnViewportSizeChanged, viewportContext->GetViewportSize());
            ViewportContextNotificationBus::Event(newContextName, &ViewportContextNotificationBus::Events::OnViewportDpiScalingChanged, viewportContext->GetDpiScalingFactor());
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
            return m_defaultViewportContextName;
        }

        ViewportContextPtr ViewportContextManager::GetDefaultViewportContext() const
        {
            return GetViewportContextByName(m_defaultViewportContextName);
        }

        bool ViewportContextManager::EraseView(const Name& contextName, ViewPtr view)
        {
            ViewPtrStack& associatedViews = GetOrCreateViewStackForContext(contextName);

            auto FindView = [view](ViewGroupPtr vertexGroup)
            {
                return vertexGroup->IsViewInGroup(view);
            };

            if (auto foundIt = AZStd::find_if(associatedViews.begin(), associatedViews.end(), FindView);
                foundIt != associatedViews.end())
            {
                associatedViews.erase(foundIt);
                return true;
            }
            return false;
        }

        bool ViewportContextManager::EraseViewGroup(const Name& contextName, ViewGroupPtr viewGroup)
        {
            ViewPtrStack& associatedViews = GetOrCreateViewStackForContext(contextName);

            auto FindViewGroup = [viewGroup](ViewGroupPtr vertexGroup)
            {
                return vertexGroup->IsViewGroupViewsSame(viewGroup);
            };

            if (auto foundIt = AZStd::find_if(associatedViews.begin(), associatedViews.end(), FindViewGroup);
                foundIt != associatedViews.end())
            {
                associatedViews.erase(foundIt);
                return true;
            }
            return false;
        }

        void ViewportContextManager::PushView(const Name& context, ViewPtr view)
        {
            {
                AZStd::lock_guard lock(m_containerMutex);
                AZ_Assert(view, "Attempted to push a null view to context \"%s\"", context.GetCStr());
                AZ_Assert((view->GetUsageFlags() & View::UsageFlags::UsageCamera) != 0, "Attempted to register a non-camera view to context \"%s\", ensure the view is flagged with UsageCamera", context.GetCStr());

                // Remove from its existing position, if any, before re-adding below
                EraseView(context, view);
    
                ViewPtrStack& associatedViews = GetOrCreateViewStackForContext(context);
                ViewGroupPtr viewGroup = AZStd::make_shared<ViewGroup>();
                viewGroup->Init(ViewGroup::Descriptor{ nullptr, nullptr });
                viewGroup->SetView(view);

                //Todo: Remove this when we can disable XR pipeline if the attached main view is null.
                //Until then we create dummy views for now.
                viewGroup->CreateStereoscopicViews(context);
                associatedViews.push_back(viewGroup);
            }

            UpdateViewForContext(context);
        }
        
        void ViewportContextManager::PushView(const Name& contextName, ViewGroupPtr viewGroup)
        {
            {
                AZStd::lock_guard lock(m_containerMutex);
                AZ_Assert(viewGroup->GetNumViews() > 0, "Attempted to push a null view to context \"%s\"", contextName.GetCStr());

                // Remove from its existing position, if any, before re-adding below
                EraseView(contextName, viewGroup->GetView(ViewType::Default));

                ViewPtrStack& associatedViews = GetOrCreateViewStackForContext(contextName);
                associatedViews.push_back(viewGroup);
            }
            UpdateViewForContext(contextName);
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
                if (view == associatedViews[0]->GetView(ViewType::Default))
                {
                    AZ_Assert(false, "Attempted to pop the root view for context \"%s\"", context.GetCStr());
                    return false;
                }

                // Remove view group which contains this view
                bool viewErased = EraseView(context, view);
                if (!viewErased)
                {
                    return false;
                }
            }
            
            UpdateViewForContext(context);
            return true;
        }
        
        bool ViewportContextManager::PopView(const Name& context, ViewGroupPtr viewGroup)
        {
            {
                AZStd::lock_guard lock(m_containerMutex);
                
                auto viewStackIt = m_viewportViews.find(context);
                if (viewStackIt == m_viewportViews.end())
                {
                    return false;
                }
                ViewPtrStack& associatedViews = viewStackIt->second;
                if (viewGroup->GetView(ViewType::Default) == associatedViews[0]->GetView(ViewType::Default))
                {
                    AZ_Assert(false, "Attempted to pop the root view for context \"%s\"", context.GetCStr());
                    return false;
                }

                // Remove view group with same views
                bool viewGroupErased = EraseViewGroup(context, viewGroup);
                if (!viewGroupErased)
                {
                    return false;
                }
            }
            
            UpdateViewForContext(context);
            return true;
        }

        ViewPtr ViewportContextManager::GetCurrentView(const Name& context) const
        {
            AZStd::lock_guard lock(m_containerMutex);

            if (auto viewIt = m_viewportViews.find(context); viewIt != m_viewportViews.end())
            {
                return viewIt->second.back()->GetView(ViewType::Default);
            }
            return {};
        }

        ViewGroupPtr ViewportContextManager::GetCurrentViewGroup(const Name& contextName)
        {
            AZStd::lock_guard lock(m_containerMutex);

            if (auto viewIt = m_viewportViews.find(contextName); viewIt != m_viewportViews.end())
            {
                return viewIt->second.back();
            }
            return {};
        }

        ViewPtr ViewportContextManager::GetCurrentStereoscopicView(const Name& context, ViewType viewType) const
        {
            AZStd::lock_guard lock(m_containerMutex);

            if (auto viewIt = m_viewportViews.find(context); viewIt != m_viewportViews.end())
            {
                uint32_t xrViewIndex = static_cast<uint32_t>(viewType);
                if (xrViewIndex < viewIt->second.back()->GetNumViews())
                {
                    return viewIt->second.back()->GetView(static_cast<ViewType>(xrViewIndex));
                }      
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
                ViewGroupPtr defaultViewGroup = AZStd::make_shared<ViewGroup>();
                defaultViewGroup->Init(ViewGroup::Descriptor{ nullptr, nullptr });
                defaultViewGroup->CreateMainView(defaultViewName);
                defaultViewGroup->CreateStereoscopicViews(defaultViewName);
                viewStack.push_back(defaultViewGroup);
            }
            return viewStack;
        }

        void ViewportContextManager::UpdateViewForContext(const Name& context)
        {
            auto currentViewGroup = GetCurrentViewGroup(context);

            for (const auto& viewportData : m_viewportContexts)
            {
                ViewportContextPtr viewportContext = viewportData.second.context.lock();
                if (viewportContext && viewportContext->GetName() == context)
                {
                    viewportContext->SetDefaultViewGroup(currentViewGroup);

                    ViewportContextIdNotificationBus::Event(
                        viewportContext->GetId(),
                        &ViewportContextIdNotificationBus::Events::OnViewportDefaultViewChanged,
                        currentViewGroup->GetView(ViewType::Default));
                    break;
                }
            }

            ViewportContextNotificationBus::Event(
                context,
                &ViewportContextNotificationBus::Events::OnViewportDefaultViewChanged,
                currentViewGroup->GetView(ViewType::Default));
        }
    } // namespace RPI
} // namespace AZ
