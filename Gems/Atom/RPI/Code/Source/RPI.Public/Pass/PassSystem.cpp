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
#include <AzCore/Debug/EventTrace.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/sort.h>
#include <AzCore/Interface/Interface.h>

#include <AzCore/Serialization/Json/JsonUtils.h>

#include <Atom/RHI/FrameGraphBuilder.h>

#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <Atom/RPI.Public/Pass/PassDefines.h>
#include <Atom/RPI.Public/Pass/PassFactory.h>
#include <Atom/RPI.Public/Pass/PassSystem.h>
#include <Atom/RPI.Public/Pass/PassLibrary.h>
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
            m_rootPass = CreatePass<ParentPass>(Name{"Root"});
            m_rootPass->m_flags.m_partOfHierarchy = true;
            m_rootPass->m_flags.m_createChildren = false;

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

        void PassSystem::QueueForBuild(Pass* pass)
        {
            AZ_Assert(pass != nullptr, "Queuing nullptr pass in PassSystem::QueueForBuild");
            m_buildPassList.push_back(pass);
        }

        void PassSystem::QueueForRemoval(Pass* pass)
        {
            AZ_Assert(pass != nullptr, "Queuing nullptr pass in PassSystem::QueueForRemoval");
            m_removePassList.push_back(pass);
        }

        void PassSystem::QueueForInitialization(Pass* pass)
        {
            AZ_Assert(pass != nullptr, "Queuing nullptr pass in PassSystem::QueueForInitialization");
            m_initializePassList.push_back(pass);
        }

        // Sort so passes with less depth (closer to the root) are first. Used when changes 
        // in the parent passes can affect the child passes, like with attachment building.
        void SortPassListAscending(AZStd::vector< Ptr<Pass> >& passList)
        {
            AZStd::sort(passList.begin(), passList.end(),
                [](const Ptr<Pass>& lhs, const Ptr<Pass>& rhs)
                {
                    return (lhs->GetTreeDepth() < rhs->GetTreeDepth());
                });
        }

        // Sort so passes with greater depth (further from the root) get called first. Used in the case of
        // delete, as we want to avoid deleting the parent first since this invalidates the child pointer.
        void SortPassListDescending(AZStd::vector< Ptr<Pass> >& passList)
        {
            AZStd::sort(passList.begin(), passList.end(),
                [](const Ptr<Pass>& lhs, const Ptr<Pass>& rhs)
                {
                    return (lhs->GetTreeDepth() > rhs->GetTreeDepth());
                }
            );
        }

        void PassSystem::RemovePasses()
        {
            m_state = PassSystemState::RemovingPasses;
            AZ_PROFILE_SCOPE(RPI, "PassSystem: RemovePasses");

            if (!m_removePassList.empty())
            {
                SortPassListDescending(m_removePassList);

                for (const Ptr<Pass>& pass : m_removePassList)
                {
                    pass->RemoveFromParent();
                }

                m_removePassList.clear();
            }

            m_state = PassSystemState::Idle;
        }

        void PassSystem::BuildPasses()
        {
            m_state = PassSystemState::BuildingPasses;
            AZ_PROFILE_SCOPE(RPI, "PassSystem: BuildPasses");

            m_passHierarchyChanged = m_passHierarchyChanged || !m_buildPassList.empty();

            // While loop is for the event in which passes being built add more pass to m_buildPassList
            while(!m_buildPassList.empty())
            {
                AZ_Assert(m_removePassList.empty(), "Passes shouldn't be queued removal during build attachment process");

                AZStd::vector< Ptr<Pass> > buildListCopy = m_buildPassList;
                m_buildPassList.clear();

                // Erase passes which were removed from pass tree already (which parent is empty)
                auto unused = AZStd::remove_if(buildListCopy.begin(), buildListCopy.end(),
                    [](const RHI::Ptr<Pass>& currentPass)
                    {
                        return !currentPass->m_flags.m_partOfHierarchy;
                    });
                buildListCopy.erase(unused, buildListCopy.end());

                SortPassListAscending(buildListCopy);

                for (const Ptr<Pass>& pass : buildListCopy)
                {
                    pass->Reset();
                }
                for (const Ptr<Pass>& pass : buildListCopy)
                {
                    pass->Build(true);
                }
            }

            if (m_passHierarchyChanged)
            {
#if AZ_RPI_ENABLE_PASS_DEBUGGING
                if (!m_isHotReloading)
                {
                    AZ_Printf("PassSystem", "\nFinished building passes:\n");
                    DebugPrintPassHierarchy();
                }
#endif
            }

            m_state = PassSystemState::Idle;
        }

        void PassSystem::InitializePasses()
        {
            m_state = PassSystemState::InitializingPasses;
            AZ_PROFILE_SCOPE(RPI, "PassSystem: InitializePasses");

            m_passHierarchyChanged = m_passHierarchyChanged || !m_initializePassList.empty();

            while (!m_initializePassList.empty())
            {
                AZStd::vector< Ptr<Pass> > initListCopy = m_initializePassList;
                m_initializePassList.clear();

                // Erase passes which were removed from pass tree already (which parent is empty)
                auto unused = AZStd::remove_if(initListCopy.begin(), initListCopy.end(),
                    [](const RHI::Ptr<Pass>& currentPass)
                    {
                        return !currentPass->m_flags.m_partOfHierarchy;
                    });
                initListCopy.erase(unused, initListCopy.end());

                SortPassListAscending(initListCopy);

                for (const Ptr<Pass>& pass : initListCopy)
                {
                    pass->Initialize();
                }
            }

            if (m_passHierarchyChanged)
            {
                // Signal all passes that we have finished initialization
                m_rootPass->OnInitializationFinished();
            }

            m_state = PassSystemState::Idle;
        }

        void PassSystem::Validate()
        {
            m_state = PassSystemState::ValidatingPasses;

            if (PassValidation::IsEnabled())
            {
                if (!m_passHierarchyChanged)
                {
                    return;
                }

                AZ_PROFILE_SCOPE(RPI, "PassSystem: Validate");

                PassValidationResults validationResults;
                m_rootPass->Validate(validationResults);
                validationResults.PrintValidationIfError();
            }

            m_state = PassSystemState::Idle;
        }

        void PassSystem::ProcessQueuedChanges()
        {
            AZ_PROFILE_SCOPE(RPI, "PassSystem: ProcessQueuedChanges");
            RemovePasses();
            BuildPasses();
            InitializePasses();
            Validate();
        }

        void PassSystem::FrameUpdate(RHI::FrameGraphBuilder& frameGraphBuilder)
        {
            AZ_PROFILE_SCOPE(RPI, "PassSystem: FrameUpdate");

            ResetFrameStatistics();
            ProcessQueuedChanges();

            m_state = PassSystemState::Rendering;
            Pass::FramePrepareParams params{ &frameGraphBuilder };

            {
                m_rootPass->FrameBegin(params);
            }
        }

        void PassSystem::FrameEnd()
        {
            AZ_PROFILE_SCOPE(RHI, "PassSystem: FrameEnd");

            m_state = PassSystemState::FrameEnd;

            m_rootPass->FrameEnd();

            // remove any pipelines that are marked as ExecuteOnce
            // the immediate children of m_rootPass are the top-level nodes of each pipeline
            for (const Ptr<Pass>& pass : m_rootPass->GetChildren())
            {
                RenderPipeline* pipeline = pass->m_pipeline;
                if (pipeline && pipeline->IsExecuteOnce())
                {
                    pipeline->RemoveFromScene();
                }
            }

            m_passHierarchyChanged = false;

            m_state = PassSystemState::Idle;
        }

        void PassSystem::Shutdown()
        {
            RemovePasses();
            m_buildPassList.clear();
            m_rootPass = nullptr;
            m_passFactory.Shutdown();
            m_passLibrary.Shutdown();
            AZ_Assert(m_passCounter == 0, "Pass leaking has occurred! There are %d passes that have not been deleted.\n", m_passCounter);
            Interface<PassSystemInterface>::Unregister(this);
        }

        const Ptr<ParentPass>& PassSystem::GetRootPass()
        {
            return m_rootPass;
        }

        PassSystemState PassSystem::GetState() const
        {
            return m_state;
        }

        bool PassSystem::IsHotReloading() const
        {
            return m_isHotReloading;
        }

        void PassSystem::SetHotReloading(bool hotReloading)
        {
            m_isHotReloading = hotReloading;
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

        // --- Pass Factory Functions --- 

        void PassSystem::AddPassCreator(Name className, PassCreator createFunction)
        {
            m_passFactory.AddPassCreator(className, createFunction);
        }

        Ptr<Pass> PassSystem::CreatePassFromClass(Name passClassName, Name passName)
        {
            return m_passFactory.CreatePassFromClass(passClassName, passName);
        }

        Ptr<Pass> PassSystem::CreatePassFromTemplate(const AZStd::shared_ptr<PassTemplate>& passTemplate, Name passName)
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

        bool PassSystem::HasPassesForTemplateName(const Name& templateName) const
        {
            return m_passLibrary.HasPassesForTemplate(templateName);
        }

        bool PassSystem::AddPassTemplate(const Name& name, const AZStd::shared_ptr<PassTemplate>& passTemplate)
        {
            return m_passLibrary.AddPassTemplate(name, passTemplate);
        }

        const AZStd::shared_ptr<PassTemplate> PassSystem::GetPassTemplate(const Name& name) const
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
