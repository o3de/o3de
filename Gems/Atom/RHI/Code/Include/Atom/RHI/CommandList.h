/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Viewport.h>
#include <Atom/RHI.Reflect/Scissor.h>
#include <Atom/RHI/DrawItem.h>
#include <Atom/RHI/DispatchItem.h>
#include <Atom/RHI/DispatchRaysItem.h>
#include <Atom/RHI/CopyItem.h>
#include <Atom/RHI/RayTracingAccelerationStructure.h>
#include <Atom/RHI/RayTracingBufferPools.h>

namespace AZ
{
    namespace RHI
    {
        class Query;
        class ScopeProducer;

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

            /// Submits a single dispatch rays item for processing on the command list.
            virtual void Submit(const DispatchRaysItem& dispatchRaysItem) = 0;

            /// Starts predication on the command list.
            virtual void BeginPredication(const Buffer& buffer, uint64_t offset, PredicationOp operation) = 0;

            /// End predication on the command list.
            virtual void EndPredication() = 0;

            /// Builds a Bottom Level Acceleration Structure (BLAS) for ray tracing operations, which is made up of RayTracingGeometry entries
            virtual void BuildBottomLevelAccelerationStructure(const RHI::RayTracingBlas& rayTracingBlas) = 0;

            /// Builds a Top Level Acceleration Structure (TLAS) for ray tracing operations, which is made up of RayTracingInstance entries that refer to a BLAS entry
            virtual void BuildTopLevelAccelerationStructure(const RHI::RayTracingTlas& rayTracingTlas) = 0;

            /// Defines the submit range for a CommandList
            /// Note: the default is 0 items, which disables validation for items submitted outside of the framegraph
            struct SubmitRange
            {
                // the zero-based start index of the range
                uint32_t m_startIndex = 0;

                // the end index of the range
                // Note: this is an exclusive index, meaning submitted item indices should be less than this index
                uint32_t m_endIndex = 0;

                // returns the number of items in the range
                uint32_t GetCount() const { return m_endIndex - m_startIndex; }
            };

            /// Sets the submit range for this command list
            void SetSubmitRange(const SubmitRange& submitRange) { m_submitRange = submitRange; }

            /// Validates a submit item against the range for this command list, and tracks the total number of submits
            void ValidateSubmitItem(const SubmitItem& submitItem);

            /// Validates the total number of submits against the expected number
            void ValidateTotalSubmits(const ScopeProducer* scopeProducer);

            /// Resets the total number of submits
            void ResetTotalSubmits() { m_totalSubmits = 0; }

        private:
            SubmitRange m_submitRange;
            uint32_t m_totalSubmits = 0;
        };
    }
}
