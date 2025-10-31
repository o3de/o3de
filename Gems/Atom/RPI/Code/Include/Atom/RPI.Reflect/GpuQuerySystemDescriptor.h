/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>

#include <Atom/RPI.Reflect/Configuration.h>
#include <Atom/RHI.Reflect/QueryPoolDescriptor.h>

namespace AZ
{
    class ReflectContext;

    namespace RPI
    {
        //! A descriptor used to create a render pipeline
        struct ATOM_RPI_REFLECT_API GpuQuerySystemDescriptor final
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
