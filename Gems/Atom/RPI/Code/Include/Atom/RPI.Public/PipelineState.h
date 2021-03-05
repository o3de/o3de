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

#include <Atom/RHI/PipelineStateDescriptor.h>
#include <Atom/RHI/PipelineState.h>

#include <Atom/RPI.Public/Pass/RenderPass.h>
#include <Atom/RPI.Public/Shader/Shader.h>

namespace AZ
{
    namespace RPI
    {
        //! The PipelineStateForDraw caches descriptor for RHI::PipelineState's creation so the RHI::PipelineState can be created
        //! or updated later when Scene's render pipelines changed or any other data in the descriptor has changed.
        class PipelineStateForDraw
            : public AZStd::intrusive_base
        {
        public:
            AZ_CLASS_ALLOCATOR(PipelineStateForDraw, SystemAllocator, 0);

            PipelineStateForDraw();
            PipelineStateForDraw(const PipelineStateForDraw& right);
            ~PipelineStateForDraw() = default;

            //! Initialize the pipeline state from a shader and one of its shader variant
            //! The previous data will be reset
            void Init(const Data::Instance<Shader>& shader, const AZStd::vector<AZStd::pair<Name, Name>>* optionAndValues = nullptr);

            //! Update the pipeline state descriptor for the specified scene
            //! This is usually called when Scene's render pipelines changed
            void SetOutputFromScene(const Scene* scene, RHI::DrawListTag overrideDrawListTag = RHI::DrawListTag());

            void SetOutputFromPass(RenderPass* pass);

            //! Set input stream
            void SetInputStreamLayout(const RHI::InputStreamLayout& inputStreamLayout);

            //! Re-create the RHI::PipelineState for the input Scene.
            //! The created RHI::PipelineState will be cached and can be acquired by using GetPipelineState()
            const RHI::PipelineState* Finalize();

            //! Get cached RHI::PipelineState.
            //! It triggers an assert if the pipeline state is dirty.
            const RHI::PipelineState* GetRHIPipelineState() const;

            //! Return the reference of the pipeline state descriptor which can be used to modify the descriptor directly.
            //! It sets this pipeline state to dirty when it's called regardless user modify the descriptor or not.
            RHI::PipelineStateDescriptorForDraw& Descriptor();

            const RHI::PipelineStateDescriptorForDraw& ConstDescriptor() const;

        private:
            RHI::PipelineStateDescriptorForDraw m_descriptor;

            Data::Instance<RPI::Shader> m_shader;
            const RHI::PipelineState* m_pipelineState = nullptr;
            
            bool m_initDataFromShader : 1;      // whether it's initialized from a shader
            bool m_hasOutputData : 1;        // whether it has the data from the scene
            bool m_dirty : 1;                   // whether descriptor is dirty
        };
    }
}
