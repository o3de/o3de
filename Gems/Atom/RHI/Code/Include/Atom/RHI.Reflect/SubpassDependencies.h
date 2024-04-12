/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/ScopeId.h>
#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <Atom/RHI.Reflect/Handle.h>
#include <Atom/RHI/ResourcePool.h>
#include <Atom/RHI/QueryPool.h>
#include <Atom/RHI/Fence.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/array.h>

namespace AZ::RHI
{
    //! This class serves as an opaque container for RHI specific data related with Subpass Dependencies.
    //! Being an opaque handle, it can be freely used and passed around within the RPI, typically wrapped
    //! around an AZstd::shared_ptr.
    //! 
    //! Background History:
    //! Vulkan requires a valid VkRenderPass when creating a Pipeline State Object for any given shader.
    //! PSO creation usually occurs way earlier than when the RHI creates a VkRenderPass for RHI::Scopes.
    //! Vulkan "only" requires that both VkRenderPasses to be compatible, but they don't have to be same.
    //! 
    //! Pseudo runtime example with Two INDEPENDENT VkRenderPasses:
    //! At time 1, during RPI::RasterPass Initialization:
    //!     Create dummy "VkRenderPass_A" for a shader named "PSO_A".
    //!     Create "PSO_A" using "VkRenderPass_A".
    //! At time 2, during another RPI::RasterPass Initialization:
    //!     Create dummy "VkRenderPass_B" for a shader named "PSO_B".
    //!     Create "PSO_B" using "VkRenderPass_B".
    //! At time 3, in the future, when the RHI builds the Frame Graph:
    //!     Create Scope_A with "VkRenderPass_C"
    //!     Create Scope_B with "VkRenderPass_D"
    //! At time 4, when the RHI submits commands
    //!     VkCmdBeginRenderPass (VkRenderPass_C)
    //!         VkCmdBindPipeline(PSO_A that was created with VkRenderPass_A)  // VkRenderPass_A must be "compatible" with VkRenderPass_C.
    //!         VkCmdDraw(...)
    //!     VkCmdEndRenderPass (VkRenderPass_C)
    //!     VkCmdBeginRenderPass (VkRenderPass_D)
    //!         VkCmdBindPipeline(PSO_B that was created with VkRenderPass_B) // VkRenderPass_D must be "compatible" with VkRenderPass_B.
    //!         VkCmdDraw(...)
    //!     VkCmdEndRenderPass (VkRenderPass_D)
    //! In the example above, because it is NOT using subpasses, it is relatively easy to make compatible VkRenderPasses.
    //! Also, this abstract class did NOT exist and WAS NOT necessario in that scenario.
    //!
    //! Subpasses is the mechanism by which Vulkan provides programmable controls to get maximum performance of Tiled Based GPUs.
    //! So, assuming the pipeline mentioned above can be merged as subpasses the timeline would look like this.
    //! At time 1, during RPI::RasterPass Initialization:
    //!     Create dummy "VkRenderPass_A" for a shader named "PSO_A".
    //!     Create "PSO_A" using "VkRenderPass_A".
    //! At time 2, during another RPI::RasterPass Initialization:
    //!     Create dummy "VkRenderPass_B" for a shader named "PSO_B".
    //!     Create "PSO_B" using "VkRenderPass_B".
    //! At time 3, in the future, when the RHI builds the Frame Graph:
    //!     Create Scope_A and Scope_B as subpasses with "VkRenderPass_C"
    //! At time 4, when the RHI submits commands
    //!     VkCmdBeginRenderPass (VkRenderPass_C)
    //!         VkCmdBindPipeline(PSO_A that was created with VkRenderPass_A)  // VkRenderPass_A must be "compatible" with VkRenderPass_C.
    //!         VkCmdDraw(...)
    //!     VkCmdNextSubpass(1)
    //!         VkCmdBindPipeline(PSO_B that was created with VkRenderPass_B) // VkRenderPass_B must be "compatible" with VkRenderPass_C
    //!         VkCmdDraw(...)
    //!     VkCmdEndRenderPass (VkRenderPass_C)
    //!
    //! When declaring Subpass Dependencies (Which is required during VkRenderPass creation), it was found that Vulkan is very strict
    //! when validating Compatibility. In the end it boiled down to make sure that all VkRenderPasses must have IDENTICAL Subpass Dependencies
    //! otherwise vulkan would trigger validation errors. And this is why this abstract class was introduced. Because it helps to encapsulate
    //! all the details required for creating VkSubpassDependency (for Vulkan) and share the exact same information when creating all related
    //! VkRenderPasses. 
    class SubpassDependencies
    {
    public:
        AZ_RTTI(SubpassDependencies, "{C1902708-76B9-44E8-A1F5-7B5C876A6C51}");

        SubpassDependencies() = default;
        virtual ~SubpassDependencies() = default;

        //! @returns true if this opaque blob contains valid data.
        //!     Each RHI would have its own custom logic.
        virtual bool IsValid() const = 0;
    };
}
