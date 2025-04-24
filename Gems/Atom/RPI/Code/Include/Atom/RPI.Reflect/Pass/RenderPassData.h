/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Reflect/Configuration.h>
#include <Atom/RHI.Reflect/Limits.h>
#include <Atom/RHI.Reflect/ShaderDataMappings.h>
#include <Atom/RPI.Reflect/Pass/PassData.h>

namespace AZ
{
    namespace RPI
    {
        //! Custom data for the RenderPass. Should be specified in the PassRequest.
        struct ATOM_RPI_REFLECT_API RenderPassData
            : public PassData
        {
            AZ_RTTI(RenderPassData, "{37DE2402-5BAA-48E5-AAC5-3625DFC06BD6}", PassData);
            AZ_CLASS_ALLOCATOR(RenderPassData, SystemAllocator);

            RenderPassData() = default;
            virtual ~RenderPassData() = default;

            static void Reflect(ReflectContext* context);

            //! A grouping of values and value names used to bind data to the per-pass shader resource groups.
            RHI::ShaderDataMappings m_mappings;

            bool m_bindViewSrg = false;
        };
    } // namespace RPI
} // namespace AZ

