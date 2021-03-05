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


#include <MorphTargets/MorphTargetComputePass.h>
#include <Atom/Feature/SkinnedMesh/SkinnedMeshOutputStreamManagerInterface.h>

#include <Atom/RPI.Public/Shader/Shader.h>

#include <Atom/RHI/CommandList.h>

namespace AZ
{
    namespace Render
    {
        MorphTargetComputePass::MorphTargetComputePass(const RPI::PassDescriptor& descriptor)
            : RPI::ComputePass(descriptor)
        {
        }

        RPI::Ptr<MorphTargetComputePass> MorphTargetComputePass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<MorphTargetComputePass> pass = aznew MorphTargetComputePass(descriptor);
            return pass;
        }

        Data::Instance<RPI::Shader> MorphTargetComputePass::GetShader() const
        {
            return m_shader;
        }

        void MorphTargetComputePass::BuildAttachmentsInternal()
        {
            // The same buffer that skinning writes to is used to manage the computed vertex deltas that are passed from the
            // morph target pass to the skinning pass. This simplifies things by only requiring one class to manage the memory
            AttachBufferToSlot(Name{ "MorphTargetDeltaOutput" }, SkinnedMeshOutputStreamManagerInterface::Get()->GetBuffer());
        }

        void MorphTargetComputePass::AddDispatchItem(const RHI::DispatchItem* dispatchItem)
        {
            AZ_Assert(dispatchItem != nullptr, "invalid dispatchItem");

            AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
            //using an unordered_set here to prevent redundantly adding the same dispatchItem to the submission queue
            //(i.e. if the same morph target exists in multiple views, it can call AddDispatchItem multiple times with the same item)
            m_dispatches.insert(dispatchItem);
        }

        void MorphTargetComputePass::BuildCommandList(const RHI::FrameGraphExecuteContext& context, [[maybe_unused]] const RPI::PassScopeProducer& producer)
        {
            RHI::CommandList* commandList = context.GetCommandList();

            SetSrgsForDispatch(commandList);

            AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
            for (const RHI::DispatchItem* dispatchItem : m_dispatches)
            {
                commandList->Submit(*dispatchItem);
            }

            // Clear the dispatch items. They will need to be re-populated next frame
            m_dispatches.clear();
        }
    }   // namespace Render
}   // namespace AZ
