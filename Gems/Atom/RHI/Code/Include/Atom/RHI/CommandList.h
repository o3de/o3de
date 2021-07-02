/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Viewport.h>
#include <Atom/RHI.Reflect/Scissor.h>
#include <Atom/RHI/DrawItem.h>
#include <Atom/RHI/DispatchItem.h>
#include <Atom/RHI/CopyItem.h>
#include <Atom/RHI/RayTracingAccelerationStructure.h>
#include <Atom/RHI/RayTracingBufferPools.h>

namespace AZ
{
    namespace RHI
    {
        class Query;
        struct DispatchRaysItem;

        /**
        * Supported operations for rendering predication.
        */
        enum class PredicationOp : uint32_t
        {
            EqualZero = 0,  ///< Enables predication if predication value is zero.
            NotEqualZero,   ///< Enables predication if predication value is not zero.
            Count
        };

        class CommandList
        {
        public:
            /// Assigns a list of viewports to the raster stage of the graphics pipe.
            virtual void SetViewports(const Viewport* viewports, uint32_t count) = 0;

            /// Assigns a list of scissors to the raster stage of the graphics pipe.
            virtual void SetScissors(const Scissor* scissors, uint32_t count) = 0;

            /// Assigns a scissor to the raster stage of the graphics pipe.
            void SetScissor(const Scissor& scissor)
            {
                SetScissors(&scissor, 1);
            }

            /// Assigns a viewport to the raster stage of the graphics pipe
            void SetViewport(const Viewport& viewport)
            {
                SetViewports(&viewport, 1);
            }

            /**
             * Assigns a shader resource group for draw on the graphics pipe, at the binding slot
             * determined by the layout used to create the shader resource group.
             * @param shaderResourceGroup The shader resource group to bind.
             */
            virtual void SetShaderResourceGroupForDraw(const ShaderResourceGroup& shaderResourceGroup) = 0;

            /**
             * Assigns a shader resource group for dispatch on compute pipe, at the binding slot
             * determined by the layout used to create the shader resource group.
             * @param shaderResourceGroup The shader resource group to bind.
             */
            virtual void SetShaderResourceGroupForDispatch(const ShaderResourceGroup& shaderResourceGroup) = 0;

            /// Submits a single copy item for processing on the command list.
            virtual void Submit(const CopyItem& copyItem) = 0;

            /// Submits a single draw item for processing on the command list.
            virtual void Submit(const DrawItem& drawItem) = 0;

            /// Submits a single dispatch item for processing on the command list.
            virtual void Submit(const DispatchItem& dispatchItem) = 0;

            //! Submits a single dispatch rays item for processing on the command list.
            virtual void Submit(const DispatchRaysItem& dispatchRaysItem) = 0;

            /// Starts predication on the command list.
            virtual void BeginPredication(const Buffer& buffer, uint64_t offset, PredicationOp operation) = 0;

            /// End predication on the command list.
            virtual void EndPredication() = 0;

            //! Builds a Bottom Level Acceleration Structure (BLAS) for ray tracing operations, which is made up of RayTracingGeometry entries
            virtual void BuildBottomLevelAccelerationStructure(const RHI::RayTracingBlas& rayTracingBlas) = 0;

            //! Builds a Top Level Acceleration Structure (TLAS) for ray tracing operations, which is made up of RayTracingInstance entries that refer to a BLAS entry
            virtual void BuildTopLevelAccelerationStructure(const RHI::RayTracingTlas& rayTracingTlas) = 0;
        };
    }
}
