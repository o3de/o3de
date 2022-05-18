/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <Atom/RHI/DrawListTagRegistry.h>

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
#include <Atom/RPI.Public/WindowContext.h>

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

        RenderPipelinePtr RenderPipeline::CreateRenderPipelineForWindow(const RenderPipelineDescriptor& desc, const WindowContext& windowContext)
        {
            RenderPipeline* pipeline = aznew RenderPipeline();
            PassSystemInterface* passSystem = PassSystemInterface::Get();

            PassDescriptor swapChainDescriptor(Name(desc.m_name));
            Name tempalteName = Name(desc.m_rootPassTemplate.c_str());
            swapChainDescriptor.m_passTemplate = passSystem->GetPassTemplate(tempalteName);

            pipeline->m_passTree.m_rootPass = aznew SwapChainPass(swapChainDescriptor, &windowContext);
            pipeline->m_windowHandle = windowContext.GetWindowHandle();

            InitializeRenderPipeline(pipeline, desc);

            return RenderPipelinePtr(pipeline);
        }

        void RenderPipeline::InitializeRenderPipeline(RenderPipeline* pipeline, const RenderPipelineDescriptor& desc)
        {
            pipeline->m_descriptor = desc;
            pipeline->m_mainViewTag = Name(desc.m_mainViewTagName);
            pipeline->m_nameId = desc.m_name.data();
            pipeline->m_activeRenderSettings = desc.m_renderSettings;
            pipeline->m_passTree.m_rootPass->SetRenderPipeline(pipeline);
            pipeline->m_passTree.m_rootPass->m_flags.m_isPipelineRoot = true;
            pipeline->m_passTree.m_rootPass->ManualPipelineBuildAndInitialize();
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
            SortedPipelineViewTags viewTags;
            m_passTree.m_rootPass->GetPipelineViewTags(viewTags);

            // Use a new list for building pipeline views since we may need information from the previous list in m_views in the process
            PipelineViewMap newViewsByTag;

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
        }

        void RenderPipeline::CollectDrawListMaskForViews(PipelineViews& views)
        {
            views.m_drawListMask.reset();
            views.m_passesByDrawList.clear();
            m_passTree.m_rootPass->GetViewDrawListInfo(views.m_drawListMask, views.m_passesByDrawList, views.m_viewTag);
        }

        void RenderPipeline::SetPersistentView(const PipelineViewTag& viewTag, ViewPtr view)
        {
            auto viewItr = m_pipelineViewsByTag.find(viewTag);
            if (viewItr != m_pipelineViewsByTag.end())
            {
                PipelineViews& pipelineViews = viewItr->second;

                if (pipelineViews.m_type == PipelineViewType::Transient)
                {
                    AZ_Assert(false, "View [%s] was set as transient view. Use AddTransientView function to add a view for this tag.", viewTag.GetCStr());
                    return;
                }
                if (pipelineViews.m_type == PipelineViewType::Unknown)
                {
                    pipelineViews.m_type = PipelineViewType::Persistent;
                    pipelineViews.m_views.resize(1);
                }
                ViewPtr previousView = pipelineViews.m_views[0];
                if (view)
                {
                    view->OnAddToRenderPipeline();
                }
                pipelineViews.m_views[0] = view;

                if (previousView)
                {
                    previousView->SetPassesByDrawList(nullptr);
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
            ViewPtr defaultView;
            const AZStd::vector<ViewPtr>& views = GetViews(m_mainViewTag);
            if (!views.empty())
            {
                defaultView = views[0];
            }
            return defaultView;
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

        void RenderPipeline::AddTransientView(const PipelineViewTag& viewTag, ViewPtr view)
        {
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
            }
        }

        bool RenderPipeline::HasViewTag(const PipelineViewTag& viewTag) const
        {
            return m_pipelineViewsByTag.find(viewTag) != m_pipelineViewsByTag.end();
        }

        PipelineViewTag RenderPipeline::GetMainViewTag() const
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
            AZ_Assert(m_scene == scene, "Pipeline isn't added to the specified scene");
            m_scene = nullptr;
            PassSystemInterface::Get()->RemoveRenderPipeline(this);

            m_drawFilterTag.Reset();
            m_drawFilterMask = 0;
        }

        void RenderPipeline::ProcessQueuedPassChanges()
        {
            m_passTree.ProcessQueuedChanges();
        }

        void RenderPipeline::OnPassModified()
        {
            if (m_needsPassRecreate)
            {
                // Process any queued changes before we attempt to reload the pipeline
                m_passTree.ProcessQueuedChanges();

                // Attempt to re-create hierarchy under root pass
                Ptr<ParentPass> newRoot = azrtti_cast<ParentPass*>(m_passTree.m_rootPass->Recreate().get());
                newRoot->SetRenderPipeline(this);
                newRoot->m_flags.m_isPipelineRoot = true;
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

                    m_wasPassModified = true;
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
                m_needsPassRecreate = false;
            }

            if (m_wasPassModified)
            {
                m_passTree.m_rootPass->SetRenderPipeline(this);
                BuildPipelineViews();
                m_wasPassModified = false;
                if (m_scene)
                {
                    SceneNotificationBus::Event(m_scene->GetId(), &SceneNotification::OnRenderPipelinePassesChanged, this);
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

            OnPassModified();

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
                    // Reset persistent view: clean draw list mask and draw lists
                    pipelineViews.m_views[0]->Reset();
                    pipelineViews.m_views[0]->SetPassesByDrawList(&pipelineViews.m_passesByDrawList);
                }
            }
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
            if (GetRenderMode() != RenderPipeline::RenderMode::NoRender)
            {
                m_passTree.m_rootPass->FrameBegin(params);
            }
        }

        void RenderPipeline::PassSystemFrameEnd()
        {
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

        void RenderPipeline::SetPassModified()
        {
            m_wasPassModified = true;
        }

        void RenderPipeline::SetPassNeedsRecreate()
        {
            m_needsPassRecreate = true;
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

        RHI::DrawFilterTag RenderPipeline::GetDrawFilterTag() const
        {
            return m_drawFilterTag;
        }

        RHI::DrawFilterMask RenderPipeline::GetDrawFilterMask() const
        {
            return m_drawFilterMask;
        }

        void RenderPipeline::SetDrawFilterTag(RHI::DrawFilterTag tag)
        {
            m_drawFilterTag = tag;
            if (m_drawFilterTag.IsValid())
            {
                m_drawFilterMask = 1 << tag.GetIndex();
            }
            else
            {
                m_drawFilterMask = 0;
            }
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
    }
}
