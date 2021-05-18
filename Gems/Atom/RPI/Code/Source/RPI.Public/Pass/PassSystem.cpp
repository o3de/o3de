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

#include <AtomCore/Serialization/Json/JsonUtils.h>

#include <Atom/RHI/CpuProfiler.h>
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
            Interface<PassSystemInterface>::Register(this);
            m_passLibrary.Init();
            m_passFactory.Init(&m_passLibrary);
            m_rootPass = CreatePass<ParentPass>(Name{"Root"});
            m_rootPass->m_flags.m_partOfHierarchy = true;
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

        void PassSystem::QueueForBuildAttachments(Pass* pass)
        {
            AZ_Assert(pass != nullptr, "Queuing nullptr pass in PassSystem::QueueForBuildAttachments");
            m_buildAttachmentsList.push_back(pass);
        }

        void PassSystem::QueueForRemoval(Pass* pass)
        {
            AZ_Assert(pass != nullptr, "Queuing nullptr pass in PassSystem::QueueForRemoval");
            m_removePassList.push_back(pass);
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
            AZ_ATOM_PROFILE_FUNCTION("RPI", "PassSystem: RemovePasses");

            if (!m_removePassList.empty())
            {
                SortPassListDescending(m_removePassList);

                for (const Ptr<Pass>& pass : m_removePassList)
                {
                    pass->RemoveFromParent();
                }

                m_removePassList.clear();
            }
        }

        void PassSystem::BuildPassAttachments()
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzRender);
            AZ_ATOM_PROFILE_FUNCTION("RPI", "PassSystem: BuildPassAttachments");

            m_isBuilding = true;

            m_passHierarchyChanged = !m_buildAttachmentsList.empty();

            // While loop is for the event in which passes being built add more pass to m_buildAttachmentsList
            while(!m_buildAttachmentsList.empty())
            {
                AZ_Assert(m_removePassList.empty(), "Passes shouldn't be queued removal during build attachment process");

                AZStd::vector< Ptr<Pass> > buildListCopy = m_buildAttachmentsList;
                m_buildAttachmentsList.clear();

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
                    pass->BuildAttachments();
                }

                // Signal all passes that we have finished building
                m_rootPass->OnBuildAttachmentsFinished();
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

            m_isBuilding = false;
        }

        void PassSystem::Validate()
        {
            AZ_ATOM_PROFILE_FUNCTION("RPI", "PassSystem: Validate");

            if (PassValidation::IsEnabled())
            {
                if (!m_passHierarchyChanged)
                {
                    return;
                }

                AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzRender);

                PassValidationResults validationResults;
                m_rootPass->Validate(validationResults);
                validationResults.PrintValidationIfError();
            }
        }

        void PassSystem::ProcessQueuedChanges()
        {
            RemovePasses();
            BuildPassAttachments();
            Validate();
        }

        void PassSystem::FrameUpdate(RHI::FrameGraphBuilder& frameGraphBuilder)
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzRender);
            AZ_ATOM_PROFILE_FUNCTION("RPI", "PassSystem: FrameUpdate");

            ProcessQueuedChanges();
            Pass::FramePrepareParams params{ &frameGraphBuilder };
            m_rootPass->FrameBegin(params);
        }

        void PassSystem::FrameEnd()
        {
            AZ_ATOM_PROFILE_FUNCTION("RHI", "PassSystem: FrameEnd");

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
        }

        void PassSystem::Shutdown()
        {
            RemovePasses();
            m_buildAttachmentsList.clear();
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

        bool PassSystem::IsBuilding() const
        {
            return m_isBuilding;
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

        const AZStd::vector<Pass*>& PassSystem::GetPassesForTemplateName(const Name& templateName) const
        {
            return m_passLibrary.GetPassesForTemplate(templateName);
        }

        bool PassSystem::AddPassTemplate(const Name& name, const AZStd::shared_ptr<PassTemplate>& passTemplate)
        {
            return m_passLibrary.AddPassTemplate(name, passTemplate);
        }

        const AZStd::shared_ptr<PassTemplate> PassSystem::GetPassTemplate(const Name& name) const
        {
            return m_passLibrary.GetPassTemplate(name);
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

        AZStd::vector<Pass*> PassSystem::FindPasses(const PassFilter& passFilter) const
        {
            return m_passLibrary.FindPasses(passFilter);
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
