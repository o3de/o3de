/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/PipelineStateDescriptor.h>
#include <Atom/RHI.Reflect/DX12/ShaderStageFunction.h>

namespace AZ::DX12
{
    namespace ShaderUtils
    {
        //! Patch a shader bytecode with the proper values of the specialization constants found in the
        //! pipeline descriptor.
        ShaderByteCode PatchShaderFunction(
            const ShaderStageFunction& shaderFunction,
            const RHI::PipelineStateDescriptor& descriptor);

        //! Patch a shader bytecode with the proper values of the specialization constants found in the
        //! pipeline descriptor. If the pipeline descriptor is not using specialization constants, it returns the
        //! shader bytecode unchanged. If it needs to patch it, the patched shader bytecode is stored in the provided container.
        //! Refer to RFC (https://github.com/o3de/sig-graphics-audio/blob/main/rfcs/SpecializationConstants/SpecializationConstants.md)
        //! for more details on how specialization constants works on DX12 
        ShaderByteCodeView PatchShaderFunction(
            const ShaderStageFunction& shaderFunction,
            const RHI::PipelineStateDescriptor& descriptor,
            AZStd::vector<ShaderByteCode>& patchedShaderContainer);

        //! Signs a DXIL blob so it can be used by the driver. Only needed if the bytecode has been modified.
        bool SignByteCode(ShaderByteCode& bytecode);
    }
} // namespace AZ
