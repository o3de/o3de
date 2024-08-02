/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/InputStreamLayout.h>
#include <Atom/RHI.Reflect/RenderAttachmentLayout.h>
#include <Atom/RHI.Reflect/RenderStates.h>
#include <Atom/RHI.Reflect/ShaderStageFunction.h>
#include <AzCore/Utils/TypeHash.h>
#include <Atom/RHI.Reflect/PipelineLayoutDescriptor.h>
#include <Atom/RHI/SpecializationConstant.h>

namespace AZ::RHI
{
    enum class PipelineStateType : uint32_t
    {
        Draw = 0,
        Dispatch,
        RayTracing,
        Count
    };

    const uint32_t PipelineStateTypeCount = static_cast<uint32_t>(PipelineStateType::Count);

    //! A base class to pipeline state descriptor.
    class PipelineStateDescriptor
    {
    public:
        AZ_RTTI(PipelineStateDescriptor, "{B334AE47-53CB-438C-B799-DCA542FF8D5D}");

        virtual ~PipelineStateDescriptor() = default;

        //! Returns the derived pipeline state type.
        PipelineStateType GetType() const;

        //! Returns the hash of the pipeline state descriptor contents.
        HashValue64 GetHash() const;

        bool operator == (const PipelineStateDescriptor& rhs) const;

        //! The pipeline layout describing the shader resource bindings.
        ConstPtr<PipelineLayoutDescriptor> m_pipelineLayoutDescriptor = nullptr;

        //! Values for specialization constants.
        AZStd::vector<SpecializationConstant> m_specializationData;

    protected:
        PipelineStateDescriptor(PipelineStateType pipelineStateType);

        virtual HashValue64 GetHashInternal() const = 0;

    private:
        PipelineStateType m_type = PipelineStateType::Count;
    };

    //! Describes state necessary to build a compute pipeline state object. The compute pipe
    //! requires a pipeline layout and the shader byte code descriptor. Call Finalize after
    //! assigning data to build the hash value.
    //!
    //! NOTE: This class does not serialize. This is by design. the pipeline layout and shader
    //! byte code is likely shared by many PSOs and the serialization system would simply duplicate
    //! all of that data. However, the individual pieces are serializable, so a higher-level system
    //! could easily construct a PSO library.
    class PipelineStateDescriptorForDispatch final
        : public PipelineStateDescriptor
    {
    public:
        AZ_RTTI(PipelineStateDescriptorForDispatch, "{78E775AD-DCA8-4408-9A42-CF78DEB1E640}", PipelineStateDescriptor);

        PipelineStateDescriptorForDispatch();

        /// Computes the hash value for this descriptor.
        HashValue64 GetHashInternal() const override;

        bool operator == (const PipelineStateDescriptorForDispatch& rhs) const;

        /// The compute function containing byte code to compile.
        ConstPtr<ShaderStageFunction> m_computeFunction;
    };

    //! Describes state necessary to build a graphics pipeline state object (PSO). The graphics pipe
    //! requires a pipeline layout and the shader byte code descriptor, as well as the fixed-function
    //! input assembly stream layout, render target attachment layout, and various render states.
    //!
    //! NOTE: This class does not serialize by design. See PipelineStateDescriptorForDispatch for details.
    class PipelineStateDescriptorForDraw final
        : public PipelineStateDescriptor
    {
    public:
        AZ_RTTI(PipelineStateDescriptorForDraw, "{7121C45A-8102-42CE-827D-AB5199701CB2}", PipelineStateDescriptor);

        PipelineStateDescriptorForDraw();

        /// Computes the hash value for this descriptor.
        HashValue64 GetHashInternal() const override;

        bool operator == (const PipelineStateDescriptorForDraw& rhs) const;

        /// [Required] The vertex function to compile.
        ConstPtr<ShaderStageFunction> m_vertexFunction;

        /// [Optional] The geometry function to compile.
        ConstPtr<ShaderStageFunction> m_geometryFunction;

        /// [Required] The fragment function used to compile.
        ConstPtr<ShaderStageFunction> m_fragmentFunction;

        /// The input assembly vertex stream layout for the pipeline.
        InputStreamLayout m_inputStreamLayout;

        /// The render target configuration for the pipeline.
        RenderAttachmentConfiguration m_renderAttachmentConfiguration;

        /// Various render states for the pipeline.
        RenderStates m_renderStates;
    };

    //! Describes state necessary to build a ray tracing pipeline state object. 
    class PipelineStateDescriptorForRayTracing final
        : public PipelineStateDescriptor
    {
    public:
        AZ_RTTI(PipelineStateDescriptorForRayTracing, "{1B55AD28-A56E-4BCD-92EE-22C2F89ABBE5}", PipelineStateDescriptor);

        PipelineStateDescriptorForRayTracing();

        //! Computes the hash value for this descriptor.
        HashValue64 GetHashInternal() const override;

        bool operator == (const PipelineStateDescriptorForRayTracing& rhs) const;

        // The ray tracing shader byte code
        ConstPtr<ShaderStageFunction> m_rayTracingFunction;
    };
}
