/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Interface/Interface.h>

#include <AzCore/Serialization/Json/JsonUtils.h>

#include <Atom/RHI/FrameGraphBuilder.h>

#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <Atom/RPI.Public/Pass/PassDefines.h>
#include <Atom/RPI.Public/Pass/PassFactory.h>
#include <Atom/RPI.Public/Pass/PassLibrary.h>
#include <Atom/RPI.Public/Pass/PassSystem.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/Pass/Specific/SwapChainPass.h>
#include <Atom/RPI.Public/RenderPipeline.h>

#include <Atom/RPI.Reflect/Pass/ComputePassData.h>
#include <Atom/RPI.Reflect/Pass/CopyPassData.h>
#include <Atom/RPI.Reflect/Pass/DownsampleMipChainPassData.h>
#include <Atom/RPI.Reflect/Pass/FullscreenTrianglePassData.h>
#include <Atom/RPI.Reflect/Pass/EnvironmentCubeMapPassData.h>
#include <Atom/RPI.Reflect/Pass/RenderToTexturePassData.h>
#include <Atom/RPI.Reflect/Pass/PassAsset.h>
#include <Atom/RPI.Reflect/Pass/PassData.h>
#include <Atom/RPI.Reflect/Pass/PassTemplate.h>
#include <Atom/RPI.Reflect/Pass/RasterPassData.h>
#include <Atom/RPI.Reflect/Pass/RenderPassData.h>
#include <Atom/RPI.Reflect/Pass/SlowClearPassData.h>


namespace AZ
{
    namespace RPI
    {

        PassSystemInterface* PassSystemInterface::Get()
        {
            return Interface<PassSystemInterface>::Get();
        }

        PassSystem::PassSystem()
        {
        }

        void PassSystem::Reflect(AZ::ReflectContext* context)
        {
            PassAttachmentRef::Reflect(context);
            PassConnection::Reflect(context);
            PassFallbackConnection::Reflect(context);
            PassAttachmentSizeMultipliers::Reflect(context);
            PassAttachmentSizeSource::Reflect(context);
            PassAttachmentDesc::Reflect(context);
            PassImageAttachmentDesc::Reflect(context);
            PassBufferAttachmentDesc::Reflect(context);
            PassSlot::Reflect(context);

            PassData::Reflect(context);
            SlowClearPassData::Reflect(context);
            CopyPassData::Reflect(context);
            RenderPassData::Reflect(context);
            ComputePassData::Reflect(context);
            DownsampleMipChainPassData::Reflect(context);
            RasterPassData::Reflect(context);
            FullscreenTrianglePassData::Reflect(context);
            EnvironmentCubeMapPassData::Reflect(context);
            RenderToTexturePassData::Reflect(context);

            PassAsset::Reflect(context);
            PassTemplate::Reflect(context);
            PassRequest::Reflect(context);
        }

        void PassSystem::GetAssetHandlers(AssetHandlerPtrList& assetHandlers)
        {
            assetHandlers.emplace_back(MakeAssetHandler<PassAssetHandler>());
        }

        void PassSystem::Init()
        {
            m_state = PassSystemState::InitializingPassSystem;

            Interface<PassSystemInterface>::Register(this);
            m_passLibrary.Init();
            m_passFactory.Init(&m_passLibrary);

            // Build root pass
            m_rootPass = CreatePass<ParentPass>(Name{"Root"});
            m_rootPass->m_flags.m_partOfHierarchy = true;
            m_rootPass->m_flags.m_createChildren = false;

            // Manually clear pass list and build root pass since it is subject to enqueing expections
            m_passesWithoutPipeline.m_buildPassList.clear();
            m_rootPass->Build();
            m_rootPass->Initialize();
            m_rootPass->OnInitializationFinished();

            // Build root pass for PassesWithoutPipeline collection
            m_passesWithoutPipeline.m_rootPass = CreatePass<ParentPass>(Name{ "PassesWithoutPipeline" });
            m_passesWithoutPipeline.m_rootPass->m_flags.m_createChildren = false;
            m_rootPass->AddChild(m_passesWithoutPipeline.m_rootPass);

            ProcessQueuedChanges();

            // Here you can specify the name of a pass you would like to break into during execution
            // If you enable AZ_RPI_ENABLE_PASS_DEBUGGING, then any pass matching the specified name will debug
            // break on any instance of the AZ_RPI_BREAK_ON_TARGET_PASS macro. See Pass::Build for an example
            // m_targetedPassDebugName = "MyPassName";

            m_state = PassSystemState::Idle;
        }

        void PassSystem::InitPassTemplates()
        {
            AZ_Assert(m_rootPass, "PassSystem::Init() need to be called");
            m_loadTemplatesEvent.Signal();
        }

        bool PassSystem::LoadPassTemplateMappings(const AZStd::string& templateMappingPath)
        {
            return m_passLibrary.LoadPassTemplateMappings(templateMappingPath);
        }

        void PassSystem::WriteTemplateToFile(const PassTemplate& passTemplate, AZStd::string_view assetFilePath)
        {
            PassAsset passAsset;
            passAsset.m_passTemplate = passTemplate.CloneUnique();
            JsonSerializationUtils::SaveObjectToFile(&passAsset, assetFilePath);
        }

        // --- Queue Functions ---

        void PassSystem::QueueForBuild(Pass* pass)
        {
            AZ_Assert(pass != nullptr, "Queuing nullptr pass in PassSystem::QueueForBuild");
            if (pass == m_rootPass.get())
            {
                return;
            }
            else if (pass->GetRenderPipeline())
            {
                pass->GetRenderPipeline()->m_passTree.m_buildPassList.push_back(pass);
            }
            else
            {
                m_passesWithoutPipeline.m_buildPassList.push_back(pass);
            }
        }

        void PassSystem::QueueForRemoval(Pass* pass)
        {
            AZ_Assert(pass != nullptr, "Queuing nullptr pass in PassSystem::QueueForRemoval");
            if (pass == m_rootPass.get())
            {
                return;
            }
            else if (pass->GetRenderPipeline())
            {
                pass->GetRenderPipeline()->m_passTree.m_removePassList.push_back(pass);
            }
            else
            {
                m_passesWithoutPipeline.m_removePassList.push_back(pass);
            }
        }

        void PassSystem::QueueForInitialization(Pass* pass)
        {
            AZ_Assert(pass != nullptr, "Queuing nullptr pass in PassSystem::QueueForInitialization");
            if (pass == m_rootPass.get())
            {
                return;
            }
            else if (pass->GetRenderPipeline())
            {
                pass->GetRenderPipeline()->m_passTree.m_initializePassList.push_back(pass);
            }
            else
            {
                m_passesWithoutPipeline.m_initializePassList.push_back(pass);
            }
        }

        // --- Frame Update Functions --- 

        void PassSystem::ProcessQueuedChanges()
        {
            AZ_PROFILE_SCOPE(RPI, "PassSystem: ProcessQueuedChanges");

            // Erase any passes with pipelines from the passes without pipeline container
            m_passesWithoutPipeline.EraseFromLists([](const RHI::Ptr<Pass>& currentPass)
                {
                    return (currentPass->m_pipeline != nullptr);
                });

            // Process passes that don't have a pipeline
            m_passesWithoutPipeline.ProcessQueuedChanges();
        }

        void PassSystem::FrameUpdate(RHI::FrameGraphBuilder& frameGraphBuilder)
        {
            AZ_PROFILE_FUNCTION(RPI);

            ResetFrameStatistics();
            ProcessQueuedChanges();

            m_state = PassSystemState::Rendering;
            Pass::FramePrepareParams params{ &frameGraphBuilder };

            for (RenderPipeline*& pipeline : m_renderPipelines)
            {
                pipeline->PassSystemFrameBegin(params);
            }
            m_passesWithoutPipeline.m_rootPass->UpdateConnectedBindings();
            m_passesWithoutPipeline.m_rootPass->FrameBegin(params);
        }

        void PassSystem::FrameEnd()
        {
            AZ_PROFILE_FUNCTION(RPI);

            m_state = PassSystemState::FrameEnd;

            for (RenderPipeline*& pipeline : m_renderPipelines)
            {
                pipeline->PassSystemFrameEnd();
            }
            m_passesWithoutPipeline.m_rootPass->FrameEnd();

            // Copy list of render pipelines because some pipelines might be removed in next loop execution
            AZStd::vector< RenderPipeline* > renderPipelinesCopy = m_renderPipelines;

            // Remove any pipelines that are marked as ExecuteOnce
            for (RenderPipeline*& pipeline : renderPipelinesCopy)
            {
                if (pipeline && pipeline->IsExecuteOnce())
                {
                    pipeline->RemoveFromScene();
                }
            }

            m_state = PassSystemState::Idle;
        }

        // --- Misc --- 

        void PassSystem::Shutdown()
        {
            m_passesWithoutPipeline.ClearQueues();
            m_passesWithoutPipeline.m_rootPass = nullptr;
            m_rootPass = nullptr;

            AZ_Assert(m_passCounter == 0, "Pass leaking has occurred! There are %d passes that have not been deleted.\n", m_passCounter);

            m_passFactory.Shutdown();
            m_passLibrary.Shutdown();

            Interface<PassSystemInterface>::Unregister(this);
        }

        const Ptr<ParentPass>& PassSystem::GetRootPass()
        {
            return m_rootPass;
        }

        void PassSystem::AddRenderPipeline(RenderPipeline* renderPipeline)
        {
            m_renderPipelines.push_back(renderPipeline);
            m_rootPass->AddChild(renderPipeline->m_passTree.m_rootPass);
        }

        void PassSystem::RemoveRenderPipeline(RenderPipeline* renderPipeline)
        {
            renderPipeline->m_passTree.ProcessQueuedChanges();
            renderPipeline->m_passTree.m_rootPass->SetEnabled(false);
            renderPipeline->m_passTree.m_rootPass->QueueForRemoval();
            renderPipeline->m_passTree.ProcessQueuedChanges();

            erase(m_renderPipelines, renderPipeline);
        }

        void PassSystem::AddPassWithoutPipeline(const Ptr<Pass>& pass)
        {
            m_passesWithoutPipeline.m_rootPass->AddChild(pass);
        }

        PassSystemState PassSystem::GetState() const
        {
            return m_state;
        }

        void PassSystem::DebugPrintPassHierarchy()
        {
            AZ_Printf("PassSystem", "\n------- PASS HIERARCHY -------\n");
            m_rootPass->DebugPrint();
            AZ_Printf("PassSystem", "\n------------------------------\n");
        }

        void PassSystem::SetTargetedPassDebuggingName(const AZ::Name& targetPassName)
        {
            m_targetedPassDebugName = targetPassName;
        }

        const AZ::Name& PassSystem::GetTargetedPassDebuggingName() const
        {
            return m_targetedPassDebugName;
        }

        void PassSystem::ConnectEvent(OnReadyLoadTemplatesEvent::Handler& handler)
        {
            handler.Connect(m_loadTemplatesEvent);
        }

        void PassSystem::ResetFrameStatistics()
        {
            m_frameStatistics.m_numRenderPassesExecuted = 0;
            m_frameStatistics.m_totalDrawItemsRendered = 0;
            m_frameStatistics.m_maxDrawItemsRenderedInAPass = 0;
        }

        PassSystemFrameStatistics PassSystem::GetFrameStatistics()
        {
            return m_frameStatistics;
        }

        void PassSystem::IncrementFrameDrawItemCount(u32 numDrawItems)
        {
            m_frameStatistics.m_totalDrawItemsRendered += numDrawItems;
            m_frameStatistics.m_maxDrawItemsRenderedInAPass = AZStd::max(m_frameStatistics.m_maxDrawItemsRenderedInAPass, numDrawItems);
        }

        void PassSystem::IncrementFrameRenderPassCount()
        {
            ++m_frameStatistics.m_numRenderPassesExecuted;
        }

        void PassSystem::DebugBreakOnPass(const Pass* pass) const
        {
            // Users can leverage this function and customize it's logic to facilitate their own debugging
            // However, any customization should be reverted and never submitted. The default logic just checks
            // the pass's name against the TargetedPassDebuggingName

            if (!pass->GetName().IsEmpty() && pass->GetName() == GetTargetedPassDebuggingName())
            {
                AZ::Debug::Trace::Instance().Break();
            }
        }

        // --- Pass Factory Functions --- 

        void PassSystem::AddPassCreator(Name className, PassCreator createFunction)
        {
            m_passFactory.AddPassCreator(className, createFunction);
        }

        Ptr<Pass> PassSystem::CreatePassFromClass(Name passClassName, Name passName)
        {
            return m_passFactory.CreatePassFromClass(passClassName, passName);
        }

        Ptr<Pass> PassSystem::CreatePassFromTemplate(const AZStd::shared_ptr<const PassTemplate>& passTemplate, Name passName)
        {
            return m_passFactory.CreatePassFromTemplate(passTemplate, passName);
        }

        Ptr<Pass> PassSystem::CreatePassFromTemplate(Name templateName, Name passName)
        {
            return m_passFactory.CreatePassFromTemplate(templateName, passName);
        }

        Ptr<Pass> PassSystem::CreatePassFromRequest(const PassRequest* passRequest)
        {
            return m_passFactory.CreatePassFromRequest(passRequest);
        }

        bool PassSystem::HasCreatorForClass(Name passClassName)
        {
            return m_passFactory.HasCreatorForClass(passClassName);
        }

        // --- Pass Library Functions --- 

        bool PassSystem::HasTemplate(const Name& templateName) const
        {
            return m_passLibrary.HasTemplate(templateName);
        }

        bool PassSystem::HasPassesForTemplateName(const Name& templateName) const
        {
            return m_passLibrary.HasPassesForTemplate(templateName);
        }

        bool PassSystem::AddPassTemplate(const Name& name, const AZStd::shared_ptr<PassTemplate>& passTemplate)
        {
            return m_passLibrary.AddPassTemplate(name, passTemplate);
        }

        const AZStd::shared_ptr<const PassTemplate> PassSystem::GetPassTemplate(const Name& name) const
        {
            return m_passLibrary.GetPassTemplate(name);
        }

        void PassSystem::RemovePassTemplate(const Name& name)
        {
            m_passLibrary.RemovePassTemplate(name);
        }

        void PassSystem::RemovePassFromLibrary(Pass* pass)
        {
            m_passLibrary.RemovePassFromLibrary(pass);
        }

        void PassSystem::RegisterPass(Pass* pass)
        {
            ++m_passCounter;
            m_passLibrary.AddPass(pass);
        }

        void PassSystem::UnregisterPass(Pass* pass)
        {
            RemovePassFromLibrary(pass);
            --m_passCounter;
        }
                
        void PassSystem::ForEachPass(const PassFilter& filter, AZStd::function<PassFilterExecutionFlow(Pass*)> passFunction)
        {
            return m_passLibrary.ForEachPass(filter, passFunction);
        }

        Pass* PassSystem::FindFirstPass(const PassFilter& filter)
        {
            Pass* foundPass = nullptr;
            m_passLibrary.ForEachPass(filter, [&foundPass](RPI::Pass* pass) ->PassFilterExecutionFlow
                {
                    foundPass = pass;
                    return PassFilterExecutionFlow::StopVisitingPasses;
                });
            return foundPass;
        }

        SwapChainPass* PassSystem::FindSwapChainPass(AzFramework::NativeWindowHandle windowHandle) const
        {
            for (const Ptr<Pass>& pass : m_rootPass->m_children)
            {
                SwapChainPass* swapChainPass = azrtti_cast<SwapChainPass*>(pass.get());
                if (swapChainPass && swapChainPass->GetWindowHandle() == windowHandle)
                {
                    return swapChainPass;
                }
            }
            return nullptr;
        }

    }   // namespace RPI
}   // namespace AZ
