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
#include <Atom/RHI.Reflect/VariableRateShadingEnums.h>
#include <Atom/RHI/DeviceDrawItem.h>
#include <Atom/RHI/DeviceDispatchItem.h>
#include <Atom/RHI/DeviceDispatchRaysItem.h>
#include <Atom/RHI/DeviceCopyItem.h>
#include <Atom/RHI/DeviceRayTracingAccelerationStructure.h>
#include <Atom/RHI/DeviceRayTracingBufferPools.h>

namespace AZ::RHI
{
    class ScopeProducer;
    class DeviceRayTracingCompactionQuery;

    //! Supported operations for rendering predication.
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

        //! Assigns a shader resource group for draw on the graphics pipe, at the binding slot
        //! determined by the layout used to create the shader resource group.
        //! @param shaderResourceGroup The shader resource group to bind.
        virtual void SetShaderResourceGroupForDraw(const DeviceShaderResourceGroup& shaderResourceGroup) = 0;

        //! Assigns a shader resource group for dispatch on compute pipe, at the binding slot
        //! determined by the layout used to create the shader resource group.
        //! @param shaderResourceGroup The shader resource group to bind.
        virtual void SetShaderResourceGroupForDispatch(const DeviceShaderResourceGroup& shaderResourceGroup) = 0;

        /// Submits a single copy item for processing on the command list.
        virtual void Submit(const DeviceCopyItem& copyItem, uint32_t submitIndex = 0) = 0;

        /// Submits a single draw item for processing on the command list.
        virtual void Submit(const DeviceDrawItem& drawItem, uint32_t submitIndex = 0) = 0;

        /// Submits a single dispatch item for processing on the command list.
        virtual void Submit(const DeviceDispatchItem& dispatchItem, uint32_t submitIndex = 0) = 0;

        /// Submits a single dispatch rays item for processing on the command list.
        virtual void Submit(const DeviceDispatchRaysItem& dispatchRaysItem, uint32_t submitIndex = 0) = 0;

        /// Starts predication on the command list.
        virtual void BeginPredication(const DeviceBuffer& buffer, uint64_t offset, PredicationOp operation) = 0;

        /// End predication on the command list.
        virtual void EndPredication() = 0;

        /// Builds a Bottom Level Acceleration Structure (BLAS) for ray tracing operations, which is made up of DeviceRayTracingGeometry entries
        virtual void BuildBottomLevelAccelerationStructure(const RHI::DeviceRayTracingBlas& rayTracingBlas) = 0;

        /// Updates a Bottom Level Acceleration Structure (BLAS) for ray tracing operations, which is made up of DeviceRayTracingGeometry entries
        virtual void UpdateBottomLevelAccelerationStructure(const RHI::DeviceRayTracingBlas& rayTracingBlas) = 0;

        /// Inserts queries for the size of the compacted Blas
        virtual void QueryBlasCompactionSizes(
            const AZStd::vector<AZStd::pair<RHI::DeviceRayTracingBlas*, RHI::DeviceRayTracingCompactionQuery*>>& blasToQuery) = 0;

        /// Copies the given sourceBlas into the compactBlas
        virtual void CompactBottomLevelAccelerationStructure(const RHI::DeviceRayTracingBlas& sourceBlas, const RHI::DeviceRayTracingBlas& compactBlas) = 0;

        /// Builds a Top Level Acceleration Structure (TLAS) for ray tracing operations, which is made up of RayTracingInstance entries that
        /// refer to a BLAS entry
        virtual void BuildTopLevelAccelerationStructure(
            const RHI::DeviceRayTracingTlas& rayTracingTlas, const AZStd::vector<const RHI::DeviceRayTracingBlas*>& changedBlasList) = 0;

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

        /// Validates a submit index against the range for this command list, and tracks the total number of submits
        void ValidateSubmitIndex(uint32_t submitIndex);

        /// Validates the total number of submits against the expected number
        void ValidateTotalSubmits(const ScopeProducer* scopeProducer);

        /// Resets the total number of submits
        void ResetTotalSubmits() { m_totalSubmits = 0; }

        /// Default value of shading rate combinator operations.
        static const ShadingRateCombinators DefaultShadingRateCombinators;

        /// Sets the Per-Draw shading rate value. This rate will be used for all subsequent draw calls of this command list.
        /// Combinators can also be specified as part of setting the rate. For ShadingRateCombinators = { Op1, Op2 },
        /// the final value is calculated as Op2(Op1(PerDraw, PerPrimitive), PerRegion)
        virtual void SetFragmentShadingRate(
            ShadingRate rate,
            const ShadingRateCombinators& combinators = DefaultShadingRateCombinators) = 0;

    private:
        SubmitRange m_submitRange;
        uint32_t m_totalSubmits = 0;
    };

    inline void CommandList::ValidateSubmitIndex([[maybe_unused]] uint32_t submitIndex)
    {
        if (m_submitRange.GetCount())
        {
            AZ_Assert((submitIndex >= m_submitRange.m_startIndex) && (submitIndex < m_submitRange.m_endIndex),
                "Submit index %d is not in the valid submission range for this CommandList (%d, %d). "
                "Call FrameGraphExecuteContext::GetSubmitRange() to retrieve the range when submitting items to the CommandList.",
                submitIndex,
                m_submitRange.m_startIndex,
                m_submitRange.m_endIndex - 1);

            m_totalSubmits++;
        }
    }
}
