/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/PipelineStateDescriptor.h>
#include <Atom/RHI.Reflect/PipelineLayoutDescriptor.h>
#include <Atom/RHI.Reflect/ShaderStageFunction.h>
#include <Atom/RHI.Reflect/InputStreamLayout.h>

namespace AZ::RHI
{
    PipelineStateDescriptor::PipelineStateDescriptor(PipelineStateType pipelineStateType)
        : m_type{pipelineStateType}
    {}

    PipelineStateType PipelineStateDescriptor::GetType() const
    {
        return m_type;
    }

    bool PipelineStateDescriptor::operator == (const PipelineStateDescriptor& rhs) const
    {
        return m_type == rhs.m_type;
    }

    void PipelineStateDescriptor::SetName(const Name& name)
    {
        m_name = name;
    }

    const Name& PipelineStateDescriptor::GetName() const
    {
        return m_name;
    }

    PipelineStateDescriptorForDraw::PipelineStateDescriptorForDraw()
        : PipelineStateDescriptor(PipelineStateType::Draw)
    {}

    PipelineStateDescriptorForDispatch::PipelineStateDescriptorForDispatch()
        : PipelineStateDescriptor(PipelineStateType::Dispatch)
    {}

    PipelineStateDescriptorForRayTracing::PipelineStateDescriptorForRayTracing()
        : PipelineStateDescriptor(PipelineStateType::RayTracing)
    {}

    AZ::HashValue64 PipelineStateDescriptorForDispatch::GetHash() const
    {
        AZ_Assert(m_pipelineLayoutDescriptor, "Pipeline layout descriptor is null.");
        AZ_Assert(m_computeFunction, "Compute function is null.");

        AZ::HashValue64 seed = AZ::HashValue64{ 0 };
        seed = TypeHash64(m_pipelineLayoutDescriptor->GetHash(), seed);
        seed = TypeHash64(m_computeFunction->GetHash(), seed);
        return seed;
    }

    AZ::HashValue64 PipelineStateDescriptorForDraw::GetHash() const
    {
        AZ_Assert(m_pipelineLayoutDescriptor, "m_pipelineLayoutDescriptor is null.");

        AZ::HashValue64 seed = AZ::HashValue64{ 0 };

        if (m_vertexFunction)
        {
            seed = TypeHash64(m_vertexFunction->GetHash(), seed);
        }
        if (m_geometryFunction)
        {
            seed = TypeHash64(m_geometryFunction->GetHash(), seed);
        }
        if (m_fragmentFunction)
        {
            seed = TypeHash64(m_fragmentFunction->GetHash(), seed);
        }

        seed = TypeHash64(m_pipelineLayoutDescriptor->GetHash(), seed);
        seed = TypeHash64(m_inputStreamLayout.GetHash(), seed);
        seed = TypeHash64(m_renderAttachmentConfiguration.GetHash(), seed);

        seed = m_renderStates.GetHash(seed);

        return seed;
    }

    AZ::HashValue64 PipelineStateDescriptorForRayTracing::GetHash() const
    {
        AZ_Assert(m_pipelineLayoutDescriptor, "Pipeline layout descriptor is null.");

        AZ::HashValue64 seed = AZ::HashValue64{ 0 };
        seed = TypeHash64(m_pipelineLayoutDescriptor->GetHash(), seed);
        seed = TypeHash64(m_rayTracingFunction->GetHash(), seed);
        return seed;
    }

    bool PipelineStateDescriptorForDraw::operator == (const PipelineStateDescriptorForDraw& rhs) const
    {
        return m_fragmentFunction == rhs.m_fragmentFunction && m_pipelineLayoutDescriptor == rhs.m_pipelineLayoutDescriptor &&
            m_renderStates == rhs.m_renderStates && m_vertexFunction == rhs.m_vertexFunction &&
            m_geometryFunction == rhs.m_geometryFunction && m_inputStreamLayout == rhs.m_inputStreamLayout && 
            m_renderAttachmentConfiguration == rhs.m_renderAttachmentConfiguration;
    }

    bool PipelineStateDescriptorForDispatch::operator == (const PipelineStateDescriptorForDispatch& rhs) const
    {
        return m_computeFunction == rhs.m_computeFunction &&
            m_pipelineLayoutDescriptor == rhs.m_pipelineLayoutDescriptor;
    }

    bool PipelineStateDescriptorForRayTracing::operator == (const PipelineStateDescriptorForRayTracing& rhs) const
    {
        return m_pipelineLayoutDescriptor == rhs.m_pipelineLayoutDescriptor &&
            m_rayTracingFunction == rhs.m_rayTracingFunction;
    }
} // namespace AZ::RHI
