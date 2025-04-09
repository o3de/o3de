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
#include <AzCore/Name/Name.h>
#include <AzCore/RTTI/TypeInfoSimple.h>
#include <AzCore/std/containers/vector.h>

namespace AZ
{
    class ReflectContext;

    namespace RPI
    {
        //! Specifies a connection that will be pointed to by the pipeline for global reference
        struct ATOM_RPI_REFLECT_API PipelineGlobalConnection final
        {
            AZ_TYPE_INFO(PipelineGlobalConnection, "{8D708E59-E94C-4B25-8B0A-5D890BC8E6FE}");

            static void Reflect(ReflectContext* context);

            //! The pipeline global name that other passes can use to reference the connection in a global way
            Name m_globalName;

            //! Name of the local binding on the pass to expose at a pipeline level for reference in a global way
            Name m_localBinding;
        };

        using PipelineGlobalConnectionList = AZStd::vector<PipelineGlobalConnection>;

        //! Base class for custom data for Passes to be specified in a PassRequest or PassTemplate.
        //! If custom data is specified in both the PassTemplate and the PassRequest, the data
        //! specified in the PassRequest will take precedent and the data in PassTemplate ignored.
        //! All classes for custom pass data must inherit from this or one of it's derived classes.
        struct ATOM_RPI_REFLECT_API PassData
        {
            AZ_RTTI(PassData, "{F8594AE8-2588-4D64-89E5-B078A46A9AE4}");
            AZ_CLASS_ALLOCATOR(PassData, SystemAllocator);

            PassData() = default;
            virtual ~PassData() = default;

            static void Reflect(ReflectContext* context);

            // Specifies global pipeline connections to the pipeline's immediate child passes
            PipelineGlobalConnectionList m_pipelineGlobalConnections;

            Name m_pipelineViewTag;

            int m_deviceIndex = RHI::MultiDevice::InvalidDeviceIndex;


            //! Only applicable for ParentPass.
            //! If set to "true" then:
            //! 0- You may get performance benefits if the GPU is a Tiled Based Rasterizer and the RHI supports TBR (like Vulkan).
            //!    This is typically the case for Mobile and XR platforms.
            //! 1- All Child passes must be subclass of RenderPass and return true to CanBecomeSubpass().
            //! 2- The Child passes will be considered as mergeable into sequential subpasses.
            bool m_mergeChildrenAsSubpasses = false;

            //! If the pass can be used as a subpass.
            bool m_canBecomeASubpass = true;
        };
    } // namespace RPI
} // namespace AZ

