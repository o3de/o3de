/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceObject.h>
#include <Atom/RHI/PipelineStateDescriptor.h>
#include <Atom/RHI.Reflect/PipelineLayoutDescriptor.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <RHI/ShaderModule.h>

namespace AZ::WebGPU
{
    class ShaderStageFunction;
    class PipelineLibrary;
    class Device;
    class PipelineLayout;

    class Pipeline
        : public RHI::DeviceObject
    {
        using Base = RHI::DeviceObject;

    public:
        AZ_CLASS_ALLOCATOR(Pipeline, AZ::ThreadPoolAllocator);
        AZ_RTTI(Pipeline, "{A937B5A4-44CE-4038-8237-E13547D873FB}", Base);

        ~Pipeline() = default;

        struct Descriptor
        {
            const RHI::PipelineStateDescriptor* m_pipelineDescritor = nullptr;
            PipelineLibrary* m_pipelineLibrary = nullptr;
        };

        RHI::ResultCode Init(Device& device, const Descriptor& descriptor);

        PipelineLayout* GetPipelineLayout() const;
        PipelineLibrary* GetPipelineLibrary() const;

    protected:
        Pipeline() = default;

        //////////////////////////////////////////////////////////////////////////
        //! Implementation API
        virtual RHI::ResultCode InitInternal(const Descriptor& descriptor, const PipelineLayout& pipelineLayout) = 0;
        virtual RHI::PipelineStateType GetType() = 0;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // RHI::Object
        void SetNameInternal(const AZStd::string_view& name) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // RHI::DeviceObject
        void Shutdown() override;
        //////////////////////////////////////////////////////////////////////////

        ShaderModule* BuildShaderModule(const RHI::ShaderStageFunction* function);

        RHI::Ptr<PipelineLayout> m_pipelineLayout;
        PipelineLibrary* m_pipelineLibrary = nullptr;
        AZStd::vector<RHI::Ptr<ShaderModule>> m_shaderModules;
    };
}
