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

#include <AzCore/RTTI/RTTI.h>

#include <Atom/RHI.Reflect/QueryPoolDescriptor.h>

namespace AZ
{
    class ReflectContext;

    namespace RPI
    {
        //! A descriptor used to create a render pipeline
        struct GpuQuerySystemDescriptor final
        {
            AZ_TYPE_INFO(GpuQuerySystemDescriptor, "{952FE9DA-96EA-473E-BED5-1B12088BABD0}");
            static void Reflect(AZ::ReflectContext* context);

            uint32_t m_occlusionQueryCount = 128u;
            uint32_t m_statisticsQueryCount = 256u;
            uint32_t m_timestampQueryCount = 256u;

            RHI::PipelineStatisticsFlags m_statisticsQueryFlags =
                RHI::PipelineStatisticsFlags::IAVertices    | RHI::PipelineStatisticsFlags::IAPrimitives    |
                RHI::PipelineStatisticsFlags::VSInvocations | RHI::PipelineStatisticsFlags::CInvocations    |
                RHI::PipelineStatisticsFlags::CPrimitives   | RHI::PipelineStatisticsFlags::PSInvocations   |
                RHI::PipelineStatisticsFlags::CSInvocations;
        }; 

    }; // namespace RPI 
}; //namespace AZ
