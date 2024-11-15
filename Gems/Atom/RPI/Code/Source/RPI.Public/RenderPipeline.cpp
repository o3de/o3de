/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/DrawListTagRegistry.h>

#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/Pass/PassSystem.h>
#include <Atom/RPI.Public/Pass/Specific/SwapChainPass.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/SceneBus.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/ViewProviderBus.h>

#include <Atom/RPI.Reflect/System/AnyAsset.h>


namespace AZ
{
    namespace RPI
    {
        RenderPipelinePtr RenderPipeline::CreateRenderPipeline(const RenderPipelineDescriptor& desc)
        {
            PassSystemInterface* passSystem = PassSystemInterface::Get();
            RenderPipeline* pipeline = aznew RenderPipeline();

            Name passName{ desc.m_name };
            if (!desc.m_rootPassTemplate.empty())
            {
                // Create pass from asset if there is a valid one
                PassRequest rootRequest;
                rootRequest.m_passName = passName;
                rootRequest.m_templateName = desc.m_rootPassTemplate;

                Ptr<Pass> rootPass = passSystem->CreatePassFromRequest(&rootRequest);
                pipeline->m_passTree.m_rootPass = azrtti_cast<ParentPass*>(rootPass.get());
            }
            else
            {
                // Otherwise create an empty root pass with pipeline name
                pipeline->m_passTree.m_rootPass = passSystem->CreatePass<ParentPass>(passName);
            }
            AZ_Assert(pipeline->m_passTree.m_rootPass != nullptr, "Error creating root pass for pipeline!");

            InitializeRenderPipeline(pipeline, desc);

            return RenderPipelinePtr(pipeline);
        }

        RenderPipelinePtr RenderPipeline::CreateRenderPipelineFromAsset(Data::Asset<AnyAsset> pipelineAsset)
        {
            const RenderPipelineDescriptor* renderPipelineDescriptor = GetDataFromAnyAsset<RenderPipelineDescriptor>(pipelineAsset);
            if (renderPipelineDescriptor == nullptr)
            {
                return nullptr;
            }

            RenderPipelinePtr pipeline = RenderPipeline::CreateRenderPipeline(*renderPipelineDescriptor);
            if (pipeline == nullptr)
            {
                AZ_Error("RPISystem", false, "Failed to create render pipeline from asset %s", pipelineAsset.GetHint().c_str());
                return nullptr;
            }

            return pipeline;
        }

        RenderPipelinePtr RenderPipeline::CreateRenderPipelineForWindow(Data::Asset<AnyAsset> pipelineAsset, const WindowContext& windowContext)
        {
            const RenderPipelineDescriptor* renderPipelineDescriptor = GetDataFromAnyAsset<RenderPipelineDescriptor>(pipelineAsset);
            if (renderPipelineDescriptor == nullptr)
            {
                return nullptr;
            }
            return CreateRenderPipelineForWindow(*renderPipelineDescriptor, windowContext);
        }

        RenderPipelinePtr RenderPipeline::CreateRenderPipelineForWindow(const RenderPipelineDescriptor& desc, const WindowContext& windowContext,
                                                                        const ViewType viewType)
        {
            RenderPipelinePtr pipeline{aznew RenderPipeline()};
            PassSystemInterface* passSystem = PassSystemInterface::Get();

            PassDescriptor swapChainDescriptor(Name(desc.m_name));
            Name templateName = Name(desc.m_rootPassTemplate.c_str());
            swapChainDescriptor.m_passTemplate = passSystem->GetPassTemplate(templateName);

            if (!swapChainDescriptor.m_passTemplate)
            {
                AZ_Error("RPISystem", false, "Root-PassTemplate %s not found!", templateName.GetCStr());
                return nullptr;
            }

            pipeline->m_passTree.m_rootPass = aznew SwapChainPass(swapChainDescriptor, &windowContext, viewType);
            pipeline->m_windowHandle = windowContext.GetWindowHandle();
            pipeline->m_viewType = viewType;
            InitializeRenderPipeline(pipeline.get(), desc);

            return pipeline;
        }

        RenderPipelinePtr RenderPipeline::CreateRenderPipelineForImage(const RenderPipelineDescriptor& desc, Data::Asset<AttachmentImageAsset> imageAsset)
        {
            RenderPipelinePtr pipeline{aznew RenderPipeline()};
            PassSystemInterface* passSystem = PassSystemInterface::Get();

            PassRequest passRequest;

            PassImageAttachmentDesc imageAttachmentDesc;
            imageAttachmentDesc.m_assetRef.m_assetId = imageAsset.GetId();
            imageAttachmentDesc.m_lifetime = RHI::AttachmentLifetimeType::Imported;
            imageAttachmentDesc.m_name = Name("OutputImage");
            passRequest.m_imageAttachmentOverrides.push_back(imageAttachmentDesc);

            auto passTemplate = passSystem->GetPassTemplate(Name(desc.m_rootPassTemplate));

            if (!passTemplate)
            {
                AZ_Error("RPI", false, "Failed to create a RenderPipeline: the render pipeline root pass template doesn't exist");
                return nullptr;
            }

            PassConnection passConnection;

            // use first output slot for connection
            for (auto slot : passTemplate->m_slots)
            {
                if (slot.m_slotType == RPI::PassSlotType::Output || slot.m_slotType == RPI::PassSlotType::InputOutput)
                {
                    passConnection.m_localSlot = slot.m_name;
                    break;
                }
            }

            if (passConnection.m_localSlot.IsEmpty())
            {
                AZ_Error("RPI", false, "Failed to create a RenderPipeline: the render pipeline root pass template doesn't have output slot for render target");
                return nullptr;
            }

            passConnection.m_attachmentRef.m_pass = "This";
            passConnection.m_attachmentRef.m_attachment = imageAttachmentDesc.m_name;
            passRequest.m_passName = desc.m_name;
            passRequest.m_templateName = desc.m_rootPassTemplate;
            passRequest.m_connections.push_back(passConnection);
                        
            auto rootPass = passSystem->CreatePassFromRequest(&passRequest);
            if (!rootPass)
            {
                AZ_Error("RPI", false, "Failed to create a RenderPipeline: failed to create root pass for the render pipeline");
                return nullptr;
            }
            pipeline->m_passTree.m_rootPass = azrtti_cast<ParentPass*>(rootPass.get());

            InitializeRenderPipeline(pipeline.get(), desc);

            return pipeline;
        }

        void RenderPipeline::InitializeRenderPipeline(RenderPipeline* pipeline, const RenderPipelineDescriptor& desc)
        {
            pipeline->m_descriptor = desc;
            pipeline->m_mainViewTag = Name(desc.m_mainViewTagName);
            pipeline->m_nameId = desc.m_name.data();
            pipeline->m_materialPipelineTagName = Name{desc.m_materialPipelineTag};
            pipeline->m_activeRenderSettings = desc.m_renderSettings;
            pipeline->m_activeAAMethod = GetAAMethodByName(desc.m_defaultAAMethod);
            pipeline->m_allowSubpassMerging = desc.m_allowSubpassMerging && RHI::RHISystemInterface::Get()->CanMergeSubpasses();
            pipeline->m_passTree.m_rootPass->SetRenderPipeline(pipeline);
            pipeline->m_passTree.m_rootPass->m_flags.m_isPipelineRoot = true;
            pipeline->m_passTree.m_rootPass->ManualPipelineBuildAndInitialize();
            
            pipeline->SetActiveAAMethod(desc.m_defaultAAMethod);
            pipeline->UpdateViewportScissor();
        }

        void RenderPipeline::UpdateViewportScissor()
        {
            for (PassAttachmentBinding& binding : m_passTree.m_rootPass->m_attachmentBindings)
            {
                if (binding.m_slotType == PassSlotType::Output || binding.m_slotType == PassSlotType::InputOutput)
                {
                    auto attachment = binding.GetAttachment();
                    if (attachment && attachment->GetAttachmentType() == RHI::AttachmentType::Image)
                    {
                        RHI::ImageDescriptor imageDesc;
                        if (attachment->m_importedResource)
                        {
                            AttachmentImage* image = static_cast<AttachmentImage*>(attachment->m_importedResource.get());
                            imageDesc = image->GetDescriptor();
                        }
                        else
                        {
                            imageDesc = attachment->m_descriptor.m_image;
                        }
                        m_viewport = RHI::Viewport(0, (float)imageDesc.m_size.m_width, 0, (float)imageDesc.m_size.m_height);
                        m_scissor = RHI::Scissor(0, 0, imageDesc.m_size.m_width, imageDesc.m_size.m_height);
                        return;
                    }
                }
            }
        }

        bool RenderPipeline::SubpassMergingSupported() const
        {
            return m_allowSubpassMerging;
        }

        RenderPipeline::~RenderPipeline()
        {
            if (m_passTree.m_rootPass)
            {
                m_passTree.m_rootPass->SetRenderPipeline(nullptr);
            }
        }

        void RenderPipeline::BuildPipelineViews()
        {
            if (m_passTree.m_rootPass == nullptr)
            {
                return;
            }

            // Get view tags from all passes.
            PipelineViewTags viewTags;
            m_passTree.m_rootPass->GetPipelineViewTags(viewTags);

            // Use a new list for building pipeline views since we may need information from the previous list in m_views in the process
            PipelineViewMap newViewsByTag;

            // re-register only views where the view-tag still exists after rebuilding.
            m_persistentViewsByViewTag.clear();

            for (const auto& tag : viewTags)
            {
                PipelineViews pipelineViews;
                if (m_pipelineViewsByTag.find(tag) != m_pipelineViewsByTag.end())
                {
                    // Copy the content from existing if it already exists
                    pipelineViews = m_pipelineViewsByTag[tag];
                    pipelineViews.m_drawListMask.reset();
                    if (pipelineViews.m_type == PipelineViewType::Transient)
                    {
                        pipelineViews.m_views.clear();
                    }
                    else if (pipelineViews.m_type == PipelineViewType::Persistent)
                    {
                        for (auto& view : pipelineViews.m_views)
                        {
                            if (view)
                            {
                                m_persistentViewsByViewTag[view.get()] = pipelineViews.m_viewTag;
                            }
                        }
                    }
                }
                else
                {
                    pipelineViews.m_viewTag = tag;
                    pipelineViews.m_type = PipelineViewType::Unknown;
                }
                newViewsByTag[tag] = pipelineViews;
                CollectDrawListMaskForViews(newViewsByTag[tag]);
            }

            m_pipelineViewsByTag = AZStd::move(newViewsByTag);

            // transient views are re-registered every frame anyway
            m_transientViewsByViewTag.clear();
        }

        void RenderPipeline::CollectDrawListMaskForViews(PipelineViews& views)
        {
            views.m_drawListMask.reset();
            views.m_passesByDrawList.clear();
            m_passTree.m_rootPass->GetViewDrawListInfo(views.m_drawListMask, views.m_passesByDrawList, views.m_viewTag);
        }

        bool RenderPipeline::CanRegisterView(const PipelineViewTag& allowedViewTag, const View* view) const
        {
            auto registeredViewItr = m_persistentViewsByViewTag.find(view);
            if (registeredViewItr != m_persistentViewsByViewTag.end() && registeredViewItr->second != allowedViewTag)
            {
                AZ_Warning("RenderPipeline", false, "View [%s] is already registered for persistent ViewTag [%s].",
                         view->GetName().GetCStr(), registeredViewItr->second.GetCStr());
                return false;
            }

            registeredViewItr = m_transientViewsByViewTag.find(view);
            if (registeredViewItr != m_transientViewsByViewTag.end() && registeredViewItr->second != allowedViewTag)
            {
                AZ_Warning("RenderPipeline", false, "View [%s] is already registered for transient ViewTag [%s].",
                         view->GetName().GetCStr(), registeredViewItr->second.GetCStr());
                return false;
            }
            return true;
        }

        void RenderPipeline::UnregisterView(ViewPtr view)
        {
            auto registeredViewItr = m_persistentViewsByViewTag.find(view.get());
            if (registeredViewItr != m_persistentViewsByViewTag.end())
            {
                return ResetPersistentView(registeredViewItr->second, view);
            }

            registeredViewItr = m_transientViewsByViewTag.find(view.get());
            if (registeredViewItr != m_transientViewsByViewTag.end())
            {
                return RemoveTransientView(registeredViewItr->second, view);
            }
        }

        void RenderPipeline::RemoveTransientView(const PipelineViewTag viewTag, ViewPtr view)
        {
            auto viewItr = m_pipelineViewsByTag.find(viewTag);
            if (viewItr != m_pipelineViewsByTag.end())
            {
                PipelineViews& pipelineViews = viewItr->second;
                if (pipelineViews.m_type == PipelineViewType::Persistent)
                {
                    AZ_Assert(
                        false, "View [%s] was set as persistent view. Use ResetPersistentView to remove this view", viewTag.GetCStr());
                    return;
                }
                for (int viewIndex = 0; viewIndex < pipelineViews.m_views.size(); ++viewIndex)
                {
                    if (pipelineViews.m_views[viewIndex] == view)
                    {
                        view->SetPassesByDrawList(nullptr);
                        pipelineViews.m_views.erase(pipelineViews.m_views.begin() + viewIndex);
                        m_transientViewsByViewTag.erase(view.get());
                        break;
                    }
                }
                if (pipelineViews.m_views.empty())
                {
                    pipelineViews.m_type = PipelineViewType::Unknown;
                }
            }
        }

        void RenderPipeline::ResetPersistentView(const PipelineViewTag viewTag, ViewPtr view)
        {
            auto viewItr = m_pipelineViewsByTag.find(viewTag);
            if (viewItr != m_pipelineViewsByTag.end())
            {
                PipelineViews& pipelineViews = viewItr->second;
                if (pipelineViews.m_views.size() == 0)
                {
                    return;
                }

                if (pipelineViews.m_type == PipelineViewType::Transient)
                {
                    AZ_Assert(
                        false,
                        "View [%s] is a transient view. Use RemoveTransientView to remove it, or wait until the next frame.",
                        viewTag.GetCStr());
                    return;
                }

                AZ_Assert(
                    pipelineViews.m_views[0] == view,
                    "View [%s] is not registered for persistent view tag [%s]",
                    pipelineViews.m_views[0]->GetName().GetCStr(),
                    viewTag.GetCStr());

                // persistent views always have exactly one view
                pipelineViews.m_views[0]->SetPassesByDrawList(nullptr);
                m_persistentViewsByViewTag.erase(pipelineViews.m_views[0].get());

                // we are removing the only view, so we have to set the type to Unknown or the engine assumes m_views[0] is valid
                pipelineViews.m_views.clear();
                pipelineViews.m_type = PipelineViewType::Unknown;

                if (m_scene)
                {
                    ViewPtr newView{ nullptr };
                    SceneNotificationBus::Event(
                        m_scene->GetId(), &SceneNotification::OnRenderPipelinePersistentViewChanged, this, viewTag, newView, view);
                }
            }
        }

        void RenderPipeline::SetPersistentView(const PipelineViewTag& viewTag, ViewPtr view)
        {
            // If a view is registered for multiple viewTags, it gets only the PassesByDrawList of whatever
            // DrawList it was registered last, which will cause a crash during SortDrawList later. So we check
            // here if the view is already registered with another viewTag.
            // TODO: remove this check and merge the PassesByDrawList if that behaviour is actually needed.
            if (!CanRegisterView(viewTag, view.get()))
            {
                AZ_Assert(false, "Can't register view [%s] with viewTag [%s]", view->GetName().GetCStr(), viewTag.GetCStr());
                return;
            }

            auto viewItr = m_pipelineViewsByTag.find(viewTag);
            if (viewItr != m_pipelineViewsByTag.end())
            {
                PipelineViews& pipelineViews = viewItr->second;

                ViewPtr previousView{ nullptr };

                if (pipelineViews.m_type == PipelineViewType::Transient)
                {
                    AZ_Assert(false, "View [%s] was set as transient view. Use AddTransientView function to add a view for this tag.", viewTag.GetCStr());
                    return;
                }
                else if (pipelineViews.m_type == PipelineViewType::Unknown) // first time registering a view for this viewTag
                {
                    pipelineViews.m_type = PipelineViewType::Persistent;
                    pipelineViews.m_views.resize(1, nullptr);
                }
                else if (pipelineViews.m_type == PipelineViewType::Persistent) // re-registering a view
                {
                    AZ_Assert(
                        pipelineViews.m_views.size() == 1, "SetPersistentView(): PipelineViewType::Persistent needs exactly one view.");
                    AZ_Assert(pipelineViews.m_views[0] != nullptr, "SetPersistentView(): previous view is invalid.");
                    previousView = pipelineViews.m_views[0];
                }

                if (view)
                {
                    view->OnAddToRenderPipeline();
                    pipelineViews.m_views[0] = view;
                    m_persistentViewsByViewTag[view.get()] = viewTag;
                }
                else { // view == nullptr
                    // we are removing the view, so we have to set the type to Unknown or the engine assumes m_views[0] is valid
                    pipelineViews.m_views.clear();
                    pipelineViews.m_type = PipelineViewType::Unknown;
                }

                if (previousView)
                {
                    previousView->SetPassesByDrawList(nullptr);
                    m_persistentViewsByViewTag.erase(previousView.get());
                }

                if (m_scene)
                {
                    SceneNotificationBus::Event(m_scene->GetId(), &SceneNotification::OnRenderPipelinePersistentViewChanged, this, viewTag, view, previousView);
                }
            }
            else
            {
                AZ_Assert(false, "View [%s] doesn't exist in render pipeline [%s]", viewTag.GetCStr(), m_nameId.GetCStr());
            }
        }

        void RenderPipeline::SetDefaultView(ViewPtr view)
        {
            SetPersistentView(m_mainViewTag, view);
        }

        ViewPtr RenderPipeline::GetDefaultView()
        {
            return GetFirstView(m_mainViewTag);
        }

        ViewPtr RenderPipeline::GetFirstView(const PipelineViewTag& viewTag)
        {
            const AZStd::vector<ViewPtr>& views = GetViews(viewTag);
            if (!views.empty())
            {
                return views[0];
            }
            return {};
        }

        void RenderPipeline::SetDefaultViewFromEntity(EntityId entityId)
        {
            ViewPtr cameraView;
            ViewProviderBus::EventResult(cameraView, entityId, &ViewProvider::GetView);
            if (cameraView)
            {
                SetDefaultView(cameraView);
            }
        }

        void RenderPipeline::SetDefaultStereoscopicViewFromEntity(EntityId entityId, RPI::ViewType viewType)
        {
            ViewPtr cameraView;
            ViewProviderBus::EventResult(cameraView, entityId, &ViewProvider::GetStereoscopicView, viewType);
            if (cameraView)
            {
                SetDefaultView(cameraView);
            }
        }

        void RenderPipeline::AddTransientView(const PipelineViewTag& viewTag, ViewPtr view)
        {
            AZ_Assert(view, "Transient View for ViewTag [%s] is invalid.", viewTag.GetCStr());
            // If a view is registered for multiple viewTags, it gets only the PassesByDrawList of whatever
            // DrawList it was registered last, which will cause a crash during SortDrawList later. So we check
            // here if the view is already registered with another viewTag.
            // TODO: remove this check and merge the PassesByDrawList if that behaviour is actually needed.
            if (!CanRegisterView(viewTag, view.get()))
            {
                AZ_Assert(false, "Can't register transient view [%s] with viewTag [%s]", view->GetName().GetCStr(), viewTag.GetCStr());
                return;
            }

            auto viewItr = m_pipelineViewsByTag.find(viewTag);
            if (viewItr != m_pipelineViewsByTag.end())
            {
                PipelineViews& pipelineViews = viewItr->second;
                if (pipelineViews.m_type == PipelineViewType::Persistent)
                {
                    AZ_Assert(false, "View [%s] was set as persistent view. Use SetPersistentView function to set a view for this tag", viewTag.GetCStr());
                    return;
                }
                if (pipelineViews.m_type == PipelineViewType::Unknown)
                {
                    pipelineViews.m_type = PipelineViewType::Transient;
                }
                view->SetPassesByDrawList(&pipelineViews.m_passesByDrawList);
                view->OnAddToRenderPipeline();
                pipelineViews.m_views.push_back(view);
                m_transientViewsByViewTag[view.get()] = viewTag;
            }
        }

        bool RenderPipeline::HasViewTag(const PipelineViewTag& viewTag) const
        {
            return m_pipelineViewsByTag.find(viewTag) != m_pipelineViewsByTag.end();
        }

        const PipelineViewTag& RenderPipeline::GetMainViewTag() const
        {
            return m_mainViewTag;
        }

        const AZStd::vector<ViewPtr>& RenderPipeline::GetViews(const PipelineViewTag& viewTag) const
        {
            auto viewItr = m_pipelineViewsByTag.find(viewTag);
            if (viewItr != m_pipelineViewsByTag.end())
            {
                return viewItr->second.m_views;
            }
            static AZStd::vector<ViewPtr> emptyList;
            return emptyList;
        }

        const RHI::DrawListMask& RenderPipeline::GetDrawListMask(const PipelineViewTag& viewTag) const
        {
            auto viewItr = m_pipelineViewsByTag.find(viewTag);
            if (viewItr != m_pipelineViewsByTag.end())
            {
                return viewItr->second.m_drawListMask;
            }
            static RHI::DrawListMask emptyMask;
            return emptyMask;
        }

        const RenderPipeline::PipelineViewMap& RenderPipeline::GetPipelineViews() const
        {
            return m_pipelineViewsByTag;
        }

        void RenderPipeline::OnAddedToScene(Scene* scene)
        {
            AZ_Assert(m_scene == nullptr, "Pipeline was added to another scene");
            m_scene = scene;
            PassSystemInterface::Get()->AddRenderPipeline(this);
        }

        void RenderPipeline::OnRemovedFromScene([[maybe_unused]] Scene* scene)
        {
            m_passTree.ClearQueues();

            AZ_Assert(m_scene == scene, "Pipeline isn't added to the specified scene");
            m_scene = nullptr;
            PassSystemInterface::Get()->RemoveRenderPipeline(this);

            m_drawFilterTagForPipelineInstanceName.Reset();
            m_drawFilterTagForMaterialPipeline.Reset();
            m_drawFilterMask = 0;
        }

        void RenderPipeline::ProcessQueuedPassChanges()
        {
            m_passTree.ProcessQueuedChanges();
        }

        void RenderPipeline::UpdatePasses()
        {
            // Rebuild Pipeline if needed, for example if passes where hot reloaded
            if (PipelineNeedsRebuild(m_pipelinePassChanges))
            {
                // Process any queued changes before we attempt to reload the pipeline
                m_passTree.ProcessQueuedChanges();

                // Attempt to re-create hierarchy under root pass
                Ptr<ParentPass> newRoot = azrtti_cast<ParentPass*>(m_passTree.m_rootPass->Recreate().get());
                newRoot->SetRenderPipeline(this);
                newRoot->m_flags.m_isPipelineRoot = true;
                newRoot->SetDeviceIndex(m_passTree.m_rootPass->GetDeviceIndex());
                newRoot->ManualPipelineBuildAndInitialize();

                // Validate the new root
                PassValidationResults validation;
                newRoot->Validate(validation);
                if (validation.IsValid())
                {
                    // Remove old pass
                    m_passTree.m_rootPass->SetRenderPipeline(nullptr);
                    m_passTree.m_rootPass->QueueForRemoval();

                    // Set new root
                    m_passTree.m_rootPass = newRoot;
                    PassSystemInterface::Get()->GetRootPass()->AddChild(m_passTree.m_rootPass);

                    // Re-Apply render pipeline change
                    m_wasModifiedByScene = false;
                    m_scene->TryApplyRenderPipelineChanges(this);
                }
                else
                {
                    AZ_Printf("PassSystem", "\n>> Pass validation failed after hot reloading pas assets. Reverting to previously valid render pipeline.\n");
                    validation.PrintValidationIfError();
#if AZ_RPI_ENABLE_PASS_DEBUGGING
                    AZ_Printf("PassSystem", "\nConstructed pass hierarchy with validation errors is as follows:\n");
                    newRoot->DebugPrint();
#endif
                }
                
                SetAAMethod(this, m_activeAAMethod);
            }

            // Build and initialize any queued passes
            m_passTree.ProcessQueuedChanges();

            if (m_pipelinePassChanges != PipelinePassChanges::NoPassChanges)
            {
                m_passTree.m_rootPass->SetRenderPipeline(this);

                // Pipeline views
                if (PipelineViewsNeedRebuild(m_pipelinePassChanges))
                {
                    BuildPipelineViews();
                }

                if (m_scene)
                {
                    SceneNotificationBus::Event(m_scene->GetId(), &SceneNotification::OnRenderPipelinePassesChanged, this);
                    SceneNotificationBus::Event(m_scene->GetId(), &SceneNotification::OnRenderPipelineChanged, this,
                        SceneNotification::RenderPipelineChangeType::PassChanged);

                    // Pipeline state lookup
                    if (PipelineStateLookupNeedsRebuild(m_pipelinePassChanges))
                    {
                        SceneRequestBus::Event(m_scene->GetId(), &SceneRequest::PipelineStateLookupNeedsRebuild);
                    }
                }
                UpdateViewportScissor();
                // Reset change flags
                m_pipelinePassChanges = PipelinePassChanges::NoPassChanges;

                if (m_scene)
                {
                    // Process any changes that may have happened due to SceneNotification Events. This may cause the
                    // m_pipelinePassChanges flag to change and be handled later.
                    m_passTree.ProcessQueuedChanges();
                }
            }
        }

        bool RenderPipeline::IsExecuteOnce()
        {
            return m_descriptor.m_executeOnce;
        }

        void RenderPipeline::RemoveFromScene()
        {
            if (m_scene == nullptr)
            {
                AZ_Assert(false, "RenderPipeline::RemoveFromScene: Pipeline [%s] isn't added to any scene", m_nameId.GetCStr());
                return;
            }

            m_scene->RemoveRenderPipeline(m_nameId);
        }

        void RenderPipeline::OnStartFrame([[maybe_unused]] float time)
        {
            AZ_PROFILE_SCOPE(RPI, "RenderPipeline: OnStartFrame");

            UpdatePasses();

            for (auto& viewItr : m_pipelineViewsByTag)
            {
                PipelineViews& pipelineViews = viewItr.second;

                if (pipelineViews.m_type == PipelineViewType::Transient)
                {
                    // Clear transient views
                    pipelineViews.m_views.clear();
                }
                else if (pipelineViews.m_type == PipelineViewType::Persistent)
                {
                    pipelineViews.m_views[0]->SetPassesByDrawList(&pipelineViews.m_passesByDrawList);
                }
            }
            m_transientViewsByViewTag.clear();
        }

        void RenderPipeline::OnFrameEnd()
        {
            if (m_renderMode == RenderMode::RenderOnce)
            {
                RemoveFromRenderTick();
            }
        }

        void RenderPipeline::PassSystemFrameBegin(Pass::FramePrepareParams params)
        {
            AZ_PROFILE_FUNCTION(RPI);
            if (GetRenderMode() != RenderPipeline::RenderMode::NoRender)
            {
                params.m_viewportState = m_viewport;
                params.m_scissorState = m_scissor;
                m_passTree.m_rootPass->UpdateConnectedBindings();
                m_passTree.m_rootPass->FrameBegin(params);
            }
        }

        void RenderPipeline::PassSystemFrameEnd()
        {
            AZ_PROFILE_FUNCTION(RPI);
            if (GetRenderMode() != RenderPipeline::RenderMode::NoRender)
            {
                m_passTree.m_rootPass->FrameEnd();
            }
        }

        void RenderPipeline::CollectPersistentViews(AZStd::map<ViewPtr, RHI::DrawListMask>& outViewMasks) const
        {
            for (auto& viewItr : m_pipelineViewsByTag)
            {
                const PipelineViews& pipelineViews = viewItr.second;

                if (pipelineViews.m_type == PipelineViewType::Persistent)
                {
                    ViewPtr view = pipelineViews.m_views[0];
                    if (outViewMasks.find(view) == outViewMasks.end())
                    {
                        // Add the view to the map with its DrawListMask if the view isn't in the list
                        outViewMasks[view] = pipelineViews.m_drawListMask;
                    }
                    else
                    {
                        // Combine the DrawListMask with the existing one if the view already exist.
                        outViewMasks[view] |= pipelineViews.m_drawListMask;
                    }
                }
            }
        }

        const PipelineGlobalBinding* RenderPipeline::GetPipelineGlobalConnection(const Name& globalName) const
        {
            for (const PipelineGlobalBinding& connection : m_pipelineGlobalConnections)
            {
                if (connection.m_globalName == globalName)
                {
                    return &connection;
                }
            }
            return nullptr;
        }

        void RenderPipeline::AddPipelineGlobalConnection(const Name& globalName, PassAttachmentBinding* binding, Pass* pass)
        {
            m_pipelineGlobalConnections.push_back(PipelineGlobalBinding{ globalName, binding, pass });
        }

        void RenderPipeline::RemovePipelineGlobalConnectionsFromPass(Pass* passOnwer)
        {
            auto iter = m_pipelineGlobalConnections.begin();
            while (iter != m_pipelineGlobalConnections.end())
            {
                if (iter->m_pass == passOnwer)
                {
                    m_pipelineGlobalConnections.erase(iter);
                }
                else
                {
                    ++iter;
                }
            }
        }

        void RenderPipeline::ClearGlobalBindings()
        {
            m_pipelineGlobalConnections.clear();
        }

        RenderPipelineId RenderPipeline::GetId() const
        {
            return m_nameId;
        }

        const Ptr<ParentPass>& RenderPipeline::GetRootPass() const
        {
            return m_passTree.m_rootPass;
        }

        void RenderPipeline::MarkPipelinePassChanges(u32 passChangeFlags)
        {
            m_pipelinePassChanges |= passChangeFlags;
        }

        Scene* RenderPipeline::GetScene() const
        {
            return m_scene;
        }

        AzFramework::NativeWindowHandle RenderPipeline::GetWindowHandle() const
        {
            return m_windowHandle;
        }

        PipelineRenderSettings& RenderPipeline::GetRenderSettings()
        {
            return m_activeRenderSettings;
        }

        const PipelineRenderSettings& RenderPipeline::GetRenderSettings() const
        {
            return m_activeRenderSettings;
        }

        void RenderPipeline::RevertRenderSettings()
        {
            m_activeRenderSettings = m_descriptor.m_renderSettings;
        }

        void RenderPipeline::AddToRenderTickOnce()
        {
            m_renderMode = RenderMode::RenderOnce;
        }

        void RenderPipeline::AddToRenderTick()
        {
            m_renderMode = RenderMode::RenderEveryTick;
        }

        void RenderPipeline::RemoveFromRenderTick()
        {
            m_renderMode = RenderMode::NoRender;
        }

        RenderPipeline::RenderMode RenderPipeline::GetRenderMode() const
        {
            return m_renderMode;
        }

        bool RenderPipeline::NeedsRender() const
        {
            return m_renderMode != RenderMode::NoRender;
        }

        RHI::DrawFilterMask RenderPipeline::GetDrawFilterMask() const
        {
            return m_drawFilterMask;
        }

        void RenderPipeline::SetDrawFilterTags(RHI::DrawFilterTagRegistry* tagRegistry)
        {   
            m_drawFilterTagForPipelineInstanceName = tagRegistry->AcquireTag(m_nameId);
            m_drawFilterTagForMaterialPipeline = tagRegistry->AcquireTag(m_materialPipelineTagName);
                        
            m_drawFilterMask = 0;
            
            if (m_drawFilterTagForPipelineInstanceName.IsValid())
            {
                m_drawFilterMask |= 1 << m_drawFilterTagForPipelineInstanceName.GetIndex();
            }
            if (m_drawFilterTagForMaterialPipeline.IsValid())
            {
                m_drawFilterMask |= 1 << m_drawFilterTagForMaterialPipeline.GetIndex();
            }
        }

        void RenderPipeline::ReleaseDrawFilterTags(RHI::DrawFilterTagRegistry* tagRegistry)
        {
            tagRegistry->ReleaseTag(m_drawFilterTagForPipelineInstanceName);
            tagRegistry->ReleaseTag(m_drawFilterTagForMaterialPipeline);
            m_drawFilterTagForPipelineInstanceName.Reset();
            m_drawFilterTagForMaterialPipeline.Reset();
        }

        const RenderPipelineDescriptor& RenderPipeline::GetDescriptor() const
        {
            return m_descriptor;
        }

        bool RenderPipeline::AddPassBefore(Ptr<Pass> newPass, const AZ::Name& referencePassName)
        {
            auto foundPass = FindFirstPass(referencePassName);

            if (!foundPass)
            {
                AZ_Warning("RenderPipeline", false, "Add pass to render pipeline failed: can't find reference pass [%s] in render pipeline [%s]",
                    referencePassName.GetCStr(), GetId().GetCStr());
                return false;
            }

            // insert the pass
            auto parentPass = foundPass->GetParent();
            auto passIndex = parentPass->FindChildPassIndex(referencePassName);
            // Note: no need to check if passIndex is valid since the pass was already found
            return parentPass->InsertChild(newPass, passIndex.GetIndex());
        }

        bool RenderPipeline::AddPassAfter(Ptr<Pass> newPass, const AZ::Name& referencePassName)
        {
            auto foundPass = FindFirstPass(referencePassName);

            if (!foundPass)
            {
                AZ_Warning("RenderPipeline", false, "Add pass to render pipeline failed: can't find reference pass [%s] in render pipeline [%s]",
                    referencePassName.GetCStr(), GetId().GetCStr());
                return false;
            }

            // insert the pass
            auto parentPass = foundPass->GetParent();
            auto passIndex = parentPass->FindChildPassIndex(referencePassName);
            // Note: no need to check if passIndex is valid since the pass was already found
            return parentPass->InsertChild(newPass, passIndex.GetIndex()+1);
        }

        Ptr<Pass> RenderPipeline::FindFirstPass(const AZ::Name& passName)
        {
            auto passFilter = RPI::PassFilter::CreateWithPassHierarchy({passName});
            passFilter.SetOwnerRenderPipeline(this);
            RPI::Ptr<RPI::Pass> foundPass = nullptr;
            RPI::PassSystemInterface::Get()->ForEachPass(passFilter, [&foundPass](RPI::Pass* pass) -> RPI::PassFilterExecutionFlow
                {
                    foundPass = pass;
                    return RPI::PassFilterExecutionFlow::StopVisitingPasses;
                });

            return foundPass;
        }

        ViewType RenderPipeline::GetViewType() const
        {
            return m_viewType;
        }

        // ---------------------------- Anti-aliasing

        bool RenderPipeline::SetActiveAAMethod(AZStd::string aaMethodName)
        {
            AntiAliasingMode antiAliasingMode = GetAAMethodByName(aaMethodName);
            if (antiAliasingMode == AntiAliasingMode::Default)
            {
                return false;
            }
            m_activeAAMethod = antiAliasingMode;
            return SetAAMethod(this, m_activeAAMethod);
        }

        AntiAliasingMode RenderPipeline::GetActiveAAMethod()
        {
            return m_activeAAMethod;
        }

        AntiAliasingMode RenderPipeline::GetAAMethodByName(AZStd::string aaMethodName)
        {
            const AZStd::unordered_map<AZStd::string, AntiAliasingMode> AAMethodsLookup = {
                {"MSAA", AntiAliasingMode::MSAA}, {"SMAA", AntiAliasingMode::SMAA}, 
                {"TAA", AntiAliasingMode::TAA}
            };
 
            auto findIt = AAMethodsLookup.find(aaMethodName);
            if (findIt != AAMethodsLookup.end())
            {
                return findIt->second;
            }
            return AntiAliasingMode::Default;
        }        

        AZStd::string RenderPipeline::GetAAMethodNameByIndex(AntiAliasingMode aaMethodIndex)
        {
            const AZStd::unordered_map<AntiAliasingMode, AZStd::string> AAMethodNameLookup = {
                {AntiAliasingMode::MSAA, "MSAA"}, {AntiAliasingMode::SMAA, "SMAA"}, 
                {AntiAliasingMode::TAA, "TAA"}
            };
 
            auto findIt = AAMethodNameLookup.find(aaMethodIndex);
            if (findIt != AAMethodNameLookup.end())
            {
                return findIt->second;
            }
            return "MSAA";
        }

        bool RenderPipeline::EnablePass(RenderPipeline* pipeline, Name& passName, bool enable)
        {
            PassFilter passFilter = PassFilter::CreateWithPassName(passName, pipeline);
            Ptr<Pass> aaPass = PassSystemInterface::Get()->FindFirstPass(passFilter);
            if (!aaPass)
            {
                return false;
            }
            if (aaPass->IsEnabled() != enable)
            {
                aaPass->SetEnabled(enable);
            }
            return true;
        }

        bool RenderPipeline::SetAAMethod(RenderPipeline* pipeline, AZStd::string aaMethodName)
        {
            AntiAliasingMode antiAliasingMode = GetAAMethodByName(aaMethodName);
            return SetAAMethod(pipeline, antiAliasingMode);
        }

        bool RenderPipeline::SetAAMethod(RenderPipeline* pipeline, AntiAliasingMode antiAliasingMode)
        {
            if (antiAliasingMode == AntiAliasingMode::Default)
            {
                return false;
            }

            const AZStd::unordered_map<AntiAliasingMode, AZStd::vector<Name>> AAPassNamesLookup = {
                {AntiAliasingMode::SMAA, {Name("SMAA1xApplyLinearHDRColorPass")}}, 
                {AntiAliasingMode::TAA, {Name("TaaPass"), Name("ContrastAdaptiveSharpeningPass")}}
            };

            for (auto& aaPassMap : AAPassNamesLookup)
            {
                AZStd::for_each(aaPassMap.second.begin(), aaPassMap.second.end(), [&pipeline, &aaPassMap, &antiAliasingMode](Name passName){
                    EnablePass(pipeline, passName, aaPassMap.first == antiAliasingMode);
                });
            }
            return true;
        }
    }
}
